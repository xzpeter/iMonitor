/*
	nms.c : some old NMS codes copied here.
	mailto : xzpeter@gmail.com
 */

#include "dm.h"
#include <string.h>

#define		MAX_MOBILE_IDLEN    			11
#define		CHAR(n)							((n)+0x30)
#define		uchar							unsigned char

void format_one_number(char *pbuf, int num, char width)
{
	uchar i;

	if (pbuf == 0x00 || width == 0x00)
		return;

	i = 0;

	if (width > 8) {
		memset(pbuf, 0x30, width-8);
		i = width - 8;
	}
	if (width >= 8) {
		pbuf[i++] = CHAR((num/10000000)%10);
	}
	if (width >= 7) {
		pbuf[i++] = CHAR((num/1000000)%10);
	}
	if (width >= 6) {
		pbuf[i++] = CHAR((num/100000)%10);
	}	
	if (width >= 5) {
		pbuf[i++] = CHAR((num/10000)%10);
	}
	if (width >= 4) {
		pbuf[i++] = CHAR((num/1000)%10);
	}
	if (width >= 3) {
		pbuf[i++] = CHAR((num/100)%10);
	}
	if (width >= 2) {
		pbuf[i++] = CHAR((num/10)%10);
	}
	pbuf[i] = CHAR(num%10);
}

/*	who: 		cell phone number
	smsdata:	data to forward
	len: 		length of data to forward.  */
int do_balance_forward(IDEV *p, char *who, char *smsdata, int len)
{
	int    i, j, ret;
	int    len1, len2;
	char paralist[1024], buf[1024];

	if(len > 220)
		len = 220;

	len &= 0xFC;	// 使其成为4的倍数

	len1 = 15 + (len/2); 	// CMGS所用的长度
	len2 = len/2;			// PDU内包含的长度

// 	send_AT_command("AT+CMGF=0");
	p->send(p, "AT+CMGF=0", NULL, AT_MODE_LINE);

	format_one_number(paralist, len1, 3);
	paralist[3] = 0x00;
// 	send_AT_command("AT+CMGS=");
	strcpy(buf, "AT_CMGS=");
	strcat(buf, paralist);
	ret = p->send(p, buf, NULL, AT_MODE_LINE);

	strcpy(paralist, "0011000D9168");
	j = 12;

	for(i=0; i<MAX_MOBILE_IDLEN; i++) {
		if(i%2 == 0)
			paralist[j+i+1] = who[i];
		else
			paralist[j+i-1] = who[i];
	}
	paralist[j+MAX_MOBILE_IDLEN-1] = 'F';
	j += MAX_MOBILE_IDLEN+1;

	strcpy(paralist+j, "0008A0");
	j += 6;

	if((len2/16) > 9)
		paralist[j++] = 0x41 + (len2/16 - 10);
	else
		paralist[j++] = 0x30 + (len2/16);

	if((len2&0x0F) > 9)
		paralist[j++] = 0x41 + ((len2&0x0F) - 10);
	else
		paralist[j++] = 0x30 + (len2&0x0F);

	memcpy(paralist+j, smsdata, len);
	j += len;

	paralist[j++]=0x1A;/* must end with 0x1A to use send() */
// 	paralist[j++]=0x00;
// 	send_AT_command(RAWDATA);
	ret = p->send(p, paralist, NULL, AT_MODE_LINE);

	// 重新设置回文本格式短信息模式
// 	send_AT_command("AT+CMGF=1");
	p->send(p, "AT+CMGF=1", NULL, AT_MODE_LINE);
	return ret;
}
