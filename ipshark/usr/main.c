#include "ips_api.h"
#include "framework.h"
#include <time.h>
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

static int test3()
{
	int err = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = {"lo"},
	};
	struct ip_key_info_set s = { 0 };
	struct ip_key_info info[512];
	int i = 0;
	int j = 0;
	err = ips_init();
	if(err) goto fail; 
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_BLACK;
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

static int test4()
{
	int err = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = {"lo"},
	};
	struct ip_key_info_set s = { 0 };
	struct ip_key_info info[512];
	int i = 0;
	int j = 0;

	err = ips_init();
	if(err) goto fail; 
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_BLACK;
	err = ips_config(&l);
	if (err) goto fail;
	err = ips_config(&l);
	if (!err) goto fail;
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

	err = ips_config(&l);
	if (!err) goto fail;
	err = ips_init();
	if(!err) goto fail; 
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
	
static void simple_ut()
{

	int total = 0;
	int fail = 0;
	runtest(test1);
	runtest(test2);
	runtest(test3);
	runtest(test4);
	printf("######################test is done, %d/%d\n", fail, total);
}
static void handle(struct ip_key_info_set s) {
	int j = 0;
	printf("s.n = [%d]\n", s.n);
	for (j = 0; j < s.n; j++) {
		objdump((const char *)&s.array[j],
				sizeof (struct ip_key_info));
	}
	ips_frame_destroy(&s);
}
static void handle2(struct ip_key_info_set s) {
	static unsigned int t = 0;
	static unsigned c = 0;

	if (t > 0xffff) {
		printf("pkts[%u]\n", t);
		t = 0;
	}
	if (c > 100000) {
		printf("c [%d], [%ld]\n", c, time(NULL));
		c = 0;
	}
	c++;
	t += s.n;
	ips_frame_destroy(&s);
}

static int ftest1() {
	int err = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = {"lo"},
	};
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_BLACK;
	ips_frame_register(handle, 0);
	err = ips_frame_start(&l);
	if (err) return err;
	sleep(10);
	ips_frame_stop();

	return err;
}
static int pftest(int a) {
	int err = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = {"lo"},
	};
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_BLACK;
	ips_frame_register(handle2, 0);
	err = ips_frame_start(&l);
	if (err) return err;
	sleep(a);
	ips_frame_stop();

	return err;
}

static int ftest2() {
	int err = 0;
	struct devname_list l = { 0 };
	struct devname n = {
		.name = {"lo"},
	};
	l.namelist = &n;
	l.devnum = 1;
	l.white_black = IPS_BLACK;
	ips_frame_register(handle, 1);
	err = ips_frame_start(&l);
	if (err) return err;
	sleep(5);

	err = ips_frame_start(&l);
	if (!err) return err;
	err = 0;
	sleep(5);
	ips_frame_stop();
	return err;
}
static void framework_ut() {
	int total = 0;
	int fail = 0;
	runtest(ftest1);
	runtest(ftest2);
	printf("#################ftest is done, %d/%d\n", fail, total);

}

int main(int argc, char **argv)
{
	int err = 0;
	int a = 0;
	simple_ut();
	framework_ut(); 
	if (argc >= 2) {
		a = atoi(argv[1]);
		a = (a < 10) ? 10 : a;
		pftest(a);
	}
	return err;
}
