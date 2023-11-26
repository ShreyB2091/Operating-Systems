#include<ulib.h>

static int fact(int n, int a)
{
  if(n==1) return 1;
  else return (fact(n-1, a)+n);
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  int ret = 0;
  int ftrace_fd = 0;
  u64 ftrace_buff[8192];

  ftrace_fd = create_trace_buffer(O_RDWR);
  if(ftrace_fd != 3)
  {
    printf("0. Test case failed\n");
    return -1;
  }
  
  ret = ftrace((unsigned long)&fact, ADD_FTRACE, 2, ftrace_fd);
  if(ret != 0)
  { 
    printf("1. Test case failed\n");
    return -1;
  }
  for(int h=0;h<5;h++)
  {
    printf("%d TESTCASE START\n", h);
    ret = ftrace((unsigned long)&fact, ENABLE_FTRACE, 0, ftrace_fd);
    if(ret != 0)
    { 
      printf("2. Test case failed\n");
      return -1;
    }
    fact(128,1);
    // printf("Function calling done\nReading start\n");
    int read_ret = read_ftrace(ftrace_fd, ftrace_buff, 8);
    /*for(int i = 0; i<(read_ret/8); i++){
    printf("ftrace_buff[%d] : %x\n", i*8, ftrace_buff[i]);
    }*/
    printf("read %d\n", read_ret);
    if(read_ret != 192)
    { 
      printf("5. Test case failed\n");
      return -1;
    }
    int j = 0;
    for(int i=128;i>=121;i--)
    {
      if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fact))
      { 
        printf("%d - 6. Test case failed\n", i);
        return -1;
      }
      if(((u64*)ftrace_buff)[j++] != i)
      { 
        printf("%d - 7. Test case failed\n", i);
        return -1;
      }
      if(((u64*)ftrace_buff)[j++] != 1)
      { 
        printf("%d - 7. Test case failed\n", i);
        return -1;
      }
    }
    fact(4,1);
    read_ret = read_ftrace(ftrace_fd, ftrace_buff, 124);
    if(read_ret != 2976)
    { 
      printf("8. Test case failed\n");
      return -1;
    }
    j = 0;
    for(int i=120;i>=-3;i--)
    {
      int k = (i <= 0)? i+4:i;
      if((u64*)(((u64*)ftrace_buff)[j++]) != (u64*)(&fact))
      { 
        printf("%d - 12. Test case failed\n", i);
        return -1;
      }

      if(((u64*)ftrace_buff)[j++] != k)
      { 
        printf("%d - 13. Test case failed\n", i);
        return -1;
      }

      if(((u64*)ftrace_buff)[j++] != 1)
      { 
        printf("%d - 14. Test case failed\n", i);
        return -1;
      }
    }
    ret = ftrace((unsigned long)&fact, DISABLE_FTRACE, 0, ftrace_fd);
    if(ret != 0)
    { 
      printf("9. Test case failed\n");
      return -1;
    }
    printf("%d TESTCASE END\n", h);
  }
  ret = ftrace((unsigned long)&fact, REMOVE_FTRACE, 0, ftrace_fd);
  if(ret != 0)
  { 
    printf("10. Test case failed\n");
    return -1;
  }
  ret = ftrace((unsigned long)&fact, ENABLE_FTRACE, 0, ftrace_fd);
  if(ret != -1)
  { 
    printf("11. Test case failed\n");
    return -1;
  }
  close(ftrace_fd);
  printf("Test case passed\n");

  return 0;
}