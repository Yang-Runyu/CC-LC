#include "decOptions.h"
#include <iostream>  
#include <cstring>  
#include <cstdio>
using namespace std;

#ifdef __unix
 
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename), (mode)))==NULL
 
#endif

int readOptions(int argc, char* argv[], char* filein, char*fileout)
{
	int i;
	// get input para
	for (i = 1; i<argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'h':
				cout << " There are three kinds of commands.\n -h(help information) -i(input file which created by our encoder) -o(output file which is .yuv) \
					\nfor example: -i output.bin -o recover.yuv " << endl;
				return -1;
				break;
			case 'i':
				i++;
				strcpy(filein, argv[i]);
				break;
			case 'o':
				i++;
				strcpy(fileout, argv[i]);
				break;
			default:
				cout << "unsupport command exist" << endl;
				cout << " There are three kinds of commands.\n -h(help information) -i(input file which created by our encoder) -o(output file which is .yuv) \
					\nfor example: -i output.bin -o recover.yuv " << endl;
				return -1;
				break;
			}
		}
	}
	return 0;
}
