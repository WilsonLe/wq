#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "lib.h"

void parseMatrixSize(int *n)
{
	char str[MAX_MAT_DIM_BYTES];
	fgets(str, MAX_MAT_DIM_BYTES, stdin);
	std::string substr;
	std::stringstream ss(str);
	std::getline(ss, substr, '\n');
	*n = std::stoi(substr);
}

void parseNumPairs(int *n)
{
	char str[MAX_MAT_DIM_BYTES];
	fgets(str, MAX_MAT_DIM_BYTES, stdin);
	std::string substr;
	std::stringstream ss(str);
	std::getline(ss, substr, '\n');
	*n = std::stoi(substr);
}

void parseMatrix(int n, int ***mat)
{

	for (int row = 0; row < n; row++)
	{
		char str[MAX_MAT_DIM_BYTES];
		fgets(str, MAX_MAT_DIM_BYTES, stdin);
		std::stringstream ss(str);
		for (int col = 0; col < n; col++)
		{
			std::string substr;
			std::getline(ss, substr, ',');
			(*(mat))[row][col] = std::stoi(substr);
		}
	}
}

int main(int argc, char **argv)
{
	BLOCK_SIZE = 2;
	NUM_WORKER = 4;
	WQ_MAX = 256;
	MAX_CHAR_PER_ENTRY = 2;
	MAX_MAT_DIM = 128;
	MAX_MAT_DIM_BYTES = MAX_MAT_DIM * (MAX_CHAR_PER_ENTRY + sizeof(',')) + sizeof("\n");
	VERBOSE = 0;

	int rc = parseArgument(argc, argv);
	if (rc == 1 || rc == 0)
		return rc;

	if (VERBOSE)
	{
		printf("BLOCK_SIZE: %d\n", BLOCK_SIZE);
		printf("NUM_WORKER: %d\n", NUM_WORKER);
		printf("WQ_MAX: %d\n", WQ_MAX);
		printf("MAX_CHAR_PER_ENTRY: %d\n", MAX_CHAR_PER_ENTRY);
		printf("MAX_MAT_DIM: %d\n", MAX_MAT_DIM);
	}

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