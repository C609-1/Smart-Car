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
#include <linux/delay.h>
#include <linux/irq.h>

#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
//#include <asm/arch/regs-gpio.h>
//#include <asm/hardware.h>
#include <linux/unistd.h>

static struct class *esp8266_class;
static struct class_device *esp8266_class_dev; 

volatile unsigned long *gpbcon = NULL;
volatile unsigned long *gpbdat = NULL;

static void esp8266_GPIO_CFG(void)
{
    /*配置GPIO引脚*/
    /* 配置GPB7为串口输入引脚，GPB8为串口输出引脚*/
    *gpbcon &= ~(0x3<<(7*2));
    *gpbcon |= (0x1<<(8*2));
}

static int esp8266_open(struct inode *inode, struct file *file)
{
    esp8266_GPIO_CFG();
    return 0;
}

static ssize_t esp8266_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
    return 0;
}

static ssize_t esp8266_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
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

    /*映射引脚*/
    gpbcon = (volatile unsigned long *)ioremap(0x56000010, 12);
    gpbdat = gpbcon + 1;
    
    return 0;
}
static void esp8266_exit(void)
{
    unregister_chrdev(major, "esp8266");
    device_destroy(esp8266_class, MKDEV(major, 0));
    class_destroy(esp8266_class);
    
    iounmap(gpbcon);
}

module_init(esp8266_init);
module_exit(esp8266_exit);

MODULE_LICENSE("GPL");
