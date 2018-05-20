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
	struct termios opt;
	
	buff[0]=0x1;
	
	printf("hello world");
	
	if(fd=open("/dev/ttySAC1", O_RDWR|O_NOCTTY|O_NDELAY)<0)		//打开串口1
	{
		printf("open error");
		return 1;
	}

/*
	if(tcgetattr(fd, &opt)!=0)			//得到与fd指向对象的相关参数，并将它们保存在opt
	{
		printf("tcgetattr error");
		return 1;
	}
	
	cfsetispeed(&opt, B9600);			//设置输入波特率
	cfsetospeed(&opt, B9600);			//设置输出波特率

	printf("hello zzx1");
	
	opt.c_cflag &= ~CSIZE;  			//屏蔽其他标志位
    opt.c_cflag |= CS8;   				//设置八位数据位
    opt.c_cflag &= ~CSTOPB; 			//1位停止位
    opt.c_cflag &= ~PARENB; 			//无奇偶校验位
    opt.c_cflag &= ~INPCK;				//无奇偶校验位
    opt.c_cflag |= (CLOCAL | CREAD);		//修改控制模式，使得程序不会占用串口，并且能够从串口读取输入数据
 
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);		//如果知识串口传输数据，而不是开发终端之类的额，使用原始模式来通讯（这两行）
	opt.c_oflag &= ~OPOST;				//修改输出模式，原始数据输出
	
//    opt.c_oflag &= ~(ONLCR | OCRNL);    //添加的
 
    opt.c_iflag &= ~(ICRNL | INLCR);
    opt.c_iflag &= ~(IXON | IXOFF | IXANY);    //使用软件流控制

	opt.c_cc[VTIME] = 0;				//读取一个字符等待0*（0.1）sboo
    opt.c_cc[VMIN] = 0;					//读取字符的最少个数为0
	
	tcflush(fd, TCIOFLUSH);				//如果发生数据溢出，接受数据，但是不再读取
	
	printf("hello zzx2");
	
	if(tcsetattr(fd,TCSANOW,&opt)!= 0)		//激活配置
    {
        printf("tcsetattr error");
        return 1;
    }					               
	
	printf("hello zzx3");     */
	
	write(fd,buff,1);  

	printf("hello zzx4");
	
	close(fd);
	
	return 0;
} 
