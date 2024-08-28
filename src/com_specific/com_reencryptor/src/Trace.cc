#include "Trace.h"
#include "Defines.h"

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cerrno>

static pthread_mutex_t _traceMutexLock = PTHREAD_MUTEX_INITIALIZER;

char*			Trace::identityString = NULL;
SaLogHandleT		Trace::theLogHandle = 0;
SaLogStreamHandleT 	Trace::theLogStreamHandle = 0;
SaNameT*		Trace::theLoggerName = NULL;
SaSelectionObjectT 	Trace::theSelectionObject = 0;
SaNameT*		Trace::myLogStreamName = NULL;
pthread_t		Trace::OpenThreadId = 0;
bool 			Trace::LogFileIsOpen = false;
SaLogSeverityFlagsT 	Trace::logSevFlags = SALOG_SEV_FLAG_INFO_OFF;
void* Trace::handle_ntf = NULL;
int Trace::ntf_event_pipe[2];
static int stop_thread = 0;
static bool trace_status = false;

Trace::Trace(): debugsEnabled(false)
{

}

Trace::~Trace()
{
}

void Trace::logFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	DEBUG_COM_RP("logFilterSetCallbackT received with severity = %d and stream handle = %llu", logSeverity, logStreamHandle);
	logSevFlags = logSeverity;
}

void Trace::logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
{
	if (error != SA_AIS_OK){
		if(error==SA_AIS_ERR_TRY_AGAIN)
		{
		  if(trace_status==false){
			 syslog(LOG_USER*8 + LOG_WARNING,"comCfglogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
			 trace_status = true;
		  }
		}
		else{
		  syslog(LOG_USER*8 + LOG_WARNING,"comCfglogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
		  trace_status = false;
		}
	}
}

void Trace::fdCallbackFn(struct pollfd* pfd, void *ref)
{
	SaAisErrorT Errt;
	Errt = saLogDispatch(theLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK) {
		WARN_COM_RP("saLogDispatch FAILED: %d", Errt);
	}
}

void* Trace::LogServiceOpenThread(void * arg)
{
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;

	struct pollfd pfd;
	int   PthreadResult = 0;

	/* Set up logstream name */
	myLogStreamName = ImmUtil::Instance().makeSaNameT(std::string(COMSA_LOG_STREAM_DN));

	DEBUG_COM_RP("LogServiceOpenThread calling pthread_detach");
	if ((PthreadResult = pthread_detach(OpenThreadId)) != 0)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %d", "COM re-encryption participant -- LogService failed to detach thread", PthreadResult);
	}

	DEBUG_COM_RP("Executing LogServiceOpenThread");
	for (;Errt != SA_AIS_OK;)
	{
		LOG_API_CALL(saLogStreamOpen_2(
					theLogHandle,
					myLogStreamName,
					(SaLogFileCreateAttributesT_2*)NULL,
					0,
					SA_TIME_ONE_MINUTE,
					&theLogStreamHandle));
		DEBUG_COM_RP("After saLogStreamOpen_2 Return value = %d, LogStreamHandle = %lld", Errt, theLogStreamHandle);
		if (Errt == SA_AIS_ERR_TIMEOUT)
		{
			syslog(LOG_USER*8 + LOG_WARNING, "%s", "COM re-encryption participant -- Call to saLogStreamOpen_2 returned time-out. Retrying");
		}
		else if (Errt == SA_AIS_ERR_UNAVAILABLE)
		{
			// If not currently a member of the cluster, we might become later so sleep a while and try again
			syslog(LOG_USER*8 + LOG_WARNING, "%s", "COM re-encryption participant -- Call to saLogStreamOpen_2 returned SA_AIS_ERR_UNAVAILABLE. Retrying");
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
						syslog(LOG_USER*8 + LOG_ERR, "%s %d %s", "COM re-encryption participant -- Call to nanosleep returned", errno, "exiting");
						pthread_exit(&Errt);
						return NULL;
					}
				}
			}
		}
	} // end for

	pfd.events = POLLIN;
	pfd.fd = theSelectionObject;

	handle_ntf = TimerSelect::Instance().timerCreateHandle_r();
	TimerSelect::Instance().poll_maxfd(handle_ntf, 2);

	if (TimerSelect::Instance().poll_setcallback(handle_ntf, fdCallbackFn, pfd, NULL) != 0)
	{
		WARN_COM_RP("poll_setcallback failed");
		pthread_exit(&Errt);
		return NULL;
	}
	LogFileIsOpen = true;
	OpenThreadId  = 0;
	if (pipe(ntf_event_pipe) != 0) {
		ERR_COM_RP("Failed to create pipe");
	} else {
		/* Set the read-end of the pipe in non-blocking mode */
		int rc = fcntl(ntf_event_pipe[0], F_GETFL);
		if (rc < 0) abort();
		if (fcntl(ntf_event_pipe[0], F_SETFL, rc | O_NONBLOCK)) abort();
	}
	while (!stop_thread) {
		if (TimerSelect::Instance().poll_execute(handle_ntf) != 0)
			ERR_COM_RP("Drop out of the poll-loop for com-reencryptor logstream callbacks");

	}
	pthread_exit(&Errt);
	return NULL;
}

bool Trace::comSaCfgLogStreamWrite(MwSpiSeverityT severity, const char* fmt, va_list arg)
{
	bool retVal = true;
	SaAisErrorT Errt;

	if ( severity > MW_SA_LOG_SEV_INFO){
		severity = MW_SA_LOG_SEV_INFO;
	}

	bool isLogAllowed = logSevFlags & SEVERITY_FLAG(severity);

	if(isLogAllowed || debugsEnabled)
	{
		char buffer[4096];
		vsnprintf(buffer, sizeof(buffer), fmt, arg);

		if (!LogFileIsOpen)
		{
			syslog(LOG_USER*8 + LOG_INFO, "%s", buffer);
		}
		else
		{
			/* Call SA Forum log utility and write to the system log. We are using the asynchronous interface
			 * just because the synchronous one hasn't been implemented yet....
			 */
			SaLogRecordT LogRecord;
			SaLogBufferT LogBuffer;

			LogRecord.logTimeStamp = SA_TIME_UNKNOWN; 			/* Let the log subsystem time stamp it */
			LogRecord.logHdrType = SA_LOG_GENERIC_HEADER;
			LogRecord.logHeader.genericHdr.notificationClassId	= NULL;
			LogRecord.logHeader.genericHdr.logSvcUsrName = theLoggerName;
			LogRecord.logHeader.genericHdr.logSeverity = SEVERITY(severity);

			LogBuffer.logBuf = (unsigned char*)buffer;
			LogBuffer.logBufSize = strlen(buffer);
			LogRecord.logBuffer = &LogBuffer;
			LOG_API_CALL(saLogWriteLogAsync(theLogStreamHandle, 0, SA_LOG_RECORD_WRITE_ACK, &LogRecord));
			if (Errt != SA_AIS_OK)
			{
				WARN_COM_RP("Failure after saLogWriteLog error = %d", Errt);
				syslog(LOG_USER*8 + LOG_WARNING, "Failure after saLogWriteLog error = %d",Errt);
				retVal = false;
			}
		}
	}
	return retVal;
}

SaAisErrorT Trace::getSeverityFilter(int32_t* sevFilter)
{
	DEBUG_COM_RP("getSeverityFilter(): Enter");
	SaAisErrorT ret = SA_AIS_OK;
	SaImmAttrNameT attributeName[2];
	attributeName[0] = (SaStringT)LOGM_LOG_SEVERITYFILTER;
	attributeName[1] = (char *)'\0';
	SaImmAttrValuesT_2** attributeValue;

	if((ret = ImmUtil::Instance().getAttributeValue(std::string(COMSA_LOGM_DN), std::string(LOGM_LOG_SEVERITYFILTER), &attributeValue)) != SA_AIS_OK) {
		WARN_COM_RP("getSeverityFilter(): ImmUtil::getAttributeValue failed %d", ret);
	} else {
		DEBUG_COM_RP("getSeverityFilter(): ImmUtil::getAttributeValue : SUCCESS");
		SaImmAttrValuesT_2* oneAttrValue = attributeValue[0];
		if (strcmp(oneAttrValue->attrName, (char*)LOGM_LOG_SEVERITYFILTER)) {
			WARN_COM_RP("getSeverityFilter(): Expected attributeName returned %s. Received: %s", (char*)LOGM_LOG_SEVERITYFILTER, oneAttrValue->attrName);
			ret = SA_AIS_ERR_INVALID_PARAM;
		} else {
			if (oneAttrValue->attrValueType != SA_IMM_ATTR_SAINT32T) {
				WARN_COM_RP("getSeverityFilter(): Expected attrValueType returned %d. Received: %d", SA_IMM_ATTR_SAINT32T, oneAttrValue->attrValueType);
				ret = SA_AIS_ERR_INVALID_PARAM;
			} else {
				for(int i = 0 ; i < oneAttrValue->attrValuesNumber; i++){
					*sevFilter = *sevFilter | SEVERITY_FLAG(*((SaInt32T *)oneAttrValue->attrValues[i]));
				}
				DEBUG_COM_RP("getSeverityFilter() found attribute, value: %d.", *sevFilter);
			}
		}
	}
	DEBUG_COM_RP("getSeverityFilter(): Leave");
	return ret;
}

bool Trace::initLogStream()
{
	bool retVal = true;
	SaVersionT 		theVersion = {logReleaseCode, logMajorVersion, logMinorVersion};
	SaLogCallbacksT	theCallBacks = {logFilterSetCallbackT, NULL, logWriteLogCallbackT};
	SaAisErrorT 	Errt = SA_AIS_OK;
	int				pThreadResult = 0;

	DEBUG_COM_RP("Trace::initLogStream ENTER");

	// Create the identity string for Re-encryption Participant
	asprintf(&identityString, "%s", "COM RP logger");

	theLoggerName = ImmUtil::Instance().makeSaNameT(std::string(identityString));

	/* Open the SA Forum log service */
	LOG_API_CALL(saLogInitialize(&theLogHandle, &theCallBacks , &theVersion));

	if (Errt != SA_AIS_OK) {
		WARN_COM_RP("Failure after saLogInitialize error = %d", Errt);
		retVal = false;
	} else {
	    /* Get a selection object to catch the callbacks */
	    LOG_API_CALL(saLogSelectionObjectGet(theLogHandle ,&theSelectionObject));
	    if (Errt != SA_AIS_OK) {
		    WARN_COM_RP("Failure after saLogSelectionObjectGet error = %d\n", Errt);
		    retVal = false;
	    } else {
	        int32_t severityFilter = 0;
	        Errt = getSeverityFilter(&severityFilter);

	        if(Errt != SA_AIS_OK){
			DEBUG_COM_RP("Trace::initLogStream(): get severityFilter from ComSaCfgLogStream log instance is failed, ret = %d", Errt);
	        } else {
			DEBUG_COM_RP("Trace::initLogStream(): severityFilter from ComSaCfgLogStream log instance is  %d", severityFilter);
			logSevFlags = severityFilter;
	        }

	        // Create the open thread
	        DEBUG_COM_RP("Trace::initLogStream() Calling pthread_create");
	        if ((pThreadResult = pthread_create(&OpenThreadId,NULL, &LogServiceOpenThread, NULL)) != 0) {
		        DEBUG_COM_RP("Trace::initLogStream() pthread_create failed pThreadResult = %d", pThreadResult);
	        }
	    }
	}
	DEBUG_COM_RP("Trace::initLogStream() LEAVE (%d)", retVal);
	return retVal;
}

void Trace::finalizeLogStream()
{
	DEBUG_COM_RP("Trace::finalizeLogStream ENTER");
	SaAisErrorT Errt = SA_AIS_OK;

	if (OpenThreadId != 0)
	{
		// Stomp it to death.....quite brutally
		pthread_kill(OpenThreadId, SIGKILL);
	}

	TimerSelect::Instance().poll_unsetcallback(handle_ntf, theSelectionObject);

	/* Close the SA Forum log stream */
	LogFileIsOpen = false;

	LOG_API_CALL_TRY_AGAIN(saLogStreamClose(theLogStreamHandle));
	if (Errt != SA_AIS_OK)
	{
		DEBUG_COM_RP("saLogStreamClose failed handle: %llu, ret: %d", theLogStreamHandle, Errt);
	}

	/* Finalize use of the SA Forum logging utility */
	LOG_API_CALL_TRY_AGAIN(saLogFinalize(theLogHandle));
	if (Errt != SA_AIS_OK)
	{
		DEBUG_COM_RP("saLogFinalize failed handle: %llu, ret: %d", theLogHandle, Errt);
	}

	if (identityString != NULL)
	{
		free(identityString);
		identityString = NULL;
	}

	DEBUG_COM_RP("Trace::finalizeLogStream LEAVE");
}

SaLogSeverityFlagsT Trace::getLogSeverityFlags() {
	return logSevFlags;
}

bool Trace::start()
{
	bool ret = true;

	ret = initLogStream();

	if(ret)
		DEBUG_COM_RP("Trace::start(): Trace service started successfully!");

	char* enableDebugs = getenv("ENABLE_DEBUGS");
	if (enableDebugs != NULL && (0 == strcmp(enableDebugs, "1"))) {
		debugsEnabled = true;
	}

	return ret;
}

bool Trace::stop()
{
	bool retVal = true;

	finalizeLogStream();
	stop_thread=1;

	DEBUG_COM_RP("Trace::stop(): Trace service stopped!");
	return retVal;
}

void traceWrite(int priority, const char* fmt, ...)
{
	if (pthread_mutex_lock(&_traceMutexLock) != 0) {
		return;
	}

	va_list arg;
	va_start(arg, fmt);

	Trace::Instance().comSaCfgLogStreamWrite((MwSpiSeverityT)priority, fmt, arg);
	va_end(arg);

	pthread_mutex_unlock(&_traceMutexLock);
}
