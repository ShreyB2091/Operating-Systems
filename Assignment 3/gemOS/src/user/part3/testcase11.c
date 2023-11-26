#include<ulib.h>

#define MMAP_AREA_START ((long)(0x180200000))
#define PAGE ((long)4096)

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
     
    char* mem1=mmap(MMAP_AREA_START + 10*PAGE, 25*PAGE-10,PROT_READ|PROT_WRITE, 0);
    if(mem1!=MMAP_AREA_START + 10*PAGE)
    {
        printf("failed 1\n");
        return 1;
    }
    long ret=mprotect(mem1+7*PAGE, 5*PAGE-4090, PROT_READ);
    if(ret!=0)
    {
        printf("failed 2\n");
        return 1;
    }
    ret=munmap(mem1+PAGE*8, PAGE*2);
    if(ret!=0)
    {
        printf("failed 3\n");
        return 1;
    }
    ret=mprotect(mem1+21*PAGE, 4*PAGE-1, PROT_READ);
    if(ret!=0)
    {
        printf("failed 4\n");
        return 1;
    }
    ret=munmap(mem1+14*PAGE, 6*PAGE);
    if(ret!=0)
    {
        printf("failed 5\n");
        return 1;
    }
    ret=mmap(MMAP_AREA_START+11*PAGE,9*PAGE, PROT_READ|PROT_WRITE, 0 );
    if(ret!=MMAP_AREA_START+PAGE)
    {
        printf("failed 6, %x\n", (ret-MMAP_AREA_START));
        return 1;
    }
    ret=mmap(MMAP_AREA_START+9*PAGE, 2*PAGE, PROT_READ, MAP_FIXED);
    if(ret!=-1)
    {
        printf("failed 7\n");
        return 1;
    }

    int fd=open("new_file.txt",O_RDWR| O_CREAT );
    if(fd!=3)
    {
        printf("failed 8\n");
        return 1;
    }
    char buff[100]="some text";
    ret=write(fd, buff, 10);
    if(ret!=10)
    {
        printf("failed 9\n");
        return 1;
    }
	lseek(fd, 0, SEEK_SET); // seek to start of file
	

    mem1[12*PAGE]='X';

    int pid = cfork();
    if(pid)
    {
        sleep(600);
        printf("parent\n");
        pmap(1);

        int fd2=open("new_file2.txt", O_RDWR | O_CREAT );
        if(fd2!=4)
        {
            printf("failed 10 %d\n", fd2);
            return 1;
        }
    //     char buf3[100];
    //     ret=read(3, buf3, 10);
    //     if(ret!=10)
    //     {
    //         printf("failed 11\n");
    //         return 1;
    //     }
    //     char buff2[100]="other txt";
    //     if(ustrcmp(buf3, buff2)!=0)
    //     {
    //         printf("failed 12\n");
    //         return 1;
    //     }
    //     if(mem1[12*PAGE]!='X')
    //     {
    //         printf("failed 13\n");
    //         return 1;
    //     }

    //     for(int i=0;i<24;i++)
    //     {
    //         mem1[PAGE*i]='A';
    //     }
    //     pmap(0);
        
    }
    else
    {
        printf("child\n");
        pmap(1);
        char buff2[100];
        ret=read(3, buff2, 10);
        if(ret!=10)
        {
            printf("failed 14 %d\n", ret);
            return 1;
        }
    //     if(ustrcmp(buff, buff2)!=0)
    //     {
    //         printf("failed 15\n");
    //         return 1;
    //     }
    //     char buff3[100]="other txt";
    //     ret=write(3, buff3, 10);
    //     if(ret!=10)
    //     {
    //         printf("failed 16\n");
    //         return 1;
    //     }
    //     close(3);

    //     if(mem1[12*PAGE]!='X')
    //     {
    //         printf("failed 17\n");
    //         return 1;
    //     }

    //     for(int i=0;i<24;i++)
    //     {
    //         mem1[PAGE*i]='B';
    //     }
    //     pmap(0);

    }

    int cow_fault = get_cow_fault_stats();
    printf("cow fault: %x \n", cow_fault);
    if(cow_fault > 0)
        printf("Testcase passed\n");
    else
        printf("Testcase failed\n");
}