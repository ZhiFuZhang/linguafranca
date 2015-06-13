#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include "intern.h"

static DEFINE_SEMAPHORE(ips_sem);
static enum {
	CLOSE = 0,
	OPEN,
	NAME_SET,
	RUN
} dev_state = CLOSE;
static int cas(int expected, int new)
{
	int err = 0;
	down_interruptible(&ips_sem);
	if (dev_state){
		dev_state = OPEN;
	}
	else {
		err = -EINVAL;
	}
	up(&ips_sem);
	return err;
}

static void set(int new) 
{
	down_interruptible(&ips_sem);
	dev_state = new;
	up(&ips_sem);
}

static int ips_open(struct inode *node, struct file *filep)
{
	int err = 0;
	err = cas(CLOSE, OPEN);
	if (err) return err;
	err = devset_init();
	if (err) goto fail;
	err = ip_queue_init();
	if (err) goto ip_queue_fail;
	hook_register();
	return 0;
ip_queue_fail:
	devset_exit();
fail:
	return err;
}
static int ips_release(struct inode *node, struct file *filep)
{
	dev_state = CLOSE;
	hook_unregister();
	(void)ip_queue_exit();
	(void)devset_exit();
	return 0;
}
static long ips_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	union {
		struct devname_list l;
		struct ip_key_info_set s;
	} data;
	struct devname *d = NULL;
	int cmdnum = 0;
	int err = 0;
	cmdnum = _IOC_NR(cmd);
	pr_debug(IPS"cmd [%d], [%d]\n", cmdnum, cmd);

	if (unlikely(_IOC_TYPE(cmd) != IPS_CMD_MAGIC)) {
		pr_err("magic wrong \n");
		return -EIO;
	}
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err){
		pr_err("access error:NFS_CMD_NUM:[%d]\n", cmdnum);
 		return -EFAULT;
	}
	switch (cmd) {
	case IPS_SET_NAMELIST:
		err = cas(OPEN, NAME_SET);
		if (err) return err;
		copy_from_user(&data.l, arg, sizeof(struct devname_list));
		d = data.l.namelist;
		err = !access_ok(VERIFY_READ, (void __user *)d,
				data.l.devnum * sizeof( struct devname));
		break;
	case IPS_FETCH_INFO:
		break;
	default:
		pr_err("handle error:IPS_CMD_NUM:[%d]\n", cmdnum);
		return -EFAULT;
	}
	return 0;
}

static int proc_show(struct seq_file *m, void *v)
{
	return devset_show(m);
}

static int proc_single_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_show, PDE_DATA(inode));
}

static const struct file_operations proc_ops = {
	.owner          = THIS_MODULE,
	.open           = proc_single_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,

};
static struct file_operations ips_ops = {
	        .owner = THIS_MODULE,
		.open           = ips_open,
	        .unlocked_ioctl = ips_ioctl,
		.release	= ips_release,
};
static struct cdev dev;
static dev_t devno;
static void ips_clean(void)
{

	remove_proc_entry(IPS_DEV_PROC, NULL);
	cdev_del(&dev);
	unregister_chrdev_region(devno, 1);
}

static  void __exit  ips_exit(void)
{

	pr_info(IPS"ips_exit\n");
	ips_clean();
	return;
}
static int __init ips_init(void)
{
	int err = 0;
	pr_info(IPS"ips_init\n");
#ifdef DEBUG
	pr_info(IPS"debug mode is open\n");
#endif
	err = alloc_chrdev_region(&devno, 0, 1, IPS_DEV_NAME);
	if (unlikely(err < 0)){
		pr_err(IPS"failed to allocate device region, %d\n", err);
		return err;
	}
	cdev_init(&dev, &ips_ops);
	dev.owner = THIS_MODULE;
	dev.ops = &ips_ops;
	err = cdev_add(&dev, devno, 1);
	if (unlikely(err)){
		unregister_chrdev_region(devno, 1);
		pr_err(IPS"cdev_add failed, %d\n", err);
		return err;
	}
	if (unlikely(!proc_create_data(IPS_DEV_PROC,
					0444, NULL, &proc_ops, NULL))) {
		pr_err(IPS"could not create proc %s\n", IPS_DEV_PROC);
		goto fail;
	}

	return 0;
fail:
	ips_clean();
	return err;
}


/* following code is for unit test */
#ifdef  UNITTEST
static  void __exit  ips_ut_exit(void)
{
	pr_info(IPS"ips_ut_exit\n");
}
static int __init ips_ut_init(void)
{
	
	pr_info(IPS"ips_ut_init\n");
	runtestsuite(devset_ut);
	runtestsuite(ip_queue_ut);
	return 0;
}

module_init(ips_ut_init);
module_exit(ips_ut_exit);

#else

module_init(ips_init);
module_exit(ips_exit);

#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhi Fu Zhang <zzfooo@hotmail.com>");
MODULE_DESCRIPTION("ipshark");
MODULE_VERSION("0.1");


