#include<ulib.h>

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5){
	int ret = create_trace_buffer(1234);
	if(ret != -EINVAL){
    printf("1. invalid mode check failed\n");
    return -1;
  }

  // functionality testing
  ret = create_trace_buffer(O_READ);
  if(ret != 3){
    printf("2. error in allocating least fd\n");
    return -1;
  }
  char buff[4096];
  for(int i = 0; i<4096; i++){
    buff[i] = 'A' + i%26;
  }

  ret = read(3, buff, 10);
  if(ret != 0){
    printf("4. read from empty buff\n");
    return -1;
  }

  // stress test

  int fd = create_trace_buffer(O_RDWR);
  char readbuff[4096];

  for(int i = 0; i<50; i++){
    int retval = write(fd, buff, i);
    if(retval != i){
      printf("@iter %d failed write call returned %d\n", i, retval);
      return -1;
    }
    int sum = 0;
    for(int j = 0; j<50; j++){
      int retval = read(fd, readbuff+sum, j);
      if(sum + j <= i && retval != j){
        printf("@iterout %d @iterin %d failed read call returned %d\n", i, j, retval);
        return -1;
      }
      else if(sum <= i && sum + j >= i && retval != i - sum){
        printf("@iterout %d @iterin %d failed read call returned (elif) %d\n", i, j, retval);
        return -1;
      }
      else if(sum >= i && retval != 0){
        printf("@iterout %d @iterin %d failed read call returned (last elif) %d\n", i, j, retval);
        return -1;
      }
      sum += j;
    }
    for(int k = 0; k<i; k++){
      if(readbuff[k] != buff[k]){
        printf("@iterout %d @iterin %d failed consistency check\n", i, k);
// printf("%c%c, %c%c\n", buff[0], buff[1], readbuff[0], readbuff[1]);
        return -1;
      }
    }
  }

	// loop over
	ret = write(fd, buff, 3000);
	if(ret != 3000){
		printf("failed write call returned %d\n", ret);
    return -1;
	}
	ret = write(fd, buff, 3000);
	if(ret != 1096){
		printf("failed write call returned %d\n", ret);
    return -1;
	}
	ret = read(fd, readbuff, 100);
	if(ret != 100){
		printf("failed read call returned %d\n", ret);
    return -1;
	}
	for(int i = 0; i<100; i++){
		if(buff[i] != readbuff[i]){
			printf("failed read consistency %d\n", ret);
      return -1;
		}
	}

  close(3);
  ret = create_trace_buffer(1);
  if(ret != 3){
    printf("error in close?\n");
  }
  close(3);
  close(fd);

  printf("tc passed\n");
	return 0;
}