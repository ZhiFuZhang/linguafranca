#ifndef __UT__H__
#define __UT__H__
#include "../intern.h"
#ifdef IPS
#undef IPS
#define IPS "ipshark2_ut:"
#endif

#define __rainy 
#define __sunny
#define runtest(r, fun)	  \
{				  \
	int i_runtest__t = fun(); \
	r.total++;		  \
	if (i_runtest__t != 0) {  \
		r.fail++;	  \
		pr_info(IPS"testcase:%s failed, fail/total %d/%d@@\n", \
		   #fun, r.fail, r.total);\
	} else	{		  \
		pr_debug(IPS"\ttestcase:%s passed \n", #fun); \
	}			 \
}
struct ut_result{
	int total;
	int fail;
};

#define runtestsuite(fun)			\
{						\
	struct ut_result __r_ut = fun();	\
	pr_info(IPS"#testsuite#:%s test result, fail/total %d/%d\n", \
			#fun, __r_ut.fail, __r_ut.total);	\
}



#endif

