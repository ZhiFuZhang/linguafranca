#include <linux/dcache.h>
#include <linux/hash.h>
#include <linux/jhash.h>
#include <linux/kernel.h>
#include "intern.h"

#define DEVNAME_HASH_BITS  8
#define DEVNAME_HASH_SIZE  (1 << DEVNAME_HASH_BITS)
#define DEV_NAME_HASH_INITVAL 19830409

static struct hlist_head *dev_hashtable = NULL;
static int white_black = IPS_NONE;
static u32 small_jhash(u32 hash)
{
	return hash & (( 1<< DEVNAME_HASH_BITS) - 1);
}

int devset_init(void)
{
	int i = 0;
	struct hlist_head *hash = kmalloc(sizeof(*hash) * DEVNAME_HASH_SIZE,GFP_KERNEL);
	dev_hashtable = hash;
	if (hash == NULL) return -ENOMEM;
	for (i = 0; i < DEVNAME_HASH_SIZE; i++)	{
		INIT_HLIST_HEAD(&hash[i]);
	}
	return 0;
}
static u32 dev_hash(const char *devname)
{
	u32 h = jhash(devname, IFNAMSIZ, DEV_NAME_HASH_INITVAL);
	return h;
}
/* devname must have a space whose size is IFNAMSIZ*/
static struct hlist_head *dev_hashhead(u32 h)
{
	h =  small_jhash(h);
	return &dev_hashtable[h];
}

static struct dev_entry *get_entry_in_hlist(const char * name, u32 hash,
	       	struct hlist_head *head)
{

	struct dev_entry *pos = NULL;
	hlist_for_each_entry(pos, head, node){
		if ((hash == pos->hash)
			&& (strncmp(pos->n.name, name, IFNAMSIZ) == 0)){
			return pos;
		}
	}
	return NULL;
}
/* the pointer in l shall be a kernel space address */
int devset_add(const struct devname_list *l)
{
	int ii = 0;
	struct hlist_head *head = NULL;
	struct dev_entry *newnode = NULL;
	char *name = NULL;
	u32 hash = 0;

	if (unlikely(dev_hashtable == NULL)){
		pr_err(IPS"devset is not initailed yet\n");       
		return -ENOMEM;
	}
	if (unlikely(white_black != IPS_NONE)) return -EEXIST;
	if (unlikely(l == NULL)) return -EFAULT;
	if (unlikely(l->namelist == NULL)) return -EFAULT;
	if (unlikely(l->white_black == IPS_NONE)) return -ENOEXEC;

	white_black = l->white_black;
	for (ii = 0; ii < l->devnum; ii++) {
		name = l->namelist[ii].name;
		hash = dev_hash(name);
		head = dev_hashhead(hash);
		if (unlikely(get_entry_in_hlist(name, hash, head) != NULL)) {
			pr_err(IPS"devset_add,entry exist in hashtable,"
					"ignore it, [%s]\n", name);
			continue;
		}
		newnode = kmalloc(sizeof(struct dev_entry), GFP_KERNEL);
		if (newnode == NULL) {
			pr_err(IPS"devset_add,kmemory failed,"
					"ignore it, [%s]\n", name);
			continue;
		}
		newnode->hash =hash;
		memcpy(&newnode->n.name, name, IFNAMSIZ);
		INIT_HLIST_NODE(&newnode->node);
		hlist_add_head(&newnode->node, head);
	}
	return 0;
}


bool devset_ignore(const char *devname)
{
	u32 hash = dev_hash(devname);
	struct hlist_head *head = dev_hashhead(hash);
	if (get_entry_in_hlist(devname, hash, head)) {
		pr_debug(IPS"dev in the list\n");
		return (white_black == IPS_BLACK);
	}  else {

		pr_debug(IPS"dev not in the list\n");
		return (white_black == IPS_WHITE);
	}
}

void devset_exit(void)
{
	struct dev_entry *pos = NULL;
	struct hlist_node *n = NULL;
	struct hlist_head *hash = dev_hashtable;
	int i = 0;
	for (i = 0; i < DEVNAME_HASH_SIZE; i++)	{
		hlist_for_each_entry_safe(pos, n, &hash[i], node) {
			hlist_del(&pos->node);
			kfree(pos);
		}
		INIT_HLIST_HEAD(&hash[i]);
	}
	kfree(hash);
	dev_hashtable = NULL;
	white_black = IPS_NONE;
}

#ifdef  UNITTEST
static int __rainy devset_add_test1(void) {
	int s = devset_add(NULL);
	if (s ==  -EFAULT) return 0;
	return 1;
}

static int __rainy devset_add_test2(void) {
	int s = 0;
	struct devname_list l = { 0 };
	s = devset_add(&l);
	if (s ==  -EFAULT) return 0;
	return 1;

}

static int __rainy devset_add_test3(void) {
	int s = 0;
	struct devname_list l = { 0 };
	struct devname n[0];
	l.namelist = &n[0];
	l.white_black = IPS_NONE;
	s = devset_add(&l);
	if (s ==  -ENOEXEC) return 0;
	return 1;
}
static int __rainy devset_add_test4(void) {
	int s = 0;
	struct devname_list l = { 0 };
	struct devname n[0];
	l.namelist = &n[0];
	l.white_black = IPS_WHITE;
	white_black = IPS_BLACK;
	s = devset_add(&l);
	white_black = IPS_NONE;
	if (s == -EEXIST) return 0;
	return 1;

}
static int __sunny devset_add_test5(void) {
	int s = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = {"eth195"},
	};
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_WHITE;
	s = devset_add(&l);
	if (s != 0) return 1;
	if (!devset_ignore(n.name)) return 0;
	return 1;
}
static int __sunny devset_add_test6(void) {
	int s = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = "eth295",
	};
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_BLACK;
	s = devset_add(&l);
	if (s != 0) return 1;
	if (devset_ignore(n.name)) return 0;
	return 1;
}

static int __sunny devset_add_test7(void) {
	int s = 0;
	struct devname_list l = { 0 };
	struct devname n[] = {
		[0] = {
			.name = "eth295",
		},
		[1] = {
			.name = "eth296",
		},
		[2] = {
			.name = "eth285",
		},
		[3] = {
			.name = "eth275",
		},


	};
	l.namelist = &n[0];
	l.devnum = sizeof(n)/sizeof(struct devname);
	l.white_black = IPS_BLACK;
	s = devset_add(&l);
	if (s != 0) return 1;
	if (devset_ignore(n[2].name)) return 0;
	return 1;
}

static int __sunny devset_ignore_test8(void) {

	if (devset_ignore("hello")) return 1;
	return 0;
}

static int __sunny devset_ignore_test9(void) {
	white_black = IPS_WHITE;
	if (devset_ignore("hello")) return 0;
	return 1;
}
static int __sunny devset_exit_test10(void) {
	if (white_black == IPS_NONE && dev_hashtable == NULL) 
		return 0;
	return 1;
}
struct ut_result devset_ut()
{
	int s = 0;
	struct ut_result r = {0,0};
	struct ut_result crtical_fail = {0xffff, 0xffff};
	pr_info(IPS"devset_ut started\n");
	s = devset_init();
	if (s != 0) return crtical_fail;
	runtest(r, devset_add_test1);
	runtest(r, devset_add_test2);
	runtest(r, devset_add_test3);
	runtest(r, devset_add_test4);
	runtest(r, devset_add_test5);

	devset_exit();
	s = devset_init();
	if (s != 0) return crtical_fail;
	runtest(r, devset_add_test6);

	devset_exit();
	s = devset_init();
	if (s != 0) return crtical_fail;
	runtest(r, devset_add_test7);
	runtest(r, devset_ignore_test8);
	runtest(r, devset_ignore_test9);
	
	devset_exit();
	runtest(r, devset_exit_test10);
	pr_info(IPS"devset_ut completed\n");
	return r;
}

#endif
