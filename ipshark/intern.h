#ifndef __INTERN_H__HEAD
#define __INTERN_H__HEAD

#include "ips.h"
#define IPS "ipshark:"

struct dev_entry{
	struct devname n;
	u32 hash;
	struct hlist_node node;
};

int devset_init(void);
void devset_exit(void);
int devset_add(const struct devname_list __kernel *l);
bool devset_ignore(const char *devname);

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
#else
#define devset_ut {0, 0}
#endif

#endif
