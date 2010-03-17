/*
   rbuf.h : a simple round buffer.
*/
#ifndef		__RBUF_H__
#define		__RBUF_H__

/* max buffer size allowed */
#define		MAX_GBUF_SIZE		1000000
#define		MIN_LINE_BUF_SIZE	256

typedef struct _rbuf RBUF;

struct _rbuf {
	char *g_buf;
	char *line_buf;
	char *cur_read_p;
	char *cur_write_p;
	unsigned int buffered;
	unsigned int g_buf_size;
	unsigned int line_buf_size;

	char *(*readline)(RBUF *);
	unsigned int (*write)(RBUF *, char *data, unsigned int len);
	unsigned int (*read)(RBUF *, char *data, unsigned int len);
	int (*strchr)(RBUF *, char c);
};

/* functions to use */
RBUF *rbuf_init(unsigned int size);
RBUF *rbuf_release(RBUF *rbuf);

#endif
