#ifndef PmSaSelectTimer_H
#define PmSaSelectTimer_H
#include <stdlib.h>
#include <sys/time.h>
#include <poll.h>
#include <assert.h>
#include <cstdlib>

/**
 * @defgroup PmSaSelectTimer Timer and poll call-back framework.
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
class PmSaSelectTimer
{
	PmSaSelectTimer();
public:
	static PmSaSelectTimer& Instance() {
			static PmSaSelectTimer PmSaSelectTimer;
			return PmSaSelectTimer;
	}

	~PmSaSelectTimer();

	/**
	 * Declaration of the timeout call-back function.
	 * If the function returns != 0, the timer will be restarted, and thus
	 * creating a periodic timer.
	 */
	typedef int (*TimeoutReceiver)(void* userRef);

	/**
	 * Defines the type of a file-descriptor event call-back function.
	 */
	typedef void (*FdCallbackFn)(struct pollfd* pfd, void *ref);

	typedef enum TimerState {
	  Running,  								/* Timer is scheduled, waiting for time to expire */
	  Active,   								/* Timer has expired and timeout-handler is called */
	  Deleted   								/* Timer was deleted by timeout-handler */
	}TimerStateT;

	typedef struct TimerInternal {
	    int mS;
	    TimeoutReceiver sr;
	    void* ref;
	    struct timeval expires;
	    long int lastsec;						/* Used to detect system clock steppings */
	    TimerStateT state;
	    TimerInternal* next;
	}TimerInternalT;

	/**
	 * poll interface extensions
	 */
	typedef struct PollFn {
	    FdCallbackFn fn;
	    void* userRef;
	}PollFnT;

	typedef struct pollextensions {
	    unsigned int maxfd;
	    struct pollfd* spollfd;
	    unsigned int nfds;
	    PollFnT* spollfn;
	}pollextensionsT;

	typedef struct TimeHandle {
		struct timeval delay;	         		/* Time until the next timer expires */
		TimerInternalT* timers;        	/* List of running timers */
		TimerInternalT* freeTimerList; 	/* List of deleted reusable timers */
		int freeTimers;                      	/* Number of reusable timers */
		TimerInternalT* activeTimer;   	/* The active timer (if any) */
		pollextensionsT* polld;
	}TimeHandleT;

	/**
	 * Creates a "handle" used in subsequent "_r" functions.
	 * The "_r" functions are reentrant (thread safe).
	 */
	void* timerCreateHandle_r(void);

	/**
	 * Sets the maximum number of file descriptors.
	 * @param handle A handle returned by ::timerCreateHandle_r.
	 * @param maxfd The maximum number of file descriptors.
	 */
	void poll_maxfd(void* handle, int maxfd);

        /**
	 * Returns the delay until the next timer expires. This value is
	 * intended to be used in a `select' call.
	 * @param handle A handle returned by ::timerCreateHandle_r.
	 */
	struct timeval* timerGetDelay_r(void* handle);

	/**
	 * @deprecated Non thread-safe version of the ::timerGetDelay_r funtion.
	 */
	static void rescedule(struct TimeHandle* handle, struct TimerInternal* t, struct timeval* tv);

	/**
	 * Set a call-back function for events on a file descriptor.
	 * @param handle A handle returned by ::timerCreateHandle_r.
	 * @fn The call-back function.
	 * @pfd Defines the file descriptor and the events to monitor.
	 * @ref A user reference that is passed back in the call-back.
	 * @return 0 on success, -1 if failed.
	 */
	int poll_setcallback(void* handle, FdCallbackFn fn, struct pollfd pfd, void *ref);

	/**
	 * Remove a call-back function for a file descriptor.
	 * @param handle A handle returned by ::timerCreateHandle_r.
	 */
	void poll_unsetcallback(void* handle, int fd);

        /**
	 * Waits for an event on a file descriptor or a timeout and makes
	 * the appropriate call-backs. This function should be called in a
	 * conrol loop.
	 * @param handle A handle returned by ::timerCreateHandle_r.
	 * @return 0 on success, != 0 otherwise.
	 */
	int poll_execute(void* handle);
};

#endif //PmSaSelectTimer_H
