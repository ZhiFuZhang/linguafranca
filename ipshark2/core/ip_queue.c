struct ip_queue STRUCT_KFIFO_PTR(unsigned short);

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

static u32 putfail = 0;
static u32 getfail = 0;
static struct queue_data_ptr * __percpu qptr;
static struct ip_key_info_wrap *(*num2addr)(unsigned short) = NULL;
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

int ip_queue_create(const struct ips_cpu_queue_size *s,
		int num, unsigned short default_size,
		struct ip_key_info_wrap *(*num2addr_fun)(unsigned short))
{
	int cpu = 0;
	struct queue_data_ptr *c = NULL;
	int ret = 0;
	unsigned short pre = 1;
	unsigned short i = 0;
	unsigned short size = 0;
	num2addr = num2addr_fun;
	qptr = alloc_percpu(struct queue_data_ptr);
	if (qptr == NULL) return -ENOMEM;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr);
		c->dataptr = kzalloc(sizeof(struct queue_data), GFP_KERNEL);
		if (c->dataptr == NULL){
			ret = -ENOMEM;
			break;
		}
		c->dataptr->start = pre;
		size = getsize(cpu, s, num, default_size);
		pre = pre + size;
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
			if (kfifo_put(&c->dataptr->idle, i) == 0) {
				ret = -ENOMEM;
				pr_err(IPS"critical issue, idlist is full,\n");
				break;
			}
			
		}
	}
	if (ret) {
		ip_queue_exit();
	}
	return ret;

}

int ip_queue_recycle(unsigned short idlist *, int size)
{
	int i = 0; 
	int cpu = 0;
	bool empty = false;
	struct queue_data_ptr *c = NULL;
	int ret = 0;
	struct ip_key_info_wrap* addr = NULL;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr);
		empty = true;
		for (i = 0; i < size; i++) {
			if (idlist[i] == 0) continue;
			if (idlist[i] >= c->datptr->start
					&& idlist[i] < c->datraptr->end) {
				addr = num2addr(idlist[i]);
				addr->state = MEM_USED;
				if(kfifo_put(&c->dataptr->idle, idlist[i]) == 0){
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

int ip_queue_get(unsigned short *idlist, int num)
{
	int cpu = 0;
	int copied = 0;
	struct queue_data_ptr *c = NULL;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr);
		if (kfifo_is_empty(&qptr->dataptr->ready)) continue;
		copied += kfifo_out(&qptr->dataptr->ready,
				idlist + copied, num);
		num -= copied;
		if (num <= 0) break;
	}
	return copied;
	
}
void ip_queue_put(const struct ip_key_info *src)
{
	bool  in_softirq = in_softirq();
	int ret = 0;
	STRUCT_IP_FIFO_PTR *c = NULL;
	if (unlikely(ip_fifo == NULL)) return 0;
	if (!in_softirq) {
		local_bh_disable();
	}

	unsigned short s = 0;
	struct ip_key_info_wrap* addr = NULL;
	struct queue_data_ptr *c = NULL;
	c = get_cpu_ptr(qptr);
	if (0 == kfifo_get(&qptr->dataptr->idle, s)){
		getfail++;
		return;
	}
	addr = num2addr(s);
	memcpy(&addr->info, src, sizeof(struct ip_key_info));
	addr->times++;
	addr->state = MEM_INUSE;
	if (0 == kfifo_put(&qptr->dataptr->ready, s)) {
		kfifo_put(&qptr->dataptr->idle, s);
	}
	put_cpu_ptr(qptr);

}

void ip_queue_exit(void)
{
	int cpu = 0;
	struct queue_data_ptr *c = NULL;
	int ret = 0;
	if (qptr != NULL){
		for_each_possible_cpu(cpu) {
			c = per_cpu_ptr(qptr);
			if (c->dataptr != NULL) {
				kiffo_free(&c->dataptr->idle);
				kiffo_free(&c->dataptr->ready);
				kfree(c->dataptr);
				c->dataptr = NULL;
			}
		}
		free_percpu(qptr);
	}
	num2addr = NULL; 
}
