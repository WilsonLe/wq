#include <stdio.h>
#include <iostream>
#include "lib.h"

// setup time code
struct timespec begin, end;

/**
 * @brief get elapsed time since start of program
 *
 * @return double in seconds.milliseconds
 */
double time()
{
	clock_gettime(CLOCK_REALTIME, &end);
	double elapsed = (end.tv_sec - begin.tv_sec) * 1000 + (end.tv_nsec - begin.tv_nsec) * 1e-6;
	return elapsed;
}

int main(int argc, char **argv)
{
	// handling argument
	initArgument();

	int rc = parseArgument(argc, argv);
	if (rc == 1 || rc == 0)
		return rc;

	if (VERBOSE)
	{
		printf("BLOCK_SIZE: %d\n", BLOCK_SIZE);
		printf("NUM_WORKER: %d\n", NUM_WORKER);
		printf("WORKER NAMES: ");
		for (int i = 0; i < NUM_WORKER - 1; i++)
			printf("%s, ", _WORKER_NAMES[i]);
		printf("%s\n", _WORKER_NAMES[NUM_WORKER - 1]);
		printf("WQ_MAX: %d\n", WQ_MAX);
		printf("MAX_CHAR_PER_ENTRY: %d\n", MAX_CHAR_PER_ENTRY);
		printf("MAX_MAT_DIM: %d\n", MAX_MAT_DIM);
		printf("VERBOSE: %d\n", VERBOSE);
		printf("TIME: %d\n", TIME);
	}

	// start timer
	clock_gettime(CLOCK_REALTIME, &begin);

	// read input
	input in = readInput();

	if (VERBOSE && TIME)
	{
		printf("read input: %f ms\n", time());
	}

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

	if (VERBOSE && TIME)
	{
		printf("init queue: %f ms\n", time());
	}

	// spawn worker thread pool
	int numThreads = NUM_WORKER;
	pthread_t tid[numThreads];
	for (int i = 0; i < numThreads; i++)
	{
		worker_t_input input;
		input.queue = &queue;
		input.data_ptr = &(in.data);
		input.workerName = _WORKER_NAMES[i];
		input.data_len = in.n;
		pthread_create(&tid[i], NULL, createWorkerThread, &input);
	}

	if (VERBOSE && TIME)
	{
		printf("spawn worker thread pool: %f ms\n", time());
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

	if (VERBOSE && TIME)
	{
		printf("divide up the work: %f ms\n", time());
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

	if (VERBOSE && TIME)
	{
		printf("produced all work to work queue: %f ms\n", time());
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

	if (VERBOSE && TIME)
	{
		printf("sequentially transpose diagonal blocks in-place: %f ms\n", time());
	}

	// join threads
	for (int i = 0; i < numThreads; i++)
	{
		pthread_cond_signal(queue.fill);
		pthread_join(tid[i], NULL);
	}

	printMatrix(in.data, in.n);
	if (TIME)
		printf("%f\n", time());

	// free memory
	for (int i = 0; i < in.n; i++)
		free(in.data[i]);
	free(in.data);
	free(buffer);

	pthread_mutex_destroy(&mutex);
	return 0;
}