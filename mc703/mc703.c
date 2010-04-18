/*
   lc6311.c : device related codes.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../dm.h"
#include "mc703.h"

/*============================================*/
/* 			some special functions			  */
/*============================================*/
/* return 1 if there is a device to adopt,
   set all the devices that you will take to '1' in dev_to_take list ,
   and also fill IDEV_DEV_FILE struct for further use.
   ps. type_array is something like : "000101000000000", 
 */
int mc703_device_file_adoptation(char *type_array, 
		unsigned char *dev_to_take, IDEV_DEV_FILE *dev_file)
{
	char *model_array = "101";
	char *pch;
	int i, n;

	if ((pch = strstr(type_array, model_array)) == NULL)
		/* didn't find the model array */
		return 0;

	n = pch - type_array;
	for (i = 0; i < 3; i++)
		dev_to_take[n+i] = 1;

	snprintf(dev_file->base_device, DEV_FILE_NAME_LEN, 
			"ttyUSB%d", n+2);
	snprintf(dev_file->related_device.prefix, DEV_FILE_NAME_LEN, 
			"ttyUSB");
	dev_file->related_device.start_num = n;
	dev_file->related_device.end_num = n+1;

	return 1;
}

/* do the startup work for your module, returns 0 if startup ok, 
   else if error.  this is a very special function, we can do normal 
   send and recv in this function, since the module is still in status
   INIT now. normally, send & recv should be done by daemon thread. */
int mc703_module_startup(IDEV *p)
{
	/*

流程：
1:
ATE0
2:
ATV1
3:
AT^PREFMODE=8
4:
AT+CPIN?		---->		OK
5:
sleep(5);
6:
AT+CSQ			---->		+CSQ: 31,99
7:
AT+CPMS="SM","SM","SM"
8:
AT^HSMSSS=0,0,1,0
9:
AT+CNMI=1,1,0,1,0
10:
AT+CMGF=1
11:
AT^PPPCFG="ctwap@mycdma.cn","vnet.mobi"
*/
	int result;
	char buf[256];
    char *pTmp;

	result = daemon_send(p, "ATE0", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	result = daemon_send(p, "ATV1", buf);
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

	result = daemon_send(p, "AT^PREFMODE=8", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	result = daemon_send(p, "AT+CPIN?", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	// wait for checking network
	sleep(5);

	while(1) {
		result = daemon_send(p, "AT+CSQ", buf);
		if (result)
			return -2;
		if ((pTmp = strstr(buf, "+CSQ:")))
        {
            sscanf(pTmp, "+CSQ:%d", &result);
                if (result != 99)
                {
                    break;
                }
                else 
                {
                    printf("Waiting to register(%d): %s", result, pTmp);
                    sleep(3);
                }
            break;        
        }
	}
		
// 	sleep(3);
// 	daemon_flush(p);
	result = daemon_send(p, "AT+CPMS=\"SM\",\"SM\",\"SM\"", buf);
	if (result)
		return -2;
	/* CPMS return 2 times, just skip this first */
// 	if (!strstr(buf, "OK"))
// 		return -1;

	result = daemon_send(p, "AT^HSMSSS=0,0,1,0", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;
		
	result = daemon_send(p, "AT+CNMI=1,1,0,1,0", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	result = daemon_send(p, "AT+CMGF=1", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;

	result = daemon_send(p, "AT^PPPCFG=\"ctwap@mycdma.cn\",\"vnet.mobi\"", buf);
	if (result)
		return -2;
	if (!strstr(buf, "OK"))
		return -1;


	daemon_flush(p);

	return 0;
}

int mc703_check_device_file(char *dev_name)
{
	int fd, len, ret;
	char *cmd;
	char buf[256];

	snprintf(buf, 256, "%s%s", DEV_DIR, dev_name);

	if ((fd = serial_open_port(buf, 1)) < 0)
		return 0;

	ret = 0;
	cmd = "AT+CGMM\r";
	bzero(buf, sizeof(buf));
	write(fd, cmd, strlen(cmd));
	usleep(200000);
	len = read(fd, buf, 256);
	buf[len] = 0x00;

	if (strstr(buf, "MC703")) {
// 		dm_log(NULL, "%s", buf);
		ret = 1;
	}

	close(fd);

	return ret;
}

int mc703_get_related_device(char *base_dev, RELATED_DEV *prd)
{
	int n;
	strcpy(prd->prefix, "ttyUSB");
	sscanf(base_dev + strlen(prd->prefix), "%d", &n);
	if (n < 5)
		return -1;
	prd->start_num = n - 2;
	prd->end_num = n - 1;
	return 0;
}

int mc703_send_sms(IDEV *p, char *who, char *data)
{
	/* p->send must be implemented first */
	char cmd_buf[512];
	int ret;

	/* clear sms return flag for mc703 modem */
	p->private_data.mc703.sms_return = MC703_SMS_RETURN_WAIT;

	/* first, get the ownership of the device */
	//	lock_device(p);
	if ((ret = strlen(who)) >= MAX_MOBILE_ID_LEN) {
		dm_log(p, "CELLID TOO LONG in common_send_sms(), [len=%d] : %s", ret, who); goto send_sms_error; }
	if (strlen(data) >= 500) {
		dm_log(p, "DATA TOO LONG in common_send_sms(), [len=%d].", data);
		goto send_sms_error;
	}
	ret = snprintf(cmd_buf, 510, "AT^HCMGS=\"%s\"", who);
	/* send AT+HCMGS cmd */
	ret = (int)p->send(p, cmd_buf, NULL, AT_MODE_LINE);
	if (ret != AT_RAWDATA) {
		dm_log(p, "SEND SMS cmd error, it's %d, should %d.", 
				ret, AT_RAWDATA);
		goto send_sms_error;
	}
	sleep(1);
	/* send rawdata */
	snprintf(cmd_buf, 510, "%s\x1A", data);
	ret = (int)p->send(p, cmd_buf, NULL, AT_MODE_LINE);
	if (ret != AT_OK) {
		dm_log(p, "SEND RAW DATA error, return is %d, should %d.", 
				ret, AT_OK);
		goto send_sms_error;
	}
	/* send ok */

	while (p->private_data.mc703.sms_return == MC703_SMS_RETURN_WAIT) {
		if (idev_is_sick(p)) 
			goto send_sms_error;
		usleep(300000);
	}

	dm_log(p, "SMS sent : %s ---> %s", data, who);
//	unlock_device(p);
	return 0;
	
send_sms_error:
//	unlock_device(p);
	return -1;

}

int mc703_parse_line(IDEV *p, char *line)
{
	/* first, we should notice a flag if a sms received */
	if (strstr(line, "^HCMGSS"))
		p->private_data.mc703.sms_return = MC703_SMS_RETURN_OK;
	else if (strstr(line, "^HCMGSF"))
		p->private_data.mc703.sms_return = MC703_SMS_RETURN_ERR;

	return 0;
}

int mc703_start_call(IDEV *p, char *who)
{
	char cmd[64];
	int n = strlen(who);
	if (n < 0 || n > 20)
		return AT_NOT_RET;
	snprintf(cmd, 64, "AT+CDV%s", who);
	return p->send(p, cmd, NULL, AT_MODE_LINE);
}


int mc703_stop_call(IDEV *p)
{
	return p->send(p, "AT+CHV", NULL, AT_MODE_LINE);
}

int mc703_network_status(IDEV *p, char *buf)
{
	return p->send(p, "AT^SYSINFO", buf, AT_MODE_LINE);
}
