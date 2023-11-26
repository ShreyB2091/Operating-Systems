#include<ulib.h>

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  int pages = 4096;
  char * mm1 = mmap(NULL, pages*2, PROT_READ|PROT_WRITE, 0);
 
 
  mm1[15] = 'X';
  

  int pid = cfork();
  if(pid){
    sleep(60);
    mm1[0]='Y';
    pid = cfork();
  }
  else{
  
   mm1[0]='X';
   exit(0);
  }
  mm1[1]='D';
  mm1[5000]='S';
  if(!pid){
    exit(0);
  }
 
int cow_fault = get_cow_fault_stats();
printf("#cowfaults = %d\n", cow_fault);
 
  if(cow_fault == 10)
      printf("cow fault testcase passed\n");
  else
      printf("cow faults testcase failed\n");

  return 0;
}