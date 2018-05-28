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

    //read(fd, buff, 1);
    /*循环查询*/
    while (1)
    {
        //  int poll(struct pollfd *fds, nfds_t nfds, int timeout); 
        /*  
            fds：struct pollfd 数组，其中管理了文件描述以及输入参数
            ndfs: 需要通过 poll 机制管理的文件描述符数量
            timeout：超时时间，单位为毫秒
        */
        ret = poll(fds, 1, 5000);
        if (ret)
        {
            read(fd, buff, 11);
            for (i = 0; i < 11; i++)
                printf("%0x", buff[i]);
        }
    }
    printf("esp test end\n");

    close(fd);

    return 0;
} 
