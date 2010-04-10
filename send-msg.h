/*
 * send-msg.h
 *
 *  Created on: 2010-2-6
 *      Author: feng
 */

#ifndef SENDMSG_H_
#define SENDMSG_H_

#include "dm.h"
#include "type.h"
#include "timer.h"

#define MSG_LOW_PRIOR	0
#define MSG_HIGH_PRIOR	1
#define MSG_HURRY_PRIOR	2

#define NEED_STATUS_REPORT	1
#define NO_STATUS_REPORT	0

#define MAX_MSG_LEN	160
#define MAX_TARGET_LEN	16

typedef struct _send_msg_buf SEND_MSG;

struct _send_msg_buf
{
	IDEV *pIDEV;
	uint8 status_report_needed;
	uint8 prior;
	uint8 send_retry;
	struct timeval send_time;
	char target[MAX_TARGET_LEN];
	char cont[MAX_MSG_LEN];
	SEND_MSG *next;
};

/* functions to use */
extern int init_s_msg(SEND_MSG *msg, IDEV *pIDEV);
extern int set_s_msg_report_needed(SEND_MSG *msg);
extern int set_s_msg_prior(SEND_MSG *msg, uint8 prior);
extern int set_s_msg_target(SEND_MSG *msg, const char *target);
extern int set_s_msg_cont(SEND_MSG *msg, const char *cont);

extern int send_msg(SEND_MSG *msg);
extern void handle_send_msg(void);

#endif /* SENDMSG_H_ */
