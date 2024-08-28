/*
 * SA_Defines.h
 *
 *  Created on: Jan 10, 2019
 *      Author: xsanman
 */

#ifndef SRC_GENERIC_INCL_SA_DEFINES_H_
#define SRC_GENERIC_INCL_SA_DEFINES_H_

#include "MafMwSpiLog_1.h"
#include "saLog.h"

//log api version
#define logReleaseCode 'A'
#define logMajorVersion 2
#define logMinorVersion 3

#define IMM_MAX_RETRIES         3
#define USLEEP_HALF_SECOND      500000
#define NANOSLEEP_SECONDS       60

#define SA_LOG_SEV_FLAG_ALL      0x007F
#define SALOG_SEV_FLAG_INFO_OFF  0x003F//INFO log bit is not set
#define SALOG_SEV_FLAG_NONE      0x0000
#define SALOG_SEV_FLAG_DEFAULT   0x0007

//com config log streams
#define COM_LOG_STREAM_DN        "safLgStrCfg=ComCfgLogStream,safApp=safLogService"
#define LM_LOG_STREAM_DN        "safLgStrCfg=LmCfgLogStream,safApp=safLogService"
#define COMSA_LOG_STREAM_DN      "safLgStrCfg=ComSaCfgLogStream,safApp=safLogService"
#define LMSA_LOG_STREAM_DN      "safLgStrCfg=LmSaCfgLogStream,safApp=safLogService"
#define ALARM_LOG_STREAM_DN      "safLgStrCfg=FaultManagementCfgLogAlarmStream,safApp=safLogService"
#define ALERT_LOG_STREAM_DN      "safLgStrCfg=FaultManagementCfgLogAlertStream,safApp=safLogService"
#define CMD_LOG_STREAM_DN        "safLgStrCfg=ComCliCmdLogStream,safApp=safLogService"
#define SECURITY_LOG_STREAM_DN   "safLgStrCfg=ComSecLogStream,safApp=safLogService"
#define ALARM_LOGM_DN            "logId=FaultManagementCfgLogAlarmStream,CmwLogMlogMId=1"
#define ALERT_LOGM_DN            "logId=FaultManagementCfgLogAlertStream,CmwLogMlogMId=1"
#define SALOG_LOGM_DN            "logId=saLogSystem,CmwLogMlogMId=1"
#define COM_LOGM_DN              "logId=ComCfgLogStream,CmwLogMlogMId=1"
#define LM_LOGM_DN               "logId=LmCfgLogStream,CmwLogMlogMId=1"
#define COMSA_LOGM_DN            "logId=ComSaCfgLogStream,CmwLogMlogMId=1"
#define LMSA_LOGM_DN             "logId=LmSaCfgLogStream,CmwLogMlogMId=1"
#define CMD_LOGM_DN              "logId=ComCliCmdLogStream,CmwLogMlogMId=1"
#define SECURITY_LOGM_DN         "logId=ComSecLogStream,CmwLogMlogMId=1"

//config log stream class attributes
#define LOG_STREAM_FILENAME_PARAM           "saLogStreamFileName"
#define LOG_STREAM_MAX_FILE_SIZE_PARAM      "saLogStreamMaxLogFileSize"
#define LOG_STREAM_MAX_FILE_ROTATED_PARAM   "saLogStreamMaxFilesRotated"
#define LOG_STREAM_FIXED_LOG_REC_SIZE_PARAM "saLogStreamFixedLogRecordSize"
#define LOGM_LOG_SEVERITYFILTER             "severityFilter"

#define SECURITY_AUTHORIZATION_MESSAGE_1     4
#define SECURITY_AUTHORIZATION_MESSAGE_2     10
#define LOG_AUDIT_MESSAGE                    13
#define LOG_AUDIT_MESSAGE_LOCAL1             17
#define ALARM_LOG                            100
#define ALERT_LOG                            101
#define COMMAND_LOG                          300
#define SECURITY_LOG                         500
#define COMCFG_LOG                           501
#define SALOGSYSTEM_LOG                      502

// default values for log file attributes for alarms
#define DEFAULT_ALARM_LOG_FILE_NAME          "FmAlarmLog"
#define DEFAULT_ALARM_LOG_FILE_PATH_NAME     "FaultManagementLog/alarm"
#define DEFAULT_ALARM_MAX_LOG_FILE_SIZE      500000
#define DEFAULT_ALARM_MAX_LOG_REC_SIZE       1024
#define DEFAULT_ALARM_HA_PROPERTY            SA_TRUE
#define DEFAULT_ALARM_LOG_FILE_FULL_ACTION   SA_LOG_FILE_FULL_ACTION_ROTATE
#define DEFAULT_ALARM_MAX_FILES_ROTATED      11
#define DEFAULT_ALARM_FORMAT_EXPRESSION      "@Cb"

// default values for log file attributes for alerts
#define DEFAULT_ALERT_LOG_FILE_NAME          "FmAlertLog"
#define DEFAULT_ALERT_LOG_FILE_PATH_NAME     "FaultManagementLog/alert"
#define DEFAULT_ALERT_MAX_LOG_FILE_SIZE      500000
#define DEFAULT_ALERT_MAX_LOG_REC_SIZE       1024
#define DEFAULT_ALERT_HA_PROPERTY            SA_TRUE
#define DEFAULT_ALERT_LOG_FILE_FULL_ACTION   SA_LOG_FILE_FULL_ACTION_ROTATE
#define DEFAULT_ALERT_MAX_FILES_ROTATED      11
#define DEFAULT_ALERT_FORMAT_EXPRESSION      "@Cb"

// default values for log file attributes for com cli commands
#define DEFAULT_CMD_LOG_FILE_NAME          "ComCliCommandLog"
#define DEFAULT_CMD_LOG_FILE_PATH_NAME     "COM"
#define DEFAULT_CMD_MAX_LOG_FILE_SIZE      5000000
#define DEFAULT_CMD_MAX_LOG_REC_SIZE       0
#define DEFAULT_CMD_HA_PROPERTY            SA_TRUE
#define DEFAULT_CMD_LOG_FILE_FULL_ACTION   SA_LOG_FILE_FULL_ACTION_ROTATE
#define DEFAULT_CMD_MAX_FILES_ROTATED      11
#define DEFAULT_CMD_FORMAT_EXPRESSION      "@Cb"



#define DEFAULT_ALARM_LOG_STREAM_NAME        "safLgStr=FaultManagementLogAlarmStream"
#define DEFAULT_ALERT_LOG_STREAM_NAME        "safLgStr=FaultManagementLogAlertStream"
#define AlarmLogFileName                     "AlarmLogFileName="
#define AlertLogFileName                     "AlertLogFileName="
#define CmdLogFileName                       "CommandLogFileName="
#define AlarmFilesRotated                    "AlarmFilesRotated="
#define AlertFilesRotated                    "AlertFilesRotated="
#define CmdFilesRotated                      "CommandFilesRotated="
#define AlarmMaxLogFileSize                  "AlarmMaxLogFileSize="
#define AlertMaxLogFileSize                  "AlertMaxLogFileSize="
#define CmdMaxLogFileSize                    "CommandMaxLogFileSize="
#define AlarmMaxLogRecordSize                "AlarmMaxLogRecordSize="
#define AlertMaxLogRecordSize                "AlertMaxLogRecordSize="
#define CmdMaxLogRecordSize                  "CommandMaxLogRecordSize="

#define AMF_LOG_MAX_TRIES                    4
#define AMF_LOG_TRY_INTERVAL                 25000

#define SAF_LOG_CONFIG                       "logConfig=1,safApp=safLogService"
#define SAF_LOG_PATH_ATTR_NAME               "logRootDirectory"

#define IMM_API_CALL(F) { \
		int count = 0; \
		do { \
			if (count++) { \
				usleep(USLEEP_HALF_SECOND); \
			} \
			rc = F; \
		} while (((SA_AIS_ERR_TRY_AGAIN == rc) || (SA_AIS_ERR_BUSY == rc)) && (count <= IMM_MAX_RETRIES)); \
}

#define LOG_API_CALL(f) \
  do { Errt = f; if (Errt == SA_AIS_ERR_TRY_AGAIN) sleep(SleepTime);\
  } while (Errt == SA_AIS_ERR_TRY_AGAIN);


#define LOG_API_CALL_TRY_AGAIN(f) do {\
	unsigned int tryCount = 0;\
	for (Errt = f; Errt == SA_AIS_ERR_TRY_AGAIN && tryCount < AMF_LOG_MAX_TRIES; Errt = f, tryCount++){\
		usleep(AMF_LOG_TRY_INTERVAL);\
	}} while (0);

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE -1
#endif

/* the SPI designers have designed the MW_SA_ constant so that they
 * should numericaly match the Unix and SA_LOG values. However we
 * chose to decouple this to make it insensitive to future changes.
 * But *if* they match we want an efficient implementation.
 */

#if MW_SA_LOG_SEV_EMERGENCY != LOG_EMERG \
 || MW_SA_LOG_SEV_ALERT != LOG_ALERT \
 || MW_SA_LOG_SEV_CRITICAL != LOG_CRIT \
 || MW_SA_LOG_SEV_ERROR != LOG_ERR \
 || MW_SA_LOG_SEV_WARNING != LOG_WARNING \
 || MW_SA_LOG_SEV_NOTICE != LOG_NOTICE \
 || MW_SA_LOG_SEV_INFO != LOG_INFO

#warning Inefficient LOG-LEVEL translation
static inline int SYSLOG_LEVEL(s) {
	ENTER_MWSA_LOG();
	switch (s) {
	case MW_SA_LOG_SEV_EMERGENCY: return LOG_EMERG;
	case MW_SA_LOG_SEV_ALERT: return LOG_ALERT;
	case MW_SA_LOG_SEV_CRITICAL: return LOG_CRIT;
	case MW_SA_LOG_SEV_ERROR: return LOG_ERR;
	case MW_SA_LOG_SEV_WARNING: return LOG_WARNING;
	case MW_SA_LOG_SEV_NOTICE: return LOG_NOTICE;
	case MW_SA_LOG_SEV_INFO: return LOG_INFO;
	}
	LEAVE_MWSA_LOG();
	return 0;
}
#else
#define SYSLOG_LEVEL(s) (int)(s)
#endif

#if MW_SA_LOG_SEV_EMERGENCY != SA_LOG_SEV_EMERGENCY \
 || MW_SA_LOG_SEV_ALERT != SA_LOG_SEV_ALERT \
 || MW_SA_LOG_SEV_CRITICAL != SA_LOG_SEV_CRITICAL \
 || MW_SA_LOG_SEV_ERROR != SA_LOG_SEV_ERROR \
 || MW_SA_LOG_SEV_WARNING != SA_LOG_SEV_WARNING \
 || MW_SA_LOG_SEV_NOTICE != SA_LOG_SEV_NOTICE \
 || MW_SA_LOG_SEV_INFO != SA_LOG_SEV_INFO

#warning Inefficient Severity translation
static inline int SEVERITY(s) {
	switch (s) {
	case MW_SA_LOG_SEV_EMERGENCY: return SA_LOG_SEV_EMERGENCY;
	case MW_SA_LOG_SEV_ALERT: return SA_LOG_SEV_ALERT;
	case MW_SA_LOG_SEV_CRITICAL: return SA_LOG_SEV_CRITICAL;
	case MW_SA_LOG_SEV_ERROR: return SA_LOG_SEV_ERROR;
	case MW_SA_LOG_SEV_WARNING: return SA_LOG_SEV_WARNING;
	case MW_SA_LOG_SEV_NOTICE: return SA_LOG_SEV_NOTICE;
	case MW_SA_LOG_SEV_INFO: return SA_LOG_SEV_INFO;
	}
}

#else
#define SEVERITY(s) (int)(s)
#endif

static inline SaLogSeverityFlagsT SEVERITY_FLAG( MwSpiSeverityT s) {
	switch (s) {
	case MW_SA_LOG_SEV_EMERGENCY: return SA_LOG_SEV_FLAG_EMERGENCY;
	case MW_SA_LOG_SEV_ALERT: return SA_LOG_SEV_FLAG_ALERT;
	case MW_SA_LOG_SEV_CRITICAL: return SA_LOG_SEV_FLAG_CRITICAL;
	case MW_SA_LOG_SEV_ERROR: return SA_LOG_SEV_FLAG_ERROR;
	case MW_SA_LOG_SEV_WARNING: return SA_LOG_SEV_FLAG_WARNING;
	case MW_SA_LOG_SEV_NOTICE: return SA_LOG_SEV_FLAG_NOTICE;
	case MW_SA_LOG_SEV_INFO: return SA_LOG_SEV_FLAG_INFO;
	}
	return SA_LOG_SEV_FLAG_INFO;
}

#endif /* SRC_GENERIC_INCL_SA_DEFINES_H_ */
