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
	
	if(fd=open("/dev/ttySAC1", O_RDWR|O_NOCTTY|O_NDELAY)<0)		//�򿪴���1
	{
		printf("open error");
		return 1;
	}

/*
	if(tcgetattr(fd, &opt)!=0)			//�õ���fdָ��������ز������������Ǳ�����opt
	{
		printf("tcgetattr error");
		return 1;
	}
	
	cfsetispeed(&opt, B9600);			//�������벨����
	cfsetospeed(&opt, B9600);			//�������������

	printf("hello zzx1");
	
	opt.c_cflag &= ~CSIZE;  			//����������־λ
    opt.c_cflag |= CS8;   				//���ð�λ����λ
    opt.c_cflag &= ~CSTOPB; 			//1λֹͣλ
    opt.c_cflag &= ~PARENB; 			//����żУ��λ
    opt.c_cflag &= ~INPCK;				//����żУ��λ
    opt.c_cflag |= (CLOCAL | CREAD);		//�޸Ŀ���ģʽ��ʹ�ó��򲻻�ռ�ô��ڣ������ܹ��Ӵ��ڶ�ȡ��������
 
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);		//���֪ʶ���ڴ������ݣ������ǿ����ն�֮��Ķʹ��ԭʼģʽ��ͨѶ�������У�
	opt.c_oflag &= ~OPOST;				//�޸����ģʽ��ԭʼ�������
	
//    opt.c_oflag &= ~(ONLCR | OCRNL);    //��ӵ�
 
    opt.c_iflag &= ~(ICRNL | INLCR);
    opt.c_iflag &= ~(IXON | IXOFF | IXANY);    //ʹ�����������

	opt.c_cc[VTIME] = 0;				//��ȡһ���ַ��ȴ�0*��0.1��sboo
    opt.c_cc[VMIN] = 0;					//��ȡ�ַ������ٸ���Ϊ0
	
	tcflush(fd, TCIOFLUSH);				//�����������������������ݣ����ǲ��ٶ�ȡ
	
	printf("hello zzx2");
	
	if(tcsetattr(fd,TCSANOW,&opt)!= 0)		//��������
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
