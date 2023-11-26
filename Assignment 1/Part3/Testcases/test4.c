// #include <stdio.h>
// #include <unistd.h>
// #include <stdlib.h>
// #include "../mylib.h"

// #define _4MB (4 * 1024 * 1024)
// #define _1GB (1024 * 1024 * 1024)

// int main() {
//   void *p = memalloc(_4MB - 8);
//   void *q = memalloc(_4MB - 8);
//   // void *r = memalloc(_4MB - 8);
//   printf("p: %u\n", p);
//   printf("q: %u\n", q);
//   // printf("r: %u\n", r);
//   // memfree(p);
//   // memfree(q);
//   // printf("p: %u\n", p);
//   // printf("q: %u\n", q);
//   // printf("r: %u\n", r);
// }

#include <stdio.h>
#include <unistd.h>
#include "../mylib.h"

//first fit approach + freed memory chunk should be inserted at the head of the free list
//check metadata is maintained properly
int main()
{
	char *p1 = 0;
	char *p2 = 0;
	char *p3 = 0;
	char *p4 = 0;
	char *next = 0;
	int ret = 0;

	p1 = (char *)memalloc(4194296);
	if((p1 == NULL) || (p1 == (void*)-1))
	{
		printf("1.Testcase failed\n");
		return -1;
	}
    p2 = (char *)memalloc(4194296);
	if((p2 == NULL) || (p2 == (void*)-1))
	{
		printf("2.Testcase failed\n");
		return -1;
	}

	// p3 = (char *)memalloc(4194283);
	// if((p3 == NULL) || (p3 == (void*)-1))
	// {
	// 	printf("3.Testcase failed\n");
	// 	return -1;
	// }
	ret = memfree(p2);
	if(ret != 0)
	{
		printf("4.Testcase failed\n");
		return -1;
	}
    ret = memfree(p1);
	if(ret != 0)
	{
		printf("4.Testcase failed\n");
		return -1;
	}
	
	printf("Testcase passed\n");
	// return 0;
}