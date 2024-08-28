 /*******************************************************************************
  * Copyright (C) 2010 by Ericsson AB
  * S - 125 26  STOCKHOLM
  * SWEDEN, tel int + 46 10 719 0000
  *
  * The copyright to the computer program herein is the property of
  * Ericsson AB. The program may be used and/or copied only with the
  * written permission from Ericsson AB, or in accordance with the terms
  * and conditions stipulated in the agreement/contract under which the
  * program has been supplied.
  *
  * All rights reserved.
  *
  *
  *   Author: uablrek
  *
  *   Date:   2010-05-21
  *
  *   File: SelectTimer.c
  *
  *  Implements call-back timers when a "select" is used.
  *
  *  Reviewed: efaiami 2010-08-17
  *
  *  Modify: efaiami 2011-02-21 for trace and log function
  *  Modify: uabjoy  2014-03-18 Adding support for trace with the Trace CC.
  *  Modify: xdonngu 2014-11-18 HT20869: core dump at poll_execute
  *
  *******************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <trace.h>
#include "SelectTimer.h"
#include "InternalTrace.h"

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

struct TimerInternal {
    int mS;
    TimeoutReceiver sr;
    void* ref;
    struct timeval expires;

    Dx(unsigned int mark);
    long int lastsec;						/* Used to detect system clock steppings */
    enum TimerState state;
    struct TimerInternal* next;
};

struct TimeHandle {
    struct timeval delay;	         		/* Time until the next timer expires */
    struct TimerInternal* timers;        	/* List of running timers */
    struct TimerInternal* freeTimerList; 	/* List of deleted reusable timers */
    int freeTimers;                      	/* Number of reusable timers */
    struct TimerInternal* activeTimer;   	/* The active timer (if any) */

    Dx(unsigned int mark);
    struct pollextensions* polld;
};

static struct TimeHandle dhandle = {
	.delay ={0,0},
	.timers = 0,
	.freeTimerList = 0,
	.freeTimers = 0,
	.activeTimer = 0,
#ifdef DEBUG_SELTIMER
	.mark = 0,
#endif
	.polld = 0
};

/* Forward declarations: */
static void rescedule(
    struct TimeHandle* handle, struct TimerInternal* t, struct timeval* tv);
static void poll_deleteextension(void* handle);
Dx(static void checkListIntergrity(struct TimeHandle* th, int die));

void*
timerCreateHandle_r(void)
{
    DEBUG_COMSA("Enter timerCreateHandle_r.");
	ENTER_MWSA_GENERAL();
	struct TimeHandle* th =
		(struct TimeHandle*)malloc(sizeof(struct TimeHandle));
   	memset(th, 0, sizeof(struct TimeHandle));
    DEBUG_COMSA("Leave timerCreateHandle_r.");
	LEAVE_MWSA_GENERAL();
    return (void*)th;
}

void
timerDestroyHandle_r(void* handle)
{
	ENTER_MWSA_GENERAL();
    	struct TimerInternal* t, *tmp;
    	struct TimeHandle* th = (struct TimeHandle*)handle;

    	if (handle == 0) {
		LEAVE_MWSA_GENERAL();
		return;
	}

    	Dx(checkListIntergrity(th, 0));
    	t = th->timers;
    	while (t != 0) {
		tmp = t;
		t = t->next;
		free(tmp);
    	}
    	t = th->freeTimerList;
    	while (t != 0) {
		tmp = t;
		t = t->next;
		free(tmp);
   	}
    	free(th->activeTimer); /* may be null */
    	poll_deleteextension(handle);
    	free(handle);
	LEAVE_MWSA_GENERAL();
}

/** ----------------------------------------------------------------------
  *  timerGetDelay --
  *	Returns the delay until the next timer expires. This value is
  *	intended to be used in a `select' call.
  */
struct timeval*
timerGetDelay(void)
{
	ENTER_MWSA_GENERAL();
	LEAVE_MWSA_GENERAL();
    return timerGetDelay_r(&dhandle);
}

struct timeval*
timerGetDelay_r(void* handle)
{
	ENTER_MWSA_GENERAL();
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
	       	   might take time */
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
	LEAVE_MWSA_GENERAL();
    	return &(th->delay);
}

/** ----------------------------------------------------------------------
 * dipcEnvSetPeriodicTimer --
 */

void*
timerStart(int mS, TimeoutReceiver sr, void* ref)
{
	ENTER_MWSA_GENERAL();
	LEAVE_MWSA_GENERAL();
    	return timerStart_r(&dhandle, mS, sr, ref);
}

void*
timerStart_r(void* handle, int mS, TimeoutReceiver sr, void* ref)
{
	ENTER_MWSA_GENERAL();
    	struct TimerInternal* t;
    	struct TimeHandle* th = (struct TimeHandle*)handle;

    	Dx(checkListIntergrity(th, 1));

    	if (th->freeTimers > 0) {
		assert(th->freeTimerList != 0);
		t = th->freeTimerList;
		th->freeTimerList = t->next;
		th->freeTimers--;
    	} else {
		assert(th->freeTimerList == 0);
		t = (struct TimerInternal*) malloc(sizeof(struct TimerInternal));
		Dl(7, printf("timerStart_r: Allocating timer %u\n",(unsigned int)t));
    	}

    	t->mS = mS;
    	t->sr = sr;
    	t->ref = ref;
    	t->lastsec = 0;
    	Dx(t->mark = 0);
    	gettimeofday(&t->expires, 0);
    	rescedule(th, t, 0);
    	Dx(checkListIntergrity(th, 1));
	LEAVE_MWSA_GENERAL();
    	return (void*) t;
}

void
timerStop(void* timerRef)
{
	ENTER_MWSA_GENERAL();
    	timerStop_r(&dhandle, timerRef);
	LEAVE_MWSA_GENERAL();
}

void
timerStop_r(void* handle, void* timerRef)
{
	ENTER_MWSA_GENERAL();
    	struct TimerInternal* item;
    	struct TimerInternal* prev;
    	struct TimerInternal* t;
    	struct TimeHandle* th;
    	th = (struct TimeHandle*)handle;
    	t = (struct TimerInternal*) timerRef;

    	if (t->state == Running) {
		Dl(9, printf("timerStop_r: Stopping Running timer %u\n",
		     (unsigned int)timerRef));
		prev = 0;
		item = th->timers;
		while (item && (t != item)) {
	    	prev = item;
	    	item = item->next;
		}

		if (!item) {
	    	Dl(5, printf("timerStop_r: Running NOT FOUND\n"));
		LEAVE_MWSA_GENERAL();
	    	return;
		}

		if (prev) {
	    	prev->next = item->next;
		} else {
	    	th->timers = item->next;
		}

		if (th->freeTimers < 4) {
	    	item->next = th->freeTimerList;
	    	th->freeTimerList = item;
	    	th->freeTimers++;
		} else {
	    	Dl(7, printf("timerStop_r: Running found and freed\n"));
	    	free(item);
		}
    	} else {
		Dl(9,printf("timerStop_r: Stopping Active timer %u\n",
		    (unsigned int)timerRef));
		t->state = Deleted;
    	}
	LEAVE_MWSA_GENERAL();
}


void
timerModify(void* timerRef, int mS, void* userRef)
{
	ENTER_MWSA_GENERAL();
    	struct TimerInternal* t;
    	t = (struct TimerInternal*) timerRef;
    	t->mS = mS;
    	t->ref = userRef;
	LEAVE_MWSA_GENERAL();
}

#ifdef DEBUG_SELTIMER
void
timerPrintStatus(void)
{
	ENTER_MWSA_GENERAL();
    	struct TimerInternal* item;
    	printf("Active timers:\n");
    	if (dhandle.timers == 0) {
		printf("   None\n");
		LEAVE_MWSA_GENERAL();
		return;
    	}
    	item = dhandle.timers;
    	while (item) {
		printf("   %-6d mS\n", item->mS);
		item = item->next;
    	}
	LEAVE_MWSA_GENERAL();
}

static void
checkListIntergrity(struct TimeHandle* th, int die)
{
	ENTER_MWSA_GENERAL();
    	unsigned int mark;
    	struct TimerInternal* item = th->timers;
    	struct TimerInternal* prev = 0;
    	unsigned int count = 0;

    	th->mark++;
    	mark = th->mark;
    	while (item != 0) {
		if (item->mark == mark) {
	    	if (die) {
				assert(item->mark != mark);
	    	}
	    	printf("===> SelectTimer: Circular list broken up!\n");
	    	assert(prev != 0);
	    	prev->next = 0;
		}
		item->mark = mark;
		count++;
		prev = item;
		item = item->next;
    	}
	LEAVE_MWSA_GENERAL();
}
#endif

/* ----------------------------------------------------------------------
 * rescedule --
 *	Re-scedule a timer that has expired into the timer queue.
 */

static void
rescedule(struct TimeHandle* th, struct TimerInternal* t, struct timeval* tv)
{
	ENTER_MWSA_GENERAL();
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
	LEAVE_MWSA_GENERAL();
}

/** ======================================================================
 * poll interface extensions
 */
struct PollFn {
    FdCallbackFn fn;
    void* userRef;
};

struct pollextensions {
    unsigned int maxfd;
    struct pollfd* spollfd;
    unsigned int nfds;
    struct PollFn* spollfn;
};

static void
poll_deleteextension(void* handle)
{
	ENTER_MWSA_GENERAL();
    	struct TimeHandle* th = (struct TimeHandle*)handle;
    	struct pollextensions* e;
    	e = th->polld;
    	if (e != 0) {
		free(e->spollfd);
		free(e->spollfn);
		free(e);
    	}
	LEAVE_MWSA_GENERAL();
}


void
poll_maxfd(void* handle, int maxfd)
{
    DEBUG_COMSA("Enter poll_maxfd.");

    ENTER_MWSA_GENERAL();
    struct pollextensions* e;
    struct TimeHandle* th = (struct TimeHandle*)handle;
    assert(th->polld == 0);
    assert(maxfd > 0);

    e = (struct pollextensions*)malloc(sizeof(struct pollextensions));
    e->maxfd = maxfd;
    e->spollfd = (struct pollfd*)malloc(sizeof(struct pollfd)*maxfd);
    e->nfds = 0;
    e->spollfn = (struct PollFn*)malloc(sizeof(struct PollFn)*maxfd);
    th->polld = e;
    DEBUG_COMSA("Leave poll_maxfd.");

    LEAVE_MWSA_GENERAL();
}

int
poll_execute(void* handle)
{
	ENTER_MWSA_GENERAL();
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
	LEAVE_MWSA_GENERAL();
	return 0;
}

int
poll_setcallback(void* handle, FdCallbackFn fn, struct pollfd pfd, void *ref)
{
	ENTER_MWSA_GENERAL();
    	unsigned int i;
    	struct pollextensions* e;
    	struct pollfd* spollfd;
    	struct TimeHandle* th = (struct TimeHandle*)handle;
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
    	return -1;				/* Table full */
	LEAVE_MWSA_GENERAL();
}

void
poll_unsetcallback(void* handle, int fd)
{
	ENTER_MWSA_GENERAL();
    	unsigned int i;
    	int index = -1;
    	struct pollextensions* e;
    	struct pollfd* spollfd;
   	struct TimeHandle* th = (struct TimeHandle*)handle;
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
	LEAVE_MWSA_GENERAL();
}



/** ======================================================================
  * TEST PROGRAM
  */

#ifdef STAND_ALONE_TEST
#include <stdio.h>

void* timerRef;
void* h;
int
timeoutReceiver(void* userRef)
{
	ENTER_MWSA_GENERAL();
    	int mS = (int)userRef;
    	mS /= 2;
    	if (mS < 100) mS = 2000;
    	timerModify(timerRef, mS, (void*)mS);
   	printf("Next timeout in %d mS\n", mS);
	LEAVE_MWSA_GENERAL();
   	return 1;			/* Return != 0 means; "restart timer" */
}

static void stdinEvent(struct pollfd* pfd, void *ref)
{
	ENTER_MWSA_GENERAL();
    	printf("Quitting ...\n");
    	timerStop_r(h, timerRef);
    	timerDestroyHandle_r(h);
	LEAVE_MWSA_GENERAL();
	exit(0);
}

int
main()
{
    struct pollfd pfd = {0, POLLIN, 0};
    h = timerCreateHandle_r();
    poll_maxfd(h, 2);
    timerRef = timerStart_r(h, 2000, timeoutReceiver, (void*)2000);
    timerRef = timerStart_r(h, 4000, timeoutReceiver, (void*)2000);
    timerRef = timerStart_r(h, 1000, timeoutReceiver, (void*)2000);
    timerRef = timerStart_r(h, 1200, timeoutReceiver, (void*)2000);
    poll_setcallback(h, stdinEvent, pfd, 0);
    poll_unsetcallback(h, 0);
    poll_setcallback(h, stdinEvent, pfd, 0);
    while (poll_execute(h) == 0);
    printf("Fault! Drop out of the control-loop ...\n");
    return 0;
}
#endif
