/*
   dm-idev.c : device monitor base structure and functions
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "dm.h"
#include "rbuf.h"
#include "mod-common.h"
#include "lc6311/lc6311.h"
#include "sim4100/sim4100.h"
#include "nms.h"

const DEV_MODEL dev_model[SUPPORTED_DEVICES] = {
	{
		.name					= "UNKNOWN", 
		.open_port 				= common_open_port, 
		.close_port				= common_close_port, 
		.check_device_file 		= common_check_device_file, 
		.get_related_device		= common_get_related_device,
		.module_startup			= common_module_startup,
		.device_file_adoptation	= common_device_file_adoptation,
		.send					= common_send,
		.send_sms				= common_send_sms,
		.forward				= common_forward,
	},
	{
		.name					= "LC6311", 
		.open_port 				= common_open_port, 
		.close_port				= common_close_port, 
		.check_device_file 		= lc6311_check_device_file, 
		.get_related_device		= lc6311_get_related_device,
		.module_startup			= lc6311_module_startup,
		.device_file_adoptation	= lc6311_device_file_adoptation,
		.send					= common_send,
		.send_sms				= common_send_sms,
		.forward				= common_forward,
	}, 
	{
		.name					= "SIM4100", 
		.open_port 				= common_open_port, 
		.close_port				= common_close_port, 
		.check_device_file 		= sim4100_check_device_file, 
		.get_related_device		= common_get_related_device,
		.module_startup			= sim4100_module_startup,
		.device_file_adoptation	= sim4100_device_file_adoptation,
		.send					= common_send,
		.send_sms				= common_send_sms,
		.forward				= common_forward,
	}
};

/* call this to check if your at cmd has recved */
int check_at_recved(IDEV *p)
{
	return (p->at.status == AT_RECVED);
}

/* if you have collected all the data you need in AT_BUF, call it to 
   tell the daemon. */
void at_recved_done(IDEV *p)
{
	idev_lock(p);
	p->at.status = AT_READY;
	idev_unlock(p);
}

/* check if at buf is ready for your command */
int check_at_ready(IDEV *p)
{
	return (p->at.status == AT_READY);
}

/* when you have fulfilled at_sendbuf and keyword as you want, call it 
   to let daemon know. */
void at_send_it(IDEV *p)
{
	idev_lock(p);
	p->at.status = AT_REQUEST;
	idev_unlock(p);
}

int idev_lock(IDEV *p)
{
	return pthread_mutex_lock(&p->mutex);
}

int idev_unlock(IDEV *p)
{
	return pthread_mutex_unlock(&p->mutex);
}

int lock_device(IDEV *p)
{
	return pthread_mutex_lock(&p->dev_mutex);
}

int unlock_device(IDEV *p)
{
	return pthread_mutex_unlock(&p->dev_mutex);
}

/* ONLY IF we do the assignment of the functions here, 
   can we use these functions in the host thread later on. */
void idev_register_methods(IDEV *pidev, IDEV_TYPE type)
{
	pidev->open_port = dev_model[type].open_port;
	pidev->close_port = dev_model[type].close_port;
	pidev->module_startup = dev_model[type].module_startup;
	pidev->send = dev_model[type].send;
	pidev->send_sms = dev_model[type].send_sms;
	pidev->forward = dev_model[type].forward;
}

#define		RBUF_SIZE			4096

/* init a idevice, the name string is used to identify the device, 
   type is used to decide which driver the device will use. */
IDEV * idev_init(char *name, RELATED_DEV *rdev, IDEV_TYPE type)
{
	IDEV *pidev = NULL;

	/* do the malloc */
	pidev = malloc(sizeof(IDEV));
	if (pidev == NULL)
		return NULL;
	bzero(pidev, sizeof(IDEV));

	/* malloc for rbuf */
	pidev->r = rbuf_init(RBUF_SIZE);
	if (pidev->r == NULL)
		goto free_idev;

	if (pidev) {
		/* malloc ok, do the initialize */
		pidev->enabled = 0;
		/* type cannot be changed after init of the idev */
		pidev->type = type;
		/* default group is NONE */
		pidev->group = NONE;
		/* default status is INIT */
		pidev->status = INIT;
		/* init mutex */
		// pidev->mutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_init(&(pidev->mutex), NULL);
		pthread_mutex_init(&(pidev->dev_mutex), NULL);

		strcpy(pidev->dev_file.base_device, name);
		strcpy(pidev->name, name);
		memcpy(&pidev->dev_file.related_device, rdev, sizeof(RELATED_DEV));

		/* register related model functions to this device */
		idev_register_methods(pidev, type);
	}

	return pidev;

free_idev:
	free(pidev);
	return NULL;
}

/* free a idev structure */
void idev_release(IDEV *pidev)
{
	/* I don't know if it is ok or not. */
	/* TOFIX: how to free a structure with mutex ? */
	lock_device(pidev);
	idev_lock(pidev);
	idev_unlock(pidev);
	unlock_device(pidev);

	if (pthread_mutex_destroy(&pidev->mutex) || 
		pthread_mutex_destroy(&pidev->dev_mutex)) {
		dm_log(pidev, "VITAL!! mutex is destroyed while some thread "
				"is using it!");
	}
	pidev->close_port(pidev);
	pidev->r = rbuf_release(pidev->r);
	free(pidev);
}

void idev_set_enable(IDEV *pidev)
{
	idev_lock(pidev);
	pidev->enabled = 1;
	idev_unlock(pidev);
}	

int idev_get_enable(IDEV *pidev)
{
	return pidev->enabled;
}

void idev_set_status(IDEV *pidev, IDEV_STATUS status)
{
	idev_lock(pidev);
	pidev->status = status;
	idev_unlock(pidev);
	dm_log(pidev, "change status to %d.", status);
}

IDEV_STATUS idev_get_status(IDEV *pidev)
{
	return pidev->status;
}

void idev_set_disable(IDEV *pidev)
{
	idev_lock(pidev);
	pidev->enabled = 0;
	idev_unlock(pidev);
}	

void idev_set_group(IDEV *pidev, IDEV_GROUP group)
{
	idev_lock(pidev);
	pidev->group = group;
	idev_unlock(pidev);
}

int idev_check_active(IDEV *pidev)
{
	/* if the device's "if00-port0" device file is still here, 
	   we say it's still alive. */
// 	char file_name[FULL_FILE_NAME_LEN];
// 	format_full_dev_file_name(file_name, pidev->name, 0, 0);
// 	return (!access(file_name, F_OK));
	return 1;
}
