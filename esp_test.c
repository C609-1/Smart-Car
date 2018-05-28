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
    char buff[11];
    int i = 0;

    memset(buff, 0, sizeof(buff));
    printf("esp test beign\n");

    fd=open("/dev/esp8266", O_RDWR);
    if(fd<0)		//打开串口1
    {
        printf("open error");
        return 1;
    }

    write(fd, buff, 1);
    
    printf("esp test end\n");

    close(fd);

    return 0;
} 
