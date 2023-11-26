#include<ulib.h>

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5){
  int fd = create_trace_buffer(O_RDWR);
  if(fd != 3){
    printf("error allocing buffer\n");
    return -1;
  }
  int retval = read(fd, &main, 10);
  if(retval != -EBADMEM){
    printf("no write perms in main code!\n");
    return -1;
  }
  retval = write(fd, &main, 10);
  if(retval != 10){
    printf("read perms ok in main code!\n");
    return -1;
  }
  retval = write(fd, NULL, 10);
  if(retval != -EBADMEM){
    printf("no read perms in NULL!\n");
    return -1;
  }
  retval = read(fd, NULL, 10);
  if(retval != -EBADMEM){
    printf("no write perms in NULL!\n");
    return -1;
  }
  close(fd);
	printf("tc passed\n");
	return 0;
}