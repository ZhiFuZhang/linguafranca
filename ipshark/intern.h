
#define IPS "ipshark:"



#ifdef  UNITTEST
#undef IPS
#define IPS "ipshark_ut:"
/* return faile test case num */
int devset_ut(void);
#else
#define devset_ut 0
#endif
