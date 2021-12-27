/*** Credits for this test script: Shubham Mishra (18CS10066) ***/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
#include <assert.h>

#define NUM_FORKS 2
#define PROC_FILENAME "/proc/partb_1_18CS10069"

int proc_main(int *entries, int n)
{
    int fd = open(PROC_FILENAME, O_RDWR);
    if (fd == -1){
        printf("Open Error: %d\n", errno);
        return 0;
    }
    int out = -1;

    char c = (char)n;
    write(fd, &c, sizeof(char));

    for (int i = 0; i < n; i++){
        errno = 0;
        int ret = write(fd, &entries[i], sizeof(int));
        printf("Write: %d %d %d\n", getpid(), ret, errno);
        usleep(100);
    }

    for (int i = 0; i < n; i++){
        errno = 0;
        int ret = read(fd, &out, sizeof(int));
        printf("Read: %d %d %d %d\n", getpid(), out, ret, errno);
        entries[i] = out;
        usleep(100);
    }
    
    close(fd);

    return 0;
}

int main()
{
    int payload[] = {1, 2, 3, 4};
    int desired_result[] = {3, 1, 2, 4};

    int pid = getpid();
    for (int i = 0; i < NUM_FORKS; i++){
        fork();
    }

    proc_main(payload, 4);

    printf("Pid %d Result: [", getpid());
    for (int i = 0; i < 4; i++){
        printf("%d ", payload[i]);
    }
    printf("]\n");

    for (int i = 0; i < 4; i++){
        assert(payload[i] == desired_result[i]);
    }

    int status;
    wait(&status);
    wait(&status);

    return 0;
}