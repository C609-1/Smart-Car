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


#define DELAYTIME 104       //单位us，波特率为9600时，每位的延时时间

static struct class *esp8266_class;
static struct class_device *esp8266_class_dev; 

volatile unsigned long *gpbcon = NULL;
volatile unsigned long *gpbdat = NULL;

extern long tty_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/*串口写函数*/
static void uart_write(unsigned char data)
{
    struct file *filep;
    int ret;
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

/*串口读函数*/
static unsigned char uart_read(void)
{
    struct file *filep;
    int ret;
    char data;
    mm_segment_t old_fs;
    long timeout = 1000;
    int length = 0;
    struct termios settings;
    
    old_fs = get_fs();
    set_fs(get_ds());
    
    filep = filp_open("/dev/ttySAC1", O_RDWR | O_NDELAY , 0777);
    if (IS_ERR(filep))
    {
        printk("filp_open failed!\n");
        return 1;
    }

    if (filep->f_op->poll)
    {
        struct poll_wqueues table;
        struct timeval start, now;
        
        tty_ioctl(filep, TCGETS, (unsigned long)&settings);
        tty_ioctl(filep, TCIOFLUSH, (unsigned long)&settings);
        do_gettimeofday(&start);
        poll_initwait(&table);
        while (1)
        {
            long elapsed;
            int mask; 

            mask = filep->f_op->poll(filep, &table.pt); 
            if (mask & (POLLRDNORM | POLLRDBAND | POLLIN | POLLHUP | POLLERR)) 
            { 
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
    if ((length = filep->f_op->read(filep, &data, 1, &filep->f_pos)) > 0) 
    {
        printk("data is: %0x\n", data);
    }
    else 
    { 
        printk("debug: failed\n");
    }
    printk("length is: %d\n", length);
    printk("data is: %0x\n", data);
    set_fs(old_fs);
    filp_close(filep, NULL);
    
    return data;
}

static int esp8266_open(struct inode *inode, struct file *file)
{
    struct file *f;
    struct termios settings;
    mm_segment_t oldfs;
    
    oldfs = get_fs(); 
    set_fs(KERNEL_DS);  
    // Set speed 
     
    f = filp_open("/dev/ttySAC1", O_RDWR | O_NDELAY | O_NOCTTY , 0777);
    f->f_op->unlocked_ioctl(f, TCGETS, (unsigned long)&settings); 

    settings.c_cflag &= ~CBAUD; 
    settings.c_cflag |= B9600; 

    settings.c_cflag |= (CLOCAL | CREAD); 

    settings.c_cflag |= PARENB; 
    settings.c_cflag &= ~PARODD; 
    settings.c_cflag &= ~CSTOPB; 
    settings.c_cflag &= ~CSIZE; 
    settings.c_cflag |= CS8;

    //settings.c_iflag = 1; 
    settings.c_iflag = (INPCK | ISTRIP);
    settings.c_oflag = 0; 
    settings.c_lflag = 0; 

    f->f_op->unlocked_ioctl(f, TCSETS, (unsigned long)&settings);
    set_fs(oldfs);
    filp_close(f, NULL);
    printk("open esp8266!\n");
    return 0;
}

static ssize_t esp8266_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
    char tmp = 0x08;
    printk("esp8266 write");
    uart_write(tmp);
    return 0;
}

static ssize_t esp8266_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    unsigned char data;
    int error = 0;
    
    data = uart_read();
    printk("data is: %0x\n", data);
    return 0;
}

static int esp8266_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static struct  file_operations  esp8266_fops = {
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
