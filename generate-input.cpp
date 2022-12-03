#include <fstream>
#include <iostream>

/**
 * @brief generate input matrix
 * -n [number] to specify input size
 * @return int, 0 on success, -1 on error
 */
int main(int argc, char **argv)
{
	int n;
	if (argc == 0)
	{
		printf("Invalid usage, use -h for details\n");
		return -1;
	}

	if (argc == 1)
	{
		n = 64;
	}
	else if (strcmp(argv[1], "-h") == 0)
	{
		printf("Use -n <number> to specify input size. Defaults to 128.\n");
	}
	else if (strcmp(argv[1], "-n") == 0)
	{
		n = std::stoi(argv[2]);
	}
	else
	{
		printf("Invalid usage, use -h for details");
		return -1;
	}
	std::ofstream outfile;
	outfile.open("input.txt");
	outfile << n << std::endl;
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
		{
			outfile << j << ',';
		}
		outfile << std::endl;
	}
	return 0;
}