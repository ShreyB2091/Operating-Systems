#include <ulib.h>

/*simple cfork testcase */
//made by Geetika and Aditya
int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
    long pid;
    long *va = (long *)expand(100, MM_WR);
    *va = 10;
    printf("Main number:%d\n", *va);
    long cow_fault;

    pid = cfork();

    if (pid)
    {
        sleep(60);
        long pid = getpid();
        printf("Parent pid:%u\n", pid);
        printf("Parent number:%d\n", *va);
    }
    else
    {

        long pid = getpid();
        printf("Child pid:%u\n", pid);
        *va = 100;
        printf("Child number:%d\n", *va);
        pid = cfork();

        if (pid)
        {
            sleep(60);
            long pid = getpid();
            printf("Parent pid:%u\n", pid);
            printf("Parent number:%d\n", *va);
        }
        else
        {

            long pid = getpid();
            printf("Child pid:%u\n", pid);
            *va = 1000;
            printf("Child number:%d\n", *va);
        }

        exit(0);
    }

    cow_fault = get_cow_fault_stats();
    printf("cow fault: %x \n", cow_fault);
    if (cow_fault > 0)
        printf("Testcase passed\n");
    else
        printf("Testcase failed\n");
    return 0;
}


/*
Main number:10
Child pid:2
Child number:100
Child pid:3
Child number:1000
Parent pid:1
Parent number:10
cow fault: 0x9
Testcase passed
Parent pid:2
Parent number:100
*/