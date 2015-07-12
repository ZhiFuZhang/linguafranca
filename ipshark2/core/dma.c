#include <linux/fs.h>
#include <linux/module.h>
#include <linux/mm.h>		/* everything */
#include <linux/errno.h>	/* error codes */
#include <asm/pgtable.h>

#include "intern.h"

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
	//struct ips_dev *ptr, *dev = vma->vm_private_data;
	struct page *page = NULL;
	void *pageptr = NULL; /* default to "missing" */

	offset = (vmf->pgoff) + (vma->vm_pgoff << PAGE_SHIFT);
	//if (offset >= dev->size) goto out; /* out of range */

	offset >>= PAGE_SHIFT; 
	/*
	for (ptr = dev; ptr && offset >= dev->qset;) {
		ptr = ptr->next;
		offset -= dev->qset;
	}*/
	//if (ptr && ptr->data) pageptr = ptr->data[offset];
	//if (!pageptr) goto out; /* hole or end-of-file */
	pageptr =  vma->vm_private_data;
	pr_info(IPS"vmalloc addr [%p]\n", pageptr);

	/*
	 *	 * After ips lookup, "page" is now the address of the page
	 *		 * needed by the current process. Since it's a vmalloc address,
	 *			 * turn it into a struct page.
	 *				 */
	page = vmalloc_to_page(pageptr);
	if (!page)  
		return VM_FAULT_SIGBUS; 

	get_page(page);
	vmf->page = page;
	//if (type)
	//	*type = VM_FAULT_MINOR;
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
	vma->vm_private_data = filp->private_data;
	ips_vma_open(vma);
	return 0;
}

int ips_mem_create(size_t esize)
{

}
