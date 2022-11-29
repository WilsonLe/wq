#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "lib.h"

void parseMatrixSize(int *n)
{
	char str[256];
	fgets(str, 256, stdin);
	*n = std::stoi(str);
}

void parseMatrix(int n, int ***mat) {

}

int main()
{
	int n;
	parseMatrixSize(&n);
	printf("%d\n", n);
	int **mat = (int **)malloc(sizeof(int *) * n);
	for (int i = 0; i < n; i++)
		mat[i] = (int *)malloc(sizeof(int) * n);
	parseMatrix(n, &mat);
	printMatrix(mat, n);
	return 0;
}