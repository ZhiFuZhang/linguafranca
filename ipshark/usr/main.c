#include "ips_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/errno.h>

static void objdump(const char *s, size_t len)
{
	int i = 0;
	printf("begin\n");
	for (i = 0; i < len; i++) {
		printf("%02x ", (unsigned char)s[i]);
		if ((i+1) % 16 == 0) {
			printf("\n");
		}
	}
	printf("\nend\n");
}
static int test1()
{
	int err = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = {"enp0s3"},
	};
	struct ip_key_info_set s = { 0 };
	struct ip_key_info info[512];
	int i = 0;
	int j = 0;
	err = ips_init();
	if(err) goto fail; 
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_WHITE;
	err = ips_config(&l);
	if (err) goto fail;
	s.n = 512;
	s.array = info;
	for (i = 0; i < 10; i++) {
		err = ips_fetch(&s);
		if (err && s.n == 0) {
			err = 0;
		} else {
			if (s.n <= 0) { 
				err = -1;
			}
			if (err) goto fail;
			
			for (j = 0; j < s.n; j++) {
				objdump((const char *)&s.array[j],
						sizeof (struct ip_key_info));
			}
			printf("get it [%d], [%d]\n", i, s.n);
		}

		if (err) goto fail;
		s.n = 512; 
		sleep(1);
	}

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
	int t = 10;
	if (argc < 2){
		printf("need more argument\n");
		return -1;
	}
	if (argc >= 3) {
		c = atoi(argv[2]);
		if (c > 1) t = c;
	}

	c = atoi(argv[1]);
	if (c == 1) {
		simple_ut();
		sleep(t);

	} else {
		ips_init();
		sleep(t);
		ips_exit();
	}
	return err;
}
