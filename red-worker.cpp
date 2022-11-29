#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "lib.h"

void parseMatrixSize(int *n)
{
	char str[MAT_MAX];
	fgets(str, MAT_MAX, stdin);
	*n = std::stoi(str);
}

void parseMatrix(int n, int ***mat)
{

	for (int row = 0; row < n; row++)
	{
		for (int col = 0; col < n; col++)
		{
			std::string substr;
			char str[MAT_MAX];
			fgets(str, MAT_MAX, stdin);
			std::stringstream ss(str);
			while (ss.good())
			{
				std::getline(ss, substr, ',');
				(*(mat))[row][col] = std::stoi(substr);
			}
		}
	}
}

int main()
{
	int n;
	parseMatrixSize(&n);
	printf("n: %d\n", n);
	int **mat = (int **)malloc(sizeof(int *) * n);
	for (int i = 0; i < n; i++)
		mat[i] = (int *)malloc(sizeof(int) * n);
	parseMatrix(n, &mat);
	printMatrix(mat, n);
	return 0;
}