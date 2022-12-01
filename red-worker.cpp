#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "lib.h"

void parseMatrixSize(int *n)
{
	char str[MAT_MAX];
	fgets(str, MAT_MAX, stdin);
	std::string substr;
	std::stringstream ss(str);
	std::getline(ss, substr, '\n');
	*n = std::stoi(substr);
}

void parseNumPairs(int *n)
{
	char str[MAT_MAX];
	fgets(str, MAT_MAX, stdin);
	std::string substr;
	std::stringstream ss(str);
	std::getline(ss, substr, '\n');
	*n = std::stoi(substr);
}

void parseMatrix(int n, int ***mat)
{

	for (int row = 0; row < n; row++)
	{
		char str[MAT_MAX];
		fgets(str, MAT_MAX, stdin);
		std::stringstream ss(str);
		for (int col = 0; col < n; col++)
		{
			std::string substr;
			std::getline(ss, substr, ',');
			(*(mat))[row][col] = std::stoi(substr);
		}
	}
}

int main()
{
	int n;
	int numPairs;
	parseMatrixSize(&n);
	parseNumPairs(&numPairs);

	int ***out_topRights = (int ***)malloc(sizeof(int **) * numPairs);
	for (int i = 0; i < numPairs; i++)
	{
		out_topRights[i] = (int **)malloc(sizeof(int **) * n);
		for (int j = 0; j < n; j++)
			out_topRights[i][j] = (int *)malloc(sizeof(int) * n);
	}
	int ***out_bottomLefts = (int ***)malloc(sizeof(int **) * numPairs);
	for (int i = 0; i < numPairs; i++)
	{
		out_bottomLefts[i] = (int **)malloc(sizeof(int **) * n);
		for (int j = 0; j < n; j++)
			out_bottomLefts[i][j] = (int *)malloc(sizeof(int) * n);
	}

	for (int k = 0; k < numPairs; k++)
	{
		parseMatrix(n, &(out_topRights[k]));
		parseMatrix(n, &(out_bottomLefts[k]));

		// transpose
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
			{
				int temp = out_topRights[k][i][j];
				out_topRights[k][i][j] = out_bottomLefts[k][j][i];
				out_bottomLefts[k][j][i] = temp;
			}
	}
	for (int i = 0; i < numPairs; i++)
	{
		printMatrix(out_topRights[i], n);
		printMatrix(out_bottomLefts[i], n);
	}

	return 0;
}