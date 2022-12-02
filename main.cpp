#include <stdio.h>
#include <iostream>
#include "lib.h"

void *createWorkerThread(void *input)
{
	worker_t_input *_input = (worker_t_input *)input;
	consume(_input->queue, _input->threadId, _input->data_ptr, _input->data_len);
	return NULL;
}

int main(int argc, char *argv[])
{
	// read input
	input in = readInput();

	// setup work queue data
	int fill_ptr = 0;
	int use_ptr = 0;
	int count = 0;
	int numConsumed = 0;
	char stopOnEmpty = 0;
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
	queue.stopOnEmpty = &stopOnEmpty;

	// spawn worker thread pool
	int numThreads = NUM_WORKER;
	pthread_t tid[numThreads];
	for (int i = 0; i < numThreads; i++)
	{
		worker_t_input input;
		input.queue = &queue;
		input.data_ptr = &(in.data);
		input.threadId = i + 2; // start from red2 -> red5
		input.data_len = in.n;
		pthread_create(&tid[i], NULL, createWorkerThread, &input);
	}

	// divide up the work
	int blockSize = BLOCK_SIZE;

	// allocate blocks
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

	// assign data values to blocks
	for (int i = 0; i < in.n; i++)
		for (int j = i; j < in.n; j++)
		{
			int **temp = blocks[i / blockSize][j / blockSize];
			blocks[i / blockSize][j / blockSize][i % blockSize][j % blockSize] = in.data[i][j];
			blocks[j / blockSize][i / blockSize][j % blockSize][i % blockSize] = in.data[j][i];
		}

	// iterate through blocks to produce to work queue
	for (int i = 0; i < in.n / blockSize; i++)
	{
		int numBlocks = (in.n / blockSize) - i - 1; // ignore the i==j blocks
		if (numBlocks == 0)
			continue;
		// printf("numBlocks: %d\n", numBlocks);
		int ***topRightBlocks = (int ***)malloc(sizeof(int **) * numBlocks);
		for (int i = 0; i < numBlocks; i++)
		{
			topRightBlocks[i] = (int **)malloc(sizeof(int *) * blockSize);
			for (int j = 0; j < numBlocks; j++)
				topRightBlocks[i][j] = (int *)malloc(sizeof(int) * blockSize);
		}
		int ***bottomLeftBlocks = (int ***)malloc(sizeof(int **) * numBlocks);
		for (int i = 0; i < numBlocks; i++)
		{
			bottomLeftBlocks[i] = (int **)malloc(sizeof(int *) * blockSize);
			for (int j = 0; j < numBlocks; j++)
				bottomLeftBlocks[i][j] = (int *)malloc(sizeof(int) * blockSize);
		}

		for (int j = i + 1; j < in.n / blockSize; j++)
		{
			topRightBlocks[j - i - 1] = blocks[i][j];
			bottomLeftBlocks[j - i - 1] = blocks[j][i];
		}
		queue_d *data = (queue_d *)malloc(sizeof(queue_d));
		data->n = blockSize;
		data->numPairs = numBlocks;
		data->topRights = topRightBlocks;
		data->bottomLefts = bottomLeftBlocks;
		// printf("producing numBlocks: %d\n", data->numPairs);
		// for (int i = 0; i < numBlocks; i++)
		// {
		// 	printMatrix(data->topRights[i], data->n);
		// 	printMatrix(data->bottomLefts[i], data->n);
		// }
		produce(&queue, data);
	}

	// toggle stopOnEmpty, then signal all worker thread to wake up and return
	stopOnEmpty = 1;

	// sequentially transpose diagonal blocks in-place
	for (int k = 0; k < in.n / blockSize; k++)
	{
		for (int i = 0; i < BLOCK_SIZE; i++)
		{
			for (int j = i + 1; j < BLOCK_SIZE; j++)
			{
				int x = k * BLOCK_SIZE + j;
				int y = k * BLOCK_SIZE + i;
				int tmp = in.data[y][x];
				in.data[y][x] = in.data[x][y];
				in.data[x][y] = tmp;
			}
		}
	}

	// join threads
	for (int i = 0; i < numThreads; i++)
	{
		pthread_cond_signal(queue.fill);
		pthread_join(tid[i], NULL);
	}

	printMatrix(in.data, in.n);

	// free memory
	for (int i = 0; i < in.n; i++)
		free(in.data[i]);
	free(in.data);
	free(buffer);

	pthread_mutex_destroy(&mutex);
	return 0;
}