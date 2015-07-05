#include <linux/jhash.h>
#include  "devset.h"
#define DEV_NAME_HASH_INITVAL 59
static struct {
	bool black;
	size_t elenum;
	size_t size;
	char *name;
	u32 *hash;

}data;
void *__devset_create(bool black, size_t elenum, size_t size)
{
	void *p = kcalloc(elenum, size, GFP_KERNEL);
	void *h = NULL;
	data.name = NULL;
	data.hash = NULL;
	if (p == NULL) return p;
	h = kcalloc(elenum, sizeof(*data.hash), GFP_KERNEL);	
	if (h == NULL) {
		kfree(p);
		return NULL;
	}
	data.name = p;
	data.hash = h;
	data.black = black;
	data.size = size;
	return p;
}

void __devset_order(void)
{
	size_t i = 0;
	size_t j = 0;
	char *name = data.name;
	size_t small = 0;
	u32 tmphash = 0;
	char *tmpname = NULL;
	if (unlikely(data.hash == NULL || data.name == NULL)) return;
        tmpname =kzalloc(data.size, GFP_KERNEL);	
	for (i = 0; i < data.elenum; i++){
		data.hash[i] = jhash(name,
				strlen(name),DEV_NAME_HASH_INITVAL);
		name += data.size;
	}

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
	u32 h = jhash(s, strlen(s), DEV_NAME_HASH_INITVAL);
	size_t low = 0;
	size_t high = data.elenum - 1;
	size_t t = 0;
	if (unlikely(data.hash == NULL || data.name == NULL)) return false;
	while(low <= high) {
		if (h < data.hash[high] && h > data.hash[low]){
			t = low + (high - low) /2;
			if (data.hash[t] > h) {
				low = t + 1;
			} else if (data.hash[t] < h) {
				high = t - 1;
			} else {
				break;
			}
		} else if (h == data.hash[high]) {
			t = high;

		} else if (h == data.hash[low]) {
			t = low;

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
}


