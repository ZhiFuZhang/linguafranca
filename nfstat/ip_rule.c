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

struct nfs_rule_entry {
	struct rb_node node;
	struct nfs_rule rule;
};

static struct rb_root ruletree = RB_ROOT;
DEFINE_RWLOCK(ruletreelock);

static inline struct nfs_rule_entry *new()
{
	struct nfs_rule_entry *entry = kzalloc(sizeof(struct nfs_rule_entry),
		    GFP_KERNEL);
	return entry;
}

static inline void delete(struct nfs_rule_entry *entry)
{
	kfree(entry);
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
		return memcmp(src, dst, sizeof(struct nfs_ipaddr));
	}
	return 0;
}
static int rulecmp(const nfs_rule *src, const nfs_rule *dst)
{
	int ret = 0;
	if (src->protocol > dst->protocol) {
		return 1;
	} else if (src->protocol < dst ->protocol) {
		return -1;
	} 
	if (src->dir > dst->dir) {
		return 1;
	} else if (src->dir < dst->dir) {
		return -1;
	}
	ret = portcmp(src->lport; dst->lport);
	if (ret !=0) return ret;
	ret = portcmp(src->rport, dst->rport);
	if (ret !=0) return ret;
	ret = ipcmp(src->lip, dst->lip);
	if (ret !=0) return ret;
	ret = ipcmp(src->rip, dst->rip);
	if (ret !=0) return ret;
	return 0;
}

s16  findnfsrule(const struct nfs_rule *rule) 
{
	struct rb_node *node = NULL;
	struct rb_rule_entry *entry = NULL;
	int ret = 0;
	if (rule == NULL) return NULL;
	read_lock_irqsave(&ruletreelock);
	node = ruletree.rb_node;
	while(node) {
		entry = cntainer_of(node, struct nfs_rule_entry, node);
		ret = rulecmp(&entry->rule, rule);
		if (ret > 0){
			node = node->rb_left;
		} else if (ret < 0) {
			node = node->rb_right;
		} else {
			read_lock_irqrestore(&ruletreelock);
			return entry->rule.typeidx;
		}
	}
	read_lock_irqrestore(&ruletreelock);
	return -1;
}

bool addnfsrule(const struct nfs_rule *rule)
{
	struct nfs_rule_entry *entry = NULL;
	struct rb_node **newnode = NULL;
	struct rb_node *parent = NULL;
	int ret = 0;

	struct nfs_rule_entry *newentry = new();
	if (newentry == NULL) return false;
	memcpy((void *)&newentry->rule, (void *)rule, sizeof(struct nfs_rule));

	write_lock_irqsave(&ruletreelock);
	newnode = &ruletree.rb_node;

	while(*newnode) {
		entry = container_of(*newnode, struct nfs_rule_entry, node);
		parent = *newnode;
		ret = rulcmp(&entry->rule, rule);
		if (ret > 0) {
			newnode = &(*newnode)->rb_left;
		} else if (ret < 0) {
			newnode = &(*newnode)->rb_right;
		} else {
			write_lock_irqrestore(&ruletreelock);
			delete(newentry);
			return false;
		}
	}

	rb_link_node(&newentry->node, parent, newnode);
	rb_insert_color(&newentry->node, &ruletree);
	write_lock_irqrestore(&ruletreelock);
	return true;
}

inline bool rmvnfsrule(const struct nfs_rule *rule)
{
	struct nfs_rule_entry *entry = NULL;
	if (entry == NULL) return false;
	write_lock_irqsave(&ruletreelock);
	rb_erase(&entry->node, &ruletree);
	write_unlock_irqrestore(&ruletreelock);
	
	delete(entry);
	return true;

}
