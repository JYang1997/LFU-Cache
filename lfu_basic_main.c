#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "lfu_basic_sim.h"
#include <assert.h>

int TIME_FLAG = 0;
double tt_time =0;

int trace_processing(LFU_Cache_t* cache, FILE* fd) {

	char *keyStr;
	uint64_t key;
	char* ret;
	char   line[256];

	int totMiss = 0;
	///////////////////progress bar////////////////////////////
	char bar[28];
	int i;
	uint32_t d = 0;
	//first line contain total reference number
	bar[0] = '[';
	bar[26] = ']';
	bar[27]='\0';
	for (i=1; i<=25; i++) bar[i] = ' ';
	ret=fgets(line, 256, fd);
	keyStr = strtok(line, " ");
	keyStr = strtok(NULL, " ");
	uint64_t total = strtoull(keyStr, NULL, 10);
	uint64_t star = total/100;
	///////////////////////////////////////////////////////////
	i=1;
	while ((ret=fgets(line, 256, fd)) != NULL)
	{
		key = strtoull(line, NULL, 10);
		// keyStr = strtok(line, "\n");
		
		struct timeval  tv1, tv2;
		gettimeofday(&tv1, NULL);

		if (access(cache, key) == CACHE_MISS)	totMiss++;
		
		gettimeofday(&tv2, NULL);
		tt_time += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
		d++;
		
		if(d % star == 0)
		{	
			if( d%(star*4) == 0)
				bar[i++] = '#';
			printf("\rProgress: %s%d%% %d", bar, (int)(d/(double)total*100)+1, d);
			fflush(stdout);
		}
		
	}
	printf("\n");
	return totMiss;
}


int main(int argc, char const *argv[])
{
	char buf[1024];


	char* input_format = "arg1: input filename\n \
arg2: Cache Size\n \
arg3: timer flag (1 yes 0 no)\n";

	if(argc < 4) { 
		printf("%s", input_format);
		return 0;
	}
	//open file for read key value
	FILE*       rfd = NULL;
	if(atoi(argv[3]) == 1) TIME_FLAG = 1;
	else if(atoi(argv[3]) == 0) TIME_FLAG = 0;
	else printf("%s", input_format);

	if((rfd = fopen(argv[1],"r")) == NULL)
	{ perror("open error for read"); return -1; }
	

	struct timeval  tv1, tv2;
	gettimeofday(&tv1, NULL);
	//cache init
	LFU_Cache_t *cache = cacheInit(strtoul(argv[2], NULL, 10));
	gettimeofday(&tv2, NULL);
	tt_time += (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);


	int totMiss = trace_processing(cache, rfd);
	fprintf(stderr,"%d, %f\n",cache->capacity, totMiss/(double)cache->totRef);

	if(TIME_FLAG == 1)
		printf ("Total time = %f seconds\n", tt_time);

	cacheFree(cache);
	fclose(rfd);
	return 0;
}

