#include<ulib.h>

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    printf("------------- Part 1 Testcases ---------------\n");

    char buff[4096];
    char big_buff[8192];
    char empty_buff[4096];
    int fd;
    int ret_val;
    for(int i = 0, j = 0; i < 4096; i++) {
        j = i % 26;
        buff[i] = ('A' + j);
    }
    for(int i = 0, j = 0; i < 8192; i++) {
        j = i % 26;
        big_buff[i] = 'A' + j;
    }

    printf("------------- Testcase 1 -------------\n");
    fd = create_trace_buffer(O_RDWR);
    if(fd != 3) {
        printf("1. Test case failed\n");
        return -1;
    }
    ret_val = write(fd, buff, 5);
    if(ret_val != 5) {
        printf("2. Test case failed\n");
        return -1;
    }
    close(fd);
    printf("----------- Testcase 1 passed --------\n");

    printf("------------ Testcase 2 --------------\n");
    fd = create_trace_buffer(O_RDWR);
    if(fd != 3) {
        printf("1. Test case failed\n");
        return -1;
    }
    ret_val = write(fd, buff, 10);
    if(ret_val != 10) {
        printf("2. Test case failed\n");
        return -1;
    }
    ret_val = read(fd, empty_buff, 5);
    if(ret_val != 5) {
        printf("3. Test case failed\n");
        return -1;
    }
    close(fd);
    printf("----------- Testcase 2 passed --------\n");

    printf("------------ Testcase 3 --------------\n");
    fd = create_trace_buffer(O_RDWR);
    if(fd != 3) {
        printf("1. Test case failed\n");
        return -1;
    }
    ret_val = write(fd, buff, 10);
    if(ret_val != 10) {
        printf("2. Test case failed\n");
        return -1;
    }
    ret_val = read(fd, empty_buff, 10);
    if(ret_val != 10) {
        printf("3. Test case failed\n");
        return -1;
    }
    if(empty_buff[0] != 'A' || empty_buff[9] != 'J') {
        printf("4. Test case failed\n");
        return -1;
    }
    close(fd);
    printf("----------- Testcase 3 passed --------\n");

    printf("------------ Testcase 4 --------------\n");
    for(int i = 3; i < 16; i++) {
        fd = create_trace_buffer(O_RDWR);
        if(fd != i) {
            printf("1. Test case failed\n");
            return -1;
        }
    }
    fd = create_trace_buffer(O_RDWR);
    if(fd != -EINVAL) {
        printf("2. Test case failed\n");
        return -1;
    }
    fd = 3;
    ret_val = write(fd, buff, 100);
    if(ret_val != 100) {
        printf("3. Test case failed\n");
        return -1;
    }
    for(int i = 0; i < 10; i++) {
        ret_val = read(fd, empty_buff, 10);
        if(ret_val != 10) {
            printf("4. Test case failed\n");
            return -1;
        }
        if(empty_buff[0] != 'A' + ((10 * i) % 26)) {
            printf("5. Test case failed for i = %d\n", i);
            return -1;
        }
    }
    
    fd = 4;
    ret_val = read(fd, big_buff, 5);
    if(ret_val != 0) {
        printf("6. Test case failed\n");
        return -1;
    }
    for(int i = 3; i < 16; i++) {
        fd = close(i);
        if(fd != 0) {
            printf("7. Test case failed\n");
            return -1;
        }
    }
    printf("----------- Testcase 4 passed --------\n");

    printf("------------ Testcase 5 --------------\n");
    fd = create_trace_buffer(O_RDWR);
    if(fd != 3) {
        printf("1. Test case failed\n");
        return -1;
    }
    buff[0] = 'h';
    buff[1] = 'e';
    buff[2] = buff[3] = 'l';
    buff[4] = 'o';
    ret_val = write(fd, buff, 5);
    if(ret_val != 5) {
        printf("2. Test case failed\n");
        return -1;
    }
    char str[5];
    ret_val = read(fd, str, 100);
    if(ret_val != 5) {
        printf("3. Test case failed\n");
        return -1;
    }
    if(str[0] != 'h' || str[1] != 'e' || str[3] != 'l' || str[4] != 'o') {
        printf("4. Test case failed\n");
        return -1;
    }
    close(fd);
    printf("----------- Testcase 5 passed --------\n");

    printf("------------ Testcase 6 --------------\n");
    dup(2);
    fd = create_trace_buffer(O_RDWR);
    fd = create_trace_buffer(O_RDWR);
    if(fd != 5) {
        printf("1. Test case failed\n");
        return -1;
    }
    ret_val = write(fd - 1, big_buff, 8192);
    if(ret_val != 4096) {
        printf("2. Test case failed\n");
        return -1;
    }
    ret_val = write(fd, buff, 1000);
    if(ret_val != 1000) {
        printf("3. Test case failed\n");
        return -1;
    }
    ret_val = read(fd - 1, empty_buff, 5000);
    if(ret_val != 4096) {
        printf("4. Test case failed\n");
        return -1;
    }
    if(empty_buff[0] != 'A' || empty_buff[4095] != 'A' + (4095 % 26)) {
        printf("5. Test case failed\n");
        return -1;
    }
    close(3);
    close(4);
    close(5);
    ret_val = write(4, buff, 5);
    if(ret_val != -EINVAL) {
        printf("6. Test case failed\n");
        return -1;
    }
    printf("----------- Testcase 6 passed --------\n");


    printf("------------ Testcase 7 --------------\n");
    fd = create_trace_buffer(O_RDWR);
    if(fd != 3) {
        printf("1. Test case failed\n");
        return -1;
    }
    printf("1. Test case passed\n");
    char *test_buff = mmap(NULL, 100, PROT_READ | PROT_WRITE, MAP_POPULATE);
    for(int i = 0; i < 10; i++) {
        test_buff[i] = 'A';
    }
    printf("Memory allocation done\n");
    unsigned long write_buff = 0x140000000;
    printf("%x\n", test_buff);
    ret_val = write(fd, (void *)write_buff, 5);
    printf("Data written\n");
    if(ret_val != -EBADMEM) {
        printf("2. Test case failed\n");
        return -1;
    }
    printf("2. Test case passed\n");
    close(fd);
    printf("----------- Testcase 7 passed --------\n");

    printf("------------ Testcase 8 --------------\n");
    fd = create_trace_buffer(O_RDWR);
    if(fd != 3) {
        printf("1. Test case failed\n");
        return -1;
    }
    write_buff = 0x800000000 - 0x10;
    ret_val = write(fd, (void *)write_buff, 5);
    if(ret_val != 5) {
        printf("2. Test case failed\n");
        return -1;
    }
    write_buff = 0x600000000;
    ret_val = write(fd, (void *)write_buff, 1);
    if(ret_val != -EBADMEM) {
        printf("3. Test case failed\n");
        return -1;
    }
    char *write_buff_2 = mmap (NULL, 4096, PROT_READ|PROT_WRITE, MAP_POPULATE);
    if(!write_buff_2) {
        printf("Unable to process test case\n");
        return 0;
    }
    for (int i = 0; i < 10; i++)
        write_buff_2[i] = 's';

	ret_val = write (fd, write_buff_2, 5);
    if(ret_val != 5) {
        printf("4. Test case failed\n");
        return -1;
    }
    close(fd);
    printf("----------- Testcase 8 passed --------\n");

    return 0;
}