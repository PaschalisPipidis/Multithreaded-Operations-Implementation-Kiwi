#include "../engine/db.h"
#include <pthread.h>

typedef struct
{
	DB* db;
	long int count;
	int r;
} pthread_args;

pthread_mutex_t lock_w;
pthread_mutex_t lock_r;