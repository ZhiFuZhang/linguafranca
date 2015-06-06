#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include <linux/preempt_mask.h>
#include "intern.h"
#include "ips.h"
#define IP_FIFO_SIZE 1024
typedef STRUCT_KFIFO(struct ip_key_info, IP_FIFO_SIZE) STRUCT_IP_FIFO;
static  STRUCT_IP_FIFO *__percpu ip_fifo = NULL;

int ip_queue_init()
{
	int cpu = 0;
	STRUCT_IP_FIFO *c = NULL;
	ip_fifo = alloc_percpu(STRUCT_IP_FIFO);
	if (ip_fifo == NULL){
		return -ENOMEM;
	}
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(ip_fifo, cpu);
		INIT_KFIFO(*c);
	}
}

int ip_queue_put(const struct ip_key_info *info) {
	bool  in_softirq = in_softirq();
	int ret = 0;
	STRUCT_IP_FIFO *c = NULL;
	if (!in_softirq) {
		local_bh_disable();
	}
	c = get_cpu_ptr(ip_fifo);
	ret = kfifo_put(c, info);
	put_cpu_ptr(ip_fifo);
	if (!in_softirq) {
		local_bh_enable();
	}
	return ret;
}

void ip_queue_get(struct ip_key_info_set *s){
	int cpu = 0;
	struct ip_key_info *info_array = s->array;
	size_t infonum = s->n;
	STRUCT_IP_FIFO *c = NULL;
	int copied = 0;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(ip_fifo, cpu);
		if (kfifo_is_empty(c)) continue;
		copied += kfifo_out(c, info_array + copied, infonum);
		infonum -= copied;
		if (infonum <= 0) break;
	}
	s->n = copied;
}
bool ip_queue_empty() {
	int cpu = 0;
	STRUCT_IP_FIFO *c = NULL;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(ip_fifo, cpu);
		if (kfifo_is_empty(c)){
			continue;
		} else {
			return true;
		}	
	}
	return false;
}

void ip_queue_exit()
{
	free_percpu(ip_fifo);	
}

#ifdef UNITTEST
struct ut_result ip_queue_ut(void){


}

#endif
