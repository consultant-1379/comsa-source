#ifndef __REENCRYPTOR_DEFINES_H
#define __REENCRYPTOR_DEFINES_H

#include "MafMwSpiLog_1.h"
#include "saLog.h"

/* ImmUtil */
#define IMM_RELEASE_CODE 'A'
#define IMM_MAJOR_VERSION 2
#define IMM_MINOR_VERSION 15

#define USLEEP_HALF_SECOND 500000
#define USLEEP_ONE_SECOND 1000000
#define IMM_MAX_RETRIES 3
#define SA_MAX_NAME_LENGTH 256
#define ERROR_STRING_MAX_LENGTH 256

/* Trace */
#define TIMER_SIG (SIGRTMIN+1)
#define SECONDS_TO_SLEEP 20
#define NANOSECONDS_TO_SLEEP 0
#define NANOSLEEP_SECONDS 60

/* FileReaderUtil */
#define SEMI_COLON_DELIMITER ";"
#define COMMA_DELIMITER ","
#define MAX_FILE_ACCESS_RETRIES 120

/* SecUtil */
#define SEC_CRYPTO_OK 0 //Reference from sec/crypto_status.h
#define SEC_CRYPTO_LIB "libsec_crypto_api.so.1"

/* Reencryptor */
#define COM_APR "COM_APR_9010443"
#define SEC_ENCRYPTION_PARTICIPANTM_OBJECT "secEncryptionParticipantMId=1"
#define SEC_ENCRYPTION_PARTICIPANT_RDN_NAME "secEncryptionParticipantId"
#define SEC_ENCRYPTION_PARTICIPANT_RDN_VALUE SEC_ENCRYPTION_PARTICIPANT_RDN_NAME"="COM_APR
#define SEC_ENCRYPTION_PARTICIPANT_OBJECT SEC_ENCRYPTION_PARTICIPANT_RDN_VALUE","SEC_ENCRYPTION_PARTICIPANTM_OBJECT
#define ENCRYPTION_KEY_UUID "encryptionKeyUuid"
#define ENCRYPTION_STATUS "encryptionStatus"
#define ADDITIONAL_STATUS_INFO "additionalStatusInfo"

#define COM_REENCRYPTOR "COM_REENCRYPTOR"
#define COM_REENCRYPTOR_APPLIER "@"COM_REENCRYPTOR
#define SEC_ENCRYPTION_PARTICIPANTM "SecEncryptionParticipantM"
#define SEC_ENCRYPTION_PARTICIPANT "SecEncryptionParticipant"

/* LOGM */
#define COMSA_LOG_STREAM_DN		"safLgStrCfg=ComSaCfgLogStream,safApp=safLogService"
#define COMSA_LOGM_DN			"logId=ComSaCfgLogStream,CmwLogMlogMId=1"
#define LOGM_LOG_SEVERITYFILTER		"severityFilter"

#define SALOG_SEV_FLAG_INFO_OFF  0x003F //INFO log bit is not set
#define AMF_LOG_MAX_TRIES 4
#define AMF_LOG_TRY_INTERVAL 25000

#define logReleaseCode 'A'
#define logMajorVersion 2
#define logMinorVersion 3

static inline SaLogSeverityFlagsT SEVERITY_FLAG(MwSpiSeverityT s) {
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

#endif // __REENCRYPTOR_DEFINES_H
