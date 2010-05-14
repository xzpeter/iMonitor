/*
 * file		: dm-config.h
 * desc		: common definitions of iMonitor
 * mailto	: xzpeter@gmail.com
 */

#ifndef		__DM_CONFIG_H__
#define		__DM_CONFIG_H__

/* determine the max number of supported modems 
	 at the same time */
#define		MAX_DEVICE_NO					8

/* max scan range of device file */
#define		MAX_DEVICES_NUM_OF_SAME_PREFIX	32		

#define		MAX_MOBILE_ID_LEN		20
#define		MAX_USERS_PER_MODEM		100
#define		AT_TIMEOUT_SEC  60
#define		MAX_TASKID_LEN  8

#endif
