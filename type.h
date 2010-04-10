#ifndef TYPE_H_
#define TYPE_H_

typedef unsigned char byte;
typedef unsigned char uchar;
typedef unsigned char uint8;

typedef unsigned int uint;
typedef unsigned short ushort;

typedef unsigned int uint32;
typedef signed int sint32;

typedef unsigned char 	AT_STAT;

#ifndef NULL
#define NULL	(void *)0
#endif

#define  TRUE           1
#define  FALSE          0

#define true		1
#define false		0

#define SETCC_MODE		1
#define CHECKCC_MODE	0

#define ACCESSMODE_NONE			0
#define ACCESSMODE_GSM			1
#define ACCESSMODE_GSMCOMPACT		2
#define ACCESSMODE_TDD			3


#define ISDIGIT(c)	(((c) >= 0x30) && ((c) <= 0x39))
#define NOTDIGIT(c) (((c) < 0x30) || ((c) > 0x39))
#define DIGIT(c)	((c)-0x30)
#define CHAR(n)		((n)+0x30)

#endif /* TYPE_H_ */
