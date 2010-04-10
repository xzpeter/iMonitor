#ifndef		__DM_UTILS_H__
#define		__DM_UTILS_H__

#define		DEV_DIR					"/dev/"
#define		FILE_NAME_LEN			256
#define		FULL_FILE_NAME_LEN		512

#include "dm-idev.h"

extern int daemon_send(IDEV *p, char *cmd, char *d_at_buf);
extern void daemon_flush(IDEV *p);
extern int serial_open_port(char *portdevice, int block);
extern void split_dev_filename(char *file, char *key, int *n);
extern int find_device_name(char *fullname, char *dev_name);
extern void format_full_dev_file_name(char *file_name, char *dev_name, 
	int ifcon, int port);
extern void show_device_info(IDEV *pidev);
extern void *device_daemon(void *pidev);

#endif
