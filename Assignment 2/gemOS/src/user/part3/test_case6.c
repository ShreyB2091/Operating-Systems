#include <ulib.h>

static int func1(int a,int b)
{
  return a + b;
}

int func2(int a, int b, int c, int d) {
  int r1 = func1(a, b);
  int r2 = func1(c, d);
  int r3 = func1(r1, r2);
  return r3;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{

  int ret = 0;
  int ftrace_fd = 0;
  u64 ftrace_buff[4096];

  ftrace_fd = create_trace_buffer(O_RDWR);
  if(ftrace_fd != 3)
  {
    printf("0. Test case failed\n");
    return -1;
  }
  
  ret = ftrace((unsigned long)&func1, ADD_FTRACE, 2, ftrace_fd);
  if(ret != 0)
  { 
    printf("1. Test case failed\n");
    return -1;
  }

  ret = ftrace((unsigned long)&func1, ENABLE_BACKTRACE, 0, ftrace_fd);
  if(ret != 0)
  { 
    printf("2. Test case failed\n");
    return -1;
  }
  // func1(5,10);
  ret = ftrace((unsigned long)&func2, ADD_FTRACE, 4, ftrace_fd);
  if(ret != 0)
  { 
    printf("3. Test case failed\n");
    return -1;
  }

  ret = ftrace((unsigned long)&func2, ENABLE_BACKTRACE, 0, ftrace_fd);
  if(ret != 0)
  { 
    printf("4. Test case failed\n");
    return -1;
  }
  func2(1, 2, 3, 4);

  ret = read_ftrace(ftrace_fd, ftrace_buff, 10);
  printf("RET %d\n", ret);
  if(ret != 200) {
    printf("5. Test case failed\n");
    return -1;
  }

  if((u64*)(((u64*)ftrace_buff)[0]) != (u64*)(&func2))
	{ 
		printf("6. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[1] != 0x1)
	{ 
		printf("7. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[2] != 0x2)
	{ 
		printf("8. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[3] != 0x3)
	{ 
		printf("9. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[4] != 0x4)
	{ 
		printf("10. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[5]) != (u64*)(&func2))
	{ 
		printf("11. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[6]) != (u64*)(0x100000fde))
	{ 
		printf("12. Test case failed\n");
		return -1;
	}

  if((u64*)(((u64*)ftrace_buff)[7]) != (u64*)(&func1))
	{ 
		printf("6. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[8] != 0x1)
	{ 
		printf("7. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[9] != 0x2)
	{ 
		printf("8. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[10]) != (u64*)(&func1))
	{ 
		printf("11. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[11]) != (u64*)(0x100000e27))
	{ 
		printf("12. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[12]) != (u64*)(0x100000fde))
	{ 
		printf("12. Test case failed\n");
		return -1;
	}

  if((u64*)(((u64*)ftrace_buff)[13]) != (u64*)(&func1))
	{ 
		printf("6. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[14] != 0x3)
	{ 
		printf("7. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[15] != 0x4)
	{ 
		printf("8. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[16]) != (u64*)(&func1))
	{ 
		printf("11. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[17]) != (u64*)(0x100000e39))
	{ 
		printf("12. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[18]) != (u64*)(0x100000fde))
	{ 
		printf("12. Test case failed\n");
		return -1;
	}

  if((u64*)(((u64*)ftrace_buff)[19]) != (u64*)(&func1))
	{ 
		printf("6. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[20] != 0x3)
	{ 
		printf("7. Test case failed\n");
		return -1;
	}
  if(((u64*)ftrace_buff)[21] != 0x7)
	{ 
		printf("8. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[22]) != (u64*)(&func1))
	{ 
		printf("11. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[23]) != (u64*)(0x100000e4b))
	{ 
		printf("12. Test case failed\n");
		return -1;
	}
  if((u64*)(((u64*)ftrace_buff)[24]) != (u64*)(0x100000fde))
	{ 
		printf("12. Test case failed\n");
		return -1;
	}
  printf("Testcase passed\n");
  return 0;
}