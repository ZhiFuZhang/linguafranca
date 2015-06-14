#include "ips_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <linux/errno.h>
static int test1()
{
	int err = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = {"eth195"},
	};
	struct ip_key_info_set s = { 0 };
	struct ip_key_info info[3];
	err = ips_init();
	if(err) goto fail; 
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_WHITE;
	err = ips_config(&l);
	if (err) goto fail;
	s.n = 3;
	s.array = info;
	err = ips_fetch(&s);
	if (err && s.n == 0)  err = 0;

fail:
	ips_exit();
	return err;
}

static int test2()
{
	int err = 0;
	err = ips_init();
	if(err) goto fail; 
	
	err = ips_init();
	if (err == -EEXIST) err = 0;

fail:
	ips_exit();
	return err;
}

static void simple_ut()
{
#define runtest(f)	\
{			\
	int err = f();	\
	total++;	\
	if (err){	\
		fail++;	\
		printf("%s,%dfailed,%d/%d\n", #f,err,fail,total);	\
	} else {	\
			\
		printf("%s,passed\n", #f);			\
	}		\
}			\
	
	int total = 0;
	int fail = 0;
	runtest(test1);
	runtest(test2);

	printf("test is done, %d/%d\n", fail, total);
}

int main(int argc, char **argv)
{
	int err = 0;
	int c = 0;
	if (argc < 2){
		printf("need more argument\n");
		return -1;
	}
	c = atoi(argv[1]);
	if (c == 1) {
		simple_ut();
	} else {
		ips_init();
		ips_exit();
	}
	return err;
}
