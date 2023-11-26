#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../mylib.h"

#define NUM 4
#define _1GB (1024 * 1024 * 1024)

//Handling large allocations
int main()
{
	char *p[NUM];
	char *q = 0;
	int ret = 0;
	int a = 0;

	for(int i = 0; i < NUM; i++)
	{
		p[i] = (char*)memalloc(_1GB);
		// printf("%u\t%lu\n", p[i], *((unsigned long*)p[i] - 1));
		if((p[i] == NULL) || (p[i] == (char*)-1))
		{
			printf("1.Testcase failed\n");
			return -1;
		}
		// printf("Allocation done\n");
		for(int j = 0; j < _1GB; j++)
		{
			// printf("a\n");
			p[i][j] = 'a';
		}
	}
	// printf("Starting deallocation\n");
	for(int i = 0; i < NUM; i++)
	{
		ret = memfree(p[i]);
		// printf("Deallocated %d\n", i);
		if(ret != 0)
		{
			printf("2.Testcase failed\n");
			return -1;
		}
	}

	printf("Testcase passed\n");
	return 0;
}


