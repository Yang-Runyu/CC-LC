#include <iostream>  
#include <assert.h>
#include <fstream>
#include <iomanip>
#include "../../source/commonlib/global_arithmetic.h"
#include "../../source/commonlib/Picture.h"
#include "../../source/commonlib/CodingUnit.h"
#include "../../source/commonlib/utility.h"
#include "encOptions.h"
#include <time.h>
using namespace std;

#ifdef __unix
 
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename), (mode)))==NULL
 
#endif

void compressCU(CodingUnit* pcCU)
{
	int height = pcCU->getHeight();
	int width = pcCU->getWidth();
	unsigned char* value = pcCU->getValue();
	unsigned char* img = pcCU->getImg();
	int depth = pcCU->getDepth();
	int pic_width = pcCU->pic_width;
	int pic_height = pcCU->pic_height;
	int loc_row = pcCU->location_row;
	int loc_col = pcCU->location_col;

	/*is bound -- split*/
	int side_length = g_max_CU / (2 << depth);
	if (side_length == g_min_CU / 2)    //height or width can't be divided by g_min_CU -- error;
	{
		assert(height == g_min_CU && width == g_min_CU);
	}
	else if(height != 2 * side_length || width != 2 * side_length)   //out of bound
	{
		if (side_length >= height && side_length >= width)
		{
			pcCU->split_flag = true;
			CodingUnit *CU = new CodingUnit; CU->m_context = pcCU->m_context;
			CU->create(img, pic_height, pic_width, height, width, loc_row, loc_col, depth + 1);
			pcCU->child[0] = CU; CU->father = pcCU;
			compressCU(CU);
			return;
		}
		else if (side_length >= height && side_length < width)
		{
			pcCU->split_flag = true;
			CodingUnit *CU1 = new CodingUnit; CU1->m_context = pcCU->m_context;
			CU1->create(img, pic_height, pic_width, height, side_length, loc_row, loc_col, depth + 1);
			pcCU->child[0] = CU1; CU1->father = pcCU;
			compressCU(CU1);
			CodingUnit *CU2 = new CodingUnit; CU2->m_context = pcCU->m_context;
			CU2->create(img, pic_height, pic_width, height, width - side_length, loc_row, loc_col + side_length, depth + 1);
			pcCU->child[1] = CU2; CU2->father = pcCU;
			compressCU(CU2);
			return;
		}
		else if (side_length < height && side_length >= width)
		{
			pcCU->split_flag = true;
			CodingUnit *CU1 = new CodingUnit; CU1->m_context = pcCU->m_context;
			CU1->create(img, pic_height, pic_width, side_length, width, loc_row, loc_col, depth + 1);
			pcCU->child[0] = CU1; CU1->father = pcCU;
			compressCU(CU1);
			CodingUnit *CU2 = new CodingUnit; CU2->m_context = pcCU->m_context;
			CU2->create(img, pic_height, pic_width, height - side_length, width, loc_row + side_length, loc_col, depth + 1);
			pcCU->child[2] = CU2; CU2->father = pcCU;
			compressCU(CU2);
			return;
		}
		else
		{
			pcCU->split_flag = true;
			CodingUnit *CU1 = new CodingUnit; CU1->m_context = pcCU->m_context;
			CU1->create(img, pic_height, pic_width, side_length, side_length, loc_row, loc_col, depth + 1);
			pcCU->child[0] = CU1; CU1->father = pcCU;
			compressCU(CU1);
			CodingUnit *CU2 = new CodingUnit; CU2->m_context = pcCU->m_context;
			CU2->create(img, pic_height, pic_width, side_length, width - side_length, loc_row, loc_col + side_length, depth + 1);
			pcCU->child[1] = CU2; CU2->father = pcCU;
			compressCU(CU2);
			CodingUnit *CU3 = new CodingUnit; CU3->m_context = pcCU->m_context;
			CU3->create(img, pic_height, pic_width, height - side_length, side_length, loc_row + side_length, loc_col, depth + 1);
			pcCU->child[2] = CU3; CU3->father = pcCU;
			compressCU(CU3);
			CodingUnit *CU4 = new CodingUnit; CU4->m_context = pcCU->m_context;
			CU4->create(img, pic_height, pic_width, height - side_length, width - side_length, loc_row + side_length, loc_col + side_length, depth + 1);
			pcCU->child[3] = CU4; CU4->father = pcCU;
			compressCU(CU4);
			return;
		}
	}
	/*coding or split*/

	bool OneColor = isOneColor(value, height, width);
	bool ContiEdge = false;
	bool Inter = false;
	if (!OneColor)
	{
		ContiEdge = isContiEdge(value, height, width, width);
		if (ContiEdge) pcCU->setEdge();
	}

	/*coding or split*/
	if (OneColor)
	{
		// encode one_color_flag
		pcCU->split_flag = false;
		pcCU->oneColor = true;
		pcCU->setColorValue(value[0]);
		return;
	}
	/*else if the current CU has two colors and only one continous edge*/
	else if (ContiEdge)
	{
		//encode the edge with chain code
		pcCU->split_flag = false;
		pcCU->contiEdge = true;
		return;
	}
	else if (side_length == g_min_CU / 2)
	{
		//encode the edge with runlength
		pcCU->setRunLength();
		return;
	}
	else
	{
		/*else quad-tree partitioning*/
		pcCU->split_flag = true;
		CodingUnit *CU1 = new CodingUnit; CU1->m_context = pcCU->m_context;
		CU1->create(img, pic_height, pic_width, side_length, side_length, loc_row, loc_col, depth + 1);
		pcCU->child[0] = CU1; CU1->father = pcCU;
		compressCU(CU1);
		CodingUnit *CU2 = new CodingUnit; CU2->m_context = pcCU->m_context;
		CU2->create(img, pic_height, pic_width, side_length, side_length, loc_row, loc_col + side_length, depth + 1);
		pcCU->child[1] = CU2; CU2->father = pcCU;
		compressCU(CU2);
		CodingUnit *CU3 = new CodingUnit; CU3->m_context = pcCU->m_context;
		CU3->create(img, pic_height, pic_width, side_length, side_length, loc_row + side_length, loc_col, depth + 1);
		pcCU->child[2] = CU3; CU3->father = pcCU;
		compressCU(CU3);
		CodingUnit *CU4 = new CodingUnit; CU4->m_context = pcCU->m_context;
		CU4->create(img, pic_height, pic_width, side_length, side_length, loc_row + side_length, loc_col + side_length, depth + 1);
		pcCU->child[3] = CU4; CU4->father = pcCU;
		compressCU(CU4);
		return;
	}
}

void encodeCU(CodingUnit* pcCU, int index)
{
	// encode split_flag
	int depth = pcCU->getDepth();
	if (pcCU->split_flag)
	{
		if (pcCU->getHeight() == (g_max_CU >> depth) && pcCU->getWidth() == (g_max_CU >> depth))
		{
			pcCU->enSplitFlag();               //if the CU is not the minimum size CU, encode split_flag.
		}
		for (int childIndex = 0; childIndex < 4; childIndex++)
			if (NULL != pcCU->child[childIndex]) encodeCU(pcCU->child[childIndex], childIndex);
	}
	else
	{
		if (pcCU->getWidth() != g_min_CU || pcCU->getHeight() != g_min_CU)
		{
			pcCU->enSplitFlag();               //if the CU is not the minimum size CU, encode split_flag.
		}

		if (pcCU->getWidth() == g_min_CU && pcCU->getHeight() == g_min_CU)
		{
			pcCU->enFlag(index);
			if (pcCU->oneColor) pcCU->enOneColor();
			else if (pcCU->contiEdge) pcCU->enChainCoding();
			else pcCU->enRunLength();
		}
		else
		{
			pcCU->enOneColorFlag(index);
			if (pcCU->oneColor)	pcCU->enOneColor();
			else if (pcCU->contiEdge) pcCU->enChainCoding();
		}
	}
}


void compressPicture(Picture* pcPic)
{
	int total_num_ctu = pcPic->m_numCtuInRow * pcPic->m_numCtuInCol;

	for (int ctuIdx = 0; ctuIdx < total_num_ctu; ctuIdx++)      //compress one CTU by one CTU, can precess in parallel.
	{
		CodingUnit* pcCtu = pcPic->getCtu(ctuIdx);
		compressCU(pcCtu);
		if (pcCtu->m_context[2078] != -256)
		{
			if (abs(pcCtu->m_context[2078]) > abs(pcCtu->m_context[2079]))
			{
				pcCtu->m_context[2078] = abs(pcCtu->m_context[2078]);
				pcCtu->m_context[2079] = -pcCtu->m_context[2078];
			}
			else
			{
				pcCtu->m_context[2078] = abs(pcCtu->m_context[2079]);
				pcCtu->m_context[2079] = -abs(pcCtu->m_context[2079]);
			}
		}

		pcCtu->m_context[2074] = 0; pcCtu->m_context[2075] = 255;
		unsigned char* value = pcCtu->getValue();
		for (int i = 0; i < pcCtu->getWidth() * pcCtu->getHeight(); i++)
		{
			if (value[i] > pcCtu->m_context[2074]) pcCtu->m_context[2074] = value[i];
			if (value[i] < pcCtu->m_context[2075]) pcCtu->m_context[2075] = value[i];
		}

	}
}

void encodePicture(Picture* pcPic)
{
	int total_num_ctu = pcPic->m_numCtuInRow * pcPic->m_numCtuInCol;
	for (int ctuIdx = 0; ctuIdx < total_num_ctu; ctuIdx++)
	{
		encode_numC = false;
		CodingUnit* pcCtu = pcPic->getCtu(ctuIdx);
		if (ctuIdx == 0)
		{
			pcCtu->m_context[2076] = -256; pcCtu->m_context[2077] = 256; 
			if (pcCtu->m_context[2078] != -256) { pcCtu->m_context[2072] = g_value + pcCtu->m_context[2078]; pcCtu->m_context[2073] = g_value + pcCtu->m_context[2079];}
			else{ pcCtu->m_context[2072] = g_value;pcCtu->m_context[2073] = g_value; }
			splitCTU_context = 0;
		}
		else
		{
			if (ctuIdx < pcPic->m_numCtuInCol)
			{
				for (int i = 0; i < 520 * 4 - 8; i++) pcCtu->m_context[i] = pcPic->getCtu(ctuIdx - 1)->m_context[i];
				copy_arithmetic(pcCtu->context_element);
				pcCtu->m_context[2076] = pcPic->getCtu(ctuIdx - 1)->m_context[2078]; pcCtu->m_context[2077] = pcPic->getCtu(ctuIdx - 1)->m_context[2079];
				pcCtu->m_context[2072] = pcPic->getCtu(ctuIdx - 1)->m_context[2074]; pcCtu->m_context[2073] = pcPic->getCtu(ctuIdx - 1)->m_context[2075];

				splitCTU_context = 1;
				int CTU_height = pcPic->m_height / 8;
				if (CTU_height > 4) CTU_height = 4;
				for (int i = 0; i < CTU_height; i++)
				{
					if (depthMap[pcPic->m_width * 8 * i + ctuIdx * g_max_CU - 1] < 3) { splitCTU_context = 0; break; }
				}
				
			}
			else if (ctuIdx % pcPic->m_numCtuInCol == 0)
			{
				for (int i = 0; i < 520 * 4 - 8; i++) pcCtu->m_context[i] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[i];
				copy_arithmetic(pcCtu->context_element);
				pcCtu->m_context[2076] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2078]; pcCtu->m_context[2077] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2079];
				pcCtu->m_context[2072] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2074]; pcCtu->m_context[2073] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2075];

				splitCTU_context = 1;
				int CTU_width = pcPic->m_width / 8;
				if (CTU_width > 4) CTU_width = 4;
				for (int i = 0; i < CTU_width; i++)
				{
					if (depthMap[pcPic->m_width * (g_max_CU*(ctuIdx / pcPic->m_numCtuInCol) - 1) + i * 8] < 3) { splitCTU_context = 0; break; }
				}
			}
			else
			{
				for (int i = 0; i < 520 * 4 - 8; i++) pcCtu->m_context[i] = pcPic->getCtu(ctuIdx - 1)->m_context[i] + pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[i] - 1;
				copy_arithmetic(pcCtu->context_element);
				if (pcPic->getCtu(ctuIdx - 1)->m_context[2078] > pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2078]) pcCtu->m_context[2076] = pcPic->getCtu(ctuIdx - 1)->m_context[2078];
				else pcCtu->m_context[2076] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2078];
				if (pcPic->getCtu(ctuIdx - 1)->m_context[2079] < pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2079]) pcCtu->m_context[2077] = pcPic->getCtu(ctuIdx - 1)->m_context[2079];
				else pcCtu->m_context[2077] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2079];
				if (pcPic->getCtu(ctuIdx - 1)->m_context[2074] > pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2074]) pcCtu->m_context[2072] = pcPic->getCtu(ctuIdx - 1)->m_context[2074];
				else pcCtu->m_context[2072] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2074];
				if (pcPic->getCtu(ctuIdx - 1)->m_context[2075] < pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2075]) pcCtu->m_context[2073] = pcPic->getCtu(ctuIdx - 1)->m_context[2075];
				else pcCtu->m_context[2073] = pcPic->getCtu(ctuIdx - pcPic->m_numCtuInCol)->m_context[2075];

				splitCTU_context = 1;
				int CTU_height = (pcPic->m_height - g_max_CU * (ctuIdx / pcPic->m_numCtuInCol)) / 8;
				if (CTU_height > 4) CTU_height = 4;
				for (int i = 0; i < CTU_height; i++)
				{
					if (depthMap[pcPic->m_width * (8 * i + g_max_CU * (ctuIdx / pcPic->m_numCtuInCol)) + (ctuIdx % pcPic->m_numCtuInCol) * g_max_CU - 1] < 3) { splitCTU_context = 0; break; }
				}
				int CTU_width = (pcPic->m_width - g_max_CU * (ctuIdx % pcPic->m_numCtuInCol)) / 8;
				if (CTU_width > 4) CTU_width = 4;
				if (splitCTU_context)
				{
					for (int i = 0; i < CTU_width; i++)
					{
						if (depthMap[pcPic->m_width * (g_max_CU*(ctuIdx / pcPic->m_numCtuInCol) - 1) + (ctuIdx % pcPic->m_numCtuInCol) * g_max_CU +  i * 8] < 3) { splitCTU_context = 0; break; }
					}
				}
			}
		}

		splitCTU_flag = 1;
		int CTU_height = (pcPic->m_height - g_max_CU * (ctuIdx / pcPic->m_numCtuInCol)) / 8;
		if (CTU_height > 4) CTU_height = 4;
		int CTU_width = (pcPic->m_width - g_max_CU * (ctuIdx % pcPic->m_numCtuInCol)) / 8;
		if (CTU_width > 4) CTU_width = 4;
		for (int i = 0; i < CTU_height; i++)
		{
			for (int j = 0; j < CTU_width; j++)
			{
				if (depthMap[pcPic->m_width * (g_max_CU*(ctuIdx / pcPic->m_numCtuInCol) + 8*i) + (ctuIdx % pcPic->m_numCtuInCol) * g_max_CU + j * 8] < 3) { splitCTU_flag = 0; break; }
			}
		}

		if(splitCTU_context) acodec.encode(splitCTU_flag, aSplitCTU1);
		else acodec.encode(splitCTU_flag, aSplitCTU2);

		PCM_flag = 0;
		encodeCU(pcCtu, 0);

		if(!encode_numC) for (int i = 0; i < 520 * 4 - 8; i++) pcCtu->m_context[i] = 1;
		if (mask_flag) draw_mask(mask, pcPic->m_width, pcCtu, ctuIdx);
	}
}

void encodeHead(unsigned char* buffer, int rows, int cols, int frameNum, int type, char* file_name, unsigned char* encodeStream)
{
	acodec.set_buffer(10000000, encodeStream);
	acodec.start_encoder();

	writeByArithmetic(&acodec, &ahead, rows, 16);
	writeByArithmetic(&acodec, &ahead, cols, 16);
    writeByArithmetic(&acodec, &ahead, frameNum, 16);
  //if (type == 400)
  //{
  //  acodec.encode(0, ahead);
  //}
  //else
  //{
  //  acodec.encode(1, ahead);
  //}
}

void encodeSliceHead(unsigned char* buffer, int rows, int cols)
{
	g_value = buffer[0];
	bool oneColor = true;
	for (int i = 0; i < rows*cols; i++)
	{
		if (buffer[i] != g_value) { oneColor = false; break; }
	}
	if (oneColor) numC = 1;
	else numC = 2;
	acodec.encode(oneColor, ahead);
	encGolomb(g_value - 128, 1);
}

void removeOutlier(unsigned char* buffer, int rows, int cols)
{
	if (buffer[0] != buffer[1] && buffer[1] == buffer[cols] && buffer[1] == buffer[cols + 1] && abs(int(buffer[0]) - int(buffer[1])) == 1 && buffer[1] == adapt_point[0]) buffer[0] = buffer[1];
	if (buffer[cols - 1] != buffer[cols - 2] && buffer[cols - 2] == buffer[2 * cols - 1] && buffer[cols - 2] == buffer[2 * cols - 2] && abs(int(buffer[cols - 1]) - int(buffer[cols - 2])) == 1 && buffer[cols - 2] == adapt_point[cols - 1]) buffer[cols - 1] = buffer[cols - 2];
	if (buffer[cols*(rows - 1)] != buffer[cols*(rows - 1) + 1] && buffer[cols*(rows - 1) + 1] == buffer[cols*(rows - 2)] && buffer[cols*(rows - 1) + 1] == buffer[cols*(rows - 2) + 1] && abs(int(buffer[cols*(rows - 1)]) - int(buffer[cols*(rows - 1) + 1])) == 1 && buffer[cols*(rows - 1) + 1] == adapt_point[cols*(rows - 1)]) buffer[cols*(rows - 1)] = buffer[cols*(rows - 1) + 1];
	if (buffer[cols*rows - 1] != buffer[cols*rows - 2] && buffer[cols*rows - 2] == buffer[cols*(rows - 1) - 1] && buffer[cols*rows - 2] == buffer[cols*(rows - 1) - 2] && abs(int(buffer[cols*rows - 1]) - int(buffer[cols*rows - 2])) == 1 && buffer[cols*rows - 2] == adapt_point[cols*rows - 1]) buffer[cols*rows - 1] = buffer[cols*rows - 2];
	for (int j = 1; j < cols - 1; j++)
	{
		if (buffer[j] != buffer[j - 1] && buffer[j - 1] == buffer[j + 1] && buffer[j - 1] == buffer[cols + j] && buffer[j - 1] == buffer[cols + j - 1] && buffer[j - 1] == buffer[cols + j + 1] && abs(int(buffer[j]) - int(buffer[j - 1])) == 1 && buffer[j - 1] == adapt_point[j]) buffer[j] = buffer[j - 1];
		if (buffer[(rows - 1)*cols + j] != buffer[(rows - 1)*cols + j - 1] && buffer[(rows - 1)*cols + j - 1] == buffer[(rows - 1)*cols + j + 1] && buffer[(rows - 1)*cols + j - 1] == buffer[(rows - 2)*cols + j + 1] && buffer[(rows - 1)*cols + j - 1] == buffer[(rows - 2)*cols + j - 1] && buffer[(rows - 1)*cols + j - 1] == buffer[(rows - 2)*cols + j] && abs(int(buffer[(rows - 1)*cols + j]) - int(buffer[(rows - 1)*cols + j - 1])) == 1 && buffer[(rows - 1)*cols + j - 1] == adapt_point[(rows - 1)*cols + j]) buffer[(rows - 1)*cols + j] = buffer[(rows - 1)*cols + j - 1];
	}
	for (int i = 1; i < rows - 1; i++)
	{
		if (buffer[i*cols] != buffer[i*cols + 1] && buffer[i*cols + 1] == buffer[(i + 1)*cols + 1] && buffer[i*cols + 1] == buffer[(i - 1)*cols + 1] && buffer[i*cols + 1] == buffer[(i - 1)*cols] && buffer[i*cols + 1] == buffer[(i + 1)*cols] && abs(int(buffer[i*cols]) - int(buffer[i*cols + 1])) == 1 && buffer[i*cols + 1] == adapt_point[i*cols]) buffer[i*cols] = buffer[i*cols + 1];
		if (buffer[(i + 1)*cols - 1] != buffer[(i + 1)*cols - 2] && buffer[(i + 1)*cols - 2] == buffer[i*cols - 2] && buffer[(i + 1)*cols - 2] == buffer[(i + 2)*cols - 2] && buffer[(i + 1)*cols - 2] == buffer[i*cols - 1] && buffer[(i + 1)*cols - 2] == buffer[(i + 2)*cols - 1] && abs(int(buffer[(i + 1)*cols - 1]) - int(buffer[(i + 1)*cols - 2])) == 1 && buffer[(i + 1)*cols - 2] == adapt_point[(i + 1)*cols - 1]) buffer[(i + 1)*cols - 1] = buffer[(i + 1)*cols - 2];
	}

	for (int i = 1; i < rows - 1; i++)
	{
		for (int j = 1; j < cols - 1; j++)
		{
			if (buffer[i*cols + j] != buffer[i*cols + j - 1])
			{
				if (buffer[i*cols + j - 1] == buffer[i*cols + j + 1] && buffer[(i + 1)*cols + j] == buffer[(i - 1)*cols + j] && buffer[i*cols + j - 1] == buffer[(i + 1)*cols + j] && buffer[i*cols + j - 1] == buffer[(i + 1)*cols + j - 1] && buffer[i*cols + j - 1] == buffer[(i + 1)*cols + j + 1] && buffer[i*cols + j - 1] == buffer[(i - 1)*cols + j + 1] && buffer[i*cols + j - 1] == buffer[(i - 1)*cols + j - 1] && abs(int(buffer[i*cols + j]) - int(buffer[i*cols + j - 1])) == 1 && buffer[i*cols + j - 1] == adapt_point[i*cols + j])
					buffer[i*cols + j] = buffer[i*cols + j - 1];
			}
		}
	}
}

int main(int argc, char** argv)
{
	int begin, end;
	begin = clock();
	int rows;
	int cols;
    int frameNum = 1;
    int skip = 0;
    int remove_out = 0;
	g_adapt = 0;
    int type = 400;
	char filein[1000];
	char fileout[1000];
	char fileada[1000];
	unsigned char* mask_point = NULL;
	int error = readOptions(argc, argv, filein, fileout, &rows, &cols, &frameNum, &skip, &type, &mask_flag, &remove_out, &g_adapt, fileada);
	if (error < 0) { getchar(); return 0; }
	int size;
	int real_size;
	int skip_size;
	FILE * pFile;
	fopen_s(&pFile, filein, "rb");
	size = getFSize(pFile);
	if (type == 400)
	{
		real_size = rows * cols * frameNum;
		skip_size = rows * cols * skip;
		assert(size >= real_size + skip_size);
	}
	if (type == 420)
	{
		real_size = ((rows*cols* frameNum * 3) >> 1);
		skip_size = ((rows*cols* skip * 3) >> 1);
		assert(size >= real_size + skip_size);
	}
	unsigned char* buffer = new unsigned char[real_size];
	fseek(pFile, skip_size, SEEK_SET);
	fread(buffer, 1, real_size, pFile);

	if (g_adapt)
	{
		FILE * pFileada;
		fopen_s(&pFileada, fileada, "rb");
		adapt_map = new unsigned char[real_size];
		fseek(pFileada, skip_size, SEEK_SET);
		fread(adapt_map, 1, real_size, pFileada);
		adapt_point = adapt_map;
		fclose(pFileada);
	}

	if (mask_flag)
	{
		mask = new unsigned char[real_size];
		fseek(pFile, 0, SEEK_SET);
		fread(mask, 1, real_size, pFile);
		mask_point = mask;
	}
	fclose(pFile);
  unsigned char* graph = buffer;
  unsigned char* encodeStream = new unsigned char[10000000];
	for (int i = 0; i < 10000000; i++)
		encodeStream[i] = 0;

  depthMap = new int[rows*cols];

  encodeHead(buffer, rows, cols, frameNum, type, fileout, encodeStream);

  //bytes = acodec.getByteNum();
  //cout << bytes << endl;

  Picture* pcPic = new Picture;
  for (int i = 0; i < frameNum; i++)
  {
      reset_arithmetic();
	  if (remove_out) removeOutlier(graph, rows, cols);
      pcPic->createPicture(graph, rows, cols);
      compressPicture(pcPic);
	  if (g_adapt)
	  {
		  //pcPic->destroyPicture();
		  pcPic->createPicture(graph, rows, cols);
		  compressPicture(pcPic);
	  }
	  encodeSliceHead(graph, rows, cols);
	  if (numC > 1)
	  {
          encodePicture(pcPic);
          //pcPic->destroyPicture();
	  }

	  //cout << "aPredictColorRunFlag: " << aPredictColorRunFlag.get_count() << "  ";  aPredictColorRunFlag.printPro();
	  //cout << numC << endl;

	  if (type == 400)
	  {
		  graph += rows * cols;
		  if (mask_flag) mask += rows * cols;
		  if(g_adapt) adapt_point += rows * cols;
	  }
	  if (type == 420)
	  {
		  graph += ((rows*cols * 3) >> 1);
		  if (mask_flag) mask += ((rows*cols * 3) >> 1);
		  if (g_adapt) adapt_point += ((rows*cols * 3) >> 1);
	  }
  }

  int bytenum = acodec.stop_encoder();
	write2bin(encodeStream, bytenum, fileout);

	//write2bin(buffer, rows*cols*frameNum, "9_4_rec.yuv");

	cout << "byteNum: " << bytenum << endl;
	end = clock();
	cout << "time: " << (end - begin) / 1000.0 << " sec" << endl;
	//cout << g_count << endl;
	if (mask_flag)
	{
		write2bin(mask_point, real_size, "mask.yuv");
		delete[]mask;
	}
	/*
	delete[]adapt_map;
	delete[]encodeStream;
	delete[]buffer;
  delete[]depthMap;
  delete[]cMap;
  if (numC > 1)   delete[]colorList;*/

	return 0;
}
