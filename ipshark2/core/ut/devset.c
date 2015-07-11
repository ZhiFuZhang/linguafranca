#include "../devset.c"
#include "ut.h"

static int __sunny devset_exit_test(void)
{
	devset_exit();
	if (data.name == NULL && data.hash == NULL && !data.black
			&& data.elenum == 0 && data.size == 0)
		return 0;
	pr_info(IPS"black %d, elenum %d, size %d, hash %p, name %p\n",
			data.black, data.elenum, data.size,
			data.hash, data.name);
	return 1;
}
static int __sunny devset_create_test1(void)
{
	void *p = __devset_create(false, 5, 10);
	if (p != data.name) return 1;
	if (p == NULL){
		pr_err(IPS"create failed %d\n", __LINE__);
		return 1;
	}
	if ((!data.black) && data.elenum == 5 && data.size == 10
			&& data.hash != NULL)
		return 0;
	pr_info(IPS"black %d, elenum %d, size %d, hash %p, name %p\n",
			data.black, data.elenum, data.size,
			data.hash, data.name);

	return 1;
}
static int __sunny devset_create_test2(void)
{
	void *p =  NULL;
	if (devset_exit_test() != 0) return 1;
	p = __devset_create(true, 3, 16);
	if (p != data.name) return 1;
	if (p == NULL){
		pr_err(IPS"create failed %d\n", __LINE__);
		return 1;
	}
	if (data.black && data.elenum == 3 && data.size == 16
			&& data.hash != NULL)
		return 0;
	pr_info(IPS"black %d, elenum %d, size %d, hash %p, name %p\n",
			data.black, data.elenum, data.size,
			data.hash, data.name);

	return 1;
}

static int __sunny devset_create_test3(void)
{
	void *p =  NULL;
	if (devset_exit_test() != 0) return 1;
	p = __devset_create(true, 0, 17);
	if (p != data.name) return 1;
	if (p == NULL){
		pr_err(IPS"create failed %d\n", __LINE__);
		return 1;
	}
	if (data.black && data.elenum == 0 && data.size == 17
			&& data.hash != NULL)
		return 0;
	pr_info(IPS"black %d, elenum %d, size %d, hash %p, name %p\n",
			data.black, data.elenum, data.size,
			data.hash, data.name);
	return 1;
}
static int __rainy devset_create_test4(void)
{
	void *p =  NULL;
	if (devset_exit_test() != 0) return 1;
	p = __devset_create(true, -1, 17);
	if (p != data.name) return 1;
	if (data.name == NULL && data.hash == NULL && !data.black
			&& data.elenum == 0 && data.size == 0)
		return 0;

	return 1;
}
static int __rainy devset_create_test5(void)
{
	void *p =  NULL;
	if (devset_exit_test() != 0) return 1;
	p = __devset_create(true, 5, 0);
	if (p != data.name) return 1;
	if (data.name == NULL && data.hash == NULL && !data.black
			&& data.elenum == 0 && data.size == 0)
		return 0;

	return 1;
}
static int __sunny devset_order_test1(void)
{
	char *p = NULL;
	int i = 0;
	u32 old = 0;
	int r = 0;
	if (devset_exit_test() != 0) return 1;
	p = __devset_create(false, 15, 20);
	for (i = 0; i < 15; i++){
		p[0] = 'a' + i;
		p[1] = '0' + i%10;
		p[2] = 'A' + i;
		p[3] = 0;
		pr_info(IPS"%s\n", p);
		p += 20;
	}
	__devset_order();
	old = data.hash[0];

	p = data.name;
	for (i = 0; i < 15; i++){
		pr_debug(IPS"%u,%s\n", data.hash[i], p);
		if (data.hash[i] < old) r = 1;
		if (data.hash[i] != jhash(p, strlen(p), DEV_NAME_HASH_INITVAL))
				r = 1;
		p += 20;
		old = data.hash[i];
	}
	p = data.name;
	return r;
}
/* dependon devset_order_test1 */
static int devset_ignore_test1(void)
{
	bool i = false;
	i = devset_ignore("a0A");
	if (i) return 1;
	i = devset_ignore("j9J");
	if (i) return 1;
	i = devset_ignore("a1A");
	if (!i) return 1;
	i = devset_ignore("0123456789");
	if (!i) return 1;
	return 0;
}
static int __sunny devset_order_test2(void)
{
	char *p = NULL;
	int i = 0;
	u32 old = 0;
	int r = 0;
	if (devset_exit_test() != 0) return 1;
	p = __devset_create(true, 16, 20);
	for (i = 0; i < 16; i++){
		p[0] = 'A' + i;
		p[1] = '0' + i;
		p[2] = 'a' + i;
		p[3] = 0;
		pr_info(IPS"%s\n", p);
		p += 20;
	}
	__devset_order();
	old = data.hash[0];

	p = data.name;
	for (i = 0; i < 16; i++){
		pr_debug(IPS"%u,%s\n", data.hash[i], p);
		if (data.hash[i] < old) r = 1;
		if (data.hash[i] != jhash(p, strlen(p), DEV_NAME_HASH_INITVAL))
				r = 1;
		p += 20;
		old = data.hash[i];
	}
	p = data.name;
	return r;
}
/* dependon devset_order_test2 */
static int devset_ignore_test2(void)
{
	bool i = false;
	i = devset_ignore("B1b");
	if (!i) return 1;
	i = devset_ignore("H7h");
	if (!i) return 1;
	i = devset_ignore("c1A");
	if (i) return 1;
	i = devset_ignore("123456789");
	if (i) return 1;
	return 0;
}


struct ut_result devset_ut(void)
{
	struct ut_result r = {0, 0};
	pr_info(IPS"devset_ut started.\n");
	runtest(r, devset_create_test1);
	runtest(r, devset_exit_test);
	runtest(r, devset_create_test2);
	runtest(r, devset_create_test3);
	runtest(r, devset_create_test4);
	runtest(r, devset_create_test5);
	runtest(r, devset_order_test1);
	runtest(r, devset_ignore_test1);
	runtest(r, devset_order_test2);
	runtest(r, devset_ignore_test2);
	runtest(r, devset_exit_test);
	pr_info(IPS"devset_ut finished.\n");
	return r;
}

