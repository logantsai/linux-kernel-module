#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#define FIFO_CLEAR 0x1
#define BUFFER_LEN 20

void main(void) {

    int fd, num;
    char rd_ch[BUFFER_LEN];
    fd_set r_fds, w_fds;

    fd = open("/dev/hello0", O_RDONLY | O_NONBLOCK);
    if (fd != -1) {
        if (ioctl(fd, FIFO_CLEAR, 0) < 0)
            printf("ioctl command fialed\n");

        while (1) {
            FD_ZERO(&r_fds);
            FD_ZERO(&w_fds);
            FD_SET(fd, &r_fds);
            FD_SET(fd, &w_fds);

            select(fd + 1, &r_fds, &w_fds, NULL, NULL);

            if (FD_ISSET(fd, &r_fds))
                printf("Poll monitor:can be read\n");

            if (FD_ISSET(fd, &w_fds))
                printf("Poll monitor:can be written\n");
        }
    } else {
        printf("Device open failure\n");
    }
}
