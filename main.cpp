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

	queue queue;
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

	int blockSize = 1;
	for (int i = 0; i < in.n; i++)
	{
		int iBlock = i / blockSize;
		for (int j = i; j < in.n; j++)
		{
			int jBlock = j / blockSize;
			if (iBlock == jBlock)
			{
				// handle diagonal blocks
			}
			else
			{
				// handle pair blocks
				int **topRightBlock = (int **)malloc(sizeof(int *) * blockSize);
				for (int i = 0; i < blockSize; i++)
					topRightBlock[i] = (int *)malloc(sizeof(int) * blockSize);
				topRightBlock[i % blockSize][j % blockSize] = in.data[i][j];
				int **bottomLeftBlock = (int **)malloc(sizeof(int *) * blockSize);
				for (int i = 0; i < blockSize; i++)
					bottomLeftBlock[i] = (int *)malloc(sizeof(int) * blockSize);
				bottomLeftBlock[j % blockSize][i % blockSize] = in.data[j][i];
				queue_d *data = (queue_d *)malloc(sizeof(queue_d));
				data->n = blockSize;
				data->topRight = topRightBlock;
				data->bottomLeft = bottomLeftBlock;
				produce(&queue, data);
			}
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