#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "intern.h"
#include "ip_queue.h"
static int proc_show(struct seq_file *m, void *v)
{
	return 0;
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
int ips_create_proc(void)
{
	struct proc_dir_entry *p = proc_create_data(IPS_DEV_PROC,
					0444, NULL, &proc_ops, NULL);
	if (!p) {
		pr_err(IPS"could not create proc %s\n",
				IPS_DEV_PROC);
		return -1;
	}

	return 0;
}

void ips_rmv_proc(void)
{

	remove_proc_entry(IPS_DEV_PROC, NULL);
}
