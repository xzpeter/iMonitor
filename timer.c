#include <stdio.h>
#include "timer.h"

//struct timeval task_s_count;
//struct timeval boottime;
//struct timeval timer_s_border_networkdata;

long get_elapsed_seconds(struct timeval old_time)
{
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);
	if((cur_time.tv_usec -= old_time.tv_usec) < 0) {
		--cur_time.tv_sec;
		cur_time.tv_usec += 1000000;
	}
	cur_time.tv_sec -= old_time.tv_sec;

	return (long)cur_time.tv_sec;
}


long get_elapsed_mseconds(struct timeval old_time)
{
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);
	if((cur_time.tv_usec -= old_time.tv_usec) < 0) {
		--cur_time.tv_sec;
		cur_time.tv_usec += 1000000;
	}
	cur_time.tv_sec -= old_time.tv_sec;

	return (long)(cur_time.tv_sec*1000 + cur_time.tv_usec/1000);
}

int check_if_threshold_passed(struct timeval old_time, long threshold)
{
	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);
	if((cur_time.tv_usec -= old_time.tv_usec) < 0) {
		--cur_time.tv_sec;
		cur_time.tv_usec += 1000000;
	}
	cur_time.tv_sec -= old_time.tv_sec;

	return (cur_time.tv_sec >= threshold) ? 1 : 0;
}

void reset_one_timer(struct timeval *tv)
{
	gettimeofday(tv, NULL);
}

// void reset_task_timer(void)
// {
// 	gettimeofday(&task_s_count, NULL);
// }

/*
int main(void)
{
	time_t tt;
	struct timeval tv;

	while(1) {
		time(&tt);
		reset_one_timer(&tv);
		printf("time: %ld\n", tt);
		printf("timeval: %ld.%ld\n", tv.tv_sec, tv.tv_usec);
		while(!check_if_threshold_passed(tv, 10));
		printf("%ld seconds passed!!!\n", get_elapsed_seconds(tv));
		printf("%ld mseconds passed!!!\n", get_elapsed_mseconds(tv));
	}
}*/
