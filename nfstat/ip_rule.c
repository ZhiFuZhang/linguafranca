/*
 * A structure to store the rules which the user want to stat.
 *
 * (C) 2015  Zhi Fu Zhang <zzfooo@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include <linux/rbtree.h>
#include <linux/seq_file.h>
#include <linux/slab.h>


#include "internal.h"

#define NAME_SIZE 40
struct nfs_rule_entry {
	struct rb_node node;
	struct nfs_rule rule;
	char name[NAME_SIZE];
};

static struct rb_root ruletree = RB_ROOT;
DEFINE_RWLOCK(ruletreelock);

static int ip_rule_show(struct seq_file *m, void *v)
{
	const struct nfs_rule_entry *entry = 
		(const struct nfs_rule_entry *)m->private;
	char buf[NFS_IPSTR] = {0};

	if (entry == NULL) return -1;
	seq_puts(m, entry->name);
	seq_putc(m, '\n');
	seq_printf(m, "\trule type %d\n", entry->rule.typeidx);
	nfs_ip2str(&entry->rule.lip, buf);
	seq_printf(m, "\tlocalip:%s", buf);
	nfs_port2str(entry->rule.lport, buf);	
	seq_printf(m, "\tlocalport:%s\n", buf);

	nfs_ip2str(&entry->rule.rip, buf);
	seq_printf(m, "\tremoteip:%s", buf);
	nfs_port2str(entry->rule.rport, buf);	
	seq_printf(m, "\tremoteport:%s\n", buf);
	if (entry->rule.protocol == 0) {
		seq_puts(m, "\tprotocal:ANY");
	} else {
		seq_printf(m, "\tprotocal:%d", entry->rule.protocol);
	}
	seq_puts(m, "\tdirection:");
	seq_puts(m, nfs_dir2str(entry->rule.dir));
	seq_putc(m, '\n');
	return 0;
}

static int ip_rule_single_open(struct inode *inode, struct file *file)
{
	return single_open(file, ip_rule_show, PDE_DATA(inode));
}


static inline void delete(struct nfs_rule_entry *entry)
{
	if (entry == NULL) return;
	remove_proc_entry(entry->name, rule_dir);
	kfree(entry);
}

static const struct file_operations ops = {
	.owner          = THIS_MODULE,
	.open           = ip_rule_single_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};
static inline struct nfs_rule_entry *create(void)
{

	static u32 ipruleid = 0;
	struct nfs_rule_entry *entry = kzalloc(sizeof(struct nfs_rule_entry),
		    GFP_KERNEL);
	int cpu = 0;
	if (entry == NULL) return entry;
	cpu = get_cpu();
	snprintf(entry->name, sizeof(entry->name), "%x_%x", ipruleid++, cpu);
	put_cpu();
	if (!proc_create_data(entry->name,0444, rule_dir, &ops, entry)){
		entry->name[0] = 0;
		delete(entry);
		return NULL;
	}

	return entry;
}

static inline int portcmp(u16 src, u16 dst)
{
	if (src != 0 && dst != 0){
		if (src > dst) {
			return 1;
		} else if (src < dst) {
			return -1;
		}
	}
	return 0;
}
static inline int ipcmp(const struct nfs_ipaddr *src,
		const struct nfs_ipaddr *dst)
{
	if (src->len != 0 && dst->len != 0) {
		if (src->len > dst->len) {
			return 1;
		} else if (src->len < dst->len) {
			return -1;
		} else {
			return memcmp(src->addr, dst->addr, src->len);
		}
	}
	return 0;
}
static int rulecmp(const struct nfs_rule *src, const struct nfs_rule *dst)
{
	int ret = 0;
	if (src->protocol > dst->protocol) {
		pr_debug("protocol >\n");
		return 1;
	} else if (src->protocol < dst ->protocol) {

		pr_debug("protocol <\n");
		return -1;
	} 
	if (src->dir != NFS_ALL && dst->dir != NFS_ALL)
	{
		if (src->dir > dst->dir) {
			pr_debug("dir >\n");
			return 1;
		} else if (src->dir < dst->dir) {
			pr_debug("dir <\n");
			return -1;
		}
	}
	ret = portcmp(src->lport, dst->lport);
	pr_debug("lport cmp\n");
	if (ret !=0) return ret;
	ret = portcmp(src->rport, dst->rport);
	pr_debug("rport cmp\n");
	if (ret !=0) return ret;

	pr_debug("lip cmp\n");
	ret = ipcmp(&src->lip, &dst->lip);
	if (ret !=0) return ret;
	pr_debug("rip cmp\n");
	ret = ipcmp(&src->rip, &dst->rip);
	if (ret !=0) return ret;
	return 0;
}

static struct nfs_rule_entry  *findnfsrule(const struct nfs_rule *rule) 
{
	struct rb_node *node = NULL;
	struct nfs_rule_entry *entry = NULL;
	int ret = 0;
	char buf[NFS_IPSTR] = {0};
	if (rule == NULL) return NULL;

	pr_debug("debug.find rule begin >>>>>>>>>>\n");
	pr_debug("debug.find rule, lip[%s]\n", 
				nfs_ip2str(&rule->lip, buf));
	pr_debug("debug.find rule, rip[%s]\n", 
				nfs_ip2str(&rule->rip, buf));
	pr_debug("debug.find rule, lport[%d]\n", rule->lport);
	pr_debug("debug.find rule, rport[%d]\n", rule->rport); 

	node = ruletree.rb_node;
	while(node) {
		entry = container_of(node, struct nfs_rule_entry, node);
		ret = rulecmp(rule, &entry->rule);
		if (ret < 0){
			node = node->rb_left;
		} else if (ret > 0) {
			node = node->rb_right;
		} else {

			pr_debug("debug.rule is here @@@\n");
			return entry;
		}
	}

	pr_debug("debug.rule not found\n");

	pr_debug("debug.find rule end <<<<<<<<\n");
	return NULL;
}
s16  get_typeidx(const struct nfs_rule *rule) 
{
	struct nfs_rule_entry *entry = NULL;
	unsigned long flags =0;
	s16 idx = -1;
	read_lock_irqsave(&ruletreelock, flags);
	entry = findnfsrule(rule);
	idx = (entry == NULL) ? -1 : entry->rule.typeidx;
	read_unlock_irqrestore(&ruletreelock, flags);
	/*for pr_debug function, it is determined at compiled time */
	pr_debug("debug.get_typeidx [%d]\n", idx);
	return idx;
}

bool addnfsrule(const struct nfs_rule *rule)
{
	struct nfs_rule_entry *entry = NULL;
	struct rb_node **newnode = NULL;
	struct rb_node *parent = NULL;
	int ret = 0;

	unsigned long flags =0;
	struct nfs_rule_entry *newentry = create();
	if (newentry == NULL) return false;
	memcpy((void *)&newentry->rule, (void *)rule, sizeof(struct nfs_rule));
	pr_info("add ip rule for type[%d]\n", rule->typeidx);

	write_lock_irqsave(&ruletreelock, flags);
	newnode = &ruletree.rb_node;

	while(*newnode) {
		entry = container_of(*newnode, struct nfs_rule_entry, node);
		parent = *newnode;
		ret = rulecmp(rule, &entry->rule);
		if (ret < 0) {
			newnode = &(*newnode)->rb_left;
		} else if (ret > 0) {
			newnode = &(*newnode)->rb_right;
		} else {
			write_unlock_irqrestore(&ruletreelock, flags);
			delete(newentry);
			return false;
		}
	}

	rb_link_node(&newentry->node, parent, newnode);
	rb_insert_color(&newentry->node, &ruletree);
	write_unlock_irqrestore(&ruletreelock, flags);
	return true;
}

inline bool rmvnfsrule(const struct nfs_rule *rule)
{
	struct nfs_rule_entry *entry = NULL;
	unsigned long flags =0;

	write_lock_irqsave(&ruletreelock, flags);
	entry = findnfsrule(rule);
	if (entry == NULL){

		write_unlock_irqrestore(&ruletreelock, flags);
	       	return false;
	}
	rb_erase(&entry->node, &ruletree);
	write_unlock_irqrestore(&ruletreelock, flags);
	
	pr_info("rmv ip rule for type[%d]\n", entry->rule.typeidx);
	delete(entry);
	return true;

}

void clear_nfsrule(void)
{
	struct nfs_rule_entry *entry = NULL;
	struct nfs_rule_entry *node = NULL;
	unsigned long flags = 0;
	write_lock_irqsave(&ruletreelock, flags);
	nfs_rbtree_postorder_for_each_entry_safe(entry, node, &ruletree, node){
		delete(entry);
	}
	write_unlock_irqrestore(&ruletreelock, flags);
}


