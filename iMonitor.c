/*
   This is the iMonitor thread, which should be the main thread now.
   It is a NMS Device Manager, with hot plug-in&outs.
*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "dm.h"
/* FIX ME */
#include "mod-common.h"

#define		MAX_DEVICE_NO					8
/* max scan range of device file */
#define		MAX_DEVICES_NUM_OF_SAME_PREFIX	32		

/* this MUST be modified when 'dev_usage' is changed. */
#define		SUPPORTED_DEVICE_PREFIX_N		1

/* we'll scan all the files in the /dev/ dir, related with the keywords 
   below, and try to allocate every effective file node to a module. */
static struct _dev_usage {
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
} dev_usage[SUPPORTED_DEVICE_PREFIX_N] = {
	{ "ttyUSB", 0, 20}, 
// 	{ "ttys", 0, 1}
};

typedef struct _dev_node {
	int active;
	IDEV *idev;
} DEV_NODE;

/* this is the vital list that keeps all the device info */
static struct _dev_list {
	DEV_NODE dev[MAX_DEVICE_NO];
	int dev_total;
} dev_list;

int dev_prefix_find_index(char *prefix);
int mark_related_device_in_dev_usage(RELATED_DEV *r_dev, int value);
void dev_usage_init(void);
void dev_usage_clear_before_check(void);
int dev_list_init(void);
int find_index_in_dev_usage(char *keyword);
int clear_in_use_flag_for_device(int i);
int unregister_device(int i);
int find_empty_entry(void);
int register_device(char *name, RELATED_DEV *rdev, IDEV_TYPE type);
int add_new_unknown_device(int i, int j);
int find_device_file_in_use(int i, int j);
int remove_untached_device(int i, int j);
int device_check(void);
void show_all_active_devices(void);
int main(int argc, char *argv[]);

/* find index of the prefix in dev_usage struct */
int dev_prefix_find_index(char *prefix)
{
	int i;
	for (i = 0; i < SUPPORTED_DEVICE_PREFIX_N; i++)
		if (!strcmp(prefix, dev_usage[i].keyword))
			return i;
	/* not found */
	return -1;
}

int mark_related_device_in_dev_usage(RELATED_DEV *r_dev, int value)
{
	int ibase, j;

	if (value != 0 && value != 1)
		return -1;

	ibase = find_index_in_dev_usage(r_dev->prefix);
	if (ibase < 0)
		return -2;
	for (j = r_dev->start_num; j <= r_dev->end_num; j++)
		dev_usage[ibase].in_use[j] = value;

	return 0;
}

/* set all the devices to 'not_in_use' */
void dev_usage_init(void)
{
	int i;
	for (i = 0; i < SUPPORTED_DEVICE_PREFIX_N; i++) {
		bzero(dev_usage[i].in_use, sizeof(dev_usage[i].in_use));
		bzero(dev_usage[i].checked, sizeof(dev_usage[i].checked));
		bzero(dev_usage[i].type, sizeof(dev_usage[i].type));
		/* set all enabled as default. */
		memset(dev_usage[i].enabled, sizeof(dev_usage[i].enabled), 0x01);
	}
}

void dev_usage_clear_before_check(void)
{
	int i;
	for (i = 0; i < SUPPORTED_DEVICE_PREFIX_N; i++) {
		bzero(dev_usage[i].checked, sizeof(dev_usage[i].checked));
		bzero(dev_usage[i].type, sizeof(dev_usage[i].type));
	}
}

/* the monitor should do the init before doing anything */
int dev_list_init(void)
{
	bzero(&dev_list, sizeof(dev_list));
	return 0;
}

int find_index_in_dev_usage(char *keyword)
{
	int i;
	for (i = 0; i < SUPPORTED_DEVICE_PREFIX_N; i++)
		if (!strcmp(keyword, dev_usage[i].keyword))
			return i;

	return -1;
}

/* remove all the 'in_use' flag in dev_usage which is related to device i */
int clear_in_use_flag_for_device(int i)
{
	char buf[DEV_FILE_NAME_LEN];
	int ibase, j;
	IDEV_DEV_FILE *dev_file = &dev_list.dev[i].idev->dev_file;
	RELATED_DEV *r_dev = &dev_file->related_device;
	
	/* first, remove base device */
	split_dev_filename(dev_file->base_device, buf, &j);
	ibase = find_index_in_dev_usage(buf);
	if (ibase < 0)
		return -1;
	dev_usage[ibase].in_use[j] = 0;

	/* second, remove related devices */
	mark_related_device_in_dev_usage(r_dev, 0);

	return 0;
}

/* if a device is dead, use this function to unregister from
   device list.  */
int unregister_device(int i)
{
	IDEV *p;
	if (dev_list.dev[i].active == 0)
		return -1;

	/* 1. clear flags */
	if (clear_in_use_flag_for_device(i))
		return -2;

	p = dev_list.dev[i].idev;

	/* I don't know if it is ok or not. */
	/* TOFIX: how to free a structure with mutex ? */
	lock_device(p);
	idev_lock(p);
	idev_unlock(p);
	unlock_device(p);

	/* 2. shut down daemon thread */
	if (pthread_cancel(dev_list.dev[i].idev->thread_id))
		dm_log(NULL, "\n[VITAL ERROR] thread cancel failed. going on.\n");

	/* 3. remove device structure */
	dev_list.dev_total--;

	dev_list.dev[i].active = 0;

	idev_release(p);
	dev_list.dev[i].idev = NULL;

	dm_log(NULL, "REMOVING device index %d.", i);

	/* done */
	return 0;
}

/* find the first empty device entry, return index. */
int find_empty_entry(void)
{
	int i;
	for (i = 0; i < MAX_DEVICE_NO; i++)
		if (dev_list.dev[i].active == 0)
			return i;
	return -1;
}

/* use this function to register a device */
/* there are 3 steps here :
   1. set related 'in_use' flag in dev_usage
   2. create related data structure of the device
   3. create daemon thread for the device
   (step 1 is done before calling register) */
int register_device(char *name, RELATED_DEV *rdev, IDEV_TYPE type)
{
	IDEV *p_idev;
	int i;

	/* step 2 */
	/* check if there is enough room */
	if (dev_list.dev_total >= MAX_DEVICE_NO)
		return -1;

	i = find_empty_entry();
	if (i >= 0) {
		/* temprarily, we only support LC6311 module */
		p_idev = idev_init(name, rdev, type);
		if (p_idev) {
			/* idev_init is also ok, then update device list. */
			dev_list.dev[i].active = 1;
			dev_list.dev[i].idev = p_idev;
			dev_list.dev_total++;
		} else {
			/* idev_init failed */
			return -2;
		}
	}

	/* FIX ME */
	/* temporarily, enable module as soon as possible. */
	idev_set_enable(p_idev);
	
	/* step 3 */
	/* create daemon thread for the device. */
	if (pthread_create(&p_idev->thread_id, NULL, device_daemon, p_idev)) {
		/* thread creation error */
		dm_log(NULL, "create thread for device [%d][%s] error.", i, name);
		/* free the device structure */
		idev_release(p_idev);
		return -3;
	}

	/* all ok. */
	return i;
}

/* find device file whose prefix is dev_usage[i].keyword, 
   index is j. return index in the device list, or -1 if not found */
int find_device_file_in_use(int i, int j)
{
	char buf[DEV_FILE_NAME_LEN];
	char *keyword = dev_usage[i].keyword;

	snprintf(buf, DEV_FILE_NAME_LEN, "%s%d", keyword, j);
	for (i = 0; i < MAX_DEVICE_NO; i++) {
		if (dev_list.dev[i].active) {
			/* a active device */
			IDEV_DEV_FILE *dev_f = &dev_list.dev[i].idev->dev_file;
			RELATED_DEV *r_dev = &dev_f->related_device;

			/* if the base device file match? */
			if (!strcmp(dev_f->base_device, buf))
				return i;

			/* if the related devices match? */
			if ((!strcmp(r_dev->prefix, keyword)) && 
					j >= r_dev->start_num && j <= r_dev->end_num)
				return i;
		}
	}

	/* not found */
	return -1;
}

int remove_untached_device(int i, int j)
{
	int n;
	IDEV *p;
	/* find which device it belongs */
	n = find_device_file_in_use(i, j);
	if (n == -1)	/* not found */
		return -1;

	p = dev_list.dev[n].idev;

	/* added 3-26: 
	   before we try to obtain the device lock, we have to tell
	   everyone using the device, that "THE DEVICE IS DYING!!"*/
	idev_set_status(p, UNTACHED);
	while (idev_get_status(p) != DEAD) 
		/* wait the response from daemon thread */
		sleep(1);

	/* unregister device from list */
	if (unregister_device(n))
		return -2;

	/* remove that device, and it's related devices */
	return 0;
}

void clear_dead_modules(void)
{
	int i;
	for (i = 0; i < MAX_DEVICE_NO; i++) {
		IDEV *p = dev_list.dev[i].idev;
		if (dev_list.dev[i].active && p->status == DEAD) {
			dm_log(NULL, "device[%d] %s type %s is dead, freeing the device.", 
					i, p->name, dev_model[p->type].name);
			unregister_device(i);
		}
	}
}

IDEV_TYPE get_device_type(int i, int j)
{
	int k;
	char dev_path[256];

	snprintf(dev_path, 256, "%s%d", dev_usage[i].keyword, j);

	for (k = 0; k < SUPPORTED_DEVICES; k++)
		if (dev_model[k].check_device_file(dev_path))
			return (IDEV_TYPE)k;

	return UNKNOWN;
}

void format_one_dev_file_str(char *str, unsigned char *type, IDEV_TYPE n)
{
	int i;
	for (i = 0; i < MAX_DEVICES_NUM_OF_SAME_PREFIX; i++) {
		char c;
		if (type[i] == n)
			c = '1';
		else
			c = '0';
		str[i] = c;
	}
	str[i] = 0x00;
}

/* check if there is new device to register, or dead to remove */
/* there are 2 kinds of devices to remove :
   1. the pidev->status == DEAD (which means that the module doesn't work)
   2. device file has gone (maybe the device is disconnected) 
   we have to take all these possibilities into consideration.
 */
int device_check(void)
{
	int i, j;

	/* clear all 'checked' sign */
	dev_usage_clear_before_check();

	/* update 'checked' */
	for (i = 0; i < SUPPORTED_DEVICE_PREFIX_N; i++) {
		for (j = dev_usage[i].start_num; j <= dev_usage[i].end_num; j++) {
			char dev_file[256];
			snprintf(dev_file, 256, "%s%s%d", DEV_DIR, dev_usage[i].keyword, j);
			/* check file existance */
			if (!access(dev_file, F_OK))
				dev_usage[i].checked[j] = 1;
		}
	}

	for (i = 0; i < SUPPORTED_DEVICE_PREFIX_N; i++) {
		for (j = dev_usage[i].start_num; j <= dev_usage[i].end_num; j++) {
			if (dev_usage[i].in_use[j] == 0 && dev_usage[i].checked[j] == 1) {
				/* a new device file */
				dev_usage[i].type[j] = get_device_type(i, j);
			} else if (dev_usage[i].in_use[j] == 1 
					&& dev_usage[i].checked[j] == 0) {
				int ret;
				/* a model in use is untached, try to unreg it */
				if ((ret = remove_untached_device(i, j)) < 0) {
					/* remove error */
					dm_log(NULL, "ERROR : removing device %s%d failed.[%d]", 
							dev_usage[i].keyword, j, ret);
				} else { /* remove ok */
					dm_log(NULL, "REMOVE device %s%d index %d.", 
							dev_usage[i].keyword, j, ret);
				}
			}
		}
	}

	/* type of new discovered devices are kept in {dev_usage[*].type} */
	for (i = 0; i < SUPPORTED_DEVICES; i++) {
		int add_device;
		IDEV_DEV_FILE dev_file;
		char dev_file_str[MAX_DEVICES_NUM_OF_SAME_PREFIX+1];
		unsigned char dev_taken[MAX_DEVICES_NUM_OF_SAME_PREFIX+1];

		bzero(dev_taken, sizeof(dev_taken));
		/* here, we statically only check dev_usage[0], whose keyword 
		   is "ttyUSB" */
		format_one_dev_file_str(dev_file_str, dev_usage[0].type, (IDEV_TYPE)i);
		add_device = (int)dev_model[i].device_file_adoptation(dev_file_str, 
				dev_taken, &dev_file);

		if (add_device == 1) {
			int ret;
			/* here is step 1 of register_device, 
			   set all taken devices to 'in_use' */
			for (j = 0; j < MAX_DEVICES_NUM_OF_SAME_PREFIX; j++)
				if (dev_taken[j])
					dev_usage[0].in_use[j] = 1;
			
			ret = register_device(dev_file.base_device, 
					&dev_file.related_device, (IDEV_TYPE)i);

			if (ret >= 0 && ret <= MAX_DEVICE_NO)
				dm_log(NULL, "ADD device(%d) %s as %s module.", 
						ret, dev_file.base_device, dev_model[i].name);
			else
				dm_log(NULL, "ERROR add device %s as %s module. ret[%d].", 
						dev_file.base_device, dev_model[i].name, ret);
		}
	}

	/* finally, let's see if there is any module delared dead */
	clear_dead_modules();

	return 0;
}

void show_all_active_devices(void)
{
	int i;
	if (dev_list.dev_total == 0)
		return;

	dm_log(NULL, "=================");
	for (i = 0; i < MAX_DEVICE_NO; i++)
		if (dev_list.dev[i].active)
			show_device_info(dev_list.dev[i].idev);
	dm_log(NULL, "=================");
}

void thread_log(IDEV *p, char *test_type, char *words)
{
	char filename[64];
	FILE *file;
	time_t mtime;
	IDEV_TYPE type;

	type = p->type;

	snprintf(filename, 64, "log-%s-%s.txt", dev_model[type].name, test_type);
	file = fopen(filename, "a");
	if (file == NULL) {
		fprintf(stderr, "failed open %s.\n", filename);
		return;
	}
	mtime = time(NULL);
	fprintf(file, "%s - %s\n", ctime(&mtime), words);
	fclose(file);
}

/* task 1. AT and AT+CREG? */
void *thread_creg(void *data)
{
	char buf[64];
	char tmp[64];
	char *logid = "creg";
	int ret;
	IDEV *p = (IDEV *)data;
	idev_user_malloc(p);
	while (1) {
		lock_device(p);
		ret = (AT_RETURN)p->send(p, "AT", NULL, AT_MODE_LINE);
		unlock_device(p);
		snprintf(tmp, 64, "cmd AT sent, return is %d.", ret);
		thread_log(p, logid, tmp);
		if (idev_is_sick(p)) break;
		sleep(2);

		lock_device(p);
		ret = (AT_RETURN)p->send(p, "AT+CREG?", buf, AT_MODE_LINE);
		snprintf(tmp, 64, "\"AT+CREG?\" sent, return is %d, recv is %s", 
				ret, buf);
		thread_log(p, logid, tmp);
		unlock_device(p);
		if (idev_is_sick(p)) break;
		sleep(2);
	}
	idev_user_free(p);
	return NULL;
}

/* task 2. Send SMS ? */
void *thread_sms(void *data)
{
	char *logid = "sms";
	int ret;
	IDEV *p = (IDEV *)data;
	idev_user_malloc(p);
	while (1) {
		lock_device(p);
		ret = p->send_sms(p, "10086", "hello 10086.");
		unlock_device(p);
		if (ret)
			thread_log(p, logid, "send sms error.");
		else
			thread_log(p, logid, "send sms successfully.");
		if (idev_is_sick(p)) break;
		sleep(5);
	}
	idev_user_free(p);
	return NULL;
}

/* task 3. Call */
void *thread_call(void *data)
{
	char *logid = "call";
	int ret;
	IDEV *p = (IDEV *)data;
	idev_user_malloc(p);
	while (1) {
		lock_device(p);
		ret = p->send(p, "ATD10086;", NULL, AT_MODE_LINE);
		unlock_device(p);
		if (ret)
			thread_log(p, logid, "send ATD error.");
		else
			thread_log(p, logid, "send ATD successfully.");
		if (idev_is_sick(p)) break;
		sleep(10);

		lock_device(p);
		ret = p->send(p, "ATH", NULL, AT_MODE_LINE);
		unlock_device(p);
		if (ret)
			thread_log(p, logid, "send ATH error.");
		else
			thread_log(p, logid, "send ATH successfully.");
		if (idev_is_sick(p)) break;
		sleep(10);
	}
	idev_user_free(p);
	return NULL;
}

// #define	SIMGLE_THREAD_SINGLE_MODEM	1
#define	MULTI_THERAD_SINGLE_MODEM	1
// #define	MULTI_THREAD_MULTI_MODEM	1

/*************************************************/
#if MULTI_THERAD_MULTI_MODEM
/*************************************************/

void *multi_modem_testing(void *data)
{
	/* start all the tests */
	while (1) {
		if (dev_list.dev[0].active ) {
			IDEV *p = dev_list.dev[0].idev;
			if (p->status == READY) {
				int i;
				void *res;
				IDEV *p;
				pthread_t tid[3];
				p = dev_list.dev[0].idev;

				dm_log(NULL, "device discovered, prepare to test...");

				pthread_create(tid, NULL, thread_creg, (void *)p);
				pthread_create(tid+1, NULL, thread_sms, (void *)p);
				pthread_create(tid+2, NULL, thread_call, (void *)p);
				for (i = 0; i < 3; i++)
					pthread_join(tid[i], &res);
				/* until all dead */
			}
		}
		sleep(5);
	}
	return 0;
}

/*************************************************/
#elif MULTI_THERAD_SINGLE_MODEM
/*************************************************/

void *single_modem_testing(void *data)
{
	/* start all the tests */
	while (1) {
		if (dev_list.dev[0].active ) {
			IDEV *p = dev_list.dev[0].idev;
			if (p->status == READY) {
				int i;
				void *res;
				IDEV *p;
				pthread_t tid[3];
				p = dev_list.dev[0].idev;

				dm_log(NULL, "device discovered, prepare to test...");

				pthread_create(tid, NULL, thread_creg, (void *)p);
				pthread_create(tid+1, NULL, thread_sms, (void *)p);
				pthread_create(tid+2, NULL, thread_call, (void *)p);
				for (i = 0; i < 3; i++)
					pthread_join(tid[i], &res);
				/* until all dead */
			}
		}
		dm_log(NULL, "waiting for active modems ...");
		sleep(5);
	}
	return 0;
}

/* test single modem with multiple threads. */

int main(int argc, char *argv[])
{
	pthread_t main_thread;

	/* init device list */
	dev_list_init();
	/* set all devices to 'not_in_use', which is 0 */
	dev_usage_init();
	/* start testing thread */
	pthread_create(&main_thread, NULL, single_modem_testing, NULL);

	while(1) 
	{
		dm_log(NULL, "start to check device ...");
		device_check();
		sleep(1);
	}

	return 0;
}

/*************************************************/
#elif SINGLE_THREAD_SINGLE_MODEM/* DOING SINGLE TEST */
/*************************************************/

/* test a single module functionality, without the monitoring of iMon */
void self_test(IDEV *p)
{
	p->forward(p, "13811362029", 0);
	while (1) sleep(1);
	return;
}

int main(int argc, char *argv[])
{
	RELATED_DEV rdev;
	IDEV *p;

	rdev.prefix[0] = 0x00;
	rdev.start_num = 1;
	rdev.end_num = 0;

	register_device("ttyUSB0", &rdev, SIM4100);

	if (dev_list.dev[0].active == 0) {
		dm_log(NULL, "error register device.");
		return -1;
	}
	p = dev_list.dev[0].idev;

	while (idev_get_status(p) != READY) 
		sleep(1);

	self_test(p);

	return 0;
}

#endif
