#include <string.h>
#include "../engine/db.h"
#include "../engine/variant.h"
#include "bench.h"

#define DATAS ("testdb")

double readers_total_cost = 0.0f;
double readers_total_cost_per_op = 0.0f;
double readers_total_throughput = 0.0f;

double writers_total_cost = 0.0f;
double writers_total_cost_per_op = 0.0f;
double writers_total_throughput = 0.0f;

void _write_test(DB* db, long int count, int r)
{
	pthread_mutex_init(&lock_w, NULL);
	int i;
	double cost;
	long long start,end;
	Variant sk, sv;

	char key[KSIZE + 1];
	char val[VSIZE + 1];
	char sbuf[1024];

	memset(key, 0, KSIZE + 1);
	memset(val, 0, VSIZE + 1);
	memset(sbuf, 0, 1024);

	start = get_ustime_sec();
	for (i = 0; i < count; i++) {
		if (r)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d adding %s\n", i, key);
		snprintf(val, VSIZE, "val-%d", i);

		sk.length = KSIZE;
		sk.mem = key;
		sv.length = VSIZE;
		sv.mem = val;

		db_add(db, &sk, &sv);
		if ((i % 10000) == 0) {
			fprintf(stderr,"random write finished %d ops%30s\r", 
					i, 
					"");

			fflush(stderr);
		}
	}

	end = get_ustime_sec();
	pthread_mutex_lock(&lock_w);
	cost = end -start;
	writers_total_cost += cost;

	printf(LINE);
	printf("|Random-Write	(done:%ld): %.6f sec/op; %.1f writes/sec(estimated); cost:%.3f(sec);\n"
		,count, (double)(cost / count)
		,(double)(count / cost)
		,cost);	
	writers_total_cost_per_op += (double)(cost / count);
	writers_total_throughput += (double)(count / cost);
	pthread_mutex_unlock(&lock_w);
}

void _read_test(DB* db, long int count, int r)
{
	pthread_mutex_init(&lock_r, NULL);
	int i;
	int ret;
	int found = 0;
	double cost;
	long long start,end;
	Variant sk;
	Variant sv;
	char key[KSIZE + 1];

	start = get_ustime_sec();
	for (i = 0; i < count; i++) {
		memset(key, 0, KSIZE + 1);

		/* if you want to test random write, use the following */
		if (r)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d searching %s\n", i, key);
		sk.length = KSIZE;
		sk.mem = key;
		ret = db_get(db, &sk, &sv);
		if (ret) {
			//db_free_data(sv.mem);
			found++;
		} else {
			INFO("not found key#%s", 
					sk.mem);
    	}

		if ((i % 10000) == 0) {
			fprintf(stderr,"random read finished %d ops%30s\r", 
					i, 
					"");

			fflush(stderr);
		}
	}
	end = get_ustime_sec();
	pthread_mutex_lock(&lock_r);
	cost = end - start;
	readers_total_cost += cost;
	
	printf(LINE);
	printf("|Random-Read	(done:%ld, found:%d): %.6f sec/op; %.1f reads /sec(estimated); cost:%.3f(sec)\n",
		count, found,
		(double)(cost / count),
		(double)(count / cost),
		cost);
	readers_total_cost_per_op += (double)(cost / count);
	readers_total_throughput += (double)(count / cost);
	pthread_mutex_unlock(&lock_r);
}

