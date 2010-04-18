/*
   dm-utils.c : misc functions for device manager
*/

#define	_GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <term.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "dm.h"

/* split 'ttyUSB12' to 'ttyUSB' and '12' */
void split_dev_filename(char *file, char *key, int *n)
{
	sscanf(file, "%[^0-9]s", key);
	sscanf(file + strlen(key), "%d", n);
}

void daemon_flush(IDEV *p)
{
	char buf[256];
	int len = 1;
	while(len) {
		sleep(1);
		len = read(p->portfd, buf, 256);
		buf[len] = 0x00;
		dm_log(p, "recv : [flushed] %s", buf);
	}
}

/* since daemon itself cannot help itself to do the send(), 
   we need a specialized send() for the daemon. */
/* here, d_at_buf may overflow, we should use a bbig one, at least 256 bytes. */
int daemon_send(IDEV *p, char *cmd, char *d_at_buf)
{
	int len, cmd_len;
	char d_at_cmd[128];
	cmd_len = strlen(cmd);
	if (++cmd_len >= 127) {
		dm_log(p, "cmd_len is %d, may overflow. (buffer size is %d).", 
				cmd_len, 128);
		return -1;
	}
	strcpy(d_at_cmd, cmd);
	strcat(d_at_cmd, "\r");
	dm_log(p, "send : %s", d_at_cmd);
	len = write(p->portfd, d_at_cmd, cmd_len);
	if (len != cmd_len) {
		dm_log(p, "ERROR ATCMD at daemon_at_cmd(), wrote_len(%d), cmd_len(%d).", 
			len, cmd_len);
		return -1;
	}
	sleep(1);
	len = read(p->portfd, d_at_buf, 256);
	if (len >= 256) {
		dm_log(p, "BUF MIGHT OVERFLOWED in daemon_at_cmd(). now size is %d.", 256);
		return -1;
	}
	d_at_buf[len] = 0x00;
	dm_log(p, "recv : %s", d_at_buf);
	return 0;
}

/* print thread info, used in debugging. */
void show_device_info(IDEV *pidev)
{
	dm_log(pidev, "name\t: %s", pidev->name);
	dm_log(pidev, "type\t: %d", pidev->type);
	dm_log(pidev, "group\t: %d", pidev->group);
	dm_log(pidev, "enabled\t: %d", pidev->enabled);
	dm_log(pidev, "thread\t: %u", (unsigned int)pidev->thread_id);
	dm_log(pidev, "status\t: %u", pidev->status);
}

#define		IO_WAIT_TIME		5		/* units of 100ms */

int serial_open_port(char *portdevice, int block)
{
	int portfd, mode;
	struct termios tty;

	mode = O_RDWR;

	portfd = open(portdevice, mode);

	if (portfd == -1) {
		dm_log(NULL, "Can't Open Serial Port:%s\n",portdevice);
		return -1;
	}

	tcgetattr(portfd, &tty);
	/*串口参数：115200bps，使用非规范方式，采用软件流控制*/

	//设置输入输出波特率为115200bps
	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	//使用非规范方式
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE);

	//采用软件流控制，不使用硬件流控制
	tty.c_iflag |= (IXON | IXOFF | IXANY);
	tty.c_cflag &= ~CRTSCTS;

	/* 8N1,不使用奇偶校验 */

	//传输数据时使用8位数据位
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;

	//一位停止位,不使用奇偶校验
	tty.c_cflag &= ~(PARENB | CSTOPB);

	//忽略DCD信号，启用接收装置
	tty.c_cflag |= (CLOCAL | CREAD);

	/* Minimum number of characters to read */
	tty.c_cc[VMIN] = 0;
	/* Time to wait for data (tenths of seconds)*/
	tty.c_cc[VTIME] = IO_WAIT_TIME;

	/* 使设置立即生效 */
	tcsetattr(portfd, TCSANOW, &tty);

	return portfd;
}

void at_buffer_init(IDEV_AT_BUF *at)
{
	bzero(at->send_buf, sizeof(at->send_buf));
	bzero(at->recv_buf, sizeof(at->recv_buf));
	bzero(at->keyword, sizeof(at->keyword));
	at->ret = AT_NOT_RET;
	at->status = AT_READY;
}

void do_init_work(IDEV *p)
{
	int startup_ok, retry;
	/* the module is enabled by main process */
	dm_log(p, "device enabled ... prepare to startup ...");

	/* try to open port */
	if (p->open_port(p)) {
		dm_log(p, "open_port() failed.");
		idev_set_status(p, DEAD);
		return;
	}

	dm_log(p, "PORT OPEN successfully. fd = %d.", 
			p->dev_file.base_device, p->portfd);

	startup_ok = 0;
	retry = 3;
	/* try startup */
	while (retry--) {
		if (!p->module_startup(p)) {
			startup_ok = 1;
			break;
		}
	}

	if (!startup_ok) {
		dm_log(p, "device startup failed.");
		idev_set_status(p, DEAD);
		return;
	}
	dm_log(p, "STARTUP ok.");

	/* start up ok */
	at_buffer_init(&p->at);
	idev_set_status(p, READY);
}

/* all the infomation handles in here */
int handle_line(IDEV *p, char *line)
{
	/* modem specified function to parse the line */
	p->parse_line(p, line);
	return 0;
}

/* this function is repeated called in daemon normal work.
   it's job is to help the host do the send & recv work of at cmd. 
   there should be 5 status, any of the status can only be changed 
   by 'host' OR 'daemon', never both. */
void do_send_and_recv(IDEV *p)
{
	IDEV_AT_BUF *at = &p->at;
	RBUF *r = p->r;

	/* do the responses */
	switch (at->status) {
		case AT_READY:
			break;
		case AT_REQUEST:
			{
				/* host has announced a query cmd */
				int datalen, sent;
				datalen = strlen(at->send_buf);
				dm_log(p, "send : %s", at->send_buf);
				sent = write(p->portfd, at->send_buf, datalen);
				if (sent != datalen) {
					dm_log(p, "ERROR WRITE PORT, sent is %d, datalen %d.", 
							sent, datalen);
					at->ret = AT_NOT_RET;
					at->status = AT_RECVED;
					break;
				}
				/* clear recv buffer */
				bzero(at->recv_buf, sizeof(at->recv_buf));
				at->status = AT_SENT;
				sleep(1);
				break;
			}
		case AT_SENT:
			break;
		case AT_RECVED:
			break;
	}

	/* do the daily recv */
	{
		unsigned int len, done;
		char buf[256];
		char *pch, *pch2;
		len = read(p->portfd, buf, 255);

		if (len > 0) {
			/* do some special treatment to string "\n>" when using SMS 
			   at cmd. solution is : add another "\n" after '\n>' */
			buf[len] = 0x00;
			dm_log(p, "recv : %s", buf);
			if (!strncmp(buf, "> ", 2))
				pch = buf;	/* "> " is found at the beginning (normally) */
			else
				pch = strstr(buf, "\n> ");	/* found in the middle */
			if (pch) {		/* no matter where, we found "> " */
				pch += 2;
				for (pch2 = buf+len-1; pch2 > pch; pch2--)
					*(pch2+1) = *pch2;
				*(pch2+1) = '\n';
				len++;
			}
			/* after this, "\n>" are converted to "\n>\n", 
			   now we can recog it.  so tricky ... */

			done = r->write(r, buf, len);
			if (done != len) {
				dm_log(p, "RBUF wrote length %d, while %d needed to write, "
					"possibly the buffer is not big enough.", done, len);
			}
		} else if (len < 0) {
			dm_log(p, "error in read(): %s", strerror(errno));
			idev_set_status(p, DEAD);
			return;
		}
	}

	/* handle related infomation */
	{
		char *line;
		while ((line = r->readline(r))) {
// 			dm_log(p, "recv : %s", line);
			/* recved a whole line */
			if (at->status == AT_SENT) {

				/* record required response */
				if (at->mode & AT_MODE_LINE) {
					if (strcasestr(line, at->keyword))
						/* always the newest! */
						strcpy(at->recv_buf, line);
				} else { /* mode BLOCK */
					strcat(at->recv_buf, line);
				}

				/* host is waiting for "OK" and "keyword" */
				if (strstr(line, "OK")) {
					at->ret = AT_OK;
					at->status = AT_RECVED;
				} else if (strstr(line, "CME ERROR")) {
					at->ret = AT_CME_ERROR;
					at->status = AT_RECVED;
				} else if (strstr(line, "CMS ERROR")) {
					at->ret = AT_CMS_ERROR;
					at->status = AT_RECVED;
				} else if (strstr(line, "ERROR")) {
					at->ret = AT_ERROR;
					at->status = AT_RECVED;
				} else if (!strncmp(line, "> ", 2)) {
					at->ret = AT_RAWDATA;
					at->status = AT_RECVED;
				} 
			}
			handle_line(p, line);
		}
	}
}

/* daemon thread, to take full control of a device. */
void *device_daemon(void *pdata)
{
	IDEV *p = (IDEV *)pdata;
	dm_log(p, "DAEMON STARTED.");
// 	show_device_info(p);

	/* daemon main process */
	while (1) {
		switch (idev_get_status(p)) {
			case INIT:
				if (idev_get_enable(p))
					do_init_work(p);
				break;
			case READY:
			case WORKING:
				do_send_and_recv(p);
				/* do send and recv */
				break;
			case UNTACHED:
				dm_log(p, "UNTACHED ?! Roger ! now there are %d users"
						" accessing the modem.", p->users_count);
				idev_set_status(p, WAIT_USERS);
				break;
			case WAIT_USERS:
				if (p->users_count == 0) {
					dm_log(p, "all users acknowledged. marking DEAD.");
					idev_set_status(p, DEAD);
				}
				break;
			case DEAD:
				dm_log(p, "hello main proc, I am dead ...");
				sleep(10);
				break;
			default:
				break;
		}
		usleep(100000);
	}

	return (void *)0;
}
