/*
   dm-idev.h : device monitor base structure and functions
*/

#ifndef		__DM_BASE_H__
#define		__DM_BASE_H__

#include <pthread.h>
#include "rbuf.h"

typedef void *(* handler)(void *);
typedef void *(* handler2)(void *, void *);
typedef void *(* handler3)(void *, void *, void *);

/* how many devices are supported */
#define		SUPPORTED_DEVICES				3
typedef enum _idev_type {
	UNKNOWN		= 0,
	LC6311		= 1,
	SIM4100		= 2,
// 	ETHERNET	= 3,
// 	WLAN		= 4, 
} IDEV_TYPE;

typedef enum _idev_group {
	NONE	=	0, 
	COMM	=	1,
	TESTEE	=	2, 
	BOTH	=	3,	/* BOTH = COMM & TESTEE */
} IDEV_GROUP;

typedef enum _idev_status {
	INIT	=	1,
	READY	=	2, 
	WORKING	=	3, 
	DEAD	=	99
} IDEV_STATUS;

typedef enum _at_status {
	AT_READY, 		/* normal stat, after host picked the results, HOST writable */
	AT_REQUEST, 	/* after the host sends a request, DAEMON writable */
	AT_SENT, 		/* after the daemon sent the cmd, DAEMON writable */
	AT_RECVED, 		/* after the daemon recved the response, HOST writable */
} AT_STATUS;

typedef enum _at_return {
	AT_NOT_RET		=	-1, 	/* not returned */
	AT_OK			=	0, 		/* return "OK" */
	/* CAUTION! if we got AT_RAWDATA returned, don't forget a 'Ctrl^z' then. */
	AT_RAWDATA		=	1,		/* return ">", waiting for SMS input. */
	AT_ERROR		=	2,		/* return "ERROR" */
	AT_CME_ERROR	=	3,		/* return "AT_CME_ERROR" */
	AT_CMS_ERROR	=	4,		/* return "AT_CMS_ERROR" */
	AT_HWERROR		=	5, 		/* hardware error */
} AT_RETURN;

#define		IDEV_SEND_BUFLEN	128
#define		IDEV_RECV_BUFLEN	256
#define		IDEV_KEYWORD_LEN	32

typedef struct _idev_at_buf {
	char send_buf[IDEV_SEND_BUFLEN];
	char recv_buf[IDEV_RECV_BUFLEN];
	char keyword[IDEV_KEYWORD_LEN];
	AT_STATUS status;
	AT_RETURN ret;
} IDEV_AT_BUF;

#define		IDEV_NAME_LEN		256
#define		DEV_FILE_NAME_LEN	64
#define		GLOBAL_BUF_LEN		1024

typedef struct _related_dev {		/* range of related dev */
	char prefix[DEV_FILE_NAME_LEN];
	int start_num;
	int end_num;
} RELATED_DEV;

typedef struct _idev_dev_file {		/* related device files */
	char base_device[DEV_FILE_NAME_LEN];	/* base dev file */
	RELATED_DEV related_device;
} IDEV_DEV_FILE;

typedef struct _idev IDEV;

struct _idev {
	/* idev data structures */
	RBUF *r;					/* a simple round buffer */
	IDEV_AT_BUF at;				/* at cmd buffer */
	char name[IDEV_NAME_LEN];	/* device (file) name */
	IDEV_TYPE type;				/* decide which driver to use */
	IDEV_GROUP group;			/* COMM, TESTEE or IDLE */ 
	int enabled;				/* if the device is activated */
	pthread_t thread_id;		/* thread id of the listening thread */
	IDEV_STATUS status;			/* INIT, READY, WORKING, DEAD */
	pthread_mutex_t mutex;		/* mutex of the idev struct */
	pthread_mutex_t dev_mutex;	/* mutex of the abstract device */
	int portfd;					/* file handler of the serial port */
	IDEV_DEV_FILE dev_file;		/* keep related device files */

	/* idev methods */
	int (*open_port)(IDEV *);			/* open comm port, e.g. serial */
	void (*close_port)(IDEV *);			/* close comm port, e.g. serial */
	int (*module_startup)(IDEV *);		/* do the start up work, not using at_buf */
	AT_RETURN (*send)(IDEV *, char *, char *);				/* use this to send a at cmd (buffered) */
	int (*send_sms)(IDEV *, char *, char *);			/* send a SMS using send() */
	int (*rawsend)(IDEV *, char *, int );
	int (*forward)(IDEV *, char *, char *, int );
};

typedef struct _dev_model {
	char name[32];
	/* only arg is a file name, returns 1 if the file belongs to it */
	int (*open_port)(IDEV *);			/* open comm port, e.g. serial */
	void (*close_port)(IDEV *);			/* close comm port, e.g. serial */
	int (*check_device_file)(char *);
	int (*get_related_device)(char *, RELATED_DEV *);
	int (*module_startup)(IDEV *);		/* do the start up work, not using at_buf */
	int (*device_file_adoptation)(char *, unsigned char *, IDEV_DEV_FILE *);
	int (*send)(IDEV *, char *, char *);
	int (*send_sms)(IDEV *, char *, char *);			
	int (*rawsend)(IDEV *, char *, int );
	int (*forward)(IDEV *, char *, char *, int );
} DEV_MODEL;

/* keep module specified infomations */
extern const DEV_MODEL dev_model[SUPPORTED_DEVICES];

/* host related functions */
extern int check_at_recved(IDEV *p);
extern void at_recved_done(IDEV *p);
extern int check_at_ready(IDEV *p);
extern void at_send_it(IDEV *p);

/* other idev related API functions */

extern IDEV * idev_init(char *name, RELATED_DEV *rdev, IDEV_TYPE type);
extern void idev_release(IDEV *p_idev);
extern void idev_set_enable(IDEV *p_idev);
extern int idev_get_enable(IDEV *p_idev);
extern void idev_set_disable(IDEV *p_idev);
extern void idev_set_group(IDEV *p_idev, IDEV_GROUP group);
// extern int idev_check_active(IDEV *p);
extern void idev_set_status(IDEV *p_idev, IDEV_STATUS status);
extern IDEV_STATUS idev_get_status(IDEV *p_idev);
extern int idev_lock(IDEV *p);
extern int idev_unlock(IDEV *p);
int lock_device(IDEV *p);
int unlock_device(IDEV *p);

#endif
