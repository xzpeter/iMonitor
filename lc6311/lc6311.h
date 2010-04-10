#ifndef		__LC6311_H__
#define		__LC6311_H__

int lc6311_device_file_adoptation(char *type_array, 
		unsigned char *dev_to_take, IDEV_DEV_FILE *dev_file);
int lc6311_module_startup(IDEV *p);
int lc6311_get_related_device(char *base_dev, RELATED_DEV *prd);
int lc6311_check_device_file(char *dev_name);

#endif
