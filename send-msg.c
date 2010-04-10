/*
 * send-msg.c
 *
 *  Created on: 2010-2-6
 *      Author: feng
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "type.h"
#include "timer.h"
#include "send-msg.h"

//#define INFO_DEBUG

#define MAX_MSG_QUEUE_LEN	20
#define MAX_SEND_RETRY	3

#define MAX_SEND_IN_ONE_LOOP	3
#define SEND_INTERVAL			1

SEND_MSG *msg_queue_begin = NULL;
SEND_MSG *msg_queue_add = NULL;
uint8	msg_queue_len = 0;

void set_msg_high_level(SEND_MSG * msg)
{
	msg->prior = MSG_HIGH_PRIOR;
}

//void set_msg_before_high_level(SEND_MSG * msg)
//{
//	SEND_MSG* p = msg_queue_begin;
//
//	do
//	{
//		set_msg_high_level(p);
//	}while((p->next != msg) && (p=p->next));
//
//	set_msg_high_level(msg);
//}

//set all msg to be ready to be send
void set_msg_all_high_level(void)
{
	SEND_MSG* p = msg_queue_begin;

	do
	{
		set_msg_high_level(p);
	}while((p->next != NULL) && (p=p->next));
}

//add one msg into msg queue
//return 0 if queue is full
int add_new_msg(SEND_MSG *msg)
{
	if(msg_queue_len > MAX_MSG_QUEUE_LEN)
		return 0;

	if(msg_queue_len == 0)
	{
		msg_queue_begin = malloc(sizeof(SEND_MSG));
		memcpy(msg_queue_begin, msg, sizeof(SEND_MSG));

		msg_queue_add = msg_queue_begin;
	}
	else
	{
		msg_queue_add->next = malloc(sizeof(SEND_MSG));
		memcpy(msg_queue_add->next, msg, sizeof(SEND_MSG));

		msg_queue_add = msg_queue_add->next;
	}

	msg_queue_add->next = NULL; //be safe
	reset_one_timer(&msg_queue_add->send_time);

	msg_queue_len ++;
	return msg_queue_len;
}

int clear_one_msg(void)
{
	if(msg_queue_len <=0)
		return false;

	free(msg_queue_begin);
	msg_queue_begin = msg_queue_begin->next;

	msg_queue_len --;
	return true;
}

//get one msg from msg queue
//return false if there is no msg in queue
SEND_MSG *get_one_msg(void)
{
	if(msg_queue_len <=0)
		return NULL;

	if ( msg_queue_begin->send_retry >= MAX_SEND_RETRY)
		clear_one_msg();

	if (msg_queue_begin != NULL)
	{
		return msg_queue_begin;
	}
	else
	{
		return NULL;
	}
}

int send_one_msg(SEND_MSG *msg)
{
	//TODO:waiting to add function
//	if (msg->pIDEV->enabled)
//	{//send msg
//		idev_lock(msg->pIDEV);
//		msg->pIDEV->send_msg(msg->pIDEV, msg->target, msg->cont);
//		idev_unlock(msg->pIDEV);
//	}
	msg->send_retry ++;

	return true;
}

int send_msg(SEND_MSG *msg)
{
//	msg->send_retry = 0; //for safe

	switch(msg->prior)
	{
		case MSG_HURRY_PRIOR:
			//check target modem
			while((send_one_msg(msg) == false) && (msg->send_retry < MAX_SEND_RETRY))
				sleep(SEND_INTERVAL);
			break;
		case MSG_LOW_PRIOR:
			//add to msg queue
			add_new_msg(msg);
			break;
		case MSG_HIGH_PRIOR:
			//add to msg queue
			add_new_msg(msg);
			//change all msg' status to high prior
			set_msg_all_high_level();
			break;
	}
	return 1;
}

void handle_send_msg(void)
{
	//check info queue
	SEND_MSG *pSMS;
	int i;

	for(i=0; i<MAX_SEND_IN_ONE_LOOP; i++)
	{
		//no msg in queue
		if((pSMS = get_one_msg()) == NULL)
		{
			i --;
			break;
		}

		if(pSMS->prior != MSG_HIGH_PRIOR)
		{
			i --;
			break;
		}

		//send one msg
		if(send_one_msg(pSMS))
			clear_one_msg();

		sleep(SEND_INTERVAL);
	}

	//check if the msg has waited too long
	if((pSMS = get_one_msg()))
	{
		if(check_if_threshold_passed(pSMS->send_time, MINUTE(10)))
			set_msg_high_level(pSMS);
	}
}

//wrapper of send msg
int init_s_msg(SEND_MSG *msg, IDEV *pIDEV)
{
	memset(msg, 0x00, sizeof(SEND_MSG));

	msg->pIDEV = pIDEV;

	return true;
}

int set_s_msg_report_needed(SEND_MSG *msg)
{
	msg->status_report_needed = NEED_STATUS_REPORT;

	return true;
}

int set_s_msg_prior(SEND_MSG *msg, uint8 prior)
{
	msg->prior = prior;

	return true;
}

int set_s_msg_target(SEND_MSG *msg, const char *target)
{
	if(strlen(target) > (MAX_TARGET_LEN-1))
		return false;
	else
		strcpy(msg->target, target);

	return true;
}

int set_s_msg_cont(SEND_MSG *msg, const char *cont)
{
	if(strlen(cont) > (MAX_MSG_LEN-1))
		return false;
	else
		strcpy(msg->cont, cont);

	return true;
}

#ifdef INFO_DEBUG
void print_all_msg(void)
{
	SEND_MSG *temp_msg;
	int msg_num = 1;

	while(temp_msg=get_one_msg())
	{
		printf("msg num: %d\n", msg_num++);
		printf("%s\n", temp_msg->target);
		printf("%s\n", temp_msg->cont);
		printf("%d\n", temp_msg->prior);

		if (msg_num == 4)
		{
			set_msg_before_high_level(temp_msg);
		}
	}
}

int main()
{
	SEND_MSG temp_msg;
	int testNum=1;
	int testCont=1;
	int i;

	for(i=0;i<2;i++)
	{
		sprintf(temp_msg.target, "target%d", testNum++);
		sprintf(temp_msg.cont, "cont%d", testCont++);
		temp_msg.prior = SMS_LOW_PRIOR;

		send_msg(&temp_msg);
		printf("have sent one: %d\n", testNum-1);
	}
	print_all_msg();

	printf("####### splite line #######\n");

	for(i=0;i<2;i++)
	{
		sprintf(temp_msg.target, "target%d", testNum++);
		sprintf(temp_msg.cont, "cont%d", testCont++);
		temp_msg.prior = SMS_LOW_PRIOR;

		send_msg(&temp_msg);
		printf("have sent one: %d\n", testNum-1);
	}

	sprintf(temp_msg.target, "target%d", testNum++);
	sprintf(temp_msg.cont, "cont%d", testCont++);
	temp_msg.prior = SMS_HIGH_PRIOR;

	send_msg(&temp_msg);
	printf("have sent one: %d\n", testNum-1);

	print_all_msg();

	printf("####### splite line #######\n");


	return 0;
}
#endif

