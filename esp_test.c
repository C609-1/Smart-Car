#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>

int main(int argc,char **argv)
{
    int fd;
    char buff[11];
    int i = 0;
    int ret = 0;

    memset(buff, 0, sizeof(buff));
    printf("esp test beign\n");

    fd=open("/dev/esp8266", O_RDWR);
    if(fd<0)		//打开串口1
    {
        printf("open error");
        return 1;
    }

    while (1)
    {
        ret = read(fd, buff, 11);
        if (ret > 0)
            for (i = 0; i < ret; i++)
                printf("%0x,",buff[i]);
    }
    printf("esp test end\n");

    close(fd);

    return 0;
} 
