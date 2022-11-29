#ifndef LIB
#define LIB
#define WQ_MAX 256
#define MAT_MAX 65536

typedef struct input
{
	int n;
	int **data;
} input;

input readInput();

typedef struct queue_d
{
	int n;
	int **topRight;
	int **bottomLeft;
} queue_d;

typedef struct queue
{
	queue_d ***buffer;
	int *use_ptr;
	int *fill_ptr;
	pthread_mutex_t *mutex;
	int *count;
	pthread_cond_t *fill;
	pthread_cond_t *empty;
} queue;

typedef struct worker_t_input
{
	queue *queue;
	int threadId;
} worker_t_input;

void put(queue_d ***buffer, int *fill_ptr, int *count, queue_d *data);

queue_d *get(queue_d ***buffer, int *use_ptr, int *count);

void printMatrix(int **mat, int n);

void consume(queue *queue, int threadId);

void produce(queue *queue, queue_d *work);

#endif