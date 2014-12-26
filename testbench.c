/**********************************************************************
 *
 *  SLAB BENCHMARK CASE 
 *
 *
 *  HOW TO BUILD:
 *
 *   - unix:     gcc testbench.c -o testbench
 *   - windows:  cl testbench.c 
 *
 * 
 *  TESTING RESULT: Intel Cetrino Duo 1.8GHz
 *
 *   - benchmark with malloc / free:
 *        total time is 16634ms maxmem is 104857599bytes
 *   - benchmark with ikmem_alloc / ikmem_free:
 *        total time is 1715ms maxmem is 104857599bytes
 *        pages in=76576 out=74435 waste=34.72%
 *   
 *
 *  NOTE:
 *  
 *  You can decrease "CASE_TIMES" under an old computer, "4000000" 
 *  times will cost almost 20 seconds in my Cetrino Duo.
 *  
 *
 **********************************************************************/


//#define IMLOG_ENABLE
//#define IMEM_DEBUG
#include "imembase.c"

#define CASE_TIMES		4000000
#define CASE_LIMIT		(64 * 1024)		// 0-64KB each block
#define CASE_HIWATER	(100 << 20)		// 100MB memory
#define CASE_PROB		(55)			// probability of (alloc) is 60% 

#if (defined(_WIN32) || defined(WIN32))
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#elif defined(__unix)
#include <sys/time.h>
#include <unistd.h>
#else
#error it can only be compiled under windows or unix
#endif

#include <stdio.h>
#include <time.h>

/* gettime */
iulong gettime()
{
	#if (defined(_WIN32) || defined(WIN32))
	return timeGetTime();
	#else
	static struct timezone tz={ 0,0 };
	struct timeval time;
	gettimeofday(&time,&tz);
	return (time.tv_sec * 1000 + time.tv_usec / 1000);
	#endif
}

/* random */
#define RANDOM(n) (xrand() % (iulong)(n))
iulong xseed = 0x1234567;
iulong xrand(void)
{
	return (((xseed = xseed * 214013L + 2531011L) >> 16) & 0x7fffffff);
}

/* two alloc methods dependent on the variable of kmem_turnon */
int kmem_turnon = 0;

void *xmalloc(iulong size)
{
	iulong pos, end;
	ilong *ptr;
	if (kmem_turnon) ptr = (ilong*)ikmem_malloc(size);
	else ptr = (ilong*)malloc(size);
	end = size / sizeof(ilong);
	for (pos = 0; pos < end ; pos += 1024)
		ptr[pos] = (ilong)pos;
	return ptr;
}

void xfree(void *ptr)
{
	if (kmem_turnon) ikmem_free(ptr);
	else free(ptr);
}


/* benchmark alloc / free */
ilong memory_case(ilong limit, ilong hiwater, ilong times, int rate, ilong seed)
{
	struct case_t { ilong size, m1, m2; char *ptr; };
	struct case_t *record, *p;
	iulong startup = 0, water = 0, maxmem = 0, sizev = 0;
	ilong pagesize, page_in, page_out, page_inuse;
	double waste;
	char *ptr;
	int count = 0, maxium = 0;
	int mode, size, pos;
	int retval = 0;

	record = (struct case_t*)malloc(100);
	xseed = seed;
	startup = gettime();

	for (; times >= 0; times--) {
		mode = 0;
		if (RANDOM(100) < rate) {
			size = RANDOM(limit);
			if (size < sizeof(ilong) * 2) size = sizeof(ilong) * 2;
			if (water + size >= hiwater) mode = 1;
		}	else {
			mode = 1;
		}
		
		/* TO ALLOC new memory block */
		if (mode == 0) {
			if (count + 4 >= maxium) {
				maxium = maxium? maxium * 2 : 8;
				maxium = maxium >= count + 4? maxium : count + 4;
				sizev = maxium * sizeof(struct case_t);
				record = (struct case_t*)realloc(record, 
					maxium * sizeof(struct case_t));
				assert(record);
			}
			ptr = xmalloc(size);
			assert(ptr);
			/* record pointer */
			p = &record[count++];
			p->ptr = ptr;
			p->size = size;
			p->m1 = rand() & 0x7ffffff;
			p->m2 = rand() & 0x7ffffff;
			water += size;
			/* writing magic data */
			*(ilong*)ptr = p->m1;
			*(ilong*)(ptr + p->size - sizeof(ilong)) = p->m2;
		}	
		/* TO FREE old memory block */
		else if (count > 0) {
			pos = RANDOM(count);
			record[count] = record[pos];
			p = &record[count];
			record[pos] = record[--count];
			ptr = p->ptr;
			/* checking magic data */
			if (*(ilong*)ptr != p->m1) {
				printf("[BAD] bad magic1: %lxh size=%d times=%d\n", ptr, p->size, times);
				return -1;
			}
			if (*(ilong*)(ptr + p->size - sizeof(ilong)) != p->m2) {
				printf("[BAD] bad magic2: %lxh size=%d times=%d\n", ptr, p->size, times);
				return -1;
			}
			xfree(ptr);
			water -= p->size;
		}
		if (water > maxmem) maxmem = water;
	}

	/* state page-supplier */
	page_in = imem_gfp_default.pages_new;
	page_out = imem_gfp_default.pages_del;
	page_inuse = imem_gfp_default.pages_inuse;
	pagesize = imem_page_size;

	/* free last memory blocks */
	for (pos = 0; pos < count; pos++) {
		p = &record[pos];
		ptr = p->ptr;
		if (*(ilong*)ptr != p->m1) {
			printf("[BAD] bad magic: %lxh\n", ptr);
			return -1;
		}
		if (*(ilong*)(ptr + p->size - sizeof(ilong)) != p->m2) {
			printf("[BAD] bad magic: %lxh\n", ptr);
			return -1;
		}
		xfree(ptr);
	}
	/* caculate time */
	startup = gettime() - startup;
	free(record);

	if (kmem_turnon && water > 0) {
		waste = (pagesize * page_inuse - water) * 100.0 / water;
	}	else {
		waste = 0;
	}

	printf("timing: total time is %lums maxmem is %lubytes\n", startup, maxmem);
	if (kmem_turnon) {
		printf(	"status: pages (in=%lu out=%lu inuse=%lu) waste=%.2f%%\n",
			page_in, page_out, page_inuse, waste);
	}
	printf("\n");
	return (ilong)startup;
}

int main(void)
{
	char key;

	/* init kmem interface */
	ikmem_init(0, 0, 0);

	/* testing with malloc / free */
	printf("Benchmark with malloc / free:\n");
	kmem_turnon = 0;
	memory_case(CASE_LIMIT, CASE_HIWATER, CASE_TIMES, CASE_PROB, 256);

	/* mutex implementation in cygwin is to slow, so disable mutex */
	#ifdef __CYGWIN__
	imutex_disable = 1;
	#endif
	//imutex_neglect(1);
	//imutex_disable = 1;

	/* testing with ikmem_alloc / ikmem_free */
	printf("Benchmark with ikmem_alloc / ikmem_free:\n");
	kmem_turnon = 1;
	memory_case(CASE_LIMIT, CASE_HIWATER, CASE_TIMES, CASE_PROB, 256);

	printf("anykey to continue....\n");
	scanf("%c", &key);

	/* clean environment */
	ikmem_destroy();

	return 0;
}


