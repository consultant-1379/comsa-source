#include <cstdio>
#include <cstdarg>
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <vector>
#include <map>

#include <iostream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "PmConsumerInterface.hxx"
#include "PmShowCounters.hxx"

#include "PmConsumer.hxx"
#include <ComOamSpiPm_2.h>
#include <saPmProducer.h>
#include <saPmConsumer.h>
#include "ComSA.h"
#include "pmt_sa.mock"

using namespace std;

using ::testing::NiceMock;
using ::testing::Ge;
using ::testing::_;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::Mock;
using ::testing::DoAll;
using ::testing::StrEq;
using ::testing::Eq;
using ::testing::IsNull;

#define jobCount 2
#define measurementNameCount 3
#define measObjCount 2
const char* listOfObjName[] = {"Obj1","Obj2","Obj3"};
#define time  (10*SA_TIME_ONE_SECOND)

/**
 *  This structure is used to set expectation on the iterator interface. <code>iteratorNext()</code>
 *  will return <code>returnCode</code> and set the out parameter to <code>info</code>
 */

struct Testdata
{
	SaAisErrorT returnCode;
	SaPmCIteratorInfoT info;
};

struct Testdata2
{
	SaAisErrorT returnCode;
	SaPmCIteratorInfoT_2 info;
	SaPmAggregatedValueT values[10];
};

struct Testdata4
{
	SaAisErrorT returnCode;
	SaConstStringT instance;
	SaConstStringT *jobNames;
	SaConstStringT *measurementTypes;
	SaTimeT timeout;
	SaPmCInstanceValuesT ***instanceValues;
};

typedef struct {
	//input
	void* pmHandle;
	const char* instance;
	const char** measTypes;
	const char** pmGroups;
	unsigned int requestId;
} info_gMV2;

typedef struct
{
	SaConstStringT instances;
	SaConstStringT *measurementType;
	SaConstStringT *pmGroups;
	SaConstStringT *outputNames;
} info_gMD;

/**
 * setExpectation will set expectations on the mock object called <em>mock</em>. First, we set an expectation that iterator
 * Initialize will be called. The return code is expected to be the value of <em>iiReturn<em>. If iiReturn equals SA_AIS_OK,
 * more expectations will be set, one for each element in <em>data<em>. If any of those elements contains a fault return value,
 * we will expect any number of calls of iteratorNext, all returning error codes. Finally we will expect a call to iteratorFinalize()
 *
 * @parameter[in,out] mock The google mock object that we are setting expectations on
 * @parameter[in] info the callback data that is coming from Core MW
 * @parameter[in] data array of <em>Testdata</em>, see description of TestData
 * @parameter[in] size number of elements in <em>data</em>
 * @parameter[in] iiReturn The return code that iteratorInitilize() will return
 */

void setExpectations(MockPmConsumerInterface& mock, SaPmCCallbackInfoT& info, Testdata data[], int size, SaAisErrorT iiReturn)
{
	InSequence s;
	SaPmCIteratorHandleT iHandle = (SaPmCIteratorHandleT) 666;

	EXPECT_CALL(mock, iteratorInitialize(info.jobHandler, NotNull())).WillOnce(
			DoAll(SetArgPointee<1>(iHandle), Return(iiReturn))).RetiresOnSaturation();

	if (iiReturn != SA_AIS_OK)
	{
		return; // expect no more calls on this interface, iteratorFinalize can not be called, because we do not have any handle
	}
	for (int i=0; i<size; i++)
	{
		// expect iteratorNext() with return code and return value from data[i]
		EXPECT_CALL(mock, iteratorNext(iHandle, NotNull())).WillOnce(DoAll(
				SetArgPointee<1>(data[i].info),
				Return(data[i].returnCode))).RetiresOnSaturation();

		// after any iterator fault, expect only new iterator faults
		if (data[i].returnCode != SA_AIS_OK)
		{
			EXPECT_CALL(mock, iteratorNext(iHandle, NotNull())).WillRepeatedly(DoAll(
					SetArgPointee<1>(data[i].info),
					Return(SA_AIS_ERR_NOT_EXIST)));
		}
	}

	// The iterator should always be finalized (except if iterator initialization
	// fails, that is handled by an early return of this function)
	EXPECT_CALL(mock, iteratorFinalize(iHandle)).WillOnce(Return(SA_AIS_OK)).RetiresOnSaturation();
}


void setExpectations2(MockPmConsumerInterface& mock, SaPmCCallbackInfoT& info, Testdata2 data[], int size, SaAisErrorT iiReturn)
{
	InSequence s;
	SaPmCIteratorHandleT_2 iHandle = (SaPmCIteratorHandleT_2) 666;
//	SaPmCAggregatedValuesT dataValues;

	EXPECT_CALL(mock, iteratorInitialize_2(info.jobHandler, NotNull())).WillOnce(
			DoAll(SetArgPointee<1>(iHandle), Return(iiReturn))).RetiresOnSaturation();

	if (iiReturn != SA_AIS_OK)
		return; // expect no more calls on this interface, iteratorFinalize can not be called, because we do not have any handle

	for (int i = 0; i < size; i++)
	{
		// after any iterator fault, expect only new iterator faults

//	  EXPECT_CALL(mock, iteratorNext_2(iHandle, NotNull(), &dataValues)).WillOnce(DoAll(
	  EXPECT_CALL(mock, iteratorNext_2(iHandle, NotNull(), NotNull())).WillOnce(DoAll(
				SetArgPointee<1>(data[i].info),
				SetArgPointee<2>((SaPmAggregatedValueT*)data[i].values),
				Return(data[i].returnCode))).RetiresOnSaturation();

		if (data[i].returnCode != SA_AIS_OK)
		{
			// expect iteratorNext_2() with return code and return value from data[i]
		  EXPECT_CALL(mock, iteratorNext_2(iHandle, NotNull(), NotNull())).WillRepeatedly(DoAll(
																					   SetArgPointee<1>(data[i].info),
				SetArgPointee<2>((SaPmAggregatedValueT*)data[i].values),
																					   Return(data[i].returnCode)));
		}
	}

	// The iterator should always be finalized (except if iterator initialization
	// fails, that is handled by an early return of this function)
	EXPECT_CALL(mock, iteratorFinalize_2(iHandle)).WillOnce(Return(SA_AIS_OK)).RetiresOnSaturation();
}

void setExpectations_isFeatureEnabled(MockPmConsumerInterface& mock, SaBoolT value, SaAisErrorT ret)
{
	EXPECT_CALL (mock, saPmCIsFeatureEnabled(_)).WillRepeatedly(DoAll(SetArgPointee<0>(value),Return(ret)));
}

void setExpectations_getInstanceValues_2(MockPmConsumerInterface& mock, info_gMV2 info, SaPmCInstanceValuesT_2 **instanceValues, SaAisErrorT ret1, SaAisErrorT ret2)
{
	//TODO:support for sa-ais-invalid-param err.
	EXPECT_CALL (mock, saPmCGetInstanceValues_2(_,_,_,_,_,_,_,_)).WillRepeatedly(DoAll(SetArgPointee<5>(info.requestId),
	                                                                                 SetArgPointee<6>(instanceValues),Return(ret1)));

	EXPECT_CALL (mock, saPmCInstanceValuesMemoryFree_2(_,_,_)).WillRepeatedly(Return(ret2));
}

void setExpectations_getMeasurementTypeNames(MockPmConsumerInterface& mock, info_gMD data, SaAisErrorT iiReturn)
{
	InSequence s;
	EXPECT_CALL(mock, saPmCGetMeasurementTypeNames(NotNull(),StrEq("RootX=1"),NotNull(),NotNull(),Eq(500 * SA_TIME_ONE_MILLISECOND),NotNull())).WillOnce(
			DoAll(SetArgPointee<5>(data.outputNames),
					Return(iiReturn)));

	EXPECT_CALL(mock, saPmCNamesMemoryFree(NotNull(),NotNull())).WillRepeatedly(Return(iiReturn));
}

void setExpectations_getPmGroupIds(MockPmConsumerInterface& mock, info_gMD data, SaAisErrorT iiReturn)
{
	EXPECT_CALL(mock, saPmCGetPmGroupNames(NotNull(),StrEq("RootX=1"),NotNull(),NotNull(),Eq(500*1000000LL),NotNull())).WillRepeatedly(
					DoAll(SetArgPointee<5>(data.outputNames),
							Return(iiReturn)));
}

void setExpectations_getInstanceNames(MockPmConsumerInterface& mock, SaConstStringT *measuredObjects1, SaAisErrorT rc)
{
	InSequence s;
	EXPECT_CALL(mock, saPmCGetInstanceNames(NotNull(), NotNull(),_,_,Eq(500*1000000LL),NotNull()))
					.WillRepeatedly(DoAll(SetArgPointee<5>(measuredObjects1),Return(rc)));

	EXPECT_CALL(mock,saPmCNamesMemoryFree(NotNull(),NotNull())).WillRepeatedly(Return(rc));
}

/**
 * validate() checks that <em><result</em> is equal to what we programmed the mock object with (<em>data</em> and <em>callback</em>).
 *
 * @parameter[in] data The test data that was sent to setExpectations() earlier
 * @parameter[in] size The number of elements in <em>data</em>
 * @parameter[in] result The data returned from the code we are testing, the data that is to be delivered to COM
 * @parameter[in] callback The callback data that is coming from Core MW
 */
void validate(Testdata* data, int size, ComOamSpiPmGpData_2T* result, SaPmCCallbackInfoT& callback)
{
	//ASSERT_EQ(result->gpStartTimestampInNanoSeconds, callback.gpStartTimestamp);
	//ASSERT_EQ(result->gpEndTimestampInNanoSeconds, callback.gpEndTimestamp);
	//ASSERT_STREQ(result->jobId, callback.jobname);

	/*
	 * These indexes except 'value' should be divided by two, they are increased on both entering and exiting 'tags'
	 */
	unsigned moclass = 0;
	unsigned ldn = 0;
	unsigned value = 0;

	ASSERT_NE((void*)0, result);

	for (unsigned i = 0; i<size; i++)
	{
	  	if (data[i].returnCode != SA_AIS_OK)
			break;

		if (data[i].info.objectType == PMF_JOB_ITERATOR_OBJECT_MOCLASS)
		{
			printf("objectType=PMF_JOB_ITERATOR_OBJECT_MOCLASS\n");
			printf("Expected:\tnodeName=%s\n", data[i].info.nodeName);
			printf("Received:\tmeasObjClass=%s\n", result->measObjClasses[moclass/2].measObjClass);
			ASSERT_STREQ(data[i].info.nodeName, result->measObjClasses[moclass/2].measObjClass);
			moclass++;
			ldn = 0;
			value = 0;
		}
		else if (data[i].info.objectType == PMF_JOB_ITERATOR_OBJECT_LDN)
		{
			printf("\nobjectType=PMF_JOB_ITERATOR_OBJECT_LDN\n");
			printf("Expected:\tnodeName=%s\n", data[i].info.nodeName);
			printf("Received:\tmeasObjLDN%s\n", result->measObjClasses[moclass/2].instances[ldn/2].measObjLDN);
			ASSERT_STREQ(data[i].info.nodeName, result->measObjClasses[moclass/2].instances[ldn/2].measObjLDN);
			ldn++;
			value = 0;
		}
		else if (data[i].info.objectType == PMF_JOB_ITERATOR_OBJECT_VALUE)
		{
			printf("\nobjectType=PMF_JOB_ITERATOR_OBJECT_VALUE\n");
			ASSERT_STREQ(data[i].info.nodeName, result->measObjClasses[moclass/2].instances[ldn/2].values[value/2].measType);
			EXPECT_EQ(data[i].info.valueInfo.isFloat, result->measObjClasses[moclass/2].instances[ldn/2].values[value].valueType == ComOamSpiPmValueType_2_FLOAT);
			ASSERT_EQ(data[i].info.valueInfo.isSuspect, result->measObjClasses[moclass/2].instances[ldn/2].values[value].isSuspect);
			if (data[i].info.valueInfo.isFloat)
			{
				printf("Expected:\tnodeName=%s,\tisSuspect=%d,\tfloatVal=%f\n",
					   data[i].info.nodeName, data[i].info.valueInfo.isSuspect, data[i].info.valueInfo.value.floatVal);
				printf("Received:\tmeasType=%s,\tisSuspect=%d,\tfloatVal=%f\n",
					   result->measObjClasses[moclass/2].instances[ldn/2].values[value/2].measType,
					   result->measObjClasses[moclass/2].instances[ldn/2].values[value].isSuspect,
					   result->measObjClasses[moclass/2].instances[ldn/2].values[value].value.floatVal);
				ASSERT_EQ(data[i].info.valueInfo.value.floatVal, result->measObjClasses[moclass/2].instances[ldn/2].values[value].value.floatVal);
			}
			else
			{
				 printf("Expected:\tnodeName=%s,\tisSuspect=%d,\tintVal=%d\n",
						data[i].info.nodeName, data[i].info.valueInfo.isSuspect, data[i].info.valueInfo.value.intVal);
				 printf("Received:\tmeasType=%s,\tisSuspect=%d,\tintVal=%d\n",
						result->measObjClasses[moclass/2].instances[ldn/2].values[value/2].measType,
						result->measObjClasses[moclass/2].instances[ldn/2].values[value].isSuspect,
						result->measObjClasses[moclass/2].instances[ldn/2].values[value].value.intVal);
				 ASSERT_EQ(data[i].info.valueInfo.value.intVal, result->measObjClasses[moclass/2].instances[ldn/2].values[value].value.intVal);
			}
			value++;
		}
		else
		{
			ASSERT_EQ(2 + 2, 5); // this should not happen
		}
	}
}

/**
 * validate() checks that <em><result</em> is equal to what we programmed the mock object with (<em>data</em> and <em>callback</em>).
 *
 * @parameter[in] data The test data that was sent to setExpectations() earlier
 * @parameter[in] size The number of elements in <em>data</em>
 * @parameter[in] result The data returned from the code we are testing, the data that is to be delivered to COM
 * @parameter[in] callback The callback data that is coming from Core MW
 */
void validate2(Testdata2* data, int size, ComOamSpiPmGpData_2T* result, SaPmCCallbackInfoT& callback)
{
	//ASSERT_EQ(result->gpStartTimestampInNanoSeconds, callback.gpStartTimestamp);
	//ASSERT_EQ(result->gpEndTimestampInNanoSeconds, callback.gpEndTimestamp);
	//ASSERT_STREQ(result->jobId, callback.jobname);

	/*
	 * These indexes except 'value' should be divided by two, they are increased on both entering and exiting 'tags'
	 */
	unsigned moclass = 0;
	unsigned ldn = 0;
	unsigned value = 0;

	ASSERT_NE((void*)0, result);

	for (unsigned i = 0; i<size; i++)
	{
		if (data[i].returnCode != SA_AIS_OK)
			break;

		if (data[i].info.objectType == PMF_JOB_ITERATOR_OBJECT_MOCLASS)
		{
			printf("\nobjectType=PMF_JOB_ITERATOR_OBJECT_MOCLASS\n");
			printf("Expected:\tnodeName=%s\n", data[i].info.nodeName);
			printf("Received:\tmeasObjClass=%s\n", result->measObjClasses[moclass/2].measObjClass);
			ASSERT_STREQ(data[i].info.nodeName, result->measObjClasses[moclass/2].measObjClass);
			moclass++;
			ldn = 0;
			value = 0;
		}
		else if (data[i].info.objectType == PMF_JOB_ITERATOR_OBJECT_LDN)
		{
			printf("\nobjectType=PMF_JOB_ITERATOR_OBJECT_LDN\n");
			printf("Expected:\tnodeName=%s\n", data[i].info.nodeName);
			printf("Received:\tmeasObjLDN=%s\n", result->measObjClasses[moclass/2].instances[ldn/2].measObjLDN);
			ASSERT_STREQ(data[i].info.nodeName, result->measObjClasses[moclass/2].instances[ldn/2].measObjLDN);
			ldn++;
			value = 0;
		}
		else if (data[i].info.objectType == PMF_JOB_ITERATOR_OBJECT_VALUE)
		{
			printf("\nobjectType=PMF_JOB_ITERATOR_OBJECT_VALUE\n");
			printf("Expected:\tnodeName=%s\tisSuspect=%d\n", data[i].info.nodeName,data[i].info.valueInfo.isSuspect);
			ASSERT_STREQ(data[i].info.nodeName, result->measObjClasses[moclass/2].instances[ldn/2].values[value/2].measType);
			printf("Expected:\tmultiplicity =%d \n", data[i].info.valueInfo.multiplicity);
			ASSERT_EQ(data[i].info.valueInfo.multiplicity, result->measObjClasses[moclass/2].instances[ldn/2].values[value].valueSize);

			if (data[i].info.valueInfo.multiplicity == 0)
			{
				ASSERT_EQ(result->measObjClasses[moclass/2].instances[ldn/2].values[value].valueType == ComOamSpiPmValueType_2_NIL, true);
			}
			else
			{
				EXPECT_EQ(data[i].info.valueInfo.isFloat,
					  result->measObjClasses[moclass/2].instances[ldn/2].values[value].valueType == ComOamSpiPmValueType_2_FLOATARR);
			}
			printf("Received:\tmeasType=%s\tisSuspect=%d\n",
				   result->measObjClasses[moclass/2].instances[ldn/2].values[value/2].measType,result->measObjClasses[moclass/2].instances[ldn/2].values[value].isSuspect);

			ASSERT_EQ(data[i].info.valueInfo.isSuspect, result->measObjClasses[moclass/2].instances[ldn/2].values[value].isSuspect);
			for (int j=0; j<data[i].info.valueInfo.multiplicity ; j++)
			{
				if (data[i].info.valueInfo.isFloat)
				{
					printf("Expected:\tfloatArr[%d]=%f\n",j, data[i].values[j].floatVal);
					printf("Received:\tfloatArr[%d]=%f\n",j, result->measObjClasses[moclass/2].instances[ldn/2].values[value].value.floatArr[j]);
					ASSERT_EQ(data[i].values[j].floatVal, result->measObjClasses[moclass/2].instances[ldn/2].values[value].value.floatArr[j]);
				}
				else
				{
					printf("Expected:\tIntArr[%d]=%d\n",j, data[i].values[j].intVal);
					printf("Received:\tIntArr[%d]=%d\n",j,result->measObjClasses[moclass/2].instances[ldn/2].values[value].value.intArr[j]);
					ASSERT_EQ(data[i].values[j].intVal, result->measObjClasses[moclass/2].instances[ldn/2].values[value].value.intArr[j]);
				}
			}
			value++;
		}
		else
		{
			ASSERT_EQ(2 + 2, 5); // this should not happen
		}
	}
}

SaPmCInstanceValuesT_2** populateGetInstanceValues_2output(info_gMV2 info)
{
	SaPmCInstanceValuesT_2 **instanceValues = new SaPmCInstanceValuesT_2*[measurementNameCount];

	instanceValues[0] = new SaPmCInstanceValuesT_2;
		instanceValues[0]->instance = strdup(info.instance);
		instanceValues[0]->measurementType = strdup(info.measTypes[0]);
		instanceValues[0]->pmGroup = strdup(info.pmGroups[0]);
			instanceValues[0]->valueInfo.isFloat = SA_FALSE;
			instanceValues[0]->valueInfo.isSuspect = SA_FALSE;
			instanceValues[0]->valueInfo.multiplicity = SaInt16T(measObjCount);
		instanceValues[0]->values = new SaPmAggregatedValueT[measObjCount];
			instanceValues[0]->values[0].intVal = SaUint64T(143);
			instanceValues[0]->values[1].intVal = SaUint64T(99);

	instanceValues[1] = new SaPmCInstanceValuesT_2;
		instanceValues[1]->instance = strdup(info.instance);
		instanceValues[1]->measurementType = strdup(info.measTypes[1]);
		instanceValues[1]->pmGroup = strdup(info.pmGroups[1]);
			instanceValues[1]->valueInfo.isFloat = SA_TRUE;
			instanceValues[1]->valueInfo.isSuspect = SA_TRUE;
			instanceValues[1]->valueInfo.multiplicity = SaInt16T(measObjCount);
		instanceValues[1]->values = new SaPmAggregatedValueT[measObjCount];
			instanceValues[1]->values[0].floatVal = double(98.48);
			instanceValues[1]->values[1].floatVal = double(68.76);

	instanceValues[2] = NULL;
	return instanceValues;
}

void releaseGetInstanceValues_2output(SaPmCInstanceValuesT_2 **instanceValues)
{
	if (NULL != instanceValues)
	{
		unsigned int index = 0;
		while (NULL != instanceValues[index])
		{
			if(instanceValues[index]->instance) {
				free ((void*)(instanceValues[index]->instance));
				instanceValues[index]->instance = NULL;
			}
			if(instanceValues[index]->measurementType) {
				free ((void*)(instanceValues[index]->measurementType));
				instanceValues[index]->measurementType = NULL;
			}
			if(instanceValues[index]->pmGroup) {
				free ((void*)(instanceValues[index]->pmGroup));
				instanceValues[index]->pmGroup = NULL;
			}
			if(instanceValues[index]->values) {
				delete[] instanceValues[index]->values;
				instanceValues[index]->values = NULL;
			}
			delete instanceValues[index];
			instanceValues[index] = NULL;
			++index;
		}
		delete[] instanceValues;
		instanceValues = NULL;
	}
}

/**
 *  Sets some default values in <em>SaPmCCallbackInfoT</em> so that we do not need to do this in every testcase
 *
 *  @parameter[out] callback This variable will be set to some default values.
 */

/*
 * So, better create a macro for this.  The C++ compiler complains about
 * "deprecated conversion from string constant to 'char*'", this macro
 * wraps it into a plain ol' C cast.
 */
#define CSTR2CHAR(cChars) (char *)cChars

void setDefaultCallback(SaPmCCallbackInfoT& callback)
{
	callback.clientHdlr = 1;
	callback.jobHandler = 3;
	callback.jobname = CSTR2CHAR("trala");
	callback.gpStartTimestamp = 666;
	callback.gpEndTimestamp = 6666;
}

TEST(PmConsumer, SixtyFourBitEnvironment)
{
	ASSERT_EQ(sizeof(void*), sizeof(int64_t));
}


TEST(PmConsumer, Success2)
{
	MockPmConsumerInterface mock;
	// Delete the instance created in the constructor
	if (PmConsumerInterface::s_instance != NULL)
	{
		delete PmConsumerInterface::s_instance;
	}
	PmConsumerInterface::s_instance = &mock;
	SaPmCCallbackInfoT callback;
	setDefaultCallback(callback);

	Testdata2 data[] =
	{
			// @formatter:off (java only)
			//{SA_AIS_OK, info={.isLeaf=false, .objectType=PMF_JOB_ITERATOR_OBJECT_MOCLASS, .nodeName = "nodeName", //.valueInfo={.multiplicity=5,.isFloat=true,.isSuspect=false}}
			//.multiplicity
			//.value ={.floatVal=0.5,0.7,0.9} }
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)false, (SaBoolT)false} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_trala"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {0.1,0.2,0.3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false}}, {1,2.5,3.0004}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {5,6,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_trala"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_tralb"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)false, (SaBoolT)false} }, {(SaUint64T)1,(SaUint64T)2,(SaUint64T)3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)false, (SaBoolT)true} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_tralb"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)false, (SaBoolT)false} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_trala"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {0.1,0.2,0.3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false}}, {1,2.5,3.0004}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {5,6,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){0}, (SaBoolT)false, (SaBoolT)false} }, {}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){0}, (SaBoolT)true, (SaBoolT)false} }, {}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_trala"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
			{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}},
			{SA_AIS_ERR_NOT_EXIST, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}}
			// @formatter:on (java only)
	};
	data[10].values[0].intVal = 5;
	data[10].values[1].intVal = 50;
	data[10].values[2].intVal = 500;
	data[11].values[0].intVal = 15;
	data[11].values[1].intVal = 150;
	data[11].values[2].intVal = 1500;
	setExpectations2(mock, callback, data, 25, SA_AIS_OK);
	ComOamSpiPmGpData_2* res = PmtSa::getData(&callback);

	validate2(data, 25, res, callback);

	ASSERT_EQ(2, res->size);
	ASSERT_EQ(2, res->measObjClasses[0].size);
	// ASSERT_EQ(1, res->measObjClasses[1].size);
	ASSERT_EQ(4, res->measObjClasses[0].instances[0].size);
	ASSERT_EQ(4, res->measObjClasses[0].instances[1].size);
	// ASSERT_EQ(4, res->measObjClasses[1].instances[0].size);

	ASSERT_EQ(res->isSuspect, false);

	PmtSa::freeData(res);
}


TEST(PmConsumer, ErrorCaseBadIteratorInit2)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	SaPmCCallbackInfoT callback;
	setDefaultCallback(callback);

	setExpectations2(mock, callback, NULL, 0, SA_AIS_ERR_UNAVAILABLE);
	ComOamSpiPmGpData_2* res = PmtSa::getData(&callback);

	ASSERT_EQ(res->size, 0);
	ASSERT_EQ(res->isSuspect, true);

	PmtSa::freeData(res);
	Mock::VerifyAndClear(&mock);
}

TEST(PmConsumer, ErrorCaseMoclassFails2)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	SaPmCCallbackInfoT callback;
	setDefaultCallback(callback);


	Testdata2 data[] =
	{
//		{SA_AIS_OK, {.isLeaf=false, .objectType=PMF_JOB_ITERATOR_OBJECT_MOCLASS, .nodeName = "nodeName", .valueInfo={.value={.floatVal=0.5},.isFloat=true,.isSuspect=false} }}
		{SA_AIS_ERR_BAD_HANDLE, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}}
	};

	setExpectations2(mock, callback, data, 1, SA_AIS_OK);
	ComOamSpiPmGpData_2* res = PmtSa::getData(&callback);

	validate2(data, 0, res, callback);

	ASSERT_EQ(res->size, 0);
	ASSERT_EQ(res->isSuspect, true);

	PmtSa::freeData(res);
}

TEST(PmConsumer, ErrorCaseMoClassFailsLate2)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	SaPmCCallbackInfoT callback;
	setDefaultCallback(callback);


	Testdata2 data[] =
	{
//			{SA_AIS_OK, {.isLeaf=false, .objectType=PMF_JOB_ITERATOR_OBJECT_MOCLASS, .nodeName = "nodeName", .valueInfo={.value={.floatVal=0.5},.isFloat=true,.isSuspect=false} }}
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)false, (SaBoolT)false} }, {1,2,3}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_trala"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {0.1,0.2,0.3}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false}}, {1,2.5,3.0004}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {5,6,3}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_trala"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
		{SA_AIS_ERR_BAD_HANDLE, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}}
	};

	setExpectations2(mock, callback, data, 8, SA_AIS_OK);
	ComOamSpiPmGpData_2* res = PmtSa::getData(&callback);

	validate2(data, 8, res, callback);

	ASSERT_EQ(1, res->size);
	ASSERT_EQ(1, res->measObjClasses[0].size);
	ASSERT_EQ(4, res->measObjClasses[0].instances[0].size);
	ASSERT_EQ(res->isSuspect, true);

	PmtSa::freeData(res);
}

TEST(PmConsumer, ErrorCaseFailsReadingVar2)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	SaPmCCallbackInfoT callback;
	setDefaultCallback(callback);


	Testdata2 data[] =
	{
//{SA_AIS_OK, {.isLeaf=false, .objectType=PMF_JOB_ITERATOR_OBJECT_MOCLASS, .nodeName = "nodeName", .valueInfo={.value={.floatVal=0.5},.isFloat=true,.isSuspect=false} }}
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_MOCLASS, CSTR2CHAR("moclass_trala"),   {(SaInt16T){3}, (SaBoolT)false, (SaBoolT)false} }, {1,2,3}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_LDN,       CSTR2CHAR("ldn_trala"),     {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {0.1,0.2,0.3}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false}}, {1,2.5,3.0004}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {1,2,3}},
		{SA_AIS_OK, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)false} }, {1,2,3}},
		{SA_AIS_ERR_BAD_HANDLE, {(SaBoolT)false, PMF_JOB_ITERATOR_OBJECT_VALUE,       CSTR2CHAR("value_trala"), {(SaInt16T){3}, (SaBoolT)true, (SaBoolT)true} }, {5,6,3}}
	};

	setExpectations2(mock, callback, data, 8, SA_AIS_OK);
	ComOamSpiPmGpData_2* res = PmtSa::getData(&callback);

	validate2(data, 8, res, callback);

	ASSERT_EQ(1, res->size);
	ASSERT_EQ(1, res->measObjClasses[0].size);
	ASSERT_EQ(3, res->measObjClasses[0].instances[0].size);
	ASSERT_EQ(res->isSuspect, true);

	PmtSa::freeData(res);
}

TEST(PmShowCounters, instance)
{
	PmtSa::PmShowCounters* pPmShowCounter = PmtSa::PmShowCounters::instance();
	EXPECT_EQ(pPmShowCounter != NULL, true);
	delete pPmShowCounter;
}

TEST(PmShowCounters, getMeasurementValueWithNullDn)
{
	MafReturnT retVal = pmtsa_getMeasurementValues(NULL, NULL, NULL);
	EXPECT_EQ(retVal, MafInvalidArgument);
}

TEST(PmShowCounters, getMeasurementValueWithEmptyDn)
{
	MafReturnT retVal = pmtsa_getMeasurementValues("", NULL, NULL);
	EXPECT_EQ(retVal, MafNotExist);
}


TEST(PmShowCounters, getMeasurementValueWithValidationPassed)
{
	const char* dn = "ManagedElement=1,RootX=1";
	ComOamSpiPmMeasurementFilter_3T* filter = new ComOamSpiPmMeasurementFilter_3T;
	filter->isVerbose = true;
	filter->jobId = strdup("job1");
	filter->measurementTypes = new const char*[2];
	filter->measurementTypes[0] = strdup("MeasType1");
	filter->measurementTypes[1] = NULL;
	ComOamSpiPmMeasuredObject_3T * measuredObject = new ComOamSpiPmMeasuredObject_3T;

	MafReturnT retVal = pmtsa_getMeasurementValues(dn, filter, measuredObject);

	ASSERT_EQ(MafNotExist, retVal);

	free((void*)filter->measurementTypes[0]);
	delete [] filter->measurementTypes;
	free((void*)filter->jobId);
	delete filter;
	delete measuredObject;
}

TEST(PmShowCounters, getMeasurementNames)
{
	/* MafReturnT PmShowCounters::getMeasurementNames(const char * dn, ComOamSpiPmMeasuredObjectMeasurementNames_3T* measurementNames) */
	MafReturnT retVal = pmtsa_getMeasurementNames("test", NULL);
	EXPECT_EQ(retVal, MafNoResources);
}

TEST(PmShowCounters, getPmJobIds)
{
	/* MafReturnT PmShowCounters::getPmJobIds(const char * dn, ComOamSpiPmMeasuredObjectJobIds_3T* measurementJobIds) */
	MafReturnT retVal = pmtsa_getPmJobIds("test", NULL);
	EXPECT_EQ(retVal, MafNoResources);
}

TEST(PmShowCounters, releaseMeasuredObjAlreadyReleased)
{
	ComOamSpiPmMeasuredObject_3T* measuredObject = new ComOamSpiPmMeasuredObject_3T;
	measuredObject->release = NULL;
	measuredObject->nrOfMeasurements = 0;
	measuredObject->measurements = NULL;

	releaseMeasuredObject(measuredObject);

	ASSERT_EQ(NULL, measuredObject->release);
	ASSERT_EQ(NULL, measuredObject->measurements);
	ASSERT_EQ(0, measuredObject->nrOfMeasurements);

	delete measuredObject;
}

TEST(PmShowCounters, releaseMeasuredObjNotReleasedYet)
{
	ComOamSpiPmMeasuredObject_3T* measuredObject = new ComOamSpiPmMeasuredObject_3T;
	measuredObject->release = &releaseMeasuredObject;
	measuredObject->nrOfMeasurements = measObjCount;
	measuredObject->measurements = new ComOamSpiPmMeasurement_3T[measObjCount];

	measuredObject->measurements[0].errorInformation = strdup("Error Info 1");
	measuredObject->measurements[0].gpInSeconds = 5;
	measuredObject->measurements[0].jobId = strdup("Job1");
	measuredObject->measurements[0].name = strdup("MT1");


	measuredObject->measurements[0].nrOfValues = 2;
	measuredObject->measurements[0].value = new ComOamSpiPmMeasurementValue_3T[2];
	measuredObject->measurements[0].value[0].isSuspect = true;
	measuredObject->measurements[0].value[0].value.intVal = 123;
	measuredObject->measurements[0].valueType = ComOamSpiPmMeasurementValueType_3_INT;
	measuredObject->measurements[0].value[1].isSuspect = true;
	measuredObject->measurements[0].value[1].value.intVal = 456;
	measuredObject->measurements[0].valueType = ComOamSpiPmMeasurementValueType_3_INT;

	measuredObject->measurements[1].errorInformation = strdup("Error Info 2");
	measuredObject->measurements[1].gpInSeconds = 10;
	measuredObject->measurements[1].jobId = strdup("Job2");
	measuredObject->measurements[1].name = strdup("MT2");

	measuredObject->measurements[1].nrOfValues = 3;
	measuredObject->measurements[1].value = new ComOamSpiPmMeasurementValue_3T[3];
	measuredObject->measurements[1].value[0].isSuspect = false;
	measuredObject->measurements[1].value[0].value.floatVal = 12.34;
	measuredObject->measurements[1].valueType = ComOamSpiPmMeasurementValueType_3_FLOAT;
	measuredObject->measurements[1].value[1].isSuspect = false;
	measuredObject->measurements[1].value[1].value.floatVal = 45.67;
	measuredObject->measurements[1].valueType = ComOamSpiPmMeasurementValueType_3_FLOAT;
	measuredObject->measurements[1].value[2].isSuspect = false;
	measuredObject->measurements[1].value[2].value.floatVal = 89.11;
	measuredObject->measurements[1].valueType = ComOamSpiPmMeasurementValueType_3_FLOAT;

	releaseMeasuredObject(measuredObject);

	ASSERT_EQ(NULL, measuredObject->release);
	ASSERT_EQ(NULL, measuredObject->measurements);
	ASSERT_EQ(0, measuredObject->nrOfMeasurements);

	delete measuredObject;
}

TEST(PmConsumer, isFeatureEnabledEnable)
{
	MockPmConsumerInterface dummy;
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	setExpectations_isFeatureEnabled(mock, SA_TRUE, SA_AIS_OK);
	bool feature = PmConsumerInterface::s_instance->isShowCounterV2Enabled();
	ASSERT_EQ(0, (feature == false));
}

TEST(PmConsumer, isFeatureEnabledDisable)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	setExpectations_isFeatureEnabled(mock, SA_FALSE, SA_AIS_OK);
	bool feature = PmConsumerInterface::s_instance->isShowCounterV2Enabled();
	ASSERT_EQ(0, (feature == true));
}

TEST(PmConsumer, isFeatureEnabledError)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	setExpectations_isFeatureEnabled(mock, SA_TRUE, SA_AIS_ERR_INVALID_PARAM);
	bool feature = PmConsumerInterface::s_instance->isShowCounterV2Enabled();
	ASSERT_EQ(0, (feature == true));
}

TEST(PmShowCounters, getMeasurementValues_2)
{
	ComOamSpiPmMeasurementFilter_4T* filter = new ComOamSpiPmMeasurementFilter_4T;
	filter->measuredObject                  = strdup("ManagedElement=1,RootX=1");
	filter->jobId                           = NULL;
	filter->groupIds                        = new const char*[measurementNameCount];
	filter->groupIds[0]                     = strdup("ccGroup1");
	filter->groupIds[1]                     = strdup("ccGroup2");
	filter->groupIds[2]                     = NULL;
	filter->measurementTypes                = new const char*[measurementNameCount];
	filter->measurementTypes[0]             = strdup("mt1");
	filter->measurementTypes[1]             = strdup("mt2");
	filter->measurementTypes[2]             = NULL;

	ComOamSpiPmMeasuredObjectMeasurements_4T* measObj = new ComOamSpiPmMeasuredObjectMeasurements_4T;

	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;

	info_gMV2 info = { (void*)18, filter->measuredObject, filter->measurementTypes, filter->groupIds, 3 };

	SaPmCInstanceValuesT_2 **instanceValues = populateGetInstanceValues_2output(info);

	setExpectations_getInstanceValues_2(mock, info, instanceValues, SA_AIS_OK, SA_AIS_OK);

	MafReturnT retVal = pmtsa_getMeasurementValues_2(filter, true, measObj);
	ASSERT_EQ(MafOk, retVal);
	EXPECT_EQ(2, measObj->nrOfMeasurements);
	EXPECT_EQ(0, strcmp("mt1", measObj->measurements[0].measurementTypeName));
	EXPECT_EQ(0, strcmp("mt2", measObj->measurements[1].measurementTypeName));
	EXPECT_EQ(0, strcmp("ccGroup1", measObj->measurements[0].groupId));
	EXPECT_EQ(0, strcmp("ccGroup2", measObj->measurements[1].groupId));
	EXPECT_EQ(0, strcmp("ManagedElement=1,RootX=1", measObj->measurements[0].measuredObject));
	EXPECT_EQ(0, strcmp("ManagedElement=1,RootX=1", measObj->measurements[1].measuredObject));
	EXPECT_EQ(ComOamSpiPmMeasurementValueType_4_INT, measObj->measurements[0].valueType);
	EXPECT_EQ(ComOamSpiPmMeasurementValueType_4_FLOAT, measObj->measurements[1].valueType);
	EXPECT_EQ(0, measObj->measurements[0].gpInSeconds);
	EXPECT_EQ(0, measObj->measurements[1].gpInSeconds);
	EXPECT_EQ(NULL, measObj->measurements[0].jobId);
	EXPECT_EQ(NULL, measObj->measurements[1].jobId);
	EXPECT_EQ(0,strcmp("No data returned",measObj->measurements[0].errorInformation));
	EXPECT_EQ(0,strcmp("No data returned",measObj->measurements[1].errorInformation));

	//Clear contents allocated by previous call!
	releaseMeasuredObjectMeasurements(measObj);
	EXPECT_EQ(0, measObj->nrOfMeasurements);
	EXPECT_EQ(0, (NULL != measObj->measurements));

	retVal = pmtsa_getMeasurementValues_2(filter, false, measObj);
	EXPECT_EQ(0,(NULL != measObj->measurements[0].errorInformation));
	EXPECT_EQ(0,(NULL != measObj->measurements[1].errorInformation));

	//Verify releaseMeasuredObjectMeasurements too!
	releaseMeasuredObjectMeasurements(measObj);
	EXPECT_EQ(0, measObj->nrOfMeasurements);
	EXPECT_EQ(0, (NULL != measObj->measurements));


	releaseSaConstStringArray(filter->groupIds);
	EXPECT_EQ(0, (NULL == filter->groupIds));

	releaseSaConstStringArray(filter->measurementTypes);
	EXPECT_EQ(0, (NULL == filter->measurementTypes));

	releaseSaConstString(filter->measuredObject);
	EXPECT_EQ(0, (NULL == filter->measuredObject));

	delete filter;
	filter = NULL;
	releaseGetInstanceValues_2output(instanceValues);
	delete measObj;
	measObj=NULL;
}

TEST(PmShowCounters, getMeasurementValues_2ErrorCases)
{
	ComOamSpiPmMeasurementFilter_4T* filter = new ComOamSpiPmMeasurementFilter_4T;
	filter->measuredObject                  = strdup("ManagedElement=1,RootX=1");
	filter->jobId                           = NULL;
	filter->groupIds                        = new const char*[measurementNameCount];
	filter->groupIds[0]                     = strdup("ccGroup1");
	filter->groupIds[1]                     = strdup("ccGroup2");
	filter->groupIds[2]                     = NULL;
	filter->measurementTypes                = new const char*[measurementNameCount];
	filter->measurementTypes[0]             = strdup("mt1");
	filter->measurementTypes[1]             = strdup("mt2");
	filter->measurementTypes[2]             = NULL;

	ComOamSpiPmMeasuredObjectMeasurements_4T* measObj = new ComOamSpiPmMeasuredObjectMeasurements_4T;

	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;

	info_gMV2 info = { (void*)18, filter->measuredObject, filter->measurementTypes, filter->groupIds, 3 };

	SaPmCInstanceValuesT_2 **instanceValues = populateGetInstanceValues_2output(info);

	//input container is NULL
	setExpectations_getInstanceValues_2(mock, info, instanceValues, SA_AIS_OK, SA_AIS_OK);
	MafReturnT retVal = pmtsa_getMeasurementValues_2(NULL, false, NULL);
	EXPECT_EQ(retVal, MafFailure);

	//output container is NULL
	retVal = pmtsa_getMeasurementValues_2(filter, false, NULL);
	EXPECT_EQ(retVal, MafFailure);

	//filter->measObj=NULL
	free ((void*)filter->measuredObject);
	filter->measuredObject = NULL;
	retVal = pmtsa_getMeasurementValues_2(filter, false, measObj);
	EXPECT_EQ(retVal, MafOk);
	filter->measuredObject = strdup("ManagedElement=1,RootX=1");

	//ais-try-again; reqid!=0
	setExpectations_getInstanceValues_2(mock, info, instanceValues, SA_AIS_ERR_TRY_AGAIN, SA_AIS_OK);
	retVal = pmtsa_getMeasurementValues_2(filter, false, measObj);
	EXPECT_EQ(retVal, MafTryAgain);

	//ais-try-again ; requId=0
	info.requestId = 0;
	setExpectations_getInstanceValues_2(mock, info, instanceValues, SA_AIS_ERR_TRY_AGAIN, SA_AIS_OK);
	retVal = pmtsa_getMeasurementValues_2(filter, false, measObj);
	EXPECT_EQ(retVal, MafFailure);
	info.requestId = 10;

	//ais-not-exist
	setExpectations_getInstanceValues_2(mock, info, instanceValues, SA_AIS_ERR_NOT_EXIST, SA_AIS_OK);
	retVal = pmtsa_getMeasurementValues_2(filter, true, measObj);
	EXPECT_EQ(retVal, MafNotExist);

	//ais-invalid-param
	//FIXME: modify gmock framework to accomodate _portal mocking.
	/*setExpectations_getInstanceValues_2(mock, info, instanceValues, SA_AIS_ERR_INVALID_PARAM, SA_AIS_OK);
	retVal = pmtsa_getMeasurementValues_2(filter, true, measObj);
	EXPECT_EQ(retVal, MafInvalidArgument);*/

	//ais-err-other
	setExpectations_getInstanceValues_2(mock, info, instanceValues, SA_AIS_ERR_LIBRARY, SA_AIS_OK);
	retVal = pmtsa_getMeasurementValues_2(filter, true, measObj);
	EXPECT_EQ(retVal, MafFailure);

	//instance free err
	setExpectations_getInstanceValues_2(mock, info, instanceValues, SA_AIS_OK, SA_AIS_ERR_LIBRARY);
	retVal = pmtsa_getMeasurementValues_2(filter, true, measObj);
	EXPECT_EQ(retVal, MafOk);

	releaseSaConstString(filter->measuredObject);
	releaseSaConstStringArray(filter->groupIds);
	releaseSaConstStringArray(filter->measurementTypes);
	releaseSaConstString(filter->jobId);
	delete filter;
	filter = NULL;
	releaseMeasuredObjectMeasurements(measObj);
	delete measObj;
	measObj = NULL;
	releaseGetInstanceValues_2output(instanceValues);

}

TEST(PmShowCounters, getMeasurementNames_2)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	const char* instance=strdup("RootX=1");
	const char* measurementType=strdup("MeasType1");
	const char** groups =  new const char*[3];
	groups[0]=strdup("group1");
	groups[1]=strdup("group2");
	groups[2]=NULL;
	const char** outputNames=new const char*[3];
	outputNames[0]=strdup("mtypename1");
	outputNames[1]=strdup("mtypename2");
	outputNames[2]=NULL;
	info_gMD testdata={instance,&measurementType,groups,outputNames};
	setExpectations_getMeasurementTypeNames(mock,testdata,SA_AIS_OK);
	ComOamSpiPmMeasurementFilter_4T* filter = new ComOamSpiPmMeasurementFilter_4T ;
	ComOamSpiPmMeasurementOutput_4T* measurementTypes = new ComOamSpiPmMeasurementOutput_4T;
	filter->measuredObject = strdup(instance);
	filter->groupIds= new const char*[3];
	filter->groupIds[0]=strdup("group1");
	filter->groupIds[1]=strdup("group2");
	filter->groupIds[2] = NULL;
	filter->measurementTypes = new const char*[2];
	filter->measurementTypes[0] = strdup("MeasType1");
	filter->measurementTypes[1] = NULL;
	MafReturnT retVal = pmtsa_getMeasurementNames_2(filter, measurementTypes);
	EXPECT_EQ(retVal, MafOk);
	ASSERT_EQ(true,(NULL != measurementTypes->names));
	ASSERT_EQ(0, strcmp("mtypename1", measurementTypes->names[0]));
	ASSERT_EQ(0, strcmp("mtypename2", measurementTypes->names[1]));
	ASSERT_EQ(true,(NULL==measurementTypes->names[2]));
	releaseMeasurementOutput(measurementTypes);
	ASSERT_EQ(NULL, measurementTypes->names);
	ASSERT_EQ(NULL,measurementTypes->release);
	releaseSaConstString(filter->measuredObject);
	releaseSaConstStringArray(filter->groupIds);
	releaseSaConstStringArray(filter->measurementTypes);
	releaseSaConstString(instance);
	releaseSaConstString(measurementType);
	releaseSaConstStringArray(groups);
	releaseSaConstStringArray(outputNames);
	delete filter;
	filter = NULL;
	delete measurementTypes;
	measurementTypes=NULL;
}

TEST(PmShowCounters, getMeasurementNames_2ErrorCase)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	const char* instance=strdup("ManagedElement=1,RootX=1");
	const char* measurementType=strdup("MeasType1");
	const char** groups =  new const char*[3];
	groups[0]=strdup("group1");
	groups[1]=strdup("group2");
	groups[2]=NULL;
	const char** outputNames=new const char*[3];
	outputNames[0]=strdup("mtypename1");
	outputNames[1]=strdup("mtypename2");
	outputNames[2]=NULL;
	info_gMD testdata={instance,&measurementType,groups,outputNames};
	setExpectations_getMeasurementTypeNames(mock,testdata,SA_AIS_ERR_EXIST);
	ComOamSpiPmMeasurementFilter_4T* filter = new ComOamSpiPmMeasurementFilter_4T ;
	ComOamSpiPmMeasurementOutput_4T* measurementTypes = new ComOamSpiPmMeasurementOutput_4T;
	filter->measuredObject = strdup(instance);
	filter->groupIds= new const char*[3];
	filter->groupIds[0]=strdup("group1");
	filter->groupIds[1]=strdup("group2");
	filter->groupIds[2] = NULL;
	filter->measurementTypes = new const char*[2];
	filter->measurementTypes[0] = strdup("MeasType1");
	filter->measurementTypes[1] = NULL;
	MafReturnT retVal = pmtsa_getMeasurementNames_2(filter, measurementTypes);
	EXPECT_EQ(retVal, MafFailure);
	releaseSaConstStringArray(outputNames);

	testdata.outputNames=NULL;
	setExpectations_getMeasurementTypeNames(mock, testdata,SA_AIS_OK);
	retVal = pmtsa_getMeasurementNames_2(filter, measurementTypes);
	EXPECT_EQ(retVal, MafOk);

	retVal = pmtsa_getMeasurementNames_2(filter, NULL);
	EXPECT_EQ(retVal, MafFailure);

	releaseSaConstString(filter->measuredObject);
	releaseSaConstStringArray(filter->groupIds);
	releaseSaConstStringArray(filter->measurementTypes);
	releaseSaConstString(instance);
	releaseSaConstString(measurementType);
	releaseSaConstStringArray(groups);
	delete filter;
	filter = NULL;
	delete measurementTypes;
	measurementTypes=NULL;
}

TEST(PmShowCounters, getPmGroupIds)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	ComOamSpiPmMeasurementFilter_4T* filter = new ComOamSpiPmMeasurementFilter_4T;
	ComOamSpiPmMeasurementOutput_4T* groupIds = new ComOamSpiPmMeasurementOutput_4T;
	const char* dn = strdup("RootX=1");
	const char** mtype = new const char*[3];
	mtype[0] = strdup("mtype1");
	mtype[1] = strdup("mtype2");
	mtype[2] = NULL;
	const char** group = new const char*[2];
	group[0]=strdup("group1");
	group[1]=NULL;
	const char** outputNames=new const char*[3];
	outputNames[0]=strdup("groupname1");
	outputNames[1]=strdup("groupname2");
	outputNames[2]=NULL;
	info_gMD data ={dn,mtype,group,outputNames};
	filter->measuredObject = strdup(dn);
	filter->groupIds= new const char*[2];
	filter->groupIds[0]=strdup("group1");
	filter->groupIds[1]=NULL;
	filter->measurementTypes = new const char*[3];
	filter->measurementTypes[0]=strdup("mtype1");
	filter->measurementTypes[1]=strdup("mtype2");
	filter->measurementTypes[2]=NULL;
	setExpectations_getPmGroupIds(mock, data, SA_AIS_OK);
	EXPECT_CALL(mock,saPmCNamesMemoryFree(NotNull(),NotNull())).WillRepeatedly(Return(SA_AIS_OK));
	MafReturnT retval= pmtsa_getPmGroupIds(filter,groupIds);
	EXPECT_EQ(retval, MafOk);
	ASSERT_EQ(true,(NULL != groupIds->names));
	ASSERT_EQ(0, strcmp("groupname1",groupIds->names[0]));
	ASSERT_EQ(0, strcmp("groupname2", groupIds->names[1]));
	ASSERT_EQ(true,(NULL==groupIds->names[2]));
	releaseMeasurementOutput(groupIds);
	ASSERT_EQ(NULL,groupIds->names);
	releaseSaConstString(filter->measuredObject);
	releaseSaConstStringArray(filter->groupIds);
	releaseSaConstStringArray(outputNames);
	releaseSaConstStringArray(filter->measurementTypes);
	releaseSaConstString(dn);
	releaseSaConstStringArray(mtype);
	releaseSaConstStringArray(group);
	delete filter;
	filter = NULL;
	delete groupIds;
	groupIds=NULL;
}

TEST(PmShowCounters, getPmGroupIds_ErrorCase)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	ComOamSpiPmMeasurementFilter_4T* filter = new ComOamSpiPmMeasurementFilter_4T;
	ComOamSpiPmMeasurementOutput_4T* groupIds = new ComOamSpiPmMeasurementOutput_4T;
	const char* dn = strdup("RootX=1");
	const char** mtype = new const char*[3];
	mtype[0] = strdup("mtype1");
	mtype[1] = strdup("mtype2");
	mtype[2] = NULL;
	const char** group = new const char*[2];
	group[0]=strdup("group1");
	group[1]=NULL;
	const char** outputNames=new const char*[3];
	outputNames[0]=strdup("groupname1");
	outputNames[1]=strdup("groupname2");
	outputNames[2]=NULL;
	info_gMD data ={dn,mtype,group,outputNames};
	filter->measuredObject = strdup(dn);
	filter->groupIds= new const char*[2];
	filter->groupIds[0]=strdup("group1");
	filter->groupIds[1]=NULL;
	filter->measurementTypes = new const char*[3];
	filter->measurementTypes[0]=strdup("mtype1");
	filter->measurementTypes[1]=strdup("mtype2");
	filter->measurementTypes[2]=NULL;
	setExpectations_getPmGroupIds(mock, data, SA_AIS_ERR_NOT_EXIST);
	MafReturnT retval= pmtsa_getPmGroupIds(filter,groupIds);
	EXPECT_EQ(retval, MafNotExist);
	releaseSaConstStringArray(outputNames);

	data.outputNames=NULL;
	setExpectations_getPmGroupIds(mock,data,SA_AIS_OK);
	retval = pmtsa_getPmGroupIds(filter, groupIds);
	EXPECT_EQ(MafOk, retval);

	retval = pmtsa_getPmGroupIds(filter, NULL);
	EXPECT_EQ(retval, MafFailure);

	releaseSaConstString(filter->measuredObject);
	releaseSaConstStringArray(filter->groupIds);
	releaseSaConstStringArray(filter->measurementTypes);
	releaseSaConstString(dn);
	releaseSaConstStringArray(mtype);
	releaseSaConstStringArray(group);
	delete filter;
	filter = NULL;
	delete groupIds;
	groupIds=NULL;
}

TEST(PmShowCounters, getInstanceNamesWithValidationPassed)
{
	MockPmConsumerInterface mock;
	PmConsumerInterface::s_instance = &mock;
	ComOamSpiPmMeasurementFilter_4T* filter = new ComOamSpiPmMeasurementFilter_4T;
	ComOamSpiPmMeasurementOutput_4T* measuredObjects = new ComOamSpiPmMeasurementOutput_4T;
	filter->measuredObject = strdup("ManagedElement=1,RootX=1");
	filter->measurementTypes = new const char*[2];
	filter->measurementTypes[0] = strdup("CcMT-1");
	filter->measurementTypes[1] = NULL;
	filter->groupIds = new const char*[2];
	filter->groupIds[0] = strdup("CcGroup1");
	filter->groupIds[1] = NULL;
	SaConstStringT *measuredObjects1 = new SaConstStringT[2];
	measuredObjects1[0] = strdup("ABC");
	measuredObjects1[1] = NULL;
	setExpectations_getInstanceNames(mock, measuredObjects1, SA_AIS_OK);
	MafReturnT ret = pmtsa_getMeasuredObjects(filter, measuredObjects);
	ASSERT_EQ(MafOk, ret);
	ASSERT_EQ(true,(NULL != measuredObjects->names));
	ASSERT_EQ(0,strcmp("ABC",measuredObjects->names[0]));
	ASSERT_EQ(true, (NULL== measuredObjects->names[1]));
	releaseMeasurementOutput(measuredObjects);
	releaseSaConstStringArray(filter->measurementTypes);
	releaseSaConstStringArray(filter->groupIds);
	releaseSaConstString(filter->measuredObject);
	delete filter;
	filter=NULL;
	releaseSaConstStringArray(measuredObjects1);
	delete measuredObjects;
	measuredObjects = NULL;
}

TEST(PmShowCounters, getInstanceNamesWithErrorInfo)
{
		MockPmConsumerInterface mock;
		PmConsumerInterface::s_instance = &mock;
		ComOamSpiPmMeasurementFilter_4T* filter = new ComOamSpiPmMeasurementFilter_4T;
		ComOamSpiPmMeasurementOutput_4T* measuredObjects = new ComOamSpiPmMeasurementOutput_4T;
		filter->measuredObject = strdup("RootX=1");
		filter->measurementTypes = new const char*[2];
		filter->measurementTypes[0] = strdup("CcMT-1");
		filter->measurementTypes[1] = NULL;
		filter->groupIds = new const char*[2];
		filter->groupIds[0] = strdup("CcGroup1");
		filter->groupIds[1] = NULL;
		SaConstStringT *measuredObjects1 = NULL;
		setExpectations_getInstanceNames(mock, measuredObjects1, SA_AIS_ERR_NOT_EXIST);
		MafReturnT ret = pmtsa_getMeasuredObjects(filter, measuredObjects);
		EXPECT_EQ(MafNotExist, ret);

		setExpectations_getInstanceNames(mock, measuredObjects1,SA_AIS_OK);
		ret = pmtsa_getMeasuredObjects(filter, measuredObjects);
		EXPECT_EQ(MafOk, ret);

		ret = pmtsa_getMeasuredObjects(filter, NULL);
		EXPECT_EQ(MafFailure, ret);

		releaseSaConstStringArray(filter->measurementTypes);
		releaseSaConstStringArray(filter->groupIds);
		releaseSaConstString(filter->measuredObject);
		delete filter;
		filter=NULL;
		releaseSaConstStringArray(measuredObjects1);
		delete measuredObjects;
		measuredObjects = NULL;

}

TEST(PmShowCounters, fetchGroupIdDescription)
{
	ComOamSpiPmGroupIdDescription_4T* description = new ComOamSpiPmGroupIdDescription_4T;
	MafReturnT retVal = pmtsa_getGroupIdDescription(NULL, description);
	EXPECT_EQ(retVal, MafFailure);

	const char* groupId = strdup("CcGroup1");
	retVal = pmtsa_getGroupIdDescription(groupId, description);
	EXPECT_EQ(retVal, MafFailure);

	free ((void*)(groupId));
	groupId = NULL;
	delete description;
	description = NULL;
}

TEST(PmShowCounters, releasePmGroupIdDescNotReleasedYet)
{
	ComOamSpiPmGroupIdDescription_4T* description = new ComOamSpiPmGroupIdDescription_4T;
	description->release = &releaseGroupIdDescription;
	description->description = strdup("Testing release function of getGroupIdDescription");
	releaseGroupIdDescription(description);
	ASSERT_EQ(NULL, description->release);
	ASSERT_EQ(NULL, description->description);

	description->release = &releaseGroupIdDescription;
	description->description = "";
	releaseGroupIdDescription(description);
	ASSERT_EQ(NULL, description->release);
	ASSERT_EQ(NULL, description->description);
	delete description;
	description = NULL;

}

TEST(PmShowCounters, fetchMeasurementTypeDescription)
{
	ComOamSpiPmMeasurementTypeDescription_4T* description = new ComOamSpiPmMeasurementTypeDescription_4T;
	MafReturnT retVal = pmtsa_getMeasurementTypeDescription(NULL, NULL, description);
	EXPECT_EQ(retVal, MafFailure);

	const char* measurementType = strdup("CcMT-1");
	const char** groupIds = new const char*[3];
	groupIds[0]= strdup("CcGroup1");
	groupIds[1]= strdup("CcGroup1");
	groupIds[2]= NULL;
	retVal = pmtsa_getMeasurementTypeDescription(measurementType, groupIds, description);
	EXPECT_EQ(retVal, MafFailure);

	releaseSaConstStringArray(groupIds);
	releaseSaConstString(measurementType);

	delete description;
	description = NULL;
}

TEST(PmShowCounters, releaseMeasurementTypeInfoNotReleasedYet)
{
	ComOamSpiPmMeasurementTypeDescription_4T* description = new ComOamSpiPmMeasurementTypeDescription_4T;
	description->release = &releaseMeasurementTypeDescription;
	description->nrOfValues = 2;
	description->measurementTypeInfo = new ComOamSpiPmMeasurementTypeInfo_4T[2];

	description->measurementTypeInfo[0].groupId = strdup("CcGroup1");
	description->measurementTypeInfo[0].description = strdup("Description of measurementType under CcGroup1");

	description->measurementTypeInfo[1].groupId = strdup("CcGroup2");
	description->measurementTypeInfo[1].description = strdup("Description of measurementType under CcGroup2");

	releaseMeasurementTypeDescription(description);
	ASSERT_EQ(NULL, description->release);
	ASSERT_EQ(NULL, description->measurementTypeInfo);
	ASSERT_EQ(0, description->nrOfValues);

	description->release = &releaseMeasurementTypeDescription;
	description->nrOfValues = 2;
	description->measurementTypeInfo = new ComOamSpiPmMeasurementTypeInfo_4T[2];

	description->measurementTypeInfo[0].groupId = strdup("CcGroup1");
	description->measurementTypeInfo[0].description = "";

	description->measurementTypeInfo[1].groupId = strdup("CcGroup2");
	description->measurementTypeInfo[1].description = strdup("Description of measurementType under CcGroup2");
	releaseMeasurementTypeDescription(description);
	ASSERT_EQ(NULL, description->release);
	ASSERT_EQ(NULL, description->measurementTypeInfo);
	ASSERT_EQ(0, description->nrOfValues);

	delete description;
	description = NULL;
}
