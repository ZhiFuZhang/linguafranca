#ifndef __DEVSET_H__
#define  __DEVSET_H__
#include "intern.h"

void *__devset_create(bool black, int elenum, int size);
void __devset_order(void);
bool devset_ignore(const char *s);
void devset_exit(void);

#define devset_init(c, userpointer)					\
({									\
	struct ips_config *l = c;					\
	int err = 0;							\
	int len = 0;							\
	bool black = true;						\
	void *kp = NULL;						\
	do {								\
		black = l->white_black;					\
		len = l->devnum;					\
		kp = __devset_create(black, len, sizeof(l->devlist[0]));\
		if (kp == NULL){ err = -1; break;}			\
		err = copy_from_user(kp, userpointer->devlist,		\
				len * sizeof(l->devlist[0]));		\
		if (err) break;						\
		__devset_order();					\
	} while(0);							\
	if (err) devset_exit();						\
	err;								\
})

#endif
