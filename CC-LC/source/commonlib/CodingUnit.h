#pragma once
#include "global_arithmetic.h"
#include <iostream> 
using namespace std;

class CodingUnit
{
public:
	CodingUnit();
	~CodingUnit();
	void create(unsigned char* pImg, int h, int w, int CU_h, int CU_w, int loc_row, int loc_col, int dep);
	void create(unsigned char* pImg, int h, int w, int CU_h, int CU_w, int loc_row, int loc_col, int dep, int max_cu);
	void destroy(CodingUnit* CU);
	int getWidth();
	int getHeight();
	int getDepth();
	void getValContext();
	unsigned char* getValue();
	unsigned char* getImg();
	int getColorValue();
	void setColorValue(int color);
	int* getEdge();
	int getEdgeLength();
	void setEdge();
	void setRunLength();
	int* getRunLength();

    void enSplitFlag();
	void enOneColorFlag(int pos);
	void enFlag(int pos);
	void enContinueEdgeFlag();
	void enOneColor();
	void enChainCoding();
	void enRunLength();

    int deSplitFlag();
	int deOneColorFlag(int pos);
	int deFlag(int pos);
	int deContinueEdgeFlag();
	void deOneColor(unsigned char*pImg);
	void deChainCoding(unsigned char*pImg);
	void deRunLength(unsigned char*pImg);

	int pic_width;
	int pic_height;
	bool split_flag;
	bool oneColor;
	bool contiEdge;
	int location_row;
	int location_col;
	int context;
	int mode;   // left:0  right:1
	CodingUnit* child[4];
	CodingUnit* father;
	int* ForbiddenLine;

	int predict1;
	int predict2;

	int* m_context;    // 0-519  color0   520-999 element 2072 predict_max 2073 predict_min  2074 max 2075 min 2076 dif_predict_max 2077 dif_predict_min 2078 dif_max 2079 dif_min
	int* context_aColor0;
	int* context_element;
	int* context_aColor1;

private:
	unsigned char *m_value;
	unsigned char *pImage;
	int m_height;
	int m_width;
	int m_depth;
	int colorValue;
	int* Edge;        //color1  color2_y  color2_x  color2  chaincode
	int edgeLength;
	int runLengthMode; //0:horizontal 1:horizontal S-shaped 2:vertical 3:vertical S-shaped 
					   //4:lefttop horizontal first S-shaped 5:lefttop vertical first S-shaped 6:righttop horizontal first S-shaped 7:righttop vertical first S-shaped
	int* runLength;   //value1 value1_length value2 value2_length ... 

};


