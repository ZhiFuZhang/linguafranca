#include <linux/dcache.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/netdevice.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include <linux/preempt_mask.h>
#include <linux/wait.h>
#include "intern.h"
#include "ips.h"
#define IP_FIFO_SIZE (1024 * 16)
typedef STRUCT_KFIFO(struct ip_key_info, IP_FIFO_SIZE) STRUCT_IP_FIFO;
typedef struct {
	STRUCT_IP_FIFO *fifo;
	bool alarm;
}STRUCT_IP_FIFO_PTR;
static STRUCT_IP_FIFO_PTR *__percpu ip_fifo = NULL;
static DECLARE_WAIT_QUEUE_HEAD(wq);
int ip_queue_init(void)
{
	int cpu = 0;
	STRUCT_IP_FIFO_PTR *c = NULL;
	int ret = 0;
	ip_fifo = alloc_percpu(STRUCT_IP_FIFO_PTR);
	if (ip_fifo == NULL){
		return -ENOMEM;
	}
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(ip_fifo, cpu);
		c->fifo = NULL;
		c->alarm = false;
		c->fifo = vmalloc(sizeof(STRUCT_IP_FIFO));
		if (unlikely(c->fifo == NULL)) {
			ret = -ENOMEM;	
			break;
		}
		INIT_KFIFO(*(c->fifo));
	}
	if (ret != 0) {
		ip_queue_exit();	
	}
	return ret;
}
static u64 un_queued_pkts = 0;
static u64 put_pkts = 0;
static u64 get_pkts = 0;
int ip_queue_put(const struct ip_key_info *info) 
{
	bool  in_softirq = in_softirq();
	int ret = 0;
	STRUCT_IP_FIFO_PTR *c = NULL;
	if (unlikely(ip_fifo == NULL)) return 0;
	if (!in_softirq) {
		local_bh_disable();
	}
	c = get_cpu_ptr(ip_fifo);
	ret = kfifo_put(c->fifo, info);
	if (unlikely(ret == 0)) {
		un_queued_pkts++;
	} else {
		put_pkts++;
	}
	if (c->alarm) {
		if (kfifo_len(c->fifo) < (IP_FIFO_SIZE * 3 /5)) {
			pr_warn(IPS"queue is used less than 3/5,clear warn."
					"[%ld]\n", ((long)c) & 0xffffff);
			c->alarm = false;

		}
	} else {
		if (kfifo_len(c->fifo) > (IP_FIFO_SIZE * 4 /5)) {
			c->alarm = true;
			pr_warn(IPS"queue is used more than 4/5,warning.[%ld]\n",
					((long)c&0xffffffff));
		} 
	}
	put_cpu_ptr(ip_fifo);
	if (!in_softirq) {
		local_bh_enable();
	}
	return ret;
}

int ip_queue_show(struct seq_file *m)
{

	seq_printf(m, "un handled pkts number[%llu]\n", un_queued_pkts);
	seq_printf(m, "pkts put in queue [%llu]\n", put_pkts);
	seq_printf(m, "pkts get from queue [%llu]\n", get_pkts);
	return 0;
}

void ip_queue_wake_up(void)
{
	static int num = 0;
	num++;

	if (num > 32) {
		num = 0;
		wake_up_interruptible(&wq);
	}
}
static bool ip_queue_empty(void) 
{
	int cpu = 0;
	STRUCT_IP_FIFO_PTR *c = NULL;
	if (unlikely(ip_fifo == NULL)) return true;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(ip_fifo, cpu);
		if (kfifo_is_empty(c->fifo)){
			continue;
		} else {
			return true;
		}	
	}
	return false;
}
long ip_queue_wait(void) 
{
	return wait_event_interruptible_timeout(wq, !ip_queue_empty(), HZ/50); 
}

void ip_queue_get(struct ip_key_info_set *s)
{
	int cpu = 0;
	struct ip_key_info *info_array = s->array;
	size_t infonum = s->n;
	STRUCT_IP_FIFO_PTR *c = NULL;
	int copied = 0;

	if (unlikely(ip_fifo == NULL)) {
		s->n = 0;	
		return ;
	}
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(ip_fifo, cpu);
		if (kfifo_is_empty(c->fifo)) continue;
		copied += kfifo_out(c->fifo, info_array + copied, infonum);
		infonum = s->n - copied;
		if (infonum <= 0) break;
	}
	s->n = copied;
	pr_debug(IPS"get from queue %d\n", s->n);
	get_pkts+= copied;
}


void ip_queue_exit(void)
{
	int cpu = 0;
	STRUCT_IP_FIFO_PTR *c = NULL;
	if (unlikely(ip_fifo == NULL)) return;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(ip_fifo, cpu);
		vfree(c->fifo);
		c->fifo = NULL;
	}
	free_percpu(ip_fifo);	
	ip_fifo = NULL;
	un_queued_pkts = 0;
	put_pkts = 0;
	get_pkts = 0;
}

#ifdef UNITTEST
static int __sunny ip_queue_init_test(void){
	int ret = ip_queue_init();
	if (ret == 0) {
		if (ip_fifo == NULL) 
			return 1;
		else
			return 0;
	}
	if (ret ==  -ENOMEM){
		if (ip_fifo == NULL)
			return 0;
		else
			return 1;
	}
	return 1;

}

static int __rainy ip_queue_get_put_test(void){
	char buf[2*sizeof(struct ip_key_info)];
	struct ip_key_info_set s = {
		.buf = &buf[0],
		.n = 2
	};
	ip_queue_get(&s);
	if (s.n != 0) return 1;
	return 0;

}
static int __sunny ip_queue_get_put_test2(void){
	struct ip_key_info info = {
		.in = {"in"},
		.out = {"ount"},
		.s4 = {192, 168,1, 10},
		.d4 = {232,198,6.72},
		.version = 4,
		.direct = 2,
		.protocol = 23,
		.sport = 1293,
		.dport = 65535,
		.totallen = 3545
	};
	char buf[2*sizeof(struct ip_key_info)];
	struct ip_key_info_set s = {
		.buf = &buf[0],
		.n = 2
	};
	int r = ip_queue_put(&info);
	if (r != 1) {
		pr_debug(IPS"ip_queue_put failed\n");
	       	return 1;
	}
	ip_queue_get(&s);
	if (s.n != 1) return 1;
	if (memcmp(&info, s.array, sizeof(info)) != 0) return 1;
	return 0;
	
}
static int __sunny ip_queue_get_put_test3(void){
	struct ip_key_info info = {
		.in = {"in"},
		.out = {"ount"},
		.s4 = {192, 168,1, 10},
		.d4 = {232,198,6.72},
		.version = 4,
		.direct = 2,
		.protocol = 23,
		.sport = 1293,
		.dport = 65535,
		.totallen = 3545
	};
	char buf[5*sizeof(struct ip_key_info)];
	struct ip_key_info_set s = {
		.buf = &buf[0],
		.n = 5
	};
	int r = 0;
	int i = 0;
	int cpu = get_cpu();
	pr_debug(IPS"cpu %d\n", cpu);
	for (i = 0; i < 1024; i++) {
		r = ip_queue_put(&info);
		if (r != 1) {
			pr_debug(IPS"ip_queue_put [%d] failed, %d\n",i, r);
			return 1;
		}
	}

	r = ip_queue_put(&info);
	if (r != 0) {
			pr_debug(IPS"ip_queue_put full failed\n");
			return 1;
	}
	put_cpu();
	ip_queue_get(&s);
	if (s.n != 5) return 1;
	if (memcmp(&info, s.array, sizeof(info)) != 0) return 1;
	return 0;
	
}
static int __sunny ip_queue_get_put_test4(void){
	struct ip_key_info info = {
		.in = {"in"},
		.out = {"ount"},
		.s4 = {192, 168,1, 10},
		.d4 = {232,198,6.72},
		.version = 4,
		.direct = 2,
		.protocol = 23,
		.sport = 1293,
		.dport = 65535,
		.totallen = 3545
	};
	static char buf[1024*sizeof(struct ip_key_info)];
	struct ip_key_info_set s = {
		.buf = &buf[0],
		.n = 1024
	};
	int r = 0;
	int i = 0;
	ip_queue_exit();
	r = ip_queue_init();
	if (r != 0) {
		pr_debug(IPS"test4, init failed\n");
		return 1;
	}

	/* for up, it is in one kfifo 
	 * for smp, it may be in different kfifo.
	 * however, we can get all of it at one time.
	 */	
	for (i = 0; i < 1024; i++) {
		r = ip_queue_put(&info);
		if (r != 1) {
			pr_debug(IPS"ip_queue_put failed\n");
			return 1;
		}
	}

	ip_queue_get(&s);

	pr_debug(IPS"ip_queue_get all, [%lu] \n", (long unsigned int)s.n);
	if (s.n != 1024) return 1;
	if (memcmp(&info, s.array, sizeof(info)) != 0) return 1;
	return 0;
	
}

static int __sunny ip_queue_exit_test(void){
	ip_queue_exit();
	if (ip_fifo == NULL) return 0;
	return 1;
}

struct ut_result ip_queue_ut(void){
	struct ut_result r = {0, 0};
	struct ut_result critical_fail = {0xffff, 0xffff};
	pr_info(IPS"ip_queue_ut started\n");
	runtest(r, ip_queue_init_test);
	if (ip_fifo == NULL) return critical_fail;
	runtest(r, ip_queue_get_put_test);
	runtest(r, ip_queue_get_put_test2);
	runtest(r, ip_queue_get_put_test3);
	runtest(r, ip_queue_get_put_test4);
	runtest(r, ip_queue_exit_test);
	return r;
}

#endif
