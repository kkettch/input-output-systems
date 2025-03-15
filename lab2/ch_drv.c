#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/ctype.h>

static dev_t first;
static struct cdev c_dev; 
static struct class *cl;

static int total_count = 0;  
static int local_counts[100];
static int write_index = 0; 

static int my_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: open()\n");
  return 0;
}

static int my_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: close()\n");
  return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    char result[256]; 
    int result_len = 0;
    int i;

    result_len = snprintf(result, sizeof(result), "Total written characters: %d\n", total_count);
    
    for (i = 0; i < write_index; i++) {
        result_len += snprintf(result + result_len, sizeof(result) - result_len, "Write %d: %d characters\n", i + 1, local_counts[i]);
    }

    // если ничего не осталось для чтения то возвращаем 0
    if (*off >= result_len)
        return 0;

    // если длина больше чем осталось для чтения, то уменьшаем длину
    if (len > result_len - *off)
        len = result_len - *off;

    // копирование из ядра в пользовательской пространство
    if (copy_to_user(buf, result + *off, len))
        return -EFAULT;

    *off += len;
    printk(KERN_INFO "Driver: read()\n");

    // возвращаем количество реально скопированных байт
    return len;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    char *kbuf;
    int i = 0;
    int local_count = 0;

    kbuf = kmalloc(len + 1, GFP_KERNEL); // выделение памяти в ядре для буфера
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, buf, len)) { // получение данных из пользовательского буфера в ядро
        kfree(kbuf);
        return -EFAULT;
    }

    kbuf[len] = '\0'; // строка закончилась

    for (i = 0; i < len; i++) {
        if (isalpha(kbuf[i]))
            local_count++;
    }

    total_count += local_count;
    
    if (write_index < 100) {
        local_counts[write_index++] = local_count;
    }

    printk(KERN_INFO "Driver: write()", local_count, total_count);

    kfree(kbuf);
    return len;
}

static struct file_operations mychdev_fops =
{
  .owner = THIS_MODULE,
  .open = my_open,
  .release = my_close,
  .read = my_read,
  .write = my_write
};
 
static int __init ch_drv_init(void)
{
  printk(KERN_INFO "Hello!\n");
  if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0)
  {
		return -1;
  }
  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
  {
		unregister_chrdev_region(first, 1);
		return -1;
  }
  if (device_create(cl, NULL, first, NULL, "var3") == NULL)
  {
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
  }
  cdev_init(&c_dev, &mychdev_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
		device_destroy(cl, first);
		class_destroy(cl);
		unregister_chrdev_region(first, 1);
		return -1;
  }
  return 0;
}
 
static void __exit ch_drv_exit(void)
{
    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "Bye!!!\n");
}
 
module_init(ch_drv_init);
module_exit(ch_drv_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("The first kernel module");

