/*
 * test-case.h : test cases for iMonitor
 * mailto : xzpeter@gmail.com
 */
void thread_log(IDEV *p, char *test_type, char *words)
{
	char filename[64];
	FILE *file;
	time_t mtime;
	IDEV_TYPE type;

	type = p->type;

	snprintf(filename, 64, "log-%s-%s.txt", dev_model[type].name, test_type);
	file = fopen(filename, "a");
	if (file == NULL) {
		fprintf(stderr, "failed open %s.\n", filename);
		return;
	}
	mtime = time(NULL);
	fprintf(file, "%s - %s\n", ctime(&mtime), words);
	fclose(file);
}

/* task 1. AT and AT+CREG? */
void *thread_creg(void *data)
{
	char buf[64];
	char tmp[64];
	char *logid = "creg";
	int ret;
	IDEV *p = (IDEV *)data;
	idev_user_malloc(p);
	while (1) {
		lock_device(p);
		ret = p->probe(p);
		unlock_device(p);
		snprintf(tmp, 64, "AT sent, return is %d.", ret);
		thread_log(p, logid, tmp);
		if (idev_is_sick(p)) break;
		sleep(2);

		lock_device(p);
// 		ret = (AT_RETURN)p->send(p, "AT+CREG?", buf, AT_MODE_LINE);
		ret = p->network_status(p, buf);
		snprintf(tmp, 64, "REQ SYSINFO, return is %d, recv is %s", 
				ret, buf);
		thread_log(p, logid, tmp);
		unlock_device(p);
		if (idev_is_sick(p)) break;
		sleep(2);
	}
	idev_user_free(p);
	return NULL;
}

/* task 2. Send SMS ? */
void *thread_sms(void *data)
{
	char *who;
	int ret;
	int sleep_len;
	char *logid = "sms";
	IDEV *p = (IDEV *)data;
	idev_user_malloc(p);

	/* check the modem type to decide
		 which id to call */
	if (p->type == MC703) {
		who = "10000";
		sleep_len = 120;
	} else {
		who = "10086";
		sleep_len = 20;
	}

	while (1) {
		lock_device(p);
 		ret = p->send_sms(p, who, "hello!");
		unlock_device(p);
		if (ret)
			thread_log(p, logid, "send sms error.");
		else
			thread_log(p, logid, "send sms successfully.");
		if (idev_is_sick(p)) break;
		sleep(sleep_len);
	}
	idev_user_free(p);
	return NULL;
}

/* task 3. Call */
void *thread_call(void *data)
{
	char *logid = "call";
	char *who;
	int ret;
	IDEV *p = (IDEV *)data;
	idev_user_malloc(p);

	/* check the modem type to decide
		 which id to call */
	if (p->type == MC703)
		who = "10000";
	else
		who = "10086";

	while (1) {
		lock_device(p);
// 		ret = p->send(p, "AT+CDV10000", NULL, AT_MODE_LINE);
		ret = p->start_call(p, who);
		unlock_device(p);
		if (ret)
			thread_log(p, logid, "send ATD error.");
		else
			thread_log(p, logid, "send ATD successfully.");
		if (idev_is_sick(p)) break;
		sleep(10);

		lock_device(p);
// 		ret = p->send(p, "AT+CHV", NULL, AT_MODE_LINE);
		ret = p->stop_call(p);
		unlock_device(p);
		if (ret)
			thread_log(p, logid, "send ATH error.");
		else
			thread_log(p, logid, "send ATH successfully.");
		if (idev_is_sick(p)) break;
		sleep(10);
	}
	idev_user_free(p);
	return NULL;
}

/* predefine a cmd list for random cmd test */
#define	CMD_N  6
static char *cmd_list[] = {
	"AT+CPMS=\"SM\",\"SM\",\"SM\"",
	"at+clcc", 
	"at+csq", 
	"at+cmgf=1", 
	"at+cnmi?", 
	"at+cpms?", 
};

void *thread_random_cmd(void *data)
{
	char *logid = "random";
	int ret;
	char buf[64];
	IDEV *p = (IDEV *)data;
	idev_user_malloc(p);

	while (1) {
		int r;
		r = rand() % CMD_N;

		lock_device(p);
		ret = p->send(p, cmd_list[r], NULL, AT_MODE_LINE);
		unlock_device(p);

		if (ret) {
			snprintf(buf, 64, "%s(ERROR)", cmd_list[r]);
			thread_log(p, logid, buf);
		} else {
			snprintf(buf, 64, "%s(OK)", cmd_list[r]);
			thread_log(p, logid, buf);
		}

		if (idev_is_sick(p)) break;
		sleep(1);
	}
	idev_user_free(p);
	return NULL;
}

void *multi_modem_testing(void *data)
{
	/* start all the tests */
	while (1) {
		int i;
		for (i = 0; i < MAX_DEVICE_NO; i++) {
			IDEV *p = dev_list.dev[i].idev;
			if (dev_list.dev[i].active && 
					(dev_list.dev[i].during_test == 0) &&
					p->status == READY) {
				pthread_t tid[4];

				dev_list.dev[i].during_test = 1;
				dm_log(NULL, "device %s discovered, prepare to test...", 
						p->name);

				pthread_create(tid, NULL, thread_creg, (void *)p);
				pthread_create(tid+1, NULL, thread_sms, (void *)p);
				pthread_create(tid+2, NULL, thread_call, (void *)p);
				pthread_create(tid+3, NULL, thread_random_cmd, (void *)p);
			}
		}
		sleep(5);
	}
	return 0;
}

void *single_modem_testing(void *data)
{
	/* start all the tests */
	while (1) {
		if (dev_list.dev[0].active ) {
			IDEV *p = dev_list.dev[0].idev;
			if (p->status == READY) {
				int i;
				void *res;
				IDEV *p;
				pthread_t tid[3];
				p = dev_list.dev[0].idev;

				dm_log(NULL, "device discovered, prepare to test...");

				pthread_create(tid, NULL, thread_creg, (void *)p);
				pthread_create(tid+1, NULL, thread_sms, (void *)p);
				pthread_create(tid+2, NULL, thread_call, (void *)p);
				for (i = 0; i < 3; i++)
					pthread_join(tid[i], &res);
				/* until all dead */
			}
		}
// 		dm_log(NULL, "waiting for active modems ...");
		sleep(5);
	}
	return 0;
}

