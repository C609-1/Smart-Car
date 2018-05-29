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


static struct class *esp8266_class;
static struct class_device *esp8266_class_dev;

static struct timer_list  esp8266_timer;                /*定义一个定时器*/
static DECLARE_WAIT_QUEUE_HEAD(esp8266_waitq);       /*定义一个队列*/
volatile int time_status;            //定时器时间到设置的变量
volatile int have_data = 0;          //是否有数据



static struct uart_read_opt {
    unsigned char *data;
    int length;
};

/*定时器处理函数*/
static void esp8266_timer_function(unsigned long data)
{
    del_timer(&esp8266_timer);
    time_status = 0;
    printk("esp8266_timer_function!\n");
}

/*串口写函数*/
static void uart_write(unsigned char data)
{
    struct file *filep;
    mm_segment_t old_fs;
    
    filep = filp_open("/dev/ttySAC1", O_RDWR | O_NDELAY, 0777);
    if (IS_ERR(filep))
    {
        printk("filp_open failed!\n");
        return;
    }
    old_fs = get_fs();
    set_fs(get_ds());
    vfs_write(filep, &data, 1, &filep->f_pos);
    printk("data is: %0x\n", data);
    set_fs(old_fs);
    filp_close(filep, NULL);
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
    mm_segment_t old_fs;
    long timeout = 1000;
    int length = 0;
    struct uart_read_opt res;
    int i = 0;
    int j = 0;
    int times = 0;          //判断读几次
    
    old_fs = get_fs();
    set_fs(get_ds());
    
    filep = filp_open("/dev/ttySAC1", O_RDWR, 0777);
    
    
    if (IS_ERR(filep))
    {
        printk("filp_open failed!\n");
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
                if (mode == 1)
                    have_data = 1;
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
    printk("count is: %d\n", count);
    
    /*判断读几次*/
    if (count%8 > 0)
        times = 1+count/8;
    else
        times = 1;
    for (j = 0; j < times; j++)
    {
        if ((length = filep->f_op->read(filep, data, 8, &filep->f_pos)) > 0) 
        {
            printk("uart_read has recive length is: %d\n", length);
            for (i = 0; i < length; i++)
                printk("%0x", data[i]);
            printk("\n");
            data = data + length;
        }
        else 
        {
            printk("debug: failed\n");
        }
    }
    set_fs(old_fs);
    filp_close(filep, NULL);
    
    if (length > 0)
        res.data = data;
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

    /*尝试读数据*/
    recv = uart_read(length+9, 0);
    data = recv.data;
    size = recv.length;
    
    if (data == NULL)
        return 1;
    
    /*判断str是否在返回字符中*/
    for (i = 0; i < size; i++)
    {
        if (data[i] == *str)
        {
            i++;
            str++;
            if (data[i] == *str)
                return 0;
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
        uart_write(*cmd);
        cmd++;
    }
    /*发送结束符*/
    uart_write('\r');
    uart_write('\n');
    
    /*需要等待应答*/
    if (*ack!=' ' && waittime>0)
    {
        /*等待倒计时*/
        esp8266_timer.expires =waittime*10*(jiffies+HZ/1000);            /*定时waittime*10ms*/
        esp8266_timer.data=50;
        time_status = 1;
        add_timer(&esp8266_timer);
        printk("time_status is: %d\n", time_status);
        while (time_status)
        {
            res = esp8266_check_cmd(ack, length);       //等待响应指令
            if (res == 0)
            {
                del_timer(&esp8266_timer);
                time_status = 0;
                break;
            }
            printk("time_status is: %d\n", time_status);
        }
    }
    
    return res;
}

/*
向esp8266发送指定数据
data:发送的数据
ack:期待的应答结果，如果为空，则表示不需要等待应答
waittime:等待时间
返回值：0，发送成功（得到了期待的应答结果）
        1，发送失败
*/
static unsigned char esp8266_send_data(unsigned char *data, unsigned char *ack, unsigned int waittime)
{
    
}

/*
esp_8266退出透传模式
返回值:0,退出成功
       1,退出失败
*/
static unsigned char esp8266_quit_trans(void)
{
    
}

/*
获取esp8266模块的连接状态
返回值：0,未连接；1,连接成功
*/
static unsigned char esp8266_consta_check(void)
{
    
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

static ssize_t esp8266_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
    /*发送指令*/
    unsigned char *cmd = "AT+CIPMODE=0";
    unsigned char ack[2];
    unsigned int waittime;
    unsigned int length = 0;
    
    ack[0] = 'O';
    ack[1] = 'K';
    waittime = 20;
    length = sizeof(cmd)-1;                 //字符数组的长度
    if (esp8266_send_cmd(cmd, ack, waittime, length))
        printk("esp8266 send cmd failed!!!!\n");
    else
        printk("esp8266 send cmd success!!!\n");
    return 0;
}

static ssize_t esp8266_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct uart_read_opt ret;
    
    /*使用while循环轮询uart_read函数，因为此函数有select、poll机制*/
    while (!have_data)
    {
        ret = uart_read(count, 1);
    }
    have_data = 0;
    /*读数据到用户空间*/
    copy_to_user(buf, ret.data, count);
    kfree(ret.data);
    
    return 0;
}

static int esp8266_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

/*判断esp8266是否可以读*/
static ssize_t esp8266_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    poll_wait(filp, &esp8266_waitq, wait); // 不会立即休眠

    if(have_data)
        mask |= POLLIN | POLLRDNORM;
    return mask;
    return 0;
}

static struct file_operations  esp8266_fops = {
    .owner           =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open            =   esp8266_open,
    .write           =   esp8266_write,
    .read            =   esp8266_read,
    .unlocked_ioctl  =   esp8266_ioctl,
    .poll            =   esp8266_poll,
};

int  major;

static int esp8266_init(void)
{
    init_timer(&esp8266_timer);              /*初始化一个定时器*/
    esp8266_timer.function = esp8266_timer_function;
    
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
