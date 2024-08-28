/* ***************************************************************************
* Copyright (C) 2012 by Ericsson AB
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
* File: PmtSaTrace.hxx
*
* Author: ejonajo 2011-09-01
*
* Reviewed: efaiami 2012-04-14
*
************************************************************************** */

#ifndef PMTSATRACE_HXX_
#define PMTSATRACE_HXX_

/**
* @file PmtSaTrace.hxx
*
* @brief Holds syslog access routines for trace etc
*
* This file holds all functionality needed for PMT-SA so that it
* can report errors etc to the syslog on the running node.  There
* are three levels of debug-writing that can be done, and they're
* actually called from macros so that they'll contain file and line
* information as well.
*/
#include <cstdio>
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <saLog.h>
#include "com_ericsson_common_comsa_pmtsa.h"
#include "imm_utils.h"
#include "saname_utils.h"
#include "MafMwSpiLog_1.h"

/*
* Trace tags, used by trace macros below , uses 32 bits => 32 different tags
*/
#define TRACE_TAG_LOG     (1<<0)
#define TRACE_TAG_ENTER   (1<<1)
#define TRACE_TAG_LEAVE   (1<<2)
#define TRACE_TAG_TRACE1  (1<<3)
#define TRACE_TAG_TRACE2  (1<<4)
#define TRACE_TAG_TRACE3  (1<<5)
#define TRACE_TAG_TRACE4  (1<<6)
#define TRACE_TAG_TRACE5  (1<<7)
#define TRACE_TAG_TRACE6  (1<<8)
#define TRACE_TAG_TRACE7  (1<<9)
#define TRACE_TAG_TRACE8  (1<<10)


#define TRACE_BUFFER_SIZE_PMTSA 4096
extern int trace_flag;
/*
* Global trace variable
*/
struct log_state_T {
	int level;
	int tags;
	int mode;
};
extern struct log_state_T g_Log;
static const unsigned int SleepTime = 1;

void log_control(const struct log_state_T* state, struct log_state_T* old);
extern char* _createString(char * buffer, char const* fmt, ...);
extern void enter_leave_log_print(const char *file, int line, int priority, int category, const char *format, ...);

#define LOG_PREFIX "PMT-SA:"
#define LOG_LEVEL_INFO LOG_INFO
#define LOG_LEVEL_DEBUG LOG_DEBUG
#define LOG_MODE_STDERR 16
#define TRACE_TAG_ENTER (1<<1)
#define TRACE_TAG_LEAVE (1<<2)
#define LOG_MODE_TIMESTAMP 2
#define LOG_MODE_FILE 4
#define LOG_MODE_SYSLOG 8
#define LOG_MODE_FILELINE 32
#define LOG_MODE_STDOUT 64

/* PMTSA TRACE DOMAIN */

#define ERR_PMTSA(arg...) do { \
		char buffer[TRACE_BUFFER_SIZE_PMTSA]; \
		tracepoint(com_ericsson_common_comsa_pmtsa, COMSA_ERROR, _createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	} while(0)

#define WARN_PMTSA(arg...) do { \
		char buffer[TRACE_BUFFER_SIZE_PMTSA]; \
		tracepoint(com_ericsson_common_comsa_pmtsa, COMSA_WARNING, _createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	} while(0)

#define LOG_PMTSA(arg...) do { \
		char buffer[TRACE_BUFFER_SIZE_PMTSA]; \
		tracepoint(com_ericsson_common_comsa_pmtsa, COMSA_LOG, _createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	} while(0)

#define ENTER_PMTSA() do { \
		tracepoint(com_ericsson_common_comsa_pmtsa, COMSA_ENTER, "Enter", __FILE__, __FUNCTION__, __LINE__);\
		if ((LOG_LEVEL_DEBUG <= g_Log.level) && (TRACE_TAG_ENTER & g_Log.tags)) { \
			enter_leave_log_print(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 9, "%s", __FUNCTION__); \
		} \
	} while(0)

#define LEAVE_PMTSA() do { \
		tracepoint(com_ericsson_common_comsa_pmtsa, COMSA_LEAVE, "Leave", __FILE__, __FUNCTION__, __LINE__);\
		if ((LOG_LEVEL_DEBUG <= g_Log.level) && (TRACE_TAG_LEAVE & g_Log.tags)) { \
			enter_leave_log_print(__FILE__, __LINE__, LOG_LEVEL_DEBUG, 10, "%s", __FUNCTION__); \
		} \
	} while(0)

#define DEBUG_PMTSA(arg...) do { \
		char buffer[TRACE_BUFFER_SIZE_PMTSA]; \
		tracepoint(com_ericsson_common_comsa_pmtsa, COMSA_DEBUG, _createString(buffer, arg), __FILE__, __FUNCTION__, __LINE__);\
	} while(0)

#ifdef BUILD_WITH_TRACE_TESTS

#define TRACE_ALL_LEVELS_PMTSA() do { \
		ENTER_PMTSA();\
		ERR_PMTSA("COMSA TEST: ERR test string in %s", "pmtsa");\
		WARN_PMTSA("COMSA TEST: WARN test string in %s", "pmtsa");\
		LOG_PMTSA("COMSA TEST: LOG test string in %s", "pmtsa");\
		DEBUG_PMTSA("COMSA TEST: DEBUG string in %s", "pmtsa");\
		LEAVE_PMTSA(); \
	} while(0)

/* Add a call to this macro in PerfMgmtTransferSA.cxx startup to get all available tracepoints */
#define PRINT_ALL_TRACE_DOMAIN_WITH_ALL_LEVELS_PMTSA() do { \
		TRACE_ALL_LEVELS_PMTSA(); \
	} while(0)
#endif /* BUILD_WITH_TRACE_TESTS */

namespace PmtSa {

	/**
* @ingroup PmtSa
*
* This class handles writing to syslog.  The supposed way of using
* it is by the predefined macros starting with PMTSA_.
*
*/
	class PmtSaTrace {

	public:
		/**
	* @ingroup PmtSa
	*
	* Calls the private method that does the actual print of info
	* to the syslog.  The last parameter is a 'printf'-formatted
	* string, and then all the arguments are handled.
	*
	* @param[in]   priority    Level of importance
	* @param[in]   fileName    Typically given by C/C++ preprocessor
	* @param[in]   lineNumber  Typically given by C/C++ preprocessor
	* @param[in]   format      Printf-like format string for printout
	*
	* The method takes a variable amount of parameters, the number of
	* parameters should match the number of variables that are supposed
	* to be printed via the format string.
	*/
		static void log(int priority, const char* fileName,
		const int lineNumber, const char* format, ...);
		static void debug_log(int priority, const char* fileName,
		const int lineNumber, const char* format, ...);
		// Public functions to initialize/finalize the LogM log service
                bool initLogStream();
                void finalizeLogStream();
		static PmtSaTrace& Instance() {
				static PmtSaTrace sTrace;
				return sTrace;
			}

	private:
		static char* pmtSaIdentityString;
		static SaLogHandleT			theLogHandle;
		static SaLogStreamHandleT	 	theLogStreamHandle;
		static SaNameT				theLoggerName;
		static SaSelectionObjectT 		theSelectionObject;
		static SaNameT				pmtSaLogStreamName;
		static pthread_t			OpenThreadId;
		static bool 				LogFileIsOpen;
		static SaLogSeverityFlagsT logSevFlags;
		static int ntf_event_pipe[2];
		static void* handle_ntf;
		static void* LogServiceOpenThread(void*);
		static void fdCallbackFn(struct pollfd* pfd, void *ref);
		static void logFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity);
		static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error);
		void setComSaLogSevFilter();
                bool comSaCfgLogStreamWrite(MwSpiSeverityT severity, const char* buffer);
		SaAisErrorT getSeverityFilter(SaImmAccessorHandleT accessorHandle, char *dnName, int32_t* severityFilter);

		static void vlog(int priority, const char* fileName,
		const int lineNumber, char const* format, va_list ap);

	};

	/*
* The PMTSA_DEBUG macro should only be used for debugging :), thus
* it's not part of the released code.
*/

#define NDEBUG

#ifdef NDEBUG
#define PMTSA_DEBUG(arg...) PmtSa::PmtSaTrace::debug_log(LOG_DEBUG, __FILE__, __LINE__,arg)
#else
#define PMTSA_DEBUG(arg...)
#endif

	/**
* @ingroup PmtSa
*
* Use this macro for writing standard log-info like messages to the syslog.
*/
#define PMTSA_LOG(arg...) PmtSa::PmtSaTrace::log(LOG_NOTICE, __FILE__, __LINE__,arg)

	/**
* @ingroup PmtSa
*
* Use this macro for writing warning messages to the syslog.
*/
#define PMTSA_WARN(arg...) PmtSa::PmtSaTrace::log(LOG_WARNING, __FILE__, __LINE__,arg)

	/**
* @ingroup PmtSa
*
* Use this macro for writing error messages to the syslog.
*/
#define PMTSA_ERR(arg...) PmtSa::PmtSaTrace::log(LOG_ERR, __FILE__, __LINE__,arg)
}

#endif /* PMTSATRACE_HXX_ */
