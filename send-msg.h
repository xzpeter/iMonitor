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
#include "task-type.h"

#define MSG_LOW_PRIOR	0
#define MSG_HIGH_PRIOR	1
#define MSG_HURRY_PRIOR	2

#define NEED_STATUS_REPORT	1
#define NO_STATUS_REPORT	0

#define MAX_MSG_LEN	160
#define MAX_TARGET_LEN	16

/* 彩信notification长度 */
#define  MAX_SMS_MMS_LEN                300

#define  MAX_SEQ_LEN                    4

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

////////////////////////////////////////////////////////////////////
//  接收下来或者需要发送的短信息的暂存缓冲区的类型定义            //
////////////////////////////////////////////////////////////////////
typedef union _sms_buff {
        char smsdata[MAX_SMS_MMS_LEN];
        struct {
                char magic[1];
                //uchar separator1;
                char monitor[MAX_MOBILE_IDLEN];
                //uchar separator2;
                char network;
                char montype;
                char taskid[MAX_TASKID_LEN];
                //uchar separator3;
                char seq[MAX_SEQ_LEN];
                //uchar separator4;
                char tasktype[2];
                //uchar separator5;
                char conttype[2];
                //uchar separator6;
                char segtotal[2];
                //uchar separator7;
                char segnum[2];
                //uchar separator8;
                char contlen[3];
                //uchar separator9;
                char cc[2];
                char cont[1];
        } hdr;
} sms_buff;

/* functions to use */
extern int init_s_msg(SEND_MSG *msg, IDEV *pIDEV);
extern int set_s_msg_report_needed(SEND_MSG *msg);
extern int set_s_msg_prior(SEND_MSG *msg, uint8 prior);
extern int set_s_msg_target(SEND_MSG *msg, const char *target);
extern int set_s_msg_cont(SEND_MSG *msg, const char *cont);

extern int send_msg(SEND_MSG *msg);
extern void handle_send_msg(void);

#endif /* SENDMSG_H_ */
