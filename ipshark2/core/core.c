#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "devset.h"
#include "dma.h"
#include "hook.h"
#include "ip_queue.h"
#include "proc.h"
static int ips_open(struct inode *node, struct file *filep)
{
	pr_info(IPS"open\n");
	return 0;
}
static int ips_release(struct inode *node, struct file *filep)
{
	pr_info(IPS"release\n");
	return 0;
}

static int ips_config(struct ips_config *c)
{
	struct ips_config config;
	int ret = copy_from_user(&config, c, sizeof(struct ips_config));
	int s = 0;
	struct ips_config *p = NULL;
	int t = sizeof(ips_config) + sizeof(config.devlist[0]) * config.devnum;
	if (ret) goto fail;
	p =  kmalloc(t, GFP_KERNEL);
	if (p == NULL) goto fail;
	ret = copy_from_user(p, c, t);
	if (ret) goto fail2;
	s = ip_queue_create(config.queue_size,
		sizeof(config.queue_size)/sizeof(struct ips_cpu_queue_size),
		config.default_queue_size, config.wait_ms);
	if (s < 0) {
		ret = s;
		goto ip_queue_fail;
	}
	config.dma_size = s;
	ret = devset_init(p);
	if (ret) goto devset_fail;
	ret = copy_to_user(c, &config, sizeof(config));
	if (ret) goto devset_fail;
	hook_register();
	return 0;
devset_fail:
	devset_exit();
ip_queue_fail:
	ip_queue_exit();
fail2:
	kfree(p);
	p = NULL;
fail:
	return ret;
}

static int ips_fetch(void)
{
	int ret = ip_queue_wait();
	if (ret ==  0) {
		if (ip_queue_has_data()) {
			ip_queue_move2dma();
			return ret;
		}
		return -1;
	} else if (ret > 0) {
		ip_queue_move2dma();
		return ret;
	} else {
		return ret;
	}
	
}

static int ips_freemem(struct ips_recycle_array *p)
{
	struct ips_recycle_array a;
	int ret = copy_from_user(&a, p, sizeof(struct ips_recycle_array));
	if (ret) return ret;
	ip_queue_recycle(a.idx_array, a.idx_num);
	return ret;
}

static long ips_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int cmdnum = 0;
	int err = 0;
	cmdnum = _IOC_NR(cmd);
	pr_debug(IPS"cmd [%d], [%d]\n", cmdnum, cmd);
/* cmd check, begin */
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
/* cmd check, end */

	switch (cmd) {
	case  IPS_CONFIG:
		err = ips_config((struct ips_config *)arg);
		break;
	case IPS_FETCH_INFO:
		err = ips_fetch();
		break;
	case IPS_FREE_MEM:
		err = ips_freemem((struct ips_recycle_array *)arg);
	default:
		pr_err("handle error:IPS_CMD_NUM:[%d]\n", cmdnum);
		return -EFAULT;
	}

	pr_debug(IPS"ioctl errno[%d]\n", err);
	return err;
}

static struct file_operations ips_ops = {
	        .owner = THIS_MODULE,
		.open           = ips_open,
	        .unlocked_ioctl = ips_ioctl,
		.mmap		= ips_mmap,
		.release	= ips_release,
};

static struct cdev dev;
static dev_t devno;
static void ips_clean(void)
{

	ips_rmv_proc();
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
	err = ips_create_proc();
	if (err){
		goto fail;
	}
	return 0;
fail:
	ips_clean();
	return err;
}


module_init(ips_init);
module_exit(ips_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhi Fu Zhang <zzfooo@hotmail.com>");
MODULE_DESCRIPTION("ipshark2");
MODULE_VERSION("0.1");


