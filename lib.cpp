#include <stdio.h>
#include <pthread.h>
#include <sstream>
#include <fstream>
#include <string>
#include "lib.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

void put(queue_d ***buffer, int *fill_ptr, int *count, queue_d *data)
{
	(*buffer)[*fill_ptr] = data;
	*fill_ptr = (*fill_ptr + 1) % MAX;
	(*count)++;
}

queue_d *get(queue_d ***buffer, int *use_ptr, int *count)
{
	queue_d *tmp = (*buffer)[*use_ptr];
	*use_ptr = (*use_ptr + 1) % MAX;
	(*count)--;
	return tmp;
}

void invoke()
{
	int link[2];
	char foo[4096];
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
		execl("/usr/bin/ssh", "ssh", "lab", "ls", (char *)0);
		exit(0);
	}
	else
	{
		printf("parent process, pid = %u\n", getpid());
		close(link[1]);
		int status;
		if (waitpid(pid, &status, 0) > 0)
		{
			if (WIFEXITED(status) && !WEXITSTATUS(status))
			{
				printf("program execution successful\n");
				int nbytes = read(link[0], foo, sizeof(foo));
				printf("Output: (%.*s)\n", nbytes, foo);
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

void consume(queue *queue)
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
		invoke();
	}
}

void produce(queue *queue, queue_d *data)
{
	pthread_mutex_lock(queue->mutex);
	while (*(queue->count) == MAX)
		pthread_cond_wait(queue->empty, queue->mutex);
	put(queue->buffer, queue->fill_ptr, queue->count, data);
	pthread_cond_signal(queue->fill);
	pthread_mutex_unlock(queue->mutex);
}

void printMatrix(int **mat, int n)
{
	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < n; j++)
			printf("%d ", mat[i][j]);
		printf("\n");
	}
}

input readInput()
{
	input in;
	std::ifstream file("input.txt");
	if (file.is_open())
	{
		std::string line;
		std::getline(file, line);
		in.n = stoi(line);
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
				in.data[row][col] = stoi(substr);
				col++;
			}
			row++;
		}
		file.close();
	}
	return in;
}
