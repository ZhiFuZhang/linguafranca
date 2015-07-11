struct idle_queue STRUCT_KFIFO_PTR(unsigned short);

struct queue_data{
	unsigned short start;
	unsigned short end;
	struct idle_queue fifo;
	bool alarm;
};
struct queue_data_ptr{
	struct queue_data * dataptr;
};

static u32 putfail = 0;
static u32 getfail = 0;
static struct queue_data_ptr * __percpu qptr;

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

int idle_queue_create(const struct ips_cpu_queue_size *s,
		int num, unsigned short default_size)
{
	int cpu = 0;
	struct queue_data_ptr *c = NULL;
	int ret = 0;
	unsigned short pre = 1;
	qptr = alloc_percpu(struct queue_data_ptr);
	if (qptr == NULL) return -ENOMEM;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr);
		c->dataptr = kzalloc(sizeof(struct queue_data_ptr), GFP_KERNEL);
		if (c->dataptr == NULL){
			ret = -ENOMEM;
			break;
		}
		c->dataptr->start = pre;
		pre = pre + getsize(cpu, s, num, default_size);
		c->dataptr->end = pre;
		if(kfifo_alloc(&c->dataptr->fifo, pre, GFP_KERNEL)){
			ret = -ENOMEM;
			break;
		}

	}
	if (ret) {
		idle_queue_exit();
	}
	return ret;

}

int idle_queue_put(unsigned short idlist *, int size)
{
	int i = 0; 
	int cpu = 0;
	bool empty = false;
	struct queue_data_ptr *c = NULL;
	int ret = 0;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr);
		empty = true;
		for (i = 0; i < size; i++) {
			if (idlist[i] == 0) continue;
			if (idlist[i] >= c->datptr->start
					&& idlist[i] < c->datraptr->end)
			{
				ret = kfifo_put(&c->dataptr->fifo, idlist[i]);
				if (ret == 0){
					putfail++;
				}
				idlist[i] = 0;
			}
			empty = false;
		}
		if (empty) break;
	}
	return ret;
}

unsigned short idle_queue_get(void)
{
	unsigned short s = 0;
	struct queue_data_ptr *c = NULL;
	c = get_cpu_ptr(qptr);
	if (0 == kfifo_get(&qptr->dataptr->fifo, s)){
		getfail++;
		s = 0;
	}
	put_cpu_ptr(qptr);
	return s;
}

void idle_queue_exit(void)
{
	int cpu = 0;
	struct queue_data_ptr *c = NULL;
	int ret = 0;
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(qptr);
		kiffo_free(&c->dataptr->fifo);
		kfree(c->dataptr);
		c->dataptr = NULL;
	}
	free_percpu(qptr);
}
