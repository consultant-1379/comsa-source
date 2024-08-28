#include <list>
#include <string>
#include <iostream>
#include <assert.h>
#include <gtest/gtest.h>
#include <assert.h>
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
#include <saLog.h>
#include <ComSALogService.h>
#include <SaInternalTrace.h>
#include <MafMwSpiServiceIdentities_1.h>
#include <ComSA.h>
#include <SelectTimer.h>
#include <imm_utils.h>

/* Log-to-Trace duplication if COM_LOGGING_LEVEL=8. */
#include <MafMwSpiTrace_1.h>

#define ALARM_LOG							100
#define ALERT_LOG							101

// default values for log file attributes for alarms
#define DEFAULT_ALARM_LOG_FILE_NAME "FmAlarmLog"
#define DEFAULT_ALARM_MAX_FILES_ROTATED 11
#define DEFAULT_ALARM_MAX_LOG_FILE_SIZE 500000

// default values for log file attributes for alerts
#define DEFAULT_ALERT_LOG_FILE_NAME "FmAlertLog"
#define DEFAULT_ALERT_MAX_FILES_ROTATED 11
#define DEFAULT_ALERT_MAX_LOG_FILE_SIZE 500000

#define DEFAULT_ALARM_LOG_FILE_PATH_NAME "FaultManagementLog/alarm"
#define DEFAULT_ALERT_LOG_FILE_PATH_NAME "FaultManagementLog/alert"

#define SAF_LOG_CONFIG "logConfig=1,safApp=safLogService"
#define SAF_LOG_PATH_ATTR_NAME "logRootDirectory"

const SaVersionT imm_version = { immReleaseCode, immMajorVersion, immMinorVersion };
void* comSASThandle = NULL;
void* comSASThandle_ntf = NULL;

extern "C" SaLogStreamHandleT alarmLogStreamHandle;
extern "C" int alarmLogFileIsOpen;
extern "C" SaNameT alarmLoggerName;
extern "C" int alertLogFileIsOpen;
extern "C" SaNameT alertLoggerName;
extern "C" SaLogStreamHandleT alertLogStreamHandle;
extern "C" int cmdLogFileIsOpen;
extern "C" SaNameT cmdLoggerName;
extern "C" SaLogStreamHandleT cmdLogStreamHandle;
extern "C" int secLogFileIsOpen;
extern "C" SaNameT secLoggerName;
extern "C" SaLogStreamHandleT secLogStreamHandle;
extern "C" int LogFileIsOpen;
extern "C" SaNameT theLoggerName;
extern "C" SaLogStreamHandleT theLogStreamHandle;

extern "C" MafReturnT logWrite (uint32_t eventId, MwSpiSeverityT severity, MwSpiFacilityT facility, const char* databuffer);

extern "C" void parseSaLogConfig(char* configText);
extern "C" int createSymLinkForAlarmsAndAlerts(const char *path, const char *directory, const char * linkDir, const char * linkName);
extern "C" MafReturnT findLogPathDirName(SaImmHandleT immOmHandle, char* dnName, char* attrName, char** filePath);
extern "C" SaAisErrorT readSafLogPath(char** attrVal);
extern "C" void readCfgLogStreamsInfoFromImm();
extern "C" void getLoggerInfo( MwSpiFacilityT facility,  MwSpiSeverityT* severity, int** isLogFileOpen, SaNameT **loggerName, SaLogStreamHandleT** logStreamHandle, bool* isLogAllowed);
extern "C" MafReturnT modifyCfgLogStreamInstances();
extern "C" MafReturnT maf_ComLogServiceOpen();
extern "C" MafReturnT initCompCfgLogStream();
extern "C" MafReturnT initCompSaCfgLogStream();
extern "C" MafReturnT cmdLogServiceOpen();
extern "C" MafReturnT secLogServiceOpen();
extern "C" void finalizeCfgLogStream();
extern "C" void finalizeCompSaCfgLogStream();
extern "C" MafReturnT cmdLogServiceClose();
extern "C" MafReturnT secLogServiceClose();
extern "C" void compCfgLogWriteLogCallbackT(SaInvocationT invocation, SaAisErrorT error);

extern bool updateAlarmLogFileNameAttr;
extern bool updateAlertLogFileNameAttr;
extern bool updateCmdLogFileNameAttr;
extern SaLogSeverityFlagsT alarmLogSevFlags;
extern SaLogSeverityFlagsT alertLogSevFlags;
extern SaLogSeverityFlagsT cmdLogSevFlags;
extern SaLogSeverityFlagsT secLogSevFlags;
extern SaLogSeverityFlagsT saSystemLogSevFlags;
extern SaLogSeverityFlagsT compLogSevFlags;
extern SaLogSeverityFlagsT comSaLogSevFlags;
extern char* _CC_NAME;
extern char* IdentityString;
int tryagainCount =0;

SaLogFileCreateAttributesT_2 alarmLogFileAttributes;
SaLogFileCreateAttributesT_2 alertLogFileAttributes;
SaLogFileCreateAttributesT_2 cmdLogFileAttributes;

SaAisErrorT autoRetry_saImmOmInitialize_returnValue;
SaAisErrorT autoRetry_saImmOmAccessorInitialize_returnValue;
SaAisErrorT autoRetry_saImmOmAccessorGet_2_returnValue;
SaAisErrorT autoRetry_saImmOmFinalize_returnValue;
SaImmAttrValuesT_2** attrValues_Returned = NULL;
typedef struct {
	SaUint16T length;
	SaUint8T value[256];
} TheSaNameT;

SaAisErrorT saImmOmAdminOwnerInitialize(SaImmHandleT immHandle,
	     const SaImmAdminOwnerNameT adminOwnerName,
	     SaBoolT releaseOwnershipOnFinalize, SaImmAdminOwnerHandleT *ownerHandle) {
	printf("----> saImmOmAdminOwnerInitialize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAdminOwnerSet(SaImmAdminOwnerHandleT ownerHandle,
		const SaNameT **objectNames, SaImmScopeT scope){
	printf("----> saImmOmAdminOwnerSet \n");
	return SA_AIS_OK;
}
SaAisErrorT saImmOmCcbInitialize(SaImmAdminOwnerHandleT ownerHandle,
		SaImmCcbFlagsT ccbFlags, SaImmCcbHandleT *ccbHandle){
	printf("----> saImmOmCcbInitialize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAdminOwnerRelease(SaImmAdminOwnerHandleT ownerHandle,
		 const SaNameT **objectNames, SaImmScopeT scope){
	printf("----> saImmOmAdminOwnerRelease \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbValidate(SaImmCcbHandleT ccbHandle){
	printf("----> saImmOmCcbValidate \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbApply(SaImmCcbHandleT ccbHandle){
	printf("----> saImmOmCcbApply \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbObjectModify_2(SaImmCcbHandleT ccbHandle,
				  const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods){
	printf("----> saImmOmCcbObjectModify_2 \n");
	return SA_AIS_OK;
}
SaImmAttrValueT* allocateUint32AttrValueArray(uint32_t value)
{
    SaImmAttrValueT* attrValues = (void**) malloc(1 * sizeof(void*));
    if(attrValues != NULL)
    {
        attrValues[0] = malloc(sizeof(uint32_t));
        *(uint32_t*) attrValues[0]   = value;
    }
    return attrValues;
}

SaImmAttrModificationT_2* allocateUint32AttrMod(const char* attrName, uint32_t value)
{
    SaImmAttrModificationT_2* modAttrs = (SaImmAttrModificationT_2*) malloc(sizeof(SaImmAttrModificationT_2));
    if(modAttrs != NULL){
        modAttrs->modType                   = SA_IMM_ATTR_VALUES_REPLACE;
        modAttrs->modAttr.attrName          = strdup(attrName);
        modAttrs->modAttr.attrValueType     = SA_IMM_ATTR_SAUINT32T;
        modAttrs->modAttr.attrValuesNumber  = 1;
        modAttrs->modAttr.attrValues        = allocateUint32AttrValueArray(value);
    }
    return modAttrs;
}

SaImmAttrValueT* allocateint32AttrValueArray(int32_t* value, int32_t numVal)
{
    SaImmAttrValueT* attrValues = (void**) malloc(numVal * sizeof(void*));
    if(attrValues != NULL)
    {
        for (unsigned int i = 0; i < numVal; i++)
        {
            attrValues[i] = malloc(sizeof(int32_t));
            *(int32_t*) attrValues[i]   = value[i];
        }
    }
    return attrValues;
}

SaImmAttrValuesT_2* allocateint32AttrValues(const char* attrName, int32_t *value, int32_t numValues)
{
	SaImmAttrValuesT_2* attrValues = (SaImmAttrValuesT_2*) malloc(sizeof(SaImmAttrValuesT_2));
	attrValues->attrName           = strdup(attrName);
	attrValues->attrValueType      = SA_IMM_ATTR_SAINT32T;
	attrValues->attrValuesNumber   = numValues;
	attrValues->attrValues         = allocateint32AttrValueArray(value, numValues);
	return attrValues;
}

SaImmAttrValueT* allocateUint64AttrValueArray(uint64_t value)
{
    SaImmAttrValueT* attrValues = (void**) malloc(1 * sizeof(void*));
    if(attrValues != NULL)
    {
        attrValues[0] = malloc(sizeof(uint64_t));
        *(uint64_t*) attrValues[0]   = value;
    }
    return attrValues;
}

SaImmAttrModificationT_2* allocateUint64AttrMod(const char* attrName, uint64_t value)
{
    SaImmAttrModificationT_2* modAttrs = (SaImmAttrModificationT_2*) malloc(sizeof(SaImmAttrModificationT_2));
    if(modAttrs != NULL){
        modAttrs->modType                   = SA_IMM_ATTR_VALUES_REPLACE;
        modAttrs->modAttr.attrName          = strdup(attrName);
        modAttrs->modAttr.attrValueType     = SA_IMM_ATTR_SAUINT64T;
        modAttrs->modAttr.attrValuesNumber  = 1;
        modAttrs->modAttr.attrValues        = allocateUint64AttrValueArray(value);
    }
    return modAttrs;
}

SaImmAttrValueT* allocateStringAttrValueArray(const char* value)
{
    SaImmAttrValueT* attrValues = (void**) malloc(1 * sizeof(void*));
    if(attrValues != NULL)
    {
        attrValues[0] = (void*) malloc(sizeof(char*));
        *(char**) attrValues[0] = strdup(value);
    }
    return attrValues;
}


SaImmAttrValuesT_2* allocateStringAttrValues(const char* attrName, const char* value)
{
	SaImmAttrValuesT_2* attrValues = (SaImmAttrValuesT_2*) malloc(sizeof(SaImmAttrValuesT_2));
	attrValues->attrName           = strdup(attrName);
	attrValues->attrValueType      = SA_IMM_ATTR_SASTRINGT;
	attrValues->attrValuesNumber   = 1;
	attrValues->attrValues         = allocateStringAttrValueArray(value);

	return attrValues;
}
SaImmAttrModificationT_2* allocateStringAttrMod(const char* attrName, const char* value)
{
    SaImmAttrModificationT_2* modAttrs = (SaImmAttrModificationT_2*) malloc(sizeof(SaImmAttrModificationT_2));
    if(modAttrs != NULL){
        modAttrs->modType                   = SA_IMM_ATTR_VALUES_REPLACE;
        modAttrs->modAttr.attrName          = strdup(attrName);
        modAttrs->modAttr.attrValueType     = SA_IMM_ATTR_SASTRINGT;
        modAttrs->modAttr.attrValuesNumber  = 1;
        modAttrs->modAttr.attrValues        = allocateStringAttrValueArray(value);
    }
    return modAttrs;
}

void freeAttrMod(SaImmAttrModificationT_2* attrMod)
{
    if(attrMod !=  NULL){
        unsigned int i = 0;
        for (i = 0; i < attrMod->modAttr.attrValuesNumber; i++)
        {
            if( attrMod->modAttr.attrValueType == SA_IMM_ATTR_SASTRINGT){
                free(*((char **)(attrMod->modAttr.attrValues[i])));
            }
            if(attrMod->modAttr.attrValues[i]!= NULL){
                free(attrMod->modAttr.attrValues[i]);
            }
        }
        if(attrMod->modAttr.attrValues != NULL){
            free(attrMod->modAttr.attrValues);
        }
        if(attrMod->modAttr.attrName != NULL){
            free(attrMod->modAttr.attrName);
        }
        free(attrMod);
    }
}

void freeAttrValues(SaImmAttrValuesT_2* attrValues)
{
	for (unsigned int i = 0; i < attrValues->attrValuesNumber; i++)
	{
		if(SA_IMM_ATTR_SASTRINGT == attrValues->attrValueType) {
			free(*(char**)attrValues->attrValues[i]);
		}
		free(attrValues->attrValues[i]);
		attrValues->attrValues[i] = NULL;
	}
	free(attrValues->attrValues);
	attrValues->attrValues = NULL;
	free(attrValues->attrName);
	attrValues->attrName= NULL;
	free(attrValues);
	attrValues = NULL;
}

void om_imm_finalize(SaImmHandleT immOmHandle, SaImmAccessorHandleT accessorHandle)
{
	//ENTER_IMM_OM();
	SaAisErrorT error = SA_AIS_OK;
	//DEBUG_IMM_OM("om_imm_finalize(): Finalizing the IMM Object Implementer handle %llu.", immOmHandle);
	if(immOmHandle != (SaImmHandleT)0)
	{
		if (accessorHandle != (SaImmAccessorHandleT)0)
		{
			if ((error = saImmOmAccessorFinalize(accessorHandle)) != SA_AIS_OK)
			{
				//ERR_IMM_OM("om_imm_finalize(): saImmOmAccessorFinalize(%llu) FAILED: %s", accessorHandle, getOpenSAFErrorString(error));
			}
		}
		if ((error = autoRetry_saImmOmFinalize(immOmHandle))!= SA_AIS_OK)
		{
			//ERR_IMM_OM("om_imm_finalize(): saImmOmFinalize FAILED: %s", getOpenSAFErrorString(error));
		}
		else
		{
			//DEBUG_IMM_OM("om_imm_finalize(): Successfully finalized the immOmHandle");
		}
	}
	else
	{
		//DEBUG_IMM_OM("om_imm_finalize(): immOmHandle is null, not calling saImmOmFinalize()");
	}
	//LEAVE_IMM_OM();
}

SaAisErrorT saLogDispatch(SaLogHandleT logHandle, SaDispatchFlagsT dispatchFlags)
{
	return SA_AIS_OK;
}

SaAisErrorT	saLogStreamOpen_2(SaLogHandleT logHandle,
			   const SaNameT *logStreamName,
			   const SaLogFileCreateAttributesT_2 *logFileCreateAttributes,
			   SaLogStreamOpenFlagsT logStreamOpenFlags,
			   SaTimeT timeout, SaLogStreamHandleT *logStreamHandle){
	return SA_AIS_OK;
}

//MafMwSpiTrace_1T maf_comSATrace_interface;


SaAisErrorT saLogInitialize(SaLogHandleT *logHandle, const SaLogCallbacksT *callbacks, SaVersionT *version){
	return SA_AIS_OK;
}

SaAisErrorT saLogSelectionObjectGet(SaLogHandleT logHandle, SaSelectionObjectT *selectionObject){
	return SA_AIS_OK;
}

SaAisErrorT saLogStreamClose(SaLogStreamHandleT logStreamHandle){
	return SA_AIS_OK;
}

SaAisErrorT saLogFinalize(SaLogHandleT logHandle){
	return SA_AIS_OK;
}

SaAisErrorT saLogWriteLogAsync(SaLogStreamHandleT logStreamHandle, SaInvocationT invocation, SaLogAckFlagsT ackFlags, const SaLogRecordT *logRecord){
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
		const SaNameT *objectName, const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes)
{

	if (0 == accessorHandle) {
		return SA_AIS_ERR_BAD_HANDLE;
	}

	const TheSaNameT* objName = reinterpret_cast<const TheSaNameT*>(objectName);
	std::string obj((char*)objName->value, static_cast<size_t>(objName->length));
	std::string attr((char*)attributeNames[0]);
	printf("----> saImmOmAccessorGet_2 \n");
	if (attrValues_Returned != NULL) *attributes = attrValues_Returned;
	return autoRetry_saImmOmAccessorGet_2_returnValue;
}
void Om_finalize_imm(SaImmHandleT immOmHandle){
	printf("----> Om_finalize_imm \n");
}

const char *getOpenSAFErrorString(SaAisErrorT error)
{
	printf("----> getOpenSAFErrorString \n");
	const char* ret = "SA_AIS_OK (1)";
	return ret;
}

int poll_setcallback(
	void* handle, FdCallbackFn fn, struct pollfd pfd, void *ref){
	return 0;
}

void
poll_unsetcallback(void* handle, int fd)
{
	printf("----> poll_unsetcallback \n");
}


// This function is needed because EXPECT_EQ compares only long int, but we want to compare 2 unsigned long long integers
bool unsignedLongLongCompare(SaUint64T num1, SaUint64T num2)
{
	if(num1 == num2)
	{
		return true;
	}
	else
	{
		return false;
	}
}

// MOCKED FUNCTIONS
char const* coremw_xmlnode_contents(xmlNode* cur)
{
	return NULL;
}

char const* coremw_xmlnode_get_attribute(xmlNode* n, char const* attr_name)
{
	return NULL;
}

TEST (logService, positiveTest1)
{
	// Facility is set to be an alarm
	MafReturnT res;
	uint32_t eventId = 12121;
	MwSpiSeverityT severity = 121;
	MwSpiFacilityT facility = 100;
	const char* databuffer = "a";
	res = logWrite(eventId, severity, facility, databuffer);
	EXPECT_EQ(res, 0);
}


TEST (logService, positiveTest2)
{
	// Facility is set to be an alert
	MafReturnT res;
	uint32_t eventId = 1;
	MwSpiSeverityT severity = 2;
	MwSpiFacilityT facility = 101;
	char* databuffer = "This is the data buffer in unittest";

	res = logWrite (eventId, severity, facility, databuffer);
	EXPECT_EQ (res, 0);
}

// Parsing config of alarm, alert and command type of logs
TEST (logService, parseSaLogConfigPositiveTest1)
{
	bool res = false;
	alarmLogFileAttributes.logFileName = DEFAULT_ALARM_LOG_FILE_NAME;
	alertLogFileAttributes.logFileName = DEFAULT_ALERT_LOG_FILE_NAME;
	cmdLogFileAttributes.logFileName = DEFAULT_CMD_LOG_FILE_NAME;
	alarmLogFileAttributes.maxFilesRotated = DEFAULT_ALARM_MAX_FILES_ROTATED;
	alertLogFileAttributes.maxFilesRotated = DEFAULT_ALERT_MAX_FILES_ROTATED;
	cmdLogFileAttributes.maxFilesRotated = DEFAULT_CMD_MAX_FILES_ROTATED;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	cmdLogFileAttributes.maxLogFileSize = DEFAULT_CMD_MAX_LOG_FILE_SIZE;
	char* alarmFileNameText = "AlarmLogFileName=TestAlarmLogFileName";
	char* alertFileNameText = "AlertLogFileName=TestAlertLogFileName";
	char* cmdFileNameText = "CommandLogFileName=TestCommandLogFileName";
	char* alarmFilesRotated = "AlarmFilesRotated=5";
	char* alertFilesRotated = "AlertFilesRotated=6";
	char* cmdFilesRotated = "CommandFilesRotated=7";
	char* alarmFileSizeText = "AlarmMaxLogFileSize=123000321";
	char* alertFileSizeText = "AlertMaxLogFileSize=76530845";
	char* cmdFileSizeText = "CommandMaxLogFileSize=76530846";
	char * expectedAlarmLogFileName = "TestAlarmLogFileName";
	char * expectedAlertLogFileName = "TestAlertLogFileName";
	char * expectedCmdLogFileName = "TestCommandLogFileName";
	SaUint64T expectedAlarmFilesRotated = 5;
	SaUint64T expectedAlertFilesRotated = 6;
	SaUint64T expectedCmdFilesRotated = 7;
	SaUint64T expectedAlarmLogFilesize = 123000321;
	SaUint64T expectedAlertLogFilesize = 76530845;
	SaUint64T expectedCmdLogFilesize = 76530846;
	parseSaLogConfig(alarmFileNameText);
	parseSaLogConfig(alertFileNameText);
	parseSaLogConfig(alarmFilesRotated);
	parseSaLogConfig(alertFilesRotated);
	parseSaLogConfig(alarmFileSizeText);
	parseSaLogConfig(alertFileSizeText);
	parseSaLogConfig(cmdFileNameText);
	parseSaLogConfig(cmdFileSizeText);
	parseSaLogConfig(cmdFilesRotated);

	EXPECT_STREQ(alarmLogFileAttributes.logFileName, expectedAlarmLogFileName);
	EXPECT_STREQ(alertLogFileAttributes.logFileName, expectedAlertLogFileName);
	EXPECT_STREQ(cmdLogFileAttributes.logFileName, expectedCmdLogFileName);
	EXPECT_EQ(alertLogFileAttributes.maxFilesRotated, expectedAlertFilesRotated);
	EXPECT_EQ(alarmLogFileAttributes.maxFilesRotated, expectedAlarmFilesRotated);
	EXPECT_EQ(cmdLogFileAttributes.maxFilesRotated, expectedCmdFilesRotated);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(cmdLogFileAttributes.maxLogFileSize, expectedCmdLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigPositiveTest2)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	char* alarmConfigText = "AlarmMaxLogFileSize =4723564";
	char* alertConfigText = "AlertMaxLogFileSize =   2356235";
	SaUint64T expectedAlarmLogFilesize = 4723564;
	SaUint64T expectedAlertLogFilesize = 2356235;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigPositiveTest3)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	char* alarmConfigText = "AlarmMaxLogFileSize =4723564 ";
	char* alertConfigText = "AlertMaxLogFileSize =   2356235   ";
	char* extraLine = "";
	SaUint64T expectedAlarmLogFilesize = 4723564;
	SaUint64T expectedAlertLogFilesize = 2356235;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	parseSaLogConfig(extraLine);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigPositiveTest4)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	char* alarmConfigText = "AlarmMaxLogFileSize = 563463 \n";
	char* alertConfigText = "AlertMaxLogFileSize =   923578\n";
	char* extraLine1 = "\n";
	char* extraLine2 = "";
	SaUint64T expectedAlarmLogFilesize = 563463;
	SaUint64T expectedAlertLogFilesize = 923578;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	parseSaLogConfig(extraLine1);
	parseSaLogConfig(extraLine2);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigPositiveTest5)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	char* alarmConfigText = "AlarmMaxLogFileSize=1";
	char* alertConfigText = "AlertMaxLogFileSize=1";
	char* extraLine1 = "\n";
	char* extraLine2 = "";
	SaUint64T expectedAlarmLogFilesize = 1;
	SaUint64T expectedAlertLogFilesize = 1;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	parseSaLogConfig(extraLine1);
	parseSaLogConfig(extraLine2);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigNegativeTest1)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	// negative case: not valid syntax
	char* alarmConfigText = "AlarmMaxLogFileSize 4723564"; // no "=" here
	char* alertConfigText = "AlertMaxlogFileSize=2356235"; // "log" instead of "Log"
	// for negative case: we won't update the default value which is 500000
	// so we test as a positive case were the expected value must be 500000
	SaUint64T expectedAlarmLogFilesize = 500000;
	SaUint64T expectedAlertLogFilesize = 500000;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigsNegativeTest2)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	// negative case: not valid syntax
	char* alarmConfigText = "AlarmMaxLogFileSize = 4723564";
	char* alertConfigText = "AlertMaxlogFileSize=2356235"; // "log" instead of "Log"
	// for negative case: we won't update the default value which is 500000
	// so we test as a positive case were the expected value must be 500000
	SaUint64T expectedAlarmLogFilesize = 4723564;
	SaUint64T expectedAlertLogFilesize = 500000;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigNegativeTest3)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	// negative case: not valid syntax
	char* alarmConfigText = "AlarmMaxLogFileSize 4723564"; // no "=" here
	char* alertConfigText = "AlertMaxLogFileSize=2356235";
	// for negative case: we won't update the default value which is 500000
	// so we test as a positive case were the expected value must be 500000
	SaUint64T expectedAlarmLogFilesize = 500000;
	SaUint64T expectedAlertLogFilesize = 2356235;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigNegativeTest4)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	// negative case: not valid syntax
	char* alarmConfigText = "AlarmMaxLogFileSize = 4723564B"; // illegal character
	char* alertConfigText = "AlertMaxLogFileSize=235a6235"; // illegal character
	// for negative case: we won't update the default value which is 500000
	// so we test as a positive case were the expected value must be 500000
	SaUint64T expectedAlarmLogFilesize = 500000;
	SaUint64T expectedAlertLogFilesize = 500000;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Parsing config of alarm and alert type of logs
TEST (logService, parseSaLogConfigNegativeTest5)
{
	bool res = false;
	alarmLogFileAttributes.maxLogFileSize = DEFAULT_ALARM_MAX_LOG_FILE_SIZE;
	alertLogFileAttributes.maxLogFileSize = DEFAULT_ALERT_MAX_LOG_FILE_SIZE;
	// negative case: not valid syntax
	char* alarmConfigText = "AlarmMaxLogFileSize = 47rv23564"; // illegal characters
	char* alertConfigText = "AlertMaxLogFileSize=25g6f2e35"; // illegal characters
	// for negative case: we won't update the default value which is 500000
	// so we test as a positive case were the expected value must be 500000
	SaUint64T expectedAlarmLogFilesize = 500000;
	SaUint64T expectedAlertLogFilesize = 500000;
	parseSaLogConfig(alarmConfigText);
	parseSaLogConfig(alertConfigText);
	res = unsignedLongLongCompare(alarmLogFileAttributes.maxLogFileSize, expectedAlarmLogFilesize);
	EXPECT_TRUE(res);
	res = unsignedLongLongCompare(alertLogFileAttributes.maxLogFileSize, expectedAlertLogFilesize);
	EXPECT_TRUE(res);
}

// Creating symlink for NBI visibility of alarm and alert type of logs
TEST (logService, createSymlinkForAlarmAndAlertLogsPositiveTest1)
{
	// the real symlink won't be created this time, we check only the result of the functions
	const char * safLogPath = "/storage/no-backup/coremw/var/log/saflog";
	const char * linkDir = "/var/filem/internal_root";
	const char * alarmLinkName = "AlarmLogs";
	const char * alertLinkName = "AlertLogs";
	int alarmResult = createSymLinkForAlarmsAndAlerts(safLogPath, DEFAULT_ALARM_LOG_FILE_PATH_NAME, linkDir, alarmLinkName);
	int alertResult = createSymLinkForAlarmsAndAlerts(safLogPath, DEFAULT_ALERT_LOG_FILE_PATH_NAME, linkDir, alertLinkName);
	EXPECT_TRUE(alarmResult);
	EXPECT_TRUE(alertResult);
}

// Creating symlink for NBI visibility of alarm and alert type of logs
TEST (logService, createSymlinkForAlarmAndAlertLogsNegativeTest1)
{
	// the real symlink won't be created this time, we check only the result of the functions
	const char * linkDir = "/var/filem/internal_root";
	const char * alarmLinkName = "AlarmLogs";
	const char * alertLinkName = "AlertLogs";
	int alarmResult = createSymLinkForAlarmsAndAlerts(NULL, DEFAULT_ALARM_LOG_FILE_PATH_NAME, linkDir, alarmLinkName);
	int alertResult = createSymLinkForAlarmsAndAlerts(NULL, DEFAULT_ALERT_LOG_FILE_PATH_NAME, linkDir, alertLinkName);
	EXPECT_FALSE(alarmResult);
	EXPECT_FALSE(alertResult);
}

// Creating symlink for NBI visibility of alarm and alert type of logs
TEST (logService, createSymlinkForAlarmAndAlertLogsNegativeTest2)
{
	// the real symlink won't be created this time, we check only the result of the functions
	const char * safLogPath = "/storage/no-backup/coremw/var/log/saflog";
	const char * linkDir = "/var/filem/internal_root";
	const char * alarmLinkName = "AlarmLogs";
	const char * alertLinkName = "AlertLogs";
	int alarmResult = createSymLinkForAlarmsAndAlerts(safLogPath, NULL, linkDir, alarmLinkName);
	int alertResult = createSymLinkForAlarmsAndAlerts(safLogPath, NULL, linkDir, alertLinkName);
	EXPECT_FALSE(alarmResult);
	EXPECT_FALSE(alertResult);
}

// Creating symlink for NBI visibility of alarm and alert type of logs
TEST (logService, createSymlinkForAlarmAndAlertLogsNegativeTest3)
{
	// the real symlink won't be created this time, we check only the result of the functions
	const char * safLogPath = "/storage/no-backup/coremw/var/log/saflog";
	const char * alarmLinkName = "AlarmLogs";
	const char * alertLinkName = "AlertLogs";
	int alarmResult = createSymLinkForAlarmsAndAlerts(safLogPath, DEFAULT_ALARM_LOG_FILE_PATH_NAME, NULL, alarmLinkName);
	int alertResult = createSymLinkForAlarmsAndAlerts(safLogPath, DEFAULT_ALERT_LOG_FILE_PATH_NAME, NULL, alertLinkName);
	EXPECT_FALSE(alarmResult);
	EXPECT_FALSE(alertResult);
}

// Creating symlink for NBI visibility of alarm and alert type of logs
TEST (logService, createSymlinkForAlarmAndAlertLogsNegativeTest4)
{
	// the real symlink won't be created this time, we check only the result of the functions
	const char * safLogPath = "/storage/no-backup/coremw/var/log/saflog";
	const char * linkDir = "/var/filem/internal_root";
	int alarmResult = createSymLinkForAlarmsAndAlerts(safLogPath, DEFAULT_ALARM_LOG_FILE_PATH_NAME, linkDir, NULL);
	int alertResult = createSymLinkForAlarmsAndAlerts(safLogPath, DEFAULT_ALERT_LOG_FILE_PATH_NAME, linkDir, NULL);
	EXPECT_FALSE(alarmResult);
	EXPECT_FALSE(alertResult);
}

// Creating symlink for NBI visibility of alarm and alert type of logs
TEST (logService, createSymlinkForAlarmAndAlertLogsNegativeTest5)
{
	// the real symlink won't be created this time, we check only the result of the functions
	int alarmResult = createSymLinkForAlarmsAndAlerts(NULL, NULL, NULL, NULL);
	int alertResult = createSymLinkForAlarmsAndAlerts(NULL, NULL, NULL, NULL);
	EXPECT_FALSE(alarmResult);
	EXPECT_FALSE(alertResult);
}

TEST (logService, readSafLogPathTest)
{
	SaAisErrorT res;
	char * safLogPath;
	char * attributeName = SAF_LOG_PATH_ATTR_NAME;
	SaImmAttrValuesT_2* attrVal1 = allocateStringAttrValues(SAF_LOG_PATH_ATTR_NAME , "saLogSystem");
	attrValues_Returned = &attrVal1;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	res = readSafLogPath(&safLogPath);
	EXPECT_STREQ(safLogPath, "saLogSystem");
	EXPECT_EQ(res, SA_AIS_OK);
	freeAttrValues(attrVal1);
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}

TEST (logService, readCfgLogStreamsInfoFromImm)
{
	SaAisErrorT res;
	char * safLogPath;
	char * attributeName = LOG_STREAM_FILENAME_PARAM;
	//alarm stream names  are not matched
	alarmLogFileAttributes.logFileName = "AlarmStream1";
	alertLogFileAttributes.logFileName = "AlertStream1";
	cmdLogFileAttributes.logFileName = "CmdStream1";
	SaImmAttrValuesT_2* attrVal1 = allocateStringAttrValues(LOG_STREAM_FILENAME_PARAM , "AlarmStream");
	attrValues_Returned = &attrVal1;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	readCfgLogStreamsInfoFromImm();
	EXPECT_EQ(updateAlarmLogFileNameAttr, true);
	EXPECT_EQ(updateAlertLogFileNameAttr, true);
	EXPECT_EQ(updateCmdLogFileNameAttr, true);
	freeAttrValues(attrVal1);
	//alert stream names are matched
	updateAlarmLogFileNameAttr = false;
	attrVal1 = allocateStringAttrValues(LOG_STREAM_FILENAME_PARAM , "AlertStream1");
	attrValues_Returned = &attrVal1;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	readCfgLogStreamsInfoFromImm();
	EXPECT_EQ(updateAlertLogFileNameAttr, false);
	EXPECT_EQ(updateAlarmLogFileNameAttr, true);
	EXPECT_EQ(updateCmdLogFileNameAttr, true);
	freeAttrValues(attrVal1);
	// check severity filter received from IMM at startup
	attributeName = LOGM_LOG_SEVERITYFILTER;
	int32_t sevFilter[2] = { 2 , 4}; // i.e third and fifth bit is set
	attrVal1 = allocateint32AttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter , 2);
	attrValues_Returned = &attrVal1;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	readCfgLogStreamsInfoFromImm();
	EXPECT_EQ(alarmLogSevFlags, 20);
	EXPECT_EQ(alertLogSevFlags, 20);
	EXPECT_EQ(cmdLogSevFlags, 20);
	freeAttrValues(attrVal1);
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}
TEST (logService, getLoggerInfo_alarm_log)
{
	MwSpiFacilityT facility = 100;
	MwSpiSeverityT severity = 2;
	int* isLogFileOpen;
	SaNameT* loggerName;
	SaLogStreamHandleT* logStreamHandle;
	bool isLogAllowed;

	alarmLogStreamHandle = 10;
	alarmLogFileIsOpen = FALSE;
	saNameSet("alarmLoggerName", &alarmLoggerName);

	getLoggerInfo(facility, &severity, &isLogFileOpen, &loggerName, &logStreamHandle, &isLogAllowed);

	EXPECT_TRUE(strcmp(saNameGet(loggerName), "alarmLoggerName") == 0);

	bool res = unsignedLongLongCompare(alarmLogStreamHandle, *logStreamHandle);
	EXPECT_TRUE(res);
	EXPECT_TRUE(*isLogFileOpen == FALSE);
	EXPECT_TRUE(isLogAllowed);

}

TEST (logService, getLoggerInfo_alert_log)
{
	MwSpiFacilityT facility = 101;
	MwSpiSeverityT severity = 2;
	int* isLogFileOpen;
	SaNameT* loggerName;
	SaLogStreamHandleT* logStreamHandle;
	bool isLogAllowed;

	alertLogFileIsOpen = FALSE;
	saNameSet("alertLoggerName", &alertLoggerName);
	alertLogStreamHandle = 10;

	getLoggerInfo(facility, &severity, &isLogFileOpen, &loggerName, &logStreamHandle, &isLogAllowed);

	EXPECT_TRUE(strcmp(saNameGet(loggerName), "alertLoggerName") == 0);

	bool res = unsignedLongLongCompare(alertLogStreamHandle, *logStreamHandle);
	EXPECT_TRUE(res);
	EXPECT_TRUE(*isLogFileOpen == FALSE);
	EXPECT_TRUE(isLogAllowed);

}

TEST (logService, getLoggerInfo_command_log)
{
	MwSpiFacilityT facility = 300;
	MwSpiSeverityT severity = 2;
	int* isLogFileOpen;
	SaNameT* loggerName;
	SaLogStreamHandleT* logStreamHandle;
	bool isLogAllowed;

	cmdLogFileIsOpen = FALSE;
	saNameSet("cmdLoggerName", &cmdLoggerName);
	cmdLogStreamHandle = 10;

	getLoggerInfo(facility, &severity, &isLogFileOpen, &loggerName, &logStreamHandle, &isLogAllowed);

	EXPECT_TRUE(strcmp(saNameGet(loggerName), "cmdLoggerName") == 0);

	bool res = unsignedLongLongCompare(cmdLogStreamHandle, *logStreamHandle);
	EXPECT_TRUE(res);
	EXPECT_TRUE(*isLogFileOpen == FALSE);
	EXPECT_TRUE(isLogAllowed);
}

TEST (logService, getLoggerInfo_security_log)
{
	MwSpiFacilityT facility = 500;
	MwSpiSeverityT severity = 2;
	int* isLogFileOpen;
	SaNameT* loggerName;
	SaLogStreamHandleT* logStreamHandle;
	bool isLogAllowed;

	secLogFileIsOpen = FALSE;
	saNameSet("secLoggerName", &secLoggerName);
	secLogStreamHandle = 10;

	getLoggerInfo(facility, &severity, &isLogFileOpen, &loggerName, &logStreamHandle, &isLogAllowed);

	EXPECT_TRUE(strcmp(saNameGet(loggerName), "secLoggerName") == 0);

	bool res = unsignedLongLongCompare(secLogStreamHandle, *logStreamHandle);
	EXPECT_TRUE(res);
	EXPECT_TRUE(*isLogFileOpen == FALSE);
	EXPECT_TRUE(isLogAllowed);
}

TEST (logService, getLoggerInfo_default)
{
	MwSpiFacilityT facility = 105;
	MwSpiSeverityT severity = 2;
	int* isLogFileOpen;
	SaNameT* loggerName;
	SaLogStreamHandleT* logStreamHandle;
	bool isLogAllowed;

	LogFileIsOpen = FALSE;
	saNameSet("defaultLoggerName", &theLoggerName);
	theLogStreamHandle = 10;

	getLoggerInfo(facility, &severity, &isLogFileOpen, &loggerName, &logStreamHandle, &isLogAllowed);

	EXPECT_TRUE(strcmp(saNameGet(loggerName), "defaultLoggerName") == 0);

	bool res = unsignedLongLongCompare(theLogStreamHandle, *logStreamHandle);
	EXPECT_TRUE(res);
	EXPECT_TRUE(*isLogFileOpen == FALSE);
	EXPECT_TRUE(isLogAllowed);

}

TEST (logService, modifyCfgLogStreamInstances)
{
	MafReturnT ret;
	ret = modifyCfgLogStreamInstances();
	EXPECT_EQ(ret, MafOk);
}

TEST (logService, maf_ComLogServiceOpen)
{
	MafReturnT myRetVal;
	int32_t sevFilter[2] = {2 , 4};
	SaImmAttrValuesT_2* attrVal = allocateint32AttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter, 2);
	attrValues_Returned = &attrVal;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	myRetVal = maf_ComLogServiceOpen();
	EXPECT_EQ(saSystemLogSevFlags, 20);
	EXPECT_EQ(myRetVal, MafOk);
	sleep(1);
	myRetVal = ComLogServiceClose();
	EXPECT_EQ(myRetVal, MafOk);
	freeAttrValues(attrVal);
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}

TEST (logService, initComCfgLogStream)
{
	MafReturnT myRetVal;
	int32_t sevFilter[2] = {1 , 4};
	SaImmAttrValuesT_2* attrVal = allocateint32AttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter, 2);
	attrValues_Returned = &attrVal;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	myRetVal = initCompCfgLogStream();
	EXPECT_EQ(compLogSevFlags, 18);
	EXPECT_EQ(myRetVal, MafOk);
	sleep(1);
	finalizeCfgLogStream();
	freeAttrValues(attrVal);
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}

TEST (logService, initLmCfgLogStream)
{
        MafReturnT myRetVal;
	_CC_NAME="lm";
        int32_t sevFilter[2] = {1 , 4};
        SaImmAttrValuesT_2* attrVal = allocateint32AttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter, 2);
        attrValues_Returned = &attrVal;
        autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
        myRetVal = initCompCfgLogStream();
        EXPECT_EQ(compLogSevFlags, 18);
        EXPECT_EQ(myRetVal, MafOk);
        sleep(1);
        finalizeCfgLogStream();
        freeAttrValues(attrVal);
        autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}

TEST (logService, initComSaCfgLogStream)
{
	MafReturnT myRetVal;
	int32_t sevFilter[2] = {2 , 3};
	SaImmAttrValuesT_2* attrVal = allocateint32AttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter, 2);
	attrValues_Returned = &attrVal;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	myRetVal = initCompSaCfgLogStream();
	EXPECT_EQ(comSaLogSevFlags, 12);
	EXPECT_EQ(myRetVal, MafOk);
	sleep(1);
	finalizeCompSaCfgLogStream();
	freeAttrValues(attrVal);
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}

TEST (logService, initLmSaCfgLogStream)
{
        MafReturnT myRetVal;
	_CC_NAME="lm";
        int32_t sevFilter[2] = {2 , 3};
        SaImmAttrValuesT_2* attrVal = allocateint32AttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter, 2);
        attrValues_Returned = &attrVal;
        autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
        myRetVal = initCompSaCfgLogStream();
        EXPECT_EQ(comSaLogSevFlags, 12);
        EXPECT_EQ(myRetVal, MafOk);
        sleep(1);
        finalizeCompSaCfgLogStream();
        freeAttrValues(attrVal);
        autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}

TEST (logService, cmdLogServiceOpen)
{
	MafReturnT myRetVal;
	int32_t sevFilter[2] = {2 , 4};
	SaImmAttrValuesT_2* attrVal = allocateint32AttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter, 2);
	attrValues_Returned = &attrVal;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	readCfgLogStreamsInfoFromImm();
	myRetVal = cmdLogServiceOpen();
	EXPECT_EQ(cmdLogSevFlags, 20);
	EXPECT_EQ(myRetVal, MafOk);
	sleep(1);
	myRetVal = cmdLogServiceClose();
	EXPECT_EQ(myRetVal, MafOk);
	freeAttrValues(attrVal);
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}

TEST (logService, secLogServiceOpen)
{
	MafReturnT myRetVal;
	int32_t sevFilter[2] = {2 , 4};
	SaImmAttrValuesT_2* attrVal = allocateint32AttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter, 2);
	attrValues_Returned = &attrVal;
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_OK;
	readCfgLogStreamsInfoFromImm();
	IdentityString = "secLogger";
	myRetVal = secLogServiceOpen();
	EXPECT_EQ(secLogSevFlags, 20);
	EXPECT_EQ(myRetVal, MafOk);
	sleep(1);
	myRetVal = secLogServiceClose();
	EXPECT_EQ(myRetVal, MafOk);
	freeAttrValues(attrVal);
	autoRetry_saImmOmAccessorGet_2_returnValue = SA_AIS_ERR_FAILED_OPERATION; //reset
}

TEST (logService, compCfgLogWriteLogCallbackT)
{
	SaAisErrorT Errt = SA_AIS_ERR_TRY_AGAIN;
	SaInvocationT invocation = 0;
	for (int i=1;i<=5;i++){
                Errt = SA_AIS_ERR_TRY_AGAIN;
		if(i==3)
			Errt=SA_AIS_ERR_NOT_EXIST;
	        compCfgLogWriteLogCallbackT(invocation,Errt);
        }
        EXPECT_EQ(tryagainCount, 2);
}
