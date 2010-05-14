/*
 * file : dm-uid.h
 * desc : all the UID related codes are here
 * mail : xzpeter@gmail.com
 */

#ifndef __DM_UID_H__
#define	__DM_UID_H__

/* CAUTION : this file has a cross-ref with dm-idev.h
 * my solution is a little bit tricky. 
 * if you want to include this file, you should just
 * include 'dm-idev.h'. Or, if you want to use all the
 * IDEV apis, you should only include 'dm.h' and it'll be ok.
 * ARBITARY INCLUDING HEAD MAY CAUSE COMPILATION ERROR. */

#define  UID_STR_LEN_MAX  (64)

/* this is UID structure, which is used in device identification */
typedef struct _uid {
	char type;
	char s[UID_STR_LEN_MAX];
} UID;

/* this is the pointer of UID structure */
typedef UID * PUID;

/* return 0 if pu1 == pu2 */
int uid_compare(PUID pu1, PUID pu2);
/* copy pu2 -> pu1 */
int uid_copy(PUID pu1, PUID pu2);
/* clear uid */
int uid_clear(PUID pu);
/* return type if PUID is valid, or return -1 if err */
char uid_get_type(PUID pu);
/* return pointer to key str(s) of UID if PUID is valid, or return NULL */
char *uid_get_s(PUID pu);

/* to seek for a specific device by simcard 
 * return NULL if not found, or return the PUID */
IDEV * uid_seek_by_sim(char *sim);
/* seek device by UID */
IDEV * uid_seek(PUID pu);
/* fillin a UID structure with 'type' and 'str' */
int uid_fill(PUID pu, char type, char *s);
/* check if the device is available by indexing UID 
 * return 1 if valid, or 0.*/
int uid_is_valid(PUID pu);
/* check if device with UID can be a COMMUNICATOR */
int uid_is_a_comm(PUID pu);
/* check if device with UID can be a TESTEE */
int uid_is_a_testee(PUID pu);

#endif
