#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
//#include <termbits.h>

//#include "uart_api.h"
int main(int argc,char **argv)
{
    int fd;
    char buff[1];
    buff[0]=0x1;

    printf("hello world\n");

    if(fd=open("/dev/esp8266", O_RDWR)<0)		//打开串口1
    {
        printf("open error");
        return 1;
    }

    read(fd, buff, 1);
    printf("%0x\n", buff[0]);
    printf("hello zzx4\n");

    close(fd);

    return 0;
} 
