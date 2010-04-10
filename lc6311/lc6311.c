/*
   lc6311.c : device related codes.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../dm.h"

/*============================================*/
/* 			some special functions			  */
/*============================================*/
/* return 1 if there is a device to adopt,
   set all the devices that you will take to '1' in dev_to_take list ,
   and also fill IDEV_DEV_FILE struct for further use.
   ps. type_array is something like : "000101000000000", 
 */
int lc6311_device_file_adoptation(char *type_array, 
		unsigned char *dev_to_take, IDEV_DEV_FILE *dev_file)
{
	char *model_array = "000101";
	char *pch;
	int i, n;

	if ((pch = strstr(type_array, model_array)) == NULL)
		/* didn't find the model array */
		return 0;

	n = pch - type_array;
	for (i = 0; i < 6; i++)
		dev_to_take[n+i] = 1;

	snprintf(dev_file->base_device, DEV_FILE_NAME_LEN, 
			"ttyUSB%d", n+5);
	snprintf(dev_file->related_device.prefix, DEV_FILE_NAME_LEN, 
			"ttyUSB");
	dev_file->related_device.start_num = n;
	dev_file->related_device.end_num = n+4;

	return 1;
}

/* do the startup work for your module, returns 0 if startup ok, 
   else if error.  this is a very special function, we can do normal 
   send and recv in this function, since the module is still in status
   INIT now. normally, send & recv should be done by daemon thread. */
int lc6311_module_startup(IDEV *p)
{
	/*

流程：
1:
AT+CFUN=1
2:
AT+COPS=0
3:
AT+CREG?		--->		+CREG: 1,1
4:
AT+CPMS="SM","SM","SM"
5:
AT+CMGF=1
6:
AT+CSCS="GSM"
*/
	int result;
	char buf[256];

	result = daemon_send(p, "ATE=0", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	result = daemon_send(p, "AT+CFUN=1", buf);
	if (result)
		return -2;
	/* wait for some time to get the OK */
	sleep(3);
	/* just read */
	result = daemon_send(p, "AT", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	result = daemon_send(p, "AT+COPS=0", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	while(1) {
		sleep(3);
		result = daemon_send(p, "AT+CREG?", buf);
		if (result)
			return -2;
		if (strstr(buf, "+CREG: 1,1"))
			break;
	}
		
// 	sleep(3);
// 	daemon_flush(p);
	result = daemon_send(p, "AT+CPMS=\"SM\",\"SM\",\"SM\"", buf);
	if (result)
		return -2;
	/* CPMS return 2 times, just skip this first */
// 	if (!strstr(buf, "OK"))
// 		return -1;

	result = daemon_send(p, "AT+CMGF=1", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	result = daemon_send(p, "AT+CSCS=\"GSM\"", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	result = daemon_send(p, "AT+CSMP=145,71,32,0", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	daemon_flush(p);

	return 0;
}

int lc6311_check_device_file(char *dev_name)
{
	int fd, len, ret;
	char *cmd;
	char buf[256];

	snprintf(buf, 256, "%s%s", DEV_DIR, dev_name);

	if ((fd = serial_open_port(buf, 1)) < 0)
		return 0;

	ret = 0;
	cmd = "AT+CGMR\r";
	bzero(buf, sizeof(buf));
	write(fd, cmd, strlen(cmd));
	usleep(200000);
	len = read(fd, buf, 256);
	buf[len] = 0x00;

	if (strstr(buf, "+CGMR: LC6311")) {
// 		dm_log(NULL, "%s", buf);
		ret = 1;
	}

	close(fd);

	return ret;
}

int lc6311_get_related_device(char *base_dev, RELATED_DEV *prd)
{
	int n;
	strcpy(prd->prefix, "ttyUSB");
	sscanf(base_dev + strlen(prd->prefix), "%d", &n);
	if (n < 5)
		return -1;
	prd->start_num = n - 5;
	prd->end_num = n - 1;
	return 0;
}
