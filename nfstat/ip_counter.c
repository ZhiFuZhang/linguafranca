/*
 * A structure to store packet counters for each ip.
 *
 * (C) 2015  Zhi Fu Zhang <zzfooo@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 */

#include <linux/kernel.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include <linux/rbtree.h>

#include "internal.h"

struct nfs_counter_vector {
	u64 *number;
	u64 *bytes;
};

struct ip_counter_entry {
	struct rb_node node;
	nfs_ipaddr ip;/*0:addlen, 1~17 addr */
	struct nfs_counter_vector *counter
};

static struct rb_root iptree = RB_ROOT;
/*only when the iptree insert/delete, it is write mode */
DEFINE_RWLOCK(iptreelock);
static u32 ipnumber = 0;
static u8 maxtype = 8;

static inline void delete(struct ip_counter_entry *entry)
{

	struct nfs_counter_vector *c = NULL;
	if (entry == NULL) return;
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
	struct ip_counter_entry *entry = kzalloc(sizeof(struct ip_counter_entry),
		    GFP_KERNEL);
	struct nfs_counter_vector *c = NULL;
	if (entry == NULL) return NULL;
	entry->counter = NULL;
	entry->counter = alloc_percpu(struct nfs_counter_vector);
	if (entry->counter == NULL) {
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
inline u8 nfstypesize()
{
	return maxtype;
}
static struct ip_counter_entry *findipentry(const nfs_ipaddr *ip)
{
	struct rb_node *node = NULL;
	struct ip_counter_entry *entry = NULL;
	int ret = 0;
	if (ip == NULL) return NULL;

	node = iptree.rb_node;
	while(node) {
		entry = container_of(node, struct ip_counter_entry, node);
		ret = memcmp((void*) &entry->ip,(void*) ip, sizeof(nfs_ipaddr));
		if (ret > 0)
			node = node->rb_left;
		else if (result < 0)
			node = node->rb_right;
		else {
			return entry;
		}
	}
	return NULL;

}

/* addipentry is not threaded */
bool addipentry(const nfs_ipaddr *ip)
{
	struct ip_counter_entry *entry = NULL;
	struct rb_node **newnode = NULL;
	struct rb_node *parent = NULL;
	int ret = 0;
	struct ip_counter_entry *newentry = new();
	if (newentry == NULL) return false;
	memcpy((void*)&newentry->ip, (void*)ip, sizeof(nfs_ipaddr));
	
	write_lock_irqsave(&iptreelock);
	newnode = &iptree.rb_node;
	while(*newnode) {
		entry = container_of(*newnode, struct ip_counter_entry, node);
		parent = *newnode;
		ret = memcmp(&entry->ip, ip, sizeof(nfs_ipaddr));
		if (ret > 0)
			newnode = & (*newnode)->rb_left;
		else if (ret < 0)
			newnode = & (*newnode)->rb_right;
		else {

			write_unlock_irqrestore(&iptreelock);
			delete(newentry);
			return false;
		}
	}

	rb_link_node(&newentry->node, parent, newnode);
	rb_insert_color(&newentry->node, &iptree);
	ipnumber++;
	write_unlock_irqrestore(&iptreelock);
	return true;
}
inline bool rmvipentry(const nfs_ipaddr *ip)
{
	struct ip_counter_entry *entry = NULL

	write_lock_irqsave(&iptreelock);
	entry = findipentry(ip);
	if (entry == NULL) {

		write_unlock_irqrestore(&iptreelock);
		return false;
	}
	rb_erase(&entry->node, &iptree);
	ipnumber--;
	write_unlock_irqrestore(&iptreelock);

	delete(entry);
	return true;

}

inline void inccounter(struct nfs_ipaddr *ip, u8 typeidx, u64 bytes);
{
	
	struct ip_counter_entry *entry = NULL
	struct nfs_counter_vector *vector = NULL;
	read_lock_irqsave(&iptreelock);

	entry = findipentry(ip);
	if (entry == NULL) {
		read_unlock_irqrestore(&iptreelock);
		return false;
	}
	vector = get_cpu_ptr(entry->counter);
	vector->number[typeidx]++;
	vector->bytes[typeidx] += bytes;
	put_cpu_ptr(entry->counter);

	read_unlock_irqrestore(&iptreelock);

}

#define copy_counter(buf, data, len)			\
{							\
	ret = memcpy(buf, data, len);		\
	if (ret != 0){					\
		read_unlock_irqrestore(&iptreelock);	\
		return -2;				\
	}						\
	buf = buf_+ len;				\
}


int readcounter(char  *buf, size_t len)
{
	struct ip_counter_entry *entry, *node;
	struct nfs_counter_vector *c = NULL;
	u8 i = 0;
	u64 number = 0;
	u64 bytes = 0;
	char *begin = buf;
	int ret = 0;

	read_lock_irqsave(&iptreelock);
	if (len < ipnumber * (sizeof(nfs_ipaddr)
			       	+ sizeof(u64) * 2 * maxtype)) {

		read_unlock_irqrestore(&iptreelock);
		return -1;

	}
	rbtree_postorder_for_each_entry_safe(entry, node, iptree, node) {
		copy_counter(buf, &entry->ip, sizeof(nfs_ipaddr));
		for (i = 0; i < maxtype) {
			number = 0;
			bytes = 0;
			for_each_possible_cpu(cpu) {
				c = per_cpu_ptr(entry->counter, cpu);
				number += c->number[i];
				bytes += c->bytes[i];
			}
			copy_counter(buf, &number, sizeof(u64));
			copy_counter(buf, &bytes, sizeof(u64));
		}
	}
	read_unlock_irqrestore(&iptreelock);
	return buf - begin;
}

