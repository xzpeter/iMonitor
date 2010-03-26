/*
   mod-common.c : shared idev methods are put here
*/

#include <stdio.h>
#include <term.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "dm.h"
#include "nms.h"

#define			MAX_CELLID_LEN		20

/****************************
* LOCALLY USED FUNCTIONS	*
****************************/

int common_set_sms_mode(IDEV *p, int n)
{
	char cmd[32];

	if (p == NULL || (n != 0 && n != 1))
		return -1;

	snprintf(cmd, 32, "AT+CMGF=%d", n);
	return p->send(p, cmd, NULL, AT_MODE_LINE);
}

/****************************
* VIRTUAL IDEVICE FUNCTIONS	*
****************************/

/* CAUTION : before call all these functions in the host thread, 
   we should always get the mutex of the idev.
   AND : we should never touch the mutex in all these functions below, 
   or deadlocks may occur. */

/* send a SMS */
int common_send_sms(IDEV *p, char *who, char *data)
{
	/* p->send must be implemented first */
	char cmd_buf[512];
	int ret;
	/* first, get the ownership of the device */
// 	lock_device(p);
	if ((ret = strlen(who)) >= MAX_CELLID_LEN) {
		dm_log(p, "CELLID TOO LONG in common_send_sms(), [len=%d] : %s",
				ret, who);
		goto send_sms_error;
	}
	if (strlen(data) >= 500) {
		dm_log(p, "DATA TOO LONG in common_send_sms(), [len=%d].", data);
		goto send_sms_error;
	}
	ret = snprintf(cmd_buf, 510, "AT+CMGS=\"%s\"", who);
	/* send CMGS cmd */
	ret = (int)p->send(p, cmd_buf, NULL, AT_MODE_LINE);
	if (ret != AT_RAWDATA) {
		dm_log(p, "SEND SMS cmd error, it's %d, should %d.", 
				ret, AT_RAWDATA);
		goto send_sms_error;
	}
	sleep(1);
	/* send rawdata */
	snprintf(cmd_buf, 510, "%s\x1A", data);
	ret = (int)p->send(p, cmd_buf, NULL, AT_MODE_LINE);
	if (ret != AT_OK) {
		dm_log(p, "SEND RAW DATA error, return is %d, should %d.", 
				ret, AT_OK);
		goto send_sms_error;
	}
	/* send ok */
	dm_log(p, "SMS sent : %s ---> %s", data, who);
// 	unlock_device(p);
	return 0;

send_sms_error:
// 	unlock_device(p);
	return -1;
}

/* doing forward of a sms index N to another cell phone WHO */
int common_forward(IDEV *p, char *who, int n)
{
	char cmd[64], recv_buf[1024], *pch, *cont;
	int contlen;

	if (p == NULL || strlen(who) > MAX_MOBILE_ID_LEN ||
			n < 0 || n > 39 ) 
		return -1;

// 	/* set cmgf -> 0 */
// 	if (common_set_sms_mode(p, 0)) {
// 		dm_log(p, "set_sms_mode error.");
// 		return -2;
// 	}

	/* first, read the sms */
	snprintf(cmd, 64, "AT^DMGR=%02d", n);
	if (p->send(p, cmd, recv_buf, AT_MODE_BLOCK)) {
		dm_log(p, "read sms error.");
		return -3;
	}

	pch = strstr(recv_buf, "^DMGR:");
	if (pch == NULL) {
		dm_log(p, "strstr() error.");
		return -4;
	}
	pch = strchr(pch, '\n');
	if (pch == NULL) {
		dm_log(p, "strstr() error 2.");
		return -5;
	}
	cont = ++pch; /* points to the raw sms data */
	pch = strchr(pch, '\n');
	if (pch == NULL) {
		dm_log(p, "strstr() error 3.");
		return -6;
	}
	contlen = pch - cont;
	dm_log(p, "length of sms to forward: %d", contlen);
	*pch = 0x00;	/* cut the string */

	puts("===============\n");
	puts(cont);
	puts("===============\n");
	
	if (do_balance_forward(p, who, cont, contlen)) {
		dm_log(p, "error in do_balance_forward().");
		return -7;
	}

	return 0;
}

/****************************
*  BASIC IDEVICE FUNCTIONS	*
****************************/

/* CAUTION!! this function is really common used, but really dangerous, 
   you must be sure that *data is OBSOLUTELY large enough for any possible
   data. the recommended length is 1024 AT LEAST */
AT_RETURN common_send(IDEV *p, char *data, char *result, int mode)
{
	char *p1, *p2;
	char key[128] = {0};
	int len;

	if ((len = strlen(data)) >= 500) {
		dm_log(p, "AT CMD TOO LONG, in common_send().");
		return AT_NOT_RET;
	}

	/* wait until device is ready */
	while (!check_at_ready(p))
		usleep(100000);
	if (idev_is_sick(p))
		return AT_NOT_RET;

	p->at.mode = mode;
	p->at.recv_buf[0] = 0x00; /* clear recv buffer */

	if ( mode & AT_MODE_LINE ) {
		/* find the keyword */
		p1 = strpbrk(data, "+^");
		if (p1) {
			p2 = strpbrk(p1++, "=?");
			if (p2) { 
				memcpy(key, p1, p2-p1);
				key[p2-p1] = 0x00;
			} else {
				strcpy(key, p1);
			}
		} else {
			/* no key */
			key[0] = 0x00;
		}
		strcpy(p->at.keyword, key);
	} else { /* AT_MODE_BLOCK */
		/* save all the block, not a keyword */
		p->at.keyword[0] = 0x00;
	}

	if (data[len-1] == 0x1a) 
		/* do special treatment while sending sms */
		strcpy(p->at.send_buf, data);
	else
		snprintf(p->at.send_buf, sizeof(p->at.send_buf), 
			"%s\r", data);

	/* order to send! */
	at_send_it(p);
	/* wait until job is done */
	while (!check_at_recved(p))
		usleep(100000);
	if (idev_is_sick(p))
		return AT_NOT_RET;

	/* if the caller supplied a buffer to recv results, let's do it. */
	if (result)
		strcpy(result, p->at.recv_buf);

	/* data recved in p->at.recv_buf */
	at_recved_done(p);
	return p->at.ret;
}

ssize_t common_rawsend(IDEV *p, char *buf, int n)
{
	return write(p->portfd, buf, n);
}

ssize_t common_rawrecv(IDEV *p, char *buf, int n)
{
	int len;
	len = read(p->portfd, buf, n);
	buf[len] = 0x00;
	return len;
}

/* return 1 if there is a device to adopt,
   set all the devices that you will take to '1' in dev_to_take list */
int common_device_file_adoptation(char *type_array, 
		unsigned char *dev_to_take, IDEV_DEV_FILE *dev_file)
{
	/* never adopt a device */
	return -1;
}

/* do the startup work for your module, 
   returns 0 if startup ok, else if error. */
int common_module_startup(IDEV *p)
{
	return -1;
}

/* use the base_dev to decide your related devices, 
   put the result in the RELATED_DEV structure given */
int common_get_related_device(char *base_dev, RELATED_DEV *prd)
{
	return 0;
}

/* do anything you want to check whether the device file
   belongs to you. return 1 if so, return 0 to discard. */
int common_check_device_file(char *file)
{
	/* by default, adopt nothing */
	return 0;
}

void common_close_port(IDEV *p)
{
	idev_lock(p);
	if (p->portfd) {
		close(p->portfd);
		p->portfd = 0;
	}
	idev_unlock(p);
}

int common_open_port(IDEV *pidev)
{
	char portdevice[256];
	int portfd;

	/* if the port is already opened */
	if (pidev->portfd > 0) {
		dm_log(pidev, "port already opened.");
		return 0;
	}

	snprintf(portdevice, 256, "%s%s", DEV_DIR, pidev->dev_file.base_device);

	/* try to open port */
	if ((portfd = serial_open_port(portdevice, 1)) < 0) {
		dm_log(pidev, "serial_open_port() failed.");
		return -1;
	}

	/* save fd */
	idev_lock(pidev);
	pidev->portfd = portfd;
	idev_unlock(pidev);

	return 0;
}
