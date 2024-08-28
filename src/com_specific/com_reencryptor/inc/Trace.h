#ifndef __REENCRYPTOR_TRACE_H
#define __REENCRYPTOR_TRACE_H

#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <syslog.h>
#include <stdarg.h>
#include <poll.h>
#include <fcntl.h>
#include <csignal>
#include "TimerSelect.h"
#include "ImmUtil.h"

#define ERR_COM_RP(arg...) do { \
		traceWrite(LOG_ERR, arg); \
} while(0)

#define WARN_COM_RP(arg...) do { \
		traceWrite(LOG_WARNING, arg); \
} while(0)

#define INFO_COM_RP(arg...) do { \
		traceWrite(LOG_NOTICE, arg); \
} while(0)

#define DEBUG_COM_RP(arg...) do { \
		traceWrite(LOG_DEBUG, arg); \
} while(0)

#define LOG_API_CALL(f) \
  do { Errt = f; if (Errt == SA_AIS_ERR_TRY_AGAIN) usleep(USLEEP_ONE_SECOND);\
  } while (Errt == SA_AIS_ERR_TRY_AGAIN);


#define LOG_API_CALL_TRY_AGAIN(f) do {\
	unsigned int tryCount = 0;\
	for (Errt = f; Errt == SA_AIS_ERR_TRY_AGAIN && tryCount < AMF_LOG_MAX_TRIES; Errt = f, tryCount++){\
		usleep(AMF_LOG_TRY_INTERVAL);\
	}} while (0);

void traceWrite(int priority, const char* fmt, ...)
__attribute__ ((format(printf, 2, 3)));

class Trace
{
	static char* identityString;
	static SaLogHandleT			theLogHandle;
	static SaLogStreamHandleT	 	theLogStreamHandle;
	static SaNameT*				theLoggerName;
	static SaSelectionObjectT 		theSelectionObject;
	static SaNameT*				myLogStreamName;
	static pthread_t			OpenThreadId;
	static bool 				LogFileIsOpen;
	static SaLogSeverityFlagsT logSevFlags;
	static int ntf_event_pipe[2];
	static void* handle_ntf;
	bool debugsEnabled;

	Trace();
	static void* LogServiceOpenThread(void*);
	static void fdCallbackFn(struct pollfd* pfd, void *ref);
	static void logFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity);
	static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error);
	bool initLogStream();
	void finalizeLogStream();
	SaAisErrorT getSeverityFilter(int32_t* severityFilter);

public:

	static Trace& Instance() {
		static Trace sTrace;
		return sTrace;
	}

	~Trace();

	bool start();
	bool stop();
	bool comSaCfgLogStreamWrite(MwSpiSeverityT severity, const char* fmt, va_list arg);
	SaLogSeverityFlagsT getLogSeverityFlags();
};

#endif
