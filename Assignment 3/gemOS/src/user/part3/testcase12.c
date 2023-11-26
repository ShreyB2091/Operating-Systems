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


    mem1[12*PAGE]='X';

    int pid = cfork();
    if(pid)
    {

        sleep(600);
        printf("parent for testing mprotect cases\n");
        pmap(1);

        if(mem1[12*PAGE]!='X')
        {
            printf("failed 13\n");
            return 1;
        }

        for(u64 i=0;i<25;i++)
        {
            if(i!=8 && i!=9 && !(i<=19 && i>=14))
            {
                int x=mem1[PAGE*i];
            }
        }
        for(u64 i=0;i<24;i++)
        {
            if(i<7 || (i>=12 && i<14 ) || (i==20))
            {
                mem1[PAGE*i]='X';
            }
        }






        mprotect(MMAP_AREA_START+PAGE*34, PAGE*11, PROT_READ);

        pmap(1);


        *(u64*)(MMAP_AREA_START+PAGE*50)='A';
        printf("test case failed 16.5\n");
        return -1;

        
        
    }
    else
    {
        printf("child\n");
        pmap(1);
        if(mem1[12*PAGE]!='X')
        {
            printf("failed 17\n");
            return 1;
        }

        for(u64 i=0;i<25;i++)
        {
            if(i<7 || (i>=12 && i<4 ) || (i==20))
            {
                mem1[PAGE*i]='X';
            }
        }
        for(u64 i=0;i<24;i++)
        {
            if(i!=8 && i!=9 && !(i<=19 && i>=14))
            {
                int x=mem1[PAGE*i];
            }
        }
        pmap(0);
        ret=munmap(MMAP_AREA_START+PAGE, 100*PAGE);
        if(ret==-1)
        {
            printf("failed 18\n");
            return -1;
        }
        
        for(u64 i=1;i<=10;i+=2)
        {
            ret=mmap(MMAP_AREA_START+PAGE*i*i, PAGE*(2*i+1), PROT_READ | PROT_WRITE, MAP_FIXED);
            if(ret!=MMAP_AREA_START+PAGE*i*i)
            {
                printf("failed 19\n");
                return -1;
            }
            for(u64 j=i*i;j<(i+1)*(i+1);j++)
            {
                if(j%3==0)
                {
                    *(u64*)(MMAP_AREA_START+PAGE*j)='A'+j%26;
                }
            }
        }

        pmap(0);
        int pid2=cfork();
        if(pid2)
        {
            sleep(300);
            printf("child parent for testing mmap cases:\n");
            {
                for(u64 i=1;i<=10;i+=2)
                {
                    for(u64 j=i*i;j<(i+1)*(i+1);j++)
                    {
                        char x=*(u64*)(MMAP_AREA_START+PAGE*j);
                        if(j%3==0 && x!=('A'+j%26))
                        {
                            printf("failed 19.5\n");
                            return 1;
                        }
                    }
                }
                pmap(1);
                ret=mmap(MMAP_AREA_START+PAGE*5, 4*PAGE-4090, PROT_READ|PROT_WRITE, MAP_FIXED);
                if(ret==-1)
                {
                    printf("failed 20\n");
                    return 1;
                }
                pmap(1);
                ret=mmap(NULL, 8*PAGE-1, PROT_READ, 0);
                if(ret==-1)
                {
                    printf("failed 21\n");
                    return 1;
                }
                pmap(1);
                ret=mmap(0, 8*PAGE-1, PROT_READ, MAP_FIXED);
                if(ret!=-1)
                {
                    printf("failed 22\n");
                    return 1;
                }
            }
        }
        else
        {
            printf("child child for testing munmap cases :\n");

            for(u64 i=1;i<=10;i+=2)
            {
                for(u64 j=i*i;j<(i+1)*(i+1);j+=10)
                {
                    char x=*(u64*)(MMAP_AREA_START+PAGE*j);
                    if(j%3==0 && x!=('A'+j%26))
                    {
                        printf("failed 19.5\n");
                        return 1;
                    }
                }
            }

            munmap(MMAP_AREA_START+7*PAGE, 100*PAGE);
            pmap(1);
        }


    }

    int cow_fault = get_cow_fault_stats();
    printf("cow fault: %x \n", cow_fault);
    if(cow_fault > 0)
        printf("Testcase passed\n");
    else
        printf("Testcase failed\n");

    return 0;
}


/*
child


        ###########     VM Area Details         ################
        VM_Area:[6]             MMAP_Page_Faults[1]

        [0x180201000    0x180211000] #PAGES[16] R W _
        [0x180211000    0x180212000] #PAGES[1]  R _ _
        [0x180214000    0x180216000] #PAGES[2]  R _ _
        [0x180216000    0x180218000] #PAGES[2]  R W _
        [0x18021E000    0x18021F000] #PAGES[1]  R W _
        [0x18021F000    0x180223000] #PAGES[4]  R _ _

        ###############################################

VM_Area:[6]     MMAP_Page_Faults[16]
VM_Area:[5]     MMAP_Page_Faults[35]
child child for testing munmap cases :


        ###########     VM Area Details         ################
        VM_Area:[1]             MMAP_Page_Faults[41]

        [0x180201000    0x180204000] #PAGES[3]  R W _

        ###############################################

cow fault: 0x5
Testcase passed
child parent for testing mmap cases:


        ###########     VM Area Details         ################
        VM_Area:[-1]            MMAP_Page_Faults[77]

        [0x180201000    0x180204000] #PAGES[3]  R W _
        [0x180209000    0x180210000] #PAGES[7]  R W _
        [0x180219000    0x180224000] #PAGES[11] R W _
        [0x180231000    0x180240000] #PAGES[15] R W _
        [0x180251000    0x180264000] #PAGES[19] R W _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[5]             MMAP_Page_Faults[77]

        [0x180201000    0x180204000] #PAGES[3]  R W _
        [0x180205000    0x180210000] #PAGES[11] R W _
        [0x180219000    0x180224000] #PAGES[11] R W _
        [0x180231000    0x180240000] #PAGES[15] R W _
        [0x180251000    0x180264000] #PAGES[19] R W _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[6]             MMAP_Page_Faults[77]

        [0x180201000    0x180204000] #PAGES[3]  R W _
        [0x180205000    0x180210000] #PAGES[11] R W _
        [0x180210000    0x180218000] #PAGES[8]  R _ _
        [0x180219000    0x180224000] #PAGES[11] R W _
        [0x180231000    0x180240000] #PAGES[15] R W _
        [0x180251000    0x180264000] #PAGES[19] R W _

        ###############################################

cow fault: 0x6
Testcase passed
parent for testing mprotect cases


        ###########     VM Area Details         ################
        VM_Area:[-1]            MMAP_Page_Faults[77]

        [0x180201000    0x180211000] #PAGES[16] R W _
        [0x180211000    0x180212000] #PAGES[1]  R _ _
        [0x180214000    0x180216000] #PAGES[2]  R _ _
        [0x180216000    0x180218000] #PAGES[2]  R W _
        [0x18021E000    0x18021F000] #PAGES[1]  R W _
        [0x18021F000    0x180223000] #PAGES[4]  R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[6]             MMAP_Page_Faults[94]

        [0x180201000    0x180211000] #PAGES[16] R W _
        [0x180211000    0x180212000] #PAGES[1]  R _ _
        [0x180214000    0x180216000] #PAGES[2]  R _ _
        [0x180216000    0x180218000] #PAGES[2]  R W _
        [0x18021E000    0x18021F000] #PAGES[1]  R W _
        [0x18021F000    0x180223000] #PAGES[4]  R _ _

        ###############################################

do_page_fault: (Sig_Exit) PF Error @ [RIP: 0x100001079] [accessed VA: 0x180232000] [error code: 0x6]
*/

