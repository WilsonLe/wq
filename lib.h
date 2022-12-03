#ifndef LIB
#define LIB

extern int WQ_MAX;			   // max slot in work queue buffer
extern int BLOCK_SIZE;		   // number of entry per row of block, also number of rows in matrix
extern int NUM_WORKER;		   // number of worker
extern int MAX_CHAR_PER_ENTRY; // number of character per entry (i.e 10 takes 2 chars, 100 takes 3 chars)
extern int MAX_MAT_DIM;		   // maximum matrix dimension (number of entries)
extern int MAX_MAT_DIM_BYTES;  // maximum maxtrix dimension in bytes
extern char *WORKER_NAMES;	   // string of worker ids, comma separated, no spaces
extern char **_WORKER_NAMES;   // array of string of worker ids, parsed from WORKER_NAMES
extern int VERBOSE;			   // whether to run the program in verbose mode

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
	char *workerName;
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