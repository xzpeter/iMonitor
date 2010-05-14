/*
	sim4100.c : codes related to sim4100.
	mailto : xzpeter@gmail.com
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../dm.h"
#include "../mod-common.h"

int sim4100_check_device_file(char *dev_name)
{
	int fd, len, ret;
	char *cmd;
	char buf[256];

	snprintf(buf, 256, "%s%s", DEV_DIR, dev_name);

	if ((fd = serial_open_port(buf, 1)) < 0)
		return 0;

	ret = 0;
	cmd = "ATI\r";
	bzero(buf, sizeof(buf));
	write(fd, cmd, strlen(cmd));
	usleep(200000);
	len = read(fd, buf, 256);
	buf[len] = 0x00;

	if (strstr(buf, "SIMCOM_SIM4100")) {
		ret = 1;
	}

	close(fd);

	return ret;
}

int sim4100_device_file_adoptation(char *type_array, 
		unsigned char *dev_to_take, IDEV_DEV_FILE *dev_file)
{
	char *model_array = "1";
	char *pch;
	int n;

	if ((pch = strstr(type_array, model_array)) == NULL)
		/* didn't find the model array */
		return 0;

	n = pch - type_array;
	dev_to_take[n] = 1;

	snprintf(dev_file->base_device, DEV_FILE_NAME_LEN, 
			"ttyUSB%d", n);
	dev_file->related_device.prefix[0] = 0x00; /* null string */
	/* since start_num > end_num, it will ok in any cycles */
	dev_file->related_device.start_num = 1;
	dev_file->related_device.end_num = 0;

	return 1;
}

int sim4100_module_startup(IDEV *p)
{
	int result;
	char buf[256];

	result = daemon_send(p, "ATE0", buf);
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

	while(1) {
		sleep(3);
		result = daemon_send(p, "AT+CREG?", buf);
		if (result)
			return -2;
		if (strstr(buf, "+CREG: 1,1"))
			break;
	}

	result = daemon_send(p, "AT+CMGF=1", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	/* get self SIM card number, and put into p->sim */
	if (common_get_self_sim_number(p, p->sim))
		return -3;

	daemon_flush(p);

	return 0;
}
