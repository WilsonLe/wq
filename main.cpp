#include <stdio.h>
#include <iostream>
#include "lib.h"

void *createWorkerThread(void *input)
{
	worker_t_input *_input = (worker_t_input *)input;
	consume(_input->queue, _input->threadId);
	free(input);
	return NULL;
}

int main(int argc, char *argv[])
{
	// setup work queue data
	int fill_ptr = 0;
	int use_ptr = 0;
	int count = 0;
	void **buffer = (void **)malloc(sizeof(void *) * WQ_MAX);
	pthread_cond_t empty, fill;
	pthread_cond_init(&empty, NULL);
	pthread_cond_init(&fill, NULL);
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, NULL);

	queue_attr queue;
	queue.buffer = (queue_d ***)&buffer;
	queue.use_ptr = &use_ptr;
	queue.fill_ptr = &fill_ptr;
	queue.mutex = &mutex;
	queue.count = &count;
	queue.fill = &fill;
	queue.empty = &empty;

	// spawn worker thread pool
	int numThreads = 4;
	pthread_t tid[numThreads];
	for (int i = 0; i < numThreads; i++)
	{
		worker_t_input *input = (worker_t_input *)malloc(sizeof(worker_t_input *));
		input->queue = &queue;
		input->threadId = i + 2; // start from red2 -> red5
		pthread_create(&tid[i], NULL, createWorkerThread, input);
	}

	// read input
	input in = readInput();

	// divide up the work
	int blockSize = 4;
	int ****blocks = (int ****)malloc(sizeof(int ***) * in.n / blockSize);
	for (int i = 0; i < in.n / blockSize; i++)
	{
		blocks[i] = (int ***)malloc(sizeof(int **) * in.n / blockSize);
		for (int j = 0; j < in.n / blockSize; j++)
		{
			blocks[i][j] = (int **)malloc(sizeof(int *) * blockSize);
			for (int k = 0; k < blockSize; k++)
			{
				blocks[i][j][k] = (int *)malloc(sizeof(int) * blockSize);
			}
		}
	}

	for (int i = 0; i < in.n; i++)
		for (int j = i; j < in.n; j++)
		{
			int **temp = blocks[i / blockSize][j / blockSize];
			blocks[i / blockSize][j / blockSize][i % blockSize][j % blockSize] = in.data[i][j];
			blocks[j / blockSize][i / blockSize][j % blockSize][i % blockSize] = in.data[j][i];
		}

	for (int i = 0; i < in.n / blockSize; i++)
		for (int j = i; j < in.n / blockSize; j++)
		{
			if (i != j)
			{
				int **topRightBlock = blocks[i][j];
				int **bottomLeftBlock = blocks[j][i];
				queue_d *data = (queue_d *)malloc(sizeof(queue_d));
				data->n = blockSize;
				data->topRight = topRightBlock;
				data->bottomLeft = bottomLeftBlock;
				// printf("producing:\n");
				// printMatrix(data->topRight, data->n);
				// printMatrix(data->bottomLeft, data->n);
				produce(&queue, data);
			}
		}

	// join threads
	for (int i = 0; i < numThreads; i++)
		pthread_join(tid[i], NULL);

	// free memory
	for (int i = 0; i < in.n; i++)
		free(in.data[i]);
	free(in.data);

	for (int i = 0; i < WQ_MAX; i++)
		free(buffer[i]);
	free(buffer);

	pthread_mutex_destroy(&mutex);
	return 0;
}