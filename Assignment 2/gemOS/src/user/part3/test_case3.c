#include<ulib.h>

static int fib(int a)
{
	if(a == 0) return 0;
	if(a == 1) return 1;
	return fib(a-1) + fib(a-2);
}


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
	int ret = 0;
	int ftrace_fd = 0;
	u64 ftrace_buff[512];

	ftrace_fd = create_trace_buffer(O_RDWR);
	if(ftrace_fd != 3)
	{
		printf("0. Test case failed\n");
		return -1;
	}
	
	ret = ftrace((unsigned long)&fib, ADD_FTRACE, 1, ftrace_fd);
	if(ret != 0)
	{ 
		printf("1. Test case failed\n");
		return -1;
	}

	int x = fib(5);
	if(x != 5)
	{ 
		printf("2. Test case failed\n");
		return -1;
	}
	ret = ftrace((unsigned long)&fib, ENABLE_FTRACE, 0, ftrace_fd);
	if(ret != 0)
	{ 
		printf("3. Test case failed\n");
		return -1;
	}

	x = fib(4);
	if(x != 3)
	{ 
		printf("4. Test case failed\n");
		return -1;
	}
	int read_ret = read_ftrace(ftrace_fd, ftrace_buff, 10);
	/*for(int i = 0; i<(read_ret/8); i++){
printf("ftrace_buff[%d] : %x\n", i*8, ftrace_buff[i]);
	}*/
	if(read_ret != 144)
	{ 
		printf("5. Test case failed\n");
		return -1;
	}
	int j = 0;

	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("8. Test case failed\n");
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x4)
	{ 
		printf("9. Test case failed\n");
		return -1;
	}

	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("10. Test case failed\n");
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x3)
	{ 
		printf("11. Test case failed\n");
		return -1;
	}
	
	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("12. Test case failed\n");
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x2)
	{ 
		printf("13. Test case failed\n");
		return -1;
	}

	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("14. Test case failed\n");
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x1)
	{ 
		printf("15. Test case failed\n");
		return -1;
	}
	int k = 16;
	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x0)
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x1)
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x2)
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x1)
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fib))
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	if(((u64*)ftrace_buff)[j++] != 0x0)
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	ret = ftrace((unsigned long)&fib, REMOVE_FTRACE, 0, ftrace_fd);
	if(ret != 0)
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	ret = ftrace((unsigned long)&fib, REMOVE_FTRACE, 0, ftrace_fd);
	if(ret != -1)
	{ 
		printf("%d. Test case failed\n", k++);
		return -1;
	}

	close(ftrace_fd);

	printf("Test case passed\n");

	return 0;
}