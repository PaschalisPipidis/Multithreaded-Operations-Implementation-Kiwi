#include "bench.h"
#define DATAS ("testdb")

extern double readers_total_cost, readers_total_cost_per_op, readers_total_throughput;
extern double writers_total_cost, writers_total_cost_per_op, writers_total_throughput;

void _random_key(char *key,int length) {
	int i;
	char salt[36]= "abcdefghijklmnopqrstuvwxyz0123456789";

	for (i = 0; i < length; i++)
		key[i] = salt[rand() % 36];
}

void _print_header(int count)
{
	double index_size = (double)((double)(KSIZE + 8 + 1) * count) / 1048576.0;
	double data_size = (double)((double)(VSIZE + 4) * count) / 1048576.0;

	printf("Keys:\t\t%d bytes each\n", 
			KSIZE);
	printf("Values: \t%d bytes each\n", 
			VSIZE);
	printf("Entries:\t%d\n", 
			count);
	printf("IndexSize:\t%.1f MB (estimated)\n",
			index_size);
	printf("DataSize:\t%.1f MB (estimated)\n",
			data_size);

	printf(LINE1);
}

void _print_environment()
{
	time_t now = time(NULL);

	printf("Date:\t\t%s", 
			(char*)ctime(&now));

	int num_cpus = 0;
	char cpu_type[256] = {0};
	char cache_size[256] = {0};

	FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
	if (cpuinfo) {
		char line[1024] = {0};
		while (fgets(line, sizeof(line), cpuinfo) != NULL) {
			const char* sep = strchr(line, ':');
			if (sep == NULL || strlen(sep) < 10)
				continue;

			char key[1024] = {0};
			char val[1024] = {0};
			strncpy(key, line, sep-1-line);
			strncpy(val, sep+1, strlen(sep)-1);
			if (strcmp("model name", key) == 0) {
				num_cpus++;
				strcpy(cpu_type, val);
			}
			else if (strcmp("cache size", key) == 0)
				strncpy(cache_size, val + 1, strlen(val) - 1);	
		}

		fclose(cpuinfo);
		printf("CPU:\t\t%d * %s", 
				num_cpus, 
				cpu_type);

		printf("CPUCache:\t%s\n", 
				cache_size);
	}
}

// NEW CODE
void* parallel_read(void* args)
{
	pthread_args* pargs = (pthread_args*)args;
	_read_test(pargs->db, pargs->count, pargs->r);
	return NULL;
}

// NEW CODE
void* parallel_write(void* args)
{
	pthread_args* pargs = (pthread_args*)args;
	_write_test(pargs->db, pargs->count, pargs->r);
	return NULL;
}

void usage()
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "./kiwi-bench <read|write|> <op_num> [random]\n");
	fprintf(stderr, "./kiwi-bench <p-read> <op_num> <thread_num> [random]\n");
	fprintf(stderr, "./kiwi-bench <p-rw> <op_num> <thread_num> <writer_perc> [random]\n\n");

	fprintf(stderr, "read:           read from database\n");
	fprintf(stderr, "write:          write to database\n");
	fprintf(stderr, "p-read:         parallel read from database\n");
	fprintf(stderr, "p-rw:           parallel read and write\n\n");
	
	fprintf(stderr, "op_num:         number of operations to execute\n");
	fprintf(stderr, "thread_num:     number of threads (>0) that execute op_num operations\n");
	fprintf(stderr, "writer_perc:    percentage of write operations in [0, 100] (p-rw mode only)\n");
	fprintf(stderr, "random:		if present, operation values are randomized\n");
	exit(1);
}

int main(int argc,char** argv)
{
	long int count;
	int thread_count;
	DB* db;

	srand(time(NULL));
	printf("%s", argv[1]);
	if (argc < 3) usage();
	if ((argc < 4) && (strcmp(argv[1], "p-read") == 0)) usage();
	if ((argc < 5 && strcmp(argv[1], "p-rw") == 0)) usage();

	db = db_open(DATAS);

	FILE *fstats = fopen("stats.csv", "a+");
	if(fstats == NULL)
	{
		fprintf(stderr, "Failed to create/open stats file, exiting");
		exit(1);
	}
	else
	{
		int c = fgetc(fstats);
		if (c == EOF)
			fprintf(fstats, "OPERATION_TYPE, OP_COUNT, THREAD_COUNT, OP_TYPE_PERCENT, RANDOM_KEYS(Y/N), TOTAL_COST, AVG_COST_PER_THREAD, TOTAL_COST_PER_OP, AVG_COST_PER_OP, TOTAL_THROUGHPUT, AVG_THROUGHPUT\n");
	}
	
	if (strcmp(argv[1], "write") == 0) {
		int r = 0;
		count = atoi(argv[2]);
		
		_print_header(count);
		_print_environment();
		if (argc > 3) r = 1;
		_write_test(db, count, r);
		fprintf(stdout, "Total writer cost: %fsec, Total writer cost per operation: %fsec/op, Total writer throughput: %fops/sec\n",
						writers_total_cost, writers_total_cost_per_op, writers_total_throughput);
		fprintf(fstats, "%s, %ld, 1, 100, %d, %f, -1, %f, -1, %f, -1\n",
		argv[1], count, r, writers_total_cost, writers_total_cost_per_op, writers_total_throughput);
	} else if (strcmp(argv[1], "read") == 0) {
		int r = 0;
		count = atoi(argv[2]);
		
		_print_header(count);
		_print_environment();
		if (argc > 3) r = 1;
		_read_test(db, count, r);
		fprintf(stdout, "Total reader cost: %fsec, Total reader cost per operation: %fsec/op, Total reader throughput: %fops/sec\n",
						readers_total_cost, readers_total_cost_per_op, readers_total_throughput);
		fprintf(fstats, "%s, %ld, 1, 100, %d, %f, -1, %f, -1, %f, -1\n",
		argv[1], count, r, readers_total_cost, readers_total_cost_per_op, readers_total_throughput);
	} else if (strcmp(argv[1], "p-read") == 0) {
		thread_count = atoi(argv[3]);
		if (thread_count <= 0) usage();
		int r = 0;
		count = atoi(argv[2]);
		
		_print_header(count);
		_print_environment();
		if (argc == 5) r = 1;
		
		pthread_t threads[thread_count];
		pthread_args args = {db, count, r};

		for(int i = 0; i < thread_count; i++) pthread_create(&threads[i], NULL, parallel_read, (void*)&args);
		for(int i = 0; i < thread_count; i++) pthread_join(threads[i], NULL);
		fprintf(stdout, "Total reader cost: %fsec, Total reader cost per operation: %fsec/op, Total reader throughput: %fops/sec\n",
						readers_total_cost, readers_total_cost_per_op, readers_total_throughput);
		fprintf(stdout, "Average reader cost per thread: %fsec, Average reader cost per operation: %fsec/op, Average reader thread throughput: %fops/sec\n",
						readers_total_cost/(double)thread_count, readers_total_cost_per_op/(double)thread_count, readers_total_throughput/(double)thread_count);
		fprintf(fstats, "%s, %ld, %d, 100, %d, %f, %f, %f, %f, %f, %f\n",
		argv[1], count, thread_count, r, readers_total_cost, readers_total_cost/(double)thread_count, readers_total_cost_per_op, readers_total_cost_per_op/(double)thread_count, readers_total_throughput, readers_total_throughput/(double)thread_count);
	} else if (strcmp(argv[1], "p-rw") == 0) {
		thread_count = atoi(argv[3]);
		if (thread_count <= 0) usage();
		int r = 0;
		int writer_perc;
		count = atoi(argv[2]);

		_print_header(count);
		_print_environment();
		if (argc > 5) r = 1;

		thread_count = atoi(argv[3]);
		if (thread_count < 0) usage();

		writer_perc = atoi(argv[4]);
		int writer_count = thread_count/2;
        int reader_count = thread_count - writer_count;

        float w_perc = (float)writer_perc/100;

        int w_op = (count * w_perc)/writer_count;
        int r_op = (count *(1.0f - w_perc))/reader_count;

        pthread_t writers[writer_count];
        pthread_t readers[reader_count];
        pthread_args rargs = {db, r_op, r};
		pthread_args wargs = {db, w_op, r};

		for(int i = 0; i < writer_count; i++) pthread_create(&writers[i], NULL, parallel_write, (void*)&wargs);
		for(int i = 0; i < reader_count; i++) pthread_create(&readers[i], NULL, parallel_read, (void*)&rargs);
		for(int i = 0; i < writer_count; i++) pthread_join(writers[i], NULL);
		for(int i = 0; i < reader_count; i++) pthread_join(readers[i], NULL);
		fprintf(stdout, "Total reader cost: %fsec, Total reader cost per operation: %fsec/op, Total reader throughput: %fops/sec\n",
						readers_total_cost, readers_total_cost_per_op, readers_total_throughput);
		fprintf(stdout, "Average reader cost per thread: %fsec, Average reader cost per operation: %fsec/op, Average reader thread throughput: %fops/sec\n",
						readers_total_cost/(double)reader_count, readers_total_cost_per_op/(double)reader_count, readers_total_throughput/(double)reader_count);
		fprintf(stdout, "Total writer cost: %fsec, Total writer cost per operation: %fsec/op, Total writer throughput: %fops/sec\n",
						writers_total_cost, writers_total_cost_per_op, writers_total_throughput);
		fprintf(stdout, "Average writer cost per thread: %fsec, Average writer cost per operation: %fsec/op, Average writer thread throughput: %fops/sec\n", 
						writers_total_cost/(double)writer_count, writers_total_cost_per_op/(double)writer_count, writers_total_throughput/(double)writer_count);
		fprintf(fstats, "%s(r), %ld, %d, %d, %d, %f, %f, %f, %f, %f, %f\n",
		argv[1], count, reader_count, (100-writer_perc), r, readers_total_cost, readers_total_cost/(double)reader_count, readers_total_cost_per_op, readers_total_cost_per_op/(double)reader_count, readers_total_throughput, readers_total_throughput/(double)reader_count);
		fprintf(fstats, "%s(w), %ld, %d, %d, %d, %f, %f, %f, %f, %f, %f\n",
		argv[1], count, writer_count, writer_perc, r, writers_total_cost, writers_total_cost/(double)writer_count, writers_total_cost_per_op, writers_total_cost_per_op/(double)writer_count, writers_total_throughput, writers_total_throughput/(double)writer_count);
	} else usage();
	db_close(db);

	return 1;
}

