#include <pthread.h>
#include <stdlib.h>
#include<string.h>
#include "framework.h"
static void (*handle)(struct ip_key_info_set s);
static int newthreadused;
static int running = 0;
static pthread_t tid;
struct {
	void * (*run)();
        void (*destroy)(struct ip_key_info_set *);
} function;


static void *ips_frame_mt_run() {
	struct ip_key_info_set s = { 0 };
	size_t lastn = 0;
	size_t nextn = 16;
	int err = 0;
	while (running) {
		if (nextn) {
			s.n = nextn;
			s.array = (struct ip_key_info *)
				malloc(s.n * sizeof (struct ip_key_info));
			if (s.array == NULL) exit(99);
		} else {

			s.n = lastn;
		}
		lastn = s.n;

		err = ips_fetch(&s);
		if (err) {
			nextn = 0;
		} else {
			nextn = s.n;
			if (s.n < lastn /4) {
				nextn = lastn /2;
			}  else if (s.n == lastn) {
				nextn = lastn * 2;
			}
			if (nextn < 16) nextn = 16;
			handle(s);
		}
	}
	return NULL;
}

static void *ips_frame_ut_run() {
	struct ip_key_info_set s = { 0 };
	struct ip_key_info info[512];
	int err = 0;
	while (running) {
		s.n = sizeof(info)/sizeof(struct ip_key_info);
		s.array = info;
		err = ips_fetch(&s);
		if (!err) {
			handle(s);
		}
	}
	return NULL;
}

int ips_frame_start(struct devname_list *l) {
	int err = ips_init();
	pthread_t ntid;
	if (err) return err;
	err = ips_config(l);
	if (err) goto fail;
	running = 1;
	err = pthread_create(&ntid, NULL, function.run,  NULL);
	if (err) goto fail;
	tid = ntid;
	return err;
fail:
	running = 0;
	ips_exit();
	return err;
}

void ips_frame_mt_destroy(struct ip_key_info_set *s) {
	if (s == NULL || s->buf == NULL) return;
	free(s->buf);
	s->n = 0;
	s->buf = NULL;
}

void ips_frame_ut_destroy(struct ip_key_info_set *s) {
	memset(s->buf, 0, sizeof(struct ip_key_info) * s->n);
	s->n = 0;
}
void ips_frame_destroy(struct ip_key_info_set *s) {
	function.destroy(s);
}

void ips_frame_stop() {
	running = 0;
	if (tid > 0) pthread_join(tid, NULL);
	ips_exit();
}

void ips_frame_register(void (*h)(struct ip_key_info_set s), int newthread) {
	if (running) return;
	handle = h;
	newthreadused = newthread;
	if (newthread) {
		function.run = ips_frame_mt_run;
		function.destroy = ips_frame_mt_destroy;
	} else {
		function.run = ips_frame_ut_run;
		function.destroy = ips_frame_ut_destroy;
	}
}
