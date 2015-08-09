#include "../ip_queue.c"
#include "ut.h"

static __sunny  int ip_queue_create_test1(void)
{
	struct ips_cpu_queue_size s[] = {
		[0] = {.cpuidx = 0, .size = 512},
		[1] = {.cpuidx = 2, .size = 128}
	};
	unsigned short ds = 256;
	int t = ip_queue_create(s,
			sizeof(s)/sizeof(struct ips_cpu_queue_size), ds, 50);
	int cpu = 0;
	struct queue_data_ptr *c = NULL;
	unsigned short n = 0;
	unsigned short i = 0;
	if (m == NULL || qptr == NULL) return 1;
	if (t <= 0) return 255;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr, cpu);
		if (c->dataptr == NULL) return 2;

		if (c->dataptr->start != n) return 3;
		if (cpu == 0) {
			if (c->dataptr->end - c->dataptr->start != 512)
				return 4;

		} else if (cpu == 2) {
			if (c->dataptr->end - c->dataptr->start != 128)
				return 4;
		} else {
			if (c->dataptr->end - c->dataptr->start != 256)
			       	return 4;
		}
		if (!kfifo_is_empty(&c->dataptr->ready)) return 5;
		while(!kfifo_is_empty(&c->dataptr->idle)){
			if (0 == kfifo_get(&c->dataptr->idle, &i)) {
				return 6;
			}
			if (i != n) return 7;
			n++;
		}
		if (n != c->dataptr->end) return 8;
	}
	if (sizeof(struct ips_dma_area) + n * sizeof(struct ip_key_info_wrap) 
		!= t)
			return 9;
	return 0;
}

static __sunny int ip_queue_exit_test1(void)
{
	ip_queue_exit();
	if (m != NULL || qptr != NULL) return 1;
	return 0;
}

static __sunny int ip_queue_e2e_test1(void)
{
	struct ips_cpu_queue_size s[] = {
		[0] = {.cpuidx = 0, .size = 512},
		[1] = {.cpuidx = 2, .size = 128}
	};
	unsigned short ds = 256;
	int t = ip_queue_create(s,
			sizeof(s)/sizeof(struct ips_cpu_queue_size), ds, 60);
	struct ip_key_info info = {
		.in = {"hello"},
		.sport = 11111,
		.dport = 33333,
		.totallen = 0
	};
	struct ip_key_info_wrap *w = NULL; 
	int i = 0;
	if (t <= 0) return 1;
	for (i = 0; i < m->ip_key_info_total_size; i++) {
		ip_queue_put(&info);
		ip_queue_move2dma();
		if (m->idx_num != 1) return 3;
		w = num2addr(m->idx_array[0]);
		if (w->times != 1) return 4;
		if (w->state != MEM_INUSE) return 5;
		if (memcmp(&w->info, &info, sizeof(info)) != 0) return 6;
		ip_queue_recycle(m->idx_array, m->idx_num);
		if (w->times != 1) return 8;
		if (w->state != MEM_USED) return 9;
	}
	ip_queue_exit();
	return 0;
}

static __sunny int ip_queue_e2e_test2(void)
{
	struct ips_cpu_queue_size s[] = {
		[0] = {.cpuidx = 0, .size = 512},
		[1] = {.cpuidx = 2, .size = 128}
	};
	unsigned short ds = 256;
	int t = ip_queue_create(s,
			sizeof(s)/sizeof(struct ips_cpu_queue_size), ds, 70);
	struct ip_key_info info = {
		.in = {"ipshark2"},
		.sport = 41111,
		.dport = 23333,
		.totallen = 0
	};
	struct ip_key_info_wrap *w = NULL; 
	int i = 0;
	int j = 0;
	int k = 0;
	if (t <= 0) return 1;
	for (i = 0; i < m->ip_key_info_total_size; i++) {
		k = ((i * i)%ips_idx_array_size) + 1;
		info.totallen++;
		for (j = 0; j < k; j++) {
			ip_queue_put(&info);
		}
		ip_queue_move2dma();
		if (m->idx_num != k) return 3;
		for (j = 0; j < k; j++) {
			w = num2addr(m->idx_array[j]);
			if (w->times == 0)
				return 4;
			if (w->state != MEM_INUSE) return 5;
			if (memcmp(&w->info, &info, sizeof(info)) != 0)
				return 6;
		}
		ip_queue_recycle(m->idx_array, m->idx_num);
		if (w->state != MEM_USED) return 9;
	}
	ip_queue_exit();
	return 0;
}

struct ut_result ip_queue_ut(void)
{
	struct ut_result r = {0, 0};
	pr_info(IPS"%s started.\n", __FUNCTION__);
	runtest(r, ip_queue_create_test1);
	runtest(r, ip_queue_exit_test1);
	runtest(r, ip_queue_e2e_test1);
	runtest(r, ip_queue_e2e_test2);
	pr_info(IPS"%s finished.\n", __FUNCTION__);
	return r;
}

