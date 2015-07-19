#include <linux/kfifo.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include "ip_queue.h"
struct ip_queue __STRUCT_KFIFO_PTR(unsigned short, 0, unsigned short);
struct queue_data{
	unsigned short start;
	unsigned short end;
	struct ip_queue idle;
	struct ip_queue ready;
	bool alarm;
};
struct queue_data_ptr{
	struct queue_data * dataptr;
};

static struct ips_dma_area *m = NULL;
static u32 putfail = 0;
static u32 getfail = 0;
static u32 puttimes = 0;
static struct queue_data_ptr * __percpu qptr;

static struct ip_key_info_wrap *num2addr(unsigned short i)
{
	return m->elem + i;
}

static unsigned short getsize(int cpu, const struct ips_cpu_queue_size *s,
		int num, unsigned short default_size)
{
	int i = -1;
	for (i = 0; i < num; i++) {
		if (s[i].cpuidx == cpu){
			return s[i].size;
		}
	}
	return default_size;

}

void *ip_queue_dma_addr(void)
{
	return (void *)m;
}
int ip_queue_create(const struct ips_cpu_queue_size *s,
		int num, unsigned short default_size)
{
	int cpu = 0;
	struct queue_data_ptr *c = NULL;
	int ret = 0;
	int total = 0;
	unsigned short pre = 0;
	unsigned short i = 0;
	unsigned short size = 0;
	qptr = alloc_percpu(struct queue_data_ptr);
	if (qptr == NULL) return -ENOMEM;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr, cpu);
		c->dataptr = kzalloc(sizeof(struct queue_data), GFP_KERNEL);
		if (c->dataptr == NULL){
			ret = -ENOMEM;
			break;
		}
		c->dataptr->start = pre;
		size = getsize(cpu, s, num, default_size);
		pre = pre + size;

		/* it is much more than 65535, wrap around */
		if (pre <= size) {
			ret = -EINVAL;
			break;
		}
		c->dataptr->end = pre;

		if(kfifo_alloc(&c->dataptr->ready, size, GFP_KERNEL)){
			ret = -ENOMEM;
			break;
		}
		if(kfifo_alloc(&c->dataptr->idle, size, GFP_KERNEL)){
			ret = -ENOMEM;
			break;
		}
		for (i = c->dataptr->start; i < pre; i++) {
			if (kfifo_put(&c->dataptr->idle, &i) == 0) {
				ret = -ENOMEM;
				pr_err(IPS"critical issue, idlist is full,\n");
				break;
			}
			
		}
	}
	if (ret) {
		ip_queue_exit();
		return ret;
	}
	total = sizeof(struct ips_dma_area)
			+ (sizeof (struct ip_key_info_wrap)) * pre;
	m = vmalloc(total);
	if (m == NULL) {
		ret = -ENOMEM;
		ip_queue_exit();
		return ret;
	}
	memset(m, 0, total);
	m->idx_num = 0;
	m->ip_key_info_total_size = pre;
	ret = total;
	return ret;
}
int ip_queue_recycle(unsigned short *idlist, int size)
{
	int i = 0; 
	int cpu = 0;
	bool empty = false;
	struct queue_data_ptr *c = NULL;
	int ret = 0;
	struct ip_key_info_wrap* addr = NULL;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr, cpu);
		empty = true;
		for (i = 0; i < size; i++) {
			if (idlist[i] == 0) continue;
			if (idlist[i] >= c->dataptr->start
					&& idlist[i] < c->dataptr->end) {
				addr = num2addr(idlist[i]);
				addr->state = MEM_USED;
				if(kfifo_put(&c->dataptr->idle, &idlist[i]) == 0){
					ret = -ENOMEM;
					putfail++;
					pr_err(IPS"critical issue, idlist is full,\n");
				}
				idlist[i] = 0;
			}
			empty = false;
		}
		if (empty) break;
	}
	return ret;
}

int ip_queue_move2dma(void)
{
	unsigned short *idlist = m->idx_array;
	int num = ips_dma_idx_size;
	int cpu = 0;
	int copied = 0;
	struct queue_data_ptr *c = NULL;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr, cpu);
		if (kfifo_is_empty(&qptr->dataptr->ready)) continue;
		copied += kfifo_out(&qptr->dataptr->ready,
				idlist + copied, num);
		num -= copied;
		if (num <= 0) break;
	}
	m->idx_num = copied;
	return copied;
}

bool ip_queue_shall_wakeup(void)
{
	if (puttimes > ips_dma_idx_size /2){
		puttimes = 0;
		return true;
	}
	return false;
}

bool ip_queue_has_data(void)
{
	return puttimes > 0;
}

void ip_queue_put(const struct ip_key_info *src)
{
	unsigned short s = 0;
	struct ip_key_info_wrap* addr = NULL;
	struct queue_data_ptr *c = NULL;
	if (unlikely(qptr == NULL)) return ;

	puttimes++;
	c = get_cpu_ptr(qptr);
	if (0 == kfifo_get(&qptr->dataptr->idle, &s)){
		getfail++;
	} else {
		addr = num2addr(s);
		memcpy(&addr->info, src, sizeof(struct ip_key_info));
		addr->times++;
		addr->state = MEM_INUSE;
		if (0 == kfifo_put(&qptr->dataptr->ready, &s)) {
			kfifo_put(&qptr->dataptr->idle, &s);
		}
	}
	put_cpu_ptr(qptr);

}

void ip_queue_exit(void)
{
	int cpu = 0;
	struct queue_data_ptr *c = NULL;
	if (qptr != NULL){
		for_each_possible_cpu(cpu) {
			c = per_cpu_ptr(qptr, cpu);
			if (c->dataptr != NULL) {
				kfifo_free(&c->dataptr->idle);
				kfifo_free(&c->dataptr->ready);
				kfree(c->dataptr);
				c->dataptr = NULL;
			}
		}
		free_percpu(qptr);
	}
	if (m != NULL) {
		vfree(m);
	}
}
