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
 *   File:   ComSATraceService.c
 *
 *   Author: uablrek
 *
 *   Date:   2010-06-13
 *
 *========================================================================
 *   Date:   2010-09-07
 *   Author: egorped
 * 			Continued development adding
 * 			        - Group filtering
 * 				- Configuration file parsing
 *
 *=======================================================================
 *   Reviewed: efaiami 2010-08-17
 *             efaiami 2010-10-04
 *
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify: efaiami 2013-01-23  Implement SDP2150 - extended Group level 0
 *   Modify: xdonngu 2014-07-15  Using timer and signal handler for polling the trace configuration files
 *   Modify: xdonngu 2014-07-15  Using shared memory to transfer trace_flag value from comsa lib to pmtsa lib instead of polling "hard code path" file
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 */

#include "ComSA.h"
#include "MafMwSpiTrace_1.h"
#include "MafMwSpiTrace_2.h"
#include "MafMwSpiLog_1.h"
#include "MafMwSpiServiceIdentities_1.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <poll.h>
#include <saLog.h>
#include "SaInternalTrace.h"
#include "SelectTimer.h"
#include "imm_utils.h"
#include "LogEventProducer.h"
#include "LoggingEvent.h"

#ifndef UNIT_TEST
int trace_flag = 0;
// Use the implementation date for shared memory key
key_t shm_key = (key_t)20140715;
int segment_id = 0;
int *ptr_trace_flag = NULL;
#else
extern key_t shm_key;
extern int segment_id;
extern int *ptr_trace_flag;
extern int tryagainCount;
#endif


char const* configfile_name = NULL;
static pthread_mutex_t trace_lock = PTHREAD_MUTEX_INITIALIZER;
static char* compIdentityString = NULL;
static SaNameT              compCfgLoggerName;
static SaNameT              compCfgLogStreamName;
static SaLogHandleT         compCfgLogHandle = 0;
static SaLogStreamHandleT   compCfgLogStreamHandle = 0;
static SaSelectionObjectT   compCfgSelectionObject;
static pthread_t            compCfgOpenThreadId = 0;
static int                  compCfgLogFileIsOpen = FALSE;
static const unsigned int   SleepTime = 1;
static bool comCfglog_status=false;
#ifndef UNIT_TEST
static SaLogSeverityFlagsT  compLogSevFlags = SALOG_SEV_FLAG_INFO_OFF;
#else
SaLogSeverityFlagsT  compLogSevFlags = SALOG_SEV_FLAG_INFO_OFF;
#endif

#define LOCK() if (pthread_mutex_lock(&trace_lock) != 0) abort()
#define UNLOCK() if (pthread_mutex_unlock(&trace_lock) != 0) abort()

#define GLOBAL_TRACE_STR "COMSA_GLOBAL_TRACE"
#define GROUP_TRACE_STR  "COMSA_TRACE_GROUP_"
#define MAX_TRACE_GRP_NO 32

/*
 * Trace SPI v2 params
 */
#define MAX_SUBSYSTEM_NAME_LEN  64
#define MAX_SUBSYSTEM_NUM       256 //256 is recommended from SPI.


#define SUBSYSTEM_PREFIX               "SUBSYSTEM_"
#define SUBSYSTEM_PREFIX_STR           "SUBSYSTEM_PREFIX"
#define TRACE_SUBSYSTEM_LMSA_STR       "SUBSYSTEM_LMSA"
#define TRACE_SUBSYSTEM_LM_STR         "SUBSYSTEM_LMSERVER"
#define TRACE_SUBSYSTEM_MAF_STR        "SUBSYSTEM_MAF"
#define MAF_SUBSYSTEM_PREFIX           "Maf"
#define TRACE_FILE_NAME_LEN     1024
#define HOST_NAME_STR_LEN         64

#define LMSERVER_SUBSYSTEM_PREFIX      "LmServer::"
#define LMCLIENT_SUBSYSTEM_PREFIX      "LmClient::"

#define LM_TIMESTAMP_SIZE         23
#define MAX_TRACE_LEN           2048

#define SECONDS_TO_SLEEP 20
#define NANOSECONDS_TO_SLEEP 0

timer_t timerid;
#define TIMER_SIG (SIGRTMIN)

/*
 * File global variables
 *
 */
static uint32_t	current_mask = 0;		// Default is all trace statements turned off

pthread_t	config_thread_t;

/*
 * Required by Trace SPI v2
 */
typedef struct
{
    MafMwSpiTraceSubsystemHandle_2T subsystemHandle;
    char subsystem[MAX_SUBSYSTEM_NAME_LEN];
    uint32_t lmServerUse;
} subsystemMapT;

static subsystemMapT subsystemArray[MAX_SUBSYSTEM_NUM];

enum {
    SUBSYSTEM_LMSA_ID = 0,
    SUBSYSTEM_LM_ID   = 1,
    SUBSYSTEM_MAF_ID  = 2,
    SUBSYSTEM_MAX_ID
};

void pushComDebugLogLevelEvent();

static bool subsystemState[SUBSYSTEM_MAX_ID] = {0};
char const* lmsa_trace_config_relative_path = "/lmsa-apr9010528/etc/lm_sa_trace.conf";

char trace_config_file_full_name[TRACE_FILE_NAME_LEN] = {'\0'};

static const char* traceLevelStr[] =
{
    /*
     * system is unusable
     */
    "EMERG",
    /*
     * action must be taken immediately
     */
    "ALERT",
    /*
     * critical conditions
     */
    "CRITI",
    /*
     * error conditions
     */
    "ERROR",
    /*
     * warning conditions
     */
    "WARNI",
    /*
     * normal, but significant, condition
     */
    "NORMA",
    /*
     * informational message
     */
    "INFOR",
    /*
     * debug information with system-level scope (set of programs)
     * This level is not expected to be used by applications running on MAF
     * but it is included for easier mapping to LTTng.
     */
    "DSYST",
    /*
     * debug information with program-level scope (set of processes)
     */
    "DPROG",
    /*
    * debug information with process-level scope (set of modules)
    */
    "DPROC",
    /*
     * debug information with module (executable/library) scope (set of units)
     */
    "DMODU",
    /*
     * debug information with compilation unit scope (set of functions)
     */
    "DUNIT",
    /*
     * debug information with function-level scope
     */
    "DFUNC",
    /*
     * debug information with line-level scope
     */
    "DLINE",
    /*
     * debug-level lower than line
     */
    "DOTHE"
};


/*
 * Help functions
 */

static char const* get_timestamp(void)
{
	static time_t last_time = 0;
	time_t now = time(NULL);
	static char buf[32];
	if (last_time != now) {
		struct tm atm;
		strftime(buf, sizeof(buf), "%F %T", localtime_r(&now,&atm));
		last_time = now;
	}
	return buf;
}

static bool case_insensitive_compare(const char* ucstr, char* str)
{
	/* this is always invented again, wish that someone could put it in stdlib */
	bool ret_val = false;
	int	i;

	if (strlen(str) >= strlen(ucstr))
	{
		ret_val = true;
		for (i = 0; i < strlen(ucstr); i++)
		{
			if (toupper(str[i]) != ucstr[i])
			{
				ret_val = false;
				break;
			}
		}
	}
	return ret_val;
}

static char const* initTraceConfigFileName()
{
	FILE *pso_path_file;
	pso_path_file = fopen("/usr/share/ericsson/cba/pso/storage-paths/config", "r");
	if(pso_path_file != NULL)
	{
		fscanf(pso_path_file, "%s", trace_config_file_full_name);
		fclose(pso_path_file);
		sprintf(trace_config_file_full_name, "%s%s%s", trace_config_file_full_name, "/", lmsa_trace_config_relative_path);
		return (char const*)trace_config_file_full_name;
	}
	else
	{
		ERR_MWSA_TRACE("initTraceConfigFileName() unable to open \"/usr/share/ericsson/cba/pso/storage-paths/config\"");
		return NULL;
	}
}


static bool get_enabled_state(const char* buf)
{
	bool ret_val = false;

	char* cptr = strchr(buf,'=');
	if (cptr != NULL)
	{
		cptr++;
		ret_val = (toupper(*cptr) == 'E');		// OK, because in worst case it will point to the terminating '\0'
	}
	return ret_val;
}


static int get_trace_group_no(const char* buf)
{
	static const char trace_group_str[] = GROUP_TRACE_STR;
	static const int trace_group_str_len = sizeof(trace_group_str) / sizeof(trace_group_str[0]) - 1;

	int	ret_val = 0;
	int	i;
	if (strlen(buf) > trace_group_str_len)
	{
		for (i = 0; i < trace_group_str_len; i++)
		{
			if (toupper(buf[i]) != trace_group_str[i])
			{
				goto end_exit;
			}
		}
		if (isdigit(buf[trace_group_str_len]) &&
			isdigit(buf[trace_group_str_len+1]))
		{
			ret_val = (buf[trace_group_str_len] - '0') * 10 + (buf[trace_group_str_len+1] - '0');
			if (!get_enabled_state(buf)) // If not enabled, return 0 regardless
				ret_val = 0;
		}
	}
	end_exit:
	return ret_val;
}

/*
 * load subsystem status from configuration file
 */
static void getSubsystemStatus(char* buf)
{
	if (buf)
	{
		if (case_insensitive_compare(TRACE_SUBSYSTEM_LMSA_STR, buf))
		{
			subsystemState[SUBSYSTEM_LMSA_ID] = get_enabled_state(buf);
		}
		else if (case_insensitive_compare(TRACE_SUBSYSTEM_LM_STR, buf))
		{
			subsystemState[SUBSYSTEM_LM_ID] = get_enabled_state(buf);
		}
		else if (case_insensitive_compare(TRACE_SUBSYSTEM_MAF_STR, buf))
		{
			subsystemState[SUBSYSTEM_MAF_ID] = get_enabled_state(buf);
		}
		DEBUG_MWSA_TRACE ("subsystem_status, %d %d %d", subsystemState[SUBSYSTEM_LMSA_ID],
		                                    subsystemState[SUBSYSTEM_LM_ID],
		                                    subsystemState[SUBSYSTEM_MAF_ID]);
	}
}

static void load_new_configuration_comsa()
{
	FILE*		cfg_fp = NULL;
	uint32_t	local_mask = 0;
	if ((cfg_fp = fopen(configfile_name,"r")) != NULL)
	{
		char	buf[MAX_PATH_DATA_LENGTH];
		for (;;)
		{
			int	trace_grp_no = 0;
			if (fgets(buf,sizeof(buf),cfg_fp) == NULL)
				break;
			if ('#' == buf[0]) // Comment
				continue;
			if (case_insensitive_compare(GLOBAL_TRACE_STR,buf))
			{
				if (!get_enabled_state(buf))
				{
					local_mask = 0;     // Just in case this wasn't the first thing in the file
					break;              // Just leave, if global trace off, well it's off
				}
			}
			trace_grp_no = get_trace_group_no(buf);

			if (trace_grp_no >= 0 && trace_grp_no <= MAX_TRACE_GRP_NO)
			{
				local_mask |= 1 << (trace_grp_no);
				//DEBUG_MWSA_TRACE("load_new_configuration() called trace_grp_no=%d local_mask=%8.8x",trace_grp_no,local_mask);
			}
		}
		current_mask = local_mask;
		if (current_mask & ( 1 << (2-1)))
		{
			trace_flag = 1;
		}
		else
		{
			trace_flag = 0;
		}
		// update to shared memory
		if(ptr_trace_flag)
		{
			*ptr_trace_flag = trace_flag;
		}

		fclose(cfg_fp);
	}
	else
	{
		WARN_MWSA_TRACE("loading config [%s] failed", configfile_name);
	}
}

static void load_new_configuration_lmsa()
{
	FILE* cfg_fp = NULL;
	char  buf[512] = {'\0'};

	DEBUG_MWSA_TRACE("TRACE loadNewConfiguration called");

	if (configfile_name == NULL)
	{
		configfile_name = initTraceConfigFileName();
	}
	if ((cfg_fp = fopen(configfile_name,"r")) != NULL)
	{
		for (;;)
		{
			if (fgets(buf,sizeof(buf),cfg_fp) == NULL)
			{
				break;
			}
			else if ('#' == buf[0]) // Comment
			{}
			else if (case_insensitive_compare(SUBSYSTEM_PREFIX, buf))
			{
				getSubsystemStatus(buf);
			}
		}
		fclose(cfg_fp);
	}
	else
	{
		WARN_MWSA_TRACE("loading config [%s] failed: %d", configfile_name, errno);
	}

	if(subsystemState[SUBSYSTEM_LMSA_ID])
	{
		trace_flag = 1;
	}
	else
	{
		trace_flag = 0;
	}
	//NOTE: No need to update ptr_trace_flag in shared memory as LM doesn't use PMTSA library at present.
}

static void load_new_configuration()
{
	if (0 == strcmp((const char*)_CC_NAME, "lm"))
	{
		load_new_configuration_lmsa();
	}
	else
	{
		load_new_configuration_comsa();
	}
}

/*
 * Check if config file updated. If so. load and parse
 */
void *check_and_load_configuration(void* arg)
{
	static time_t time_last_changed = 0;

	char* cmd = NULL;
	asprintf(&cmd, "%s%s", _CC_NAME, "sa_pso trcconf");
	if (processOpen(cmd) != 0)
	{
		WARN_MWSA_TRACE("%s called failed", cmd);
	}
	free(cmd);

	if (0 == strcmp((const char*)CC_NAME, "Lm"))
	{
		if (configfile_name == NULL)
		{
			configfile_name = initTraceConfigFileName();
		}
	}
	struct stat filestat;

	if (configfile_name != NULL)
	{
		if (stat(configfile_name, &filestat) == 0)
		{
			if (filestat.st_mtime != time_last_changed)
			{
				time_last_changed = filestat.st_mtime;
				load_new_configuration();
			}
		}
		else
		{
			WARN_MWSA_TRACE("Stat:ing [%s] failed - errno %d", configfile_name, errno);
		}
	}
	else
	{
		WARN_MWSA_TRACE("configfile_name is NULL");
	}

	return NULL;
}


/**
 * thread function looking handling updates to the config file
 */
void* configFileLoadThread(void* arg)
{
	struct timespec sleep_time;
	struct timespec remaining_time;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

	for (;;)
	{
		sleep_time.tv_sec = SECONDS_TO_SLEEP;
		sleep_time.tv_nsec= NANOSECONDS_TO_SLEEP;
		int res;
		for (res = nanosleep(&sleep_time,&remaining_time); res == -1 && errno == EINTR;)
		{
			sleep_time.tv_sec 	= remaining_time.tv_sec;
			sleep_time.tv_nsec 	= remaining_time.tv_nsec;
			res = nanosleep(&sleep_time,&remaining_time);
		}
		if (0 == res)
		{
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			check_and_load_configuration(NULL);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		}
	}
	return NULL;
}

MafReturnT compCfgLogStreamWrite(MwSpiSeverityT severity, const char* databuffer, int comp_trace_flag){
	DEBUG_MWSA_LOG("compCfgLogStreamWrite(): ENTER");
	MafReturnT RetVal = MafOk;
	SaAisErrorT Errt;

	if ( severity > MW_SA_LOG_SEV_INFO){
		severity = MW_SA_LOG_SEV_INFO;
	}
	bool isLogAllowed = compLogSevFlags & SEVERITY_FLAG(severity);
	if(isLogAllowed || comp_trace_flag == 1){
		if (!compCfgLogFileIsOpen)
		{
			syslog(LOG_USER*8 + LOG_INFO, "%s",databuffer);
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
			LogRecord.logHeader.genericHdr.logSvcUsrName		= &compCfgLoggerName;
			LogRecord.logHeader.genericHdr.logSeverity = SEVERITY(severity);

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
			LOG_API_CALL(saLogWriteLogAsync(compCfgLogStreamHandle, 0, SA_LOG_RECORD_WRITE_ACK, &LogRecord));
			if (Errt != SA_AIS_OK)
			{
				WARN_MWSA_LOG("Failure after saLogWriteLog error = %d", Errt);
				RetVal = MafFailure;
			}
		}
	}
	return RetVal;
}

/*
 * Interface functions;
 */
static void coremw_traceWrite(uint32_t group, const char* traceMessageStr)
{
	//DEBUG_MWSA_TRACE("coremw_traceWrite() called group =%d mask=%8.8x message %s",group,current_mask, traceMessageStr);
	if( trace_flag == 0 || ((group >= 0) && (group <= MAX_TRACE_GRP_NO) && ((current_mask & (1 << group)) != 0)))
	{
		// Treat all trace/debug messages as INFO as there is no loglevel support for DEBUG/TRACE in CoreMW
		compCfgLogStreamWrite(SA_LOG_SEV_INFO,traceMessageStr,trace_flag);
	}
}

static void coremw_traceWriteEnter(
	uint32_t group, const char* funcName, const char* traceMessageStr)
{
	//DEBUG_MWSA_TRACE("coremw_traceWriteEnter() called group =%d mask=%8.8x message %s",group,current_mask, traceMessageStr);
	if( trace_flag == 0 || ((group >= 0) && (group <= MAX_TRACE_GRP_NO) && ((current_mask & (1 << group)) != 0)))
	{
		compCfgLogStreamWrite(SA_LOG_SEV_INFO,traceMessageStr,trace_flag);
	}
}

static void coremw_traceWriteExit(
	uint32_t group, const char* funcName, uint32_t exitCode, const char* traceMessageStr)
{
	//DEBUG_MWSA_TRACE("coremw_traceWriteExit() called group =%d mask=%8.8x message %s",group,current_mask, traceMessageStr);
	if( trace_flag == 0 || ((group >= 0) && (group <= MAX_TRACE_GRP_NO) && ((current_mask & (1 << group)) != 0)))
	{
		compCfgLogStreamWrite(SA_LOG_SEV_INFO,traceMessageStr,trace_flag);
	}

}

/*
 * Interface functions for trace SPI V2
 */
MafReturnT coremw_registerSubsystem(const char* subsystem, MafMwSpiTraceSubsystemHandle_2T* subsystemHandle);
void coremw_traceWrite_2(MafMwSpiTraceSubsystemHandle_2T subsystem,
                    MafMwSpiTraceLevelT level,
                    const char* file,
                    uint32_t line,
                    const char* func,
                    const char* format,
                    va_list args);

//Helper functions
char* getSubsystemInfo(MafMwSpiTraceSubsystemHandle_2T subsystemHandle, uint32_t *lmServerUse);
static bool isMAFSubsystemEnabled(char* subsystemName);
const char* getTraceLevelStr( MafMwSpiTraceLevelT level);

MafReturnT coremw_registerSubsystem(const char* subsystem,
                                    MafMwSpiTraceSubsystemHandle_2T* subsystemHandle)
{
	int i = 0;

	if((subsystem == NULL) || (subsystemHandle == NULL))
	{
		return MafInvalidArgument;
	}

	if(strlen(subsystem) >= MAX_SUBSYSTEM_NAME_LEN)
	{
		return MafInvalidArgument;
	}

	DEBUG_MWSA_TRACE("register subsystem %s", subsystem);

	for(i = 0; i < MAX_SUBSYSTEM_NUM; i++)
	{
		if (strcmp(subsystem, (const char*)subsystemArray[i].subsystem) == 0)
		{
			subsystemHandle->handle = subsystemArray[i].subsystemHandle.handle;
			return MafOk;
		}
	}

	for(i = 0; i < MAX_SUBSYSTEM_NUM; i++)
	{
		if (0 == subsystemArray[i].subsystemHandle.handle)
		{
			subsystemArray[i].subsystemHandle.handle = i+1;
			subsystemHandle->handle = i+1;
			strncpy((char*)subsystemArray[i].subsystem, subsystem, MAX_SUBSYSTEM_NAME_LEN);

			if((strncmp(subsystem, LMSERVER_SUBSYSTEM_PREFIX, strlen(LMSERVER_SUBSYSTEM_PREFIX)) == 0)
				|| (strncmp(subsystem, LMCLIENT_SUBSYSTEM_PREFIX, strlen(LMCLIENT_SUBSYSTEM_PREFIX)) == 0))
			{
				subsystemArray[i].lmServerUse = 1;
			}
			return MafOk;
		}
	}
	return MafNoResources;
}

void coremw_traceWrite_2(MafMwSpiTraceSubsystemHandle_2T subsystem,
                         MafMwSpiTraceLevelT level,
                         const char* file,
                         uint32_t line,
                         const char* func,
                         const char* format,
                         va_list args)
{
	char trace_string[MAX_TRACE_LEN]={'\0'};
	int prefix_len = 0;
	char* subsystemName = NULL;
	uint32_t isLmServerUse = 0;
	int comp_traceFlag = 0;  // this value is used for logging when LogM is not available


	subsystemName = getSubsystemInfo(subsystem, &isLmServerUse);
	if (subsystemName)
	{
		// it belongs to LM subsystem
		if (isLmServerUse)
		{
				comp_traceFlag = (int)subsystemState[SUBSYSTEM_LM_ID];
				char timestamp[LM_TIMESTAMP_SIZE + 1]= {'\0'};
				strncpy(timestamp,format,LM_TIMESTAMP_SIZE);
				format+=LM_TIMESTAMP_SIZE;
				sprintf(trace_string, "[%s %s %s %s %d %s]  ",
						timestamp,
						subsystemName,
						getTraceLevelStr(level),
						file,
						line,
						func);
				prefix_len = strlen(trace_string);
				vsnprintf(trace_string+prefix_len, MAX_TRACE_LEN-prefix_len, format, args);
		}
		// it belongs to MAF subsystem
		else
		{
				comp_traceFlag = (int)isMAFSubsystemEnabled(subsystemName);
				sprintf( trace_string, "[%s %s %s %s %u %s]  ",
						get_timestamp(),
						subsystemName,
						getTraceLevelStr(level),
						basename((char*)file),
						line,
						func);
				prefix_len = strlen(trace_string);
				vsnprintf(trace_string+prefix_len, MAX_TRACE_LEN-prefix_len, format, args);
		}

		if(trace_string[0]!='\0')
		{
			compCfgLogStreamWrite(level,trace_string, comp_traceFlag);
		}

	}
	else
	{
		WARN_MWSA_TRACE("subsys is not registered");
	}
}

/*
 * Get subsystem name by subsystem handle
 */
char* getSubsystemInfo(MafMwSpiTraceSubsystemHandle_2T subsystemHandle, uint32_t *lmServerUse)
{

	if( (subsystemHandle.handle > 0) &&
			(subsystemHandle.handle < (MAX_SUBSYSTEM_NUM+1) )   &&
			(subsystemHandle.handle == subsystemArray[subsystemHandle.handle-1].subsystemHandle.handle) )
	{
		*lmServerUse = subsystemArray[subsystemHandle.handle-1].lmServerUse;
		return (char*)subsystemArray[subsystemHandle.handle-1].subsystem;
	}
	else
		return NULL;
}

/*
 * Check if MAF subsystem enabled
 */
static bool isMAFSubsystemEnabled(char* subsystemName)
{
	if( (strncmp(subsystemName, MAF_SUBSYSTEM_PREFIX, strlen(MAF_SUBSYSTEM_PREFIX)) == 0)
			&& subsystemState[SUBSYSTEM_MAF_ID] )
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * Translate enum type level to string
 */
const char* getTraceLevelStr( MafMwSpiTraceLevelT level )
{
	if ((level <= MafMwSpiTraceDebug) && (level >= MafMwSpiTraceEmerg))
	{
		return traceLevelStr[level];
	}
	else
	{
		return "UNKNOWN";
	}
}

static void timerHandlerLoadConfigFile(int sig, siginfo_t *si, void *uc)
{
	pthread_attr_t attr;
	int rc = pthread_attr_init(&attr);
	if(rc != 0)
	{
		return;
	}
	rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(rc != 0)
	{
		rc = pthread_attr_destroy(&attr);
		return;
	}
	rc = pthread_create(&config_thread_t, &attr, check_and_load_configuration, NULL);
	if(rc != 0)
	{
		rc = pthread_attr_destroy(&attr);
		return;
	}
	pthread_attr_destroy(&attr);
}

/*
 * Start/Stop
 */

/*
 * This structure is accessed directly by the Log service if COM //DEBUG_MWSA_TRACE
 * is active (COM_LOGGING_LEVEL=8). So, keep it public for now.
 */

MafMwSpiTrace_1T maf_comSATrace_interface = {
	MafMwSpiTrace_1Id,
	coremw_traceWrite,
	coremw_traceWriteEnter,
	coremw_traceWriteExit
};

MafMwSpiTrace_2T maf_comSATrace_interface_2 = {
	MafMwSpiTrace_2Id,
	coremw_registerSubsystem,
	coremw_traceWrite_2,
};

MafMgmtSpiInterface_1T* maf_comSATraceInterface(void)
{
	DEBUG_MWSA_TRACE("maf_comSATraceInterface called...");
	return (MafMgmtSpiInterface_1T*)&maf_comSATrace_interface;
}

MafMgmtSpiInterface_1T* maf_comSATraceInterface_2(void)
{
	DEBUG_MWSA_TRACE("maf_comSATraceInterface_2 called...");
	return (MafMgmtSpiInterface_1T*)&maf_comSATrace_interface_2;
}


void pushComDebugLogLevelEvent(){
	MafStreamLoggingLevelChangeValue_2T event ;
	event.newLevel = compLogSevFlags;
	event.streamType = COMCFG_LOG;
	push_LogEventProducer(&event);
}

/*
 * Callbacks
 */

static void compLogFilterSetCallbackT(SaLogStreamHandleT logStreamHandle, SaLogSeverityFlagsT logSeverity)
{
	DEBUG_MWSA_LOG("compLogFilterSetCallbackT received with severity = %d and stream handle = %llu",logSeverity, logStreamHandle);
	compLogSevFlags = logSeverity;
	if (0 != strcmp((const char*)_CC_NAME, "lm")) {
		pushComDebugLogLevelEvent();
	}
}

#ifndef UNIT_TEST
static void compCfgLogWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
#else
void compCfgLogWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error)
#endif
{
	if (error != SA_AIS_OK){
		if(error==SA_AIS_ERR_TRY_AGAIN)
		{
		  if(comCfglog_status==false){
			 syslog(LOG_USER*8 + LOG_WARNING,"compCfglogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
			 comCfglog_status = true;
			 #ifdef UNIT_TEST
				 WARN_MWSA_LOG("compCfglogWriteLogCallbackT and error=%d", (int)error);
				 tryagainCount = tryagainCount+1;
			 #endif
		  }
		}
		else{
		  syslog(LOG_USER*8 + LOG_WARNING,"compCfglogWriteLogCallbackT,  invocation=%llu and error=%d", invocation, (int)error);
		  comCfglog_status = false;
		  #ifdef UNIT_TEST
				 WARN_MWSA_LOG("compCfglogWriteLogCallbackT and error=%d", (int)error);
		  #endif
		}
	}
}

static void compCfgfdCallbackFn(struct pollfd* pfd, void *ref)
{
	SaAisErrorT Errt;
	Errt = saLogDispatch(compCfgLogHandle, SA_DISPATCH_ALL);
	if (Errt != SA_AIS_OK)
		WARN_MWSA_LOG("saLogDispatch FAILED: %d", Errt);
}

static void* compCfgLogServiceOpenThread()
{
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;
	ENTER_MWSA_LOG();
	struct pollfd pfd;
	int   PthreadResult = 0;

	/* Set up logstream name */
	if(0 == strcmp((const char*)_CC_NAME, "lm")) {
		saNameSet((const char*)LM_LOG_STREAM_DN, &compCfgLogStreamName);
	}else{
		saNameSet((const char*)COM_LOG_STREAM_DN, &compCfgLogStreamName);
	}
	DEBUG_MWSA_LOG("LogServiceOpenThread Calling pthread_detach");
	if ((PthreadResult =pthread_detach(compCfgOpenThreadId)) != 0)
	{
		syslog(LOG_USER*8 + LOG_WARNING, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogService failed to detach thread ",PthreadResult);
	}

	DEBUG_MWSA_LOG("Executing LogServiceOpenThread");
	for (;Errt != SA_AIS_OK;)
	{
		DEBUG_MWSA_LOG("Calling saLogStreamOpen_2 Log stream name = %s length = %u", saNameGet(&compCfgLogStreamName), (unsigned) strlen(saNameGet(&compCfgLogStreamName)));
		LOG_API_CALL(saLogStreamOpen_2(
				compCfgLogHandle,
				&compCfgLogStreamName,
				(SaLogFileCreateAttributesT_2*)NULL,
				0,
				SA_TIME_ONE_MINUTE,
				&compCfgLogStreamHandle));
		DEBUG_MWSA_LOG("After saLogStreamOpen_2 Return value = %d, LogStreamHandle = %lld",Errt,compCfgLogStreamHandle);
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
		auditlogger(_CC_NAME,LOG_INFO,1,"security_audit","lmCfgLogService is successfully opened");
	} else{
		LOG_MWSA_LOG("ComCfgLogService is successfully opened");
	}
	pfd.events = POLLIN;
	pfd.fd = compCfgSelectionObject;
	if (poll_setcallback(comSASThandle_ntf, compCfgfdCallbackFn, pfd, NULL) != 0)
	{
		WARN_MWSA_LOG("poll_setcallback failed");
		Errt = MafFailure;
	}else{
		compCfgLogFileIsOpen = TRUE;
		compCfgOpenThreadId  = 0;
	}
	pthread_exit(&Errt);
	return NULL;
}

void setCompLogMSevFilter()
{
	SaImmHandleT immOmHandle = 0;
	SaImmAccessorHandleT accessorHandle = 0;
	SaAisErrorT Errt = SA_AIS_OK;

	DEBUG_MWSA_LOG("setCompLogMSevFilter");
	if((Errt = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &imm_version)) != SA_AIS_OK) {
		WARN_MWSA_LOG("setCompLogMSevFilter: saImmOmInitialize failed %s", getOpenSAFErrorString(Errt));
	} else {
		if((Errt = autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandle)) != SA_AIS_OK) {
			WARN_MWSA_LOG("setCompLogMSevFilter: saImmOmAccessorInitialize failed %s", getOpenSAFErrorString(Errt));
		} else {
			int32_t sevFilter = 0;
			if (0 == strcmp((const char*)_CC_NAME, "lm")) {
				Errt = getSeverityFilter(accessorHandle,(SaStringT)LM_LOGM_DN, &sevFilter);
			}else{
				Errt = getSeverityFilter(accessorHandle,(SaStringT)COM_LOGM_DN, &sevFilter);
			}
			if(Errt != SA_AIS_OK){
				DEBUG_MWSA_LOG("setCompLogMSevFilter: get severityFilter from salog instance is failed, ret = %d", Errt);
			} else {
				DEBUG_MWSA_LOG("setCompLogMSevFilter: severityFilter from salog instance is  %d", sevFilter);
				compLogSevFlags = sevFilter;
			}
		}
		om_imm_finalize(immOmHandle, accessorHandle);
	}
}

MafReturnT initCompCfgLogStream()
{
	SaVersionT  theVersion = { logReleaseCode, logMajorVersion, logMinorVersion};
	SaLogCallbacksT theCallBacks = {compLogFilterSetCallbackT, NULL, compCfgLogWriteLogCallbackT};
	SaAisErrorT Errt = SA_AIS_OK;
	MafReturnT  myRetVal = MafOk;
	int   PthreadResult = 0;
	/* Set up the logger name */
	if(0 == strcmp((const char*)_CC_NAME, "lm")) {   // this if for  LM
		compLogSevFlags = SALOG_SEV_FLAG_DEFAULT; // for LM the default value set is EMERG ALERT CRITICAL
		asprintf(&compIdentityString, "%s", "LM SA logger");
	}else{
		asprintf(&compIdentityString, "%s", "COM SA logger");

	}
	saNameSet(compIdentityString, &compCfgLoggerName);
		/* Open the SA Forum log service */
		LOG_API_CALL(saLogInitialize(&compCfgLogHandle ,&theCallBacks , &theVersion));

	if (Errt != SA_AIS_OK)
	{
		WARN_MWSA_LOG("Failure after saLogInitialize error = %d", Errt);
		DEBUG_MWSA_LOG("Version releasecode %d major %d minor %d", theVersion.releaseCode,theVersion.majorVersion,theVersion.minorVersion);
		myRetVal = MafFailure;
	}else{
		/* Get a selection object to catch the callbacks */
			LOG_API_CALL(saLogSelectionObjectGet(compCfgLogHandle ,&compCfgSelectionObject));
		if (Errt != SA_AIS_OK)
		{
			WARN_MWSA_LOG("Failure after saLogSelectionObjectGet error = %d", Errt);
			myRetVal = MafFailure;
		}else{
			// read severity from the log instance
			setCompLogMSevFilter();

			// Create the open thread
			DEBUG_MWSA_LOG("initCompCfgLogStream Calling pthread_create");

				if ((PthreadResult = pthread_create(&compCfgOpenThreadId,NULL, &compCfgLogServiceOpenThread, NULL)) != 0)
				{
					DEBUG_MWSA_LOG("initCompCfgLogStream pthread_create failed PthreadResult = %d",PthreadResult);
					syslog(LOG_USER*8 + LOG_ERR, "%s %s %d", _CC_NAME_UPPERCASE, "SA -- LogStream failed to create ",PthreadResult);
				}
		}
	}
	DEBUG_MWSA_LOG("initCompCfgLogStream LEAVE (%d)",myRetVal);
	LEAVE_MWSA_LOG();
	return myRetVal;
}

void finalizeCfgLogStream(){
	SaAisErrorT Errt = SA_AIS_OK;
	if (compCfgOpenThreadId != 0)
	{
		pthread_kill(compCfgOpenThreadId, SIGKILL);
	}

	poll_unsetcallback(comSASThandle_ntf, compCfgSelectionObject);

	/* Close the SA Forum log stream */
	compCfgLogFileIsOpen = FALSE;
	LOG_API_CALL_TRY_AGAIN(saLogStreamClose(compCfgLogStreamHandle));
	if (Errt != SA_AIS_OK)
	{
		DEBUG_MWSA_TRACE("saLogStreamClose failed handle: %llu, ret: %d", compCfgLogStreamHandle, Errt);
	} else {
		DEBUG_MWSA_TRACE("saLogStreamClose: %llu for compCfgLogStream ret: %d", compCfgLogStreamHandle, Errt);
		if (0 == strcmp((const char*)_CC_NAME, "lm")) {
			auditlogger(_CC_NAME,LOG_INFO,1,"security_audit","lmCfgLogService is successfully closed");
		}else{
			LOG_MWSA_LOG("ComCfgLogService is successfully closed");
		}
	}

	/* Finalize use of the SA Forum logging utility */
	LOG_API_CALL_TRY_AGAIN(saLogFinalize(compCfgLogHandle));
	if (Errt != SA_AIS_OK)
	{
		DEBUG_MWSA_TRACE("saLogFinalize failed handle: %llu, ret: %d", compCfgLogHandle, Errt);
	}

	if (compIdentityString != NULL)
	{
		free(compIdentityString);
		compIdentityString = NULL;
	}
}

MafReturnT comSATraceStart(xmlNode* cfg_file)
{
	// Start SA Internal tracing and Trace service
	initCompSaCfgLogStream();
	initCompCfgLogStream();
	DEBUG_MWSA_TRACE("comSATraceStart called...");

	configfile_name = coremw_xmlnode_contents(cfg_file);
	if (configfile_name == NULL) {
		WARN_MWSA_TRACE("No config-file specified - Trace disabled");
		return MafOk;
	}
	DEBUG_MWSA_TRACE("comSATraceStart config file name...%s", configfile_name);

	/* Allocate a shared memory segment. */
	segment_id = shmget(shm_key, sysconf(_SC_PAGESIZE), IPC_CREAT | 0666);
	if(segment_id < 0)
	{
		ERR_MWSA_TRACE("comSATraceStart(): can't get shared memory, segment_id = %d, errno: %d", segment_id, errno);
	}
	else
	{
		/* Attach the shared memory segment. */
		ptr_trace_flag = (int*) shmat(segment_id, NULL, 0);
		if(ptr_trace_flag == (int *) -1)
		{
			ERR_MWSA_TRACE("comSATraceStart(): can't attach to shared memory, segment_id = %d, errno: %d, pmtsa trace will be disabled!", segment_id, errno);
			ptr_trace_flag = NULL;
		}
	}

	check_and_load_configuration(NULL);

	// If get shared memory successful, setup a timer for updating shared value.
	if(ptr_trace_flag)
	{
		/* timer variables */
		struct sigevent sev;
		struct itimerspec its;
		sigset_t mask;
		struct sigaction sa;
		struct sigaction old_sa;
		/* Establish handler for timer signal */
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = timerHandlerLoadConfigFile;
		sigemptyset(&sa.sa_mask);
		if (sigaction(TIMER_SIG, &sa, &old_sa) == -1)
		{
			DEBUG_MWSA_TRACE("sigaction err");
		}
		if(SIG_DFL == old_sa.sa_handler)
		{
			/* Block timer signal temporarily */
			sigemptyset(&mask);
			sigaddset(&mask, TIMER_SIG);
			if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
			{
				DEBUG_MWSA_TRACE("sigprocmask lock error: %d", errno);
			}
			/* Create the timer */
			sev.sigev_notify = SIGEV_SIGNAL;
			sev.sigev_signo = TIMER_SIG;
			sev.sigev_value.sival_ptr = &timerid;
			if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
			{
				DEBUG_MWSA_TRACE("timer_create error: %d", errno);
			}
			/* Start the timer */
			its.it_value.tv_sec = SECONDS_TO_SLEEP;
			its.it_value.tv_nsec = NANOSECONDS_TO_SLEEP;
			its.it_interval.tv_sec = its.it_value.tv_sec;
			its.it_interval.tv_nsec = its.it_value.tv_nsec;
			if (timer_settime(timerid, 0, &its, NULL) == -1)
			{
				DEBUG_MWSA_TRACE("timer_settime error: %d", errno);
			}
			/* Unlock the timer signal, so that timer notification
			   can be delivered */
			if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
			{
				DEBUG_MWSA_TRACE("sigprocmask unlock error: %d", errno);
			}
		}
		else
		{
			ERR_MWSA_TRACE("TIMER: old action for signal (SIGRTMIN) (%d) is not SIG_DFL (0): %lld, can't register signal handler for pmtsa trace config polling, pmtsa trace will be disabled.",
					TIMER_SIG, (long long)old_sa.sa_handler);
			sigaction(TIMER_SIG, &old_sa, NULL);
		}
	}

	coremw_traceWrite(0, "====== TRACE (RE)STARTED ======");
	return MafOk;
}

/**
 * Start LMSA trace service.
 * This function is called when MAF start LMSA component.
 */
MafReturnT lmSATraceStart()
{
	int thread_ret_val;

	LOG_MWSA_TRACE("lmSATraceStart called...");

	initCompCfgLogStream();
	initCompSaCfgLogStream();

	/* init subsystem data structure */
	memset(subsystemArray, 0, sizeof(subsystemArray));

	check_and_load_configuration(NULL);

	if (( thread_ret_val = pthread_create(&config_thread_t,NULL, configFileLoadThread, NULL)) != 0)
	{
		WARN_MWSA_TRACE("Config file thread creation failed, [%d]", thread_ret_val);
		return MafPrepareFailed;
	}

	DEBUG_MWSA_TRACE("lmSATraceStart done...");

	return MafOk;
}

void comSATraceStop(void)
{
	LOG_MWSA_TRACE("comSATraceStop called...");
	finalizeCfgLogStream();
	finalizeCompSaCfgLogStream();
	configfile_name = NULL;

	if(ptr_trace_flag)
	{
		shmdt(ptr_trace_flag);
		timer_delete(timerid);
		// marked shared memory should be deallocated when no thread attach to it.
		shmctl(segment_id, IPC_RMID, 0);
	}
}

/**
 * Stop LMSA trace service.
 * This function is called when MAF terminate LMSA component.
 */
void lmSATraceStop(void)
{
	LOG_MWSA_TRACE("lmSATraceStop called...");

	finalizeCfgLogStream();
	finalizeCompSaCfgLogStream();

	if (configfile_name == NULL)
	{
		return;
	}

	int status = pthread_cancel(config_thread_t);
	if(status < 0)
	{
		LOG_MWSA_TRACE("pthread_kill failed");
	}
	else
	{
		LOG_MWSA_TRACE("pthread_kill successful");
	}

	status = pthread_join(config_thread_t, NULL);
	if(status < 0)
	{
		LOG_MWSA_TRACE("pthread_join failed");
	}
	else
	{
		LOG_MWSA_TRACE("pthread_join successful");
	}


}
