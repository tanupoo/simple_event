/*
 * Copyright (C) 2010 Shoichi Sakane <sakane@tanu.org>, All rights reserved.
 * See the file LICENSE in the top level directory for more details.
 */

/*
 * select-based simple event library.
 */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#include "simple_event.h"

int
simple_ev_init(struct simple_ev_ctx **ev_ctx)
{
	struct simple_ev_ctx *new;

	if ((new = malloc(sizeof(*new))) == NULL)
		err(1, "ERROR: %s: malloc()", __FUNCTION__);

	TAILQ_INIT(&(new->qhead_timer));
	TAILQ_INIT(&(new->qhead_io));
	memset(&(new->tv_prev), 0, sizeof(new->tv_prev));

	*ev_ctx = new;

	return 0;
}

void
simple_ev_destroy(struct simple_ev_ctx *ev_ctx)
{
	free(ev_ctx);
}

int
simple_ev_set_timer(struct simple_ev_ctx *ev_ctx,
    int (*cb)(void *), void *cb_ctx,
    struct timeval *timer, int f_repeatable)
{
	struct simple_ev_item_timer *ev;

	if ((ev = calloc(1, sizeof(struct simple_ev_item_timer))) == NULL)
		err(1, "ERROR: %s: calloc()", __FUNCTION__);

	ev->io_type = SIMPLE_EV_TYPE_TIMER;
	ev->cb = cb;
	ev->cb_ctx = cb_ctx;
	ev->tv_interval = *timer;
	ev->tv_timer = *timer;
	ev->f_repeatable = f_repeatable;

	TAILQ_INSERT_HEAD(&ev_ctx->qhead_timer, ev, link);

	return 0;
}

int
simple_ev_set_nio(struct simple_ev_ctx *ev_ctx,
    int (*cb)(void *), void *cb_ctx, int fd)
{
	struct simple_ev_item_io *ev;

	if ((ev = calloc(1, sizeof(struct simple_ev_item_io))) == NULL)
		err(1, "ERROR: %s: calloc()", __FUNCTION__);

	ev->io_type = SIMPLE_EV_TYPE_NET_IO;
	ev->cb = cb;
	ev->cb_ctx = cb_ctx;
	ev->fd = fd;

	TAILQ_INSERT_HEAD(&ev_ctx->qhead_io, ev, link);

	return 0;
}

int
simple_ev_set_sio(struct simple_ev_ctx *ev_ctx,
    int (*cb)(void *), void *cb_ctx, int fd)
{
	struct simple_ev_item_io *ev;

	if ((ev = calloc(1, sizeof(struct simple_ev_item_io))) == NULL)
		err(1, "ERROR: %s: calloc()", __FUNCTION__);

	ev->io_type = SIMPLE_EV_TYPE_SERIAL_IO;
	ev->cb = cb;
	ev->cb_ctx = cb_ctx;
	ev->fd = fd;

	TAILQ_INSERT_HEAD(&ev_ctx->qhead_io, ev, link);

	return 0;
}

void
simple_ev_init_fdset(struct simple_ev_ctx *ev_ctx,
    fd_set *rfd, fd_set *wfd, fd_set *efd, int *fd_max)
{
	FD_ZERO(rfd);
	FD_ZERO(wfd);
	FD_ZERO(efd);
	ev_ctx->rfd = rfd;
	ev_ctx->wfd = wfd;
	ev_ctx->efd = efd;
	*fd_max = 0;
}

/* this function _never initialize_ the fd_max. */
void
simple_ev_set_fdset(struct simple_ev_ctx *ev_ctx, int *fd_max)
{
	struct simple_ev_item_io *ev;

	if (!ev_ctx)
		return;

	/* NOTE: don't initialize the fd_max */

	TAILQ_FOREACH(ev, &ev_ctx->qhead_io, link) {
		FD_SET(ev->fd, ev_ctx->rfd);
		*fd_max = *fd_max > ev->fd ? *fd_max : ev->fd;
	}
}

#define timercmp_gt(a, b)                     \
        (((a)->tv_sec == (b)->tv_sec) ?       \
            ((a)->tv_usec > (b)->tv_usec) :   \
            ((a)->tv_sec > (b)->tv_sec))

void
simple_ev_set_timeout(struct simple_ev_ctx *ev_ctx, struct timeval **timeout)
{
	struct simple_ev_item_timer *ev;
	struct timeval tv_min;

	if (!ev_ctx || !TAILQ_FIRST(&ev_ctx->qhead_timer))
		return;

	if (*timeout == NULL) {
		if ((*timeout = calloc(1, sizeof(**timeout))) == NULL)
			err(1, "ERROR: %s: callc(timeval)", __FUNCTION__);
	}

	/* initialize the timeout */
	memset(&tv_min, 0, sizeof(tv_min));

	TAILQ_FOREACH(ev, &ev_ctx->qhead_timer, link) {
		if (!tv_min.tv_sec && !tv_min.tv_usec) {
			tv_min = ev->tv_timer;
			continue;
		}
		if (timercmp_gt(&tv_min, &ev->tv_timer)) {
			tv_min = ev->tv_timer;
			continue;
		}
	}

	**timeout = tv_min;

	/* initialize the tv_prev if it's in the first */
	if (!ev_ctx->tv_prev.tv_sec && !ev_ctx->tv_prev.tv_usec)
		gettimeofday(&ev_ctx->tv_prev, NULL);

	return;
}

void
simple_ev_check_timer(struct simple_ev_ctx *ev_ctx)
{
	struct simple_ev_item_timer *ev;
	struct timeval tv_cur;
	struct timeval tv_diff;

	if (!ev_ctx)
		return;

	gettimeofday(&tv_cur, NULL);
	timersub(&tv_cur, &ev_ctx->tv_prev, &tv_diff);

	TAILQ_FOREACH(ev, &ev_ctx->qhead_timer, link) {
		if (timercmp_gt(&ev->tv_timer, &tv_diff)) {
			timersub(&ev->tv_timer, &tv_diff, &ev->tv_timer);
			continue;
		}
		if (ev->cb)
			ev->cb(ev->cb_ctx);
		if (!ev->f_repeatable) {
			TAILQ_REMOVE(&ev_ctx->qhead_timer, ev, link);
			free(ev);
		}
		/* reset the timer */
		ev->tv_timer = ev->tv_interval;
	}
	gettimeofday(&ev_ctx->tv_prev, NULL);
}

void
simple_ev_check_io(struct simple_ev_ctx *ev_ctx)
{
	struct simple_ev_item_io *ev;

	if (!ev_ctx)
		return;

	TAILQ_FOREACH(ev, &ev_ctx->qhead_io, link) {
		if (FD_ISSET(ev->fd, ev_ctx->rfd))
			ev->cb(ev->cb_ctx);
	}
}

void
simple_ev_check(struct simple_ev_ctx *ev_ctx)
{
	simple_ev_check_timer(ev_ctx);
	simple_ev_check_io(ev_ctx);
}

