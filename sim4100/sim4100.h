/*
	sim4100.h : codes related to sim4100.
	mailto : xzpeter@gmail.com
*/

#ifndef		__SIM4100_H__
#define		__SIM4100_H__

int sim4100_check_device_file(char *dev_name);
int sim4100_device_file_adoptation(char *type_array, 
		unsigned char *dev_to_take, IDEV_DEV_FILE *dev_file);
int sim4100_module_startup(IDEV *p);

#endif
