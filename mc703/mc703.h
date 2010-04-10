#ifndef		__MC703_H__
#define		__MC703_H__

int mc703_device_file_adoptation(char *type_array, 
		unsigned char *dev_to_take, IDEV_DEV_FILE *dev_file);
int mc703_module_startup(IDEV *p);
int mc703_get_related_device(char *base_dev, RELATED_DEV *prd);
int mc703_check_device_file(char *dev_name);
int mc703_send_sms(IDEV *p, char *who, char *data);


#endif
