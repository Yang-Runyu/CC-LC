#include <iostream>  
#include <assert.h>
#include <fstream>
#include "../../source/commonlib/CodingUnit.h"
#include "../../source/commonlib/Picture.h"
#include "decOptions.h"
#include "../../source/commonlib/utility.h"
#include <time.h>
using namespace std;
#ifdef __unix
 
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename), (mode)))==NULL
 
#endif

void decodeCU(CodingUnit* pcCU, unsigned char* pImg, int max_cu, int min_cu, int index)
{
	//śÁąęÖžÎť
	int loc_row = pcCU->location_row;
	int loc_col = pcCU->location_col;
	int h = pcCU->pic_height;
	int w = pcCU->pic_width;
	int CU_h = pcCU->getHeight();
	int CU_w = pcCU->getWidth();
	int depth = pcCU->getDepth();
	int side_length = max_cu >> (depth + 1);
	if (loc_row + 2 * side_length > h || loc_col + 2 * side_length > w)
	{
		pcCU->split_flag = true;
		if (loc_row + side_length >= h && loc_col + side_length >= w)
		{
			CodingUnit *CU = new CodingUnit; CU->m_context = pcCU->m_context;
			CU->create(pImg, h, w, CU_h, CU_w, loc_row, loc_col, depth + 1, max_cu);
			pcCU->child[0] = CU; CU->father = pcCU;
			decodeCU(pcCU->child[0], pImg, max_cu, min_cu,0);
		}
		else if (loc_row + side_length >= h && loc_col + side_length < w)
		{
			CodingUnit *CU1 = new CodingUnit; CU1->m_context = pcCU->m_context;
			CU1->create(pImg, h, w, CU_h, side_length, loc_row, loc_col, depth + 1, max_cu);
			pcCU->child[0] = CU1; CU1->father = pcCU;
			decodeCU(pcCU->child[0], pImg, max_cu, min_cu,0);
			CodingUnit *CU2 = new CodingUnit; CU2->m_context = pcCU->m_context;
			CU2->create(pImg, h, w, CU_h, CU_w - side_length, loc_row, loc_col + side_length, depth + 1, max_cu);
			pcCU->child[1] = CU2; CU2->father = pcCU;
			decodeCU(pcCU->child[1], pImg, max_cu, min_cu,1);
		}
		else if (loc_row + side_length < h && loc_col + side_length >= w)
		{
			CodingUnit *CU1 = new CodingUnit; CU1->m_context = pcCU->m_context;
			CU1->create(pImg, h, w, side_length, CU_w, loc_row, loc_col, depth + 1, max_cu);
			pcCU->child[0] = CU1; CU1->father = pcCU;
			decodeCU(pcCU->child[0], pImg, max_cu, min_cu,0);
			CodingUnit *CU2 = new CodingUnit; CU2->m_context = pcCU->m_context;
			CU2->create(pImg, h, w, CU_h - side_length, CU_w, loc_row + side_length, loc_col, depth + 1, max_cu);
			pcCU->child[2] = CU2; CU2->father = pcCU;
			decodeCU(pcCU->child[2], pImg, max_cu, min_cu,2);
		}
		else
		{
			CodingUnit *CU1 = new CodingUnit; CU1->m_context = pcCU->m_context;
			CU1->create(pImg, h, w, side_length, side_length, loc_row, loc_col, depth + 1, max_cu);
			pcCU->child[0] = CU1; CU1->father = pcCU;
			decodeCU(pcCU->child[0], pImg, max_cu, min_cu,0);
			CodingUnit *CU2 = new CodingUnit; CU2->m_context = pcCU->m_context;
			CU2->create(pImg, h, w, side_length, CU_w - side_length, loc_row, loc_col + side_length, depth + 1, max_cu);
			pcCU->child[1] = CU2; CU2->father = pcCU;
			decodeCU(pcCU->child[1], pImg, max_cu, min_cu,1);
			CodingUnit *CU3 = new CodingUnit; CU3->m_context = pcCU->m_context;
			CU3->create(pImg, h, w, CU_h - side_length, side_length, loc_row + side_length, loc_col, depth + 1, max_cu);
			pcCU->child[2] = CU3; CU3->father = pcCU;
			decodeCU(pcCU->child[2], pImg, max_cu, min_cu,2);
			CodingUnit *CU4 = new CodingUnit; CU4->m_context = pcCU->m_context;
			CU4->create(pImg, h, w, CU_h - side_length, CU_w - side_length, loc_row + side_length, loc_col + side_length, depth + 1, max_cu);
			pcCU->child[3] = CU4; CU4->father = pcCU;
			decodeCU(pcCU->child[3], pImg, max_cu, min_cu,3);
		}
	}
	else if (CU_h == min_cu && CU_w == min_cu)
	{
		int flag = pcCU->deFlag(index);
		switch (flag)
		{
		case 0:pcCU->oneColor = 1; pcCU->contiEdge = 0; pcCU->deOneColor(pImg); break;
		case 1:pcCU->oneColor = 0; pcCU->contiEdge = 1; pcCU->deChainCoding(pImg); break;
		case 2:pcCU->oneColor = 0; pcCU->contiEdge = 0; pcCU->deRunLength(pImg); 
		}
	}
	else
	{
		if (!pcCU->deSplitFlag())
		{
			pcCU->oneColor = pcCU->deOneColorFlag(index);
			if (pcCU->oneColor)
			{
				pcCU->deOneColor(pImg);
				pcCU->contiEdge = 0;
			}
			else
			{
				pcCU->contiEdge = 1;
                pcCU->deChainCoding(pImg);
			}
		}
		else
		{
			pcCU->split_flag = true;
			int h = pcCU->pic_height;
			int w = pcCU->pic_width;
			int CU_h = pcCU->getHeight();
			int CU_w = pcCU->getWidth();
			int m_depth = pcCU->getDepth();
			int loc_row = pcCU->location_row;
			int loc_col = pcCU->location_col;
			CodingUnit *CU1 = new CodingUnit; CU1->m_context = pcCU->m_context;
			CU1->create(pImg, h, w, CU_h / 2, CU_w / 2, loc_row, loc_col, m_depth + 1, max_cu);
			pcCU->child[0] = CU1; CU1->father = pcCU;
			CodingUnit *CU2 = new CodingUnit; CU2->m_context = pcCU->m_context;
			CU2->create(pImg, h, w, CU_h / 2, CU_w / 2, loc_row, loc_col + CU_w / 2, m_depth + 1, max_cu);
			pcCU->child[1] = CU2; CU2->father = pcCU;
			CodingUnit *CU3 = new CodingUnit; CU3->m_context = pcCU->m_context;
			CU3->create(pImg, h, w, CU_h / 2, CU_w / 2, loc_row + CU_h / 2, loc_col, m_depth + 1, max_cu);
			pcCU->child[2] = CU3; CU3->father = pcCU;
			CodingUnit *CU4 = new CodingUnit; CU4->m_context = pcCU->m_context;
			CU4->create(pImg, h, w, CU_h / 2, CU_w / 2, loc_row + CU_h / 2, loc_col + CU_w / 2, m_depth + 1, max_cu);
			pcCU->child[3] = CU4; CU4->father = pcCU;
			decodeCU(pcCU->child[0], pImg, max_cu, min_cu,0);
			decodeCU(pcCU->child[1], pImg, max_cu, min_cu,1);
			decodeCU(pcCU->child[2], pImg, max_cu, min_cu,2);
			decodeCU(pcCU->child[3], pImg, max_cu, min_cu,3);
		}
	}
}


void decodePicture(Picture* pcPic,unsigned char* pImg, int max_cu, int min_cu)
{
	int total_num_ctu = pcPic->m_numCtuInRow * pcPic->m_numCtuInCol;
	for (int ctuIdx = 0; ctuIdx < total_num_ctu; ctuIdx++)
	{
		PCM_flag = 0;
		encode_numC = false;
		CodingUnit* pcCtu = pcPic->getCtu(ctuIdx);
		if (ctuIdx == 0)
		{
			pcCtu->m_context[2076] = -256; pcCtu->m_context[2077] = 256;
			pcCtu->m_context[2072] = g_value; pcCtu->m_context[2073] = g_value; 
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
						if (depthMap[pcPic->m_width * (g_max_CU*(ctuIdx / pcPic->m_numCtuInCol) - 1) + (ctuIdx % pcPic->m_numCtuInCol) * g_max_CU + i * 8] < 3) { splitCTU_context = 0; break; }
					}
				}
			}
		}

		if (splitCTU_context) splitCTU_flag = acodec.decode(aSplitCTU1); 
		else splitCTU_flag = acodec.decode(aSplitCTU2);

		decodeCU(pcCtu, pImg, max_cu, min_cu, 0);

		if (!encode_numC) for (int i = 0; i < 520 * 4 - 8; i++) pcCtu->m_context[i] = 1;

		if (!PCM_flag)
		{
			pcCtu->m_context[2074] = 0; pcCtu->m_context[2075] = 255;
			for (int i = 0; i < pcCtu->getHeight(); i++)
			{
				for (int j = 0; j < pcCtu->getWidth(); j++)
				{
					if (pImg[(i + pcCtu->location_row)* pcCtu->pic_width + j + pcCtu->location_col] > pcCtu->m_context[2074]) pcCtu->m_context[2074] = pImg[(i + pcCtu->location_row)* pcCtu->pic_width + j + pcCtu->location_col];
					if (pImg[(i + pcCtu->location_row)* pcCtu->pic_width + j + pcCtu->location_col] < pcCtu->m_context[2075]) pcCtu->m_context[2075] = pImg[(i + pcCtu->location_row)* pcCtu->pic_width + j + pcCtu->location_col];
				}
			}
		}

	}
}

int main(int argc, char** argv)
{
	int begin, end;
	begin = clock();

	char filein[1000];
	char fileout[1000];
	readOptions(argc, argv, filein, fileout);
	FILE * pFile;
	int size;
	fopen_s(&pFile, filein, "rb");
	size = getFSize(pFile);
	unsigned char* buffer = new unsigned char[size];
	fread(buffer, 1, size, pFile);
	fclose(pFile);

	acodec.set_buffer(10000000, buffer);
	acodec.start_decoder();

	int pic_height = readByArithmetic(&acodec, &ahead, 16);
	int pic_width = readByArithmetic(&acodec, &ahead, 16);
    int frameNum = readByArithmetic(&acodec, &ahead, 16);
    //int type = acodec.decode(ahead);
	int type = 0;
    depthMap = new int[pic_height*pic_width];

    unsigned char* pic = new unsigned char[(pic_height*pic_width*frameNum*(type+2))>>1];
    for (int i = 0; i < (pic_height*pic_width*frameNum*(type+2))>>1; i++) pic[i] = 0;
   
	Picture* pcPic = new Picture;
    unsigned char* slice = pic;
    for (int i = 0; i < frameNum; i++)
    {
		reset_arithmetic();
		bool oneColor = acodec.decode(ahead);
		g_value = decGolomb(1) + 128;
		if (oneColor)
		{
			for (int i = 0; i < pic_height*pic_width; i++)
			{
				slice[i] = g_value;
			}
		}
		else
		{
			pcPic->createPicture(slice, pic_height, pic_width, g_max_CU, g_min_CU);
			decodePicture(pcPic, slice, g_max_CU, g_min_CU);
			//pcPic->destroyPicture();
		}
	    slice += (pic_height*pic_width*(type + 2)) >> 1;
		//printf("%d", i);
    }

	acodec.stop_decoder();
    end = clock();
    cout << "height: " << pic_height << endl;
    cout << "width: " << pic_width << endl;
    cout << "time: " << (end - begin) / 1000.0 << " sec" << endl;

    write2bin(pic, pic_width*(pic_height*frameNum*(type+2))>>1, fileout);

	//delete[] buffer;
	//delete[]depthMap;
	//delete[]colorList;
	return 0;
}
