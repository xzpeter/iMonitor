/*
 * file : dm-base.h
 * desc : all the important structures in iMon stores here. 
 *        APPLICAIONs SHOULD NOT INCLUDE ME!
 * mail : xzpeter@gmail.com
 */

#ifndef		__DM_BASE_H__
#define		__DM_BASE_H__

#include "dm-config.h"

typedef struct _dev_usage {
	/* e.g. 'ttyUSB' */
	char *keyword;
	/* 0 <= start_num <= end_num < MAX_DEVICES_NUM_OF_SAME_PREFIX */
	/* if start_num > end_num, no device would be checked. */
	unsigned int start_num;
	unsigned int end_num;		
	/* if enabled is set to 0, the corresponding dev is skipped */
	unsigned char enabled[MAX_DEVICES_NUM_OF_SAME_PREFIX];
	/* if in_use[n] is set, the file '/dev/keywordn' is in use */
	unsigned char in_use[MAX_DEVICES_NUM_OF_SAME_PREFIX];
	/* if we find 'keywordn' file exists, then set checked[n] */
	unsigned char checked[MAX_DEVICES_NUM_OF_SAME_PREFIX];
	/* keep the model type that 'keywordn' belongs to */
	unsigned char type[MAX_DEVICES_NUM_OF_SAME_PREFIX];
} DEV_USAGE;

typedef struct _dev_node {
	int active;
	int during_test;	/* used only in modem test */
	IDEV *idev;
} DEV_NODE;

/* this is the vital list that keeps all the device info */
typedef struct _dev_list {
	DEV_NODE dev[MAX_DEVICE_NO];
	int dev_total;
} DEV_LIST;

extern DEV_LIST dev_list;

#endif
