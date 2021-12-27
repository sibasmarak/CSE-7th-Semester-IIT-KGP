/*** Credits for this test script: Shubham Mishra (18CS10066) ***/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#define PROC_FILENAME "/proc/partb_1_18CS10069"

int main()
{
    int fd = open(PROC_FILENAME, O_RDWR);
    int fd2 = open(PROC_FILENAME, O_RDWR);
    assert(fd != -1 && fd2 == -1);

    unsigned int N = 250;
    char c = (char)N;
    int n; // = write(fd, &c, sizeof(char));

    // assert(n < 0 && errno == EINVAL);

    N = 2;
    c = (char)N;
    n = write(fd, &c, sizeof(char));
    assert(n == 1);

    int x = 5;
    errno = 0;
    write(fd, &x, sizeof(int));
    write(fd, &x, sizeof(int));
    write(fd, &x, sizeof(int));
    assert(errno == EACCES);

    errno = 0;
    read(fd, &x, sizeof(int));
    read(fd, &x, sizeof(int));
    read(fd, &x, sizeof(int));
    assert(errno == EACCES);



    close(fd);

    

    return 0;
}