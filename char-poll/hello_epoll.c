#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <strings.h>

#define FIFO_CLEAR 0x1
#define BUFFER_LEN 20

void main(void)
{
    int fd;
    struct epoll_event ev_hello;
    int err;
    int epfd;

    fd = open("/dev/hello0", O_RDONLY | O_NONBLOCK);
    if (fd != -1) {
        if (ioctl(fd, FIFO_CLEAR, 0) < 0)
            printf("ioctl command failed\n");

        epfd = epoll_create(1);
        if (epfd < 0) {
            perror("epoll_create()"); return;
        }

        bzero(&ev_hello, sizeof(struct epoll_event));
        ev_hello.events = EPOLLIN | EPOLLPRI;

        err = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev_hello);
        if (err < 0) {
            perror("epoll_ctl()"); return;
        }

        err = epoll_wait(epfd, &ev_hello, 1, 15000);
        if (err < 0) {
            perror("epoll_wait()");
        } else if (err == 0) {
            printf("No data input in FIFO within 15 seconds.\n");
        } else {
            printf("FIFO is not empty\n");
        }

        err = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev_hello);
        if (err < 0)
            perror("epoll_ctl()");
    } else {
        printf("Device open failure\n");
    }
}
