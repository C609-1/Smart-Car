#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

int main(int argc,char **argv)
{
    int fd;
    int i;
    int tmp = 0x08;
    int result;
    
    fd = open("/dev/esp8266", O_RDWR);
    if (fd < 0)
        printf("can't open /dev/esp8266 !\n");
    
    result = write(fd, &tmp, 1);
    if (result < 0)
        printf("write esp8266 failed!\n");
    
    return 0;
} 
