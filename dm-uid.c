/*
 * file : dm-uid.c
 * desc : all the UID related codes are here
 * mail : xzpeter@gmail.com
 */

#include <string.h>
#include "dm-idev.h"

/* we HAVE TO include this to finish our task */
#include "dm-base.h"

/* return 0 if pu1 == pu2 */
int uid_compare(PUID pu1, PUID pu2)
{
	if (pu1->type != pu2->type)
		return -1;
	if (strncmp(pu1->s, pu2->s, UID_STR_LEN_MAX))
		return -2;
	return 0;
}

/* copy pu2 -> pu1 */
int uid_copy(PUID pu1, PUID pu2)
{
	if (pu1 == NULL || pu2 == NULL)
		return -1;
	memcpy(pu1, pu2, sizeof(UID));
	return 0;
}

/* clear uid */
int uid_clear(PUID pu)
{
	if (pu == NULL)
		return -1;
	bzero(pu, sizeof(UID));
	return 0;
}

/* to seek for a specific device by simcard 
 * return NULL if not found, or return the IDEV pointer */
IDEV * uid_seek_by_keystr(char *sim)
{
	int i;
	IDEV *p;
	PUID pu;
	for (i = 0; i < MAX_DEVICE_NO; i++) {
		p = dev_list.dev[i].idev;
		pu = &p->uid;
		if (strncmp(pu->s, sim, UID_STR_LEN_MAX))
			continue;
		/* find the device */
		return p;
	}
	return NULL;
}

/* seek UID, return IDEV pointer if exist. */
IDEV * uid_seek(PUID pu)
{
	int i;
	IDEV *p;
	PUID pu2;
	for (i = 0; i < MAX_DEVICE_NO; i++) {
		p = dev_list.dev[i].idev;
		pu2 = &p->uid;
		if (uid_compare(pu, pu2))
			continue;
		/* find the device */
		return p;
	}
	return NULL;
}

/* fill in a UID structure, from 'type' and 'str' keyword */
int uid_fill(PUID pu, char type, char *s)
{
	if (pu == NULL)
		return -1;
	pu->type = type;
	if (strlen(s) == 0)
		return -2;
	strncpy(pu->s, s, UID_STR_LEN_MAX);
	return 0;
}

/* return type if PUID is valid, or return -1 if err */
char uid_get_type(PUID pu)
{
	if (pu == NULL)
		return -1;
	return pu->type;
}

/* return pointer to key str(s) of UID if PUID is valid, or return NULL */
char *uid_get_s(PUID pu)
{
	if (pu == NULL)
		return NULL;
	return pu->s;
}

/* check if the device is available by indexing UID 
 * return 1 if valid, or 0.*/
int uid_is_valid(PUID pu)
{
	IDEV *p = uid_seek(pu);
	if (p == NULL)
		return 0;
	if (idev_is_sick(p))
		return 0;
	return 1;
}

/* check if device with PUID is a xxx */
static int uid_is_a_xxx(PUID pu, IDEV_GROUP group)
{
	/* first, find the device */
	IDEV *p = uid_seek(pu);
	/* not found */
	if (p == NULL)
		return 0;
	/* the device is not healthy ? */
	if (idev_is_sick(p))
		return 0;
	if (idev_get_group(p) & group)
		return 1;
	return 0;
}	

/* check if device with UID can be a COMMUNICATOR */
int uid_is_a_comm(PUID pu)
{
	return uid_is_a_xxx(pu, COMM);
}

/* check if device with UID can be a TESTEE */
int uid_is_a_testee(PUID pu)
{
	return uid_is_a_xxx(pu, TESTEE);
}

