#ifndef		__MOD_COMMON_H__
#define		__MOD_COMMON_H__

int common_send_sms(IDEV *p, char *who, char *data);

AT_RETURN common_send(IDEV *p, char *data, char *result, int mode);

ssize_t common_rawsend(IDEV *p, char *buf, int n);

ssize_t common_rawrecv(IDEV *p, char *buf, int n);

int common_device_file_adoptation(char *type_array, 
		unsigned char *dev_to_take, IDEV_DEV_FILE *dev_file);

/* startup codes for modems */
int common_module_startup(IDEV *p);

/* check if the device file 'file' is the specific type */
int common_check_device_file(char *file);

/* return related devices refers to base device */
int common_get_related_device(char *base_dev, RELATED_DEV *prd);

/* close a port */
void common_close_port(IDEV *p);

/* common open a port */
int common_open_port(IDEV *pidev);

/* forward a sms */
int common_forward(IDEV *p, char *who, int n);

#endif
