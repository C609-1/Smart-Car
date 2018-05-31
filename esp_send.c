#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <linux/ioctl.h>

#define     TCPCLINET_WIFIAP        _IOWR('X', 0, struct esp8266_settings*)
#define     TCPCLINET_WIFISTA       _IOWR('X', 1, struct esp8266_settings*)
#define     TCPCLINET_APSTA         _IOWR('X', 2, struct esp8266_settings*)

#define     TCPSERVER_WIFIAP        _IOWR('X', 3, struct esp8266_settings*)
#define     TCPSERVER_WIFISTA       _IOWR('X', 4, struct esp8266_settings*)
#define     TCPSERVER_APSTA         _IOWR('X', 5, struct esp8266_settings*)

#define     UDP_WIFIAP              _IOWR('X', 6, struct esp8266_settings*)
#define     UDP_WIFISTA             _IOWR('X', 7, struct esp8266_settings*)
#define     UDP_APSTA               _IOWR('X', 8, struct esp8266_settings*)

#define     ESP8266_SETUP           _IOWR('X', 9, struct esp8266_settings*)

/*设置esp8266参数格式*/
struct esp8266_settings {
    unsigned int portnum;                   //端口号
    unsigned int IP[4];                     //IP地址的4位
    unsigned char wifiap_ssid[20];          //对外SSID号
    unsigned char wifiap_encryption[15];        //wpa/wpa2 aes加密方式
    unsigned char wifiap_password[20];      //连接密码
};

int main(int argc,char **argv)
{
    int fd;
    char buff[] = "hello esp8266";
    char recbuff[12];
    int i = 0;
    int ret = 0;
    int flag = 0;
    struct esp8266_settings settings;
    unsigned char wifiap_ssid[] = "esp8266_test";
    unsigned char wifiap_encryption[] = "wpawpa2_aes";
    unsigned char wifiap_password[] = "12345678";

    
    memset(&settings, 0, sizeof(struct esp8266_settings));
    printf("esp test beign\n");
    settings.portnum = 8086;
    strncpy((char *)settings.wifiap_ssid, (char *)wifiap_ssid, 20);
    strncpy((char *)settings.wifiap_encryption, (char *)wifiap_encryption, 15);
    strncpy((char *)settings.wifiap_password, (char *)wifiap_password, 20);
    fd=open("/dev/esp8266", O_RDWR);
    if(fd<0)
    {
        printf("open error");
        return 1;
    }

    /*测试esp8266发送数据*/
    /*1、初始化esp8266*/
    ret = ioctl(fd, ESP8266_SETUP, (struct esp8266_settings *)&settings);
    if (ret != 0)
    {
        printf("ioctl failed!\n");
        return 1;
    }
    ret = ioctl(fd, TCPSERVER_WIFIAP, (struct esp8266_settings *)&settings);
    if (ret != 0)
    {
        printf("ioctl failed!\n");
        return 1;
    }
    /*2、确保连接成功*/
    while (!flag)
    {
        read(fd, recbuff, sizeof(recbuff));
        /*判断返回的数组是否为空*/
        for (i = 0; i < sizeof(recbuff); i++)
        {
            if (recbuff[i] != 0)
            {
                flag = 1;               //接收到数据
                break;
            }
        }
    }
    for (i = 0; i < sizeof(recbuff); i++)
    {
        printf("%0x", recbuff[i]);
        printf(" ");
    }
    /*3、发送数据*/
    ret = write(fd, buff, sizeof(buff));
    if (ret < 0)
    {
        printf("write failed!\n");
        return 1;
    }
    printf("esp test end\n");

    close(fd);

    return 0;
} 
