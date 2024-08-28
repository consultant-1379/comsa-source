/*
 * ComSATrace.cc
 *
 *  Created on: Jan 9, 2019
 *      Author: zyxxroj
 */

#include "SaInternalTrace.h"
#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <errno.h>
#include "SelectTimer.h"
#include "InternalTrace.h"
#include "UtilsInterface.h"
#include "imm_utils.h"

static char* compSaIdentityString = NULL;
static SaNameT compSaCfgLoggerName;
static SaLogHandleT compSaCfgLogHandle = 0;
static SaLogStreamHandleT compSaCfgLogStreamHandle = 0;
static SaSelectionObjectT compSaCfgSelectionObject;
static SaNameT compSaCfgLogStreamName;
static pthread_t compSaCfgOpenThreadId = 0;
static int compSaCfgLogFileIsOpen = FALSE;
static const unsigned int SleepTime = 1;
static bool status=false;
extern int trace_flag;
#ifndef UNIT_TEST
static SaLogSeverityFlagsT comSaLogSevFlags = SALOG_SEV_FLAG_INFO_OFF;
#else
SaLogSeverityFlagsT comSaLogSevFlags = SALOG_SEV_FLAG_INFO_OFF;
#endif
/*
 * call backs
 */

static void compSaLogFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	if(trace_flag) {
		syslog(LOG_USER*8 + LOG_INFO,"compSaLogFilterSetCallbackT, received severity = %d, stream handle = %llu",logSeverity, logStreamHandle);
	}
	comSaLogSevFlags = logSeverity;
}

static void compSaCfgLogWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	if (error != SA_AIS_OK){
		if(error==SA_AIS_ERR_TRY_AGAIN)
		{
			if(status==false){
				syslog(LOG_USER*8 + LOG_WARNING,"compSaLogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
				status = true;
			}
		}
		else{
			syslog(LOG_USER*8 + LOG_WARNING,"compSaLogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
			status = false;
		}
	}
}

static void compSaCfgfdCallbackFn(struct pollfd* pfd, void *ref)
{
	SaAisErrorT Errt;
	Errt = saLogDispatch(compSaCfgLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK)
		WARN_MWSA_LOG("saLogDispatch FAILED: %d", Errt);
}

static void* compSaCfgLogServiceOpenThread()
{
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;
	ENTER_MWSA_LOG();
	struct pollfd pfd;
	int   PthreadResult = 0;

	/* Set up logstream name */
	if(0 == strcmp((const char*)_CC_NAME, "lm")) {
		saNameSet((const char*)LMSA_LOG_STREAM_DN, &compSaCfgLogStreamName);
	}else{
		saNameSet((const char*)COMSA_LOG_STREAM_DN, &compSaCfgLogStreamName);
	}

	DEBUG_MWSA_LOG("LogServiceOpenThread Calling pthread_detach");
	if ((PthreadResult = pthread_detach(compSaCfgOpenThreadId)) != 0)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to detach thread ",PthreadResult);
	}

	DEBUG_MWSA_LOG("Executing LogServiceOpenThread");
	for (;Errt != SA_AIS_OK;)
	{
		//myLogStreamName.value[myLogStreamName.length] = '\0';
		DEBUG_MWSA_LOG("Calling saLogStreamOpen_2 Log stream name = %s length = %u", saNameGet(&compSaCfgLogStreamName), (unsigned) strlen(saNameGet(&compSaCfgLogStreamName)));
		LOG_API_CALL(saLogStreamOpen_2(
					 compSaCfgLogHandle,
					 &compSaCfgLogStreamName,
					 (SaLogFileCreateAttributesT_2*)NULL,
					 0,
					 SA_TIME_ONE_MINUTE,
					 &compSaCfgLogStreamHandle));
		DEBUG_MWSA_LOG("After saLogStreamOpen_2 Return value = %d, LogStreamHandle = %lld",Errt,compSaCfgLogStreamHandle);
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
						pthread_exit(&Errt);
						return NULL;
					}
				}
			}
		}
	} // end for

	if (0 == strcmp((const char*)_CC_NAME, "lm")) {
		auditlogger(_CC_NAME,LOG_INFO,1,"security_audit","lmSaCfgLogService is successfully opened");
	}else{
		LOG_MWSA_LOG("ComSaCfgLogService is successfully opened");
	}
	pfd.events = POLLIN;
	pfd.fd = compSaCfgSelectionObject;
	if (poll_setcallback(comSASThandle_ntf, compSaCfgfdCallbackFn, pfd, NULL) != 0)
	{
		WARN_MWSA_LOG("poll_setcallback failed");
	}else{
		compSaCfgLogFileIsOpen = TRUE;
		compSaCfgOpenThreadId  = 0;
	}
	pthread_exit(&Errt);
	return NULL;
}

void setCompSaLogMSevFilter()
{
	SaImmHandleT immOmHandle = 0;
	SaImmAccessorHandleT accessorHandle = 0;
	SaAisErrorT Errt = SA_AIS_OK;

	DEBUG_MWSA_LOG("setCompSaLogMSevFilter");
	if((Errt = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &imm_version)) != SA_AIS_OK) {
		WARN_MWSA_LOG("setCompSaLogMSevFilter: saImmOmInitialize failed %s", getOpenSAFErrorString(Errt));
	} else {
		if((Errt = autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandle)) != SA_AIS_OK) {
			WARN_MWSA_LOG("setCompSaLogMSevFilter: saImmOmAccessorInitialize failed %s", getOpenSAFErrorString(Errt));
		} else {
			int32_t sevFilter = 0;
			if (0 == strcmp((const char*)_CC_NAME, "lm")) {
				Errt = getSeverityFilter(accessorHandle,(SaStringT)LMSA_LOGM_DN, &sevFilter);
			}else{
				Errt = getSeverityFilter(accessorHandle,(SaStringT)COMSA_LOGM_DN, &sevFilter);
			}
			if(Errt != SA_AIS_OK){
				DEBUG_MWSA_LOG("setCompSaLogMSevFilter: get severityFilter from salog instance is failed, ret = %d", Errt);
			} else {
				DEBUG_MWSA_LOG("setCompSaLogMSevFilter: severityFilter from salog instance is  %d", sevFilter);
				comSaLogSevFlags = sevFilter;
			}
		}
		om_imm_finalize(immOmHandle, accessorHandle);
	}
}

MafReturnT initCompSaCfgLogStream()
{
	SaVersionT 		theVersion = { logReleaseCode, logMajorVersion, logMinorVersion};
	SaLogCallbacksT	theCallBacks = {compSaLogFilterSetCallbackT, NULL, compSaCfgLogWriteLogCallbackT};
	SaAisErrorT 	Errt = SA_AIS_OK;
	MafReturnT		myRetVal = MafOk;
	int   PthreadResult = 0;
	/* Set up the logger name */
	if(0 == strcmp((const char*)_CC_NAME, "lm")) {
		asprintf(&compSaIdentityString, "%s", "LM SA logger");
	}else{
		asprintf(&compSaIdentityString, "%s", "COM SA logger");
	}
	saNameSet(compSaIdentityString, &compSaCfgLoggerName);
	/* Open the SA Forum log service */
	LOG_API_CALL(saLogInitialize(&compSaCfgLogHandle ,&theCallBacks , &theVersion));
	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("initCompSaCfgLogStream Failure after saLogInitialize error = %d", Errt);
		DEBUG_MWSA_LOG("initCompSaCfgLogStream Version releasecode %d major %d minor %d", theVersion.releaseCode,theVersion.majorVersion,theVersion.minorVersion);
		myRetVal = MafFailure;
	}else{
		/* Get a selection object to catch the callbacks */
		LOG_API_CALL(saLogSelectionObjectGet(compSaCfgLogHandle ,&compSaCfgSelectionObject));
		if (Errt != SA_AIS_OK)
		{
			WARN_MWSA_LOG("initCompSaCfgLogStream Failure after saLogSelectionObjectGet error = %d", Errt);
			myRetVal = MafFailure;
		}else{
			// read severity from the log instance
			setCompSaLogMSevFilter();

			// Create the open thread
			DEBUG_MWSA_LOG("initCompSaCfgLogStream Calling pthread_create");
			if ((PthreadResult = pthread_create(&compSaCfgOpenThreadId,NULL, &compSaCfgLogServiceOpenThread, NULL)) != 0)
			{
				DEBUG_MWSA_LOG("initCompSaCfgLogStream pthread_create failed PthreadResult = %d",PthreadResult);
				syslog(LOG_USER*8 + LOG_ERR, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to create ",PthreadResult);
			}
		}
	}
	DEBUG_MWSA_LOG("initCompSaCfgLogStream LEAVE (%d)",myRetVal);
	LEAVE_MWSA_LOG();
	return myRetVal;
}

void finalizeCompSaCfgLogStream(){
	SaAisErrorT Errt = SA_AIS_OK;
	if (compSaCfgOpenThreadId != 0)
	{
		pthread_kill(compSaCfgOpenThreadId, SIGKILL);
	}

	poll_unsetcallback(comSASThandle_ntf, compSaCfgSelectionObject);

	/* Close the SA Forum log stream */
	compSaCfgLogFileIsOpen = FALSE;
	LOG_API_CALL_TRY_AGAIN(saLogStreamClose(compSaCfgLogStreamHandle));
	if (Errt != SA_AIS_OK)
	{
		DEBUG_MWSA_TRACE("saLogStreamClose failed handle: %llu, ret: %d", compSaCfgLogStreamHandle, Errt);
	} else {
		DEBUG_MWSA_TRACE("saLogStreamClose %llu for compSaCfgLogStream ret: %d", compSaCfgLogStreamHandle, Errt);
		if (0 == strcmp((const char*)_CC_NAME, "lm")) {
			auditlogger(_CC_NAME,LOG_INFO,1,"security_audit","lmSaCfgLogService is successfully closed");
		}else{
			LOG_MWSA_LOG("ComSaCfgLogService is successfully closed");
		}
	}

	/* Finalize use of the SA Forum logging utility */
	LOG_API_CALL_TRY_AGAIN(saLogFinalize(compSaCfgLogHandle));
	if (Errt != SA_AIS_OK)
	{
		DEBUG_MWSA_TRACE("saLogFinalize failed handle: %llu, ret: %d", compSaCfgLogHandle, Errt);
	}

	if (compSaIdentityString != NULL)
	{
		free(compSaIdentityString);
		compSaIdentityString = NULL;
	}
}

MafReturnT compSaCfgLogStreamWrite(MwSpiSeverityT severity, const char* databuffer){
	MafReturnT RetVal = MafOk;
	SaAisErrorT Errt;

	/* Call SA Forum log utility and write to the system log. We are using the asynchronous interface
	 * just because the synchronous one hasn't been implemented yet....
	 */
	SaLogRecordT LogRecord;
	SaLogBufferT LogBuffer;
	const unsigned int SleepTime = 1;

	LogRecord.logTimeStamp = SA_TIME_UNKNOWN; 			/* Let the log subsystem time stamp it */
	LogRecord.logHdrType = SA_LOG_GENERIC_HEADER;
	LogRecord.logHeader.genericHdr.notificationClassId	= NULL;
	LogRecord.logHeader.genericHdr.logSvcUsrName		= &compSaCfgLoggerName;
	LogRecord.logHeader.genericHdr.logSeverity = SEVERITY(severity);

	LogBuffer.logBuf = (unsigned char*)databuffer;
	LogBuffer.logBufSize = strlen(databuffer);
	LogRecord.logBuffer = &LogBuffer;
	LOG_API_CALL(saLogWriteLogAsync(compSaCfgLogStreamHandle, 0, SA_LOG_RECORD_WRITE_ACK, &LogRecord));
	if (Errt != SA_AIS_OK)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "Failure after saLogWriteLog error = %d",Errt);
		RetVal = MafFailure;
	}
	return RetVal;
}

void coremw_vlog(int priority, char const* fmt, va_list ap)
{
	char buffer[TRACE_BUFFER_SIZE];
	if(_CC_NAME_SA == NULL)
	{
		char* tmp = convertToUpperCase((const char*)CC_NAME);
		asprintf(&_CC_NAME_SA, "%s%s", tmp, "_SA");
		if (tmp)
		{
			free(tmp);
		}
	}

	int len = strlen((const char*)_CC_NAME_SA);
	strcpy(buffer, (const char*)_CC_NAME_SA);
	buffer[len] = ' ';
	len++;
	vsnprintf(buffer+len, sizeof(buffer)-len, fmt, ap);
	if (!compSaCfgLogFileIsOpen){
		syslog(LOG_USER*8 + priority, "%s", buffer);
	}else{
		if ( priority > MW_SA_LOG_SEV_INFO){
			priority = MW_SA_LOG_SEV_INFO;
		}
		compSaCfgLogStreamWrite((MwSpiSeverityT)priority,(const char*)buffer);
	}
}

#ifndef UNIT_TEST
void coremw_log(int priority, char const* fmt, ...)
{
	MwSpiSeverityT tmpSev = (MwSpiSeverityT)priority;
	if ( tmpSev > MW_SA_LOG_SEV_INFO){
		tmpSev = MW_SA_LOG_SEV_INFO;
	}
	bool isLogAllowed = comSaLogSevFlags & SEVERITY_FLAG(tmpSev);
	if(isLogAllowed || (trace_flag == 1) ){
		va_list ap;
		va_start(ap, fmt);
		coremw_vlog(priority, fmt, ap);
		va_end(ap);
	}
}

void coremw_debug_log(int priority, char const* fmt, ...)
{
	MwSpiSeverityT tmpSev = (MwSpiSeverityT)priority;
	if ( tmpSev > MW_SA_LOG_SEV_INFO){
		tmpSev = MW_SA_LOG_SEV_INFO;
	}
	bool isLogAllowed = comSaLogSevFlags & SEVERITY_FLAG(tmpSev);
	if(isLogAllowed || (trace_flag == 1) ){
		va_list ap;
		va_start(ap, fmt);
		coremw_vlog(priority, fmt, ap);
		va_end(ap);
	}
}
#endif
