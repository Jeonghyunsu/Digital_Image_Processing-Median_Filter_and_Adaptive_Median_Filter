#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <algorithm>
#include <string>
#include <chrono>

#define BASESIZE_HEIGHT 512
#define BASESIZE_WIDTH 512
#define NFILTERSIZE 3			//Filter의 Basic Size
#define NFILTERSIZE_MAX 11		//Adaptive Filter의 Max Size
#define SALT 0.1			    //Salt의 확률
#define PEPPER 0.1				//Pepper의 확률

using namespace std;

void MedianFilter(unsigned char **Img_in, unsigned char **Out, int nHeight, int nWidth, int nFilterSize);
void AdaptiveMedianFilter(unsigned char **Img_in, unsigned char **Out, int nHeight, int nWidth, int nFilterSize, int nFilterSize_Max);
unsigned char** Padding(unsigned char** In, int nHeight, int nWidth, int nFilterSize);
void InputSaltPepperNoise(unsigned char** In, unsigned char** Out, int nHeight, int nWidth, float fSProb, float fPProb);
unsigned char** MemAlloc2D(int nHeight, int nWidth, unsigned char nInitVal);
void MemFree2D(unsigned char **Mem, int nHeight);

int main()
{
	FILE * input, *output;
	unsigned char** ori_img = MemAlloc2D(BASESIZE_HEIGHT, BASESIZE_WIDTH, 0);
	unsigned char** saltpepper_img = MemAlloc2D(BASESIZE_HEIGHT, BASESIZE_WIDTH, 0);
	unsigned char** filter_img = MemAlloc2D(BASESIZE_HEIGHT, BASESIZE_WIDTH, 0);

	// circuit512.raw 원본에 노이즈르 삽입하는 코드로 최초 실행 후에 주석 처리
	/*
	fopen_s(&input, "circuit512.raw", "rb");
	if (input == NULL) {
	cout << "Input File open failed";
	return 0;
	}
	for (int h = 0; h < BASESIZE_HEIGHT; h++)
	fread(ori_img[h], sizeof(unsigned char), BASESIZE_WIDTH, input);

	cout << "Salt and Pepper noise 적용중..." << endl;
	InputSaltPepperNoise(ori_img, saltpepper_img, BASESIZE_HEIGHT, BASESIZE_WIDTH, SALT, PEPPER);

	fopen_s(&output, "salt_pepper_0.175.raw", "wb");
	if (output == NULL) {
	cout << "Output File open failed";
	return 0;
	}
	for (int i = 0; i < BASESIZE_HEIGHT; i++)
	fwrite(saltpepper_img[i], sizeof(unsigned char), BASESIZE_WIDTH, output);
	*/

	const char* input_filename = "salt_pepper_0.25.raw";
	const char* output_filename = "AdaptiveMedian_0.25_11.raw";

	// 실행 시간 계산을 위한 코드 삽입
	chrono::system_clock::time_point start = chrono::system_clock::now();

	fopen_s(&input, input_filename, "rb");
	if (input == NULL) {
		cout << "Input File open failed";
		return 0;
	}
	for (int h = 0; h < BASESIZE_HEIGHT; h++)
		fread(saltpepper_img[h], sizeof(unsigned char), BASESIZE_WIDTH, input);

	// Median Filter or Adaptive Median Filter를 선택적으로 적용
	//MedianFilter(saltpepper_img, filter_img, BASESIZE_HEIGHT, BASESIZE_WIDTH, NFILTERSIZE);
	AdaptiveMedianFilter(saltpepper_img, filter_img, BASESIZE_HEIGHT, BASESIZE_WIDTH, NFILTERSIZE, NFILTERSIZE_MAX);

	fopen_s(&output, output_filename, "wb");
	if (output == NULL) {
		cout << "Output File open failed";
		return 0;
	}
	for (int i = 0; i < BASESIZE_HEIGHT; i++)
		fwrite(filter_img[i], sizeof(unsigned char), BASESIZE_WIDTH, output);

	chrono::duration<double> sec = chrono::system_clock::now() - start;
	cout << output_filename << "를 수행하는 걸린 시간(초) : " << sec.count() << " seconds" << endl;
	cout << endl;

	fclose(input);
	fclose(output);
	return 0;
}

void AdaptiveMedianFilter(unsigned char **Img_in, unsigned char **Out, int nHeight, int nWidth, int nFilterSize, int nFilterSize_Max)
{
	int nPadSize = (int)(nFilterSize / 2);
	unsigned char** In_Pad = Padding(Img_in, nHeight, nWidth, nFilterSize);
	unsigned char* sorting = new unsigned char[nFilterSize_Max*nFilterSize_Max];//sorting 범위 최대 지정

	int nFilter_Sizeup;
	int A1, A2, Zxy, Zmed, B1, B2, Result;

	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			A1 = 0, A2 = 0, Zmed = 0, B1 = 0, B2 = 0, Result = 0;
			nFilter_Sizeup = nFilterSize;

			Zxy = In_Pad[h + 1][w + 1];
			//Level A
			while (nFilter_Sizeup <= nFilterSize_Max)
			{
				//mask sorting
				int sort_num = 0;
				for (int n = 0; n < nFilter_Sizeup; n++)
					for (int m = 0; m < nFilter_Sizeup; m++)
					{
						//Out of Range
						if (h + n <= nHeight + 2 - 1 && h + m <= nWidth + 2 - 1)
						{
							sorting[sort_num] = In_Pad[h + n][w + m];
							sort_num++;
						}
					}
				sort(sorting, sorting + nFilter_Sizeup*nFilter_Sizeup);
				Zmed = sorting[(nFilter_Sizeup*nFilter_Sizeup) / 2];

				A1 = Zmed - sorting[0];
				A2 = Zmed - sorting[nFilter_Sizeup*nFilter_Sizeup - 1];

				//Level A → Level B
				if (A1 > 0 && A2 < 0)
					break;
				//Level A Repeat
				else
					nFilter_Sizeup += 2;
			}

			//Level B
			B1 = Zxy - sorting[0];
			B2 = Zxy - sorting[nFilter_Sizeup*nFilter_Sizeup - 1];
			if (B1 > 0 && B2 < 0)
			{
				Result = Zxy;
			}
			else
			{
				Result = Zmed;
			}

			Out[h][w] = (unsigned char)Result;
		}
	}
	delete[] sorting;
	MemFree2D(In_Pad, nHeight + 2 * nPadSize);
}

void MedianFilter(unsigned char **Img_in, unsigned char **Out, int nHeight, int nWidth, int nFilterSize)
{
	int nPadSize = (int)(nFilterSize / 2);
	int med = (int)(nFilterSize*nFilterSize / 2);
	unsigned char** In_Pad = Padding(Img_in, nHeight, nWidth, nFilterSize);
	unsigned char* sorting = new unsigned char[nFilterSize*nFilterSize];

	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			int sort_num = 0;
			for (int n = 0; n < nFilterSize; n++)
			{
				for (int m = 0; m< nFilterSize; m++)
				{
					sorting[sort_num] = In_Pad[h + n][w + m];
					sort_num++;
				}
			}

			sort(sorting, sorting + nFilterSize*nFilterSize);
			Out[h][w] = sorting[med];
		}
	}
	delete[] sorting;
	MemFree2D(In_Pad, nHeight + 2 * nPadSize);
}

unsigned char** Padding(unsigned char** In, int nHeight, int nWidth, int nFilterSize)
{
	int nPadSize = (int)(nFilterSize / 2);
	unsigned char** Pad = MemAlloc2D(nHeight + 2 * nPadSize, nWidth + 2 * nPadSize, 0);

	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			Pad[h + nPadSize][w + nPadSize] = In[h][w];
		}
	}

	for (int h = 0; h < nPadSize; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			Pad[h][w + nPadSize] = In[0][w];
			Pad[h + (nHeight - 1)][w + nPadSize] = In[nHeight - 1][w];
		}
	}

	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nPadSize; w++)
		{
			Pad[h + nPadSize][w] = In[h][0];
			Pad[h + nPadSize][w + (nWidth - 1)] = In[h][nWidth - 1];
		}
	}

	for (int h = 0; h < nPadSize; h++)
	{
		for (int w = 0; w < nPadSize; w++)
		{
			Pad[h][w] = In[0][0];
			Pad[h + (nHeight - 1)][w] = In[nHeight - 1][0];
			Pad[h][w + (nWidth - 1)] = In[0][nWidth - 1];
			Pad[h + (nHeight - 1)][w + (nWidth - 1)] = In[nHeight - 1][nWidth - 1];
		}
	}

	return Pad;
}

void InputSaltPepperNoise(unsigned char** In, unsigned char** Out, int nHeight, int nWidth, float fSProb, float fPProb)
{
	float Low = fSProb;
	float High = 1.0f - fPProb;
	float fRand;

	srand(GetTickCount());

	for (int h = 0; h < nHeight; h++)
	{
		for (int w = 0; w < nWidth; w++)
		{
			fRand = ((float)rand() / RAND_MAX);

			if (fRand < Low)
			{
				Out[h][w] = 255;
			}
			else if (fRand > High)
			{
				Out[h][w] = 0;
			}
			else Out[h][w] = In[h][w];
		}
	}
}

unsigned char** MemAlloc2D(int nHeight, int nWidth, unsigned char nInitVal)
{
	unsigned char** rtn = new unsigned char*[nHeight];
	for (int n = 0; n < nHeight; n++)
	{
		rtn[n] = new unsigned char[nWidth];
		memset(rtn[n], nInitVal, sizeof(unsigned char) * nWidth);
	}
	return rtn;
}

void MemFree2D(unsigned char **Mem, int nHeight)
{
	for (int n = 0; n < nHeight; n++)
	{
		delete[] Mem[n];
	}
	delete[] Mem;
}