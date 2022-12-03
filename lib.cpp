#include <stdio.h>
#include <pthread.h>
#include <sstream>
#include <fstream>
#include <string.h>
#include "lib.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

int WQ_MAX;
int MAT_MAX_DIM_BYTES;
int BLOCK_SIZE;
int NUM_WORKER;
int MAX_MAT_DIM;
int MAX_CHAR_PER_ENTRY;
int MAX_MAT_DIM_BYTES;
int VERBOSE;

void printMatrix(int **mat, int n)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
			printf("%d,", mat[i][j]);
		printf("\n");
	}
}

void put(queue_d ***buffer, int *fill_ptr, int *count, queue_d *data)
{
	(*buffer)[*fill_ptr] = data;
	*fill_ptr = (*fill_ptr + 1) % WQ_MAX;
	(*count)++;
}

queue_d *get(queue_d ***buffer, int *use_ptr, int *count)
{
	queue_d *tmp = (*buffer)[*use_ptr];
	*use_ptr = (*use_ptr + 1) % WQ_MAX;
	(*count)--;
	return tmp;
}

void invoke(char *redId, int n, int numPairs, int ****in_topRightMatrices, int ****in_bottomLeftMatrices, int ****out_topRightMatrices, int ****out_bottomLeftMatrices)
{
	int link[2];
	int lineSize = (sizeof(char) * MAX_CHAR_PER_ENTRY + sizeof(',')) * n + sizeof('\n');
	int matSize = lineSize * n;
	int outputSize = matSize * numPairs * 2;
	char outputBytes[outputSize];
	memset(outputBytes, 0, outputSize);
	pipe(link);
	int pid = fork();
	if (pid == -1)
	{
		printf("can't fork, error occurred\n");
		exit(-1);
	}
	else if (pid == 0)
	{
		dup2(link[1], STDOUT_FILENO);
		close(link[0]);
		close(link[1]);
		// setup data to invoke red_worker
		std::stringstream ss;
		ss << n << "$'\n'";
		ss << numPairs << "$'\n'";
		for (int k = 0; k < numPairs; k++)
		{
			for (int i = 0; i < n; i++)
			{
				for (int j = 0; j < n; j++)
					ss << std::to_string((*in_topRightMatrices)[k][i][j]) << ",";
				ss << "$'\n'";
			}
			for (int i = 0; i < n; i++)
			{
				for (int j = 0; j < n; j++)
					ss << std::to_string((*in_bottomLeftMatrices)[k][i][j]) << ",";
				ss << "$'\n'";
			}
		}
		// invoke
		execl("/usr/bin/ssh", "ssh", redId, "~/CS401/wq/red_worker", "<<<", ss.str().c_str(), (char *)0);
		exit(0);
	}
	else
	{
		close(link[1]);
		int status;
		if (waitpid(pid, &status, 0) > 0)
		{
			if (WIFEXITED(status) && !WEXITSTATUS(status))
			{
				read(link[0], outputBytes, sizeof(outputBytes));

				// parse output of worker thread
				std::stringstream ss(outputBytes);
				// printf("ss: \n%s", ss.str().c_str());
				std::string substr;
				for (int pair = 0; pair < numPairs; pair++)
				{
					for (int row = 0; row < n; row++)
					{
						for (int col = 0; col < n; col++)
						{
							std::getline(ss, substr, ',');
							(*out_topRightMatrices)[pair][row][col] = std::stoi(substr);
						}
						std::getline(ss, substr, '\n');
					}
					for (int row = 0; row < n; row++)
					{
						for (int col = 0; col < n; col++)
						{
							std::getline(ss, substr, ',');
							(*out_bottomLeftMatrices)[pair][row][col] = std::stoi(substr);
						}
						std::getline(ss, substr, '\n');
					}
				}
			}
			else if (WIFEXITED(status) && WEXITSTATUS(status))
			{
				if (WEXITSTATUS(status) == 127)
				{
					// execv failed
					printf("execv failed\n");
				}
				else
				{
					printf("program terminated normally, but returned a non-zero status\n");
				}
			}
			else
			{
				printf("program didn't terminate normally\n");
			}
		}
		else
		{
			// waitpid() failed
			printf("waitpid() failed\n");
		}
	}
}

void consume(queue_attr *queue, int threadId, int ***data_ptr, int data_len)
{
	while (true)
	{
		pthread_mutex_lock(queue->mutex);
		while (*(queue->count) == 0)
		{
			if (*(queue->stopOnEmpty) == 0)
				pthread_cond_wait(queue->fill, queue->mutex);
			if (*(queue->stopOnEmpty) == 1)
			{
				pthread_mutex_unlock(queue->mutex);
				return;
			}
		}

		queue_d *data = get(queue->buffer, queue->use_ptr, queue->count);
		pthread_cond_signal(queue->empty);
		pthread_mutex_unlock(queue->mutex);

		// worker thread do stuff
		// parse data from work queue
		int ***out_topRights = (int ***)malloc(sizeof(int **) * data->numPairs);
		for (int i = 0; i < data->numPairs; i++)
		{
			out_topRights[i] = (int **)malloc(sizeof(int **) * data->n);
			for (int j = 0; j < data->n; j++)
				out_topRights[i][j] = (int *)malloc(sizeof(int) * data->n);
		}
		int ***out_bottomLefts = (int ***)malloc(sizeof(int **) * data->numPairs);
		for (int i = 0; i < data->numPairs; i++)
		{
			out_bottomLefts[i] = (int **)malloc(sizeof(int **) * data->n);
			for (int j = 0; j < data->n; j++)
				out_bottomLefts[i][j] = (int *)malloc(sizeof(int) * data->n);
		}

		char _[4] = "lab";
		char *redId = strcat(_, std::to_string(threadId).c_str());
		invoke(redId, data->n, data->numPairs, &(data->topRights), &(data->bottomLefts), &out_topRights, &out_bottomLefts);

		// assign output value
		int numBlocks = data_len / BLOCK_SIZE;

		// offset number of blocks from top and left
		// I.e blockNum = 0 refers to the top row and left column blocks
		int blockNum = numBlocks - data->numPairs - 1;

		for (int k = 0; k < data->numPairs; k++)
		{
			int blockIndex = blockNum + k;
			int blockOffset = blockIndex * BLOCK_SIZE + BLOCK_SIZE;
			for (int i = 0; i < BLOCK_SIZE; i++)
			{
				for (int j = 0; j < BLOCK_SIZE; j++)
				{
					int x = blockOffset + j;
					int y = blockNum * BLOCK_SIZE + i;
					(*data_ptr)[y][x] = out_topRights[k][i][j];
					(*data_ptr)[x][y] = out_bottomLefts[k][j][i];
				}
			}
		}
	}
}

void produce(queue_attr *queue, queue_d *data)
{
	pthread_mutex_lock(queue->mutex);
	while (*(queue->count) == WQ_MAX)
		pthread_cond_wait(queue->empty, queue->mutex);
	put(queue->buffer, queue->fill_ptr, queue->count, data);
	pthread_cond_signal(queue->fill);
	pthread_mutex_unlock(queue->mutex);
}

input readInput()
{
	input in;
	std::ifstream file("input.txt");
	if (file.is_open())
	{
		std::string line;
		std::getline(file, line);
		in.n = std::stoi(line);
		in.data = (int **)malloc(sizeof(int *) * in.n);
		for (int i = 0; i < in.n; i++)
			in.data[i] = (int *)malloc(sizeof(int) * in.n);

		for (int row = 0; row < in.n; row++)
		{
			std::getline(file, line);
			std::stringstream ss(line);
			for (int col = 0; col < in.n; col++)
			{
				std::string substr;
				std::getline(ss, substr, ',');
				in.data[row][col] = std::stoi(substr);
			}
		}
		file.close();
	}
	return in;
}

/**
 * @brief check if input string is a natural number N = {0, 1, 2, 3, ...}
 *
 * @param s input string
 * @return int
 * 0 if it is not a natural number
 * 1 if it is a natural number
 */
int isNaturalNumber(char *s)
{
	for (int i = 0; i < strlen(s); i++)
	{
		if (isdigit(s[i]) == 0)
			return 0;
	}
	return 1;
}

/**
 * @brief parses input command line argument
 *
 * @return int
 * -1: continue main normally
 * 1: return 1 in main
 * 0: return 0 in main
 */
int parseArgument(int argc, char **argv)
{
	if (argc == 0)
	{
		printf("Invalid usage, use -h for details\n");
		return 1;
	}
	else if (argc == 2)
	{
		if (strcmp(argv[1], "-h") == 0)
		{
			printf("Use --block-size <number> to specify block size to break down the matrix\n");
			printf("Use --num-worker <number> to specify number of worker consuming from work queue\n");
			printf("Use --wq-max <number> to specify number of slots in work queue\n");
			printf("Use --max-char-per-entry <number> to specify number of maximum character per entry (i.e 24 takes 2 characters, 100 takes 3 characters)\n");
			printf("Use --max-mat-dim <number> to specify number of maximum matrix size\n");
			return 0;
		}
		else if (strcmp(argv[1], "-v") == 0)
		{
			VERBOSE = 1;
			return -1;
		}
		else
		{
			printf("Invalid usage, use -h for details");
			return 1;
		}
	}
	else
	{
		char skipNextIter = 0;
		for (int i = 1; i < argc; i++)
		{
			if (skipNextIter == 1)
			{
				skipNextIter = 0;
				continue;
			}

			char *arg = argv[i];
			if (strcmp(arg, "--block-size") == 0)
			{
				if (isNaturalNumber(argv[i + 1]))
				{
					BLOCK_SIZE = atoi(argv[i + 1]);
					skipNextIter = 1;
				}
				else
				{
					printf("Invalid argument type, expect <number>, got '%s'\n", argv[i + 1]);
					return 1;
				}
			}
			else if (strcmp(arg, "--num-worker") == 0)
			{
				if (isNaturalNumber(argv[i + 1]))
				{
					NUM_WORKER = atoi(argv[i + 1]);
					skipNextIter = 1;
				}
				else
				{
					printf("Invalid argument type, expect <number>, got '%s'\n", argv[i + 1]);
					return 1;
				}
			}
			else if (strcmp(arg, "--wq-max") == 0)
			{
				if (isNaturalNumber(argv[i + 1]))
				{
					WQ_MAX = atoi(argv[i + 1]);
					skipNextIter = 1;
				}
				else
				{
					printf("Invalid argument type, expect <number>, got '%s'\n", argv[i + 1]);
					return 1;
				}
			}
			else if (strcmp(arg, "--max-char-per-entry") == 0)
			{
				if (isNaturalNumber(argv[i + 1]))
				{
					MAX_CHAR_PER_ENTRY = atoi(argv[i + 1]);
					skipNextIter = 1;
				}
				else
				{
					printf("Invalid argument type, expect <number>, got '%s'\n", argv[i + 1]);
					return 1;
				}
			}
			else if (strcmp(arg, "--max-mat-dim") == 0)
			{
				if (isNaturalNumber(argv[i + 1]))
				{
					MAX_MAT_DIM = atoi(argv[i + 1]);
					skipNextIter = 1;
				}
				else
				{
					printf("Invalid argument type, expect <number>, got '%s'\n", argv[i + 1]);
					return 1;
				}
			}
			else if (strcmp(arg, "-v") == 0)
			{
				VERBOSE = 1;
			}
		}
		return -1;
	}
}

void *createWorkerThread(void *input)
{
	worker_t_input *_input = (worker_t_input *)input;
	consume(_input->queue, _input->threadId, _input->data_ptr, _input->data_len);
	return NULL;
}
