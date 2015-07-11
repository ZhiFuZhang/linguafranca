#ifndef __DEVSET_H__
#define  __DEVSET_H__
#include "intern.h"

void *__devset_create(bool black, int elenum, int size);
void __devset_order(void);
bool devset_ignore(char *s);
void devset_exit(void);

#define devset_init(l)							\
({									\
	int err = 0;							\
	int len = 0;							\
	bool black = true;						\
	void *kp = NULL;						\
	do {								\
		err = get_user(black, &l->white_black);			\
		if (err) break;					\
		err = get_user(len, &l->devnum);			\
		if (err) break;					\
		kp = __devset_create(black, len, sizeof(l->devlist[0]));\
		if (kp == NULL) break;				\
		err = copy_from_user(kp, &l->devlist,			\
			len * sizeof(l->devlist[0]));			\
		if (err) break;					\
		__devset_order();					\
	} while(0);							\
	if (err) devset_exit();						\
	err;								\
})

#endif
