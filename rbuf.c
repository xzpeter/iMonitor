/*
   rbuf.c : a simple round buffer.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rbuf.h"

/* we can modify this function for specific use */
char *readline(RBUF *r)
{
	int len;
	len = r->strchr(r, '\n');
	if (len > r->line_buf_size - 1) {
		fprintf(stderr, "cannot read line, line buffer length is %d, "
			"read line need %d.\n", r->line_buf_size, len);
		return NULL;
	} else if (len > 0) {
		r->read(r, r->line_buf, len);
		r->line_buf[len] = 0x00;
		return r->line_buf;
	}
	/* not found new line */
	return NULL;
}

/* find the next char 'c' in current buffer, return len from 
   cur_read_p to the char. return 0 if not found, else return>0  */
int rbuf_strchr(RBUF *r, char c)
{
	char *p;
	if (r->cur_read_p <= r->cur_write_p) {
		/* no need to wrap */
		for (p = r->cur_read_p; p < r->cur_write_p; p++)
			if (*p == c) return (p - r->cur_read_p + 1);
	} else {
		/* wrap needed */
		int rest;
		char *end = r->g_buf + r->g_buf_size;
		for (p = r->cur_read_p; p < end; p++)
			if (*p == c) return (p - r->cur_read_p + 1);
		rest = end - r->cur_read_p;
		for (p = r->g_buf; p < r->cur_write_p; p++)
			if (*p == c) return (rest + p - r->g_buf + 1);
	}

	/* not found */
	return 0;
}

unsigned int rbuf_write(RBUF *r, char *data, unsigned int len)
{
	unsigned int to_write, rest;

	rest = r->g_buf_size - r->buffered;
	to_write = rest >= len ? len : rest;

	/* from cur_write_p to end of g_buf */
	rest = r->g_buf + r->g_buf_size - r->cur_write_p;
	if (rest > to_write) {
		/* no need to wrap */
		memcpy(r->cur_write_p, data, to_write);
		r->cur_write_p += to_write;
	} else {
		/* wrap is needed */
		unsigned int rest_to_write;
		memcpy(r->cur_write_p, data, rest);
		data += rest;
		rest_to_write = to_write - rest;
		memcpy(r->g_buf, data, rest_to_write);
		r->cur_write_p = r->g_buf + rest_to_write;
	}

	r->buffered += to_write;
	return to_write;
}

unsigned int rbuf_read(RBUF *r, char *data, unsigned int len)
{
	unsigned int to_read, rest;

	/* how many bytes I can give */
	to_read = r->buffered >= len ? len : r->buffered;
	/* how long from readp to the end now */
	rest = r->g_buf + r->g_buf_size - r->cur_read_p;
	if (to_read < rest) {
		/* no need to wrap */
		memcpy(data, r->cur_read_p, to_read);
		r->cur_read_p += to_read;
	} else {
		/* wrap needed */
		memcpy(data, r->cur_read_p, rest);
		data += rest;
		rest = to_read - rest;
		memcpy(data, r->g_buf, rest);
		r->cur_read_p = r->g_buf + rest;
	}

	r->buffered -= to_read;
	return 0;
}

RBUF *rbuf_init(unsigned int size)
{
	RBUF *rbuf;
	unsigned int len;

	if (size > MAX_GBUF_SIZE)
		return NULL;

	rbuf = malloc(sizeof(RBUF));
	if (rbuf == NULL)
		return NULL;

	rbuf->g_buf = malloc(size);
	if (rbuf->g_buf == NULL)
		goto rbuf_free;
	rbuf->g_buf_size = size;

	len = size / 10;
	len = len < MIN_LINE_BUF_SIZE ? MIN_LINE_BUF_SIZE : len;
	rbuf->line_buf = malloc(len);
	if (rbuf->line_buf == NULL)
		goto g_buf_free;
	rbuf->line_buf_size = len;

	/* all ok */
	rbuf->readline = readline;
	rbuf->write = rbuf_write;
	rbuf->read = rbuf_read;
	rbuf->strchr = rbuf_strchr;
	rbuf->cur_read_p = rbuf->g_buf;
	rbuf->cur_write_p = rbuf->g_buf;
	rbuf->buffered = 0;

	return rbuf;

g_buf_free:
	free(rbuf->g_buf);
rbuf_free:
	free(rbuf);
	return NULL;
}

RBUF *rbuf_release(RBUF *rbuf)
{
	if (rbuf == NULL)
		return NULL;

	free(rbuf->g_buf);
	free(rbuf);

	return NULL;
}
