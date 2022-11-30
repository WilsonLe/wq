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

void invoke(char *redId, int n, int ***in_topRightMatrix, int ***in_bottomLeftMatrix, int ***out_topRightMatrix, int ***out_bottomLeftMatrix)
{
	int link[2];
	int outputSize = sizeof(char) + sizeof('\n') + (sizeof(char) * 2 * n + sizeof('\n')) * n * 2;
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
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
				ss << std::to_string((*(in_topRightMatrix))[i][j]) << ",";
			ss << "$'\n'";
		}
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < n; j++)
				ss << std::to_string((*(in_bottomLeftMatrix))[i][j]) << ",";
			ss << "$'\n'";
		}
		// printf("s: %s\n", ss.str().c_str());
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
				std::stringstream ss(outputBytes);
				std::string substr;
				std::getline(ss, substr, '\n');
				for (int row = 0; row < n; row++)
				{
					for (int col = 0; col < n; col++)
					{
						std::getline(ss, substr, ',');
						(*(out_topRightMatrix))[row][col] = std::stoi(substr);
					}
				}
				for (int row = 0; row < n; row++)
				{
					for (int col = 0; col < n; col++)
					{
						std::getline(ss, substr, ',');
						(*(out_bottomLeftMatrix))[row][col] = std::stoi(substr);
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

void consume(queue_attr *queue, int threadId)
{
	while (true)
	{
		pthread_mutex_lock(queue->mutex);
		while (*(queue->count) == 0)
			pthread_cond_wait(queue->fill, queue->mutex);
		queue_d *data = get(queue->buffer, queue->use_ptr, queue->count);
		pthread_cond_signal(queue->empty);
		pthread_mutex_unlock(queue->mutex);

		// worker thread do stuff
		int **out_topRight = (int **)malloc(sizeof(int *) * data->n);
		for (int i = 0; i < data->n; i++)
			out_topRight[i] = (int *)malloc(sizeof(int) * data->n);
		int **out_bottomLeft = (int **)malloc(sizeof(int *) * data->n);
		for (int i = 0; i < data->n; i++)
			out_bottomLeft[i] = (int *)malloc(sizeof(int) * data->n);

		char _[4] = "lab";
		char *redId = strcat(_, std::to_string(threadId).c_str());
		invoke(redId, data->n, &(data->topRight), &(data->bottomLeft), &out_topRight, &out_bottomLeft);
		printMatrix(out_bottomLeft, data->n);
		printMatrix(out_topRight, data->n);

		// free output
		for (int i = 0; i < data->n; i++)
			free(out_bottomLeft[i]);
		free(out_bottomLeft);
		for (int i = 0; i < data->n; i++)
			free(out_topRight[i]);
		free(out_topRight);
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

		int row = 0;
		while (std::getline(file, line))
		{
			std::stringstream ss(line);
			int col = 0;
			while (ss.good())
			{
				std::string substr;
				std::getline(ss, substr, ',');
				in.data[row][col] = std::stoi(substr);
				col++;
			}
			row++;
		}
		file.close();
	}
	return in;
}
