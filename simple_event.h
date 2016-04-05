#include <sys/queue.h>
#include <sys/time.h>
#include <sys/select.h>

#define SIMPLE_EV_TYPE_TIMER       1
#define SIMPLE_EV_TYPE_SERIAL_IO   2
#define SIMPLE_EV_TYPE_NET_IO      3

struct simple_ev_item_timer {
        TAILQ_ENTRY(simple_ev_item_timer) link;

        int io_type;

	int (*cb)(void *);
        void *cb_ctx;

        struct timeval tv_interval;	/* maximum value of the timer */
        struct timeval tv_timer;	/* timer to be launched */
        int f_repeatable;
};

struct simple_ev_item_io {
        TAILQ_ENTRY(simple_ev_item_io) link;

        int io_type;

	int (*cb)(void *);
        void *cb_ctx;

	int fd;
};

struct simple_ev_ctx {
	struct timeval tv_prev;	/* previous timestamp when an event happened */
	TAILQ_HEAD(simple_ev_timer_qhead, simple_ev_item_timer) qhead_timer;
	fd_set *rfd, *wfd, *efd;
	TAILQ_HEAD(simple_ev_io_qhead, simple_ev_item_io) qhead_io;
};

int simple_ev_init(struct simple_ev_ctx **);
void simple_ev_destroy(struct simple_ev_ctx *);
int simple_ev_set_timer(struct simple_ev_ctx *,
    int (*)(void *), void *, struct timeval *, int);
int simple_ev_set_nio(struct simple_ev_ctx *, int (*)(void *), void *, int);
int simple_ev_set_sio(struct simple_ev_ctx *, int (*)(void *), void *, int);
void simple_ev_init_fdset(struct simple_ev_ctx *,
    fd_set *, fd_set *, fd_set *, int *);
void simple_ev_set_fdset(struct simple_ev_ctx *, int *);
void simple_ev_set_timeout(struct simple_ev_ctx *, struct timeval **);
void simple_ev_check_timer(struct simple_ev_ctx *);
void simple_ev_check_io(struct simple_ev_ctx *);
void simple_ev_check(struct simple_ev_ctx *);
