#ifndef __INTERN_H2__HEAD
#define __INTERN_H2__HEAD
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include "ips.h"
#define IPS "ipshark2:"
struct dev_entry{
	struct devname n;
	u32 hash;
	struct hlist_node node;
};
struct ipshark_data{
	void *dma_addr;
	size_t dma_size;
	void * __percpu idle;
	void * __percpu touser;
};
int ips_create_proc(void);
void ips_rmv_proc(void);
int ips_mmap(struct file *filp, struct vm_area_struct *vma);

#endif
