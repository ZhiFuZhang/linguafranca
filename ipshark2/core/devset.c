#include <linux/jhash.h>
#include  "devset.h"
#define DEV_NAME_HASH_INITVAL 59
static struct {
	bool black;
	int elenum;
	int size;
	char *name;
	u32 *hash;

}data = {
	.black = false,
	.elenum = 0,
	.size = 0,
	.name = NULL,
	.hash = NULL
};

void *__devset_create(bool black, int elenum, int size)
{
	void *p = NULL;
	void *h = NULL;
	if (elenum < 0 || size <= 0) return NULL;
	p = kcalloc(elenum, size, GFP_KERNEL);
	data.name = NULL;
	data.hash = NULL;
	if (p == NULL) return p;
	h = kcalloc(elenum, sizeof(*data.hash), GFP_KERNEL);	
	if (h == NULL) {
		kfree(p);
		return NULL;
	}
	data.elenum = elenum;
	data.name = p;
	data.hash = h;
	data.black = black;
	data.size = size;
	return p;
}

void __devset_order(void)
{
	int i = 0;
	int j = 0;
	char *name = data.name;
	int small = 0;
	u32 tmphash = 0;
	char *tmpname = NULL;
	if (unlikely(data.hash == NULL || data.name == NULL)) return;
        tmpname =kzalloc(data.size, GFP_KERNEL);	
	for (i = 0; i < data.elenum; i++){
		data.hash[i] = jhash(name,
				strlen(name),DEV_NAME_HASH_INITVAL);

		name += data.size;
	}

	name = data.name;
	for (i = 0; i < data.elenum; i++){
		small = i;
		for (j = i + 1; j < data.elenum; j++) {
			if (data.hash[j] < data.hash[small]){
				small = j;
			}
		}
		if (small != i) {
			tmphash = data.hash[i];
			memcpy(tmpname, name, data.size);
			data.hash[i] = data.hash[small];
			memcpy(name, data.name + (data.size * small),
					data.size);
			data.hash[small] = tmphash;
			memcpy(data.name + (data.size * small), tmpname,
					data.size);

		}
		name += data.size;
	}

	kfree(tmpname);

}
bool devset_ignore(char *s)
{
	u32 h = 0;
	int low = 0;
	int high = data.elenum - 1;
	int t = 0;
	pr_debug(IPS"devset_ignore\n");
	if (high < low) return !data.black;
	h = jhash(s, strlen(s), DEV_NAME_HASH_INITVAL);
	if (unlikely(data.hash == NULL || data.name == NULL)) return false;
	while(low <= high) {
		if (h < data.hash[high] && h > data.hash[low]){
			t = low + (high - low) /2;
			pr_debug("t:%d low:%d, high%d\n", t, low, high);
			if (h < data.hash[t]) {
				high = t - 1;
			} else if (h > data.hash[t]) {
				low = t + 1;
			} else {
				break;
			}
		} else if (h == data.hash[high]) {
			t = high;
			break;

		} else if (h == data.hash[low]) {
			t = low;
			break;

		} else {
			t = -1;
			break;
		}
	}
	if (t == -1) {
		return !data.black;
	} else {
		if (strncmp(s, data.name + t * data.size, data.size) == 0) {
			return data.black;
		}
		return !data.black;

	}

}
void devset_exit(void)
{
	if (data.name) kfree(data.name);
	data.name = NULL;
	if (data.hash) kfree(data.hash);
	data.hash = NULL;
	data.black = false;
	data.elenum = 0;
	data.size = 0;
}


