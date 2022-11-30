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
	parseMatrixSize(&n);
	int **topRightMatrix = (int **)malloc(sizeof(int *) * n);
	for (int i = 0; i < n; i++)
		topRightMatrix[i] = (int *)malloc(sizeof(int) * n);
	parseMatrix(n, &topRightMatrix);
	int **bottomLeftMatrix = (int **)malloc(sizeof(int *) * n);
	for (int i = 0; i < n; i++)
		bottomLeftMatrix[i] = (int *)malloc(sizeof(int) * n);
	parseMatrix(n, &bottomLeftMatrix);

	// transpose
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
		{
			int temp = topRightMatrix[i][j];
			topRightMatrix[i][j] = bottomLeftMatrix[j][i];
			bottomLeftMatrix[j][i] = temp;
		}
	printMatrix(topRightMatrix, n);
	printMatrix(bottomLeftMatrix, n);
	return 0;
}