#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include <linux/sched.h>
#include <linux/wait.h>
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
static unsigned short wait_ms = 10;
static u32 puttimes = 0;
static struct queue_data_ptr * __percpu qptr;
static DECLARE_WAIT_QUEUE_HEAD(wq);

static u64 queue_times = 0;
static u64 recycle_times = 0;
static u32 putfail = 0;
static u32 getfail = 0;
static u32 get_invalid_idx_times = 0;
static u32 recycle_invalid_idx_times = 0;
void ip_queue_show(struct seq_file *m)
{
	seq_printf(m, "queued packets [%llu]\n", queue_times);
	seq_printf(m, "recycled idx [%llu]\n", recycle_times);

	seq_printf(m, "no idle idx, times[%u]\n", getfail);
	seq_printf(m, "ready queue full, times[%u]\n", putfail);
	seq_printf(m, "try to use USED memory, times[%u]\n",
			get_invalid_idx_times);
	seq_printf(m, "try to free freed memory,times[%u]\n",
			recycle_invalid_idx_times);
}
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
		int num, unsigned short default_size, unsigned short ms)
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
		c->dataptr->alarm = false;
		size = getsize(cpu, s, num, default_size);
		pre = pre + size;

		/* it is much more than 65535, wrap around */
		if (pre < size) {
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
	memset(m, 0xec, total);
	m->idx_num = 0;
	m->ip_key_info_total_size = pre;
	for (i = 0; i < pre; i++) {
		m->elem[i].times = 0;
		m->elem[i].state = MEM_FREE;
	}
	ret = total;
	if (ms < 10)
		wait_ms = 10;
	else if (ms > 1000)
		wait_ms = 1000;
	else 
		wait_ms = ms;

	return ret;
}
void ip_queue_recycle(unsigned short *idlist, int size)
{
	int i = 0; 
	int cpu = 0;
	struct queue_data_ptr *c = NULL;
	struct ip_key_info_wrap* addr = NULL;
	bool handled = false;
	for (i = 0; i < size; i++) {
		handled = false;
		addr = num2addr(idlist[i]);
		if (addr->state != MEM_INUSE){
			recycle_invalid_idx_times++;
			pr_err_once(IPS"recycle unused memeory \n");
			continue;	
		}
		for_each_possible_cpu(cpu) {
			c = per_cpu_ptr(qptr, cpu);
			if (idlist[i] >= c->dataptr->start
					&& idlist[i] < c->dataptr->end) {
				addr->state = MEM_USED;
				if(kfifo_put(&c->dataptr->idle, &idlist[i]) == 0){
					putfail++;
					pr_err_once(IPS"critical,idlist is full\n");
				}
				idlist[i] = 0;
				handled = true;
				recycle_times++; 
			}
		}
		if (!handled){
			pr_err(IPS"idx is invalid,idx %d\n", idlist[i]);
		}
	}
}

void ip_queue_move2dma(void)
{
	unsigned short *idlist = m->idx_array;
	int num = ips_idx_array_size;
	int cpu = 0;
	int copied = 0;
	struct queue_data_ptr *c = NULL;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr, cpu);
		if (kfifo_is_empty(&c->dataptr->ready)) continue;
		copied += kfifo_out(&c->dataptr->ready,
				idlist + copied, num);
		num -= copied;
		if (num <= 0) break;
	}
	m->idx_num = copied;
}
static bool ip_queue_shall_wakeup(void)
{
	if (puttimes > ips_idx_array_size/2){
		return true;
	}
	return false;
}

void ip_queue_wakeup(void)
{
	if (ip_queue_shall_wakeup()) {
		wake_up_interruptible(&wq);
	}
}
long ip_queue_wait(void)
{
	int remain = wait_event_interruptible_timeout(wq,
			ip_queue_shall_wakeup(), (HZ/1000) * wait_ms);
	/* meet the condition */
	if (remain > 0){
		puttimes = 0;
	}
	return remain;
}
bool ip_queue_has_data(void)
{
	if (puttimes > 0) {
		puttimes = 0;
		return true;
	}
	return false;
}

void ip_queue_put(const struct ip_key_info *src)
{
	unsigned short s = 0;
	struct ip_key_info_wrap* addr = NULL;
	struct queue_data_ptr *c = NULL;
	if (unlikely(qptr == NULL)) return ;

	puttimes++;
	c = get_cpu_ptr(qptr);
	if (0 == kfifo_get(&c->dataptr->idle, &s)){
		getfail++;
		pr_err_once("no idle idx\n");
	} else {
		addr = num2addr(s);
		while (addr->state == MEM_INUSE){
			get_invalid_idx_times++;
			pr_err_once(IPS"idx is in use!!!! [%d]\n", s);
			if (0 == kfifo_get(&c->dataptr->idle, &s)){
				getfail++;
				pr_err_once("no idle idx\n");
				return;
			}
		}

		memcpy(&addr->info, src, sizeof(struct ip_key_info));
		addr->times++;
		addr->state = MEM_INUSE;
		if (0 == kfifo_put(&c->dataptr->ready, &s)) {
			kfifo_put(&c->dataptr->idle, &s);
		}else {
			queue_times++;
		}
	}
	if (c->dataptr->alarm){
		if (kfifo_len(&c->dataptr->ready)
			       	< kfifo_size(&c->dataptr->ready)/2 ) {
			c->dataptr->alarm = false;		
			pr_warn(IPS"clear alarm for queue(%p)\n", c);
		}
	} else {
		if (kfifo_len(&c->dataptr->ready)
			       	> 3 * kfifo_size(&c->dataptr->ready)/4 ) {
			c->dataptr->alarm = true;
			pr_warn(IPS"raise alarm for queue(%p)\n", c);
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
		qptr = NULL;
	}
	if (m != NULL) {
		vfree(m);
		m = NULL;
	}
}
