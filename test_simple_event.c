/*
 * Copyright (C) 2010 Shoichi Sakane <sakane@tanu.org>, All rights reserved.
 * See the file LICENSE in the top level directory for more details.
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#include "simple_event.h"
#ifdef TEST_SIO
#include "simple_sio/simple_sio.h"
#endif
#ifdef TEST_NIO
#include "simple_netio/simple_netio.h"
#endif

struct ctx {
	int a;
} ctx;

int f_debug = 0;

char *prog_name = NULL;

void
usage()
{
	printf(
"Usage: %s [-dh]\n"
	, prog_name);

	exit(0);
}

#ifdef TEST_SIO
int
func_sio_parse(void *ctx_base)
{
	return 0;
}
#endif

#ifdef TEST_NIO
#include <errno.h>
int
cb_nio_parse(void *ctx_base)
{
	struct simple_netio_item *ctx = (struct simple_netio_item *)ctx_base;

	printf("offset=%d\n", ctx->offset);
	ctx->offset = 0;

	return 0;
}

int
cb_nio_recv(void *ctx_base)
{
	struct simple_netio_item *ctx = (struct simple_netio_item *)ctx_base;
	int recvlen;

	recvlen = recv(ctx->so, ctx->buf + ctx->offset, sizeof(ctx->buf) - ctx->offset, 0);
	if (recvlen == 0)
		return 0;
	if (recvlen < 0) {
		switch(errno) {
		case EAGAIN:
			if (ctx->head->debug >= 2)
				printf("EAGAIN\n");
			return 0;
		}
		err(1, "read()");
	}
	ctx->offset += recvlen;
	printf("recvlen=%d\n", recvlen);

	if (ctx->head->cb)
		ctx->head->cb(ctx);

	return recvlen;
}
#endif

int
func_call_2(void *ctx)
{
	printf("%s: called\n", __FUNCTION__);
	return 0;
}

int
func_call_1(void *ctx)
{
	printf("%s: called\n", __FUNCTION__);
	return 0;
}

int
run(char *dev)
{
	struct simple_ev_ctx *ev_ctx;
	struct timeval tv_timer;
	fd_set rfd, wfd, efd;
	int fd_max;
	int nfd;
	struct timeval *timeout;

	simple_ev_init(&ev_ctx);

	memset(&tv_timer, 0, sizeof(tv_timer));
	tv_timer.tv_sec = 1;
	tv_timer.tv_usec = 0;
	simple_ev_set_timer(ev_ctx, func_call_1, &ctx, &tv_timer, 1);
	tv_timer.tv_sec = 3;
	tv_timer.tv_usec = 0;
	simple_ev_set_timer(ev_ctx, func_call_2, &ctx, &tv_timer, 1);

#ifdef TEST_SIO
    {
	struct sio_ctx *sio_ctx;
	sio_ctx = sio_init(dev, 115200, 0, 512, 0,
	    func_sio_parse, NULL, 0);
	simple_ev_set_sio(ev_ctx, sio_readx, sio_ctx, sio_ctx->fd);
    }
#endif
#ifdef TEST_NIO
    {
	struct simple_netio_ctx *nio_ctx;
	struct simple_netio_item *nio_item;
	char *s_peers[] = { "127.0.0.1/9999" };
	int n_peers = 1;
	int f_udp = 1;
	int f_bind = 1;
	int f_conn = 0;

	nio_ctx = simple_netio_init_server(s_peers, n_peers,
	    f_udp, f_bind, f_conn, f_debug, cb_nio_parse);
	TAILQ_FOREACH(nio_item, &nio_ctx->qhead, link) {
		simple_ev_set_nio(ev_ctx, cb_nio_recv, nio_item,
		    nio_item->so);
	}
    }
#endif

	timeout = NULL;
	simple_ev_init_fdset(ev_ctx, &rfd, &wfd, &efd, &fd_max);
	while (1) {
		fd_max = 0;
		simple_ev_set_fdset(ev_ctx, &fd_max);
		simple_ev_set_timeout(ev_ctx, &timeout);

		nfd = select(fd_max + 1, &rfd, &wfd, &efd, timeout);
		if (nfd < 0) {
			warn("select()");
			break;
		}

		/* check it anyway regardless of timeout */
		simple_ev_check(ev_ctx);
	}

	simple_ev_destroy(ev_ctx);

	return 0;
}

int
main(int argc, char *argv[])
{
	int ch;

	prog_name = 1 + rindex(argv[0], '/');

	while ((ch = getopt(argc, argv, "dh")) != -1) {
		switch (ch) {
		case 'd':
			f_debug++;
			break;
		case 'h':
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

#ifdef TEST_NIO
	if (argc == 0)
		usage();
#endif

	run(argv[0]);

	return 0;
}

