#include "encOptions.h"
#include <iostream>
#include <cstring>  
#include <cstdio>
using namespace std;

#ifdef __unix
 
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename), (mode)))==NULL
 
#endif

int readOptions(int argc, char* argv[], char* filein, char*fileout, int* rows, int* cols, int* frameNum, int* skip, int* type, int* mask_flag, int* remove_out, int* adapt, char*fileada)
{
	int i;
	// get input para
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'h':
				cout << " There are 7 kinds of commands.\n -h(help information) -i(input file which only support .yuv now) -o(output file) -r(row number of the graph) -c(col number of the graph) \
					-f(number of frames) -s(skip number of frames) -t(type of video include 400 and 420) -m(whether use inter prediction) -M(whether print quadtree partition)\
           \nfor example: -i D:/input.yuv -r 1024 -c 2048 -f 1 -t 400 -o output.bin " << endl;
				return -1;
				break;
			case 'i':
				i++;
				strcpy(filein, argv[i]);
				if (filein[strlen(argv[i]) - 1] != 'v' || filein[strlen(argv[i]) - 2] != 'u' || filein[strlen(argv[i]) - 3] != 'y' || filein[strlen(argv[i]) - 4] != '.')
				{
					cout << "only support .yuv now" << endl;
					return -1;
				}
				break;
			case 'o':
				i++;
				strcpy(fileout, argv[i]);
				break;
			case 'r':
				sscanf(argv[++i], "%d", rows);
				break;
			case 'c':
				sscanf(argv[++i], "%d", cols);
				break;
			case 'f':
				sscanf(argv[++i], "%d", frameNum);
				break;
			case 's':
				sscanf(argv[++i], "%d", skip);
				break;
			case 't':
				sscanf(argv[++i], "%d", type);
				break;
			case 'M':
				sscanf(argv[++i], "%d", mask_flag);
				break;
			case 'R':
				sscanf(argv[++i], "%d", remove_out);
				break;
			case 'a':
				sscanf(argv[++i], "%d", adapt);
				break;
			case 'A':
				i++;
				strcpy(fileada, argv[i]);
				break;
			default:
				cout << "unsupport command exist" << endl;
				cout << " There are 7 kinds of commands.\n -h(help information) -i(input file which only support .yuv now) -o(output file) \
					-r(row number of the graph) -c(col number of the graph) -f(number of frames) -t(type of video include 400 and 420) \
          \nfor example: -i D:/input.yuv -r 1024 -c 2048 -f 1 -t 400 -o output.bin " << endl;
				return -1;
				break;
			}
		}
	}
	return 0;
}
