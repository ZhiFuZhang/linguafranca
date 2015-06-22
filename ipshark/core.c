#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
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
	down(&ips_sem);
	if (dev_state == expected){
		dev_state = new;
	}
	else {
		err = -EINVAL;
	}
	up(&ips_sem);
	return err;
}

static void set(int new) 
{
	down(&ips_sem);
	dev_state = new;
	up(&ips_sem);
}

static int ips_open(struct inode *node, struct file *filep)
{
	int err = 0;
	pr_info(IPS"open\n");
	err = cas(CLOSE, OPEN);
	if (err) goto fail;
	err = devset_init();
	if (err) goto fail;
	err = ip_queue_init();
	if (err) goto ip_queue_fail;
	hook_register();
	return 0;
ip_queue_fail:
	devset_exit();
fail:
	pr_err(IPS"open failed %d\n", err);
	return err;
}
static int ips_release(struct inode *node, struct file *filep)
{
	dev_state = CLOSE;
	hook_unregister();
	(void)ip_queue_exit();
	(void)devset_exit();
	pr_info(IPS"release\n");
	return 0;
}

static int set_namelist(void *v)
{
	struct devname *d = NULL;
	struct devname_list l;
	int err = cas(OPEN, NAME_SET);
	if (err){
		pr_err(IPS"state is wrong, %d\n", err);
	       	return err;
	}
	err= copy_from_user(&l, v, sizeof(struct devname_list));
	if (err) goto fail;
	d = l.namelist;
	if (unlikely(l.devnum <= 0)){
		err = -EINVAL;
		goto fail;
	}		
	err = !access_ok(VERIFY_READ, (void __user *)d,
				l.devnum * sizeof( struct devname));
	if (err) goto fail;
	l.namelist = vmalloc(l.devnum * sizeof(struct devname));
	if (l.namelist == NULL) {
		err = -EFAULT;	
		goto fail;
	}
	err = copy_from_user(l.namelist, d, l.devnum * sizeof(struct devname));
	if (err) goto failfree;
	err = devset_add(&l);
	if (err) goto failfree;

	vfree(l.namelist);
	return 0;

failfree:
	vfree(l.namelist);
fail:
	set(OPEN);
	return err;
}

static int fetch_info(void * v)
{

	struct ip_key_info_set s;
	
	static  struct ip_key_info arr[1024];
	char *buf = NULL;
	int err = cas(NAME_SET, RUN);
	if (err){
		pr_err(IPS"state is wrong, %d\n", err);
	       	return err;
	}
	err= copy_from_user(&s, v, sizeof(s));
	if (err) goto fail; 
	if (s.buf == NULL || s.n <= 0 || s.n > 1024) {
		err =  -EINVAL;
		goto fail;
	}

	err = ip_queue_wait();
	if (err) {
		pr_debug(IPS"wait timeout %d\n", err);
		err = -ETIME;
		s.n = 0;
		if (copy_to_user(v, &s, sizeof(s))){
			pr_err(IPS"copy to user failed");
		}
		goto fail;
	}
	buf = s.buf;
	err = !access_ok(VERIFY_WRITE, (void __user *)buf,
				s.n * sizeof(struct ip_key_info));

	if (err) goto fail;
	s.array = arr;
	ip_queue_get(&s);
	err = copy_to_user(buf, s.buf,
			s.n * sizeof(struct ip_key_info));
	if (err) goto fail;
	s.buf = buf;
	err = copy_to_user(v, &s, sizeof(s));
fail:
	set(NAME_SET);
	if (err) pr_debug(IPS"ips fetch errno [%d]\n", err);
	return err;
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
	case IPS_SET_NAMELIST:
		err = set_namelist((void*)arg);
		break;
	case IPS_FETCH_INFO:
		err =fetch_info((void*)arg);
		break;
	default:
		pr_err("handle error:IPS_CMD_NUM:[%d]\n", cmdnum);
		return -EFAULT;
	}
	pr_debug(IPS"ioctl errno[%d]\n", err);
	return err;
}

static int proc_show(struct seq_file *m, void *v)
{
	int ret = devset_show(m);
	if (ret == 0) ip_queue_show(m);
	return ret;
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


