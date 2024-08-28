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
* File: PmtSaTrace.cxx
*
* Author: ejonajo 2011-09-01
*
*  * Reviewed: efaiami 2012-04-14
*
************************************************************************** */

#include "PmtSaTrace.hxx"
#include "PmSaSelectTimer.hxx"
#include "SA_Defines.h"

#include <cstring>
#include <libgen.h>

using namespace PmtSa;

static FILE *log_file_fp = 0;
static int log_file_size = 10485760;
static char log_fname[255];
static pthread_mutex_t log_write_mutex = PTHREAD_MUTEX_INITIALIZER;
static char* log_ident = NULL;
static int stop_thread = 0;
static bool trace_status = false;

static const char *prefix_name[] = {
	"EM", "AL", "CR", "ER",
	"WA", "NO", "IN", "TR",
	"T1", "T2", "T3", "T4",
	"T5", "T6", "T7", "T8",
	">>", "<<" };
/*
* Global variables
*/
struct log_state_T g_Log =
{
	LOG_LEVEL_INFO,             /* Everything but DEBUG logs */
	-1,                         /* All tags */
	LOG_MODE_STDERR
};

char*			PmtSaTrace::pmtSaIdentityString = NULL;
SaLogHandleT		PmtSaTrace::theLogHandle = 0;
SaLogStreamHandleT 	PmtSaTrace::theLogStreamHandle = 0;
SaNameT			PmtSaTrace::theLoggerName;
SaSelectionObjectT 	PmtSaTrace::theSelectionObject = 0;
SaNameT			PmtSaTrace::pmtSaLogStreamName;
pthread_t		PmtSaTrace::OpenThreadId = 0;
bool 			PmtSaTrace::LogFileIsOpen = false;
SaLogSeverityFlagsT 	PmtSaTrace::logSevFlags = SALOG_SEV_FLAG_INFO_OFF;
void* PmtSaTrace::handle_ntf = NULL;
int PmtSaTrace::ntf_event_pipe[2];
/**
* Initialize the logger.
*
* @param ident     service name
*/
void log_init (const char *ident)
{
	int str_size = strlen(ident)+1;
	str_size = (str_size > 6 ? str_size : 6);
	log_ident = (char *)malloc(str_size * sizeof(char));
	strcpy (log_ident, ident);
}


/**
* Set new trace state.
*
* @param state     new trace state
* @param state     output, old trace state
*/
void log_control(const struct log_state_T* state, struct log_state_T* old)
{
	if (old)
	*old = g_Log;
	g_Log = *state;
}

/**
* Send output to log file.
*
* @param logfile   path to the log file
*/
void log_to_file(const char* logfile)
{
	if (g_Log.mode & LOG_MODE_FILE)
	{
		if (log_file_fp != 0)
		{
			fclose(log_file_fp);
		}
		g_Log.mode &= ~LOG_MODE_FILE;
	}

	if (logfile)
	{
		g_Log.mode |= LOG_MODE_FILE;

		int str_size = strlen(logfile);
		strncpy( log_fname, logfile, str_size );

		log_file_fp = fopen ( log_fname, "a+");
		if (log_file_fp == 0)
		{
			fprintf (stderr, "Can't open logfile '%s': %s.\n", log_fname, strerror (errno));
		}

		if( getenv( "TRACE_LOG_FILE_SIZE" ) )
		{
			log_file_size = atoi( getenv( "TRACE_LOG_FILE_SIZE" ) );
		}
	}
}

/**
* Checks the file size of the log.
*
* @return The file size of log_fname.
*/
int64_t get_log_size( void )
{
	struct stat file_status;

	if( stat( log_fname, &file_status ) != 0 )
	{
		fprintf (stderr, "Couldn't stat logfile '%s': %s.\n", log_fname, strerror (errno));
	}

	return (int64_t)file_status.st_size;
}

/**
* Rotate's the log file.
*/
void rotate_log( void )
{
	char old_log_fname[255];

	fclose(log_file_fp);

	strncpy( old_log_fname, log_fname, strlen(log_fname) );
	old_log_fname[strlen(log_fname)] = '\0';
	strncat( old_log_fname, ".1", 2 );
	rename( log_fname, old_log_fname );

	log_file_fp = fopen ( log_fname, "a+");
	if (log_file_fp == 0)
	{
		fprintf (stderr, "Can't open logfile '%s': %s.\n", log_fname, strerror (errno));
	}
}

/**
*
*
* @param file
* @param line
* @param priority
* @param format
* @param ap
*/
static void _log_printf (const char *file, int line, int level, int category, const char *format, va_list ap)
{
	char newstring[4096];
	char log_string[4096];
	char char_time[512];
	struct timeval tv;
	int num_char = 0;
	int i = 0;

	/* lock mutex */
	pthread_mutex_lock( &log_write_mutex );

	/* Rotate the log if the file size is over 10M */
	if( (g_Log.mode & LOG_MODE_FILE) && log_file_fp != 0 && log_file_size < get_log_size() )
	{
		/*fprintf(stderr, "Rotating log\n" );*/
		rotate_log();
	}

	if (((g_Log.mode & LOG_MODE_FILE) || (g_Log.mode & LOG_MODE_STDERR) || (g_Log.mode & LOG_MODE_STDOUT)) &&
			(g_Log.mode & LOG_MODE_TIMESTAMP))
	{
		gettimeofday (&tv, NULL);
        time_t t = time(0);
        struct tm *now = localtime(&t);

        char usec[10];
        sprintf(usec, "%06li", (long int)tv.tv_usec);
        char usec_str[4];
        memcpy(usec_str, usec, 3);
        usec_str[3]= '\0';

        unsigned int absTzOffset = (unsigned int)((now->tm_gmtoff>0 ? now->tm_gmtoff : -now->tm_gmtoff)/60);
		strftime (char_time, sizeof (char_time), "%Y-%m-%dT%H:%M:%S",localtime (&tv.tv_sec));
		num_char = sprintf (newstring, "%s.%s%c%02d:%02d ", char_time,usec_str,
		        		(unsigned char)((now->tm_gmtoff >= 0)?'+':'-'),
		        		(unsigned char)(absTzOffset / 60),// offset in hours
		        		(unsigned char)(absTzOffset % 60));// offset in minutes
	}

	/* Remove the path to the file. */
	char *fname = (char*)strrchr( file, '/' );
	if( !fname )
	{
		fname = (char*)file;
	} else
	{
		fname++;
	}

	if ( (level == LOG_LEVEL_DEBUG) || (g_Log.mode & LOG_MODE_FILELINE) )
	{
		i = sprintf( &newstring[ num_char ], "[%-5s] %20s:%-4d ",
		log_ident, fname, line );
		num_char += i;
		sprintf(&newstring[ num_char ], "%s : %s",
		prefix_name[ level + category ], format);
	} else
	{
		i = sprintf( &newstring[num_char], "[%-5s] ",
		log_ident );
		num_char += i;
		sprintf( &newstring[num_char], "%s : %s",
		prefix_name[ level + category ], format);
	}
	vsprintf (log_string, newstring, ap);

	/*
	* Output the log data
	*/
	if ((g_Log.mode & LOG_MODE_FILE) && log_file_fp != 0)
	{
		fprintf (log_file_fp, "%s\n", log_string);
		fflush (log_file_fp);
	}
	if (g_Log.mode & LOG_MODE_STDERR)
	{
		fprintf (stderr, "%s\n", log_string);
		fflush (stderr);
	}
	if (g_Log.mode & LOG_MODE_STDOUT)
	{
		printf("%s\n", log_string);
	}

	if ((g_Log.mode & LOG_MODE_SYSLOG) || ( level != LOG_LEVEL_DEBUG && level != LOG_LEVEL_INFO ) )
	{
		syslog (level, "%s", &log_string[num_char]);
	}

	/* unlock mutex */
	pthread_mutex_unlock( &log_write_mutex );
}

void enter_leave_log_print(const char *file, int line, int priority, int category, const char *format, ...)
{
	if(trace_flag == 1)
	{
		va_list ap;
		va_start (ap, format);
		_log_printf (file, line, priority, category, format, ap);
		va_end(ap);
	}
}

char* _createString(char * buffer, char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buffer, TRACE_BUFFER_SIZE_PMTSA - 1, fmt, ap);
	buffer[TRACE_BUFFER_SIZE_PMTSA - 1] = 0;
	va_end(ap);
	return buffer;
}
void PmtSaTrace::logFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	PMTSA_DEBUG("PmtSaTrace logFilterSetCallbackT received with severity = %d and stream handle = %llu", logSeverity, logStreamHandle);
	logSevFlags = logSeverity;
}

void PmtSaTrace::logWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
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

void PmtSaTrace::fdCallbackFn(struct pollfd* pfd, void *ref)
{
	SaAisErrorT Errt;
	Errt = saLogDispatch(theLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK) {
		PMTSA_WARN("PMT-SA saLogDispatch FAILED: %d", Errt);
	}
}

void* PmtSaTrace::LogServiceOpenThread(void * arg)
{
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;

	struct pollfd pfd;
	int   PthreadResult = 0;

	/* Set up logstream name */
	saNameSet((const char*)COMSA_LOG_STREAM_DN, &pmtSaLogStreamName);

	PMTSA_DEBUG("PmtSaTrace::LogServiceOpenThread calling pthread_detach");
	if ((PthreadResult = pthread_detach(OpenThreadId)) != 0)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %d", "PMT-SA -- LogService failed to detach thread", PthreadResult);
	}

	PMTSA_DEBUG("PmtSaTrace::Executing LogServiceOpenThread");
	for (;Errt != SA_AIS_OK;)
	{
		LOG_API_CALL(saLogStreamOpen_2(
					 theLogHandle,
					 &pmtSaLogStreamName,
					 (SaLogFileCreateAttributesT_2*)NULL,
					 0,
					 SA_TIME_ONE_MINUTE,
					 &theLogStreamHandle));
		PMTSA_DEBUG("PmtSaTrace::After saLogStreamOpen_2 Return value = %d, LogStreamHandle = %lld", Errt, theLogStreamHandle);
		if (Errt == SA_AIS_ERR_TIMEOUT)
		{
			syslog(LOG_USER*8 + LOG_WARNING, "%s", "PMT-SA -- Call to saLogStreamOpen_2 returned time-out. Retrying");
		}
		else if (Errt == SA_AIS_ERR_UNAVAILABLE)
		{
			// If not currently a member of the cluster, we might become later so sleep a while and try again
			syslog(LOG_USER*8 + LOG_WARNING, "%s", "PMT-SA -- Call to saLogStreamOpen_2 returned SA_AIS_ERR_UNAVAILABLE. Retrying");
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
						syslog(LOG_USER*8 + LOG_ERR, "%s %d %s", "PMT-SA -- Call to nanosleep returned", errno, "exiting");
						pthread_exit(&Errt);
						return NULL;
					}
				}
			}
		}
	} // end for

	pfd.events = POLLIN;
	pfd.fd = theSelectionObject;

	handle_ntf = PmSaSelectTimer::Instance().timerCreateHandle_r();
	PmSaSelectTimer::Instance().poll_maxfd(handle_ntf, 2);

	if (PmSaSelectTimer::Instance().poll_setcallback(handle_ntf, fdCallbackFn, pfd, NULL) != 0)
	{
		PMTSA_WARN("PmtSaTrace::poll_setcallback failed");
		pthread_exit(&Errt);
		return NULL;
	}
	LogFileIsOpen = true;
	OpenThreadId  = 0;
	if (pipe(ntf_event_pipe) != 0) {
		PMTSA_ERR("PmtSaTrace::Failed to create pipe");
		pthread_exit(&Errt);
		return NULL;
	} else {
		/* Set the read-end of the pipe in non-blocking mode */
		int rc = fcntl(ntf_event_pipe[0], F_GETFL);
		if (rc < 0) abort();
		if (fcntl(ntf_event_pipe[0], F_SETFL, rc | O_NONBLOCK)) abort();
	}
	while (!stop_thread) {
		if (PmSaSelectTimer::Instance().poll_execute(handle_ntf) != 0)
			PMTSA_ERR("Drop out of the poll-loop for pmtsa logstream");

	}
	pthread_exit(&Errt);
	return NULL;
}

bool PmtSaTrace::comSaCfgLogStreamWrite(MwSpiSeverityT severity, const char* buffer)
{
	bool retVal = true;
	SaAisErrorT Errt;

	if ( severity > MW_SA_LOG_SEV_INFO){
		severity = MW_SA_LOG_SEV_INFO;
	}

	bool isLogAllowed = logSevFlags & SEVERITY_FLAG(severity);

	if(isLogAllowed || trace_flag == 1)
	{
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
			LogRecord.logHeader.genericHdr.logSvcUsrName = &theLoggerName;
			LogRecord.logHeader.genericHdr.logSeverity = SEVERITY(severity);

			LogBuffer.logBuf = (unsigned char*)buffer;
			LogBuffer.logBufSize = strlen(buffer);
			LogRecord.logBuffer = &LogBuffer;
			LOG_API_CALL(saLogWriteLogAsync(theLogStreamHandle, 0, SA_LOG_RECORD_WRITE_ACK, &LogRecord));
			if (Errt != SA_AIS_OK)
			{
				syslog(LOG_USER*8 + LOG_WARNING, "PmtSaTrace::comSaCfgLogStreamWrite(): Failure after saLogWriteLog error = %d",Errt);
				retVal = false;
			}
		}
	}
	return retVal;
}
SaAisErrorT PmtSaTrace::getSeverityFilter(SaImmAccessorHandleT accessorHandle, char *dnName, int32_t* severityFilter){
	SaAisErrorT ret = SA_AIS_OK;
	*severityFilter = 0;
	if(accessorHandle != 0){
		SaImmAttrNameT attributeName[2];
		attributeName[0] = (SaStringT)LOGM_LOG_SEVERITYFILTER;
		attributeName[1] = (char *)'\0';
		SaImmAttrValuesT_2** attributeValue;
		SaNameT dn;
		saNameSet(dnName, &dn);
		if((ret = autoRetry_saImmOmAccessorGet_2(accessorHandle, &dn, attributeName, &attributeValue)) != SA_AIS_OK) {
			PMTSA_WARN("PmtSaTrace::getSeverityFilter(): autoRetry_saImmOmAccessorGet_2 failed %s", getOpenSAFErrorString(ret));
		}
		else{
			PMTSA_DEBUG("PmtSaTrace::autoRetry_saImmOmAccessorGet_2 : SUCCESS");
			SaImmAttrValuesT_2* oneAttrValue = attributeValue[0];
			if (strcmp(oneAttrValue->attrName, (char*)LOGM_LOG_SEVERITYFILTER)){
				PMTSA_WARN("PmtSaTrace::getSeverityFilter(): Expected attributeName returned by autoRetry_saImmOmAccessorGet_2() to be %s. Received: %s", (char*)LOGM_LOG_SEVERITYFILTER, oneAttrValue->attrName);
				ret = SA_AIS_ERR_INVALID_PARAM;
			}
			else{
				if (oneAttrValue->attrValueType != SA_IMM_ATTR_SAINT32T){
					PMTSA_WARN("PmtSaTrace::getSeverityFilter(): Expected attrValueType returned by autoRetry_saImmOmAccessorGet_2() to be %d. Received: %d", SA_IMM_ATTR_SAINT32T, oneAttrValue->attrValueType);
					ret = SA_AIS_ERR_INVALID_PARAM;
				}
				else{
					unsigned int i = 0;
					for(; i < oneAttrValue->attrValuesNumber; i++){
						*severityFilter = *severityFilter | SEVERITY_FLAG(*((SaInt32T *)oneAttrValue->attrValues[i]));
					}
					PMTSA_DEBUG("PmtSaTrace::getSeverityFilter() found attribute,value: %d.", *severityFilter);
				}
			}
		}
	}else{
		PMTSA_DEBUG("PmtSaTrace::getSeverityFilter(): accessorHandle is zero");
		ret = SA_AIS_ERR_BAD_HANDLE;
	}
	return ret;
}

bool PmtSaTrace::initLogStream()
{
	bool retVal = true;
	SaVersionT 		theVersion = {logReleaseCode, logMajorVersion, logMinorVersion};
	SaLogCallbacksT	theCallBacks = {logFilterSetCallbackT, NULL, logWriteLogCallbackT};
	SaAisErrorT 	Errt = SA_AIS_OK;
	int				pThreadResult = 0;

	PMTSA_DEBUG("PmtSaTrace::initLogStream(): ENTER");

	// Create the identity string for Re-encryption Participant
	asprintf(&pmtSaIdentityString, "%s", "PMT-SA logger");
	saNameSet(pmtSaIdentityString, &theLoggerName);

	/* Open the SA Forum log service */
	LOG_API_CALL(saLogInitialize(&theLogHandle, &theCallBacks , &theVersion));

	if (Errt != SA_AIS_OK) {
		PMTSA_WARN("PmtSaTrace::initLogStream(): Failure after saLogInitialize error = %d", Errt);
		retVal = false;
	} else {
	    /* Get a selection object to catch the callbacks */
	    LOG_API_CALL(saLogSelectionObjectGet(theLogHandle ,&theSelectionObject));
	    if (Errt != SA_AIS_OK) {
		    PMTSA_WARN("PmtSaTrace::initLogStream(): Failure after saLogSelectionObjectGet error = %d", Errt);
		    retVal = false;
	    } else {
	        setComSaLogSevFilter();

	        // Create the open thread
	        PMTSA_DEBUG("PmtSaTrace::initLogStream(): Calling pthread_create");
	        if ((pThreadResult = pthread_create(&OpenThreadId,NULL, &LogServiceOpenThread, NULL)) != 0) {
	            PMTSA_WARN("PmtSaTrace::initLogStream() pthread_create failed pThreadResult = %d", pThreadResult);
	        }
	    }
	}
	PMTSA_DEBUG("PmtSaTrace::initLogStream(): LEAVE (%d)", retVal);
	return retVal;
}

void PmtSaTrace::setComSaLogSevFilter()
{
	SaAisErrorT    Errt = SA_AIS_OK;
	SaImmHandleT   immOmHandle = 0;
	SaImmAccessorHandleT accessorHandle = 0;

	PMTSA_DEBUG("PmtSaTrace::setComSaLogSevFilter()");
	if((Errt = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &imm_version)) != SA_AIS_OK) {
		PMTSA_WARN("PmtSaTrace::setComSaLogSevFilter(): saImmOmInitialize failed %s", getOpenSAFErrorString(Errt));
	} else {
		if((Errt = autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandle)) != SA_AIS_OK) {
			PMTSA_WARN("PmtSaTrace::setComSaLogSevFilter(): saImmOmAccessorInitialize failed %s", getOpenSAFErrorString(Errt));
		} else {
			int32_t sevFilter = 0;
			Errt = getSeverityFilter(accessorHandle,(SaStringT)COMSA_LOGM_DN, &sevFilter);
			if(Errt != SA_AIS_OK){
				PMTSA_DEBUG("PmtSaTrace::setComSaLogSevFilter(): get severityFilter from ComSa log instance is failed, ret = %d", Errt);
			} else {
				PMTSA_DEBUG("PmtSaTrace::setComSaLogSevFilter(): severityFilter from ComSa log instance is  %d", sevFilter);
				logSevFlags = sevFilter;
			}
		}
		om_imm_finalize(immOmHandle, accessorHandle);
	}
}

void PmtSaTrace::finalizeLogStream()
{
	PMTSA_DEBUG("PmtSaTrace::finalizeLogStream() ENTER");
	SaAisErrorT Errt = SA_AIS_OK;

        stop_thread = 1;
	if (OpenThreadId != 0)
	{
		// Stomp it to death.....quite brutally
		pthread_kill(OpenThreadId, SIGKILL);
	}

	PmSaSelectTimer::Instance().poll_unsetcallback(handle_ntf, theSelectionObject);

	/* Close the SA Forum log stream */
	LogFileIsOpen = false;

	LOG_API_CALL_TRY_AGAIN(saLogStreamClose(theLogStreamHandle));
	if (Errt != SA_AIS_OK)
	{
		PMTSA_DEBUG("PmtSaTrace::saLogStreamClose(): failed handle: %llu, ret: %d", theLogStreamHandle, Errt);
	}

	/* Finalize use of the SA Forum logging utility */
	LOG_API_CALL_TRY_AGAIN(saLogFinalize(theLogHandle));
	if (Errt != SA_AIS_OK)
	{
		PMTSA_DEBUG("PmtSaTrace::saLogFinalize(): failed handle: %llu, ret: %d", theLogHandle, Errt);
	}

	if (pmtSaIdentityString != NULL)
	{
		free(pmtSaIdentityString);
		pmtSaIdentityString = NULL;
	}

	PMTSA_DEBUG("PmtSaTrace::finalizeLogStream(): LEAVE");
}

void PmtSaTrace::vlog(int priority, const char* fileName, const int lineNumber, char const* format, va_list ap)
{
	// logging using Trace
	char trace_buffer[256];
	int len = strlen(LOG_PREFIX);
	strcpy(trace_buffer, LOG_PREFIX);
	trace_buffer[len] = ' ';
	len++;
	vsnprintf(trace_buffer+len, sizeof(trace_buffer)-len, format, ap);
	switch(priority)
	{
	case LOG_NOTICE:
		LOG_PMTSA(trace_buffer);
		break;
	case LOG_WARNING:
		WARN_PMTSA(trace_buffer);
		break;
	case LOG_ERR:
		ERR_PMTSA(trace_buffer);
		break;
	case LOG_DEBUG:
		DEBUG_PMTSA(trace_buffer);
		break;
	}
	PmtSaTrace::Instance().comSaCfgLogStreamWrite((MwSpiSeverityT)priority, (const char*) trace_buffer);
}

void PmtSaTrace::log(int priority, const char* fileName, const int lineNumber, char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	PmtSaTrace::vlog(priority, fileName, lineNumber, fmt, ap);
	va_end(ap);
}

void PmtSaTrace::debug_log(int priority, const char* fileName, const int lineNumber, char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	PmtSaTrace::vlog(priority, fileName, lineNumber, fmt, ap);
	va_end(ap);
}
