#ifndef TIMER_H_
#define TIMER_H_

#include <sys/time.h>

#define MINUTE(x)	(60*(x))
#define HOUR(x)		(3600*(x))

//extern struct timeval task_s_count;
//extern struct timeval boottime;
//extern struct timeval timer_s_border_networkdata;

extern long get_elapsed_seconds(struct timeval old_time);
extern long get_elapsed_mseconds(struct timeval old_time);
extern int check_if_threshold_passed(struct timeval old_time, long seconds);
extern void reset_one_timer(struct timeval *tv);
extern void reset_task_timer();

#endif /* TIMER_H_ */
