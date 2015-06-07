#ifndef __INTERN_H__HEAD
#define __INTERN_H__HEAD

#include "ips.h"
#define IPS "ipshark:"

struct dev_entry{
	struct devname n;
	u32 hash;
	struct hlist_node node;
};
struct ip_key_info_set{
	size_t n;
	union {
		struct ip_key_info *array;
		char *buf;
	};
};
int devset_init(void);
void devset_exit(void);
int devset_add(const struct devname_list __kernel *l);
bool devset_ignore(const char *devname);

int ip_queue_init(void);
int ip_queue_put(const struct ip_key_info *info);
void ip_queue_wake_up(void);
long ip_queue_wait(void);
void ip_queue_get(struct ip_key_info_set *s);
void ip_queue_exit(void);

#ifdef  UNITTEST
#undef IPS
#define IPS "ipshark_ut:"

/* test case */
#define __rainy 
#define __sunny
#define runtest(r, fun)	  \
{				  \
	int i_runtest__t = fun(); \
	r.total++;		  \
	if (i_runtest__t != 0) {  \
		r.fail++;	  \
		pr_info(IPS"testcase:%s failed, fail/total %d/%d\n", \
		   #fun, r.fail, r.total);\
	} else	{		  \
		pr_debug(IPS"testcase:%s passed \n", #fun); \
	}			 \
}
struct ut_result{
	int total;
	int fail;
};

#define runtestsuite(fun)			\
{						\
	struct ut_result __r_ut = fun();	\
	pr_info(IPS"testsuite:%s test result, fail/total %d/%d\n", \
			#fun, __r_ut.fail, __r_ut.total);	\
}

/* return faile test case num */
struct ut_result devset_ut(void);
struct ut_result ip_queue_ut(void);
#else
#define devset_ut {0, 0}
#endif

#endif
