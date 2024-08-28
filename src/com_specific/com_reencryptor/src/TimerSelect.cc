#include "Trace.h"
#include "TimerSelect.h"
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifdef DEBUG_SELTIMER
static int dbgLvl = 5;
#define Dl(l,x) if (dbgLvl >= l) x
#define Dx(x) x
#else
#define Dl(l,x)
#define Dx(x)
#endif

enum TimerState {
  Running,  								/* Timer is scheduled, waiting for time to expire */
  Active,   								/* Timer has expired and timeout-handler is called */
  Deleted   								/* Timer was deleted by timeout-handler */
};

Dx(static void checkListIntergrity(struct TimeHandle* th, int die));


TimerSelect::TimerSelect()
{

}

TimerSelect::~TimerSelect()
{

}


struct timeval* TimerSelect::timerGetDelay_r(void* handle)
{
	struct timeval tv;
	struct TimerInternal* item;
	int rc;
	struct TimeHandle* th;
	th = (struct TimeHandle*)handle;
	Dx(checkListIntergrity(th, 1));
	gettimeofday(&tv, 0);

	while (th->timers) {
		item = th->timers;
		if ((tv.tv_sec > item->expires.tv_sec)
				|| ((tv.tv_sec == item->expires.tv_sec)
						&& (tv.tv_usec > item->expires.tv_usec))) {

			th->timers = th->timers->next;
			item->state = Active;
			assert(th->activeTimer==0);
			th->activeTimer = item;

			rc = item->sr(item->ref);  /* Run timeout-handler */

			th->activeTimer = 0;

			/* Update the time-stamp here, because the user function
			 * might take time */
			gettimeofday(&tv, 0);

			if ((item->state == Deleted) || (rc == 0)) {
				if (th->freeTimers < 4) {
					item->next = th->freeTimerList;
					th->freeTimerList = item;
					th->freeTimers++;
				} else {
					free(item);
				}
			} else {
				rescedule(th, item, &tv);
				item->lastsec = tv.tv_sec;
			}
		} else {
			/* Check for backward clock stepping */
			if (item->lastsec > tv.tv_sec) {
				Dl(5, printf("Clock stepping backwards!\n"));
				th->timers = th->timers->next;
				item->state = Active;
				item->expires.tv_sec = tv.tv_sec;
				rescedule(th, item, 0);
			}
			break;
		}
	}

	if (th->timers == 0) {
		Dl(9, printf("No active timers.\n"));
		return 0;		/* No active timers */
	}

	item = th->timers;
	if (tv.tv_usec > item->expires.tv_usec) {
		th->delay.tv_usec = item->expires.tv_usec + 1000000 - tv.tv_usec;
		th->delay.tv_sec = item->expires.tv_sec - tv.tv_sec - 1;
	} else {
		th->delay.tv_usec = item->expires.tv_usec - tv.tv_usec;
		th->delay.tv_sec = item->expires.tv_sec - tv.tv_sec;
	}

	Dl(9, printf("Delay = %ld mS\n",
			th->delay.tv_sec*1000 + th->delay.tv_usec/1000));
	Dx(checkListIntergrity(th, 1));
	return &(th->delay);
}

void TimerSelect::rescedule(struct TimeHandle* th, struct TimerInternal* t, struct timeval* tv)
{
	struct TimerInternal* item;
	struct TimerInternal* prev;

	/* Compute the expire time */
	t->expires.tv_sec += (t->mS / 1000);
	t->expires.tv_usec += ((t->mS % 1000) * 1000);
	if (t->expires.tv_usec > 1000000) {
		t->expires.tv_sec++;
		t->expires.tv_usec -= 1000000;
	}

	if (tv) {
		/* Check if the new expire time is in the future */
		if ((t->expires.tv_sec < tv->tv_sec)
				|| ((t->expires.tv_sec == tv->tv_sec)
						&& (t->expires.tv_usec < tv->tv_usec))) {

			t->expires.tv_sec = tv->tv_sec + (t->mS / 1000);
			t->expires.tv_usec = tv->tv_usec + ((t->mS % 1000) * 1000);
			if (t->expires.tv_usec > 1000000) {
				t->expires.tv_sec++;
				t->expires.tv_usec -= 1000000;
			}
		}
	}

	/* Sort on seconds */
	prev = 0;
	item = th->timers;
	while (item && (item->expires.tv_sec < t->expires.tv_sec)) {
		prev = item;
		item = item->next;
	}

	/* Sort on micro seconds */
	while (item
			&& (item->expires.tv_sec == t->expires.tv_sec)
			&& (item->expires.tv_usec < t->expires.tv_usec)) {

		prev = item;
		item = item->next;
	}

	/* Insert */
	if (prev) {
		prev->next = t;
	} else {
		th->timers = t;
	}

	t->next = item;
	t->state = Running;
}

void* TimerSelect::timerCreateHandle_r(void)
{
    DEBUG_COM_RP("Enter TimerSelect::timerCreateHandle_r.");
    TimeHandleT* th = (TimeHandleT*)malloc(sizeof(TimeHandleT));
    memset(th, 0, sizeof(TimeHandleT));
    DEBUG_COM_RP("Leave TimerSelect::timerCreateHandle_r.");
    return (void*)th;
}

void TimerSelect::poll_maxfd(void* handle, int maxfd)
{
    DEBUG_COM_RP("Enter poll_maxfd.");

    pollextensionsT* e;
    TimeHandleT* th = (TimeHandleT*)handle;
    assert(th->polld == 0);
    assert(maxfd > 0);

    e = (pollextensionsT*)malloc(sizeof(pollextensionsT));
    e->maxfd = maxfd;
    e->spollfd = (struct pollfd*)malloc(sizeof(struct pollfd)*maxfd);
    e->nfds = 0;
    e->spollfn = (PollFnT*)malloc(sizeof(PollFnT)*maxfd);
    th->polld = e;
    DEBUG_COM_RP("Leave poll_maxfd.");

}

int TimerSelect::poll_setcallback(void* handle, FdCallbackFn fn, struct pollfd pfd, void *ref)
{
	DEBUG_COM_RP("Enter poll_setcallback.");
	unsigned int i;
	pollextensionsT* e;
	struct pollfd* spollfd;
	TimeHandleT* th = (TimeHandleT*)handle;
	assert(th->polld != 0);
	e = th->polld;
	spollfd = e->spollfd;

	/* First check if an existing entry should be modified. */
	for (i = 0; i < e->nfds; i++, spollfd++) {
		if (pfd.fd == spollfd->fd) {
			spollfd->events = pfd.events;
			e->spollfn[i].fn = fn;
			e->spollfn[i].userRef = ref;
			return 1;			/* Entry modified. */
		}
	}

	if (e->nfds < e->maxfd) {
		i = e->nfds++;
		e->spollfd[i].fd = pfd.fd;
		e->spollfd[i].events = pfd.events;
		e->spollfd[i].revents = 0;
		e->spollfn[i].fn = fn;
		e->spollfn[i].userRef = ref;
		return 0;			/* New entry */
	}

	assert(e->nfds == e->maxfd);
	DEBUG_COM_RP("Leave poll_setcallback.");
	return -1;				/* Table full */
}

void TimerSelect::poll_unsetcallback(void* handle, int fd)
{
	DEBUG_COM_RP("Enter poll_unsetcallback.");

	unsigned int i;
	int index = -1;
	pollextensionsT* e;
	struct pollfd* spollfd;
	TimeHandleT* th = (TimeHandleT*)handle;
	DEBUG_COM_RP("Handle NULL check added");
        if(th == NULL){
	   ERR_COM_RP("Handle set to NULL, exiting.");
	   return;
        }
	assert(th->polld != 0);

	e = th->polld;
	spollfd = e->spollfd;

	for (i = 0; i < e->nfds; i++, spollfd++) {
		if (fd == spollfd->fd) {
			index = i;
			break;
		}
	}

	if (index < 0) return;	/* Not found */

	assert(e->nfds > 0);
	e->nfds--;

	/* Pack the arrays */
	while ((unsigned int)index < e->nfds) {
		e->spollfd[index] = e->spollfd[index+1];
		e->spollfn[index] = e->spollfn[index+1];
		index++;
	}

	DEBUG_COM_RP("Leave poll_unsetcallback.");
}

int TimerSelect::poll_execute(void* handle)
{

	int rc;
	struct timeval* tvp;
	int timeout;
	struct pollextensions* e;
	struct TimeHandle* th = (struct TimeHandle*)handle;
	assert(th->polld != 0);
	e = th->polld;
	tvp = timerGetDelay_r(handle);
	if (tvp == 0) {
		if (e->nfds == 0) {
			/* We don't wait for anything, so a poll would block forever */
			return -1;
		}
		timeout = -1;
	} else {
		if (tvp->tv_sec > (0x7fffffff >> 10)) {
			timeout = 0x7ffff300;
		} else {
			timeout = tvp->tv_sec*1000 + tvp->tv_usec/1000;
		}
	}

	do {
		rc = poll(e->spollfd, e->nfds, timeout);
		if (rc < 0 && errno == EINTR)
			continue;
	} while (0);

	if (rc < 0) return rc;

	if (rc > 0) {
		unsigned int i = 0;
		struct pollfd* spollfd = e->spollfd;
		struct PollFn* spollfn;
		int oldfd;

		while (i < e->nfds) {
			// HT20869: check for error code values here to prevent invalid access.
			if ((spollfd->revents) &&
					(!(spollfd->revents & POLLERR)) &&
					(!(spollfd->revents & POLLHUP)) &&
					(!(spollfd->revents & POLLNVAL))) {
				struct pollfd pollfdcopy = *spollfd;
				spollfn = e->spollfn + i;
				oldfd = spollfd->fd;
				(*(spollfn->fn))(&pollfdcopy, spollfn->userRef);
				rc--;
				if (rc == 0) break;

				/** This is tricky; the callback functions might have
				 *  added and/or removed callback functions
				 *  themselves. So only increment the index when the
				 *  array seeme un-altered.
				 */
				if (oldfd == spollfd->fd) {
					spollfd++;
					i++;
				}
			} else {
				spollfd++;
				i++;
			}
		}
		/** assert(rc == 0); this may happen if *alother* callback is altered
		 *  in a callback!
		 */
	}
	return 0;
}
