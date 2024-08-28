#ifndef SELECTTIMER_H
#define SELECTTIMER_H
/**
 *   Copyright (C) 2010 by Ericsson AB
 *   S - 125 26  STOCKHOLM
 *   SWEDEN, tel int + 46 10 719 0000
 *
 *   The copyright to the computer program herein is the property of
 *   Ericsson AB. The program may be used and/or copied only with the
 *   written permission from Ericsson AB, or in accordance with the terms
 *   and conditions stipulated in the agreement/contract under which the
 *   program has been supplied.
 *
 *   All rights reserved.
 *
 * 
 *   File:   SelectTimer.h
 * 
 *   Author: uablrek
 *
 *   Date:   2010-05-21
 * 
 *   Implements call-back timers when a "select" is used.
 * 
 *   Reviewed: efaiami 2010-07-08
 * 
 */

#include <sys/time.h>
#include <poll.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @defgroup SelectTimer Timer and poll call-back framework.
 * Asynchronous call-back framework for poll (select) with timers.
 * 
 * Example;
 * @code
 *	    void* handle = timerCreateHandle_r();
 *	    poll_maxfd(handle, 16);
 *	    // (register fd-callbacks and start timers here...)
 *	    while (poll_execute(handle) == 0);
 *	    perror("poll");	// (not reached unless an error occurs)
 * @endcode
 */
/*@{*/

/**
 * Declaration of the timeout call-back function.
 * If the function returns !=0, the timer will be restarted, and thus 
 * creating a periodic timer.
 */
typedef int (*TimeoutReceiver)(void* userRef);

/**
 * Creates a "handle" used in subsequent "_r" functions.
 * The "_r" functions are reentrant (thread safe).
 */
void* timerCreateHandle_r(void);

/**
 * Destroys a handle and releases all resources.
 * @param handle A handle returned by ::timerCreateHandle_r.
 */
void timerDestroyHandle_r(void* handle);

/**
 * Returns the delay until the next timer expires. This value is
 * intended to be used in a `select' call.
 * @param handle A handle returned by ::timerCreateHandle_r.
 */
struct timeval* timerGetDelay_r(void* handle);

/**
 * @deprecated Non thread-safe version of the ::timerGetDelay_r funtion.
 */
struct timeval* timerGetDelay(void);

/**
 * Starts a timer. Returns a `timerRef' that should be used if the
 * ::timerStop() is called.
 * @param handle A handle returned by ::timerCreateHandle_r.
 */
void* timerStart_r(void* handle, int mS, TimeoutReceiver sr, void* userRef);

/**
 * @deprecated Non thread-safe version of the ::timerStart_r funtion.
 */
void* timerStart(int mS, TimeoutReceiver sr, void* userRef);

/**
 * Stops a running timer (may be called in the time-out function).
 * @param handle A handle returned by ::timerCreateHandle_r.
 */
void  timerStop_r(void* handle, void* timerRef);

/**
 * @deprecated Non thread-safe version of the ::timerStop_r funtion.
 */
void  timerStop(void* timerRef);

/**
 * Modifies an active timer. Modifications only occurs when the
 * timer has expired, for instance in the time-out function.
 */
void timerModify(void* timerRef, int mS, void* userRef);

/**
 * Sets the maximum number of file descriptors.
 * @param handle A handle returned by ::timerCreateHandle_r.
 * @param maxfd The maximum number of file descriptors.
 */
void poll_maxfd(void* handle, int maxfd);

/**
 * Waits for an event on a file descriptor or a timeout and makes
 * the appropriate call-backs. This function should be called in a
 * conrol loop.
 * @param handle A handle returned by ::timerCreateHandle_r.
 * @return 0 on success, != 0 otherwise.
 */
int poll_execute(void* handle);

/**
 * Defines the type of a file-descriptor event call-back function.
 */
typedef void (*FdCallbackFn)(struct pollfd* pfd, void *ref);

/**
 * Set a call-back function for events on a file descriptor.
 * @param handle A handle returned by ::timerCreateHandle_r.
 * @fn The call-back function.
 * @pfd Defines the file descriptor and the events to monitor.
 * @ref A user reference that is passed back in the call-back.
 * @return 0 on success, -1 if failed.
 */
int poll_setcallback(
    void* handle, FdCallbackFn fn, struct pollfd pfd, void *ref);

/**
 * Remove a call-back function for a file descriptor.
 * @param handle A handle returned by ::timerCreateHandle_r.
 */
void poll_unsetcallback(void* handle, int fd);

#ifdef DEBUG_SELTIMER
/* timerPrintStatus --
 *	For Debug purposes.
 */
void timerPrintStatus(void);
#endif

/*@}*/

#ifdef __cplusplus
}
#endif
#endif 
