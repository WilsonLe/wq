#ifndef LIB
#define LIB

extern int WQ_MAX;
extern int BLOCK_SIZE;
extern int NUM_WORKER;
extern int MAX_CHAR_PER_ENTRY;
extern int MAX_MAT_DIM;
extern int MAX_MAT_DIM_BYTES;
extern int VERBOSE;

typedef struct input
{
	int n;
	int **data;
} input;

input readInput();

typedef struct queue_d
{
	int n;
	int numPairs;
	int ***topRights;
	int ***bottomLefts;
} queue_d;

typedef struct queue_attr
{
	queue_d ***buffer;
	int *use_ptr;
	int *fill_ptr;
	pthread_mutex_t *mutex;
	int *count;
	pthread_cond_t *fill;
	pthread_cond_t *empty;
	char *stopOnEmpty;
} queue_attr;

typedef struct worker_t_input
{
	queue_attr *queue;
	int threadId;
	int ***data_ptr;
	int data_len;
} worker_t_input;

void put(queue_d ***buffer, int *fill_ptr, int *count, queue_d *data);

queue_d *get(queue_d ***buffer, int *use_ptr, int *count);

void printMatrix(int **mat, int n);

void consume(queue_attr *queue, int threadId, int ***data_ptr, int data_len);

void produce(queue_attr *queue, queue_d *work);

int parseArgument(int argc, char **argv);

void *createWorkerThread(void *input);
#endif