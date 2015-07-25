#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mm.h>		/* everything */
#include <linux/errno.h>	/* error codes */
#include <asm/pgtable.h>
#include "intern.h"
#include "ip_queue.h"

void ips_vma_open(struct vm_area_struct *vma)
{

}

void ips_vma_close(struct vm_area_struct *vma)
{

}

static int ips_vma_fault(struct vm_area_struct *vma,
		struct vm_fault *vmf)
{
	unsigned long offset;
	struct page *page = NULL;
	void *pageptr = NULL; /* default to "missing" */

	offset = (vmf->pgoff) + (vma->vm_pgoff << PAGE_SHIFT);
	pr_info("dma, %lu, %lu, %lu\n", vmf->pgoff, vma->vm_pgoff, offset);
	offset >>= PAGE_SHIFT; 
	
	pageptr =  ip_queue_dma_addr();
	if (!pageptr) return VM_FAULT_NOPAGE; /* hole or end-of-file */
	pageptr =  ip_queue_dma_addr();
	pr_info(IPS"vmalloc addr [%p]\n", pageptr);

	page = vmalloc_to_page(pageptr);
	if (!page)  
		return VM_FAULT_SIGBUS; 

	get_page(page);
	vmf->page = page;
	return 0;
}



struct vm_operations_struct ips_vm_ops = {
	.open =     ips_vma_open,
	.close =    ips_vma_close,
	.fault = ips_vma_fault,
};


int ips_mmap(struct file *filp, struct vm_area_struct *vma)
{
	/* don't do anything here: "nopage" will set up page table entries */
	vma->vm_ops = &ips_vm_ops;
	vma->vm_flags |= VM_READ;
	ips_vma_open(vma);
	return 0;
}

