/*
 * ComSANtf_unittest.cc
 *
 *  Created on: Apr 30, 2013
 *      Author: eaparob
 *
 */


#include "CmCache_unittest.h"

// To make this output DEBUG printouts to std out. Change gLog level variable to LOG_LEVEL_DEBUG and remove the comment below
#define REDIRECT_LOG
#ifdef REDIRECT_LOG
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG printf
#else
#include "trace.h"
#endif

int trace_flag = 0;
bool receivedOverflowEvent = false;

/* This is a mock object of maxSAMemoryForCMEvents in ComSAComponent.c
 * There is no strict limitation for this value.
 * The present value is evaluated based on existing test scenarios.
 */
SaUint64T maxSAMemoryForCMEvents = 2048;

#ifdef REDIRECT_LOG
////////////////////////////////////3//////////////////
// LOG
//////////////////////////////////////////////////////
#ifdef  __cplusplus
extern "C" {
#endif
#define LOG_PREFIX " "
static void coremw_vlog2(int priority, char const* fmt, va_list ap) {
	char buffer[256];
	int len = strlen(LOG_PREFIX);
	strcpy(buffer, LOG_PREFIX);
	buffer[len] = ' ';
	len++;
	vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);
	printf("DEBUG: %s\n", buffer);
}

void coremw_log(int priority, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	coremw_vlog2(priority, fmt, ap);
	va_end(ap);
}
void coremw_debug_log(int priority, const char* fmt, ...) {
      va_list ap;
      va_start(ap, fmt);
      coremw_vlog2(priority, fmt, ap);
      va_end(ap);
}

#ifdef  __cplusplus
}
#endif

#endif //REDIRECT_LOG

void err_quit(char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	va_end(ap);
}

int clock_gettime (clockid_t __clock_id, struct timespec *ts)
{
	//printf("CM Cache Unittest: clock_gettime called\n");
	ts->tv_nsec = 0;
	ts->tv_sec = 1122334455;
	return 0;
}

MafOamSpiCmEvent_Notification_1T *CmEvent = NULL;
MafReturnT push_CmCacheUnittest(MafOamSpiCmEvent_Notification_1T *mafCmNot)
{
	printf("CM Cache Unittest: CM Event received from CM Cache\n\n");
	CmEvent = new MafOamSpiCmEvent_Notification_1T;
	CmEvent->txHandle = mafCmNot->txHandle;
	CmEvent->eventTime = mafCmNot->eventTime;
	CmEvent->sourceIndicator = mafCmNot->sourceIndicator;
	CmEvent->events = mafCmNot->events;
	if ((CmEvent->events != NULL)
			&& (CmEvent->events[0] != NULL)
			&& (CmEvent->events[0]->eventType == MafOamSpiCmEvent_Overflow_1))
	{
		receivedOverflowEvent = true;
	}
	return MafOk;
}

// convert the MafOamSpiCmEvent_SourceIndicator_1T enum types to strings
std::string sourceIndicatorToString(MafOamSpiCmEvent_SourceIndicator_1T sourceIndicator)
{
	switch(sourceIndicator)
	{
	case MafOamSpiCmEvent_ResourceOperation_1:
		return "ResourceOperation";
	case MafOamSpiCmEvent_ManagementOperation_1:
		return "ManagementOperation";
	case MafOamSpiCmEvent_SonOperation_1:
		return "SonOperation";
	case MafOamSpiCmEvent_Unknown_1:
		return "Unknown";
	default:
		printf("sourceIndicatorToString(): invalid sourceIndicator");
		return "";
	}
}

// convert the MafOamSpiCmEvent_EventType_1T enum types to strings
std::string eventTypeToString(MafOamSpiCmEvent_EventType_1T eventType)
{
	switch(eventType)
	{
	case MafOamSpiCmEvent_MoCreated_1:
		return "MoCreated";
	case MafOamSpiCmEvent_MoDeleted_1:
		return "MoDeleted";
	case MafOamSpiCmEvent_AttributeValueChange_1:
		return "AttributeValueChange";
	case MafOamSpiCmEvent_Overflow_1:
		return "Overflow";
	default:
		printf("eventTypeToString(): invalid eventType");
		return "";
	}
}

// This function exist in CmEventHandler also.
// The reason for duplicating is that this function is used for evaluating the result of the unit test cases,
// and evaluating the results should not be dependent on a possible future design change.
std::string AttrValueContainerToString(MafMoAttributeValueContainer_3T &avc)
{
	std::string avcStr;
	char b[100];
	switch(avc.type)
	{
	case MafOamSpiMoAttributeType_3_INT8:
		avcStr = " attrType: INT8";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%d", avc.values[i].value.i8);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_INT16:
		avcStr = " attrType: INT16";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%d", avc.values[i].value.i16);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_INT32:
		avcStr = " attrType: INT32";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%d", avc.values[i].value.i32);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_INT64:
		avcStr = " attrType: INT64";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%ld", avc.values[i].value.i64);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_UINT8:
		avcStr = " attrType: UINT8";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%u", avc.values[i].value.u8);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_UINT16:
		avcStr = " attrType: UINT16";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%u", avc.values[i].value.u16);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_UINT32:
		avcStr = " attrType: UINT32";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%u", avc.values[i].value.u32);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_UINT64:
		avcStr = " attrType: UINT64";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%lu", avc.values[i].value.u64);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_STRING:
		avcStr = " attrType: STRING";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			avcStr += " " + std::string(avc.values[i].value.theString);
		}
		break;
	case MafOamSpiMoAttributeType_3_BOOL:
		avcStr = " attrType: BOOL";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%d", avc.values[i].value.theBool);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_REFERENCE:
		printf("AttrValueContainerToString(): not supported type\n");
		avcStr = " attrType: REFERENCE";
		break;
	case MafOamSpiMoAttributeType_3_ENUM:
		avcStr = " attrType: ENUM";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b) + " values:";
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			char buf[100];
			sprintf(buf,"%ld", avc.values[i].value.theEnum);
			avcStr += " " + std::string(buf);
		}
		break;
	case MafOamSpiMoAttributeType_3_STRUCT:
		avcStr = " attrType: STRUCT";
		sprintf(b,"%u", avc.nrOfValues);
		avcStr += " nrOfValues: " + std::string(b);
		for(unsigned int i = 0; i < avc.nrOfValues; i++)
		{
			//syslog(LOG_INFO, "%s AttrValueContainerToString(): struct element %d:",log_prefix,i);
			MafMoAttributeValueStructMember_3 *SM = avc.values[i].value.structMember;
			while(SM != NULL)
			{
				//syslog(LOG_INFO, "%s AttrValueContainerToString():    while enter SM != NULL",log_prefix);
				if(SM->memberName != NULL)
				{
					//syslog(LOG_INFO, "%s AttrValueContainerToString():       SM->memberName != NULL",log_prefix);
					avcStr += " memberName: " + std::string(SM->memberName);
					if(SM->memberValue != NULL)
					{
						std::string memberValue = AttrValueContainerToString(*(SM->memberValue));
						//syslog(LOG_INFO, "%s AttrValueContainerToString():          memberValue: (%s)",log_prefix,memberValue.c_str());
						avcStr += " memberValue:" + memberValue;
					}
				}
				SM = SM->next;
			}
		}
		break;
	case MafOamSpiMoAttributeType_3_DECIMAL64:
		printf("AttrValueContainerToString(): not supported type\n");
		avcStr = " attrType: DECIMAL64";
		break;
	default:
		printf("AttrValueContainerToString(): invalid type %d\n",avc.type);
		avcStr = "";
		break;
	}
	return avcStr;
}

std::string CmEventToString()
{
	MafOamSpiCmEvent_Notification_1T* ES = CmEvent;
	if(ES == NULL)
	{
		return "NO CM NOTIFICATIONS RECEIVED BY UNITTEST";
	}
	std::string sourceIndicatorStr = sourceIndicatorToString(ES->sourceIndicator);
	MafOamSpiCmEvent_1T **events = ES->events;

	std::string eventsStr;
	for(int i = 0; events[i] != NULL; i++)
	{
		eventsStr += " dn: ";
		eventsStr += events[i]->dn;
		eventsStr += " eventType: ";
		eventsStr += eventTypeToString(events[i]->eventType);
		if(events[i]->attributes != NULL)
		{
			MafMoNamedAttributeValueContainer_3T **attrs = events[i]->attributes;
			eventsStr += " attributes:";
			std::string attrStr = "";
			for(int j = 0; attrs[j] != NULL; j++)
			{
				std::string nameStr = attrs[j]->name;
				std::string attrValueContStr = AttrValueContainerToString(attrs[j]->value);

				attrStr += " name: " + nameStr + attrValueContStr;
			}
			eventsStr += attrStr;
		}
		else
		{
			eventsStr += " attributes: NULL";
		}
	}

	char txHandleStr[20];
	sprintf(txHandleStr,"%lu",ES->txHandle);
	char eventTimeStr[50];
	sprintf(eventTimeStr,"%llu",ES->eventTime);

	std::string ret = "cmEventNotify(): txHandle: " + std::string(txHandleStr) + " sourceIndicator: " + sourceIndicatorStr + eventsStr + " eventTime: " + std::string(eventTimeStr);
	return ret;
}

/* This function frees all the memory of an MafMoAttributeValueContainer
 * If the type is struct then this function will be called recursively.
 */
void freeAVC_unittest(MafMoAttributeValueContainer_3T *avc)
{
	DEBUG("freeAVC(): freeing Attribute Value Container now\n");

	if(avc == NULL)
	{
		DEBUG("freeAVC(): Attribute Value Container = NULL\n");
		return;
	}
	else
	{
		DEBUG("freeAVC(): avc->type = %d\n",avc->type);
		DEBUG("freeAVC(): avc->nrOfValues = %u\n",avc->nrOfValues);
		for(unsigned int i = 0; i < avc->nrOfValues; i++)
		{
			DEBUG("freeAVC(): avc->values[%u]\n",i);
			if(avc->type == MafOamSpiMoAttributeType_3_STRING)
			{
				free((void*)avc->values[i].value.theString);
			}
			if(avc->type == MafOamSpiMoAttributeType_3_STRUCT)
			{
				DEBUG("freeAVC(): freeing struct members now\n");
				MafMoAttributeValueStructMember_3 *SM = avc->values[i].value.structMember;
				while(SM != NULL)
				{
					DEBUG("freeAVC(): freeing SM->memberName = (%s)\n",SM->memberName);
					free(SM->memberName);

					// recursively call the same function
					freeAVC_unittest(SM->memberValue);
					DEBUG("freeAVC(): freeing SM->memberValue\n");
					free(SM->memberValue);
					MafMoAttributeValueStructMember_3 *nextSM = SM->next;
					free(SM);
					SM =  nextSM;
				}
			}
		}
	}
	if(avc->nrOfValues >= 1)
	{
		DEBUG("freeAVC(): delete avc->values\n");
		free(avc->values);
	}
	DEBUG("freeAVC(): freeing Attribute Value Container: done\n");
}

// This function frees all the memory of the events in a CM event struct
void freeCmEvents_unittest(MafOamSpiCmEvent_1T** events)
{
	DEBUG("freeCmEvents(): Deleting the event struct array, deleting memory now\n");
	if(events != NULL)
	{
		unsigned int i=0;
		unsigned int j=0;
		for(i = 0; events[i] != NULL; i++)
		{
			if(events[i]->dn != NULL)
			{
				DEBUG("freeCmEvents(): freeing event[%u]->dn = (%s)\n",i,events[i]->dn);
				free((void*)(events[i]->dn));
			}
			if(events[i]->attributes != NULL)
			{
				for(j = 0; events[i]->attributes[j]; j++)
				{
					if(events[i]->attributes[j]->name != NULL)
					{
						DEBUG("freeCmEvents(): freeing events[%u]->attributes[%u]->name = (%s)\n",i,j,events[i]->attributes[j]->name);
						free((void*)(events[i]->attributes[j]->name));
					}
					DEBUG("freeCmEvents(): freeing events[%u]->attributes[%u]->value\n",i,j);
					freeAVC_unittest(&(events[i]->attributes[j]->value));
					DEBUG("freeCmEvents(): freeing events[%u]->attributes[%u]\n",i,j);
					free(events[i]->attributes[j]);
				}
				DEBUG("freeCmEvents(): freeing events[%u]->attributes array\n",i);
				free(events[i]->attributes);
			}
			DEBUG("freeCmEvents(): freeing event[%u]\n",i);
			free(events[i]);
		}
		DEBUG("freeCmEvents(): freeing events array\n");
		free(events);
	}
	else
	{
		DEBUG("freeCmEvents(): events = NULL\n");
	}
	DEBUG("freeCmEvents(): done\n");
}

void freeCmEvent()
{
	printf("CM Cache Unittest: freeing CmEvent now\n\n");
	if(CmEvent != NULL)
	{
		freeCmEvents_unittest(CmEvent->events);
		delete CmEvent;
		CmEvent = NULL;
	}
}

bool compareReceivedToExpected(std::string expectedNotification)
{
	bool ret = false;
	std::string receivedNotification = CmEventToString();
	// freeing CmEvent since it is not needed anymore
	freeCmEvent();
	printf("\n\nCM Cache Unittest: expectedNotification: (%s)\n\n",expectedNotification.c_str());
	printf(    "CM Cache Unittest: receivedNotification: (%s)\n\n",receivedNotification.c_str());
	if(receivedNotification == expectedNotification)
	{
		printf("CM Cache Unittest: receivedNotification = expectedNotification\n");
		ret = true;
	}
	else
	{
		printf("CM Cache Unittest: receivedNotification != expectedNotification\n");
		ret = false;
	}
	return ret;
}

MafMoAttributeValueContainer_3T* getAvcRef()
{
	MafMoAttributeValueContainer_3T *avcRef = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avcRef->type = MafOamSpiMoAttributeType_3_STRUCT;
	avcRef->nrOfValues = 1;
	avcRef->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avcRef->values[0].value.moRef = strdup("Some unused text");
	return avcRef;
}

MafMoAttributeValueContainer_3T* getAvcRef2()
{
	MafMoAttributeValueContainer_3T *avcRef = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avcRef->type = MafOamSpiMoAttributeType_3_STRUCT;
	avcRef->nrOfValues = 2;
	avcRef->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avcRef->values[0].value.moRef = strdup("Some unused text");
	avcRef->values[1].value.moRef = strdup("Some unused text2");
	return avcRef;
}

SaAisErrorT subscriberFunction_unittest()
{
	return SA_AIS_OK;
}

SaAisErrorT unsubscriberFunction_unittest()
{
	return SA_AIS_OK;
}

//****************************************************************************************************************

/* Input data: simple types only
 * Event types: attribute value change
 * number of DNs: 1
 * number of attributes: 1
 */
TEST (CmCacheTest, ConfigSimpleTypes1)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 7;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestInt8 attrType: INT8 nrOfValues: 1 values: 7 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(1, "TestInt8", "", "", avc);
	CmCache.setDn(1, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: simple types only
 * Event types: attribute value change
 * number of DNs: 1
 * number of attributes: 1
 */
TEST (CmCacheTest, ConfigSimpleTypes2)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 3;
	avc->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 3;
	avc->values[1].value.i8 = 4;
	avc->values[2].value.i8 = 5;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestInt8 attrType: INT8 nrOfValues: 3 values: 3 4 5 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(2, "TestInt8", "", "", avc);
	CmCache.setDn(2, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: simple types only
 * Event types: attribute value change
 * number of DNs: 1
 * number of attributes: 2
 */
TEST (CmCacheTest, ConfigSimpleTypes3)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_UINT64;
	avc->nrOfValues = 2;
	avc->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.u64 = 11;
	avc->values[1].value.u64 = 22;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT8;
	avc2->nrOfValues = 3;
	avc2->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i8 = 3;
	avc2->values[1].value.i8 = 4;
	avc2->values[2].value.i8 = 5;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestInt8 attrType: INT8 nrOfValues: 3 values: 3 4 5 name: TestUint64 attrType: UINT64 nrOfValues: 2 values: 11 22 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(3, "TestUint64", "", "", avc);
	CmCache.addAttr(3, "TestInt8", "", "", avc2);
	CmCache.setDn(3, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: simple types only
 * Event types: attribute value change
 * number of DNs: 2
 * number of attributes: 2 (1 attribute/ 1 DN)
 */
TEST (CmCacheTest, ConfigSimpleTypes4)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_UINT64;
	avc->nrOfValues = 2;
	avc->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.u64 = 11;
	avc->values[1].value.u64 = 22;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT8;
	avc2->nrOfValues = 3;
	avc2->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i8 = 3;
	avc2->values[1].value.i8 = 4;
	avc2->values[2].value.i8 = 5;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,Funny=9 eventType: AttributeValueChange attributes: name: TestUint64 attrType: UINT64 nrOfValues: 2 values: 11 22 dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestInt8 attrType: INT8 nrOfValues: 3 values: 3 4 5 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(4, "TestUint64", "", "", avc);
	CmCache.setDn(4, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=9", MafOamSpiCmEvent_AttributeValueChange_1, false);
	CmCache.addAttr(4, "TestInt8", "", "", avc2);
	CmCache.setDn(4, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: simple types only
 * Event types: attribute value change
 * number of DNs: 2
 * number of attributes: 4 (2 attribute/ 1 DN)
 */
TEST (CmCacheTest, ConfigSimpleTypes5)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 2;
	avc->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;
	avc->values[1].value.i8 = 22;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 3;
	avc2->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;
	avc2->values[2].value.i16 = 5;

	MafMoAttributeValueContainer_3T *avc3 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3->type = MafOamSpiMoAttributeType_3_UINT8;
	avc3->nrOfValues = 2;
	avc3->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc3->values[0].value.u8 = 33;
	avc3->values[1].value.u8 = 44;

	MafMoAttributeValueContainer_3T *avc4 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc4->type = MafOamSpiMoAttributeType_3_UINT16;
	avc4->nrOfValues = 3;
	avc4->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	avc4->values[0].value.u16 = 6;
	avc4->values[1].value.u16 = 7;
	avc4->values[2].value.u16 = 8;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation ";
	expectedNotification += "dn: ManagedElement=1,Funny=9 eventType: AttributeValueChange attributes: ";
	expectedNotification += "name: TestInt16 attrType: INT16 nrOfValues: 3 values: 3 4 5 ";
	expectedNotification += "name: TestInt8 attrType: INT8 nrOfValues: 2 values: 11 22 ";
	expectedNotification += "dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: ";
	expectedNotification += "name: TestUint16 attrType: UINT16 nrOfValues: 3 values: 6 7 8 ";
	expectedNotification += "name: TestUint8 attrType: UINT8 nrOfValues: 2 values: 33 44 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(5, "TestInt8", "", "", avc);
	CmCache.addAttr(5, "TestInt16", "", "", avc2);
	CmCache.setDn(5, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=9", MafOamSpiCmEvent_AttributeValueChange_1, false);
	CmCache.addAttr(5, "TestUint8", "", "", avc3);
	CmCache.addAttr(5, "TestUint16", "", "", avc4);
	CmCache.setDn(5, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: simple types only
 * Event types: delete object
 * number of DNs: 1
 * number of attributes: 0
 */
TEST (CmCacheTest, ConfigSimpleTypes6)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=1 eventType: MoDeleted attributes: eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.setDn(123, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=1", MafOamSpiCmEvent_MoDeleted_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: simple types only
 * Event types: create object
 * number of DNs: 1
 * number of attributes: 0
 */
TEST (CmCacheTest, ConfigSimpleTypes7)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=1 eventType: MoCreated attributes: eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.setDn(123, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=1", MafOamSpiCmEvent_MoCreated_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Creating an object with 1 attribute inside
 * Input data: simple types only
 * Event types: attribute value change
 * number of DNs: 1
 * number of attributes: 1
 */
TEST (CmCacheTest, ConfigSimpleTypes8)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 0;
	avc->values = NULL;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: MoCreated attributes: name: TestInt8 attrType: INT8 nrOfValues: 0 values: eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(1, "TestInt8", "testClassId=5", "", avc);
	CmCache.setDn(1, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_MoCreated_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: only call set with ccbLast=true (this test is needed for a special case where ccbLast=true comes with not notified attributes, so there is no addAttr call for that ccb. Since for event types of "create" CM notification is always sent independent on the isNotify flag, it is a must to send a notification)
 * Event types: create
 * number of DNs: 1
 * number of attributes: 0
 */
TEST (CmCacheTest, ConfigSimpleTypes9)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=500 eventType: MoCreated attributes: eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.setDn(509, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=500", MafOamSpiCmEvent_MoCreated_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: only call set with ccbLast=true (this test is needed for a special case where ccbLast=true comes with not notified attributes, so there is no addAttr call for that ccb. Since for event types of "delete" CM notification is always sent independent on the isNotify flag, it is a must to send a notification)
 * Event types: delete
 * number of DNs: 1
 * number of attributes: 0
 */
TEST (CmCacheTest, ConfigSimpleTypes10)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=500 eventType: MoDeleted attributes: eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.setDn(510, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=500", MafOamSpiCmEvent_MoDeleted_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: only call set with ccbLast=true (this test is needed for a special case where ccbLast=true comes with not notified attributes, so there is no addAttr call for that ccb, so CM notification is not sent if type is attrValueChange)
 * Event types: attribute value change
 * number of DNs: 1
 * number of attributes: 0
 */
TEST (CmCacheTest, ConfigSimpleTypes11)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// Set expected notification
	std::string expectedNotification = "NO CM NOTIFICATIONS RECEIVED BY UNITTEST";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.setDn(511, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=500", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: simple types only
 * Event types: attribute value change
 * number of DNs: 1
 * number of attributes: 1
 */
TEST (CmCacheTest, RuntimeSimpleTypes1)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 7;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestInt8 attrType: INT8 nrOfValues: 1 values: 7 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(0, "TestInt8", "", "", avc);
	CmCache.setDn(0, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: simple types only
 * Event types: attribute value change
 * number of DNs: 1
 * number of attributes: 2
 */
TEST (CmCacheTest, RuntimeSimpleTypes2)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_UINT64;
	avc->nrOfValues = 2;
	avc->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.u64 = 111;
	avc->values[1].value.u64 = 222;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT8;
	avc2->nrOfValues = 3;
	avc2->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i8 = 30;
	avc2->values[1].value.i8 = 40;
	avc2->values[2].value.i8 = 50;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestInt8 attrType: INT8 nrOfValues: 3 values: 30 40 50 name: TestUint64 attrType: UINT64 nrOfValues: 2 values: 111 222 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(0, "TestUint64", "", "", avc);
	CmCache.addAttr(0, "TestInt8", "", "", avc2);
	CmCache.setDn(0, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 1
 */
TEST (CmCacheTest, ConfigStructTypes1)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 7;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 7 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(101, "TestStruct", "testClassId=1", "", getAvcRef());
	CmCache.setDn(101, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(101, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt8", avc);
	CmCache.setDn(101, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 2
 */
TEST (CmCacheTest, ConfigStructTypes2)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 memberName: TestInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(102, "TestStruct", "testClassId=1", "", getAvcRef());
	CmCache.setDn(102, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(102, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt8", avc);
	CmCache.addAttr(102, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt16", avc2);
	CmCache.setDn(102, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: multi value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1 (1 multi value struct)
 * number of attributes in struct: 2 (2 attribute/1 struct)
 */
TEST (CmCacheTest, ConfigStructTypes3)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	MafMoAttributeValueContainer_3T *avc3 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3->type = MafOamSpiMoAttributeType_3_INT8;
	avc3->nrOfValues = 1;
	avc3->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3->values[0].value.i8 = 22;

	MafMoAttributeValueContainer_3T *avc4 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc4->type = MafOamSpiMoAttributeType_3_INT16;
	avc4->nrOfValues = 2;
	avc4->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc4->values[0].value.i16 = 5;
	avc4->values[1].value.i16 = 6;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestStruct attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 ";
	expectedNotification += "memberName: TestInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 ";
	expectedNotification += "memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 22 ";
	expectedNotification += "memberName: TestInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 5 6 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(103, "TestStruct", "testClassId=1", "", getAvcRef());
	CmCache.setDn(103, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(103, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt8", avc);
	CmCache.addAttr(103, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt16", avc2);
	CmCache.setDn(103, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);
	CmCache.addAttr(103, "TestStruct", "id=TestStruct_1,testClassId=1", "TestInt8", avc3);
	CmCache.addAttr(103, "TestStruct", "id=TestStruct_1,testClassId=1", "TestInt16", avc4);
	CmCache.setDn(103, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: multi value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 2 (2 multi value struct)
 * number of attributes in struct: 2 (2 attribute/1 struct)
 */
TEST (CmCacheTest, ConfigStructTypes4)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	MafMoAttributeValueContainer_3T *avc3 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3->type = MafOamSpiMoAttributeType_3_INT8;
	avc3->nrOfValues = 1;
	avc3->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3->values[0].value.i8 = 22;

	MafMoAttributeValueContainer_3T *avc4 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc4->type = MafOamSpiMoAttributeType_3_INT16;
	avc4->nrOfValues = 2;
	avc4->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc4->values[0].value.i16 = 5;
	avc4->values[1].value.i16 = 6;

	MafMoAttributeValueContainer_3T *avc5 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc5->type = MafOamSpiMoAttributeType_3_UINT64;
	avc5->nrOfValues = 1;
	avc5->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc5->values[0].value.u64 = 33;

	MafMoAttributeValueContainer_3T *avc6 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc6->type = MafOamSpiMoAttributeType_3_STRING;
	avc6->nrOfValues = 1;
	avc6->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc6->values[0].value.theString = strdup("TestString1");

	MafMoAttributeValueContainer_3T *avc7 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc7->type = MafOamSpiMoAttributeType_3_UINT64;
	avc7->nrOfValues = 1;
	avc7->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc7->values[0].value.u64 = 44;

	MafMoAttributeValueContainer_3T *avc8 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc8->type = MafOamSpiMoAttributeType_3_STRING;
	avc8->nrOfValues = 1;
	avc8->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc8->values[0].value.theString = strdup("TestString2");

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestStructA attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 ";
	expectedNotification += "memberName: TestInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 ";
	expectedNotification += "memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 22 ";
	expectedNotification += "memberName: TestInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 5 6 ";
	expectedNotification += "name: TestStructB attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: TestUint64 memberValue: attrType: UINT64 nrOfValues: 1 values: 33 ";
	expectedNotification += "memberName: TestString memberValue: attrType: STRING nrOfValues: 1 values: TestString1 ";
	expectedNotification += "memberName: TestUint64 memberValue: attrType: UINT64 nrOfValues: 1 values: 44 ";
	expectedNotification += "memberName: TestString memberValue: attrType: STRING nrOfValues: 1 values: TestString2 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(104, "TestStructA", "testClassId=1", "", getAvcRef());
	CmCache.addAttr(104, "TestStructB", "testClassId=1", "", getAvcRef());
	CmCache.setDn(104, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(104, "TestStructA", "id=TestStructA_0,testClassId=1", "TestInt8", avc);
	CmCache.addAttr(104, "TestStructA", "id=TestStructA_0,testClassId=1", "TestInt16", avc2);
	CmCache.setDn(104, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(104, "TestStructA", "id=TestStructA_1,testClassId=1", "TestInt8", avc3);
	CmCache.addAttr(104, "TestStructA", "id=TestStructA_1,testClassId=1", "TestInt16", avc4);
	CmCache.setDn(104, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(104, "TestStructB", "id=TestStructB_0,testClassId=1", "TestUint64", avc5);
	CmCache.addAttr(104, "TestStructB", "id=TestStructB_0,testClassId=1", "TestString", avc6);
	CmCache.setDn(104, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(104, "TestStructB", "id=TestStructB_1,testClassId=1", "TestUint64", avc7);
	CmCache.addAttr(104, "TestStructB", "id=TestStructB_1,testClassId=1", "TestString", avc8);
	CmCache.setDn(104, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: multi value struct type
 * Event types: attribute value change
 * number of DNs: 2 (1 attribute / 1 DN)
 * number of struct attributes: 2 (2 multi value struct)
 * number of attributes in struct: 2 (2 attribute/1 struct)
 */
TEST (CmCacheTest, ConfigStructTypes5)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	MafMoAttributeValueContainer_3T *avc3 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3->type = MafOamSpiMoAttributeType_3_INT8;
	avc3->nrOfValues = 1;
	avc3->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3->values[0].value.i8 = 22;

	MafMoAttributeValueContainer_3T *avc4 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc4->type = MafOamSpiMoAttributeType_3_INT16;
	avc4->nrOfValues = 2;
	avc4->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc4->values[0].value.i16 = 5;
	avc4->values[1].value.i16 = 6;

	MafMoAttributeValueContainer_3T *avc5 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc5->type = MafOamSpiMoAttributeType_3_UINT64;
	avc5->nrOfValues = 1;
	avc5->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc5->values[0].value.u64 = 33;

	MafMoAttributeValueContainer_3T *avc6 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc6->type = MafOamSpiMoAttributeType_3_STRING;
	avc6->nrOfValues = 1;
	avc6->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc6->values[0].value.theString = strdup("TestString1");

	MafMoAttributeValueContainer_3T *avc7 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc7->type = MafOamSpiMoAttributeType_3_UINT64;
	avc7->nrOfValues = 1;
	avc7->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc7->values[0].value.u64 = 44;

	MafMoAttributeValueContainer_3T *avc8 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc8->type = MafOamSpiMoAttributeType_3_STRING;
	avc8->nrOfValues = 1;
	avc8->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc8->values[0].value.theString = strdup("TestString2");

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,Funny=7 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: FunnyStructA attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: FunnyInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 ";
	expectedNotification += "memberName: FunnyInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 ";
	expectedNotification += "memberName: FunnyInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 22 ";
	expectedNotification += "memberName: FunnyInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 5 6 ";
	expectedNotification += "dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestStructB attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: TestUint64 memberValue: attrType: UINT64 nrOfValues: 1 values: 33 ";
	expectedNotification += "memberName: TestString memberValue: attrType: STRING nrOfValues: 1 values: TestString1 ";
	expectedNotification += "memberName: TestUint64 memberValue: attrType: UINT64 nrOfValues: 1 values: 44 ";
	expectedNotification += "memberName: TestString memberValue: attrType: STRING nrOfValues: 1 values: TestString2 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(104, "FunnyStructA", "funnyId=7", "", getAvcRef());
	CmCache.setDn(104, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(104, "TestStructB", "testClassId=1", "", getAvcRef());
	CmCache.setDn(104, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(105, "FunnyStructA", "id=TestStructA_0,funnyId=7", "FunnyInt8", avc);
	CmCache.addAttr(105, "FunnyStructA", "id=TestStructA_0,funnyId=7", "FunnyInt16", avc2);
	CmCache.setDn(105, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(105, "FunnyStructA", "id=TestStructA_1,funnyId=7", "FunnyInt8", avc3);
	CmCache.addAttr(105, "FunnyStructA", "id=TestStructA_1,funnyId=7", "FunnyInt16", avc4);
	CmCache.setDn(105, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(105, "TestStructB", "id=TestStructB_0,testClassId=5", "TestUint64", avc5);
	CmCache.addAttr(105, "TestStructB", "id=TestStructB_0,testClassId=5", "TestString", avc6);
	CmCache.setDn(105, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(105, "TestStructB", "id=TestStructB_1,testClassId=5", "TestUint64", avc7);
	CmCache.addAttr(105, "TestStructB", "id=TestStructB_1,testClassId=5", "TestString", avc8);
	CmCache.setDn(105, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* In this case in COM view the struct attribute is deleted. In IMM view: the referenced object is deleted, and the reference is changed in the other object.
 * Input data: struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: N/A
 * number of simple attributes: N/A
 */
TEST (CmCacheTest, ConfigStructTypes6)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_STRUCT;
	avc->nrOfValues = 0;
	avc->values = NULL;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestStruct attrType: STRUCT nrOfValues: 0 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache

	CmCache.addAttr(106, "TestStruct", "id=TestStruct_0,testClassId=5", "", avc);
	CmCache.setDn(106, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);
	CmCache.setDn(106, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_MoDeleted_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* In this case in COM view one element is deleted from the 2 element multivalue struct
 * Input data: multi value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1 (1 multi value struct)
 * number of attributes in struct: 2 (2 attribute/1 struct)
 */
TEST (CmCacheTest, ConfigStructTypes7)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 ";
	expectedNotification += "memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 ";
	expectedNotification += "memberName: TestInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(107, "TestStruct", "testClassId=1", "", getAvcRef());
	CmCache.setDn(107, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(107, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt8", avc);
	CmCache.addAttr(107, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt16", avc2);
	CmCache.setDn(107, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);
	CmCache.setDn(107, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_MoDeleted_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 0
 */
TEST (CmCacheTest, ConfigStructTypes8)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 0 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(101, "TestStruct", "testClassId=1", "", getAvcRef());
	CmCache.setDn(101, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 0
 */
TEST (CmCacheTest, ConfigStructTypes9)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 0 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(101, "TestStruct", "testClassId=1", "", getAvcRef2());
	CmCache.setDn(101, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 1
 */
TEST (CmCacheTest, ConfigStructTypes10)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 7;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 7 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(101, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt8", avc);
	CmCache.setDn(101, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_MoCreated_1, false);

	CmCache.addAttr(101, "TestStruct", "testClassId=1", "", getAvcRef());
	CmCache.setDn(101, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: single value struct type (without calling "addAttr" with "reference")
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 2
 */
TEST (CmCacheTest, ConfigStructTypes11)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 memberName: TestInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(311, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt8", avc);
	CmCache.addAttr(311, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt16", avc2);
	CmCache.setDn(311, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: single value struct type (without calling "addAttr" with "reference", and calling "setDn" after each "addAttr")
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 2
 */
TEST (CmCacheTest, ConfigStructTypes12)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 memberName: TestInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(311, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt8", avc);
	CmCache.setDn(311, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);
	CmCache.addAttr(311, "TestStruct", "id=TestStruct_0,testClassId=1", "TestInt16", avc2);
	CmCache.setDn(311, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Runtime struct create case: the referenced object is created in IMM.
 * Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 1
 */
TEST (CmCacheTest, RuntimeStructTypes1)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *memberAvc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc->type = MafOamSpiMoAttributeType_3_INT8;
	memberAvc->nrOfValues = 1;
	memberAvc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	memberAvc->values[0].value.i8 = 3;

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_STRUCT;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.structMember = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	avc->values[0].value.structMember->memberName = strdup("TestInt8");
	avc->values[0].value.structMember->memberValue = memberAvc;
	avc->values[0].value.structMember->next = NULL;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=4 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 3 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(0, "TestStruct", "id=TestStruct_0,testClassId=1", "", avc);
	CmCache.setDn(0, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=4", MafOamSpiCmEvent_MoCreated_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Runtime struct modify case: the referenced object is modified in IMM.
 * Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 1
 */
TEST (CmCacheTest, RuntimeStructTypes2)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *memberAvc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc->type = MafOamSpiMoAttributeType_3_INT8;
	memberAvc->nrOfValues = 1;
	memberAvc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	memberAvc->values[0].value.i8 = 3;

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_STRUCT;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.structMember = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	avc->values[0].value.structMember->memberName = strdup("TestInt8");
	avc->values[0].value.structMember->memberValue = memberAvc;
	avc->values[0].value.structMember->next = NULL;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=4 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 3 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(0, "TestStruct", "id=TestStruct_0,testClassId=1", "", avc);
	CmCache.setDn(0, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=4", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Runtime struct delete case: the referenced object is deleted in IMM.
 * Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 1
 */
TEST (CmCacheTest, RuntimeStructTypes3)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *memberAvc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc->type = MafOamSpiMoAttributeType_3_INT8;
	memberAvc->nrOfValues = 1;
	memberAvc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	memberAvc->values[0].value.i8 = 3;

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_STRUCT;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.structMember = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	avc->values[0].value.structMember->memberName = strdup("TestInt8");
	avc->values[0].value.structMember->memberValue = memberAvc;
	avc->values[0].value.structMember->next = NULL;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=4 eventType: AttributeValueChange attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 3 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(0, "TestStruct", "id=TestStruct_0,testClassId=1", "", avc);
	CmCache.setDn(0, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=4", MafOamSpiCmEvent_MoDeleted_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: runtime single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 2
 */
TEST (CmCacheTest, RuntimeStructTypes4)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *memberAvc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc->type = MafOamSpiMoAttributeType_3_INT8;
	memberAvc->nrOfValues = 1;
	memberAvc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	memberAvc->values[0].value.i8 = 3;

	MafMoAttributeValueContainer_3T *memberAvc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc2->type = MafOamSpiMoAttributeType_3_UINT32;
	memberAvc2->nrOfValues = 1;
	memberAvc2->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	memberAvc2->values[0].value.u32 = 9;

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_STRUCT;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));

	// second element of the linked list (the 3rd one is NULL which is the last element)
	MafMoAttributeValueStructMember_3 *S2 = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	S2->memberName = strdup("TestUint32");
	S2->memberValue = memberAvc2;
	S2->next = NULL;

	// head of the linked list
	MafMoAttributeValueStructMember_3 *S1 = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	S1->memberName = strdup("TestInt8");
	S1->memberValue = memberAvc;
	S1->next = S2;
	avc->values[0].value.structMember = S1;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=4 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestStruct attrType: STRUCT nrOfValues: 1 ";
	expectedNotification += "memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 3 ";
	expectedNotification += "memberName: TestUint32 memberValue: attrType: UINT32 nrOfValues: 1 values: 9 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(0, "TestStruct", "id=TestStruct_0,testClassId=1", "", avc);
	CmCache.setDn(0, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=4", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: runtime multi value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 2 (2 attribute/struct)
 */
TEST (CmCacheTest, RuntimeStructTypes5)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *memberAvc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc->type = MafOamSpiMoAttributeType_3_INT8;
	memberAvc->nrOfValues = 2;
	memberAvc->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	memberAvc->values[0].value.i8 = 3;
	memberAvc->values[1].value.i8 = 4;

	MafMoAttributeValueContainer_3T *memberAvc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc2->type = MafOamSpiMoAttributeType_3_UINT32;
	memberAvc2->nrOfValues = 3;
	memberAvc2->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	memberAvc2->values[0].value.u32 = 9;
	memberAvc2->values[1].value.u32 = 10;
	memberAvc2->values[2].value.u32 = 11;

	MafMoAttributeValueContainer_3T *memberAvc3 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc3->type = MafOamSpiMoAttributeType_3_INT8;
	memberAvc3->nrOfValues = 2;
	memberAvc3->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	memberAvc3->values[0].value.i8 = 7;
	memberAvc3->values[1].value.i8 = 8;

	MafMoAttributeValueContainer_3T *memberAvc4 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	memberAvc4->type = MafOamSpiMoAttributeType_3_UINT32;
	memberAvc4->nrOfValues = 3;
	memberAvc4->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	memberAvc4->values[0].value.u32 = 109;
	memberAvc4->values[1].value.u32 = 110;
	memberAvc4->values[2].value.u32 = 111;

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_STRUCT;
	avc->nrOfValues = 2;
	avc->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));

	// multi value struct element 0
	// second element of the linked list (the 3rd one is NULL which is the last element)
	MafMoAttributeValueStructMember_3 *S2 = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	S2->memberName = strdup("TestUint32");
	S2->memberValue = memberAvc2;
	S2->next = NULL;

	// head of the linked list
	MafMoAttributeValueStructMember_3 *S1 = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	S1->memberName = strdup("TestInt8");
	S1->memberValue = memberAvc;
	S1->next = S2;
	avc->values[0].value.structMember = S1;

	// multi value struct element 1
	// second element of the linked list (the 3rd one is NULL which is the last element)
	MafMoAttributeValueStructMember_3 *S4 = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	S4->memberName = strdup("TestUint32");
	S4->memberValue = memberAvc4;
	S4->next = NULL;

	// head of the linked list
	MafMoAttributeValueStructMember_3 *S3 = (MafMoAttributeValueStructMember_3*)calloc(1,sizeof(MafMoAttributeValueStructMember_3));
	S3->memberName = strdup("TestInt8");
	S3->memberValue = memberAvc3;
	S3->next = S4;
	avc->values[1].value.structMember = S3;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestClass=4 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestStruct attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 2 values: 3 4 ";
	expectedNotification += "memberName: TestUint32 memberValue: attrType: UINT32 nrOfValues: 3 values: 9 10 11 ";
	expectedNotification += "memberName: TestInt8 memberValue: attrType: INT8 nrOfValues: 2 values: 7 8 ";
	expectedNotification += "memberName: TestUint32 memberValue: attrType: UINT32 nrOfValues: 3 values: 109 110 111 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(0, "TestStruct", "id=TestStruct_0,testClassId=1", "", avc);
	CmCache.setDn(0, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=4", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Runtime struct create case: only the object is created in IMM which contains the reference(the referenced object is not created).
 * Input data: single value struct type
 * Event types: attribute value change
 * number of DNs: 1
 * number of struct attributes: 1
 * number of attributes in struct: 0
 */
TEST (CmCacheTest, RuntimeStructTypes6)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_STRUCT;
	avc->nrOfValues = 0;
	avc->values = NULL;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,ObjImpComplexClassSv=1 eventType: MoCreated attributes: name: TestStructAttrSv attrType: STRUCT nrOfValues: 0 eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(0, "TestStructAttrSv", "objImpComplexClassSvId=1", "", avc);
	CmCache.setDn(0, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassSv=1", MafOamSpiCmEvent_MoCreated_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: multi value struct and simple type
 * Event types: attribute value change
 * number of DNs: 2
 * number of struct attributes: 2 (2 multi value struct)
 * number of attributes in struct: 2 (2 attribute/1 struct)
 * number of simple attributes: 1
 */
TEST (CmCacheTest, ConfigMixedTypes1)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	MafMoAttributeValueContainer_3T *avc3 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3->type = MafOamSpiMoAttributeType_3_INT8;
	avc3->nrOfValues = 1;
	avc3->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3->values[0].value.i8 = 22;

	MafMoAttributeValueContainer_3T *avc4 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc4->type = MafOamSpiMoAttributeType_3_INT16;
	avc4->nrOfValues = 2;
	avc4->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc4->values[0].value.i16 = 5;
	avc4->values[1].value.i16 = 6;

	MafMoAttributeValueContainer_3T *avc5 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc5->type = MafOamSpiMoAttributeType_3_UINT64;
	avc5->nrOfValues = 1;
	avc5->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc5->values[0].value.u64 = 33;

	MafMoAttributeValueContainer_3T *avc6 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc6->type = MafOamSpiMoAttributeType_3_STRING;
	avc6->nrOfValues = 1;
	avc6->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc6->values[0].value.theString = strdup("TestString1");

	MafMoAttributeValueContainer_3T *avc7 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc7->type = MafOamSpiMoAttributeType_3_UINT64;
	avc7->nrOfValues = 1;
	avc7->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc7->values[0].value.u64 = 44;

	MafMoAttributeValueContainer_3T *avc8 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc8->type = MafOamSpiMoAttributeType_3_STRING;
	avc8->nrOfValues = 1;
	avc8->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc8->values[0].value.theString = strdup("TestString2");

	MafMoAttributeValueContainer_3T *avc9 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc9->type = MafOamSpiMoAttributeType_3_INT8;
	avc9->nrOfValues = 1;
	avc9->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc9->values[0].value.i8 = 9;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,Funny=7 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: FunnyStructA attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: FunnyInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 ";
	expectedNotification += "memberName: FunnyInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 ";
	expectedNotification += "memberName: FunnyInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 22 ";
	expectedNotification += "memberName: FunnyInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 5 6 ";
	expectedNotification += "dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestSimpleAttr attrType: INT8 nrOfValues: 1 values: 9 ";
	expectedNotification += "name: TestStructB attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: TestUint64 memberValue: attrType: UINT64 nrOfValues: 1 values: 33 ";
	expectedNotification += "memberName: TestString memberValue: attrType: STRING nrOfValues: 1 values: TestString1 ";
	expectedNotification += "memberName: TestUint64 memberValue: attrType: UINT64 nrOfValues: 1 values: 44 ";
	expectedNotification += "memberName: TestString memberValue: attrType: STRING nrOfValues: 1 values: TestString2 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(201, "FunnyStructA", "funnyId=7", "", getAvcRef());
	CmCache.setDn(201, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(201, "TestStructB", "testClassId=1", "", getAvcRef());
	CmCache.setDn(201, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(201, "FunnyStructA", "id=TestStructA_0,funnyId=7", "FunnyInt8", avc);
	CmCache.addAttr(201, "FunnyStructA", "id=TestStructA_0,funnyId=7", "FunnyInt16", avc2);
	CmCache.setDn(201, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(201, "FunnyStructA", "id=TestStructA_1,funnyId=7", "FunnyInt8", avc3);
	CmCache.addAttr(201, "FunnyStructA", "id=TestStructA_1,funnyId=7", "FunnyInt16", avc4);
	CmCache.setDn(201, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(201, "TestStructB", "id=TestStructB_0,testClassId=5", "TestUint64", avc5);
	CmCache.addAttr(201, "TestStructB", "id=TestStructB_0,testClassId=5", "TestString", avc6);
	CmCache.setDn(201, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(201, "TestStructB", "id=TestStructB_1,testClassId=5", "TestUint64", avc7);
	CmCache.addAttr(201, "TestStructB", "id=TestStructB_1,testClassId=5", "TestString", avc8);
	CmCache.setDn(201, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(201, "TestSimpleAttr", "", "", avc9);
	CmCache.setDn(201, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: multi value struct and simple type
 * Event types: attribute value change
 * number of DNs: 2
 * number of struct attributes: 2 (2 multi value struct)
 * number of attributes in struct: 2 (2 attribute/1 struct)
 * number of simple attributes: 1
 */
TEST (CmCacheTest, ConfigMixedTypes2)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc->type = MafOamSpiMoAttributeType_3_INT8;
	avc->nrOfValues = 1;
	avc->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc->values[0].value.i8 = 11;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT16;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i16 = 3;
	avc2->values[1].value.i16 = 4;

	MafMoAttributeValueContainer_3T *avc3 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3->type = MafOamSpiMoAttributeType_3_INT8;
	avc3->nrOfValues = 1;
	avc3->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3->values[0].value.i8 = 22;

	MafMoAttributeValueContainer_3T *avc4 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc4->type = MafOamSpiMoAttributeType_3_INT16;
	avc4->nrOfValues = 2;
	avc4->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	avc4->values[0].value.i16 = 5;
	avc4->values[1].value.i16 = 6;

	MafMoAttributeValueContainer_3T *avc5 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc5->type = MafOamSpiMoAttributeType_3_UINT64;
	avc5->nrOfValues = 1;
	avc5->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc5->values[0].value.u64 = 33;

	MafMoAttributeValueContainer_3T *avc6 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc6->type = MafOamSpiMoAttributeType_3_STRING;
	avc6->nrOfValues = 1;
	avc6->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc6->values[0].value.theString = strdup("TestString1");

	MafMoAttributeValueContainer_3T *avc7 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc7->type = MafOamSpiMoAttributeType_3_UINT64;
	avc7->nrOfValues = 1;
	avc7->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc7->values[0].value.u64 = 44;

	MafMoAttributeValueContainer_3T *avc8 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc8->type = MafOamSpiMoAttributeType_3_STRING;
	avc8->nrOfValues = 1;
	avc8->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc8->values[0].value.theString = strdup("TestString2");

	MafMoAttributeValueContainer_3T *avc9 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc9->type = MafOamSpiMoAttributeType_3_INT8;
	avc9->nrOfValues = 1;
	avc9->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc9->values[0].value.i8 = 9;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,Funny=7 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: FunnyStructA attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: FunnyInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 11 ";
	expectedNotification += "memberName: FunnyInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 3 4 ";
	expectedNotification += "memberName: FunnyInt8 memberValue: attrType: INT8 nrOfValues: 1 values: 22 ";
	expectedNotification += "memberName: FunnyInt16 memberValue: attrType: INT16 nrOfValues: 2 values: 5 6 ";
	expectedNotification += "dn: ManagedElement=1,TestClass=5 eventType: AttributeValueChange ";
	expectedNotification += "attributes: name: TestSimpleAttr attrType: INT8 nrOfValues: 1 values: 9 ";
	expectedNotification += "name: TestStructB attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: TestUint64 memberValue: attrType: UINT64 nrOfValues: 1 values: 33 ";
	expectedNotification += "memberName: TestString memberValue: attrType: STRING nrOfValues: 1 values: TestString1 ";
	expectedNotification += "memberName: TestUint64 memberValue: attrType: UINT64 nrOfValues: 1 values: 44 ";
	expectedNotification += "memberName: TestString memberValue: attrType: STRING nrOfValues: 1 values: TestString2 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(202, "FunnyStructA", "funnyId=7", "", getAvcRef());
	CmCache.setDn(202, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(202, "FunnyStructA", "id=TestStructA_0,funnyId=7", "FunnyInt8", avc);
	CmCache.addAttr(202, "FunnyStructA", "id=TestStructA_0,funnyId=7", "FunnyInt16", avc2);
	CmCache.setDn(202, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(202, "FunnyStructA", "id=TestStructA_1,funnyId=7", "FunnyInt8", avc3);
	CmCache.addAttr(202, "FunnyStructA", "id=TestStructA_1,funnyId=7", "FunnyInt16", avc4);
	CmCache.setDn(202, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,Funny=7", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(202, "TestSimpleAttr", "", "", avc9);
	CmCache.setDn(202, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(202, "TestStructB", "testClassId=1", "", getAvcRef());
	CmCache.setDn(202, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(202, "TestStructB", "id=TestStructB_0,testClassId=5", "TestUint64", avc5);
	CmCache.addAttr(202, "TestStructB", "id=TestStructB_0,testClassId=5", "TestString", avc6);
	CmCache.setDn(202, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, false);

	CmCache.addAttr(202, "TestStructB", "id=TestStructB_1,testClassId=5", "TestUint64", avc7);
	CmCache.addAttr(202, "TestStructB", "id=TestStructB_1,testClassId=5", "TestString", avc8);
	CmCache.setDn(202, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestClass=5", MafOamSpiCmEvent_AttributeValueChange_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: a mix of simple/multi value structs and simple types
 * Event types: mix of operations
 * number of DNs: N/A
 * number of struct attributes: N/A
 * number of attributes in struct: N/A
 * number of simple attributes: N/A
 */
TEST (CmCacheTest, ConfigMixedTypes3)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc1 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc1->type = MafOamSpiMoAttributeType_3_INT16;
	avc1->nrOfValues = 0;
	avc1->values = NULL;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_INT8;
	avc2->nrOfValues = 1;
	avc2->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.i8 = 1;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation dn: ManagedElement=1,TestMixClass=3 eventType: MoCreated attributes: ";

	expectedNotification += "name: testStructAttrMv attrType: STRUCT nrOfValues: 0 ";
	expectedNotification += "name: testStructAttrMv2 attrType: STRUCT nrOfValues: 0 ";
	expectedNotification += "name: testStructAttrMv3 attrType: STRUCT nrOfValues: 0 ";
	expectedNotification += "name: testStructAttrSv attrType: STRUCT nrOfValues: 0 ";
	expectedNotification += "name: testStructAttrSv2 attrType: STRUCT nrOfValues: 0 ";
	expectedNotification += "name: testStructAttrSv3 attrType: STRUCT nrOfValues: 0 ";

	expectedNotification += "name: testWritableInt16 attrType: INT16 nrOfValues: 0 values: ";
	expectedNotification += "name: testWritableInt8 attrType: INT8 nrOfValues: 1 values: 1 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	CmCache.addAttr(203, "testWritableInt16", "testMixClassId=3", "", avc1);
	CmCache.addAttr(203, "testWritableInt8",  "testMixClassId=3", "", avc2);
	CmCache.addAttr(203, "testStructAttrSv3", "testMixClassId=3", "", getAvcRef());
	CmCache.addAttr(203, "testStructAttrSv2", "testMixClassId=3", "", getAvcRef());
	CmCache.addAttr(203, "testStructAttrSv",  "testMixClassId=3", "", getAvcRef());
	CmCache.addAttr(203, "testStructAttrMv3", "testMixClassId=3", "", getAvcRef());
	CmCache.addAttr(203, "testStructAttrMv2", "testMixClassId=3", "", getAvcRef());
	CmCache.addAttr(203, "testStructAttrMv",  "testMixClassId=3", "", getAvcRef());

	CmCache.setDn(203, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,TestMixClass=3", MafOamSpiCmEvent_MoCreated_1, true);

	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: multi value struct and simple type
 * Event types: attribute value change
 * number of DNs: 2
 * number of struct attributes: 2 (2 multi value struct)
 * number of attributes in struct: 2 (2 attribute/1 struct)
 * number of simple attributes: 1
 */
TEST (CmCacheTest, ConfigMixedTypes4)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc1 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc1->type = MafOamSpiMoAttributeType_3_INT8;
	avc1->nrOfValues = 1;
	avc1->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc1->values[0].value.i8 = 1;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_UINT8;
	avc2->nrOfValues = 1;
	avc2->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.u8 = 2;

	MafMoAttributeValueContainer_3T *avc3_1 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3_1->type = MafOamSpiMoAttributeType_3_UINT8;
	avc3_1->nrOfValues = 1;
	avc3_1->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3_1->values[0].value.u8 = 31;

	MafMoAttributeValueContainer_3T *avc3_2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3_2->type = MafOamSpiMoAttributeType_3_UINT8;
	avc3_2->nrOfValues = 1;
	avc3_2->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3_2->values[0].value.u8 = 32;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation ";

	expectedNotification += "dn: ManagedElement=1,ObjImpComplexClassMv=3 eventType: MoCreated ";
	expectedNotification += "attributes: name: TestStructAttrMv attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: testWritableAttrUint8 memberValue: attrType: UINT8 nrOfValues: 1 values: 32 ";
	expectedNotification += "memberName: testWritableAttrUint8 memberValue: attrType: UINT8 nrOfValues: 1 values: 31 ";

	expectedNotification += "dn: ManagedElement=1,ObjImpComplexClassSv=3 eventType: MoCreated ";
	expectedNotification += "attributes: name: TestStructAttrSv attrType: STRUCT nrOfValues: 1 ";
	expectedNotification += "memberName: testWritableAttrUint8 memberValue: attrType: UINT8 nrOfValues: 1 values: 2 ";

	expectedNotification += "dn: ManagedElement=1,ObjImpTestClass=3 eventType: MoCreated ";
	expectedNotification += "attributes: name: TestInt8 attrType: INT8 nrOfValues: 1 values: 1 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	// 1. object
	CmCache.addAttr(204, "TestInt8", "", "", avc1);
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpTestClass=3", MafOamSpiCmEvent_MoCreated_1, false);

	// 2. object
	CmCache.addAttr(204, "TestStructAttrSv", "objImpComplexClassSvId=3", "", getAvcRef());
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassSv=3", MafOamSpiCmEvent_MoCreated_1, false);

	CmCache.addAttr(204, "TestStructAttrSv", "id=TestStructAttrSv_0,objImpComplexClassSvId=3", "testWritableAttrUint8", avc2);
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassSv=3", MafOamSpiCmEvent_MoCreated_1, false);

	// 3. object
	CmCache.addAttr(204, "TestStructAttrMv", "objImpComplexClassMvId=3", "", getAvcRef());
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=3", MafOamSpiCmEvent_MoCreated_1, false);

	CmCache.addAttr(204, "TestStructAttrMv", "id=TestStructAttrMv_1,objImpComplexClassMvId=3", "testWritableAttrUint8", avc3_1);
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=3", MafOamSpiCmEvent_MoCreated_1, false);

	CmCache.addAttr(204, "TestStructAttrMv", "id=TestStructAttrMv_0,objImpComplexClassMvId=3", "testWritableAttrUint8", avc3_2);
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=3", MafOamSpiCmEvent_MoCreated_1, true);
	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: multi value struct and simple type
 * Event types: attribute value change
 * number of DNs: 2
 * number of struct attributes: 2 (2 multi value struct)
 * number of attributes in struct: 2 (2 attribute/1 struct)
 * number of simple attributes: 1
 */
TEST (CmCacheTest, ConfigMixedTypes5)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	// create input AVC

	MafMoAttributeValueContainer_3T *avc1 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc1->type = MafOamSpiMoAttributeType_3_INT8;
	avc1->nrOfValues = 1;
	avc1->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc1->values[0].value.i8 = 1;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_UINT8;
	avc2->nrOfValues = 1;
	avc2->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.u8 = 2;

	MafMoAttributeValueContainer_3T *avc3_1 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3_1->type = MafOamSpiMoAttributeType_3_UINT8;
	avc3_1->nrOfValues = 1;
	avc3_1->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3_1->values[0].value.u8 = 31;

	MafMoAttributeValueContainer_3T *avc3_2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3_2->type = MafOamSpiMoAttributeType_3_UINT8;
	avc3_2->nrOfValues = 1;
	avc3_2->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3_2->values[0].value.u8 = 32;

	// Set expected notification
	std::string expectedNotification = "cmEventNotify(): txHandle: 0 sourceIndicator: ManagementOperation ";

	expectedNotification += "dn: ManagedElement=1,ObjImpComplexClassMv=3 eventType: MoCreated ";
	expectedNotification += "attributes: name: TestStructAttrMv attrType: STRUCT nrOfValues: 2 ";
	expectedNotification += "memberName: testWritableAttrUint8 memberValue: attrType: UINT8 nrOfValues: 1 values: 32 ";
	expectedNotification += "memberName: testWritableAttrUint8 memberValue: attrType: UINT8 nrOfValues: 1 values: 31 ";

	expectedNotification += "dn: ManagedElement=1,ObjImpComplexClassSv=3 eventType: MoCreated ";
	expectedNotification += "attributes: name: TestStructAttrSv attrType: STRUCT nrOfValues: 1 ";
	expectedNotification += "memberName: testWritableAttrUint8 memberValue: attrType: UINT8 nrOfValues: 1 values: 2 ";

	expectedNotification += "dn: ManagedElement=1,ObjImpTestClass=3 eventType: MoCreated ";
	expectedNotification += "attributes: name: TestInt8 attrType: INT8 nrOfValues: 1 values: 1 ";
	expectedNotification += "eventTime: 1122334455000000000";

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	// Call the interface of CM Cache
	// 1. object
	CmCache.addAttr(204, "TestInt8", "", "", avc1);
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpTestClass=3", MafOamSpiCmEvent_MoCreated_1, false);

	// 2. object
	CmCache.addAttr(204, "TestStructAttrSv", "objImpComplexClassSvId=3", "", getAvcRef());
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassSv=3", MafOamSpiCmEvent_MoCreated_1, false);

	CmCache.addAttr(204, "TestStructAttrSv", "id=TestStructAttrSv_0,objImpComplexClassSvId=3", "testWritableAttrUint8", avc2);
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassSv=3", MafOamSpiCmEvent_MoCreated_1, false);

	// 3. object
	CmCache.addAttr(204, "TestStructAttrMv", "objImpComplexClassMvId=3", "", getAvcRef());
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=3", MafOamSpiCmEvent_MoCreated_1, false);

	CmCache.addAttr(204, "TestStructAttrMv", "id=TestStructAttrMv_1,objImpComplexClassMvId=3", "testWritableAttrUint8", avc3_1);
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=3", MafOamSpiCmEvent_MoCreated_1, false);

	CmCache.addAttr(204, "TestStructAttrMv", "id=TestStructAttrMv_0,objImpComplexClassMvId=3", "testWritableAttrUint8", avc3_2);
	CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=3", MafOamSpiCmEvent_MoCreated_1, true);
	// Evaluate the result
	bool testResult = compareReceivedToExpected(expectedNotification);
	ASSERT_TRUE(testResult);
}

/* Input data: a mix of simple/multi value structs and simple types
 * Event types: mix of operations
 * number of DNs: 30
 * number of struct attributes: N/A
 * number of attributes in struct: N/A
 * number of simple attributes: N/A
 */
TEST (CmCacheTest, maxSAMemoryForCMEvents_positive)
{
	// Create CM Cache instance
	CmNotificationCache CmCache;

	MafMgmtSpiInterfacePortal_3T portal_MAF_NULL;
	cmEventHandler = new CmEventHandler(portal_MAF_NULL, subscriberFunction_unittest, unsubscriberFunction_unittest);

	// Set NULL to this global pointer, since earlier tests could write to this.
	CmEvent = NULL;

	SaUint64T CmCacheMemory = 0;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc1 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc1->type = MafOamSpiMoAttributeType_3_INT8;
	avc1->nrOfValues = 1;
	avc1->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc1->values[0].value.i8 = 1;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc2->type = MafOamSpiMoAttributeType_3_UINT8;
	avc2->nrOfValues = 1;
	avc2->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.u8 = 2;

	MafMoAttributeValueContainer_3T *avc3_1 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3_1->type = MafOamSpiMoAttributeType_3_UINT8;
	avc3_1->nrOfValues = 1;
	avc3_1->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3_1->values[0].value.u8 = 31;

	MafMoAttributeValueContainer_3T *avc3_2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	avc3_2->type = MafOamSpiMoAttributeType_3_UINT8;
	avc3_2->nrOfValues = 1;
	avc3_2->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	avc3_2->values[0].value.u8 = 32;

	for(int i = 0; i < 31; i++)
	{
		std::ostringstream os;
		os << i;
		std::string instance = os.str();

		// Call the interface of CM Cache
		// 1. object
		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.addAttr(204, "TestInt8", "", "", avc1);

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpTestClass=" + instance, MafOamSpiCmEvent_MoCreated_1, false);

		// 2. object
		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.addAttr(204, "TestStructAttrSv", "objImpComplexClassSvId=" + instance, "", getAvcRef());

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassSv=" + instance, MafOamSpiCmEvent_MoCreated_1, false);

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.addAttr(204, "TestStructAttrSv", "id=TestStructAttrSv_0,objImpComplexClassSvId=" + instance, "testWritableAttrUint8", avc2);

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassSv=" + instance, MafOamSpiCmEvent_MoCreated_1, false);

		// 3. object
		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.addAttr(204, "TestStructAttrMv", "objImpComplexClassMvId=" + instance, "", getAvcRef());

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=" + instance, MafOamSpiCmEvent_MoCreated_1, false);

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.addAttr(204, "TestStructAttrMv", "id=TestStructAttrMv_1,objImpComplexClassMvId=" + instance, "testWritableAttrUint8", avc3_1);

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=" + instance, MafOamSpiCmEvent_MoCreated_1, false);

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.addAttr(204, "TestStructAttrMv", "id=TestStructAttrMv_0,objImpComplexClassMvId=" + instance, "testWritableAttrUint8", avc3_2);

		CmCacheMemory = CmCache.getMemory();
		if(receivedOverflowEvent) break;
		CmCache.setDn(204, MafOamSpiCmEvent_ManagementOperation_1, "ManagedElement=1,ObjImpComplexClassMv=" + instance, MafOamSpiCmEvent_MoCreated_1, false);

		CmCacheMemory = CmCache.getMemory();
	}
	// Evaluate the result
	bool testResult = (receivedOverflowEvent && (CmCacheMemory <= maxSAMemoryForCMEvents));
	ASSERT_TRUE(testResult);
	receivedOverflowEvent = false;
}

TEST (CmCacheTest, memgetAVC)
{
	SaUint64T allocatedMemory = 0;

	// create input AVC
	MafMoAttributeValueContainer_3T *avc1 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	allocatedMemory = sizeof(MafMoAttributeValueContainer_3T);
	avc1->type = MafOamSpiMoAttributeType_3_INT8;
	avc1->nrOfValues = 1;
	avc1->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	allocatedMemory += sizeof(MafMoAttributeValue_3T);
	avc1->values[0].value.i8 = 1;
	ASSERT_TRUE(allocatedMemory == memgetAVC(avc1));
	freeAVC_unittest(avc1);
	free(avc1);
	avc1 = NULL;

	MafMoAttributeValueContainer_3T *avc2 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	allocatedMemory = sizeof(MafMoAttributeValueContainer_3T);
	avc2->type = MafOamSpiMoAttributeType_3_UINT64;
	avc2->nrOfValues = 2;
	avc2->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	allocatedMemory += (2 * sizeof(MafMoAttributeValue_3T));
	avc2->values[0].value.u64 = 11;
	avc2->values[1].value.u64 = 22;
	ASSERT_TRUE(allocatedMemory == memgetAVC(avc2));
	freeAVC_unittest(avc2);
	free(avc2);
	avc2 = NULL;

	MafMoAttributeValueContainer_3T *avc3 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	allocatedMemory = sizeof(MafMoAttributeValueContainer_3T);
	avc3->type = MafOamSpiMoAttributeType_3_INT8;
	avc3->nrOfValues = 3;
	avc3->values = (MafMoAttributeValue_3T*)calloc(3,sizeof(MafMoAttributeValue_3T));
	allocatedMemory += ( 3 * sizeof(MafMoAttributeValue_3T));
	avc3->values[0].value.i8 = 3;
	avc3->values[1].value.i8 = 4;
	avc3->values[2].value.i8 = 5;
	ASSERT_TRUE(allocatedMemory == memgetAVC(avc3));
	freeAVC_unittest(avc3);
	free(avc3);
	avc3 = NULL;

	MafMoAttributeValueContainer_3T *avc4 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	allocatedMemory = sizeof(MafMoAttributeValueContainer_3T);
	avc4->type = MafOamSpiMoAttributeType_3_STRING;
	avc4->nrOfValues = 1;
	avc4->values = (MafMoAttributeValue_3T*)calloc(1,sizeof(MafMoAttributeValue_3T));
	allocatedMemory += sizeof(MafMoAttributeValue_3T);
	avc4->values[0].value.theString = strdup("Winter is Coming");
	allocatedMemory += ( strlen("Winter is Coming") * sizeof(char));
	ASSERT_TRUE(allocatedMemory == memgetAVC(avc4));
	freeAVC_unittest(avc4);
	free(avc4);
	avc4 = NULL;

	MafMoAttributeValueContainer_3T *avc5 = (MafMoAttributeValueContainer_3T*)calloc(1,sizeof(MafMoAttributeValueContainer_3T));
	allocatedMemory = sizeof(MafMoAttributeValueContainer_3T);
	avc5->type = MafOamSpiMoAttributeType_3_STRING;
	avc5->nrOfValues = 2;
	avc5->values = (MafMoAttributeValue_3T*)calloc(2,sizeof(MafMoAttributeValue_3T));
	allocatedMemory += (2 * sizeof(MafMoAttributeValue_3T));
	avc5->values[0].value.theString = strdup("Fire and Blood");
	allocatedMemory += (strlen("Fire and Blood") * sizeof(char));
	avc5->values[1].value.theString = strdup("Unbowed, Unbent, Unbroken");
	allocatedMemory += (strlen("Unbowed, Unbent, Unbroken") * sizeof(char));
	ASSERT_TRUE(allocatedMemory == memgetAVC(avc5));
	freeAVC_unittest(avc5);
	free(avc5);
	avc5 = NULL;
}
