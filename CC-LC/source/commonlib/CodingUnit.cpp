#include "global_arithmetic.h"
#include "CodingUnit.h"
#include "utility.h"
#include <iostream> 
#include <assert.h>
#include <cmath>

using namespace std;

CodingUnit::CodingUnit(void)
{

}

CodingUnit::~CodingUnit()
{

}
void CodingUnit::destroy(CodingUnit* CU)
{
	for (int childIndex = 0; childIndex < 4; childIndex++)
		if (CU->child[childIndex] != NULL) CU->child[childIndex]->destroy(CU->child[childIndex]);
	if (CU->m_value != NULL) { delete[]CU->m_value; CU->m_value = NULL; }
	if (CU->Edge != NULL) { delete[]CU->Edge; CU->Edge = NULL; }
	if (CU->ForbiddenLine != NULL) { delete[]CU->ForbiddenLine; CU->ForbiddenLine = NULL; }
	if (CU->runLength != NULL) { delete[]CU->runLength; CU->runLength = NULL; }
	for (int childIndex = 0; childIndex < 4; childIndex++)
		CU->child[childIndex] = NULL;
	father = NULL;

	if (CU->m_depth == 0) { delete[] CU->m_context; CU->m_context = NULL; }
}

void CodingUnit::create(unsigned char* pImg, int h, int w, int CU_h, int CU_w, int loc_row, int loc_col, int dep)
{
	m_height = CU_h;
	m_width = CU_w;
	m_depth = dep;
	location_row = loc_row;
	location_col = loc_col;
	pic_width = w;
	pic_height = h;
	context = 0;
	m_value = new unsigned char[CU_h*CU_w];
	for (int i = 0; i < CU_h; i++)
	{
		for (int j = 0; j < CU_w; j++)
		{
			m_value[i * CU_w + j] = pImg[(i + loc_row)*w + j+ loc_col];
		}
	}
	for (int i = loc_row; i < loc_row + CU_h; i++)
	{
		for (int j = loc_col; j < loc_col + CU_w; j++)
		{
			depthMap[i * w + j] = dep;
		}
	}

	pImage = pImg;
	split_flag = false;
	oneColor = false;
	contiEdge = false;
	if (m_depth == 0) father = NULL;
	for (int childIndex = 0; childIndex < 4; childIndex++)
		child[childIndex] = NULL;
	Edge = NULL;
	runLength = NULL;
	predict1 = -1; predict2 = -1;

	if (m_depth == 0)
	{
		m_context = new int[4 * 520];
		for (int i = 0; i < 520 * 4 - 2; i++) m_context[i] = 1;
		m_context[2078] = -256;
		m_context[2079] = 256;
	}
	context_aColor0 = m_context;
	context_element = context_aColor0 + 520;
	context_aColor1 = context_element + 520;
}

void CodingUnit::create(unsigned char* pImg, int h, int w, int CU_h, int CU_w, int loc_row, int loc_col, int dep, int max_cu)
{
	pImage = pImg;
	m_height = CU_h;
	m_width = CU_w;
	m_depth = dep;
	location_row = loc_row;
	location_col = loc_col;
	pic_width = w;
	pic_height = h;
	context = 0;
	split_flag = false;
	if (m_depth == 0) father = NULL;
	for (int childIndex = 0; childIndex < 4; childIndex++)
		child[childIndex] = NULL;
	m_value = NULL;
	Edge = NULL;
	runLength = NULL;
	for (int i = loc_row; i < loc_row + CU_h; i++)
	{
		for (int j = loc_col; j < loc_col + CU_w; j++)
		{
			depthMap[i * w + j] = dep;
		}
	}
	predict1 = -1; predict2 = -1;

	if (m_depth == 0)
	{
		m_context = new int[4 * 520];
		for (int i = 0; i < 520 * 4 - 2; i++) m_context[i] = 1;
		m_context[2078] = -256;
		m_context[2079] = 256;
	}
	context_aColor0 = m_context;
	context_element = context_aColor0 + 520;
	context_aColor1 = context_element + 520;
}

int CodingUnit::getWidth()
{
	return m_width;
}

int CodingUnit::getHeight()
{
	return m_height;
}

int CodingUnit::getDepth()
{
	return m_depth;
}

unsigned char * CodingUnit::getValue()
{
	return m_value;
}

unsigned char * CodingUnit::getImg()
{
	return pImage;
}

int CodingUnit::getColorValue()
{
	return colorValue;
}

void CodingUnit::setColorValue(int color)
{
	colorValue = color;

	int color1, color2;
	int predict = predictColor1(this, &color1, &color2);
	if (predict == -1 || (predict == 1 && color1 != colorValue) || (predict == 2 && color1 != colorValue && color2 != colorValue))
	{
		if (location_col > 0) predict1 = pImage[pic_width*location_row + location_col - 1];
		if (location_row > 0) predict2 = pImage[pic_width*(location_row - 1) + location_col];

		if (location_col > 0 && location_row > 0)
		{
			int LL = abs(int(pImage[pic_width*(location_row) + location_col - 1]) - int(pImage[pic_width*(location_row) + location_col - 2]));
			int LT = abs(int(pImage[pic_width*(location_row) + location_col - 1]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
			int TL = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
			int TT = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 2) + location_col]));
			int val = LL + TL - LT - TT;

			if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
			else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
			else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
			else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
			else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row)+location_col - 2] + pImage[pic_width*(location_row - 2) + location_col] + 4) / 8;

		}
		else if (location_col == 0 && location_row > 0) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
		else if (location_col > 0 && location_row == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;

		int dif = colorValue - predict1;
		if (dif > m_context[2078]) m_context[2078] = dif;
		if (dif < m_context[2079]) m_context[2079] = dif;
	}
}

int * CodingUnit::getEdge()
{
	return Edge;
}

int CodingUnit::getEdgeLength()
{
	return edgeLength;
}

void get_context(unsigned char* block, int location_row, int location_col, int width, int block_side, int initial_y, int initial_x, int color2, int* edgeline, int* context, int* mode)
{
	unsigned char* ref = block + location_row * width + location_col;
	int line[128 * 128];
	int length = 0;
	int x, y;
	if (initial_x == 0)
	{
		ref -= block_side;
		line[0] = 0;
		x = block_side;
		y = initial_y;
		while (!((x == 0) || (x == block_side + 1) || (y == 0) || (y == block_side)))
		{
			switch (line[length])
			{
			case 0:
				if (ref[y * width + x - 1] != color2) line[++length] = 1;
				else if (ref[(y - 1) * width + x - 1] != color2) line[++length] = 0;
				else line[++length] = 3;
				break;
			case 1:
				if (x == block_side)
				{
					if (ref[y * width + x - 1] != color2) line[++length] = 1;
					else line[++length] = 0;
				}
				else
				{
					if (ref[y * width + x] != color2) line[++length] = 2;
					else if (ref[y * width + x - 1] != color2) line[++length] = 1;
					else line[++length] = 0;
				}
				break;
			case 2:
				if (ref[(y - 1) * width + x] != color2) line[++length] = 3;
				else if (ref[y * width + x] != color2) line[++length] = 2;
				else line[++length] = 1;
				break;
			case 3:
				if (x == block_side)
				{
					if (ref[(y - 1) * width + x - 1] != color2) line[++length] = 0;
					else line[++length] = 3;
				}
				else
				{
					if (ref[(y - 1) * width + x - 1] != color2) line[++length] = 0;
					else if (ref[(y - 1) * width + x] != color2) line[++length] = 3;
					else line[++length] = 2;
				}
			}
			switch (line[length])
			{
			case 0: x--; break;
			case 1: y++; break;
			case 2: x++; if (x == block_side) x++; break;
			case 3: y--;
			}
		}
	}
	else if (initial_y == 0)
	{
		ref -= block_side* width;
		line[0] = 3;
		x = initial_x;
		y = block_side;
		while (!((x == 0) || (x == block_side) || (y == 0) || (y == block_side + 1)))
		{
			switch (line[length])
			{
			case 0:
				if (y == block_side)
				{
					if (ref[(y - 1) * width + x - 1] != color2) line[++length] = 3;
					else line[++length] = 0;
				}
				else
				{
					if (ref[(y - 1) * width + x - 1] != color2) line[++length] = 3;
					else if (ref[y * width + x - 1] != color2) line[++length] = 0;
					else line[++length] = 1;
				}
				break;
			case 1:
				if (ref[y * width + x - 1] != color2) line[++length] = 0;
				else if (ref[y * width + x] != color2) line[++length] = 1;
				else line[++length] = 2;
				break;
			case 2:
				if (y == block_side)
				{
					if (ref[(y - 1) * width + x] != color2) line[++length] = 2;
					else line[++length] = 3;
				}
				else
				{
					if (ref[y * width + x] != color2) line[++length] = 1;
					else if (ref[(y - 1) * width + x] != color2) line[++length] = 2;
					else line[++length] = 3;
				}
				break;
			case 3:
				if (ref[(y - 1) * width + x] != color2) line[++length] = 2;
				else if (ref[(y - 1) * width + x - 1] != color2) line[++length] = 3;
				else line[++length] = 0;
			}
			switch (line[length])
			{
			case 0: x--; break;
			case 1: y++; if (y == block_side) y++; break;
			case 2: x++; break;
			case 3: y--;
			}
		}
	}
	if (x == 0 || y == block_side) *mode = 0;
	else *mode = 1;
	for (int i = length; i > 0; i--)
	{
		if (line[i - 1] - line[i] == 1 || line[i - 1] - line[i] == -3)  //left
		{
			if (*mode == 0) line[i] = 2;
			if (*mode == 1)
			{
				*mode = 0;
				line[i] = 1;
			}
		}
		else if (line[i - 1] - line[i] == -1 || line[i - 1] - line[i] == 3) //right
		{
			if (*mode == 1) line[i] = 2;
			if (*mode == 0)
			{
				*mode = 1;
				line[i] = 1;
			}
		}
		else
		{
			line[i] = 0;
		}
	}
	if (length - 1 > 3) *context = 3;
	else *context = length - 1;
	for (int i = 0; i < *context; i++)
	{
		edgeline[i] = line[*context - i];
	}
}

void CodingUnit::setEdge()
{
	int color1 = m_value[0];


	int Color1, Color2;
	int predict = predictColor1(this, &Color1, &Color2);
	if (predict == -1 || (predict == 1 && Color1 != color1) || (predict == 2 && Color1 != color1 && Color2 != color1))
	{
		if (location_col > 0) predict1 = pImage[pic_width*location_row + location_col - 1];
		if (location_row > 0) predict2 = pImage[pic_width*(location_row - 1) + location_col];

		if (location_col > 0 && location_row > 0)
		{
			int LL = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row)+location_col - 2]));
			int LT = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
			int TL = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
			int TT = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 2) + location_col]));
			int val = LL + TL - LT - TT;

			if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
			else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
			else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
			else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
			else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row)+location_col - 2] + pImage[pic_width*(location_row - 2) + location_col] + 4) / 8;
		}
		else if (location_col == 0 && location_row > 0) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
		else if (location_col > 0 && location_row == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;

		int dif = color1 - predict1;
		if (dif > m_context[2078]) m_context[2078] = dif;
		if (dif < m_context[2079]) m_context[2079] = dif;
	}

	int color2;
	int x = 0, y = 0;
	bool findColor2 = false;
	int i, j;
	int edgeline[128*128];
	int forbiddenline[128 * 128];
	unsigned char* ChainPath = new unsigned char [(m_height + 1) * (m_width + 1)];
	for (int i = 0; i < (m_height + 1) * (m_width + 1); i++) ChainPath[i] = 0;

	for (i = 1; i < m_height; i++)     
	{                                     
		if (m_value[i*m_width] != color1)           
		{                              
			color2 = m_value[i*m_width];
			findColor2 = true; y = i; x = 0;
			break;
		}
	}
	if (!findColor2)
	{
		for (j = 1; j < m_width; j++)
		{
			if (m_value[j] != color1)
			{
				color2 = m_value[j];
				findColor2 = true; y = 0; x = j;
				break;
			}
		}
	}
	if (!findColor2)
	{
		for (j = 1; j < m_width; j++)
		{
			if (m_value[m_width*(m_height - 1) + j] != color1)
			{
				color2 = m_value[m_width*(m_height - 1) + j];
				findColor2 = true; y = m_height - 1; x = j;
				break;
			}
		}
	}
	if (!findColor2)
	{
		for (i = 1; i < m_height - 1; i++)
		{
			if (m_value[i*m_width + m_width - 1] != color1)
			{
				color2 = m_value[i*m_width + m_width - 1];
				findColor2 = true; y = i; x = m_width - 1;
				break;
			}
		}
	}

	int dif = predictColor2(this, y, x, color1);
	if (dif < 0 || dif != color2)
	{
		dif = color2 - color1;
		if (dif > m_context[2078]) m_context[2078] = dif;
		if (dif < m_context[2079]) m_context[2079] = dif;
	}

	int initial_y = y, initial_x = x;
	edgeLength = 0;
	if (m_height >= 4)  //3OT
	{
		mode = 1;
		if (initial_x == 0 || initial_y == m_height - 1) mode = 0;
		if ((initial_x == 0 && location_col > 0) || (initial_y == 0 && location_row > 0))
		{
			get_context(getImg(), location_row, location_col, pic_width, m_width, initial_y, initial_x, color2, edgeline, &context, &mode);
		}
		edgeLength = context;
		int mode_item = mode;

		if (initial_x == 0)
		{
			edgeline[edgeLength] = 0; x++;
		}
		else if (initial_y == 0)
		{
			edgeline[edgeLength] = 3; y++;
		}
		else if (initial_y == m_height - 1)
		{
			edgeline[edgeLength] = 1;
		}
		else
		{
			edgeline[edgeLength] = 2;
		}
		while (!((x == m_width) || (y == 0) || (x == 0) || (y == m_height)))
		{
			forbiddenline[edgeLength] = -1;
			if (initial_x == 0)
			{
				if (x == 1 && y < initial_y)
				{
					if (edgeline[edgeLength] == 1) forbiddenline[edgeLength] = 2;
					if (edgeline[edgeLength] == 2) forbiddenline[edgeLength] = 4;
				}
			}
			else if (initial_y == 0)
			{
				if (x == 1)
				{
					if (edgeline[edgeLength] == 2) forbiddenline[edgeLength] = 4;
					if (edgeline[edgeLength] == 3) forbiddenline[edgeLength] = 2;
				}
				else if (x < initial_x && y == 1)
				{
					if (edgeline[edgeLength] == 1) forbiddenline[edgeLength] = 4;
					if (edgeline[edgeLength] == 2) forbiddenline[edgeLength] = 1;
				}
			}
			else if (initial_y == m_height - 1)
			{
				if (x == 1)
				{
					if (edgeline[edgeLength] == 1) forbiddenline[edgeLength] = 2;
					if (edgeline[edgeLength] == 2) forbiddenline[edgeLength] = 4;
				}
				else if (y == 1)
				{
					if (edgeline[edgeLength] == 0) forbiddenline[edgeLength] = 1;
					if (edgeline[edgeLength] == 1) forbiddenline[edgeLength] = 4;
				}
				else if (x < initial_x && y == m_height - 1)
				{
					if (edgeline[edgeLength] == 2) forbiddenline[edgeLength] = 3;
					if (edgeline[edgeLength] == 3) forbiddenline[edgeLength] = 4;
				}
			}
			else
			{
				if (x == 1)
				{
					if (edgeline[edgeLength] == 2) forbiddenline[edgeLength] = 4;
					if (edgeline[edgeLength] == 3) forbiddenline[edgeLength] = 2;
				}
				else if (y == 1)
				{
					if (edgeline[edgeLength] == 1) forbiddenline[edgeLength] = 4;
					if (edgeline[edgeLength] == 2) forbiddenline[edgeLength] = 1;
				}
				else if (y == m_height - 1)
				{
					if (edgeline[edgeLength] == 0) forbiddenline[edgeLength] = 3;
					if (edgeline[edgeLength] == 3) forbiddenline[edgeLength] = 4;
				}
				else if (x == m_width - 1 && y < initial_y)
				{
					if (edgeline[edgeLength] == 0) forbiddenline[edgeLength] = 4;
					if (edgeline[edgeLength] == 1) forbiddenline[edgeLength] = 0;
				}
			}

			ChainPath[y * (m_width + 1) + x] = 1;
			int forbidden1, forbidden2;
			for (int i = 0; i < 4; i++)
			{
				if (i == 2) continue;
				else if (i == 0) { forbidden1 = (edgeline[edgeLength] + 1) % 4; forbidden2 = (edgeline[edgeLength] + 3) % 4; }
				else if (i == 1) { forbidden1 = edgeline[edgeLength]; forbidden2 = (edgeline[edgeLength] + 3) % 4; }
				else { forbidden1 = edgeline[edgeLength]; forbidden2 = (edgeline[edgeLength] + 1) % 4; }
				switch ((edgeline[edgeLength] + i) % 4)
				{
				case 0: if (ChainPath[y * (m_width + 1) + x + 1] == 1) { if (forbiddenline[edgeLength] == forbidden1 || forbiddenline[edgeLength] == forbidden2) forbiddenline[edgeLength] = 4; else if (forbiddenline[edgeLength] != 4) forbiddenline[edgeLength] = 0; } break;
				case 1: if (ChainPath[(y - 1) * (m_width + 1) + x] == 1) { if (forbiddenline[edgeLength] == forbidden1 || forbiddenline[edgeLength] == forbidden2) forbiddenline[edgeLength] = 4; else if (forbiddenline[edgeLength] != 4) forbiddenline[edgeLength] = 1; }break;
				case 2: if (ChainPath[y * (m_width + 1) + x - 1] == 1) { if (forbiddenline[edgeLength] == forbidden1 || forbiddenline[edgeLength] == forbidden2) forbiddenline[edgeLength] = 4; else if (forbiddenline[edgeLength] != 4) forbiddenline[edgeLength] = 2; }break;
				case 3: if (ChainPath[(y + 1) * (m_width + 1) + x] == 1) { if (forbiddenline[edgeLength] == forbidden1 || forbiddenline[edgeLength] == forbidden2) forbiddenline[edgeLength] = 4; else if (forbiddenline[edgeLength] != 4) forbiddenline[edgeLength] = 3; }
				}
			}

			switch (edgeline[edgeLength])
			{
			case 0:
				if (initial_x == 0 || initial_y == m_height - 1)
				{
					if (m_value[y * m_height + x] == color1) edgeline[++edgeLength] = 3;
					else if (m_value[(y - 1) * m_height + x] == color1) edgeline[++edgeLength] = 0;
					else edgeline[++edgeLength] = 1;
				}
				else
				{
					if (m_value[(y - 1) * m_height + x] == color1) edgeline[++edgeLength] = 1;
					else if (m_value[y * m_height + x] == color1) edgeline[++edgeLength] = 0;
					else edgeline[++edgeLength] = 3;
				}
				break;
			case 1:
				if (initial_x == 0 || initial_y == m_height - 1)
				{
					if (m_value[(y - 1) * m_height + x] == color1) edgeline[++edgeLength] = 0;
					else if (m_value[(y - 1) * m_height + x - 1] == color1) edgeline[++edgeLength] = 1;
					else edgeline[++edgeLength] = 2;
				}
				else
				{
					if (m_value[(y - 1) * m_height + x - 1] == color1) edgeline[++edgeLength] = 2;
					else if (m_value[(y - 1) * m_height + x] == color1) edgeline[++edgeLength] = 1;
					else edgeline[++edgeLength] = 0;
				}
				break;
			case 2:
				if (initial_x == 0 || initial_y == m_height - 1)
				{
					if (m_value[(y - 1) * m_height + x - 1] == color1) edgeline[++edgeLength] = 1;
					else if (m_value[y * m_height + x - 1] == color1) edgeline[++edgeLength] = 2;
					else edgeline[++edgeLength] = 3;
				}
				else
				{
					if (m_value[y * m_height + x - 1] == color1) edgeline[++edgeLength] = 3;
					else if (m_value[(y - 1) * m_height + x - 1] == color1) edgeline[++edgeLength] = 2;
					else edgeline[++edgeLength] = 1;
				}
				break;
			case 3:
				if (initial_x == 0 || initial_y == m_height - 1)
				{
					if (m_value[y * m_height + x - 1] == color1) edgeline[++edgeLength] = 2;
					else if (m_value[y * m_height + x] == color1) edgeline[++edgeLength] = 3;
					else edgeline[++edgeLength] = 0;
				}
				else
				{
					if (m_value[y * m_height + x] == color1) edgeline[++edgeLength] = 0;
					else if (m_value[y * m_height + x - 1] == color1) edgeline[++edgeLength] = 3;
					else edgeline[++edgeLength] = 2;
				}
			}
			switch (edgeline[edgeLength])
			{
			case 0: x++; break;
			case 1: y--; break;
			case 2: x--; break;
			case 3: y++;
			}
		}

		for (int i = context; i < edgeLength; i++)
		{
			if (forbiddenline[i] >= 0 && forbiddenline[i] < 4)
			{
				if (forbiddenline[i] - edgeline[i] == 1 || forbiddenline[i] - edgeline[i] == -3)  //left
				{
					if (mode_item == 0) forbiddenline[i] = 2;
					if (mode_item == 1) forbiddenline[i] = 1;
				}
				else if (forbiddenline[i] - edgeline[i] == -1 || forbiddenline[i] - edgeline[i] == 3) //right
				{
					if (mode_item == 1) forbiddenline[i] = 2;
					if (mode_item == 0) forbiddenline[i] = 1;
				}
				else
				{
					forbiddenline[i] = 0;
				}
			}


			if (edgeline[i + 1] - edgeline[i] == 1 || edgeline[i + 1] - edgeline[i] == -3)  //left
			{
				if (mode_item == 0) edgeline[i] = 2;
				if (mode_item == 1)
				{
					mode_item = 0;
					edgeline[i] = 1;
				}
			}
			else if (edgeline[i + 1] - edgeline[i] == -1 || edgeline[i + 1] - edgeline[i] == 3) //right
			{
				if (mode_item == 1) edgeline[i] = 2;
				if (mode_item == 0)
				{
					mode_item = 1;
					edgeline[i] = 1;
				}
			}
			else
			{
				edgeline[i] = 0;
			}
		}
	}

	Edge = new int[edgeLength + 5];
	Edge[0] = color1;
	Edge[1] = initial_y;
	Edge[2] = initial_x;
	Edge[3] = color2;
    Edge[4] = edgeLength;
	for (i = 0; i < edgeLength; i++)
		Edge[5 + i] = edgeline[i];

	ForbiddenLine = new int[edgeLength];
	for (i = context; i < edgeLength; i++)
		ForbiddenLine[i] = forbiddenline[i];

	delete[] ChainPath;
}

int power(int a, int b)
{
	if (b == 0) return 1;
	int output = a;
	for (int i = 0; i < b - 1; i++)
		output *= a;
	return output;
}

void CodingUnit::setRunLength()
{
	runLength = new int[m_height*m_width];
	for (int i = 0; i < m_height; i++)
	{
		for (int j = 0; j < m_width; j++)
		{
			if (location_col + j == 0 && location_row + i == 0) continue;
			if (location_col + j > 0) predict1 = pImage[pic_width*(location_row + i) + location_col + j - 1];
			if (location_row + i > 0) predict2 = pImage[pic_width*(location_row + i - 1) + location_col + j];
			if (location_col + j > 0 && location_row + i > 0) 
			{
				if (location_col + j > 1 && location_row + i > 1)
				{
					int LL = abs(int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - int(pImage[pic_width*(location_row + i) + location_col + j - 2]));
					int LT = abs(int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - int(pImage[pic_width*(location_row + i - 1) + location_col + j - 1]));
					int TL = abs(int(pImage[pic_width*(location_row + i - 1) + location_col + j]) - int(pImage[pic_width*(location_row + i - 1) + location_col + j - 1]));
					int TT = abs(int(pImage[pic_width*(location_row + i - 1) + location_col + j]) - int(pImage[pic_width*(location_row + i - 2) + location_col + j]));
					int val = LL + TL - LT - TT;

					if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
					else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
					else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row + i)+location_col - 2 + j] + 2) / 4;
					else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row + i)+location_col - 2 + j] + 2) / 4;
					else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row + i)+location_col - 2 + j] + pImage[pic_width*(location_row + i - 2) + location_col + j] + 4) / 8;
				}
					
				else predict1 = (predict1 + predict2 + 1) / 2;
			}
			else if (location_col + j == 0 && location_row + i > 1) predict1 = (3 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
			else if (location_col + j == 0 && location_row + i == 1) predict1 = predict2;
			else if (location_col + j > 1 && location_row + i == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + 2) / 4;

			if (g_adapt == 1)
			{
				if (location_row + i > 0 && location_col + j > 0 && pImage[pic_width*(location_row + i) + location_col + j - 1] == pImage[pic_width*(location_row + i - 1) + location_col + j] && pImage[pic_width*(location_row + i) + location_col + j] != pImage[pic_width*(location_row + i - 1) + location_col + j] && pImage[pic_width*(location_row + i) + location_col + j - 1] == adapt_point[pic_width*(location_row + i) + location_col + j])
				{
					pImage[pic_width*(location_row + i) + location_col + j] = adapt_point[pic_width*(location_row + i) + location_col + j];
					m_value[i * m_width + j] = adapt_point[pic_width*(location_row + i) + location_col + j];
				}
				else 
				{
					if (m_value[i * m_width + j] > predict1)
					{
						if (adapt_point[pic_width*(location_row + i) + location_col + j] == m_value[i * m_width + j] - 1)
						{
							m_value[i * m_width + j]--; pImage[pic_width*(location_row + i) + location_col + j]--;
						}
					}
					else if (m_value[i * m_width + j] < predict1)
					{
						if (adapt_point[pic_width*(location_row + i) + location_col + j] == m_value[i * m_width + j] + 1)
						{
							m_value[i * m_width + j]++; pImage[pic_width*(location_row + i) + location_col + j]++;
						}
					}
				}
			}

			int dif = int(m_value[i * m_width + j]) - predict1;
			if (dif > m_context[2078]) m_context[2078] = dif;
			if (dif < m_context[2079]) m_context[2079] = dif;
			runLength[i * m_width + j] = predict1;
		}
	}
}

int * CodingUnit::getRunLength()
{
	return runLength;
}

void CodingUnit::enSplitFlag()
{
	if (splitCTU_flag == 0)
	{
		int index;
		int splitFlag = split_flag;
		if (location_col > 0 && depthMap[location_row*pic_width + location_col - 1] > m_depth) index = 1;
		else index = 0;
		if (location_row > 0 && depthMap[(location_row - 1)*pic_width + location_col] > m_depth) index++;

		switch (index)
		{
		case 0:acodec.encode(splitFlag, aSplitFlag1); context_element[offset_SplitFlag1 + splitFlag]++; break;
		case 1:acodec.encode(splitFlag, aSplitFlag2); context_element[offset_SplitFlag2 + splitFlag]++; break;
		case 2:acodec.encode(splitFlag, aSplitFlag3); context_element[offset_SplitFlag3 + splitFlag]++; break;
		}

	}
}

bool isOneLine(CodingUnit* CU, int loc1, int length,int step)
{
	unsigned char* pImg = CU->getImg();
	int color1 = pImg[loc1];

	for (int i = 1; i < length; i++)
	{
		if (pImg[loc1+i*step] != color1) return false;
	}
	return true;
}

void CodingUnit::enOneColorFlag(int pos)
{
	if (pos == 3 && father->child[0]->oneColor && father->child[1]->oneColor && father->child[2]->oneColor && father->child[0]->colorValue == father->child[1]->colorValue && father->child[0]->colorValue == father->child[2]->colorValue) return;

	if (m_height == g_min_CU)
	{
		int index = 0;
		if (location_col > 0 && isOneLine(this, pic_width*location_row + location_col - 1, m_height, pic_width)) index = 1;
		if (location_row > 0 && isOneLine(this, pic_width*(location_row - 1) + location_col, m_height, 1)) index++;

		if (oneColor)
		{
			switch (index)
			{
			case 0:acodec.encode(1, aOneColorFlag21); context_element[offset_OneColorFlag21 + 1]++; break;
			case 1:acodec.encode(1, aOneColorFlag22); context_element[offset_OneColorFlag22 + 1]++; break;
			case 2:acodec.encode(1, aOneColorFlag23); context_element[offset_OneColorFlag23 + 1]++; break;
			}
		}
		else
		{
			switch (index)
			{
			case 0:acodec.encode(0, aOneColorFlag21); context_element[offset_OneColorFlag21]++; break;
			case 1:acodec.encode(0, aOneColorFlag22); context_element[offset_OneColorFlag22]++; break;
			case 2:acodec.encode(0, aOneColorFlag23); context_element[offset_OneColorFlag23]++; break;
			}
		}
	}
	else
	{
		int index = 0;
		if (location_col > 0 && isOneLine(this, pic_width*location_row + location_col - 1, m_height, pic_width)) index = 1;
		if (location_row > 0 && isOneLine(this, pic_width*(location_row - 1) + location_col, m_height,1)) index++;
		if (oneColor)
		{
			switch (index)
			{
			case 0:acodec.encode(1, aOneColorFlag1); context_element[offset_OneColorFlag1 + 1]++;  break;
			case 1:acodec.encode(1, aOneColorFlag2); context_element[offset_OneColorFlag2 + 1]++;  break;
			case 2:acodec.encode(1, aOneColorFlag3); context_element[offset_OneColorFlag3 + 1]++;  break;
			}
		}
		else
		{
			switch (index)
			{
			case 0:acodec.encode(0, aOneColorFlag1); context_element[offset_OneColorFlag1]++; break;
			case 1:acodec.encode(0, aOneColorFlag2); context_element[offset_OneColorFlag2]++; break;
			case 2:acodec.encode(0, aOneColorFlag3); context_element[offset_OneColorFlag3]++; break;
			}
		}
	}
}

void CodingUnit::enFlag(int pos)
{
		int index = 0;
		if (location_col > 0 && isOneLine(this, pic_width*location_row + location_col - 1, m_height, pic_width)) index = 1;
		if (location_row > 0 && isOneLine(this, pic_width*(location_row - 1) + location_col, m_height, 1)) index++;

		if (pos == 3 && father->child[0]->oneColor && father->child[1]->oneColor && father->child[2]->oneColor && father->child[0]->colorValue == father->child[1]->colorValue && father->child[0]->colorValue == father->child[2]->colorValue)
		{
            if (contiEdge)
			{
				switch (index)
				{
				case 0:acodec.encode(1, aFlag1, 0); context_element[offset_Flag1 + 1]++; break;
				case 1:acodec.encode(1, aFlag2, 0); context_element[offset_Flag2 + 1]++; break;
				case 2:acodec.encode(1, aFlag3, 0); context_element[offset_Flag3 + 1]++; break;
				}
			}
			else
			{
				switch (index)
				{
				case 0:acodec.encode(2, aFlag1, 0); context_element[offset_Flag1 + 2]++; break;
				case 1:acodec.encode(2, aFlag2, 0); context_element[offset_Flag2 + 2]++; break;
				case 2:acodec.encode(2, aFlag3, 0); context_element[offset_Flag3 + 2]++; break;
				}
			}
		}
		else
		{
			if (oneColor)
			{
				switch (index)
				{
				case 0:acodec.encode(0, aFlag1); context_element[offset_Flag1]++; break;
				case 1:acodec.encode(0, aFlag2); context_element[offset_Flag2]++; break;
				case 2:acodec.encode(0, aFlag3); context_element[offset_Flag3]++; break;
				}
			}
			else if (contiEdge)
			{
				switch (index)
				{
				case 0:acodec.encode(1, aFlag1); context_element[offset_Flag1 + 1]++; break;
				case 1:acodec.encode(1, aFlag2); context_element[offset_Flag2 + 1]++; break;
				case 2:acodec.encode(1, aFlag3); context_element[offset_Flag3 + 1]++; break;
				}
			}
			else
			{
				switch (index)
				{
				case 0:acodec.encode(2, aFlag1); context_element[offset_Flag1 + 2]++; break;
				case 1:acodec.encode(2, aFlag2); context_element[offset_Flag2 + 2]++; break;
				case 2:acodec.encode(2, aFlag3); context_element[offset_Flag3 + 2]++; break;
				}
			}
		}
}

void CodingUnit::enContinueEdgeFlag()
{
	int index;
	int ContiEdge = contiEdge;
	if (location_col > 0 && isOneLine(this, pic_width*location_row + location_col - 1, m_height, pic_width)) index = 1;
	else index = 0;
	if (location_row > 0 && isOneLine(this, pic_width*(location_row - 1) + location_col, m_height, 1)) index++;
	switch (index)
	{
	case 0:acodec.encode(ContiEdge, aContinueEdgeFlag1); context_element[offset_ContinueEdgeFlag1 + ContiEdge]++;  break;
	case 1:acodec.encode(ContiEdge, aContinueEdgeFlag2); context_element[offset_ContinueEdgeFlag2 + ContiEdge]++;  break;
	case 2:acodec.encode(ContiEdge, aContinueEdgeFlag3); context_element[offset_ContinueEdgeFlag3 + ContiEdge]++;  break;
	}
}

void CodingUnit::getValContext()
{
	if (location_row > 0)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				context_aColor1[pImage[(location_row +i - 4)*pic_width + location_col + j]]++;
			}
		}
	}
	if (location_col > 0)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				context_aColor1[pImage[(location_row + i)*pic_width + location_col + j - 4]]++;
			}
		}
	}
}

void CodingUnit::enOneColor()
{
	int color1, color2;
	int predict = predictColor1(this, &color1, &color2);   //predict the color
	if (predict > 0)
	{
		if (color1 == colorValue)
		{
			acodec.encode(1, aPredictColor1Flag); context_element[offset_PredictColor1Flag + 1]++;
		}
		else
		{
			acodec.encode(0, aPredictColor1Flag); context_element[offset_PredictColor1Flag]++;
			if (predict == 2)
			{
				if (color2 == colorValue) { acodec.encode(1, aPredictColor1Flag2);  context_element[offset_PredictColor1Flag2 + 1]++;}
				else
				{
					acodec.encode(0, aPredictColor1Flag2); context_element[offset_PredictColor1Flag2]++;
					if (!encode_numC)
					{
						if (m_context[2077] == 256)
						{
							encGolomb(m_context[2078], 0);
							//encGolomb(m_context[2078] - m_context[2079], 0);
						}
						else
						{
							//encGolomb(m_context[2079] - m_context[2077], 1);
							encGolomb(m_context[2078] - m_context[2076], 1);
						}
						numC = m_context[2078] - m_context[2079] + 1;
						if (numC > 1)
						{
							aColor0.set_alphabet(numC);
							aColor0.copy(context_aColor0, m_context[2079]);
						}
						if (numC >= PCM_threshold)
						{
							PCM_flag = 1;
							encGolomb(m_context[2074] - m_context[2072], 1);
							encGolomb(m_context[2075] - m_context[2073], 1);
							if (m_context[2074] != m_context[2075])
							{
								aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
								getValContext();
								aColor1.copy(context_aColor1 + m_context[2075]);
							}
						}
						for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
						encode_numC = true;
					}
					if (numC > 1)
					{
						if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078] && color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
						{
							if (numC > 3)
							{
								acodec.encode(colorValue - predict1 - m_context[2079], aColor0, color1 - predict1 - m_context[2079], color2 - predict1 - m_context[2079]); //context_aColor0[colorValue - predict1 + 256]++;
							}
						}
						else if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078])
						{
							if (numC > 2) { acodec.encode(colorValue - predict1 - m_context[2079], aColor0, color1 - predict1 - m_context[2079]); }// context_aColor0[colorValue - predict1 + 256]++;}
						}
						else if (color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
						{
							if (numC > 2) { acodec.encode(colorValue - predict1 - m_context[2079], aColor0, color2 - predict1 - m_context[2079]); }// context_aColor0[colorValue - predict1 + 256]++;}
						}
						else { acodec.encode(colorValue - predict1 - m_context[2079], aColor0); }// context_aColor0[colorValue - predict1 + 256]++;}
					}
				}
			}
			else
			{
				if (!encode_numC)
				{
					if (m_context[2077] == 256)
					{
						encGolomb(m_context[2078], 0);
						//encGolomb(m_context[2078] - m_context[2079], 0);
					}
					else
					{
						//encGolomb(m_context[2079] - m_context[2077], 1);
						encGolomb(m_context[2078] - m_context[2076], 1);
					}
					numC = m_context[2078] - m_context[2079] + 1;
					if (numC > 1)
					{
						aColor0.set_alphabet(numC);
						aColor0.copy(context_aColor0, m_context[2079]);
					}
					if (numC >= PCM_threshold)
					{
						PCM_flag = 1;
						encGolomb(m_context[2074] - m_context[2072], 1);
						encGolomb(m_context[2075] - m_context[2073], 1);
						if (m_context[2074] != m_context[2075])
						{
							aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
							getValContext();
							aColor1.copy(context_aColor1 + m_context[2075]);
						}
					}
					for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
					encode_numC = true;
				}
				if (numC > 1)
				{
					if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078])
					{
						if (numC > 2) { acodec.encode(colorValue - predict1 - m_context[2079], aColor0, color1 - predict1 - m_context[2079]); }// context_aColor0[colorValue - predict1 + 256]++;}
					}
					else { acodec.encode(colorValue - predict1 - m_context[2079], aColor0); }// context_aColor0[colorValue - predict1 + 256]++;}
				}
			}
		}
	}
	else if (predict == -1)
	{
		if (!encode_numC)
		{
			if (m_context[2077] == 256)
			{
				encGolomb(m_context[2078], 0);
				//encGolomb(m_context[2078] - m_context[2079], 0);
			}
			else
			{
				//encGolomb(m_context[2079] - m_context[2077], 1);
				encGolomb(m_context[2078] - m_context[2076], 1);
			}
			numC = m_context[2078] - m_context[2079] + 1;
			if (numC > 1)
			{
				aColor0.set_alphabet(numC);
				aColor0.copy(context_aColor0, m_context[2079]);
			}
			if (numC >= PCM_threshold)
			{
				PCM_flag = 1;
				encGolomb(m_context[2074] - m_context[2072], 1);
				encGolomb(m_context[2075] - m_context[2073], 1);
				if (m_context[2074] != m_context[2075])
				{
					aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
					getValContext();
					aColor1.copy(context_aColor1 + m_context[2075]);
				}
			}
			for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
			encode_numC = true;
		}
		if (numC > 1)
		{
			acodec.encode(colorValue - predict1 - m_context[2079], aColor0);// context_aColor0[colorValue - predict1 + 256]++;
		}
	}
}

void get_predict_position(int block_size, int* pos1, int* pos2, int* pos3, int predict, int* num)
{
	if (predict == 1)
	{
		*pos1 = 1;
		*pos2 = 2;
		*num = 2;
	}
	else if (predict == block_size - 1)
	{
		*pos1 = block_size - 1;
		*pos2 = block_size - 2;
		*num = 2;
	}
	else
	{
		*pos1 = predict;
		*pos2 = predict + 1;
		*pos3 = predict - 1;
		*num = 3;
	}
}

int get_position(int position, int pos1, int pos2, int pos3, int block_size)
{
	int index = 0;
	for (int i = 1; i < block_size; i++)
	{
		if (i == position) return index;
		if (i == pos1 || i == pos2 || i == pos3) continue;
		index++;
	}

}

void CodingUnit::enChainCoding()
{
	int color1, color2;
	int predict = predictColor1(this, &color1, &color2);
	if (predict > 0)
	{
		if (color1 == Edge[0])
		{
			acodec.encode(1, aPredictColor1Flag); context_element[offset_PredictColor1Flag + 1]++;
		}
		else
		{
			acodec.encode(0, aPredictColor1Flag); context_element[offset_PredictColor1Flag]++;
			if (predict == 2)
			{
				if (color2 == Edge[0]) { acodec.encode(1, aPredictColor1Flag2); context_element[offset_PredictColor1Flag2 + 1]++;}
				else
				{
					acodec.encode(0, aPredictColor1Flag2); context_element[offset_PredictColor1Flag2]++;
					if (!encode_numC)
					{
						if (m_context[2077] == 256)
						{
							encGolomb(m_context[2078], 0);
							//encGolomb(m_context[2078] - m_context[2079], 0);
						}
						else
						{
							//encGolomb(m_context[2079] - m_context[2077], 1);
							encGolomb(m_context[2078] - m_context[2076], 1);
						}
						numC = m_context[2078] - m_context[2079] + 1;
						if (numC > 1)
						{
							aColor0.set_alphabet(numC);
							aColor0.copy(context_aColor0, m_context[2079]);
						}
						if (numC >= PCM_threshold)
						{
							PCM_flag = 1;
							encGolomb(m_context[2074] - m_context[2072], 1);
							encGolomb(m_context[2075] - m_context[2073], 1);
							aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
							getValContext();
							aColor1.copy(context_aColor1 + m_context[2075]);
						}
						for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
						encode_numC = true;
					}
					if (numC > 1)
					{
						if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078] && color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
						{
							if (numC > 3)
							{
								acodec.encode(Edge[0] - predict1 - m_context[2079], aColor0, color1 - predict1 - m_context[2079], color2 - predict1 - m_context[2079]);// context_aColor0[Edge[0] - predict1 + 256]++;
							}
						}
						else if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078])
						{
							if (numC > 2) { acodec.encode(Edge[0] - predict1 - m_context[2079], aColor0, color1 - predict1 - m_context[2079]); }// context_aColor0[Edge[0] - predict1 + 256]++;}
						}
						else if (color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
						{
							if (numC > 2) { acodec.encode(Edge[0] - predict1 - m_context[2079], aColor0, color2 - predict1 - m_context[2079]);}// context_aColor0[Edge[0] - predict1 + 256]++;}
						}
						else { acodec.encode(Edge[0] - predict1 - m_context[2079], aColor0); }// context_aColor0[Edge[0] - predict1 + 256]++;}
					}
				}
			}
			else
			{
				if (!encode_numC)
				{
					if (m_context[2077] == 256)
					{
						encGolomb(m_context[2078], 0);
						//encGolomb(m_context[2078] - m_context[2079], 0);
					}
					else
					{
						//encGolomb(m_context[2079] - m_context[2077], 1);
						encGolomb(m_context[2078] - m_context[2076], 1);
					}
					numC = m_context[2078] - m_context[2079] + 1;
					if (numC > 1)
					{
						aColor0.set_alphabet(numC);
						aColor0.copy(context_aColor0, m_context[2079]);
					}
					if (numC >= PCM_threshold)
					{
						PCM_flag = 1;
						encGolomb(m_context[2074] - m_context[2072], 1);
						encGolomb(m_context[2075] - m_context[2073], 1);
						aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
						getValContext();
						aColor1.copy(context_aColor1 + m_context[2075]);
					}
					for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
					encode_numC = true;
				}
				if (numC > 1)
				{
					if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078])
					{
						if (numC > 2) {
							acodec.encode(Edge[0] - predict1 - m_context[2079], aColor0, color1 - predict1 - m_context[2079]);// context_aColor0[Edge[0] - predict1 + 256]++;}
						}
						else { acodec.encode(Edge[0] - predict1 - m_context[2079], aColor0); }// context_aColor0[Edge[0] - predict1 + 256]++;}
					}
				}
			}
		}
	}
	else if (predict == -1)
	{
		if (!encode_numC)
		{
			if (m_context[2077] == 256)
			{
				encGolomb(m_context[2078], 0);
				//encGolomb(m_context[2078] - m_context[2079], 0);
			}
			else
			{
				//encGolomb(m_context[2079] - m_context[2077], 1);
				encGolomb(m_context[2078] - m_context[2076], 1);
			}
			numC = m_context[2078] - m_context[2079] + 1;
			if (numC > 1)
			{
				aColor0.set_alphabet(numC);
				aColor0.copy(context_aColor0, m_context[2079]);
			}
			if (numC >= PCM_threshold)
			{
				PCM_flag = 1;
				encGolomb(m_context[2074] - m_context[2072], 1);
				encGolomb(m_context[2075] - m_context[2073], 1);
				aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
				getValContext();
				aColor1.copy(context_aColor1 + m_context[2075]);
			}
			for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
			encode_numC = true;
		}
		if (numC > 1)
		{
			acodec.encode(Edge[0] - predict1 - m_context[2079], aColor0);// context_aColor0[Edge[0] - predict1 + 256]++;
		}
	}

	color1 = Edge[0];

	int ini_y, ini_x, ini_y2, ini_x2, predictMode1 = -2, predictMode2 = -2;
	predict = predictInitialPosition(this, &ini_y, &ini_x, &ini_y2, &ini_x2, color1, &predictMode1, &predictMode2);      //predict the initial position.
	int x1 = -1, x2 = -1, x3 = -1, y1 = -1, y2 = -1, y3 = -1, num0 = 0, num1 = 0;
	if (predict == 0 || predict == 2) get_predict_position(m_height, &y1, &y2, &y3, ini_y, &num0);
	if (predict == 1 || predict == 2) get_predict_position(m_height, &x1, &x2, &x3, ini_x2, &num1);
	int CUhSize = getBitSize(m_height);
	int CUwSize = getBitSize(m_width);
	bool predictTrue = 0;
	if (predict == 1)
	{
		ini_y = ini_y2;
		ini_x = ini_x2;
	}
	if (predict >= 0)
	{
		if (predict == 2)
		{
			if (Edge[1] == 0 && abs(ini_x2 - Edge[2]) <= 1)
			{
				ini_y = ini_y2;
				ini_x = ini_x2;
			}
		}
		if (ini_y == 0)
		{
			if (Edge[1] != 0)
			{
				acodec.encode(0, aPredictPositionFlag); context_element[offset_PredictPositionFlag]++;       //predict wrong
			}
			else
			{
				if (ini_x == 1 || ini_x == m_width - 1)
				{
					if (ini_x == Edge[2])
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(1, aPredictPositionSide); context_element[offset_PredictPositionSide + 1]++;}
						acodec.encode(0, aPredictPositionBorder);  context_element[offset_PredictPositionBorder]++; predictTrue = 1;
					}
					else if ((ini_x == 1 && Edge[2] == 2) || (ini_x == m_width - 1 && Edge[2] == m_width - 2))
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(1, aPredictPositionSide); context_element[offset_PredictPositionSide + 1]++; }
						acodec.encode(1, aPredictPositionBorder); context_element[offset_PredictPositionBorder + 1]++; predictTrue = 1;
					}
					else
					{
						acodec.encode(0, aPredictPositionFlag); context_element[offset_PredictPositionFlag]++;
					}
				}
				else
				{
					if (ini_x == Edge[2])
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;     //predict right
						if (predict == 2) { acodec.encode(1, aPredictPositionSide); context_element[offset_PredictPositionSide + 1]++;}
						acodec.encode(0, aPredictPositionMode); context_element[offset_PredictPositionMode]++;
						predictTrue = 1;
					}
					else if (ini_x + 1 == Edge[2])
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(1, aPredictPositionSide); context_element[offset_PredictPositionSide + 1]++;}
						acodec.encode(1, aPredictPositionMode); context_element[offset_PredictPositionMode + 1]++;
						predictTrue = 1;
					}
					else if (ini_x - 1 == Edge[2])
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(1, aPredictPositionSide); context_element[offset_PredictPositionSide + 1]++;}
						acodec.encode(2, aPredictPositionMode); context_element[offset_PredictPositionMode + 2]++;
						predictTrue = 1;
					}
					else
					{
						acodec.encode(0, aPredictPositionFlag); context_element[offset_PredictPositionFlag]++;
					}
				}
			}
		}
		else
		{
			if (Edge[2] != 0)
			{
				acodec.encode(0, aPredictPositionFlag); context_element[offset_PredictPositionFlag]++;
			}
			else
			{
				if (ini_y == 1 || ini_y == m_height - 1)
				{
					if (ini_y == Edge[1])
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(0, aPredictPositionSide); context_element[offset_PredictPositionSide]++; }
						acodec.encode(0, aPredictPositionBorder); context_element[offset_PredictPositionBorder]++; predictTrue = 1;
					}
					else if ((ini_y == 1 && Edge[1] == 2) || (ini_y == m_height - 1 && Edge[1] == m_height - 2))
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(0, aPredictPositionSide); context_element[offset_PredictPositionSide]++;}
						acodec.encode(1, aPredictPositionBorder); context_element[offset_PredictPositionBorder + 1]++; predictTrue = 1;
					}
					else
					{
						acodec.encode(0, aPredictPositionFlag); context_element[offset_PredictPositionFlag]++;
					}
				}
				else
				{
					if (ini_y == Edge[1])
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(0, aPredictPositionSide); context_element[offset_PredictPositionSide]++;}
						acodec.encode(0, aPredictPositionMode); context_element[offset_PredictPositionMode]++;
						predictTrue = 1;
					}
					else if (ini_y + 1 == Edge[1])
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(0, aPredictPositionSide); context_element[offset_PredictPositionSide]++;}
						acodec.encode(1, aPredictPositionMode); context_element[offset_PredictPositionMode + 1]++;
						predictTrue = 1;
					}
					else if (ini_y - 1 == Edge[1])
					{
						acodec.encode(1, aPredictPositionFlag); context_element[offset_PredictPositionFlag + 1]++;
						if (predict == 2) { acodec.encode(0, aPredictPositionSide); context_element[offset_PredictPositionSide]++;}
						acodec.encode(2, aPredictPositionMode); context_element[offset_PredictPositionMode + 2]++;
						predictTrue = 1;
					}
					else
					{
						acodec.encode(0, aPredictPositionFlag); context_element[offset_PredictPositionFlag]++;
					}
				}
			}
		}
	}
	if (!predictTrue)         //if redict wrong or not predict, encode the initial position.
	{
		if (Edge[2] == 0)            //because the initial position is on the boundary, so it is enough to encode a side mode and the one position
		{
			if (m_height == 4 && num1 == 3)	acodec.encode(0, aSideMode, 1); else acodec.encode(0, aSideMode);
			context_element[offset_SideMode]++;
			if (!((predict == 0 || predict == 2) && m_height == 4))
			{
				if (predictMode1 == 1 && y1 >= m_height / 2 && y1 < m_height - 2)
				{
					if (Edge[1] > y1)
					{
						acodec.encode(1, aPredictPosition); context_element[offset_PredictPosition + 1]++;
						int num = m_height - y1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							acodec.encode(Edge[1] - y1 - 2, Position);
						}
					}
					else
					{
						acodec.encode(0, aPredictPosition); context_element[offset_PredictPosition]++;
						int num = y1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							acodec.encode(y1 - Edge[1] - 2, Position);
						}
					}
				}
				else if (predictMode1 == -1 && y1 <= m_height / 2 && y1 > 2)
				{
					if (Edge[1] > y1)
					{
						acodec.encode(0, aPredictPosition); context_element[offset_PredictPosition]++;
						int num = m_height - y1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							acodec.encode(Edge[1] - y1 - 2, Position);
						}
					}
					else
					{
						acodec.encode(1, aPredictPosition); context_element[offset_PredictPosition + 1]++;
						int num = y1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							acodec.encode(y1 - Edge[1] - 2, Position);
						}
					}
				}
				else
				{
					Static_Data_Model Position(m_height - 1 - num0);
					int index = get_position(Edge[1], y1, y2, y3, m_height);
					acodec.encode(index, Position);
				}
			}
		}
		else if (Edge[1] == 0)
		{
			if (m_height == 4 && num0 == 3)	acodec.encode(1, aSideMode, 0); else acodec.encode(1, aSideMode);
			context_element[offset_SideMode + 1]++;
			if (!((predict == 1 || predict == 2) && m_height == 4))
			{
				if (predictMode2 == 1 && x1 >= m_height / 2 && x1 < m_height - 2)
				{
					if (Edge[2] > x1)
					{
						acodec.encode(1, aPredictPosition); context_element[offset_PredictPosition + 1]++;
						int num = m_height - x1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							acodec.encode(Edge[2] - x1 - 2, Position);
						}
					}
					else
					{
						acodec.encode(0, aPredictPosition); context_element[offset_PredictPosition]++;
						int num = x1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							acodec.encode(x1 - Edge[2] - 2, Position);
						}
					}
				}
				else if (predictMode2 == -1 && x1 <= m_height / 2 && x1 > 2)
				{
					if (Edge[2] > x1)
					{
						acodec.encode(0, aPredictPosition); context_element[offset_PredictPosition]++;
						int num = m_height - x1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							acodec.encode(Edge[2] - x1 - 2, Position);
						}
					}
					else
					{
						acodec.encode(1, aPredictPosition); context_element[offset_PredictPosition + 1]++;
						int num = x1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							acodec.encode(x1 - Edge[2] - 2, Position);
						}
					}
				}
				else
				{
					Static_Data_Model Position(m_height - 1 - num1);
					int index = get_position(Edge[2], x1, x2, x3, m_height);
					acodec.encode(index, Position);
				}
			}
		}
		else if (Edge[1] == m_height - 1)
		{
		    context_element[offset_SideMode + 2]++;
			if (m_height == 4 && num0 == 3 && num1 != 3) acodec.encode(2, aSideMode, 0);
			else if (m_height == 4 && num0 != 3 && num1 == 3) acodec.encode(2, aSideMode, 1);
			else if (m_height == 4 && num0 == 3 && num1 == 3)
			{
				acodec.encode(2, aSideMode, 0, 1);
			}
			else acodec.encode(2, aSideMode);
			Static_Data_Model Position(m_height - 1);
			acodec.encode(Edge[2] - 1, Position);
		}
		else
		{
		    context_element[offset_SideMode + 3]++;
			if (m_height == 4 && num0 == 3 && num1 != 3) acodec.encode(3, aSideMode, 0);
			else if (m_height == 4 && num0 != 3 && num1 == 3) acodec.encode(3, aSideMode, 1);
			else if (m_height == 4 && num0 == 3 && num1 == 3)
			{
				acodec.encode(3, aSideMode, 0, 1);
			}
			else acodec.encode(3, aSideMode);
			Static_Data_Model Position(m_height - 2);
			acodec.encode(Edge[1] - 1, Position);
		}
	}

	color2 = predictColor2(this, Edge[1], Edge[2], Edge[0]);        //predict second color
	if (color2 >= 0)
	{
		if (color2 == Edge[3])
		{
			acodec.encode(1, aPredictColor2Flag); context_element[offset_PredictColor2Flag + 1]++;
		}
		else
		{
			acodec.encode(0, aPredictColor2Flag); context_element[offset_PredictColor2Flag]++;
			if (!encode_numC)
			{
				if (m_context[2077] == 256)
				{
					encGolomb(m_context[2078], 0);
					//encGolomb(m_context[2078] - m_context[2079], 0);
				}
				else
				{
					//encGolomb(m_context[2079] - m_context[2077], 1);
					encGolomb(m_context[2078] - m_context[2076], 1);
				}
				numC = m_context[2078] - m_context[2079] + 1;
				if (numC > 1)
				{
					aColor0.set_alphabet(numC);
					aColor0.copy(context_aColor0, m_context[2079]);
				}
				if (numC >= PCM_threshold)
				{
					PCM_flag = 1;
					encGolomb(m_context[2074] - m_context[2072], 1);
					encGolomb(m_context[2075] - m_context[2073], 1);
					aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
					getValContext();
					aColor1.copy(context_aColor1 + m_context[2075]);
				}
				for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
				encode_numC = true;
			}

			if (numC > 1)
			{
				if (color2 - color1 >= m_context[2079] && color2 - color1 <= m_context[2078])
				{
					if (numC > 2)
					{
						if (0 >= m_context[2079] && 0 <= m_context[2078])
						{
							if (numC > 3)
							{
								acodec.encode(Edge[3] - color1 - m_context[2079], aColor0, color2 - color1 - m_context[2079], -m_context[2079]); //context_aColor0[Edge[3] - predict1 + 256]++;
							}
						}
						else
						{
							acodec.encode(Edge[3] - color1 - m_context[2079], aColor0, color2 - color1 - m_context[2079]);// context_aColor0[Edge[3] - predict1 + 256]++;
						}
					}
				}
				else
				{
					if (numC > 1)
					{
						if (0 >= m_context[2079] && 0 <= m_context[2078])
						{
							if (numC > 2)
							{
								acodec.encode(Edge[3] - color1 - m_context[2079], aColor0, -m_context[2079]);// context_aColor0[Edge[3] - predict1 + 256]++;
							}
						}
						else { acodec.encode(Edge[3] - color1 - m_context[2079], aColor0); }// context_aColor0[Edge[3] - predict1 + 256]++;}
					}
				}
			}
		}
	}
	else
	{
		if (!encode_numC)
		{
			if (m_context[2077] == 256)
			{
				encGolomb(m_context[2078], 0);
				//encGolomb(m_context[2078] - m_context[2079], 0);
			}
			else
			{
				//encGolomb(m_context[2079] - m_context[2077], 1);
				encGolomb(m_context[2078] - m_context[2076], 1);
			}
			numC = m_context[2078] - m_context[2079] + 1;
			if (numC > 1)
			{
				aColor0.set_alphabet(numC);
				aColor0.copy(context_aColor0, m_context[2079]);
			}
			if (numC >= PCM_threshold)
			{
				PCM_flag = 1;
				encGolomb(m_context[2074] - m_context[2072], 1);
				encGolomb(m_context[2075] - m_context[2073], 1);
				aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
				getValContext();
				aColor1.copy(context_aColor1 + m_context[2075]);
			}
			for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
			encode_numC = true;
		}
		if (numC > 1)
		{
			if (0 >= m_context[2079] && 0 <= m_context[2078])
			{
				if (numC > 2)
				{
					acodec.encode(Edge[3] - color1 - m_context[2079], aColor0, -m_context[2079]); //context_aColor0[Edge[3] - predict1 + 256]++;
				}
			}
			else { acodec.encode(Edge[3] - color1 - m_context[2079], aColor0); }// context_aColor0[Edge[3] - predict1 + 256]++;}
		}
	}

	if (m_height != 2)
	{
		encodeEdgeLength(this);
	}                                            //encode the edge
}


//bool isPCM(CodingUnit* CU, int loc1, int length, int step)
//{
//	unsigned char* pImg = CU->getImg();
//
//	float bits_predict = 0;
//	for (int i = 0; i < length; i++)
//	{
//		int predict1 = pImg[loc1 - 1 + i * step];
//		int predict2 = pImg[loc1 - CU->pic_width + i * step];
//		int LL = abs(int(pImg[loc1 - 1 + i * step]) - int(pImg[loc1 - 2 + i * step]));
//		int LT = abs(int(pImg[loc1 - 1 + i * step]) - int(pImg[loc1 - CU->pic_width - 1 + i * step]));
//		int TL = abs(int(pImg[loc1 - CU->pic_width + i * step]) - int(pImg[loc1 - CU->pic_width - 1 + i * step]));
//		int TT = abs(int(pImg[loc1 - CU->pic_width + i * step]) - int(pImg[loc1 - 2 * CU->pic_width + i * step]));
//		int val = LL + TL - LT - TT;
//		if (val >= 8) predict1 = (3 * predict2 + pImg[loc1 - 2 * CU->pic_width + i * step] + 2) / 4;
//		else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImg[loc1 - 2 * CU->pic_width + i * step] + 2) / 4;
//		else if (val <= -8) predict1 = (3 * predict1 + pImg[loc1 - 2 + i * step] + 2) / 4;
//		else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImg[loc1 - 2 + i * step] + 2) / 4;
//		else predict1 = (3 * predict1 + 3 * predict2 + pImg[loc1 - 2 + i * step] + pImg[loc1 - 2 * CU->pic_width + i * step] + 4) / 8;
//
//		bits_predict += aColor0.get_bits(int(pImg[loc1 + i * step]) - predict1 - CU->m_context[2079]);
//	}
//
//	float bits_PCM = 0;
//	for (int i = 0; i < length; i++) bits_PCM += aColor1.get_bits(int(pImg[loc1 + i * step]) - CU->m_context[2075]);
//
//	return bits_PCM < bits_predict;
//}

void CodingUnit::enRunLength()
{
	if (!encode_numC)
	{
		if (m_context[2077] == 256)
		{
			encGolomb(m_context[2078], 0);
			//encGolomb(m_context[2078] - m_context[2079], 0);
		}
		else
		{
			//encGolomb(m_context[2079] - m_context[2077], 1);
			encGolomb(m_context[2078] - m_context[2076], 1);
		}
		numC = m_context[2078] - m_context[2079] + 1;
		if (numC > 1)
		{
			aColor0.set_alphabet(numC);
			aColor0.copy(context_aColor0, m_context[2079]);
		}
		if (numC >= PCM_threshold)
		{
			PCM_flag = 1;
			encGolomb(m_context[2074] - m_context[2072], 1);
			encGolomb(m_context[2075] - m_context[2073], 1);
			aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
			getValContext();
			aColor1.copy(context_aColor1 + m_context[2075]);
		}
		for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
		encode_numC = true;
	}
	if (numC > 1)
	{
		if (PCM_flag)
		{
			float bits_predict = aPCM.get_bits(0);
			for (int i = 0; i < m_height; i++)
			{
				for (int j = 0; j < m_width; j++)
				{
					if (location_row + i == 0 && location_col + j == 0) continue;
					if (location_row + i > 0 && location_col + j > 0 && pImage[pic_width*(location_row + i) + location_col + j - 1] == pImage[pic_width*(location_row + i - 1) + location_col + j])
					{
						if (pImage[pic_width*(location_row + i) + location_col + j] == pImage[pic_width*(location_row + i) + location_col + j - 1]) { bits_predict += aPredictColorRunFlag.get_bits(1);}
						else
						{
							bits_predict += aPredictColorRunFlag.get_bits(0); 
							if (numC > 2)
							{
								int index = int(m_value[i*m_width + j]) - runLength[i*m_width + j] - m_context[2079];
								int forbidden = int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - runLength[i*m_width + j];
								int index_forbidden = -1;
								if (forbidden >= m_context[2079] && forbidden <= m_context[2078]) index_forbidden = forbidden - m_context[2079];
								bits_predict += aColor0.get_bits(index, index_forbidden);

							}
						}
					}
					else
					{
						int index = int(m_value[i*m_width + j]) - runLength[i*m_width + j] - m_context[2079];
						bits_predict += aColor0.get_bits(index);
					}
				}
			}

			float bits_PCM = aPCM.get_bits(1);
			for (int i = 0; i < m_height; i++)
			{
				for (int j = 0; j < m_width; j++)
				{
					if (location_row + i == 0 && location_col + j == 0) continue;
					bits_PCM += aColor1.get_bits(int(m_value[i*m_width + j]) - m_context[2075]);
				}
			}

			if (bits_PCM < bits_predict)
			{
				acodec.encode(1, aPCM);	
				for (int i = 0; i < m_height; i++)
				{
					for (int j = 0; j < m_width; j++)
					{
						if (location_row + i == 0 && location_col + j == 0) continue;
						context_aColor0[int(m_value[i*m_width + j]) - runLength[i*m_width + j] + 256]++;
						acodec.encode(int(m_value[i*m_width + j]) - m_context[2075], aColor1);
						//acodec.noencode(int(m_value[i*m_width + j]) - runLength[i*m_width + j] - m_context[2079], aColor0);
					}
				}
				return;
			}
			else
			{
				for (int i = 0; i < m_height; i++)
				{
					for (int j = 0; j < m_width; j++)
					{
						if (location_row + i == 0 && location_col + j == 0) continue;
						acodec.noencode(int(m_value[i*m_width + j]) - m_context[2075], aColor1);
					}
				}
				acodec.encode(0, aPCM);

			}
		}


		for (int i = 0; i < m_height; i++)
		{
			for (int j = 0; j < m_width; j++)
			{
				if (location_row + i == 0 && location_col + j == 0) continue;
				context_aColor0[int(m_value[i*m_width + j]) - runLength[i*m_width + j] + 256]++;

				if (location_row + i > 0 && location_col + j > 0 && pImage[pic_width*(location_row+i)+location_col+j - 1] == pImage[pic_width*(location_row+i - 1) + location_col+j])
				{
					if (pImage[pic_width*(location_row + i) + location_col + j] == pImage[pic_width*(location_row + i) + location_col + j - 1]) { acodec.encode(1, aPredictColorRunFlag); context_element[offset_PredictColorRunFlag + 1]++;}
					else
					{
						acodec.encode(0, aPredictColorRunFlag); context_element[offset_PredictColorRunFlag]++;
						if (numC > 2)
						{
							int index = int(m_value[i*m_width + j]) - runLength[i*m_width + j] - m_context[2079];
							int forbidden = int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - runLength[i*m_width + j];
							int index_forbidden = -1;
							if(forbidden >= m_context[2079] && forbidden <= m_context[2078]) index_forbidden = forbidden - m_context[2079];
							acodec.encode(index, aColor0, index_forbidden);

						}
					}
				}
				else
				{
					int index = int(m_value[i*m_width + j]) - runLength[i*m_width + j] - m_context[2079];
					acodec.encode(index, aColor0);
				}
			}
		}
	}
}

int CodingUnit::deSplitFlag()
{
	if (splitCTU_flag) return 1;

  int index;
  int splitFlag;
  if (location_col > 0 && depthMap[location_row*pic_width + location_col - 1] > m_depth) index = 1;
  else index = 0;
  if (location_row > 0 && depthMap[(location_row - 1)*pic_width + location_col] > m_depth) index++;
  switch (index)
  {
  case 0:splitFlag = acodec.decode(aSplitFlag1); break;
  case 1:splitFlag = acodec.decode(aSplitFlag2); break;
  case 2:splitFlag = acodec.decode(aSplitFlag3); break;
  }  
  return splitFlag;
}

int CodingUnit::deOneColorFlag(int pos)
{
	if (pos == 3 && father->child[0]->oneColor && father->child[1]->oneColor && father->child[2]->oneColor && father->child[0]->colorValue == father->child[1]->colorValue && father->child[0]->colorValue == father->child[2]->colorValue) return 0;

	if (m_height == g_min_CU)
	{
		int index = 0;
		if (location_col > 0 && isOneLine(this, pic_width*location_row + location_col - 1, m_height, pic_width)) index = 1;
		if (location_row > 0 && isOneLine(this, pic_width*(location_row - 1) + location_col, m_height, 1)) index++;
		switch (index)
		{
		case 0:return acodec.decode(aOneColorFlag21);
		case 1:return acodec.decode(aOneColorFlag22);
		case 2:return acodec.decode(aOneColorFlag23);
		}		
	}
	else
	{
		int index = 0;
		if (location_col > 0 && isOneLine(this, pic_width*location_row + location_col - 1, m_height, pic_width)) index = 1;
		if (location_row > 0 && isOneLine(this, pic_width*(location_row - 1) + location_col, m_height, 1)) index++;
		
		switch (index)
		{
		case 0:return acodec.decode(aOneColorFlag1);
		case 1:return acodec.decode(aOneColorFlag2);
		case 2:return acodec.decode(aOneColorFlag3);
		}
	}
}

int CodingUnit::deFlag(int pos)
{
	int index = 0;
	if (location_col > 0 && isOneLine(this, pic_width*location_row + location_col - 1, m_height, pic_width)) index = 1;
	if (location_row > 0 && isOneLine(this, pic_width*(location_row - 1) + location_col, m_height, 1)) index++;
	int flag;

	if (pos == 3 && father->child[0]->oneColor && father->child[1]->oneColor && father->child[2]->oneColor && father->child[0]->colorValue == father->child[1]->colorValue && father->child[0]->colorValue == father->child[2]->colorValue)
	{
		switch (index)
		{
		case 0:flag = acodec.decode(aFlag1, 0); context_element[offset_Flag1 + flag]++; break;
		case 1:flag = acodec.decode(aFlag2, 0); context_element[offset_Flag2 + flag]++; break;
		case 2:flag = acodec.decode(aFlag3, 0); context_element[offset_Flag3 + flag]++; break;
		}
	}
	else
	{
		switch (index)
		{
		case 0:flag = acodec.decode(aFlag1); context_element[offset_Flag1 + flag]++; break;
		case 1:flag = acodec.decode(aFlag2); context_element[offset_Flag2 + flag]++; break;
		case 2:flag = acodec.decode(aFlag3); context_element[offset_Flag3 + flag]++; break;
		}
	}
	return flag;
}

int CodingUnit::deContinueEdgeFlag()
{
	int index;
	int ContiEdge;
	if (location_col > 0 && isOneLine(this, pic_width*location_row + location_col - 1, m_height, pic_width)) index = 1;
	else index = 0;
	if (location_row > 0 && isOneLine(this, pic_width*(location_row - 1) + location_col, m_height, 1)) index++;
	switch (index)
	{
	case 0:ContiEdge = acodec.decode(aContinueEdgeFlag1); break;
	case 1:ContiEdge = acodec.decode(aContinueEdgeFlag2); break;
	case 2:ContiEdge = acodec.decode(aContinueEdgeFlag3); break;
	}
	return ContiEdge;
}

//void CodingUnit::getValContext(int* val)
//{
//	//pImage
//
//	if (location_row > 0)
//	{
//		for (int i = location_row - g_min_CU; i < g_min_CU; i++)
//		{
//			for (int j = location_col; j < g_min_CU; j++)
//			{
//				val[pImage[i*pic_width +j]]++;
//			}
//		}
//	}
//	if (location_col > 0)
//	{
//		for (int i = location_row; i < g_min_CU; i++)
//		{
//			for (int j = location_col - g_min_CU; j < g_min_CU; j++)
//			{
//				val[pImage[i*pic_width + j]]++;
//			}
//		}
//	}
//}

void CodingUnit::deOneColor(unsigned char*pImg)
{
	m_value = new unsigned char[m_height*m_width];
	int color1, color2;
	int predict = predictColor1(this, &color1, &color2);
	if (predict == 0)
	{
		color1 = g_value;
	}
	else if (predict < 0)
	{
		if (!encode_numC)
		{
			if (m_context[2077] == 256)
			{
				m_context[2078] = decGolomb(0);
			}
			else
			{
				m_context[2078] = m_context[2076] + decGolomb(1);
			}
			m_context[2079] = -m_context[2078];

			numC = m_context[2078] - m_context[2079] + 1;
			if (numC > 1)
			{
				aColor0.set_alphabet(numC);
				aColor0.copy(context_aColor0, m_context[2079]);
			}
			if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
			if (numC >= PCM_threshold)
			{
				PCM_flag = 1;
				m_context[2074] = m_context[2072] + decGolomb(1);
				m_context[2075] = m_context[2073] + decGolomb(1);
				if (m_context[2074] != m_context[2075])
				{
					aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
					getValContext();
					aColor1.copy(context_aColor1 + m_context[2075]);
				}
			}
			else PCM_flag = 0;
			for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
			encode_numC = true;
		}
		if (location_col > 0) predict1 = pImage[pic_width*location_row + location_col - 1];
		if (location_row > 0) predict2 = pImage[pic_width*(location_row - 1) + location_col];
		if (location_col > 0 && location_row > 0)
		{
			int LL = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row)+location_col - 2]));
			int LT = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
			int TL = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
			int TT = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 2) + location_col]));
			int val = LL + TL - LT - TT;

			if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
			else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
			else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
			else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
			else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row)+location_col - 2] + pImage[pic_width*(location_row - 2) + location_col] + 4) / 8;

		}
		else if (location_col == 0 && location_row > 0) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
		else if (location_col > 0 && location_row == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
		if (numC == 1) color1 = predict1 + m_context[2079];
		else
		{
			int dif = acodec.decode(aColor0) + m_context[2079];// context_aColor0[dif + 256]++;
			color1 = predict1 + dif;
		}
	}
	else
	{
		if (!acodec.decode(aPredictColor1Flag))
		{
			if (predict == 2)
			{
				if (acodec.decode(aPredictColor1Flag2)) color1 = color2;
				else
				{
					if (!encode_numC)
					{
						if (m_context[2077] == 256)
						{
							m_context[2078] = decGolomb(0);
						}
						else
						{
							m_context[2078] = m_context[2076] + decGolomb(1);
						}
						m_context[2079] = -m_context[2078];
						numC = m_context[2078] - m_context[2079] + 1;
						if (numC > 1)
						{
							aColor0.set_alphabet(numC);
							aColor0.copy(context_aColor0, m_context[2079]);
						}
						if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
						if (numC >= PCM_threshold)
						{
							PCM_flag = 1;
							m_context[2074] = m_context[2072] + decGolomb(1);
							m_context[2075] = m_context[2073] + decGolomb(1);
							if (m_context[2074] != m_context[2075])
							{
								aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
								getValContext();
								aColor1.copy(context_aColor1 + m_context[2075]);
							}
						}
						else PCM_flag = 0;
						for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
						encode_numC = true;
					}
					if (location_col > 0) predict1 = pImage[pic_width*location_row + location_col - 1];
					if (location_row > 0) predict2 = pImage[pic_width*(location_row - 1) + location_col];
					if (location_col > 0 && location_row > 0)
					{
						int LL = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row)+location_col - 2]));
						int LT = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
						int TL = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
						int TT = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 2) + location_col]));
						int val = LL + TL - LT - TT;

						if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
						else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
						else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
						else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
						else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row)+location_col - 2] + pImage[pic_width*(location_row - 2) + location_col] + 4) / 8;

					}
					else if (location_col == 0 && location_row > 0) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
					else if (location_col > 0 && location_row == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
					if (numC == 1) color1 = predict1 + m_context[2079];
					else
					{
						int dif = 0;
						if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078] && color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
						{
							if (numC > 3)
							{
								int index = acodec.decode(aColor0, color1 - predict1 - m_context[2079], color2 - predict1 - m_context[2079]);
								dif = index + m_context[2079];// context_aColor0[dif + 256]++;
							}
							else dif = get_index(0, false, color1 - predict1 - m_context[2079], color2 - predict1 - m_context[2079]) + m_context[2079];
						}
						else if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078])
						{
							if (numC > 2)
							{
								dif = acodec.decode(aColor0, color1 - predict1 - m_context[2079]) + m_context[2079];// context_aColor0[dif + 256]++;
							}
							else dif = get_index(0, false, color1 - predict1 - m_context[2079]) + m_context[2079];
						}
						else if (color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
						{
							if (numC > 2)
							{
								dif = acodec.decode(aColor0, color2 - predict1 - m_context[2079]) + m_context[2079];// context_aColor0[dif + 256]++;
							}
							else dif = get_index(0, false, color2 - predict1 - m_context[2079]) + m_context[2079];
						}
						else { dif = acodec.decode(aColor0) + m_context[2079]; }// context_aColor0[dif + 256]++;}
						color1 = predict1 + dif;
					}
				}
			}
			else
			{
				if (!encode_numC)
				{
					if (m_context[2077] == 256)
					{
						m_context[2078] = decGolomb(0);
					}
					else
					{
						m_context[2078] = m_context[2076] + decGolomb(1);
					}
					m_context[2079] = -m_context[2078];
					numC = m_context[2078] - m_context[2079] + 1;
					if (numC > 1)
					{
						aColor0.set_alphabet(numC);
						aColor0.copy(context_aColor0, m_context[2079]);
					}
					if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
					if (numC >= PCM_threshold)
					{
						PCM_flag = 1;
						m_context[2074] = m_context[2072] + decGolomb(1);
						m_context[2075] = m_context[2073] + decGolomb(1);
						if (m_context[2074] != m_context[2075])
						{
							aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
							getValContext();
							aColor1.copy(context_aColor1 + m_context[2075]);
						}
					}
					else PCM_flag = 0;
					for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
					encode_numC = true;
				}
				if (location_col > 0) predict1 = pImage[pic_width*location_row + location_col - 1];
				if (location_row > 0) predict2 = pImage[pic_width*(location_row - 1) + location_col];
				if (location_col > 0 && location_row > 0)
				{
					int LL = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row)+location_col - 2]));
					int LT = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
					int TL = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
					int TT = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 2) + location_col]));
					int val = LL + TL - LT - TT;

					if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
					else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
					else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
					else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
					else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row)+location_col - 2] + pImage[pic_width*(location_row - 2) + location_col] + 4) / 8;

				}
				else if (location_col == 0 && location_row > 0) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
				else if (location_col > 0 && location_row == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
				if (numC == 1) color1 = predict1 + m_context[2079];
				else
				{
					int dif = 0;
					if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078])
					{
						if (numC > 2)
						{
							dif = acodec.decode(aColor0, color1 - predict1 - m_context[2079]) + m_context[2079];// context_aColor0[dif + 256]++;
						}
						else dif = get_index(0, false, color1 - predict1 - m_context[2079]) + m_context[2079];
					}
					else { dif = acodec.decode(aColor0) + m_context[2079]; }// context_aColor0[dif + 256]++;}
					color1 = predict1 + dif;
				}
			}
		}
	}

	for (int i = 0; i < m_height*m_width; i++)
	{
		m_value[i] = color1;
	}
	colorValue = color1;
	for (int i = 0; i < m_height; i++)
	{
		for (int j = 0; j < m_width; j++)
		{
			pImg[(i + location_row)*pic_width + j + location_col] = m_value[i * m_width + j];
		}
	}
}

int get_position_dec(int index, int pos1, int pos2, int pos3, int block_size)
{
	int item = 0;
	for (int i = 1; i < block_size; i++)
	{
		if (i == pos1 || i == pos2 || i == pos3) continue;
		if (item == index) return i;
		item++;
	}
}

void CodingUnit::deChainCoding(unsigned char*pImg)
{
	m_value = new unsigned char[m_height*m_width];
	int CUhSize = getBitSize(m_height);
	int CUwSize = getBitSize(m_width);
	int chainPositionMode2x2;
	int color1, color2;
	int predict = predictColor1(this, &color1, &color2);
	if (predict == 0)
	{
		color1 = g_value;
	}
	else if (predict < 0)
	{
		if (!encode_numC)
		{
			if (m_context[2077] == 256)
			{
				m_context[2078] = decGolomb(0);
			}
			else
			{
				m_context[2078] = m_context[2076] + decGolomb(1);
			}
			m_context[2079] = -m_context[2078];
			numC = m_context[2078] - m_context[2079] + 1;
			if (numC > 1)
			{
				aColor0.set_alphabet(numC);
				aColor0.copy(context_aColor0, m_context[2079]);
			}
			if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
			if (numC >= PCM_threshold)
			{
				PCM_flag = 1;
				m_context[2074] = m_context[2072] + decGolomb(1);
				m_context[2075] = m_context[2073] + decGolomb(1);
				aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
				getValContext();
				aColor1.copy(context_aColor1 + m_context[2075]);
			}
			else PCM_flag = 0;
			for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
			encode_numC = true;
		}
		if (location_col > 0) predict1 = pImage[pic_width*location_row + location_col - 1];
		if (location_row > 0) predict2 = pImage[pic_width*(location_row - 1) + location_col];
		if (location_col > 0 && location_row > 0)
		{
			int LL = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row)+location_col - 2]));
			int LT = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
			int TL = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
			int TT = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 2) + location_col]));
			int val = LL + TL - LT - TT;

			if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
			else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
			else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
			else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
			else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row)+location_col - 2] + pImage[pic_width*(location_row - 2) + location_col] + 4) / 8;

		}
		else if (location_col == 0 && location_row > 0) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
		else if (location_col > 0 && location_row == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
		if (numC == 1) color1 = predict1 + m_context[2079];
		else
		{
			int dif = acodec.decode(aColor0) + m_context[2079]; //context_aColor0[dif + 256]++;
			color1 = predict1 + dif;
		}
	}
	else
	{
		if (!acodec.decode(aPredictColor1Flag))
		{
			if (predict == 2)
			{
				if (acodec.decode(aPredictColor1Flag2)) color1 = color2;
				else
				{
					if (!encode_numC)
					{
						if (m_context[2077] == 256)
						{
							m_context[2078] = decGolomb(0);
						}
						else
						{
							m_context[2078] = m_context[2076] + decGolomb(1);
						}
						m_context[2079] = -m_context[2078];
						numC = m_context[2078] - m_context[2079] + 1;
						if (numC > 1)
						{
							aColor0.set_alphabet(numC);
							aColor0.copy(context_aColor0, m_context[2079]);
						}
						if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
						if (numC >= PCM_threshold)
						{
							PCM_flag = 1;
							m_context[2074] = m_context[2072] + decGolomb(1);
							m_context[2075] = m_context[2073] + decGolomb(1);
							aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
							getValContext();
							aColor1.copy(context_aColor1 + m_context[2075]);
						}
						else PCM_flag = 0;
						for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
						encode_numC = true;
					}
					if (location_col > 0) predict1 = pImage[pic_width*location_row + location_col - 1];
					if (location_row > 0) predict2 = pImage[pic_width*(location_row - 1) + location_col];
					if (location_col > 0 && location_row > 0)
					{
						int LL = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row)+location_col - 2]));
						int LT = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
						int TL = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
						int TT = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 2) + location_col]));
						int val = LL + TL - LT - TT;

						if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
						else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
						else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
						else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
						else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row)+location_col - 2] + pImage[pic_width*(location_row - 2) + location_col] + 4) / 8;

					}
					else if (location_col == 0 && location_row > 0) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
					else if (location_col > 0 && location_row == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
					if (numC == 1) color1 = predict1 + m_context[2079];
					else
					{
						int dif = 0;
						if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078] && color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
						{
							if (numC > 3)
							{
								int index = acodec.decode(aColor0, color1 - predict1 - m_context[2079], color2 - predict1 - m_context[2079]);
								dif = index + m_context[2079]; //context_aColor0[dif + 256]++;
							}
							else dif = get_index(0, false, color1 - predict1 - m_context[2079], color2 - predict1 - m_context[2079]) + m_context[2079];
						}
						else if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078])
						{
							if (numC > 2)
							{
								dif = acodec.decode(aColor0, color1 - predict1 - m_context[2079]) + m_context[2079];// context_aColor0[dif + 256]++;
							}
							else dif = get_index(0, false, color1 - predict1 - m_context[2079]) + m_context[2079];
						}
						else if (color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
						{
							if (numC > 2)
							{
								dif = acodec.decode(aColor0, color2 - predict1 - m_context[2079]) + m_context[2079];// context_aColor0[dif + 256]++;
							}
							else dif = get_index(0, false, color2 - predict1 - m_context[2079]) + m_context[2079];
						}
						else { dif = acodec.decode(aColor0) + m_context[2079]; }//context_aColor0[dif + 256]++;	}
						color1 = predict1 + dif;
					}
				}
			}
			else
			{
				if (!encode_numC)
				{
					if (m_context[2077] == 256)
					{
						m_context[2078] = decGolomb(0);
					}
					else
					{
						m_context[2078] = m_context[2076] + decGolomb(1);
					}
					m_context[2079] = -m_context[2078];
					numC = m_context[2078] - m_context[2079] + 1;
					if (numC > 1)
					{
						aColor0.set_alphabet(numC);
						aColor0.copy(context_aColor0, m_context[2079]);
					}
					if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
					if (numC >= PCM_threshold)
					{
						PCM_flag = 1;
						m_context[2074] = m_context[2072] + decGolomb(1);
						m_context[2075] = m_context[2073] + decGolomb(1);
						aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
						getValContext();
						aColor1.copy(context_aColor1 + m_context[2075]);
					}
					else PCM_flag = 0;
					for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
					encode_numC = true;
				}
				if (location_col > 0) predict1 = pImage[pic_width*location_row + location_col - 1];
				if (location_row > 0) predict2 = pImage[pic_width*(location_row - 1) + location_col];
				if (location_col > 0 && location_row > 0)
				{
					int LL = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row)+location_col - 2]));
					int LT = abs(int(pImage[pic_width*(location_row)+location_col - 1]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
					int TL = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 1) + location_col - 1]));
					int TT = abs(int(pImage[pic_width*(location_row - 1) + location_col]) - int(pImage[pic_width*(location_row - 2) + location_col]));
					int val = LL + TL - LT - TT;

					if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
					else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
					else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
					else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
					else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row)+location_col - 2] + pImage[pic_width*(location_row - 2) + location_col] + 4) / 8;

				}
				else if (location_col == 0 && location_row > 0) predict1 = (3 * predict2 + pImage[pic_width*(location_row - 2) + location_col] + 2) / 4;
				else if (location_col > 0 && location_row == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row)+location_col - 2] + 2) / 4;
				if (numC == 1) color1 = predict1 + m_context[2079];
				else
				{
					int dif = 0;
					if (color1 - predict1 >= m_context[2079] && color1 - predict1 <= m_context[2078])
					{
						if (numC > 2)
						{
							dif = acodec.decode(aColor0, color1 - predict1 - m_context[2079]) + m_context[2079]; //context_aColor0[dif + 256]++;
						}
						else dif = get_index(0, false, color1 - predict1 - m_context[2079]) + m_context[2079];
					}
					else { dif = acodec.decode(aColor0) + m_context[2079]; }// context_aColor0[dif + 256]++;}
					color1 = predict1 + dif;
				}
			}
		}
	}

	int initial_loc_y, initial_loc_x;

	int predictTrue = 0;
	int initial_loc_y2, initial_loc_x2, predictMode1 = -2, predictMode2 = -2;
	predict = predictInitialPosition(this, &initial_loc_y, &initial_loc_x, &initial_loc_y2, &initial_loc_x2, color1, &predictMode1, &predictMode2);
	int x1 = -1, x2 = -1, x3 = -1, y1 = -1, y2 = -1, y3 = -1, num0 = 0, num1 = 0;
	if (predict == 0 || predict == 2) get_predict_position(m_height, &y1, &y2, &y3, initial_loc_y, &num0);
	if (predict == 1 || predict == 2) get_predict_position(m_height, &x1, &x2, &x3, initial_loc_x2, &num1);
	if (predict == 1)
	{
		initial_loc_y = initial_loc_y2;
		initial_loc_x = initial_loc_x2;
	}
	if (predict >= 0)
	{
		predictTrue = acodec.decode(aPredictPositionFlag);
		if (predictTrue)
		{
			if (predict == 2)
			{
				if (acodec.decode(aPredictPositionSide))
				{
					initial_loc_y = initial_loc_y2;
					initial_loc_x = initial_loc_x2;
				}
			}
			if (initial_loc_x == 1 || initial_loc_x == m_width - 1 || initial_loc_y == 1 || initial_loc_y == m_height - 1)
			{
				if (acodec.decode(aPredictPositionBorder))
				{
					if (initial_loc_x == 1) initial_loc_x++;
					if (initial_loc_x == m_width - 1) initial_loc_x--;
					if (initial_loc_y == 1) initial_loc_y++;
					if (initial_loc_y == m_height - 1) initial_loc_y--;
				}
			}
			else
			{
				int PredictPositionMode = acodec.decode(aPredictPositionMode);
				if (PredictPositionMode == 1)
				{
					if (initial_loc_y == 0) initial_loc_x++;
					else initial_loc_y++;
				}
				if (PredictPositionMode == 2)
				{
					if (initial_loc_y == 0) initial_loc_x--;
					else initial_loc_y--;
				}
			}
		}
	}
	if (!predictTrue)
	{
		int sideDirection = 0;
		if (m_height == 4 && num0 == 3 && num1 != 3) sideDirection = acodec.decode(aSideMode, 0);
		else if (m_height == 4 && num0 != 3 && num1 == 3) sideDirection = acodec.decode(aSideMode, 1);
		else if (m_height == 4 && num0 == 3 && num1 == 3)
		{
			sideDirection = acodec.decode(aSideMode, 0, 1);
		}
		else sideDirection = acodec.decode(aSideMode);
		switch (sideDirection)
		{
		case 0:
			initial_loc_x = 0;
			if ((predict == 0 || predict == 2) && m_height == 4)
			{
				if (initial_loc_y == 1) initial_loc_y = 3;
				else initial_loc_y = 1;
			}
			else
			{
				if (predictMode1 == 1 && y1 >= m_height / 2 && y1 < m_height - 2)
				{
					if (acodec.decode(aPredictPosition))
					{
						int num = m_height - y1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							initial_loc_y = acodec.decode(Position) + y1 + 2;
						}
						else initial_loc_y = m_height - 1;
					}
					else
					{
						int num = y1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							initial_loc_y = -int(acodec.decode(Position)) + y1 - 2;
						}
						else initial_loc_y = 1;
					}
				}
				else if (predictMode1 == -1 && y1 <= m_height / 2 && y1 > 2)
				{
					if (acodec.decode(aPredictPosition))
					{
						int num = y1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							initial_loc_y = -int(acodec.decode(Position)) + y1 - 2;
						}
						else initial_loc_y = 1;
					}
					else
					{
						int num = m_height - y1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							initial_loc_y = acodec.decode(Position) + y1 + 2;
						}
						else initial_loc_y = m_height - 1;
					}
				}
				else
				{
					Static_Data_Model Position(m_height - 1 - num0);
					int index = acodec.decode(Position);
					initial_loc_y = get_position_dec(index, y1, y2, y3, m_height);
				}
			}
			break;
		case 1:
			if ((predict == 1 || predict == 2) && m_width == 4)
			{
				if (initial_loc_x2 == 1) initial_loc_x = 3;
				else initial_loc_x = 1;
			}
			else
			{
				if (predictMode2 == 1 && x1 >= m_height / 2 && x1 < m_height - 2)
				{
					if (acodec.decode(aPredictPosition))
					{
						int num = m_height - x1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							initial_loc_x = acodec.decode(Position) + x1 + 2;
						}
						else initial_loc_x = m_height - 1;
					}
					else
					{
						int num = x1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							initial_loc_x = -int(acodec.decode(Position)) + x1 - 2;
						}
						else initial_loc_x = 1;
					}
				}
				else if (predictMode2 == -1 && x1 <= m_height / 2 && x1 > 2)
				{
					if (acodec.decode(aPredictPosition))
					{
						int num = x1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							initial_loc_x = -int(acodec.decode(Position)) + x1 - 2;
						}
						else initial_loc_x = 1;
					}
					else
					{
						int num = m_height - x1 - 2;
						if (num > 1)
						{
							Static_Data_Model Position(num);
							initial_loc_x = acodec.decode(Position) + x1 + 2;
						}
						else initial_loc_x = m_height - 1;
					}
				}
				else
				{
					Static_Data_Model Position(m_height - 1 - num1);
					int index = acodec.decode(Position);
					initial_loc_x = get_position_dec(index, x1, x2, x3, m_height);
				}
			}
			initial_loc_y = 0;
			break;
		case 2:
		{
			Static_Data_Model Position(m_height - 1);
			initial_loc_x = acodec.decode(Position) + 1;
			initial_loc_y = m_height - 1;
		}
		break;
		case 3:
		{
			initial_loc_x = m_width - 1;
			Static_Data_Model Position(m_height - 2);
			initial_loc_y = acodec.decode(Position) + 1;
		}
		break;
		}
	}

	color2 = predictColor2(this, initial_loc_y, initial_loc_x, color1);
	if (color2 == -1)
	{
		if (!encode_numC)
		{
			if (m_context[2077] == 256)
			{
				m_context[2078] = decGolomb(0);
			}
			else
			{
				m_context[2078] = m_context[2076] + decGolomb(1);
			}
			m_context[2079] = -m_context[2078];
			numC = m_context[2078] - m_context[2079] + 1;
			if (numC > 1)
			{
				aColor0.set_alphabet(numC);
				aColor0.copy(context_aColor0, m_context[2079]);
			}
			if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
			if (numC >= PCM_threshold)
			{
				PCM_flag = 1;
				m_context[2074] = m_context[2072] + decGolomb(1);
				m_context[2075] = m_context[2073] + decGolomb(1);
				aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
				getValContext();
				aColor1.copy(context_aColor1 + m_context[2075]);
			}
			else PCM_flag = 0;
			for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
			encode_numC = true;
		}
		predict1 = color1;
		if (numC == 1) color2 = predict1 + m_context[2079];
		else
		{
			int dif = 0;
			if (0 >= m_context[2079] && 0 <= m_context[2078])
			{
				if (numC > 2)
				{
					dif = acodec.decode(aColor0, -m_context[2079]) + m_context[2079];// context_aColor0[dif + 256]++;
				}
				else dif = get_index(0, false, -m_context[2079]) + m_context[2079];
			}
			else { dif = acodec.decode(aColor0) + m_context[2079]; }//context_aColor0[dif + 256]++; }

			color2 = predict1 + dif;
		}
	}
	else
	{
		int flag = acodec.decode(aPredictColor2Flag);
		if (!flag)
		{
			if (!encode_numC)
			{
				if (m_context[2077] == 256)
				{
					m_context[2078] = decGolomb(0);
				}
				else
				{
					m_context[2078] = m_context[2076] + decGolomb(1);
				}
				m_context[2079] = -m_context[2078];
				numC = m_context[2078] - m_context[2079] + 1;
				if (numC > 1)
				{
					aColor0.set_alphabet(numC);
					aColor0.copy(context_aColor0, m_context[2079]);
				}
				if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
				if (numC >= PCM_threshold)
				{
					PCM_flag = 1;
					m_context[2074] = m_context[2072] + decGolomb(1);
					m_context[2075] = m_context[2073] + decGolomb(1);
					aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
					getValContext();
					aColor1.copy(context_aColor1 + m_context[2075]);
				}
				else PCM_flag = 0;
				for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
				encode_numC = true;
			}
			predict1 = color1;
			if (numC == 1) color2 = predict1 + m_context[2079];
			else
			{
				int dif = 0;
				if (color2 - predict1 >= m_context[2079] && color2 - predict1 <= m_context[2078])
				{
					if (numC > 2)
					{
						if (0 >= m_context[2079] && 0 <= m_context[2078])
						{
							if (numC > 3)
							{
								int index = acodec.decode(aColor0, color2 - predict1 - m_context[2079], -m_context[2079]);
								dif = index + m_context[2079];// context_aColor0[dif + 256]++;
							}
							else dif = get_index(0, false, color2 - predict1 - m_context[2079], -m_context[2079]) + m_context[2079];
						}
						else
						{
							dif = acodec.decode(aColor0, color2 - predict1 - m_context[2079]) + m_context[2079];// context_aColor0[dif + 256]++;
						}
					}
					else dif = get_index(0, false, color2 - predict1 - m_context[2079]) + m_context[2079];
				}
				else
				{
					if (0 >= m_context[2079] && 0 <= m_context[2078])
					{
						if (numC > 2)
						{
							dif = acodec.decode(aColor0, -m_context[2079]) + m_context[2079];// context_aColor0[dif + 256]++;
						}
						else dif = get_index(0, false, -m_context[2079]) + m_context[2079];
					}
					else { dif = acodec.decode(aColor0) + m_context[2079]; }// context_aColor0[dif + 256]++;}
				}

				color2 = predict1 + dif;
			}
		}
	}

	int color3;
	for (int i = 0; i < 3; i++)
	{
		if (i != color1 && i != color2) color3 = i;
	}
	for (int i = 0; i < m_height*m_width; i++)
	{
		m_value[i] = color3;
	}

	int OT3_0 = -1;
	int OT3_1 = -1;
	int OT3_2 = -1;
	int OT3;
	int mode = 1;
	if (initial_loc_x == 0 || initial_loc_y == m_height - 1) mode = 0;
	if ((initial_loc_x == 0 && location_col > 0) || (initial_loc_y == 0 && location_row > 0))
	{
		int edgeline[3];
		get_context(pImg, location_row, location_col, pic_width, m_width, initial_loc_y, initial_loc_x, color2, edgeline, &context, &mode);
		for (int i = 0; i < context; i++)
		{
			update_context(&OT3_0, &OT3_1, &OT3_2, edgeline[i]);
		}
	}


	int loc_y = initial_loc_y;
	int loc_x = initial_loc_x;

	int direction;
	if (initial_loc_x == 0)
	{
		direction = 0; loc_x++;
		m_value[loc_y * m_width + loc_x - 1] = color2;
	}
	else if (initial_loc_y == 0)
	{
		direction = 3; loc_y++;
		m_value[(loc_y - 1) * m_width + loc_x] = color2;
	}
	else if (initial_loc_y == m_height - 1)
	{
		direction = 1;
		m_value[loc_y * m_width + loc_x] = color2;
	}
	else
	{
		direction = 2;
		m_value[loc_y * m_width + loc_x] = color2;
	}

	int index = context;
	int forbidden = -1;
	int forbid_direction = 0;
	unsigned char* ChainPath = new unsigned char[(m_height + 1) * (m_width + 1)];
	for (int i = 0; i < (m_height + 1) * (m_width + 1); i++) ChainPath[i] = 0;

	do
	{
		index += 1;
		forbidden = -1;
		if (initial_loc_x == 0)
		{
			if (loc_x == 1 && loc_y < initial_loc_y)
			{
				if (direction == 1) forbidden = 2;
				if (direction == 2) { forbidden = 4; forbid_direction = 1; }
			}
		}
		else if (initial_loc_y == 0)
		{
			if (loc_x == 1)
			{
				if (direction == 2) { forbidden = 4; forbid_direction = 3; }
				if (direction == 3) forbidden = 2;
			}
			else if (loc_x < initial_loc_x && loc_y == 1)
			{
				if (direction == 1) { forbidden = 4; forbid_direction = 2; }
				if (direction == 2) forbidden = 1;
			}
		}
		else if (initial_loc_y == m_height - 1)
		{
			if (loc_x == 1)
			{
				if (direction == 1) forbidden = 2;
				if (direction == 2) { forbidden = 4; forbid_direction = 1; }
			}
			else if (loc_y == 1)
			{
				if (direction == 0) forbidden = 1;
				if (direction == 1) { forbidden = 4; forbid_direction = 0; }
			}
			else if (loc_x < initial_loc_x && loc_y == m_height - 1)
			{
				if (direction == 2) forbidden = 3;
				if (direction == 3) { forbidden = 4; forbid_direction = 2; }
			}
		}
		else
		{
			if (loc_x == 1)
			{
				if (direction == 2) { forbidden = 4; forbid_direction = 3; }
				if (direction == 3) forbidden = 2;
			}
			else if (loc_y == 1)
			{
				if (direction == 1) { forbidden = 4; forbid_direction = 2; }
				if (direction == 2) forbidden = 1;
			}
			else if (loc_y == m_height - 1)
			{
				if (direction == 0) forbidden = 3;
				if (direction == 3) { forbidden = 4; forbid_direction = 0; }
			}
			else if (loc_x == m_width - 1 && loc_y < initial_loc_y)
			{
				if (direction == 0) { forbidden = 4; forbid_direction = 1; }
				if (direction == 1) forbidden = 0;
			}
		}

		ChainPath[loc_y * (m_width + 1) + loc_x] = 1;
		int forbidden1, forbidden2;
		for (int i = 0; i < 4; i++)
		{
			if (i == 2) continue;
			else if (i == 0) { forbidden1 = (direction + 1) % 4; forbidden2 = (direction + 3) % 4; }
			else if (i == 1) { forbidden1 = direction; forbidden2 = (direction + 3) % 4; }
			else { forbidden1 = direction; forbidden2 = (direction + 1) % 4; }
			switch ((direction + i) % 4)
			{
			case 0: if (ChainPath[loc_y * (m_width + 1) + loc_x + 1] == 1) { if (forbidden == forbidden1 || forbidden == forbidden2) { if (forbidden == forbidden1) forbid_direction = forbidden2; else forbid_direction = forbidden1; forbidden = 4; } else if (forbidden != 4) forbidden = 0; }break;
			case 1: if (ChainPath[(loc_y - 1) * (m_width + 1) + loc_x] == 1) { if (forbidden == forbidden1 || forbidden == forbidden2) { if (forbidden == forbidden1) forbid_direction = forbidden2; else forbid_direction = forbidden1; forbidden = 4; } else if (forbidden != 4) forbidden = 1; }break;
			case 2: if (ChainPath[loc_y * (m_width + 1) + loc_x - 1] == 1) { if (forbidden == forbidden1 || forbidden == forbidden2) { if (forbidden == forbidden1) forbid_direction = forbidden2; else forbid_direction = forbidden1; forbidden = 4; } else if (forbidden != 4) forbidden = 2; }break;
			case 3: if (ChainPath[(loc_y + 1) * (m_width + 1) + loc_x] == 1) { if (forbidden == forbidden1 || forbidden == forbidden2) { if (forbidden == forbidden1) forbid_direction = forbidden2; else forbid_direction = forbidden1; forbidden = 4; } else if (forbidden != 4) forbidden = 3; }
			}
		}

		if (forbidden == 4)
		{
			if (forbid_direction - direction == 1 || forbid_direction - direction == -3)  //left
			{
				if (mode == 0) OT3 = 2;
				if (mode == 1) OT3 = 1;
			}
			else if (forbid_direction - direction == -1 || forbid_direction - direction == 3) //right
			{
				if (mode == 1) OT3 = 2;
				if (mode == 0) OT3 = 1;
			}
			else
			{
				OT3 = 0;
			}
			if (OT3 == 1) mode = 1 - mode;
			direction = forbid_direction;
			switch (direction)
			{
			case 0:loc_x++; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[loc_y * m_width + loc_x - 1] = color2; else m_value[(loc_y - 1) * m_width + loc_x - 1] = color2; break;
			case 1:loc_y--; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[loc_y * m_width + loc_x] = color2; else m_value[loc_y * m_width + loc_x - 1] = color2; break;
			case 2:loc_x--; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[(loc_y - 1) * m_width + loc_x] = color2; else m_value[loc_y * m_width + loc_x] = color2; break;
			case 3:loc_y++; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[(loc_y - 1) * m_width + loc_x - 1] = color2; else m_value[(loc_y - 1) * m_width + loc_x] = color2;
			}
		}
		else
		{
			if (forbidden >= 0)
			{
				if (forbidden - direction == 1 || forbidden - direction == -3)  //left
				{
					if (mode == 0) forbidden = 2;
					if (mode == 1) forbidden = 1;
				}
				else if (forbidden - direction == -1 || forbidden - direction == 3) //right
				{
					if (mode == 1) forbidden = 2;
					if (mode == 0) forbidden = 1;
				}
				else
				{
					forbidden = 0;
				}
			}

			if (index == 1)
			{
				if (initial_loc_x == 0) OT3 = acodec.decode(a3OT_L, forbidden);
				else if (initial_loc_y == 0) OT3 = acodec.decode(a3OT_A, forbidden);
				else if (initial_loc_y == m_height - 1) OT3 = acodec.decode(a3OT_B, forbidden);
				else OT3 = acodec.decode(a3OT_R, forbidden);
			}
			else if (index == 2)
			{
				switch (OT3_0)
				{
				case 0:OT3 = acodec.decode(a3OT_0, forbidden); break;
				case 1:OT3 = acodec.decode(a3OT_1, forbidden); break;
				case 2:OT3 = acodec.decode(a3OT_2, forbidden);
				}
			}
			else if (index == 3)
			{
				switch (OT3_0 * 3 + OT3_1)
				{
				case 0:OT3 = acodec.decode(a3OT_00, forbidden); break;
				case 1:OT3 = acodec.decode(a3OT_01, forbidden); break;
				case 2:OT3 = acodec.decode(a3OT_02, forbidden); break;
				case 3:OT3 = acodec.decode(a3OT_10, forbidden); break;
				case 4:OT3 = acodec.decode(a3OT_11, forbidden); break;
				case 5:OT3 = acodec.decode(a3OT_12, forbidden); break;
				case 6:OT3 = acodec.decode(a3OT_20, forbidden); break;
				case 7:OT3 = acodec.decode(a3OT_21, forbidden); break;
				case 8:OT3 = acodec.decode(a3OT_22, forbidden);
				}
			}
			else
			{
				switch (OT3_0 * 9 + OT3_1 * 3 + OT3_2)
				{
				case 0:OT3 = acodec.decode(a3OT_000, forbidden); break;
				case 1:OT3 = acodec.decode(a3OT_001, forbidden); break;
				case 2:OT3 = acodec.decode(a3OT_002, forbidden); break;
				case 3:OT3 = acodec.decode(a3OT_010, forbidden); break;
				case 4:OT3 = acodec.decode(a3OT_011, forbidden); break;
				case 5:OT3 = acodec.decode(a3OT_012, forbidden); break;
				case 6:OT3 = acodec.decode(a3OT_020, forbidden); break;
				case 7:OT3 = acodec.decode(a3OT_021, forbidden); break;
				case 8:OT3 = acodec.decode(a3OT_022, forbidden); break;
				case 9:OT3 = acodec.decode(a3OT_100, forbidden); break;
				case 10:OT3 = acodec.decode(a3OT_101, forbidden); break;
				case 11:OT3 = acodec.decode(a3OT_102, forbidden); break;
				case 12:OT3 = acodec.decode(a3OT_110, forbidden); break;
				case 13:OT3 = acodec.decode(a3OT_111, forbidden); break;
				case 14:OT3 = acodec.decode(a3OT_112, forbidden); break;
				case 15:OT3 = acodec.decode(a3OT_120, forbidden); break;
				case 16:OT3 = acodec.decode(a3OT_121, forbidden); break;
				case 17:OT3 = acodec.decode(a3OT_122, forbidden); break;
				case 18:OT3 = acodec.decode(a3OT_200, forbidden); break;
				case 19:OT3 = acodec.decode(a3OT_201, forbidden); break;
				case 20:OT3 = acodec.decode(a3OT_202, forbidden); break;
				case 21:OT3 = acodec.decode(a3OT_210, forbidden); break;
				case 22:OT3 = acodec.decode(a3OT_211, forbidden); break;
				case 23:OT3 = acodec.decode(a3OT_212, forbidden); break;
				case 24:OT3 = acodec.decode(a3OT_220, forbidden); break;
				case 25:OT3 = acodec.decode(a3OT_221, forbidden); break;
				case 26:OT3 = acodec.decode(a3OT_222, forbidden);
				}
			}

			if (OT3 == 0)
			{
				switch (direction)
				{
				case 0:loc_x++; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[loc_y * m_width + loc_x - 1] = color2; else m_value[(loc_y - 1) * m_width + loc_x - 1] = color2; break;
				case 1:loc_y--; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[loc_y * m_width + loc_x] = color2; else m_value[loc_y * m_width + loc_x - 1] = color2; break;
				case 2:loc_x--; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[(loc_y - 1) * m_width + loc_x] = color2; else m_value[loc_y * m_width + loc_x] = color2; break;
				case 3:loc_y++; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[(loc_y - 1) * m_width + loc_x - 1] = color2; else m_value[(loc_y - 1) * m_width + loc_x] = color2;
				}
			}
			else
			{
				if (OT3 == 1) mode = 1 - mode;
				if (mode == 0)
				{
					switch (direction)
					{
					case 0:direction = 1; loc_y--; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[loc_y * m_width + loc_x] = color2; else m_value[loc_y * m_width + loc_x - 1] = color2; break;
					case 1:direction = 2; loc_x--; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[(loc_y - 1) * m_width + loc_x] = color2; else m_value[loc_y * m_width + loc_x] = color2; break;
					case 2:direction = 3; loc_y++; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[(loc_y - 1) * m_width + loc_x - 1] = color2; else m_value[(loc_y - 1) * m_width + loc_x] = color2; break;
					case 3:direction = 0; loc_x++; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[loc_y * m_width + loc_x - 1] = color2; else m_value[(loc_y - 1) * m_width + loc_x - 1] = color2;
					}
				}
				else
				{
					switch (direction)
					{
					case 0:direction = 3; loc_y++; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[(loc_y - 1) * m_width + loc_x - 1] = color2; else m_value[(loc_y - 1) * m_width + loc_x] = color2; break;
					case 1:direction = 0; loc_x++; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[loc_y * m_width + loc_x - 1] = color2; else m_value[(loc_y - 1) * m_width + loc_x - 1] = color2; break;
					case 2:direction = 1; loc_y--; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[loc_y * m_width + loc_x] = color2; else m_value[loc_y * m_width + loc_x - 1] = color2; break;
					case 3:direction = 2; loc_x--; if (initial_loc_x == 0 || initial_loc_y == m_height - 1) m_value[(loc_y - 1) * m_width + loc_x] = color2; else m_value[loc_y * m_width + loc_x] = color2;
					}
				}
			}
		}
		update_context(&OT3_0, &OT3_1, &OT3_2, OT3);
	} while (!(loc_x == m_width || loc_y == 0 || loc_x == 0 || loc_y == m_height));

	advancedExpand(m_value, m_height, m_width, color1, 0, 0);

	for (int i = 0; i < m_height*m_width; i++)
	{
		if (m_value[i] == color3) m_value[i] = color2;
	}
	delete[] ChainPath;

	for (int i = 0; i < m_height; i++)
	{
		for (int j = 0; j < m_width; j++)
		{
			pImg[(i + location_row)*pic_width + j + location_col] = m_value[i * m_width + j];
		}
	}
}

void CodingUnit::deRunLength(unsigned char*pImg)
{
	m_value = new unsigned char[m_height*m_width];
	if (!encode_numC)
	{
		if (m_context[2077] == 256)
		{
			m_context[2078] = decGolomb(0);
		}
		else
		{
			m_context[2078] = m_context[2076] + decGolomb(1);
		}
		m_context[2079] = -m_context[2078];
		numC = m_context[2078] - m_context[2079] + 1;
		if (numC > 1)
		{
			aColor0.set_alphabet(numC);
			aColor0.copy(context_aColor0, m_context[2079]);
		}
		if (location_row / g_max_CU == 0 && location_col / g_max_CU == 0) { m_context[2072] = g_value + m_context[2078]; m_context[2073] = g_value + m_context[2079]; }
		if (numC >= PCM_threshold)
		{
			PCM_flag = 1;
			m_context[2074] = m_context[2072] + decGolomb(1);
			m_context[2075] = m_context[2073] + decGolomb(1);
			aColor1.set_alphabet(m_context[2074] - m_context[2075] + 1);
			getValContext();
			aColor1.copy(context_aColor1 + m_context[2075]);
		}
		else PCM_flag = 0;
		for (int i = 0; i < 520 * 4 - 8; i++) m_context[i] = 1;
		encode_numC = true;
	}

	if (PCM_flag)
	{
		if (acodec.decode(aPCM))
		{
			for (int i = 0; i < m_height; i++)
			{
				for (int j = 0; j < m_width; j++)
				{
					if (location_row + i == 0 && location_col + j == 0)
					{
						m_value[i * m_width + j] = g_value;
						pImg[(i + location_row)*pic_width + j + location_col] = m_value[i * m_width + j];
						continue;
					}
					m_value[i * m_width + j] = acodec.decode(aColor1) + m_context[2075];
					pImg[(i + location_row)*pic_width + j + location_col] = m_value[i * m_width + j];

					if (location_col + j > 0) predict1 = pImage[pic_width*(location_row + i) + location_col + j - 1];
					if (location_row + i > 0) predict2 = pImage[pic_width*(location_row + i - 1) + location_col + j];
					if (location_col + j > 0 && location_row + i > 0)
					{
						if (location_col + j > 1 && location_row + i > 1)
						{
							int LL = abs(int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - int(pImage[pic_width*(location_row + i) + location_col + j - 2]));
							int LT = abs(int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - int(pImage[pic_width*(location_row + i - 1) + location_col + j - 1]));
							int TL = abs(int(pImage[pic_width*(location_row + i - 1) + location_col + j]) - int(pImage[pic_width*(location_row + i - 1) + location_col + j - 1]));
							int TT = abs(int(pImage[pic_width*(location_row + i - 1) + location_col + j]) - int(pImage[pic_width*(location_row + i - 2) + location_col + j]));
							int val = LL + TL - LT - TT;

							if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
							else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
							else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + 2) / 4;
							else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + 2) / 4;
							else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + pImage[pic_width*(location_row + i - 2) + location_col + j] + 4) / 8;
						}

						else predict1 = (predict1 + predict2 + 1) / 2;
					}
					else if (location_col + j == 0 && location_row + i > 1) predict1 = (3 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
					else if (location_col + j == 0 && location_row + i == 1) predict1 = predict2;
					else if (location_col + j > 1 && location_row + i == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + 2) / 4;

					context_aColor0[int(m_value[i*m_width + j]) - predict1 + 256]++;


				}
			}
			return;
		}
	}

		for (int i = 0; i < m_height; i++)
		{
			for (int j = 0; j < m_width; j++)
			{
				if (location_col + j == 0 && location_row + i == 0)
				{
					m_value[i * m_width + j] = g_value;
					pImg[(i + location_row)*pic_width + j + location_col] = m_value[i * m_width + j];
					continue;
				}

				if (location_col + j > 0) predict1 = pImage[pic_width*(location_row + i) + location_col + j - 1];
				if (location_row + i > 0) predict2 = pImage[pic_width*(location_row + i - 1) + location_col + j];
				if (location_col + j > 0 && location_row + i > 0)
				{
					if (location_col + j > 1 && location_row + i > 1)
					{
						int LL = abs(int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - int(pImage[pic_width*(location_row + i) + location_col + j - 2]));
						int LT = abs(int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - int(pImage[pic_width*(location_row + i - 1) + location_col + j - 1]));
						int TL = abs(int(pImage[pic_width*(location_row + i - 1) + location_col + j]) - int(pImage[pic_width*(location_row + i - 1) + location_col + j - 1]));
						int TT = abs(int(pImage[pic_width*(location_row + i - 1) + location_col + j]) - int(pImage[pic_width*(location_row + i - 2) + location_col + j]));
						int val = LL + TL - LT - TT;

						if (val >= 8) predict1 = (3 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
						else if (val >= 4) predict1 = (predict1 + 2 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
						else if (val <= -8) predict1 = (3 * predict1 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + 2) / 4;
						else if (val <= -4) predict1 = (predict2 + 2 * predict1 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + 2) / 4;
						else predict1 = (3 * predict1 + 3 * predict2 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + pImage[pic_width*(location_row + i - 2) + location_col + j] + 4) / 8;
					}

					else predict1 = (predict1 + predict2 + 1) / 2;
				}
				else if (location_col + j == 0 && location_row + i > 1) predict1 = (3 * predict2 + pImage[pic_width*(location_row + i - 2) + location_col + j] + 2) / 4;
				else if (location_col + j == 0 && location_row + i == 1) predict1 = predict2;
				else if (location_col + j > 1 && location_row + i == 0) predict1 = (3 * predict1 + pImage[pic_width*(location_row + i) + location_col - 2 + j] + 2) / 4;


				if (location_row + i > 0 && location_col + j > 0 && pImage[pic_width*(location_row + i) + location_col + j - 1] == pImage[pic_width*(location_row + i - 1) + location_col + j] && numC > 1)
				{
					if (acodec.decode(aPredictColorRunFlag)) { m_value[i * m_width + j] = pImage[pic_width*(location_row + i) + location_col + j - 1]; context_element[offset_PredictColorRunFlag + 1]++;}
					else
					{
						context_element[offset_PredictColorRunFlag]++;
						int dif = 0;
						int forbidden = int(pImage[pic_width*(location_row + i) + location_col + j - 1]) - predict1;
						int index_forbidden = -1;
						if (forbidden >= m_context[2079] && forbidden <= m_context[2078]) index_forbidden = forbidden - m_context[2079];
						if (numC > 2)
						{
							int index;

							index = acodec.decode(aColor0, index_forbidden);
							dif = index + m_context[2079];

						}
						else if (numC > 1)
						{
							if (index_forbidden == 0) dif = 1 + m_context[2079];
							else dif = m_context[2079];
						}
						else dif = m_context[2079];
						m_value[i * m_width + j] = predict1 + dif;
					}
				}
				else
				{
					int dif = 0;
					if (numC > 1)
					{
						int index;
						index = acodec.decode(aColor0);
						dif = index + m_context[2079]; 
					}
					else dif = m_context[2079];

					m_value[i * m_width + j] = predict1 + dif;
				}
				if (numC > 1) context_aColor0[int(m_value[i * m_width + j]) - predict1 + 256]++;


				pImg[(i + location_row)*pic_width + j + location_col] = m_value[i * m_width + j];

				if (PCM_flag) acodec.nodecode(aColor1, int(m_value[i * m_width + j]) - m_context[2075]);
			}
		}
	//printf("%d ", location_row);
	//printf("%d \n", location_col);
	//for (int i = 0; i < m_height; i++)
	//{
	//	for (int j = 0; j < m_width; j++)
	//	{
	//		printf("%d ", m_value[i * m_width + j]);
	//	}
	//	printf("\n");
	//}
	//printf("\n");
}

