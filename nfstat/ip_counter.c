#include <linux/kernel.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include <linux/rbtree.h>

#include "internal.h"

static struct rb_root iptree = RB_ROOT;
DEFINE_RWLOCK(iptreelock); /*only when the iptree insert/delete, it is write mode */
static u8 maxtype = 8;

static inline void delete(struct ip_counter_entry *entry)
{

	struct nfs_counter_vector *c = NULL;
	if (entry == NULL) return;
	kfree(entry->rulecount);
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(entry->counter, cpu);
		kfree(c->number);
		kfree(c->bytes);
	}
	free_percpu(entry->counter);
	kfree(entry);
}

static inline struct ip_counter_entry *new()
{
	struct ip_counter_entry *entry = kzalloc(sizeof(ip_counter_entry),
		    GFP_KERNEL);
	struct nfs_counter_vector *c = NULL;
	if (entry == NULL) return NULL;
	entry->rulecount = NULL;
	entry->counter = NULL;
	entry->rulecount = kzalloc(sizeof(u8) * maxtype, GFP_KERNEL);
	entry->counter = alloc_percpu(struct nfs_counter_vector);
	if (entry->rulecount == NULL || entry->counter == NULL) {
		delete(entry);
		return NULL;
	}
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(entry->counter, cpu);
		c->number = kzalloc(sizeof(u64) * maxtype, GFP_KERNEL);
		c->bytes = kzalloc(sizeof(u64) * maxtype, GFP_KERNEL);  
		if (c->number == NULL || c->bytes == NULL) {
			delete(entry);
			return NULL;
		}
	}
	
	return entry;
}



void nfsinit(u8 max)
{
	if (max > 0) maxtype = max;
}
struct ip_counter_entry *findipentry(const nfs_ipaddr *ip)
{

	struct rb_node *node = NULL;
	struct ip_counter_entry *entry = NULL;
	int ret = 0;
	if (ip == NULL) return NULL;

	read_lock_irqsave(&iptreelock);
	node = iptree->rb_node;
	while(node) {
		entry = container_of(node, struct ip_counter_entry, node);
		ret = memcmp((void*) &entry->ip,(void*) ip, sizeof(nfs_ipaddr));
		if (ret < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else {
			read_unlock_irqrestore(&iptreelock);
			return entry;
		}
	}
	read_unlock_irqrestore(&iptreelock);
	return NULL;

}
bool addipentry(const nfs_ipaddr *ip)
{
	struct ip_counter_entry *entry = NULL;
	struct rb_node **newnode = NULL;
	struct rb_node *parent = NULL;
	int ret = 0;

	read_lock_irqsave(&iptreelock);
	newnode = &iptree->rb_node;
	while(*newnode) {
		entry = container_of(*newnode, struct ip_counter_entry, node);
		parent = *newnode;
		ret = memcmp(&entry->ip, ip, sizeof(nfs_ipaddr));
		if (ret < 0)
			newnode = & (*newnode)->rb_left;
		else if (ret > )
			newnode = & (*newnode)->rb_right;
		else {

			read_unlock_irqrestore(&iptreelock);
			return false;
		}
	}

	read_unlock_irqrestore(&iptreelock);
	entry = new();
	if (entry == NULL) return false;
	memcpy((void*)&entry->ip, (void*)ip, sizeof(nfs_ipaddr));
	
	write_lock_irqsave(&iptreelock);
	rb_link_node(&entry->node, parent, newnode);
	rb_insert_color(&entry->node, iptree);
	write_unlock_irqrestore(&iptreelock);
	return true;
}
inline bool rmvipentry(const nfs_ipaddr *ip)
{
	struct ip_counter_entry *entry = findipentry(ip);
	if (entry == NULL) return false;
	write_lock_irqsave(&iptreelock);
	rb_erase(&entry->node, &iptree);
	write_unlock_irqrestore(&iptreelock);

	delete(entry);
	return true;

}
inline void incrulecount(ip_counter_entry *entry, u8 typeidx)
{
	read_lock_irqsave(&iptreelock);
	entry->rulecout[typeidx]++;
	read_unlock_irqrestore(&iptreelock);
}
inline void decrulecount(ip_counter_entry *entry, u8 typeidx)
{
	read_lock_irqsave(&iptreelock);
	entry->rulecout[typeidx]--;
	read_unlock_irqrestore(&iptreelock);

}
inline void incpkgnumber(ip_counter_entry *entry, u8 typeidx)
{
	read_lock_irqsave(&iptreelock);
	get_cpu_ptr(entry->counter)->number[typeidx]++;
	put_cpu_ptr(entry->counter);
	read_unlock_irqrestore(&iptreelock);

}
inline void incpkgbytes(ip_counter_entry *entry, u8 typeidx, u64 bytes)
{
	read_lock_irqsave(&iptreelock);
	get_cpu_ptr(entry->counter)->bytes[typeidx] += bytes;
	put_cpu_ptr(entry->counter);
	read_unlock_irqrestore(&iptreelock);
}
void getcounter_ip(const struct ip_counter_entry *entry,
		struct nfs_counter_vector *vector)
{
	struct nfs_counter_vector *c = NULL;
	u8 i = 0;
	if (vector->number == NULL || vector->byets == NULL) return;

	read_lock_irqsave(&iptreelock);
	for_each_possible_cpu(cpu) {
		c = per_cpu_ptr(entry->counter, cpu);
		for (i = 0; i < maxtype){
			vector->number[i] += c->number[i];
			vector->bytes[i] += c->bytes[i];
		}
	}

	read_unlock_irqrestore(&iptreelock);
}


/*
alloc_percpu_gfp
alloc_percpu
this_cpu_ptr
get_cpu_var
  for_each_possible_cpu(cpu) {
                  struct perf_cpu_context *cpuctx;

                                  cpuctx = per_cpu_ptr(pmu->pmu_cpu_context, cpu);

  */

