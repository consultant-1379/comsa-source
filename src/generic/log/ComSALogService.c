/*
 * ComSALogService.c --
 *
 *  Copyright (C) 2010 by Ericsson AB
 *  S - 125 26  STOCKHOLM
 *  SWEDEN, tel int + 46 10 719 0000
 *
 *  The copyright to the computer program herein is the property of
 *  Ericsson AB. The program may be used and/or copied only with the
 *  written permission from Ericsson AB, or in accordance with the terms
 *  and conditions stipulated in the agreement/contract under which the
 *  program has been supplied.
 *
 *  All rights reserved.
 *
 *  Reviewed: uablrek 2010-06-11
 *
 *  2011-02-22 egorped. Updated to survive the case where the log file cannot be open within
 *  					the stipulated time, often due to long NFS fail-over delays.
 *  					Now the log file open will be handled by a separate thread which
 *  					will continue to try until either it succceds or close is called.
 *  					Any writes to the log file will be ignored until the log is open and
 *  					the messages will be re-routed to syslog.
 *
 *
 * Modify: efaiami 2011-02-22 for log and trace function
 *
 * Modify: ejnolsz 2012-06-08 Add support for alarm and alert logs
 *
 * Modify: eaparob 2012-06-13 modify logWrite() and added alarmLogWriteLogCallbackT(), alarm_fdCallbackFn(),
 *                            alarmLogServiceOpenThread(), AlarmAndAlertLogServiceClose(), alarmAlertFilter(),
 *                            alarmLogWrite() and symlink creation for NBI visibility of alarm and alert type of logs
 *
 * Reviewed: efaiami 2010-07-31 support for alarm and alert logs
 *
 * Modify: xnikvap 2012-08-30 fixed compiler warnings
 * Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 * Modify: xdonngu 2013-12-12  run cppcheck 1.62 and fix errors and warnings
 * Modify: uabjoy 2014-03-18 Modifying to support Trace CC.
 * Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *******************************************************************************/

#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <poll.h>
#include <trace.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <saLog.h>
#include "ComSALogService.h"
#include <MafMwSpiServiceIdentities_1.h>
#include <ctype.h>
#include "ComSA.h"
#include "SelectTimer.h"
#include "imm_utils.h"
#include "LoggingEvent.h"
/* Log-to-Trace duplication if COM_LOGGING_LEVEL=8. */
#include <MafMwSpiTrace_1.h>
static void (*traceWrite)(uint32_t group, const char* traceMessageStr) = NULL;
#include "InternalTrace.h"
#include <fcntl.h>

extern int trace_flag;
static bool log_status=false;
static bool alarm_status=false;
static bool alert_status=false;
extern void stop_ntf_polling(struct pollfd* pfd, void *ref);
#ifdef UNIT_TEST
void stop_ntf_polling(struct pollfd* pfd, void *ref)
{
        return;
}
#endif

/*
 * constants
 *
 */

static char* cmdLogString = NULL;
static const unsigned int SleepTime = 1;

/*
 * globals
 */
int ntf_event_pipe[2];
SaLogFileCreateAttributesT_2 alarmLogFileAttributes;
SaLogFileCreateAttributesT_2 alertLogFileAttributes;
SaLogFileCreateAttributesT_2 cmdLogFileAttributes;
//SaLogFileCreateAttributesT_2 secLogFileAttributes;


static SaLogHandleT			theLogHandle = 0;
static SaLogHandleT			alarmLogHandle = 0;
static SaLogHandleT			alertLogHandle = 0;
static SaLogHandleT			cmdLogHandle = 0;
static SaLogHandleT			secLogHandle = 0;

int alarmLogFileIsOpen = FALSE;
SaNameT alarmLoggerName;
SaLogStreamHandleT alarmLogStreamHandle = 0;

int alertLogFileIsOpen = FALSE;
SaNameT alertLoggerName;
SaLogStreamHandleT alertLogStreamHandle = 0;

int LogFileIsOpen = FALSE;
SaNameT theLoggerName;
SaLogStreamHandleT theLogStreamHandle = 0;

int cmdLogFileIsOpen = FALSE;
SaNameT cmdLoggerName;
SaLogStreamHandleT cmdLogStreamHandle = 0;

int secLogFileIsOpen = FALSE;
SaNameT secLoggerName;
SaLogStreamHandleT secLogStreamHandle = 0;

static bool logMEnabled = false;


static SaSelectionObjectT 	theSelectionObject;
static SaSelectionObjectT 	alarmSelectionObject;
static SaSelectionObjectT 	alertSelectionObject;
static SaSelectionObjectT 	cmdSelectionObject;
static SaSelectionObjectT 	secSelectionObject;
static SaNameT				myLogStreamName;
static SaNameT				alarmLogStreamName;
static SaNameT				alertLogStreamName;
static SaNameT				cmdLogStreamName;
static SaNameT				secLogStreamName;

static pthread_t			OpenThreadId = 0;
static pthread_t			alarmOpenThreadId = 0;
static pthread_t			alertOpenThreadId = 0;
static pthread_t			cmdLogOpenThreadId = 0;
static pthread_t			secLogOpenThreadId = 0;

bool  updateAlarmLogFileNameAttr = false;
bool  updateAlertLogFileNameAttr = false;
bool  updateCmdLogFileNameAttr = false;
bool  updateSecLogFileNameAttr = false;
static const char* logAdminOwnerName = "OAMSALOG";
#ifndef UNIT_TEST
static SaLogSeverityFlagsT alarmLogSevFlags = SA_LOG_SEV_FLAG_ALL;
static SaLogSeverityFlagsT alertLogSevFlags = SA_LOG_SEV_FLAG_ALL;
static SaLogSeverityFlagsT saSystemLogSevFlags =  SALOG_SEV_FLAG_INFO_OFF;
static SaLogSeverityFlagsT cmdLogSevFlags =  SALOG_SEV_FLAG_INFO_OFF;
static SaLogSeverityFlagsT secLogSevFlags =  SALOG_SEV_FLAG_INFO_OFF;
static char* IdentityString = NULL;
#else
char* IdentityString = NULL;
SaLogSeverityFlagsT alarmLogSevFlags = SA_LOG_SEV_FLAG_ALL;
SaLogSeverityFlagsT alertLogSevFlags = SA_LOG_SEV_FLAG_ALL;
SaLogSeverityFlagsT saSystemLogSevFlags =  SALOG_SEV_FLAG_INFO_OFF;
SaLogSeverityFlagsT cmdLogSevFlags =  SALOG_SEV_FLAG_INFO_OFF;
SaLogSeverityFlagsT secLogSevFlags =  SALOG_SEV_FLAG_INFO_OFF;
#endif

#define MAX_LINE_BUFF 1024

/* Forward declaration of functions
 *
 */

#ifndef UNIT_TEST
static MafReturnT logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer);
#else
MafReturnT logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer);
#endif

extern void pushComDebugLogLevelEvent();
void pushComLogLevelEvents();


void getLoggerInfo( MwSpiFacilityT facility,  MwSpiSeverityT* severity, int** isLogFileOpen, SaNameT **loggerName, SaLogStreamHandleT** logStreamHandle, bool* isLogAllowed);

int createSymLinkForAlarmsAndAlerts(const char *path, const char *directory, const char * linkDir, const char * linkName);

/* SDP 1694 - support MAF SPI */
#ifndef UNIT_TEST
static MafReturnT maf_logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer);
#else
MafReturnT maf_logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer);
#endif


static MafMwSpiLog_1T maf_InterfaceStruct = {
	MafMwSpiLog_1Id, maf_logWrite
};



void pushSaSystemLogLevelEvent(){
	MafStreamLoggingLevelChangeValue_2T event;
	event.newLevel = saSystemLogSevFlags;
	event.streamType = SALOGSYSTEM_LOG;
	push_LogEventProducer(&event);
}

void pushComSecLogLevelEvent(){
	MafStreamLoggingLevelChangeValue_2T event;
	event.newLevel = secLogSevFlags;
	event.streamType = SECURITY_LOG;
	push_LogEventProducer(&event);
}

void pushComCommandLogLevelEvent(){
	MafStreamLoggingLevelChangeValue_2T event;
	event.newLevel = cmdLogSevFlags;
	event.streamType = COMMAND_LOG;
	push_LogEventProducer(&event);
}

void pushComLogLevelEvents(){
	if(logMEnabled){
		pushSaSystemLogLevelEvent();
		pushComSecLogLevelEvent();
		pushComCommandLogLevelEvent();
		pushComDebugLogLevelEvent();
	}
}
/*
 * Callbacks
 */

static void logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	if (error != SA_AIS_OK){
		if(error==SA_AIS_ERR_TRY_AGAIN)
		{
		  if(log_status==false){
			 syslog(LOG_USER*8 + LOG_WARNING,"logWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
			 log_status=true;
		  }
		}
		else{
		  syslog(LOG_USER*8 + LOG_WARNING,"logWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
		  log_status=false;
		}
	}
}

static void alarmLogWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	if (error != SA_AIS_OK){
		if(error==SA_AIS_ERR_TRY_AGAIN)
		{
		  if(alarm_status==false){
			 syslog(LOG_USER*8 + LOG_WARNING,"alarmLogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
			 alarm_status = true;
		  }
		}
		else{
		  syslog(LOG_USER*8 + LOG_WARNING,"alarmLogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
		  alarm_status = false;
		}
	}
}

static void alertLogWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	if (error != SA_AIS_OK){
		if(error==SA_AIS_ERR_TRY_AGAIN)
		{
		  if(alert_status==false){
			 syslog(LOG_USER*8 + LOG_WARNING,"alertLogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
			 alert_status = true;
		  }
		}
		else{
		  syslog(LOG_USER*8 + LOG_WARNING,"alertLogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
		  alert_status = false;
		}
	}
}

static void cmdLogWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	if (error != SA_AIS_OK)
		WARN_MWSA_LOG("cmdLogWriteLogCallbackT, error=%d", (int)error);
}

static void secLogWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	if (error != SA_AIS_OK)
		WARN_MWSA_LOG("secLogWriteLogCallbackT, error=%d", (int)error);
}

static void alarmLogFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("com alarm filter callback received severity = %d, stream handle = %llu",logSeverity, logStreamHandle);
	alarmLogSevFlags = logSeverity;
	LEAVE_MWSA_LOG();
}

static void alertLogFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("com alert filter callback received severity = %d, stream handle = %llu",logSeverity, logStreamHandle);
	alertLogSevFlags = logSeverity;
	LEAVE_MWSA_LOG();
}

static void logFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("logFilterSetCallbackT received with severity = %d and stream handle = %llu",logSeverity, logStreamHandle);
	saSystemLogSevFlags = logSeverity;
	pushSaSystemLogLevelEvent();
	LEAVE_MWSA_LOG();
}
static void cmdLogFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("cmdLogFilterSetCallbackT received with severity = %d and stream handle = %llu",logSeverity, logStreamHandle);
	cmdLogSevFlags = logSeverity;
	pushComCommandLogLevelEvent();
	LEAVE_MWSA_LOG();
}

static void secLogFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("secLogFilterSetCallbackT received with severity = %d and stream handle = %llu",logSeverity, logStreamHandle);
	secLogSevFlags = logSeverity;
	pushComSecLogLevelEvent();
	LEAVE_MWSA_LOG();
}

static void fdCallbackFn(struct pollfd* pfd, void *ref)
{
	ENTER_MWSA_LOG();
	SaAisErrorT Errt;
	Errt = saLogDispatch(theLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK)
		WARN_MWSA_LOG("saLogDispatch FAILED: %d", Errt);
	LEAVE_MWSA_LOG();
}

static void alarm_fdCallbackFn(struct pollfd* pfd, void *ref)
{
	ENTER_MWSA_LOG();
	SaAisErrorT Errt;
	Errt = saLogDispatch(alarmLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK)
		WARN_MWSA_LOG("saLogDispatch FAILED for alarms: %d", Errt);
	LEAVE_MWSA_LOG();
}

static void alert_fdCallbackFn(struct pollfd* pfd, void *ref)
{
	ENTER_MWSA_LOG();
	SaAisErrorT Errt;
	Errt = saLogDispatch(alertLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK)
		WARN_MWSA_LOG("saLogDispatch FAILED for alerts: %d", Errt);
	LEAVE_MWSA_LOG();
}

static void cmd_fdCallbackFn(struct pollfd* pfd, void *ref)
{
	ENTER_MWSA_LOG();
	SaAisErrorT Errt;
	Errt = saLogDispatch(cmdLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK)
		WARN_MWSA_LOG("saLogDispatch FAILED for cmd logs: %d", Errt);
	LEAVE_MWSA_LOG();
}

static void sec_fdCallbackFn(struct pollfd* pfd, void *ref)
{
	ENTER_MWSA_LOG();
	SaAisErrorT Errt;
	Errt = saLogDispatch(secLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK)
		WARN_MWSA_LOG("saLogDispatch FAILED for security logs: %d", Errt);
	LEAVE_MWSA_LOG();
}

/*
*	Global i/f
*/


MafMwSpiLog_1T* maf_ExportLogServiceInterface(void)
{
	ENTER_MWSA_LOG();
	LEAVE_MWSA_LOG();
	return (MafMwSpiLog_1T*)&maf_InterfaceStruct;
}

static void* LogServiceOpenThread(void * arg)
{
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;
	ENTER_MWSA_LOG();
	struct pollfd pfd;
	int   PthreadResult = 0;

	/* Set up logsteram name */
	saNameSet(SA_LOG_STREAM_SYSTEM, &myLogStreamName);

	DEBUG_MWSA_LOG("LogServiceOpenThread Calling pthread_detach");
	if ((PthreadResult = pthread_detach(OpenThreadId)) != 0)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to detach thread ",PthreadResult);
	}

	DEBUG_MWSA_LOG("Executing LogServiceOpenThread");
	for (;Errt != SA_AIS_OK;)
	{
		//myLogStreamName.value[myLogStreamName.length] = '\0';
		DEBUG_MWSA_LOG("Calling saLogStreamOpen_2 Los stream name = %s length = %u", saNameGet(&myLogStreamName), (unsigned) strlen(saNameGet(&myLogStreamName)));
		LOG_API_CALL(saLogStreamOpen_2(
					 theLogHandle,
					 &myLogStreamName,
					 (SaLogFileCreateAttributesT_2*)NULL,
					 0,
					 SA_TIME_ONE_MINUTE,
					 &theLogStreamHandle));
		DEBUG_MWSA_LOG("After saLogStreamOpen_2 Return value = %d, LogStreamHandle = %lld",Errt,theLogStreamHandle);
		if (Errt == SA_AIS_ERR_TIMEOUT)
		{
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 returned time-out retrying");
		}
		else if (Errt == SA_AIS_ERR_UNAVAILABLE)
		{
			// If not currently a member of the cluster, we might become later so sleep a while and try again
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 returned SA_AIS_ERR_UNAVAILABLE retrying");
			struct timespec remainingTime;
			struct timespec sleepTime;
			sleepTime.tv_nsec 	= 0;
			sleepTime.tv_sec	= NANOSLEEP_SECONDS;
			for (;;)
			{
				if (nanosleep(&sleepTime,&remainingTime) == 0)
				{
					// Awoken from sleep as planned
					break;
				}
				else
				{
					if (errno == EINTR)
					{
						// Woken by someone sending a signal. Go back to sleep for the remaining period
						sleepTime.tv_nsec = remainingTime.tv_nsec;
						sleepTime.tv_sec  = remainingTime.tv_sec;
					}
					else
					{
						syslog(LOG_USER*8 + LOG_ERR,"%s %s %d %s", _CC_NAME_UPPERCASE, "SA -- Call to nanosleep returned",errno,"exiting");
						goto end_exit;
					}
				}
			}
		}
	} // end for

	LOG_MWSA_LOG("SaLogSystem Service is successfully opened");

	pfd.events = POLLIN;
	pfd.fd = theSelectionObject;
	if (poll_setcallback(comSASThandle_ntf, fdCallbackFn, pfd, NULL) != 0)
	{
		WARN_MWSA_LOG("poll_setcallback failed");
		Errt = MafFailure;
		goto end_exit;
	}
	LogFileIsOpen = TRUE;
	OpenThreadId  = 0;
	if (pipe(ntf_event_pipe) != 0) {
		ERR_COMSA("Failed to create pipe");
		Errt = MafFailure;
		goto end_exit;
	}else {
		/* Set the read-end of the pipe in non-blocking mode */
		int rc = fcntl(ntf_event_pipe[0], F_GETFL);
		if (rc < 0) abort();
		if (fcntl(ntf_event_pipe[0], F_SETFL, rc | O_NONBLOCK)) abort();
	}
	pfd.events = POLLIN;
	pfd.fd = ntf_event_pipe[0];
	if(poll_setcallback(comSASThandle_ntf, stop_ntf_polling, pfd, NULL) != 0)
	{
		WARN_MWSA_LOG("poll_setcallback failed for alerts");
		Errt = MafFailure;
		goto end_exit;
	}
end_exit:
	pthread_exit(&Errt);
	return NULL;
}

static void* alarmLogServiceOpenThread(void * arg)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("Executing alarmLogServiceOpenThread");
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;
	struct pollfd pfd;
	int   PthreadResult = 0;

	/* Set up logstream name */
	saNameSet((const char*)ALARM_LOG_STREAM_DN , &alarmLogStreamName);

	DEBUG_MWSA_LOG("alarmLogServiceOpenThread Calling pthread_detach alarmLogServiceOpenThread");
	if ((PthreadResult = pthread_detach(alarmOpenThreadId)) != 0)
	{
		DEBUG_MWSA_LOG("LogService failed to detach thread alarmLogServiceOpenThread PthreadResult = %d",PthreadResult);
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to detach thread alarmLogServiceOpenThread ",PthreadResult);
	}

	DEBUG_MWSA_LOG("alarmLogServiceOpenThread() alarmLogStreamName.value = %s, alarmLogStreamName.length = %u", saNameGet(&alarmLogStreamName), (unsigned) strlen(saNameGet(&alarmLogStreamName)));
	for (;Errt != SA_AIS_OK;)
	{
		DEBUG_MWSA_LOG("Calling saLogStreamOpen_2 alarm log stream name = %s lenght = %u", saNameGet(&alarmLogStreamName), (unsigned) strlen(saNameGet(&alarmLogStreamName)));
		LOG_API_CALL(saLogStreamOpen_2(
					 alarmLogHandle,
					 &alarmLogStreamName,
					 (SaLogFileCreateAttributesT_2*)NULL,
					 0,
					 SA_TIME_ONE_MINUTE,
					 &alarmLogStreamHandle));
		DEBUG_MWSA_LOG("After saLogStreamOpen_2 for alarms Return value = %d, LogStreamHandle = %llu",Errt,alarmLogStreamHandle);
		if (Errt == SA_AIS_ERR_TIMEOUT)
		{
			DEBUG_MWSA_LOG("alarmLogServiceOpenThread() Errt = SA_AIS_ERR_TIMEOUT");
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 for alarms returned time-out retrying");
		}
		else if (Errt == SA_AIS_ERR_UNAVAILABLE)
		{
			// If not currently a member of the cluster, we might become later so sleep a while and try again
			DEBUG_MWSA_LOG("alarmLogServiceOpenThread() Errt = SA_AIS_ERR_UNAVAILABLE");
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 for alarms NOT returned SA_AIS_OK retrying");
			struct timespec remainingTime;
			struct timespec sleepTime;
			sleepTime.tv_nsec 	= 0;
			sleepTime.tv_sec	= NANOSLEEP_SECONDS;
			for (;;)
			{
				DEBUG_MWSA_LOG("After saLogStreamOpen_2 != SA_AIS_OK sleep then try again");
				if (nanosleep(&sleepTime,&remainingTime) == 0)
				{
					// Awoken from sleep as planned
					break;
				}
				else
				{
					if (errno == EINTR)
					{
						// Woken by someone sending a signal. Go back to sleep for the remaining period
						sleepTime.tv_nsec = remainingTime.tv_nsec;
						sleepTime.tv_sec  = remainingTime.tv_sec;
					}
					else
					{
						syslog(LOG_USER*8 + LOG_ERR,"%s %s %d %s", _CC_NAME_UPPERCASE, "SA -- Call to nanosleep for alarms returned",errno,"exiting");
						DEBUG_MWSA_LOG("%s SA -- Call to nanosleep for alarms returned (%d)", _CC_NAME_UPPERCASE, errno);
						goto end_exit;
					}
				}
			}
		}
		else if (Errt != SA_AIS_OK)
		{
			time_t seconds = 3;
			struct timespec sleepPeriod = { seconds, 0 };
			struct timespec unusedPeriod;
			WARN_MWSA_LOG("alarmLogServiceOpenThread returned (%d), wait %d seconds then retry",Errt,(int)seconds);
			while(nanosleep(&sleepPeriod, &unusedPeriod) != 0);
		}
	} // end for

	LOG_MWSA_LOG("AlarmLogService is successfully opened");
	alarmLogFileIsOpen = TRUE;
	pfd.events = POLLIN;
	pfd.fd = alarmSelectionObject;
	if(poll_setcallback(comSASThandle_ntf, alarm_fdCallbackFn, pfd, NULL) != 0)
	{
		WARN_MWSA_LOG("poll_setcallback failed for alarms");
		Errt = MafFailure;
		goto end_exit;
	}
	alarmOpenThreadId  = 0;
end_exit:
	DEBUG_MWSA_LOG("Exiting alarmLogServiceOpenThread");
	pthread_exit(&Errt);
	LEAVE_MWSA_LOG();
	return NULL; // Just to get rid of a warning from the compiler
}

static void* alertLogServiceOpenThread(void * arg)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("Executing alertLogServiceOpenThread");
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;
	struct pollfd pfd;
	int	PthreadResult = 0;

	/* Set up logstream name */
	saNameSet((const char*)ALERT_LOG_STREAM_DN, &alertLogStreamName);

	DEBUG_MWSA_LOG("alertLogServiceOpenThread Calling pthread_detach alertLogServiceOpenThread");
	if ((PthreadResult = pthread_detach(alertOpenThreadId)) != 0)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to detach thread alertLogServiceOpenThread ",PthreadResult);
	}

	for (;Errt != SA_AIS_OK;)
	{
		DEBUG_MWSA_LOG("Calling saLogStreamOpen_2 alert log stream name = %s length = %d", saNameGet(&alertLogStreamName), saNameLen(&alertLogStreamName));
		LOG_API_CALL(saLogStreamOpen_2(
					alertLogHandle,
					 &alertLogStreamName,
					 (SaLogFileCreateAttributesT_2*)NULL,
					 0,
					 SA_TIME_ONE_MINUTE,
					 &alertLogStreamHandle));
		DEBUG_MWSA_LOG("After saLogStreamOpen_2 for alerts Return value = %d, LogStreamHandle = %llu",Errt,alertLogStreamHandle);
		if (Errt == SA_AIS_ERR_TIMEOUT)
		{
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 for alert returned time-out retrying");
		}
		else if (Errt == SA_AIS_ERR_UNAVAILABLE)
		{
			// If not currently a member of the cluster, we might become later so sleep a while and try again
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 for alerts NOT returned SA_AIS_OK retrying");
			struct timespec remainingTime;
			struct timespec sleepTime;
			sleepTime.tv_nsec 	= 0;
			sleepTime.tv_sec	= NANOSLEEP_SECONDS;
			for (;;)
			{
				DEBUG_MWSA_LOG("After saLogStreamOpen_2 != SA_AIS_OK sleep then try again");
				if (nanosleep(&sleepTime,&remainingTime) == 0)
				{
					// Awoken from sleep as planned
					break;
				}
				else
				{
					if (errno == EINTR)
					{
						// Woken by someone sending a signal. Go back to sleep for the remaining period
						sleepTime.tv_nsec = remainingTime.tv_nsec;
						sleepTime.tv_sec  = remainingTime.tv_sec;
					}
					else
					{
						syslog(LOG_USER*8 + LOG_ERR,"%s %s %d %s", _CC_NAME_UPPERCASE, "SA -- Call to nanosleep for alert returned",errno,"exiting");
						DEBUG_MWSA_LOG("%s SA -- Call to nanosleep for alerts returned (%d)", _CC_NAME_UPPERCASE, errno);
						goto end_exit;
					}
				}
			}
		} else if (Errt != SA_AIS_OK) {
			time_t seconds = 3;
			struct timespec sleepPeriod = { seconds, 0 };
			struct timespec unusedPeriod;
			WARN_MWSA_LOG("alertLogServiceOpenThread returned (%d), wait %d seconds then retry",Errt,(int)seconds);
			while(nanosleep(&sleepPeriod, &unusedPeriod) != 0);
		}
	} // end for

	LOG_MWSA_LOG("AlertLogService is successfully opened");
	alertLogFileIsOpen = TRUE;
	pfd.events = POLLIN;
	pfd.fd = alertSelectionObject;
	if(poll_setcallback(comSASThandle_ntf, alert_fdCallbackFn, pfd, NULL) != 0)
	{
		WARN_MWSA_LOG("poll_setcallback failed for alerts");
		Errt = MafFailure;
		goto end_exit;
	}

	alertOpenThreadId  = 0;
end_exit:
	DEBUG_MWSA_LOG("Exiting alertLogServiceOpenThread");
	pthread_exit(&Errt);
	LEAVE_MWSA_LOG();
	return NULL; // Just to get rid of a warning from the compiler
}

static void* cmdLogServiceOpenThread(void * arg)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("Executing cmdLogServiceOpenThread");
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;
	struct pollfd pfd;
	int	PthreadResult = 0;

	/* Set up logstream name */
	saNameSet((const char*)CMD_LOG_STREAM_DN, &cmdLogStreamName);

	DEBUG_MWSA_LOG("cmdLogServiceOpenThread Calling pthread_detach cmdLogServiceOpenThread");
	if ((PthreadResult = pthread_detach(cmdLogOpenThreadId)) != 0)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to detach thread cmdLogServiceOpenThread ",PthreadResult);
	}

	for (;Errt != SA_AIS_OK;)
	{
		DEBUG_MWSA_LOG("Calling saLogStreamOpen_2 cmd log stream name = %s length = %d", saNameGet(&cmdLogStreamName), saNameLen(&cmdLogStreamName));
		LOG_API_CALL(saLogStreamOpen_2(
					cmdLogHandle,
					 &cmdLogStreamName,
					 (SaLogFileCreateAttributesT_2*)NULL,
					 0,
					 SA_TIME_ONE_MINUTE,
					 &cmdLogStreamHandle));
		DEBUG_MWSA_LOG("After saLogStreamOpen_2 for cmds Return value = %d, LogStreamHandle = %llu",Errt,cmdLogStreamHandle);
		if (Errt == SA_AIS_ERR_TIMEOUT)
		{
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 for cmd returned time-out retrying");
		}
		else if (Errt == SA_AIS_ERR_UNAVAILABLE)
		{
			// If not currently a member of the cluster, we might become later so sleep a while and try again
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 for cmds NOT returned SA_AIS_OK retrying");
			struct timespec remainingTime;
			struct timespec sleepTime;
			sleepTime.tv_nsec 	= 0;
			sleepTime.tv_sec	= NANOSLEEP_SECONDS;
			for (;;)
			{
				DEBUG_MWSA_LOG("After saLogStreamOpen_2 != SA_AIS_OK sleep then try again");
				if (nanosleep(&sleepTime,&remainingTime) == 0)
				{
					// Awoken from sleep as planned
					break;
				}
				else
				{
					if (errno == EINTR)
					{
						// Woken by someone sending a signal. Go back to sleep for the remaining period
						sleepTime.tv_nsec = remainingTime.tv_nsec;
						sleepTime.tv_sec  = remainingTime.tv_sec;
					}
					else
					{
						syslog(LOG_USER*8 + LOG_ERR,"%s %s %d %s", _CC_NAME_UPPERCASE, "SA -- Call to nanosleep for cmds returned",errno,"exiting");
						DEBUG_MWSA_LOG("%s SA -- Call to nanosleep for cmds returned (%d)", _CC_NAME_UPPERCASE, errno);
						goto end_exit;
					}
				}
			}
		} else if (Errt != SA_AIS_OK) {
			time_t seconds = 3;
			struct timespec sleepPeriod = { seconds, 0 };
			struct timespec unusedPeriod;
			WARN_MWSA_LOG("cmdLogServiceOpenThread returned (%d), wait %d seconds then retry",Errt,(int)seconds);
			while(nanosleep(&sleepPeriod, &unusedPeriod) != 0);
		}
	} // end for

	syslog(LOG_USER*8 + LOG_INFO, "CmdLogService is successfully opened");
	cmdLogFileIsOpen = TRUE;
	pfd.events = POLLIN;
	pfd.fd = cmdSelectionObject;
	if(poll_setcallback(comSASThandle_ntf, cmd_fdCallbackFn, pfd, NULL) != 0)
	{
		WARN_MWSA_LOG("poll_setcallback failed for cmds");
		Errt = MafFailure;
		goto end_exit;
	}

	cmdLogOpenThreadId  = 0;
end_exit:
	DEBUG_MWSA_LOG("Exiting cmdLogServiceOpenThread");
	pthread_exit(&Errt);
	LEAVE_MWSA_LOG();
	return NULL; // Just to get rid of a warning from the compiler
}

static void* secLogServiceOpenThread(void * arg)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("Executing secLogServiceOpenThread");
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;
	struct pollfd pfd;
	int	PthreadResult = 0;

	/* Set up logstream name */
	saNameSet((const char*)SECURITY_LOG_STREAM_DN, &secLogStreamName);

	DEBUG_MWSA_LOG("secLogServiceOpenThread Calling pthread_detach secLogServiceOpenThread");
	if ((PthreadResult = pthread_detach(secLogOpenThreadId)) != 0)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to detach thread secLogServiceOpenThread ",PthreadResult);
	}

	for (;Errt != SA_AIS_OK;)
	{
		DEBUG_MWSA_LOG("Calling saLogStreamOpen_2 security log stream name = %s length = %d", saNameGet(&secLogStreamName), saNameLen(&secLogStreamName));
		LOG_API_CALL(saLogStreamOpen_2(
					secLogHandle,
					 &secLogStreamName,
					 (SaLogFileCreateAttributesT_2*)NULL,
					 0,
					 SA_TIME_ONE_MINUTE,
					 &secLogStreamHandle));
		DEBUG_MWSA_LOG("After saLogStreamOpen_2 for security logs Return value = %d, LogStreamHandle = %llu",Errt,secLogStreamHandle);
		if (Errt == SA_AIS_ERR_TIMEOUT)
		{
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 for security logs returned time-out retrying");
		}
		else if (Errt == SA_AIS_ERR_UNAVAILABLE)
		{
			// If not currently a member of the cluster, we might become later so sleep a while and try again
			syslog(LOG_USER*8 + LOG_WARNING,"%s %s", _CC_NAME_UPPERCASE, "SA -- Call to saLogStreamOpen_2 for security logs NOT returned SA_AIS_OK retrying");
			struct timespec remainingTime;
			struct timespec sleepTime;
			sleepTime.tv_nsec 	= 0;
			sleepTime.tv_sec	= NANOSLEEP_SECONDS;
			for (;;)
			{
				DEBUG_MWSA_LOG("After saLogStreamOpen_2 != SA_AIS_OK sleep then try again");
				if (nanosleep(&sleepTime,&remainingTime) == 0)
				{
					// Awoken from sleep as planned
					break;
				}
				else
				{
					if (errno == EINTR)
					{
						// Woken by someone sending a signal. Go back to sleep for the remaining period
						sleepTime.tv_nsec = remainingTime.tv_nsec;
						sleepTime.tv_sec  = remainingTime.tv_sec;
					}
					else
					{
						syslog(LOG_USER*8 + LOG_ERR,"%s %s %d %s", _CC_NAME_UPPERCASE, "SA -- Call to nanosleep for security logs returned",errno,"exiting");
						DEBUG_MWSA_LOG("%s SA -- Call to nanosleep for security logs returned (%d)", _CC_NAME_UPPERCASE, errno);
						goto end_exit;
					}
				}
			}
		} else if (Errt != SA_AIS_OK) {
			time_t seconds = 3;
			struct timespec sleepPeriod = { seconds, 0 };
			struct timespec unusedPeriod;
			WARN_MWSA_LOG("secLogServiceOpenThread returned (%d), wait %d seconds then retry",Errt,(int)seconds);
			while(nanosleep(&sleepPeriod, &unusedPeriod) != 0);
		}
	} // end for

	syslog(LOG_USER*8 + LOG_INFO, "secLogService is successfully opened");
	secLogFileIsOpen = TRUE;
	pfd.events = POLLIN;
	pfd.fd = secSelectionObject;
	if(poll_setcallback(comSASThandle_ntf, sec_fdCallbackFn, pfd, NULL) != 0)
	{
		WARN_MWSA_LOG("poll_setcallback failed for security logs");
		Errt = MafFailure;
		goto end_exit;
	}

	secLogOpenThreadId  = 0;
end_exit:
	DEBUG_MWSA_LOG("Exiting secLogServiceOpenThread");
	pthread_exit(&Errt);
	LEAVE_MWSA_LOG();
	return NULL; // Just to get rid of a warning from the compiler
}

int createSymLinkForAlarmsAndAlerts(const char *path, const char *directory, const char * linkDir, const char * linkName)
{
	ENTER_MWSA_LOG();
	int ret = TRUE;
	if(path==NULL||directory==NULL||linkDir==NULL||linkName==NULL)
	{
		DEBUG_MWSA_LOG("createSymLinkForAlarmsAndAlerts() missing input parameter");
		LEAVE_MWSA_LOG();
		return FALSE;
	}
	DEBUG_MWSA_LOG("createSymLinkForAlarmsAndAlerts() path: (%s) directory: (%s) linkDir: (%s) linkName: (%s)",path, directory, linkDir, linkName);

	char directoryPath[500], linkPath[500];
	strcpy(directoryPath, path);
	strcat(directoryPath, "/");
	strcat(directoryPath, directory);
	strcpy(linkPath, linkDir);
	strcat(linkPath, "/");
	strcat(linkPath, linkName);
	#ifndef UNIT_TEST
	if ( symlink(directoryPath,linkPath) != 0 ) {
	    if ( errno != EEXIST ) {
	        DEBUG_MWSA_LOG("Error %s when creating symlink %s for %s", strerror(errno), linkPath, directoryPath);
	        return FALSE;
	    }
	}
	#endif
	LEAVE_MWSA_LOG();
	return ret;
}

static char logAlarmFileNameBuffer[50] = {0};
static char logAlertFileNameBuffer[50] = {0};
static char logCmdFileNameBuffer[50] = {0};
void parseLogFileName(char* configText, char* cmpText, char* type)
{
	ENTER_MWSA_LOG();
	if(!strncmp(cmpText, configText, strlen(cmpText)))
	{
		if(!strcmp(type,"alarm"))
		{
			int i = 0;
			for(; i < (strlen(configText) - strlen(cmpText)); i++)
			{
				logAlarmFileNameBuffer[i] = configText[i + strlen(cmpText)];
			}
			logAlarmFileNameBuffer[i] = '\0';
			alarmLogFileAttributes.logFileName = logAlarmFileNameBuffer;
		}
		else if(!strcmp(type,"alert"))
		{
			int i = 0;
			for(; i < (strlen(configText) - strlen(cmpText)); i++)
			{
				logAlertFileNameBuffer[i] = configText[i + strlen(cmpText)];
			}
			logAlertFileNameBuffer[i] = '\0';
			alertLogFileAttributes.logFileName = logAlertFileNameBuffer;
		}else if(!strcmp(type,"commandLog"))
		{
			int i = 0;
			for(; i < (strlen(configText) - strlen(cmpText)); i++)
			{
				logCmdFileNameBuffer[i] = configText[i + strlen(cmpText)];
			}
			logCmdFileNameBuffer[i] = '\0';
			cmdLogFileAttributes.logFileName = logCmdFileNameBuffer;
		}
	}
	LEAVE_MWSA_LOG();
}

void parseFilesRotated(char* configText, char* cmpText, char* type)
{
	ENTER_MWSA_LOG();
	if(!strncmp(cmpText, configText, strlen(cmpText)))
	{
		int maxLength = 50;
		char buffer[maxLength];
		int i = 0;
		for(; i < (strlen(configText) - strlen(cmpText)); i++)
		{
			if(isdigit(configText[i + strlen(cmpText)]))
			{
				buffer[i] = configText[i + strlen(cmpText)];
			}
			else
			{
				WARN_MWSA_LOG("illegal char in %s config file, breaking and using default values", type);
				// illegal character in config line, we just return here to skip the value update
				LEAVE_MWSA_LOG();
				return;
			}
		}
		buffer[i] = '\0';
		SaUint64T filesRotated = strtoull(buffer, NULL, 0);

		if(!strcmp(type,"alarm"))
		{
			alarmLogFileAttributes.maxFilesRotated = filesRotated;
		}
		else if(!strcmp(type,"alert"))
		{
			alertLogFileAttributes.maxFilesRotated = filesRotated;
		}
		else if(!strcmp(type,"commandLog"))
		{
			cmdLogFileAttributes.maxFilesRotated = filesRotated;
		}
	}
	LEAVE_MWSA_LOG();
}

void parseFileSize(char* configText, char* cmpText, char* type)
{
	ENTER_MWSA_LOG();
	if(!strncmp(cmpText, configText, strlen(cmpText)))
	{
		int maxLength = 50;
		char buffer[maxLength];
		int i = 0;
		for(; i < (strlen(configText) - strlen(cmpText)); i++)
		{
			if(isdigit(configText[i + strlen(cmpText)]))
			{
				buffer[i] = configText[i + strlen(cmpText)];
			}
			else
			{
				WARN_MWSA_LOG("illegal char in %s config file, breaking and using default values", type);
				// illegal character in config line, we just return here to skip the value update
				LEAVE_MWSA_LOG();
				return;
			}
		}
		buffer[i] = '\0';
		SaUint64T fileSize = strtoull(buffer, NULL, 0);

		if(!strcmp(type,"alarm"))
		{
			alarmLogFileAttributes.maxLogFileSize = fileSize;
		}
		else if(!strcmp(type,"alert"))
		{
			alertLogFileAttributes.maxLogFileSize = fileSize;
		}
		else if(!strcmp(type,"commandLog"))
		{
			cmdLogFileAttributes.maxLogFileSize = fileSize;
		}
	}
	LEAVE_MWSA_LOG();
}

void parseRecordSize(char* configText, char* cmpText, char* type)
{
	ENTER_MWSA_LOG();
	if(!strncmp(cmpText, configText, strlen(cmpText)))
	{
		int maxLength = 50;
		char buffer[maxLength];
		int i = 0;
		for(; i < (strlen(configText) - strlen(cmpText)); i++)
		{
			if(isdigit(configText[i + strlen(cmpText)]))
			{
				buffer[i] = configText[i + strlen(cmpText)];
			}
			else
			{
				WARN_MWSA_LOG("illegal char in %s config file, breaking and using default values", type);
				// illegal character in config line, we just return here to skip the value update
				LEAVE_MWSA_LOG();
				return;
			}
		}
		buffer[i] = '\0';
		SaUint64T recordSize = strtoull(buffer, NULL, 0);

		if(!strcmp(type,"alarm"))
		{
			alarmLogFileAttributes.maxLogRecordSize = recordSize;
		}
		else if(!strcmp(type,"alert"))
		{
			alertLogFileAttributes.maxLogRecordSize = recordSize;
		}
		else if(!strcmp(type,"commandLog"))
		{
			cmdLogFileAttributes.maxLogRecordSize = recordSize;
		}
	}
	LEAVE_MWSA_LOG();
}

/*
 * This function will parse alarm and alert log configuration data from the input string
 * and update the AlarmAndAlertConfig (which is a global struct)
 */
void parseSaLogConfig(char* configText)
{
	ENTER_MWSA_LOG();
	int maxLength = 50;
	if(strlen(configText) <= maxLength)
	{
		int i,j;
		char buffer[maxLength];
		// space filtering on the text
		for(i = 0, j = 0; i<strlen(configText); i++)
		{
			if(configText[i] != ' ' && configText[i] != '\n')
			{
				buffer[j] = configText[i];
				j++;
			}
		}
		buffer[j] = '\0';

		parseLogFileName(buffer, AlarmLogFileName, "alarm");
		parseLogFileName(buffer, AlertLogFileName, "alert");
		parseLogFileName(buffer, CmdLogFileName, "commandLog");

		parseFilesRotated(buffer, AlarmFilesRotated, "alarm");
		parseFilesRotated(buffer, AlertFilesRotated, "alert");
		parseFilesRotated(buffer, CmdFilesRotated, "commandLog");

		parseFileSize(buffer, AlarmMaxLogFileSize, "alarm");
		parseFileSize(buffer, AlertMaxLogFileSize, "alert");
		parseFileSize(buffer, CmdMaxLogFileSize, "commandLog");

		parseRecordSize(buffer, AlarmMaxLogRecordSize, "alarm");
		parseRecordSize(buffer, AlertMaxLogRecordSize, "alert");
		parseRecordSize(buffer, CmdMaxLogRecordSize, "commandLog");
	}
	LEAVE_MWSA_LOG();
}

/*
 * Loading configuration for log file attributes for alarms,alerts and COM commands by:
 *  1. setting the default values and
 *  2. trying to load the configuration from config file for alarms, alerts and COM command
 *     (This can overwrite the default values (1.) if data exists in config file)
 */
void loadSaLogConfig()
{
	ENTER_MWSA_LOG();
	// setting default values for log file attributes for alarms
	alarmLogFileAttributes.logFileName = DEFAULT_ALARM_LOG_FILE_NAME;
	alarmLogFileAttributes.logFilePathName = DEFAULT_ALARM_LOG_FILE_PATH_NAME;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alarmLogFileAttributes.maxLogRecordSize = DEFAULT_ALARM_MAX_LOG_REC_SIZE;
	alarmLogFileAttributes.haProperty = DEFAULT_ALARM_HA_PROPERTY;
	alarmLogFileAttributes.logFileFullAction = DEFAULT_ALARM_LOG_FILE_FULL_ACTION;
	alarmLogFileAttributes.maxFilesRotated = DEFAULT_ALARM_MAX_FILES_ROTATED;
	alarmLogFileAttributes.logFileFmt= DEFAULT_ALARM_FORMAT_EXPRESSION;

	// setting default values for log file attributes for alerts
	alertLogFileAttributes.logFileName = DEFAULT_ALERT_LOG_FILE_NAME;
	alertLogFileAttributes.logFilePathName = DEFAULT_ALERT_LOG_FILE_PATH_NAME;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogRecordSize = DEFAULT_ALERT_MAX_LOG_REC_SIZE;
	alertLogFileAttributes.haProperty = DEFAULT_ALERT_HA_PROPERTY;
	alertLogFileAttributes.logFileFullAction = DEFAULT_ALERT_LOG_FILE_FULL_ACTION;
	alertLogFileAttributes.maxFilesRotated = DEFAULT_ALERT_MAX_FILES_ROTATED;
	alertLogFileAttributes.logFileFmt= DEFAULT_ALERT_FORMAT_EXPRESSION;

	// setting default values for log file attributes for COM commands
	cmdLogFileAttributes.logFileName = DEFAULT_CMD_LOG_FILE_NAME;
	cmdLogFileAttributes.logFilePathName = DEFAULT_CMD_LOG_FILE_PATH_NAME;
	cmdLogFileAttributes.maxLogFileSize = DEFAULT_CMD_MAX_LOG_FILE_SIZE;
	cmdLogFileAttributes.maxLogRecordSize = DEFAULT_CMD_MAX_LOG_REC_SIZE;
	cmdLogFileAttributes.haProperty = DEFAULT_CMD_HA_PROPERTY;
	cmdLogFileAttributes.logFileFullAction = DEFAULT_CMD_LOG_FILE_FULL_ACTION;
	cmdLogFileAttributes.maxFilesRotated = DEFAULT_CMD_MAX_FILES_ROTATED;
	cmdLogFileAttributes.logFileFmt= DEFAULT_CMD_FORMAT_EXPRESSION;

	DEBUG_MWSA_LOG("Default logFileName for alarmLogFileAttributes: (%s)",alarmLogFileAttributes.logFileName);
	DEBUG_MWSA_LOG("Default logFileName for alertLogFileAttributes: (%s)",alertLogFileAttributes.logFileName);
	DEBUG_MWSA_LOG("Default logFileName for cmdLogFileAttributes: (%s)", cmdLogFileAttributes.logFileName);
	DEBUG_MWSA_LOG("Default maxFilesRotated for alarmLogFileAttributes: (%d)",alarmLogFileAttributes.maxFilesRotated);
	DEBUG_MWSA_LOG("Default maxFilesRotated for alertLogFileAttributes: (%d)",alertLogFileAttributes.maxFilesRotated);
	DEBUG_MWSA_LOG("Default maxFilesRotated for cmdLogFileAttributes: (%d)", cmdLogFileAttributes.maxFilesRotated);
	DEBUG_MWSA_LOG("Default maxLogFileSize for alarmLogFileAttributes: (%llu)",alarmLogFileAttributes.maxLogFileSize);
	DEBUG_MWSA_LOG("Default maxLogFileSize for alertLogFileAttributes: (%llu)",alertLogFileAttributes.maxLogFileSize);
	DEBUG_MWSA_LOG("Default maxLogFileSize for alertLogFileAttributes: (%llu)", cmdLogFileAttributes.maxLogFileSize);
	// Open config file and fetch data from there, then update config for alarms,alerts and COM commands
	FILE* cfg_fp = NULL;
	DEBUG_MWSA_LOG("Opening the config file for alarms, alerts and  COM commands");

	char strAlarmAndAlertCfgAbsPath[MAX_PATH_DATA_LENGTH];
	getClearStorage(strAlarmAndAlertCfgAbsPath);
	strcat(strAlarmAndAlertCfgAbsPath, "/");
	strcat(strAlarmAndAlertCfgAbsPath, COMSA_FOR_COREMW_DIR);
	strcat(strAlarmAndAlertCfgAbsPath, "/");

	#ifdef UNIT_TEST
		strcat(strAlarmAndAlertCfgAbsPath, "com");
	#else
		strcat(strAlarmAndAlertCfgAbsPath, _CC_NAME);
	#endif

	strcat(strAlarmAndAlertCfgAbsPath, "_sa_log.cfg");

	if ((cfg_fp = fopen(strAlarmAndAlertCfgAbsPath,"r")) != NULL)
	{
		char buf[MAX_PATH_DATA_LENGTH];
		for (;;)
		{
			if (fgets(buf,sizeof(buf),cfg_fp) == NULL)
			{
				break;
			}
			// This will update (if sa log config found) the parseSaLogConfig (which is a global struct)
			parseSaLogConfig(buf);
		}
		fclose(cfg_fp);
	}
	else
	{
		WARN_MWSA_LOG("There is no config file for alarms, alerts and COM Commands: [%s]", strAlarmAndAlertCfgAbsPath);
	}
	DEBUG_MWSA_LOG("After loading config file - logFileName for alarmLogFileAttributes: (%s)",alarmLogFileAttributes.logFileName);
	DEBUG_MWSA_LOG("After loading config file - logFileName for alertLogFileAttributes: (%s)",alertLogFileAttributes.logFileName);
	DEBUG_MWSA_LOG("After loading config file - logFileName for cmdLogFileAttributes: (%s)", cmdLogFileAttributes.logFileName);
	DEBUG_MWSA_LOG("After loading config file - maxFilesRotated for alarmLogFileAttributes: (%d)",alarmLogFileAttributes.maxFilesRotated);
	DEBUG_MWSA_LOG("After loading config file - maxFilesRotated for alertLogFileAttributes: (%d)",alertLogFileAttributes.maxFilesRotated);
	DEBUG_MWSA_LOG("After loading config file - maxFilesRotated for cmdLogFileAttributes: (%d)", cmdLogFileAttributes.maxFilesRotated);
	DEBUG_MWSA_LOG("After loading config file - maxLogFileSize for alarmLogFileAttributes: (%llu)",alarmLogFileAttributes.maxLogFileSize);
	DEBUG_MWSA_LOG("After loading config file - maxLogFileSize for alertLogFileAttributes: (%llu)",alertLogFileAttributes.maxLogFileSize);
	DEBUG_MWSA_LOG("After loading config file - maxLogFileSize for cmdLogFileAttributes: (%llu)",cmdLogFileAttributes.maxLogFileSize);
	LEAVE_MWSA_LOG();
	return;
}


static int hasWhiteSpace(char* str)
{
	if(!str)
	{
		return FALSE;
	}
	size_t len = strlen(str);
	int i = 0;
	for(; i < len; i++)
	{
		if ((str[i] == ' ') || (str[i] == '\t'))
		{
			return TRUE;
		}
	}
	return FALSE;
}

static int trimWhiteSpace(char* str)
{
	if(!str)
	{
		return FALSE;
	}
	unsigned int pos = 0;
	size_t len = strlen(str);
	if(len == 0)
	{
		// Empty string
		return TRUE;
	}
	// find the first letter position
	while(pos < len)
	{
		if ( (str[pos] != ' ') && (str[pos] != '\t') && (str[pos] != '\n'))
		{
			break;
		}
		pos++;
	}
	if(pos == len)
	{
		// empty string
		str[0] = 0;
		return TRUE;
	}
	if(pos)
	{
		// trim the leading white spaces
		memmove(str, str + pos, len - pos + 1);
		// new length
		len = strlen(str);
	}
	// find the last letter position
	pos = len - 1;
	while(pos > 0)
	{
		if ( (str[pos] != ' ') && (str[pos] != '\t') && (str[pos] != '\n'))
		{
			break;
		}
		pos--;
	}
	// No need to trim
	if(pos == (len - 1))
	{
		return TRUE;
	}
	if(pos == 0)
	{
		if ( (str[pos] != ' ') && (str[pos] != '\t') && (str[pos] != '\n'))
		{
			str[pos + 1] = 0;
		}
		else
		{
			str[pos] = 0;
		}
		return TRUE;
	}
	// trim the trailing white spaces
	str[pos + 1] = 0;
	return TRUE;
}

static void getIntRootDir(char* rootDir) {
	FILE *fp = popen(COMEA_INT_FILE_SYSTEM_COMMAND, "r");
	if(fp)
	{
	    char readStr[MAX_LINE_BUFF] = {0};
	    // Reading the file config
	    if(fgets(readStr, MAX_LINE_BUFF, fp)!=NULL)
	    {
	        trimWhiteSpace(readStr);
		    size_t len = strlen(readStr);
		    // if readStr is not empty
		    if(len)
		    {
		        DEBUG_MWSA_LOG("getIntRootDir(): str=%s", readStr);
				if(hasWhiteSpace(readStr))
				{
			        WARN_MWSA_LOG("getIntRootDir(): there are whitespace(s) inside path!!!");
			    }
				strcpy(rootDir,readStr);
				pclose(fp);
				return;
			}
			else
			{
			    WARN_MWSA_LOG("getIntRootDir(): failed due to empty path from comea spi call ");
			    pclose(fp);
			}
		}
	    else
		{
			WARN_MWSA_LOG("getIntRootDir(): failed due to failure of fgets ");
			pclose(fp);
		}

	}
	else
	{
        // COMEA SPI call command execution error!
		WARN_MWSA_LOG("getIntRootDir(): failed, %s command execution error %d",COMEA_INT_FILE_SYSTEM_COMMAND,errno);
	}
	DEBUG_MWSA_LOG("could not get internal_root path from COMEA SPI, taking it as default(/var/filem/internal_root)");
        rootDir = DEFAULT_INT_FILEM_PATH;
	return;
}

SaAisErrorT modifyLogStreamAttr(SaImmCcbHandleT* ccbHandle, const char* object, SaLogFileCreateAttributesT_2* logFileAttributes, bool updateFileSizeAttr){

	SaAisErrorT rc = SA_AIS_OK;
	const SaImmAttrModificationT_2* attMods[5];
	attMods[0] = allocateUint64AttrMod((const char*)LOG_STREAM_MAX_FILE_SIZE_PARAM, logFileAttributes->maxLogFileSize);
	attMods[1] = allocateUint32AttrMod((const char*)LOG_STREAM_MAX_FILE_ROTATED_PARAM, logFileAttributes->maxFilesRotated);
	attMods[2] = allocateUint32AttrMod((const char*)LOG_STREAM_FIXED_LOG_REC_SIZE_PARAM, logFileAttributes->maxLogRecordSize);
	if(updateFileSizeAttr){
		attMods[3] = allocateStringAttrMod((const char*)LOG_STREAM_FILENAME_PARAM, logFileAttributes->logFileName);
	}else{
		attMods[3] = NULL;
	}
	attMods[4] = NULL;
	SaNameT* immObject = makeSaNameT(object);
	IMM_API_CALL(saImmOmCcbObjectModify_2(*ccbHandle,
			immObject,
			(const SaImmAttrModificationT_2**)attMods));
	unsigned int i;
	for(i = 0; attMods[i] != NULL; i++)
	{
		freeAttrMod((SaImmAttrModificationT_2 *)attMods[i]);
	}
	free(immObject);
	return rc;
}

void log_imm_finalize(SaImmHandleT immOmHandle, SaImmAdminOwnerHandleT adminOwnerHandle){
	SaAisErrorT rc = SA_AIS_OK;
	if(adminOwnerHandle != (SaImmAdminOwnerHandleT)0)
	{
		IMM_API_CALL(saImmOmAdminOwnerFinalize(adminOwnerHandle));
		if(rc != SA_AIS_OK)
		{
			ERR_IMM_OM("log_imm_finalize(): saImmOmAdminOwnerFinalize FAILED: %d", rc);
		}
		else
		{
			DEBUG_IMM_OM("log_imm_finalize(): Successfully finalized the adminOwnerHandle");
		}
	}
	if(immOmHandle != (SaImmHandleT)0)
	{
		IMM_API_CALL(saImmOmFinalize(immOmHandle));
		if( rc!= SA_AIS_OK)
		{
			ERR_IMM_OM("log_imm_finalize(): saImmOmFinalize FAILED: %d", rc);
		}
		else
		{
			DEBUG_IMM_OM("log_imm_finalize(): Successfully finalized the immOmHandle");
		}
	}
}

MafReturnT modifyCfgLogStreamInstances()
{
	ENTER_MWSA_LOG();

	SaAisErrorT rc = SA_AIS_OK;
	SaImmHandleT logImmOmHandle = 0;
	SaImmAdminOwnerHandleT logAdminOwnerHandle = 0;
	SaImmCcbHandleT logCcbHandle = 0;
	if(autoRetry_saImmOmInitialize(&logImmOmHandle, NULL, &imm_version) != SA_AIS_OK){
		ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmInitialize failed %d", rc);
		return MafFailure;
	}
	IMM_API_CALL(saImmOmAdminOwnerInitialize(logImmOmHandle,
		                                          (const SaImmAdminOwnerNameT)logAdminOwnerName,
		                                           SA_TRUE,
		                                           &logAdminOwnerHandle));
	if(rc != SA_AIS_OK) {
		ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmAdminOwnerInitialize failed %d", rc);
		log_imm_finalize(logImmOmHandle, (SaImmAdminOwnerHandleT)0);
		return MafFailure;
	}

	SaNameT* objDns[4];
	objDns[0] = (SaNameT*)malloc(sizeof(SaNameT));
	saNameSet((const char*)ALARM_LOG_STREAM_DN, objDns[0]);
	objDns[1] = (SaNameT*)malloc(sizeof(SaNameT));
	saNameSet((const char*)ALERT_LOG_STREAM_DN, objDns[1]);
	objDns[2] = (SaNameT*)malloc(sizeof(SaNameT));
	saNameSet((const char*)CMD_LOG_STREAM_DN, objDns[2]);
	objDns[3] = NULL;

	IMM_API_CALL(saImmOmAdminOwnerSet(logAdminOwnerHandle, (const SaNameT**)objDns, SA_IMM_ONE));
	if(rc != SA_AIS_OK) {
		ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmAdminOwnerSet failed %d", rc);
		free(objDns[0]);
		free(objDns[1]);
		free(objDns[2]);
		log_imm_finalize(logImmOmHandle,logAdminOwnerHandle);
		return MafFailure;
	}
	IMM_API_CALL(saImmOmCcbInitialize(logAdminOwnerHandle, SA_IMM_CCB_REGISTERED_OI | SA_IMM_CCB_ALLOW_NULL_OI, &logCcbHandle));
	if(rc != SA_AIS_OK) {
		ERR_MWSA_LOG("modifyCfgLogStreamInstances(): logm_saImmOmCcbInitialize failed %d", rc);
		IMM_API_CALL(saImmOmAdminOwnerRelease(logAdminOwnerHandle,
				(const SaNameT**)objDns,
				SA_IMM_ONE));
		if(rc != SA_AIS_OK){
			ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmAdminOwnerRelease failed %d", rc);
		}
		free(objDns[0]);
		free(objDns[1]);
		free(objDns[2]);
		log_imm_finalize(logImmOmHandle,logAdminOwnerHandle);
		return MafFailure;
	}
	rc = modifyLogStreamAttr( &logCcbHandle, (const char*)ALARM_LOG_STREAM_DN, &alarmLogFileAttributes, updateAlarmLogFileNameAttr);

	if(rc == SA_AIS_OK){
		rc = modifyLogStreamAttr(&logCcbHandle, (const char*)ALERT_LOG_STREAM_DN, &alertLogFileAttributes, updateAlertLogFileNameAttr);
	}
	if(rc == SA_AIS_OK){
		rc = modifyLogStreamAttr(&logCcbHandle, (const char*)CMD_LOG_STREAM_DN, &cmdLogFileAttributes, updateCmdLogFileNameAttr);
	}

	if(rc != SA_AIS_OK ) {
		ERR_MWSA_LOG("modifyCfgLogStreamInstances(): modifyLogStreamAttr failed %d", rc);
		IMM_API_CALL(saImmOmAdminOwnerRelease(logAdminOwnerHandle,
				(const SaNameT**)objDns,
				SA_IMM_ONE));
		if(rc != SA_AIS_OK){
			ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmAdminOwnerRelease failed %d", rc);
		}
		free(objDns[0]);
		free(objDns[1]);
		free(objDns[2]);
		log_imm_finalize(logImmOmHandle,logAdminOwnerHandle);
		return MafFailure;
	}

	IMM_API_CALL(saImmOmCcbValidate(logCcbHandle));
	if(rc != SA_AIS_OK) {
		ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmCcbValidate failed %d", rc);
		IMM_API_CALL(saImmOmAdminOwnerRelease(logAdminOwnerHandle,
				(const SaNameT**)objDns,
				SA_IMM_ONE));
		if(rc != SA_AIS_OK){
			ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmAdminOwnerRelease failed %d", rc);
		}
		free(objDns[0]);
		free(objDns[1]);
		free(objDns[2]);
		log_imm_finalize(logImmOmHandle,logAdminOwnerHandle);
		return MafFailure;
	}

	IMM_API_CALL(saImmOmCcbApply(logCcbHandle));
	if(rc != SA_AIS_OK) {
		ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmCcbApply failed %d", rc);
		IMM_API_CALL(saImmOmAdminOwnerRelease(logAdminOwnerHandle,
				(const SaNameT**)objDns,
				SA_IMM_ONE));
		if(rc != SA_AIS_OK){
			ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmAdminOwnerRelease failed %d", rc);
		}
		free(objDns[0]);
		free(objDns[1]);
		free(objDns[2]);
		log_imm_finalize(logImmOmHandle,logAdminOwnerHandle);
		return MafFailure;
	}

	IMM_API_CALL(saImmOmAdminOwnerRelease(logAdminOwnerHandle,
			(const SaNameT**)objDns,
			SA_IMM_ONE));
	if(rc != SA_AIS_OK){
		ERR_MWSA_LOG("modifyCfgLogStreamInstances(): saImmOmAdminOwnerRelease failed %d", rc);
	}
	free(objDns[0]);
	free(objDns[1]);
	free(objDns[2]);
	log_imm_finalize(logImmOmHandle,logAdminOwnerHandle);
	LEAVE_MWSA_LOG();
	return MafOk;
}

SaAisErrorT getSeverityFilter(SaImmAccessorHandleT accessorHandle, char *dnName, SaUint16T* severityFilter){
	SaAisErrorT ret = SA_AIS_OK;
	if(accessorHandle != 0){
		SaImmAttrNameT attributeName[2];
		attributeName[0] = (SaStringT)LOGM_LOG_SEVERITYFILTER;
		attributeName[1] = '\0';
		SaImmAttrValuesT_2** attributeValue;
		SaNameT dn;
		saNameSet(dnName, &dn);
		if((ret = autoRetry_saImmOmAccessorGet_2(accessorHandle, &dn, attributeName, &attributeValue)) != SA_AIS_OK) {
			WARN_MWSA_LOG("getSeverityFilter(): autoRetry_saImmOmAccessorGet_2 failed %s", getOpenSAFErrorString(ret));
		}
		else{
			DEBUG_IMM_OM("getSeverityFilter(): dn = %s",dnName);
			SaImmAttrValuesT_2* oneAttrValue = attributeValue[0];
			if (strcmp(oneAttrValue->attrName, (char*)LOGM_LOG_SEVERITYFILTER)){
				WARN_MWSA_LOG("getSeverityFilter(): Expected attributeName returned by autoRetry_saImmOmAccessorGet_2() to be %s. Received: %s", (char*)LOGM_LOG_SEVERITYFILTER, oneAttrValue->attrName);
				ret = SA_AIS_ERR_INVALID_PARAM;
			}
			else{
				if (oneAttrValue->attrValueType != SA_IMM_ATTR_SAINT32T){
					WARN_MWSA_LOG("getSeverityFilter(): Expected attrValueType returned by autoRetry_saImmOmAccessorGet_2() to be %d. Received: %d", SA_IMM_ATTR_SAINT32T, oneAttrValue->attrValueType);
					ret = SA_AIS_ERR_INVALID_PARAM;
				}
				else{
					int i = 0;
					SaUint16T tmpSevFilter = 0;
					for(; i < oneAttrValue->attrValuesNumber; i++){
						tmpSevFilter = tmpSevFilter | SEVERITY_FLAG(*((SaInt32T *)oneAttrValue->attrValues[i]));
					}
					*severityFilter = tmpSevFilter;
					logMEnabled = true;
					DEBUG_MWSA_LOG("getSeverityFilter(): dn = %s,value: %d.", dnName, *severityFilter);
				}
			}
		}
	}else{
		DEBUG_MWSA_LOG("getSeverityFilter(): accessorHandle is zero");
		ret = SA_AIS_ERR_BAD_HANDLE;
	}
	return ret;
}

SaAisErrorT getLogStreamFileName( SaImmAccessorHandleT accessorHandle, char *dnName, char** fileName){
	SaAisErrorT ret = SA_AIS_OK;
	if(accessorHandle != 0){
		SaImmAttrNameT attributeName[2];
		attributeName[0] = (SaStringT)LOG_STREAM_FILENAME_PARAM;
		attributeName[1] = '\0';
		SaImmAttrValuesT_2** attributeValue;
		SaNameT dn;
		saNameSet(dnName, &dn);
		if((ret = autoRetry_saImmOmAccessorGet_2(accessorHandle, &dn, attributeName, &attributeValue)) != SA_AIS_OK) {
			WARN_MWSA_LOG("getLogStreamFileName(): autoRetry_saImmOmAccessorGet_2 failed %s", getOpenSAFErrorString(ret));
		}
		else{
			SaImmAttrValuesT_2* oneAttrValue = attributeValue[0];
			if (strcmp(oneAttrValue->attrName, (char*)LOG_STREAM_FILENAME_PARAM)){
				WARN_MWSA_LOG("getLogStreamFileName(): Expected attributeName returned by autoRetry_saImmOmAccessorGet_2() to be %s. Received: %s", (char*)LOG_STREAM_FILENAME_PARAM, oneAttrValue->attrName);
				ret = SA_AIS_ERR_INVALID_PARAM;
			}
			else{
				if (oneAttrValue->attrValueType != SA_IMM_ATTR_SASTRINGT){
					WARN_MWSA_LOG("getLogStreamFileName(): Expected attrValueType returned by autoRetry_saImmOmAccessorGet_2() to be %d. Received: %d", SA_IMM_ATTR_SASTRINGT, oneAttrValue->attrValueType);
					ret = SA_AIS_ERR_INVALID_PARAM;
				}
				else{
					DEBUG_MWSA_LOG("getLogStreamFileName() found attribute,value: %s.", *(SaStringT *)oneAttrValue->attrValues[0]);
					*fileName = strdup(*(SaStringT *)oneAttrValue->attrValues[0]);
				}
			}
		}
	}else{
		DEBUG_MWSA_LOG("getLogStreamFileName(): accessorHandle is zero");
		ret = SA_AIS_ERR_BAD_HANDLE;
	}
	return ret;
}

bool updateLogStreamFileNameNeeded(SaImmAccessorHandleT accessorHandle, SaStringT logStreamDn , SaLogFileCreateAttributesT_2* logFileAttributes ){
	SaAisErrorT error = SA_AIS_OK;
	char* logFileName = NULL;
	bool updateFileName = false;
	error= getLogStreamFileName(accessorHandle,logStreamDn, &logFileName);
	if(error != SA_AIS_OK){
		DEBUG_MWSA_LOG("readCfgLogStream(): get saLogStreamFileName from log stream instance is failed, ret = %d", error);
	}else{
		DEBUG_MWSA_LOG("readCfgLogStream() :  log file name = %s",logFileName);
		if(strcmp(logFileName,logFileAttributes->logFileName) != 0){
			updateFileName = true;
		}
		if(logFileName != NULL){
			free(logFileName);
		}
	}
	return updateFileName;
}
void readCfgLogStreamsInfoFromImm(){
	SaImmHandleT immOmHandle = 0;
	DEBUG_MWSA_LOG("readCfgLogStreamsInfoFromImm");
	SaImmAccessorHandleT accessorHandle = 0;
	SaAisErrorT error = SA_AIS_OK;
	if((error = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &imm_version)) != SA_AIS_OK) {
		WARN_MWSA_LOG("readCfgLogStreamsInfoFromImm(): saImmOmInitialize failed %s", getOpenSAFErrorString(error));
	}
	else {
		if((error = autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandle)) != SA_AIS_OK) {
			WARN_MWSA_LOG("readCfgLogStreamsInfoFromImm(): saImmOmAccessorInitialize failed %s", getOpenSAFErrorString(error));
		}else {
			updateAlarmLogFileNameAttr = updateLogStreamFileNameNeeded(accessorHandle, (SaStringT)ALARM_LOG_STREAM_DN, &alarmLogFileAttributes);
			updateAlertLogFileNameAttr = updateLogStreamFileNameNeeded(accessorHandle, (SaStringT)ALERT_LOG_STREAM_DN, &alertLogFileAttributes);
			updateCmdLogFileNameAttr = updateLogStreamFileNameNeeded(accessorHandle, (SaStringT)CMD_LOG_STREAM_DN, &cmdLogFileAttributes);

			error = getSeverityFilter( accessorHandle,(SaStringT)ALARM_LOGM_DN, &alarmLogSevFlags);
			if(error != SA_AIS_OK){
				DEBUG_MWSA_LOG("readCfgLogStreamsInfoFromImm(): get severityFilter from alarm log instance is failed, ret = %d", error);
			}
			error = getSeverityFilter( accessorHandle,(SaStringT)ALERT_LOGM_DN, &alertLogSevFlags);
			if(error != SA_AIS_OK){
				DEBUG_MWSA_LOG("readCfgLogStreamsInfoFromImm(): get severityFilter from alert log instance is failed, ret = %d", error);
			}
			error = getSeverityFilter( accessorHandle,(SaStringT)CMD_LOGM_DN, &cmdLogSevFlags);
			if(error != SA_AIS_OK){
				DEBUG_MWSA_LOG("readCfgLogStreamsInfoFromImm(): get severityFilter from command log instance is failed, ret = %d", error);
			}
			error = getSeverityFilter( accessorHandle,(SaStringT)SECURITY_LOGM_DN, &secLogSevFlags);
			if(error != SA_AIS_OK){
				DEBUG_MWSA_LOG("readCfgLogStreamsInfoFromImm(): get severityFilter from security log instance is failed, ret = %d", error);
			}
		}
		om_imm_finalize(immOmHandle, accessorHandle);
	}
	return;
}

SaAisErrorT readSafLogPath(char** attrVal)
{
	char * dnName = SAF_LOG_CONFIG;
	char * attrName = SAF_LOG_PATH_ATTR_NAME;
	SaImmHandleT immOmHandle = 0;
	DEBUG_IMM_OM("readSafLogPath() dn = %s, attrName = %s", dnName, attrName);
	SaImmAccessorHandleT accessorHandle = 0;
	SaImmAttrValuesT_2** attributeValue;
	SaImmAttrNameT attributeName[2];
	SaAisErrorT error = SA_AIS_OK;
	SaNameT dn;
	attributeName[0] = attrName;
	attributeName[1] = '\0';

	saNameSet(dnName, &dn);

	if((error = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &imm_version)) != SA_AIS_OK) {
		WARN_MWSA_LOG("readSafLogPath(): saImmOmInitialize failed %s", getOpenSAFErrorString(error));
	}
	else {
		if((error = autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandle)) != SA_AIS_OK) {
			WARN_MWSA_LOG("readSafLogPath(): saImmOmAccessorInitialize failed %s", getOpenSAFErrorString(error));
		}
		else {
			if((error = autoRetry_saImmOmAccessorGet_2(accessorHandle, &dn, attributeName, &attributeValue)) != SA_AIS_OK) {
				WARN_MWSA_LOG("readSafLogPath(): autoRetry_saImmOmAccessorGet_2 failed %s", getOpenSAFErrorString(error));
			}
			else{
				SaImmAttrValuesT_2* oneAttrValue = attributeValue[0];
				if (strcmp(oneAttrValue->attrName, attrName)){
					WARN_MWSA_LOG("readSafLogPath(): Expected attributeName returned by autoRetry_saImmOmAccessorGet_2() to be %s. Received: %s", attrName, oneAttrValue->attrName);
					error = SA_AIS_ERR_INVALID_PARAM;
				}
				else{
					if (oneAttrValue->attrValueType != SA_IMM_ATTR_SASTRINGT){
						WARN_MWSA_LOG("readSafLogPath(): Expected attrValueType returned by autoRetry_saImmOmAccessorGet_2() to be %d. Received: %d", SA_IMM_ATTR_SASTRINGT, oneAttrValue->attrValueType);
						error = SA_AIS_ERR_INVALID_PARAM;
					}
					else{
						DEBUG_MWSA_LOG("readSafLogPath() found attribute,value: %s.", *(SaStringT *)oneAttrValue->attrValues[0]);
						*attrVal = strdup(*(SaStringT *)oneAttrValue->attrValues[0]);
					}
				}
			}
		}
		om_imm_finalize(immOmHandle, accessorHandle);
	}
	return error;
}

MafReturnT AlarmAndAlertLogServiceOpen(void)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("AlarmAndAlertLogServiceOpen ENTER");

	char rootDir[MAX_LINE_BUFF] = {0};
	SaVersionT 		theVersion = {logReleaseCode, logMajorVersion, logMinorVersion};
	SaLogCallbacksT	alarmCallBacks = {alarmLogFilterSetCallbackT, NULL, alarmLogWriteLogCallbackT};
	SaLogCallbacksT	alertCallBacks = {alertLogFilterSetCallbackT, NULL, alertLogWriteLogCallbackT};
	SaAisErrorT 	retVal = SA_AIS_OK;
	MafReturnT      mafRetVal = MafOk;
	int				PthreadResult = 0;
	char * safLogPath = NULL;
	const char * alarmLinkName = "AlarmLogs";
	const char * alertLinkName = "AlertLogs";
	retVal = readSafLogPath(&safLogPath);
	if (retVal != SA_AIS_OK)
	{
		ERR_MWSA_LOG("AlarmAndAlertLogServiceOpen() : Retrieve path to opensaf logs failed.");
		mafRetVal = MafFailure;
		goto end_exit;
	}
	DEBUG_MWSA_LOG("AlarmAndAlertLogServiceOpen(): filePath returned by readSafLogPath():  %s", safLogPath);

	//function call to get internalRoot path
	getIntRootDir(rootDir);
	if(!createSymLinkForAlarmsAndAlerts((const char *)safLogPath, DEFAULT_ALARM_LOG_FILE_PATH_NAME, rootDir, alarmLinkName))
	{
		 WARN_MWSA_LOG("creating symlink for alarms failed with link directory is %s",rootDir);
	}
	if(!createSymLinkForAlarmsAndAlerts((const char *)safLogPath, DEFAULT_ALERT_LOG_FILE_PATH_NAME, rootDir, alertLinkName))
	{
		 WARN_MWSA_LOG("creating symlink for alerts failed with link directory is %s",rootDir);
	}

	/* Set up the logger name */
	saNameSet(IdentityString, &alarmLoggerName);
	saNameSet(IdentityString, &alertLoggerName);
	SaAisErrorT 	Errt;
	// Initialize saLog for alarms
	LOG_API_CALL(saLogInitialize(&alarmLogHandle ,&alarmCallBacks , &theVersion));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogInitialize for alarms error = %d", Errt);
		DEBUG_MWSA_LOG("Version releasecode %d major %d minor %d", theVersion.releaseCode,theVersion.majorVersion,theVersion.minorVersion);
		mafRetVal = MafFailure;
		goto end_exit;
	}
	DEBUG_MWSA_LOG("alarmLogHandle (%llu)",alarmLogHandle);

	LOG_API_CALL(saLogSelectionObjectGet(alarmLogHandle ,&alarmSelectionObject));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogSelectionObjectGet for alarms error = %d", Errt);
		mafRetVal = MafFailure;
		goto end_exit;
	}

	// Initialize saLog for alerts
	LOG_API_CALL(saLogInitialize(&alertLogHandle ,&alertCallBacks , &theVersion));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogInitialize for alerts error = %d", Errt);
		DEBUG_MWSA_LOG("Version releasecode %d major %d minor %d", theVersion.releaseCode,theVersion.majorVersion,theVersion.minorVersion);
		mafRetVal = MafFailure;
		goto end_exit;
	}
	DEBUG_MWSA_LOG("alertLogHandle (%llu)",alertLogHandle);

	LOG_API_CALL(saLogSelectionObjectGet(alertLogHandle ,&alertSelectionObject));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogSelectionObjectGet for alerts error = %d", Errt);
		mafRetVal = MafFailure;
		goto end_exit;
	}

	// Loading configs for log file attributes for alarms and alerts
	loadSaLogConfig();

	//Read log file Name from alarm/alert/command log instances
	// read severity  from the alarm/alert/command log stream  log instance
	readCfgLogStreamsInfoFromImm();
	//update alarm/alert/command log stream instances with user configured values
	modifyCfgLogStreamInstances();

	DEBUG_MWSA_LOG("AlarmAndAlertLogServiceOpen Calling pthread_create alarmLogServiceOpenThread");
	if ((PthreadResult = pthread_create(&alarmOpenThreadId,NULL, &alarmLogServiceOpenThread, NULL)) != 0)
	{
		DEBUG_MWSA_LOG("AlarmAndAlertLogServiceOpen pthread_create failed alarmLogServiceOpenThread PthreadResult = %d",PthreadResult);
		syslog(LOG_USER*8 + LOG_ERR, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to create alarmLogServiceOpenThread ",PthreadResult);
		goto end_exit;
	}
	DEBUG_MWSA_LOG("AlarmAndAlertLogServiceOpen() pthread_create(%d)", (int) alarmOpenThreadId);
	DEBUG_MWSA_LOG("AlarmAndAlertLogServiceOpen Calling pthread_create alertLogServiceOpenThread");
	if ((PthreadResult = pthread_create(&alertOpenThreadId,NULL, &alertLogServiceOpenThread, NULL)) != 0)
	{
	  DEBUG_MWSA_LOG("AlarmAndAlertLogServiceOpen pthread_create failed alertLogServiceOpenThread PthreadResult = %d, alertOpenThreadId = %d", PthreadResult, (int) alertOpenThreadId);
		syslog(LOG_USER*8 + LOG_ERR, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to create alertLogServiceOpenThread ",PthreadResult);
		goto end_exit;
	}
end_exit:
	// free allocated memory
	if(safLogPath)
	{
		free(safLogPath);
	}

	DEBUG_MWSA_LOG("AlarmAndAlertLogServiceOpen LEAVE_MWSA_LOG (%d)",mafRetVal);
	LEAVE_MWSA_LOG();
	return mafRetVal;
}

MafReturnT cmdLogServiceOpen(void)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("cmdLogServiceOpen ENTER");

	SaVersionT 		theVersion = {logReleaseCode, logMajorVersion, logMinorVersion};
	SaLogCallbacksT	cmdLogCallBacks = {cmdLogFilterSetCallbackT, NULL, cmdLogWriteLogCallbackT};
	MafReturnT      mafRetVal = MafOk;
	int				PthreadResult = 0;

	asprintf(&cmdLogString, "%s", "COM CLI");
	/* Set up the logger name */
	saNameSet(cmdLogString, &cmdLoggerName);
	SaAisErrorT 	Errt;
	// Initialize saLog for alarms
	LOG_API_CALL(saLogInitialize(&cmdLogHandle ,&cmdLogCallBacks , &theVersion));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogInitialize for alarms error = %d", Errt);
		DEBUG_MWSA_LOG("Version releasecode %d major %d minor %d", theVersion.releaseCode,theVersion.majorVersion,theVersion.minorVersion);
		mafRetVal = MafFailure;
		goto end_exit;
	}
	DEBUG_MWSA_LOG("cmdLogHandle (%llu)", cmdLogHandle);

	LOG_API_CALL(saLogSelectionObjectGet(cmdLogHandle ,&cmdSelectionObject));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogSelectionObjectGet for alarms error = %d", Errt);
		mafRetVal = MafFailure;
		goto end_exit;
	}

	DEBUG_MWSA_LOG("cmdLogServiceOpen Calling pthread_create cmdLogServiceOpenThread");
	if ((PthreadResult = pthread_create(&cmdLogOpenThreadId,NULL, &cmdLogServiceOpenThread, NULL)) != 0)
	{
		DEBUG_MWSA_LOG("cmdLogServiceOpen pthread_create failed cmdLogServiceOpenThread PthreadResult = %d",PthreadResult);
		syslog(LOG_USER*8 + LOG_ERR, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to create cmdLogServiceOpenThread ",PthreadResult);
		goto end_exit;
	}
	DEBUG_MWSA_LOG("cmdLogServiceOpen() pthread_create(%d)", (int) cmdLogOpenThreadId);

end_exit:
	DEBUG_MWSA_LOG("cmdLogServiceOpen LEAVE_MWSA_LOG (%d)",mafRetVal);
	LEAVE_MWSA_LOG();
	return mafRetVal;
}

MafReturnT secLogServiceOpen(void)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("secLogServiceOpen ENTER");

	SaVersionT 		theVersion = {logReleaseCode, logMajorVersion, logMinorVersion};
	SaLogCallbacksT	secLogCallBacks = {secLogFilterSetCallbackT, NULL, secLogWriteLogCallbackT};
	MafReturnT      mafRetVal = MafOk;
	int				PthreadResult = 0;

	saNameSet(IdentityString, &secLoggerName);
	SaAisErrorT 	Errt;
	// Initialize saLog for security logs
	LOG_API_CALL(saLogInitialize(&secLogHandle ,&secLogCallBacks , &theVersion));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogInitialize for security logs error = %d", Errt);
		DEBUG_MWSA_LOG("Version releasecode %d major %d minor %d", theVersion.releaseCode,theVersion.majorVersion,theVersion.minorVersion);
		mafRetVal = MafFailure;
		goto end_exit;
	}

	DEBUG_MWSA_LOG("secLogHandle (%llu)", secLogHandle);
	LOG_API_CALL(saLogSelectionObjectGet(secLogHandle ,&secSelectionObject));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogSelectionObjectGet for security logs error = %d", Errt);
		mafRetVal = MafFailure;
		goto end_exit;
	}

	DEBUG_MWSA_LOG("secLogServiceOpen Calling pthread_create secLogServiceOpenThread");
	if ((PthreadResult = pthread_create(&secLogOpenThreadId,NULL, &secLogServiceOpenThread, NULL)) != 0)
	{
		DEBUG_MWSA_LOG("secLogServiceOpen pthread_create failed secLogServiceOpenThread PthreadResult = %d",PthreadResult);
		syslog(LOG_USER*8 + LOG_ERR, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to create secLogServiceOpenThread ",PthreadResult);
		goto end_exit;
	}
	DEBUG_MWSA_LOG("secLogServiceOpen() pthread_create(%d)", (int) secLogOpenThreadId);

end_exit:
	DEBUG_MWSA_LOG("secLogServiceOpen LEAVE_MWSA_LOG (%d)",mafRetVal);
	LEAVE_MWSA_LOG();
	return mafRetVal;
}

MafReturnT AlarmAndAlertLogServiceClose(void)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("AlarmAndAlertLogServiceClose ENTER");
	SaAisErrorT Errt = SA_AIS_OK;
	MafReturnT	myRetVal = MafOk;
	int status;
	// Closing alarmOpenThread if still open
	if (alarmOpenThreadId != 0)
	{
	  DEBUG_MWSA_LOG("AlarmAndAlertLogServiceClose() alarmOpenThreadId = %d", (int) alarmOpenThreadId);
		status = pthread_kill(alarmOpenThreadId, SIGKILL);
		if(status < 0)
		{
			LOG_MWSA_LOG("pthread_kill failed for alearmOpenThread");
		}
		else
		{

			LOG_MWSA_LOG("pthread_kill successful for alarmOpenThread");
		}

		status = pthread_join(alarmOpenThreadId, NULL);
		if(status < 0)
		{
			LOG_MWSA_LOG("pthread_join failed for alarmOpenThread");
		}
		else
		{
			LOG_MWSA_LOG("pthread_join successful for alarmOpenThread");
		}
	}
	poll_unsetcallback(comSASThandle_ntf, alarmSelectionObject);

	// Closing alarmLogStream
	alarmLogFileIsOpen = FALSE;
	if(alarmLogStreamHandle != 0)
	{
		LOG_API_CALL_TRY_AGAIN(saLogStreamClose(alarmLogStreamHandle));
		if (Errt != SA_AIS_OK)
		{
			ERR_MWSA_LOG("saLogStreamClose(%llu) for alarm log stream failed (%d)",alarmLogStreamHandle,Errt);
			myRetVal = MafFailure;
		} else {
			LOG_MWSA_LOG("saLogStreamClose(%llu) for alarms was successful (%d)",alarmLogStreamHandle,Errt);
		}
		alarmLogStreamHandle = 0;
	}

	if(alarmLogHandle != 0)
	{
		LOG_API_CALL_TRY_AGAIN(saLogFinalize(alarmLogHandle));
		if (Errt != SA_AIS_OK)
		{
			ERR_MWSA_LOG("saLogFinalize(%llu) for alarm log stream failed (%d)",alarmLogHandle,Errt);
			myRetVal = MafFailure;
		}
		else
		{
			LOG_MWSA_LOG("saLogFinalize(%llu) for alarms was successful (%d)",alarmLogHandle,Errt);
		}
		alarmLogHandle = 0;
	}

	// Closing alertOpenThread if still open
	if (alertOpenThreadId != 0)
	{
	  DEBUG_MWSA_LOG("AlarmAndAlertLogServiceClose()  alertOpenThreadId= %d", (int) alertOpenThreadId);
		status = pthread_kill(alertOpenThreadId, SIGKILL);
		if(status < 0)
		{
			LOG_MWSA_LOG("pthread_kill failed for alertOpenThread");
		}
		else
		{

			LOG_MWSA_LOG("pthread_kill successful for alertOpenThread");
		}

		status = pthread_join(alertOpenThreadId, NULL);
		if(status < 0)
		{
			LOG_MWSA_LOG("pthread_join failed for alertOpenThread");
		}
		else
		{
			LOG_MWSA_LOG("pthread_join successful for alertOpenThread");
		}
	}
	poll_unsetcallback(comSASThandle_ntf, alertSelectionObject);

	// Closing alertLogStream
	alertLogFileIsOpen = FALSE;
	if(alertLogStreamHandle != 0)
	{
		LOG_API_CALL_TRY_AGAIN(saLogStreamClose(alertLogStreamHandle));
		if (Errt != SA_AIS_OK)
		{
			ERR_MWSA_LOG("saLogStreamClose(%llu) for alert log stream failed (%d)",alertLogStreamHandle,Errt);
			myRetVal = MafFailure;
		} else {
			LOG_MWSA_LOG("saLogStreamClose(%llu) for alerts was successful (%d)",alertLogStreamHandle,Errt);
		}
		alertLogStreamHandle = 0;
	}

	if(alertLogHandle != 0)
	{
		LOG_API_CALL_TRY_AGAIN(saLogFinalize(alertLogHandle));
		if (Errt != SA_AIS_OK)
		{
			ERR_MWSA_LOG("saLogFinalize(%llu) for alert log stream failed (%d)",alertLogHandle,Errt);
			myRetVal = MafFailure;
		}
		else
		{
			DEBUG_MWSA_LOG("saLogFinalize(%llu) for alerts was successful (%d)",alertLogHandle,Errt);
		}
		alertLogHandle = 0;
	}

	DEBUG_MWSA_LOG("AlarmAndAlertLogServiceClose LEAVE (%d)",myRetVal);
	LEAVE_MWSA_LOG();
	return myRetVal;
}


MafReturnT cmdLogServiceClose(void)
{
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("cmdLogServiceClose ENTER");
	SaAisErrorT Errt = SA_AIS_OK;
	MafReturnT	myRetVal = MafOk;
	int status;
	// Closing cmdLogOpenThread if still open
	if (cmdLogOpenThreadId != 0)
	{
	  DEBUG_MWSA_LOG("cmdLogServiceClose() cmdLogOpenThreadId = %d", (int) cmdLogOpenThreadId);
		status = pthread_kill(cmdLogOpenThreadId, SIGKILL);
		if(status < 0)
		{
			LOG_MWSA_LOG("pthread_kill failed for cmdLogOpenThread");
		}
		else
		{
			LOG_MWSA_LOG("pthread_kill successful for cmdLogOpenThread");
		}

		status = pthread_join(cmdLogOpenThreadId, NULL);
		if(status < 0)
		{
			LOG_MWSA_LOG("pthread_join failed for cmdLogOpenThread");
		}
		else
		{
			LOG_MWSA_LOG("pthread_join successful for cmdLogOpenThread");
		}
	}
	poll_unsetcallback(comSASThandle_ntf, cmdSelectionObject);

	// Closing cmdLogStream
	cmdLogFileIsOpen = FALSE;
	if(cmdLogStreamHandle != 0)
	{
		LOG_API_CALL_TRY_AGAIN(saLogStreamClose(cmdLogStreamHandle));
		if (Errt != SA_AIS_OK)
		{
			ERR_MWSA_LOG("saLogStreamClose(%llu) for cmdLogStream failed (%d)",cmdLogStreamHandle,Errt);
			myRetVal = MafFailure;
		} else {
			DEBUG_MWSA_LOG("saLogStreamClose(%llu) for cmdLogStream was successful(%d)",cmdLogStreamHandle,Errt);
			syslog(LOG_USER*8 + LOG_INFO, "cmdLogService is successfully closed");
		}
		cmdLogStreamHandle = 0;
	}

	if(cmdLogHandle != 0)
	{
		LOG_API_CALL_TRY_AGAIN(saLogFinalize(cmdLogHandle));
		if (Errt != SA_AIS_OK)
		{
			ERR_MWSA_LOG("saLogFinalize(%llu) for cmd log stream failed (%d)",cmdLogHandle,Errt);
			myRetVal = MafFailure;
		}
		else
		{
			LOG_MWSA_LOG("saLogFinalize(%llu) for cmds was successful (%d)",cmdLogHandle,Errt);
		}
		cmdLogHandle = 0;
	}

	if (cmdLogString != NULL)
	{
		free(cmdLogString);
		cmdLogString = NULL;
	}

	DEBUG_MWSA_LOG("cmdLogServiceClose LEAVE (%d)",myRetVal);
	LEAVE_MWSA_LOG();
	return myRetVal;
}
/*
 *  ComLogServiceClose
 */
MafReturnT ComLogServiceClose(void)
{
	SaAisErrorT Errt = SA_AIS_OK;
	MafReturnT	myRetVal = MafOk;

	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("ComLogServiceClose ENTER");

	if (OpenThreadId != 0)
	{
		// Stomp it to death.....quite brutally
		pthread_kill(OpenThreadId, SIGKILL);
	}

	poll_unsetcallback(comSASThandle_ntf, theSelectionObject);

	/* Close the SA Forum log stream */
	LogFileIsOpen = FALSE;
	LOG_API_CALL_TRY_AGAIN(saLogStreamClose(theLogStreamHandle));
	if (Errt != SA_AIS_OK)
	{
		ERR_MWSA_LOG("saLogStreamClose(%llu) for com logs failed (%d)",theLogStreamHandle,Errt);
		myRetVal = MafFailure;
	} else {
		LOG_MWSA_LOG("saLogStreamClose(%llu) for com logs was successful(%d)",theLogStreamHandle,Errt);
		auditlogger("com",LOG_INFO,1,"security_audit","Com LogService successfully closed");

	}

	/* Finalize use of the SA Forum logging utility */
	LOG_API_CALL_TRY_AGAIN(saLogFinalize(theLogHandle));
	if (Errt != SA_AIS_OK)
	{
		myRetVal = MafFailure;
	}

	if (IdentityString != NULL)
	{
		free(IdentityString);
		IdentityString = NULL;
	}

	DEBUG_MWSA_LOG("ComLogServiceClose LEAVE with (%d)",myRetVal);
	LEAVE_MWSA_LOG();
	return myRetVal;
}

MafReturnT secLogServiceClose(void)
{
	SaAisErrorT Errt = SA_AIS_OK;
	MafReturnT	myRetVal = MafOk;
	int status;

	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("secLogServiceClose ENTER");

	if (secLogOpenThreadId != 0)
	{
		// Stomp it to death.....quite brutally
		DEBUG_MWSA_LOG("secLogServiceClose() secLogOpenThreadId = %d", (int) secLogOpenThreadId);
		status = pthread_kill(secLogOpenThreadId, SIGKILL);
		if(status < 0)
		{
			LOG_MWSA_LOG("pthread_kill failed for secLogOpenThreadId");
		}
		else
		{

			LOG_MWSA_LOG("pthread_kill successful for secLogOpenThreadId");
		}

		status = pthread_join(secLogOpenThreadId, NULL);
		if(status < 0)
		{
			LOG_MWSA_LOG("pthread_join failed for secLogOpenThreadId");
		}
		else
		{
			LOG_MWSA_LOG("pthread_join successful for secLogOpenThreadId");
		}
	}

	poll_unsetcallback(comSASThandle_ntf, secSelectionObject);

	/* Close the SA Forum log stream */
	secLogFileIsOpen = FALSE;
	if(secLogStreamHandle != 0)
	{
		LOG_API_CALL_TRY_AGAIN(saLogStreamClose(secLogStreamHandle));
		if (Errt != SA_AIS_OK)
		{
			ERR_MWSA_LOG("saLogStreamClose(%llu) for security log stream failed (%d)",secLogStreamHandle,Errt);
			myRetVal = MafFailure;
		} else {
			DEBUG_MWSA_LOG("saLogStreamClose(%llu) for security log stream was successful (%d)",secLogStreamHandle,Errt);
			syslog(LOG_USER*8 + LOG_INFO, "SecLogService is successfully closed");
		}
		secLogStreamHandle = 0;
	}

	if(secLogHandle != 0)
	{
		LOG_API_CALL_TRY_AGAIN(saLogFinalize(secLogHandle));
		if (Errt != SA_AIS_OK)
		{
			ERR_MWSA_LOG("saLogFinalize(%llu) for sec log stream failed (%d)",secLogHandle,Errt);
			myRetVal = MafFailure;
		}
		else
		{
			LOG_MWSA_LOG("saLogFinalize(%llu) for sec logs  was successful (%d)",secLogHandle,Errt);
		}
		secLogHandle = 0;
	}

	DEBUG_MWSA_LOG("secLogServiceClose LEAVE with (%d)",myRetVal);
	LEAVE_MWSA_LOG();
	return myRetVal;
}

void setSaLogSystemSevFilter()
{
	SaAisErrorT    Errt = SA_AIS_OK;
	SaImmHandleT   immOmHandle = 0;
	SaImmAccessorHandleT accessorHandle = 0;

	DEBUG_MWSA_LOG("setSaLogSystemSevFilter");
	if((Errt = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &imm_version)) != SA_AIS_OK) {
		WARN_MWSA_LOG("setSaLogSystemSevFilter: saImmOmInitialize failed %s", getOpenSAFErrorString(Errt));
	} else {
		if((Errt = autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandle)) != SA_AIS_OK) {
			WARN_MWSA_LOG("setSaLogSystemSevFilter: saImmOmAccessorInitialize failed %s", getOpenSAFErrorString(Errt));
		} else {
			int32_t sevFilter = 0;
			Errt = getSeverityFilter(accessorHandle,(SaStringT)SALOG_LOGM_DN, &sevFilter);
			if(Errt != SA_AIS_OK){
				DEBUG_MWSA_LOG("setSaLogSystemSevFilter: get severityFilter from salog instance is failed, ret = %d", Errt);
			} else {
				DEBUG_MWSA_LOG("setSaLogSystemSevFilter: severityFilter from salog instance is  %d", sevFilter);
				saSystemLogSevFlags = sevFilter;
				logMEnabled = true;
			}
		}
		om_imm_finalize(immOmHandle, accessorHandle);
	}
}

/* SDP1694 - support MAF SPI */
MafReturnT maf_ComLogServiceOpen()
{
	DEBUG_MWSA_LOG("maf_ComLogServiceOpen ENTER");
	SaVersionT      theVersion = {logReleaseCode, logMajorVersion, logMinorVersion};
	SaLogCallbacksT theCallBacks = {logFilterSetCallbackT, NULL, logWriteLogCallbackT};
	SaAisErrorT     Errt = SA_AIS_OK;
	MafReturnT      myRetVal = MafOk;
	int             PthreadResult = 0;

	ENTER_MWSA_LOG();

	extern MafMwSpiTrace_1T maf_comSATrace_interface;
	traceWrite = maf_comSATrace_interface.traceWrite;

	#ifdef UNIT_TEST
		asprintf(&IdentityString, "%s", "COM SA logger");
	#else
		asprintf(&IdentityString, "%s%s", _CC_NAME_UPPERCASE, " SA logger");
	#endif

	/* Set up the logger name */
	saNameSet(IdentityString, &theLoggerName);

	/* Open the SA Forum log service */
	LOG_API_CALL(saLogInitialize(&theLogHandle ,&theCallBacks , &theVersion));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogInitialize error = %d", Errt);
		DEBUG_MWSA_LOG("Version releasecode %d major %d minor %d", theVersion.releaseCode,theVersion.majorVersion,theVersion.minorVersion);
		myRetVal = MafFailure;
		goto end_exit;
	}

	/* Get a selection object to catch the callbacks */
	LOG_API_CALL(saLogSelectionObjectGet(theLogHandle ,&theSelectionObject));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogSelectionObjectGet error = %d", Errt);
		myRetVal = MafFailure;
		goto end_exit;
	}

	setSaLogSystemSevFilter();

	// Create the open thread
	DEBUG_MWSA_LOG("maf_ComLogServiceOpen Calling pthread_create");
	if ((PthreadResult = pthread_create(&OpenThreadId,NULL, &LogServiceOpenThread, NULL)) != 0)
	{
		DEBUG_MWSA_LOG("maf_ComLogServiceOpen pthread_create failed PthreadResult = %d",PthreadResult);
		syslog(LOG_USER*8 + LOG_ERR, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to create ",PthreadResult);
		goto end_exit;
	}
end_exit:
	DEBUG_MWSA_LOG("maf_ComLogServiceOpen LEAVE (%d)",myRetVal);
	LEAVE_MWSA_LOG();
	return myRetVal;
}

/*
 *  Routines called via the pointer interface
 *
 */
/* SDP1694 - support MAF SPI using wrapper */
#ifndef UNIT_TEST
static MafReturnT maf_logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer){
#else
MafReturnT maf_logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer){
#endif

	ENTER_MWSA_LOG();
	MafReturnT RetVal = (MafReturnT) logWrite(eventId, severity, facility, databuffer);
	LEAVE_MWSA_LOG();
	return RetVal;

}

/*
 * Returns logger Info i.e stream handle, logger name .. based on facility and isLogAllowed based on severity
 */
void getLoggerInfo( MwSpiFacilityT facility,  MwSpiSeverityT* severity, int** isLogFileOpen, SaNameT **loggerName, SaLogStreamHandleT** logStreamHandle, bool* isLogAllowed) {
	//This block is applicable only for LM as its always looged in to saflogs(saLogSystem)
	if(!strcmp("lm", _CC_NAME)) {
		*isLogFileOpen = &LogFileIsOpen;
		*loggerName = &theLoggerName;
		*logStreamHandle = &theLogStreamHandle;
        //Removed Severity Level Conversion from INFO to NOTICE for LM Component as part of TR IA56234.
		*isLogAllowed = saSystemLogSevFlags & SEVERITY_FLAG(*severity);
		return;
	}

	DEBUG_MWSA_LOG("getLoggerInfo(), facility = %d", facility);

	if(facility == ALARM_LOG){
		*isLogFileOpen = &alarmLogFileIsOpen;
		*loggerName = &alarmLoggerName;
		*logStreamHandle = &alarmLogStreamHandle;
		*isLogAllowed = alarmLogSevFlags & SEVERITY_FLAG(*severity);
	}
	else if (facility == ALERT_LOG){
		*isLogFileOpen = &alertLogFileIsOpen;
		*loggerName = &alertLoggerName;
		*logStreamHandle = &alertLogStreamHandle;
		*isLogAllowed = alertLogSevFlags & SEVERITY_FLAG(*severity);
	}
	else if (facility == COMMAND_LOG){
		*isLogFileOpen = &cmdLogFileIsOpen;
		*loggerName = &cmdLoggerName;
		*logStreamHandle = &cmdLogStreamHandle;
		*isLogAllowed = cmdLogSevFlags & SEVERITY_FLAG(*severity);
	}
	else if (facility == SECURITY_LOG){
		*isLogFileOpen = &secLogFileIsOpen;
		*loggerName = &secLoggerName;
		*logStreamHandle = &secLogStreamHandle;
		*isLogAllowed = secLogSevFlags & SEVERITY_FLAG(*severity);
	}else{
		*isLogFileOpen = &LogFileIsOpen;
		*loggerName = &theLoggerName;
		*logStreamHandle = &theLogStreamHandle;
		//As the stream owner is coreMW and by default the loglevel value is NOTICE
		//SA modified the loglevel INFO to NOTICE to log INFO logs
		if(*severity == MW_SA_LOG_SEV_INFO){
			*severity = MW_SA_LOG_SEV_NOTICE;
		}
		*isLogAllowed = saSystemLogSevFlags & SEVERITY_FLAG(*severity);
	}
	return;
}
 /*
  *  logWrite
  */
#ifndef UNIT_TEST
static MafReturnT logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer){
#else
MafReturnT logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer){
#endif
	ENTER_MWSA_LOG();
	DEBUG_MWSA_LOG("logWrite(): ENTER");
	MafReturnT RetVal = MafOk;
	SaAisErrorT Errt;

	switch(facility)
	{
		case SECURITY_AUTHORIZATION_MESSAGE_1:
		case SECURITY_AUTHORIZATION_MESSAGE_2:
		{
			/* Send messages to the LINUX syslog */
			int priority = SYSLOG_LEVEL(severity);

			/* Priority acccording to RFC 3156, not mentioned on man pages */
			priority += (facility*8);
			syslog(priority,"%s",databuffer);
			break;
		}
		case LOG_AUDIT_MESSAGE:
		case LOG_AUDIT_MESSAGE_LOCAL1:
		{
	        auditlogger("com",LOG_NOTICE,1,"security_audit",databuffer);
			break;
		}
		case COMMAND_LOG:
		case SECURITY_LOG:
		{
			int priority = SYSLOG_LEVEL(severity);
	        auditlogger("com",priority,1,"security_audit",databuffer);
			break;
		}
		case ALARM_LOG:
		case ALERT_LOG:
		default:
		{
			SaNameT* loggerName;
			int* isLogFileOpen;
			SaLogStreamHandleT* logStreamHandle;
			MwSpiSeverityT tmpSeverity = severity;
			bool isLogAllowed = false;
			getLoggerInfo(facility,  &tmpSeverity, &isLogFileOpen, &loggerName, &logStreamHandle,&isLogAllowed);
			/* If the log file isn't open log to syslog instead with a warning message
			 *
			 */
			if (isLogAllowed || (trace_flag == 1) ){
				if (!isLogFileOpen)
				{
					syslog(LOG_USER*8 + LOG_WARNING, "%s",databuffer);
				}
				else
				{
					/* Call SA Forum log utilitiy and write to the system log. We are using the asynchronous interface
					 * just because the synchronous one hasn't been implemented yet....
					 */
					SaLogRecordT LogRecord;
					SaLogBufferT LogBuffer;

					if (facility != SECURITY_LOG &&  facility != COMMAND_LOG) {
						if (traceWrite != NULL) {
							traceWrite(0, databuffer);
						}
					}

					LogRecord.logTimeStamp = SA_TIME_UNKNOWN; 			/* Let the log subsystem time stamp it */
					LogRecord.logHdrType = SA_LOG_GENERIC_HEADER;
					LogRecord.logHeader.genericHdr.notificationClassId	= NULL;
					LogRecord.logHeader.genericHdr.logSvcUsrName = loggerName;
					LogRecord.logHeader.genericHdr.logSeverity = SEVERITY(tmpSeverity);

					LogBuffer.logBuf = (unsigned char*)databuffer;
					LogBuffer.logBufSize = strlen(databuffer);
					LogRecord.logBuffer = &LogBuffer;

					/* A matter of interest is if the "LogRecord" must
					 * remain allocated until the callback is received and
					 * if the "saLogWriteLogAsync" is thread safe.
					 *
					 * After discussions with Arne we found;
					 * 1. The LogRecord is encoded and sent in the call,
					 *    so it is safe to let it go after the call.
					 * 2. Some handle is reserved during the send which
					 *    ensures a thread safe call.
					 *
					 * Conclusion; the saLogWriteLogAsync is thread safe
					 * and we can throw away the LogRecord after the call.
					 */
					LOG_API_CALL(saLogWriteLogAsync(*logStreamHandle, 0, SA_LOG_RECORD_WRITE_ACK, &LogRecord));
					if (Errt != SA_AIS_OK)
					{
						WARN_MWSA_LOG("Failure after saLogWriteLog facility = %d, error = %d", facility, Errt);
						RetVal = MafFailure;
					}
					break;
				}
			}
		}
	}
	DEBUG_MWSA_LOG("logWrite(): RETURN with %d",RetVal);
	LEAVE_MWSA_LOG();
	return RetVal;
}
