/*
该程序实现wifi的通信，该模块工作在AP模式下，即本身发出WiFi信号，其他设备来连接该信号
使用模拟串口
*/

#include <linux/platform_device.h> 
#include <linux/spi/spi.h> 
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/slab.h> 
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/file.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
//#include <asm/arch/regs-gpio.h>
//#include <asm/hardware.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/syscalls.h>
#include <linux/termios.h>
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/time.h>


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

static struct class *esp8266_class;
static struct class_device *esp8266_class_dev;

volatile int have_data = 0;             //是否有数据
volatile int esp8266_mode = 0;          //esp8266的工作模式

/*串口读数据返回格式*/
static struct uart_read_opt {
    unsigned char *data;
    int length;
};

/*设置esp8266参数格式*/
static struct esp8266_settings {
    unsigned int portnum;                   //端口号
    unsigned int IP[4];                     //IP地址的4位
    unsigned char wifiap_ssid[20];          //对外SSID号
    unsigned char wifiap_encryption[15];        //wpa/wpa2 aes加密方式
    unsigned char wifiap_password[20];      //连接密码
};

static int esp8266_setup(void __user *arg);
static int esp8266_ap_udp(void);
static int esp8266_ap_client(void);
static int esp8266_ap_server(void __user *arg);
static int esp8266_sta_udp(void);
static int esp8266_sta_client(void);
static int esp8266_sta_server(void);
static int esp8266_apsta_udp(void);
static int esp8266_apsta_client(void);
static int esp8266_apsta_server(void);



/*串口写函数*/
static int uart_write(unsigned char data)
{
    struct file *filep;
    mm_segment_t old_fs;
    
    filep = filp_open("/dev/ttySAC1", O_RDWR | O_NDELAY, 0777);
    if (IS_ERR(filep))
    {
        printk("In uart_write +filp_open failed!\n");
        return 1;
    }
    old_fs = get_fs();
    set_fs(get_ds());
    vfs_write(filep, &data, 1, &filep->f_pos);
    printk("data is: %0x\n", data);
    set_fs(old_fs);
    filp_close(filep, NULL);
    
    return 0;
}

/*串口读函数
count:读多少个字节
mode: 0: 供内部调用
      1：供esp8266_read函数调用

函数内有select、poll机制，故可以使用轮询来调用此函数

每次只能读8个字节，故发送字节数多于8个字节时，分多次读取
故：每次发送要一定时间间隔
*/
static struct uart_read_opt uart_read(unsigned int count, unsigned int mode)
{
    struct file *filep;
    unsigned char *data;
    unsigned char *tmp;     //存放分配的内存
    mm_segment_t old_fs;
    long timeout = 1000;
    int length = 0;
    struct uart_read_opt res;
    int i = 0;
    int j = 0;
    int times = 0;          //判断读几次
    int readFlag = 0;
    
    old_fs = get_fs();
    set_fs(get_ds());
    
    filep = filp_open("/dev/ttySAC1", O_RDWR, 0777);
    
    
    if (IS_ERR(filep))
    {
        printk("In uart_read filp_open failed!\n");
        res.data = NULL;
        res.length = -1;
        return res;
    }
    
    if (filep->f_op->poll)
    {
        struct poll_wqueues table;
        struct timeval start, now;
        
        do_gettimeofday(&start);
        poll_initwait(&table);
        while (1)
        {
            long elapsed;
            int mask; 

            mask = filep->f_op->poll(filep, &table.pt); 
            if (mask & (POLLRDNORM | POLLRDBAND | POLLIN | POLLHUP | POLLERR)) 
            { 
                readFlag = 1;
                if (mode == 1)
                {
                    have_data = 1;
                }
                break; 
            } 
            do_gettimeofday(&now); 
            elapsed = (1000000 * (now.tv_sec - start.tv_sec) + now.tv_usec - start.tv_usec); 
            if (elapsed > timeout) 
            { 
                break; 
            } 
            set_current_state(TASK_INTERRUPTIBLE); 
            schedule_timeout(((timeout - elapsed) * HZ) / 10000); 
        }
        poll_freewait(&table);
    }

    filep->f_pos = 0;
    data = (unsigned char *)kmalloc(count, GFP_KERNEL);
    tmp = data;
    printk("count is: %d\n", count);
    
    /*判断读几次*/
    if (count%8 > 0)
        times = 1+count/8;
    else
        times = 1;
    if (readFlag==1 || have_data==1)
    {
        for (j = 0; j < times; j++)
        {
            if ((length = filep->f_op->read(filep, data, 8, &filep->f_pos)) > 0) 
            {
                printk("uart_read has recive length is: %d\n", length);
                for (i = 0; i < length; i++)
                {
                    printk(" ");
                    printk("%0x", data[i]);
                }
                printk("\n");
                data = data + length;
            }
            else 
            {
                printk("debug: failed\n");
                break;
            }
        }
    }
    set_fs(old_fs);
    filp_close(filep, NULL);
    
    if (length > 0)
        res.data = tmp;
    else
    {
        kfree(data);
        res.data = NULL;
    }
    res.length = length;
    
    return res;
}


/*
esp8266发送命令后，检测接收到的应答
str:期待的应答结果
返回值：0，得到期待的应答结果
        1,没有得到期待的应答结果
        
返回指令格式：发送指令字节数+5字节+2字节(OK)+2字节
*/
static unsigned char esp8266_check_cmd(unsigned char *str, unsigned int length)
{
    struct uart_read_opt recv;
    unsigned char *data;
    unsigned int size;
    int i = 0;

    /*尝试读数据，只读最后8个字节的数据，判断其中是否包含"OK"两个字节*/
    recv = uart_read(8, 0);
    data = recv.data;
    size = recv.length;
    
    if (data == NULL)
    {
        if (length < 0)
            return 2;           //read打开设备失败
        else
            return 1;
    }
    
    /*判断str是否在返回字符中*/
    for (i = 0; i < size; i++)
    {
        //printk("%0x\n", data[i]);
        if (data[i] == *str)
        {
            i++;
            str++;
            if (data[i] == *str)
            {
                printk("esp8266_check_cmd success!\n");
                return 0;
            }
            else
            {
                i--;
                str--;
            }
        }
    }
    
    /*释放分配的内存*/
    kfree(data);
    
    return 1;
}

/*
向esp8266发送命令
cmd:发送的命令字符串
ack:期待的应答结果，如果为空，则表示不需要等待应答(非字符串，只是字符数组)
waittime:等待时间
length:发送指令的字符数组的长度
返回值：0，发送成功（得到了期待的应答结果）
        1，发送失败
*/
static unsigned char esp8266_send_cmd(unsigned char *cmd, unsigned char *ack, unsigned int waittime, unsigned int length)
{
    unsigned char res = 1;
    
    /*发送命令*/
    while (*cmd != '\0')
    {
        res = uart_write(*cmd);
        if (res)
            return 1;
        cmd++;
    }
    /*发送结束符*/
    if (uart_write('\r'))
        return 1;
    if (uart_write('\n'))
        return 1;
    
    /*需要等待应答*/
    if (*ack!=' ' && waittime>0)
    {
        while (waittime--)
        {
            res = esp8266_check_cmd(ack, length);       //等待响应指令
            if (res==0 || res==2)       //返回成功或者打开设备失败，则退出循环
            {
                if (res == 2)
                    printk("ttySAC1 can't open!!!\n");
                break;
            }
        }
    }
    
    return res;
}

/*
向esp8266发送指定数据
data:发送的数据(不需要添加回车，注：格式为字符串，非字符数组)
ack:期待的应答结果，如果为空，则表示不需要等待应答
waittime:等待时间
返回值：0，发送成功（得到了期待的应答结果）
        1，发送失败
*/
static unsigned char esp8266_send_data(unsigned char *data, unsigned char *ack, unsigned int waittime)
{
    unsigned char res = 0;
    
    while (*data != '\0')
    {
        if (uart_write(*data))
            return 1;
        data++;
    }
    
    /*需要等待应答*/
    if (*ack!=' ' && waittime>0)
    {
        while (waittime--)
        {
            res = esp8266_check_cmd(ack, 0);       //等待响应指令
            if (res==0 || res==2)
            {
                break;
            }
        }
    }
    
    return res;
}

/*
esp_8266退出透传模式
返回值:0,退出成功
       1,退出失败
*/
static unsigned char esp8266_quit_trans(void)
{
    unsigned char cmd[] = "+++AT";
    unsigned char ack[2];
    unsigned char res = 0;
    
    ack[0] = 'O';
    ack[1] = 'K';
    res = esp8266_send_cmd(cmd, ack, 3, sizeof(cmd));
    
    return res;
}

/*
获取esp8266模块的连接状态
返回值：0,连接成功；1,连接失败
*/
static unsigned char esp8266_consta_check(void)
{
    if (esp8266_quit_trans())
        return 1;
    /*发送AT+CIPSTAUS指令*/
    if (!esp8266_send_cmd("AT+CIPSTATUS", "OK", 3, 13))
        return 0;
    else
        return 1;
}

static int esp8266_open(struct inode *inode, struct file *file)
{
    struct file *f;
    struct termios settings;
    mm_segment_t oldfs;
    
    oldfs = get_fs(); 
    set_fs(KERNEL_DS);  
    // Set speed 
     
    f = filp_open("/dev/ttySAC1", O_RDWR | O_NONBLOCK , 0777);
    f->f_op->unlocked_ioctl(f, TCGETS, (unsigned long)&settings); 

    settings.c_cflag &= ~CBAUD; 
    settings.c_cflag |= B115200; 

    settings.c_cflag |= (CLOCAL | CREAD); 

    settings.c_cflag &= ~PARENB; 
    settings.c_cflag &= ~PARODD; 
    settings.c_cflag &= ~CSTOPB; 
    settings.c_cflag &= ~CSIZE; 
    settings.c_cflag |= CS8;
    

    //settings.c_iflag = 1; 
    settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP    
                | INLCR | IGNCR | ICRNL | IXON);
    settings.c_oflag = 0; 
    settings.c_lflag = 0; 
    //settings.c_cc[VTIME] = 1;
    //settings.c_cc[VMIN] = 0;

    f->f_op->unlocked_ioctl(f, TCSETS, (unsigned long)&settings);
    set_fs(oldfs);
    filp_close(f, NULL);
    printk("open esp8266!\n");
    return 0;
}

/*提供写函数，只是传递数据的*/
static ssize_t esp8266_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    unsigned char ack[2];
    unsigned int waittime;
    unsigned char cmd[20];
    unsigned char *data = (unsigned char *)kmalloc(count, GFP_KERNEL);
    int ret = 0;
    if (data == NULL)
        printk("kmalloc failed!\n");
    ack[0] = 'O';
    ack[1] = 'K';
    waittime = 20;
    
    ret = copy_from_user(data, buf, count);       //拷贝数据
    if (ret < 0)
    {
        printk("copy_from_user failed!\n");
        return 0;
    }
    
    /*判断是哪个模式后，做前期发送前的准备工作*/
    switch (esp8266_mode)
    {
        case TCPCLINET_WIFIAP:
        {
            break;
        }
        case TCPCLINET_WIFISTA:
        {
            break;
        }
        case TCPCLINET_APSTA:
        {
            break;
        }
        case TCPSERVER_WIFIAP:
        {
            /*发送指定长度的数据*/
            sprintf((char *)cmd, "AT+CIPSEND=0,%d", count);
            printk("CIPSEND begin!!!!\n");
            if (esp8266_send_cmd(cmd, ack, 3, 20))
            {
                printk("CIPSEND failed!!!!!\n");
                printk("CIPSEND end!!!!\n");
                return 0;
            }
            break;
        }
        case TCPSERVER_WIFISTA:
        {
            break;
        }
        case TCPSERVER_APSTA:
        {
            break;
        }
        case UDP_WIFIAP:
        {
            break;
        }
        case UDP_WIFISTA:
        {
            break;
        }
        case UDP_APSTA:
        {
            break;
        }
        default:
        {
            printk("esp8266 mode is not legal!\n");
            return 0;
        }
    }
    
    /*发送数据*/
    printk("send data is: %s\n", data);
    if (esp8266_send_data(data, ack, waittime))
        printk("esp8266 send data failed!!!!\n");
    else
        printk("esp8266 send data success!!!\n");
    if (data != NULL)
        kfree(data);
    
    return 0;
}

/*提供读函数，只是传递数据的*/
static ssize_t esp8266_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct uart_read_opt ret;
    int res = 0;
    /*使用while循环轮询uart_read函数，因为此函数有select、poll机制*/
    while (!have_data)
    {
        ret = uart_read(count, 1);
    }
    have_data = 0;
    /*读数据到用户空间*/
    if (ret.length > 0)
    {
        res = copy_to_user(buf, ret.data, count);
        if (res < 0)
            printk("copy_to_user failed!!!\n");
        kfree(ret.data);
    }
    
    return 0;
}

/*选取模式，并完成模式之前的初始化工作*/
static int esp8266_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    
    switch (cmd)
    {
        case TCPCLINET_WIFIAP:
            ret = esp8266_ap_client();
            break;
        case TCPCLINET_WIFISTA:
            ret = esp8266_sta_client();
            break;
        case TCPCLINET_APSTA:
            ret = esp8266_apsta_client();
            break;
        case TCPSERVER_WIFIAP:
            ret = esp8266_ap_server((void __user *)arg);
            break;
        case TCPSERVER_WIFISTA:
            ret = esp8266_sta_server();
            break;
        case TCPSERVER_APSTA:
            ret = esp8266_apsta_server();
            break;
        case UDP_WIFIAP:
            ret = esp8266_ap_udp();
            break;
        case UDP_WIFISTA:
            ret = esp8266_sta_udp();
            break;
        case UDP_APSTA:
            ret = esp8266_apsta_udp();
            break;
        case ESP8266_SETUP:
            ret = esp8266_setup((void __user *)arg);
            break;
        default:
            printk("esp8266_ioctl failed!\n");
            break;
    }
    return 0;
}

/*esp8266初始化*/
static int esp8266_setup(void __user *arg)
{
    unsigned char ack[2];
    unsigned char p[50];
    struct esp8266_settings info;
    
    ack[0] = 'O';
    ack[1] = 'K';
    if (copy_from_user(&info, arg, sizeof(struct esp8266_settings)))
    {
        printk("copy_from_user failed!\n");
        return 0;
    }
    printk("esp8266_setup!!!!!\n");
    printk("wifiap_ssid: %s\n", info.wifiap_ssid);
    printk("wifiap_password: %s\n", info.wifiap_password);
    /*0、检查esp8266模块是否存在*/
    if (esp8266_send_cmd("AT", ack, 3, 2))
    {
        printk("esp8266 not exist!\n");
        return 0;
    }
    /*1、关闭esp8266的回显*/
    if (esp8266_send_cmd("ATE0", ack, 3, 4))
    {
        printk("esp8266 close show return failed!\n");
        return 0;
    }
    
    /*2、完成esp8266的初始化*/
    if (esp8266_send_cmd("AT+CWMODE=2", ack, 3, 11))
    {
        printk("AT+CWMODE=2 failed!\n");
        return 0;
    }
    if (esp8266_send_cmd("AT+RST", ack, 3, 6))
    {
        printk("AT+RST!\n");
        return 0;
    }
    msleep(2000);       //延时2s等待模块重启
    sprintf((char *)p, "AT+CWSAP=\"%s\",\"%s\",1,4", info.wifiap_ssid, info.wifiap_password);       //配置模块AP模式无线参数
    if (esp8266_send_cmd(p, ack, 1000, 50))
    {
        printk("set esp8266 failed!\n");
        return 0;
    }
    printk("esp8266_setup finished!!!!!\n");
    return 0;
}

/*AP模式UDP*/
static int esp8266_ap_udp(void)
{
    return 0;
}

/*AP模式Client*/
static int esp8266_ap_client(void)
{
    return 0;
}

/*AP模式SEVER*/
static int esp8266_ap_server(void __user *arg)
{
    unsigned char ack[2];
    unsigned char portnum[20];              //端口
    struct esp8266_settings info;
    
    ack[0] = 'O';
    ack[1] = 'K';
    if (copy_from_user(&info, arg, sizeof(struct esp8266_settings)))
    {
        printk("copy_from_user failed!\n");
        return 0;
    }
    sprintf((char *)portnum, "AT+CIPSERVER=1,%d", info.portnum);
    esp8266_mode = TCPSERVER_WIFIAP;
    if (esp8266_send_cmd("AT+CIPMUX=1", ack, 3, 11))           //0:单连接，1：多连接
    {
        printk("AT+CIPMUX=1 failed!!!!\n");
        return 0;
    }
    if (esp8266_send_cmd(portnum, ack, 3, 19))                 //开启Sever模式，端口号为8086
    {
        printk("AT+CIPSERVER=1 failed!!!!\n");
        return 0;
    }
    
    return 0;
}

/*STA模式UDP*/
static int esp8266_sta_udp(void)
{
    return 0;
}

/*STA模式Client*/
static int esp8266_sta_client(void)
{
    return 0;
}

/*STA模式SERVER*/
static int esp8266_sta_server(void)
{
    return 0;
}

/*APSTA模式UDP*/
static int esp8266_apsta_udp(void)
{
    return 0;
}

/*APSTA模式Client*/
static int esp8266_apsta_client(void)
{
    return 0;
}

/*APSTA模式SERVER*/
static int esp8266_apsta_server(void)
{
    return 0;
}

static struct file_operations  esp8266_fops = {
    .owner           =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open            =   esp8266_open,
    .write           =   esp8266_write,
    .read            =   esp8266_read,
    .unlocked_ioctl  =   esp8266_ioctl,
};

int  major;

static int esp8266_init(void)
{
    major = register_chrdev(0, "esp8266", &esp8266_fops);    // 注册, 告诉内核
    esp8266_class = class_create(THIS_MODULE, "esp8266");
    esp8266_class_dev = (struct class_device*)device_create(esp8266_class, NULL, MKDEV(major, 0), NULL, "esp8266"); /* 只要加载后就会自动创建设备节点 /dev/esp8266 */
    
    
    return 0;
}
static void esp8266_exit(void)
{
    unregister_chrdev(major, "esp8266");
    device_destroy(esp8266_class, MKDEV(major, 0));
    class_destroy(esp8266_class);
    
}

module_init(esp8266_init);
module_exit(esp8266_exit);

MODULE_LICENSE("GPL");
