/*
 * Reencryptor_unittest.cc
 *
 *  Created on: Oct 9, 2018
 *      Author: zyxxroj
 *
 */

// Library includes
#include <gtest/gtest.h>

// Reencryptor includes
#include "Defines.h"
#include "ImmUtil.h"
#include "SecUtil.h"
#include "Reencryptor.h"
#include "FileReaderUtil.h"
#include "Trace.h"
#include "SecUtil.h"
#include "Reencryptor_CmwUtil_dummy.h"

#include <cstring>

extern SaImmOiCallbacksT_2* sImmOiApplierCallbacks;
extern SaAisErrorT ccbModifyRetVal;
extern std::string ccbModifyObj;
extern bool receivedObjectCreate;
extern bool receivedCcbModify;

struct AutoCreateFile
{
	std::string fileName;

	AutoCreateFile(std::string filename = std::string("secretFile")) : fileName(filename)
	{
		//open a sample secret file
		FILE *fPtr = fopen(filename.c_str(), "w");
		if(fPtr)
		{
			fprintf(fPtr,"%s","Snmp,SnmpTargetV3,tag1;attr1,attr2");
		}
		fclose(fPtr);
	}
	~AutoCreateFile()
	{
		EXPECT_EQ(0, remove(fileName.c_str()));
	}
};

SaImmAttrValueT* allocateStringAttrValueArray(const char* value)
{
	char* sp                    = strdup(value);
	char** strp                 = new char*;
	*strp                       = sp;
	SaImmAttrValueT* attrValues = (void**) malloc(1 * sizeof(SaImmAttrValueT));
	attrValues[0]               = strp;

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

SaImmAttrValueT* allocateIntAttrValueArray(int32_t* value, int32_t numVal)
{
	SaImmAttrValueT* attrValues = (void**) malloc(numVal * sizeof(void*));
	if(attrValues != NULL)
	{
		for (unsigned int i = 0; i < numVal; i++)
		{
			attrValues[i] = new int;
			*(int32_t*) attrValues[i] = value[i];
		}
	}
	return attrValues;
}

SaImmAttrValuesT_2* allocateIntAttrValues(const char* attrName, int32_t *value, int32_t numValues)
{
	SaImmAttrValuesT_2* attrValues = (SaImmAttrValuesT_2*) malloc(sizeof(SaImmAttrValuesT_2));
	attrValues->attrName           = strdup(attrName);
	attrValues->attrValueType      = SA_IMM_ATTR_SAINT32T;
	attrValues->attrValuesNumber   = numValues;
	attrValues->attrValues         = allocateIntAttrValueArray(value, numValues);
	return attrValues;
}

void freeAttrValues(SaImmAttrValuesT_2* attrValues)
{
	free(attrValues->attrName);
	for (unsigned int i = 0; i < attrValues->attrValuesNumber; i++)
	{
		if(SA_IMM_ATTR_SASTRINGT == attrValues->attrValueType) {
			free(*(char**)attrValues->attrValues[i]);
		}
		delete attrValues->attrValues[i];
		attrValues->attrValues[i] = NULL;
	}
	free(attrValues->attrValues);
	attrValues->attrValues = NULL;
	free(attrValues);
	attrValues = NULL;
}

TEST (ReencryptorTest, FileNotExist_DelayedCreation)
{
	FileReaderUtil::Instance().init("secretFile");

	mapImmPathToAttributesT pEncryptedAttrData;
	bool result = FileReaderUtil::Instance().getSecretAttrData(pEncryptedAttrData);

	EXPECT_FALSE(result);
	EXPECT_TRUE(pEncryptedAttrData.empty());

	AutoCreateFile file;
	result = FileReaderUtil::Instance().getSecretAttrData(pEncryptedAttrData);

	EXPECT_TRUE(result);
	EXPECT_FALSE(pEncryptedAttrData.empty());
}

TEST (ReencryptorTest, getSecretAttrData)
{
	mapImmPathToAttributesT pEncryptedAttrData;
	int i = 0;
	char *dn[] = { "Snmp", "SnmpTargetV3", "tag1" };
	char *attr[] = { "attr1", "attr2"};

	AutoCreateFile file;

	FileReaderUtil::Instance().init(file.fileName.c_str());
	bool result = FileReaderUtil::Instance().getSecretAttrData(pEncryptedAttrData);
	EXPECT_EQ(true, result);

	for (mapImmPathToAttributesIterT mapIt= pEncryptedAttrData.begin(); mapIt != pEncryptedAttrData.end() ; ++mapIt )
	{
		std::queue<std::string> mocList = mapIt->first;
		std::vector<std::string> attrList = mapIt->second;

		while(!mocList.empty())
		{
			std::string className = std::string(mocList.front());
			EXPECT_EQ(0, strcmp(className.c_str(),dn[i]));
			i++;
			mocList.pop();
		}

		for (i = 0; i < static_cast<int>(attrList.size()); i++)
		{
			EXPECT_EQ(0, strcmp(attrList[i].c_str(), attr[i]));
		}
	}
}

TEST (ReencryptorTest, reencrypt)
{
	std::string oldSecret = "1:oIvxVToboOwV5k6QVAdrmFhIvutlPI7kGjUUFi1FD3PCM8wb9YCe5W3QEQGWRo8fFE";
	std::string newSecret;
	bool reencrypt_val = SecUtil::Instance().reencrypt(oldSecret, newSecret);
	EXPECT_EQ(reencrypt_val, true);
	EXPECT_EQ(newSecret, "Avi:oIvxVToboOwV5k6QVAdrmFhIvutlPI7kGjUUFi1FD3PCM8wb9YCe5W3QEQGWRo8fFE");
}

TEST (ReencryptorTest, encryptFail)
{
	std::string oldSecret = "1:@failEncrypt@oIvxVToboOwV5k6QVAdrmFhIvutlPI7kGjUUFi1FD3PCM8wb9YCe5W3QEQGWRo8fFE";
	std::string newSecret;
	bool reencrypt_val = SecUtil::Instance().reencrypt(oldSecret, newSecret);
	EXPECT_FALSE(reencrypt_val);
}

TEST (ReencryptorTest, decryptFail)
{
	std::string oldSecret = "@failDecrypt@1:IvxVToboOwV5k6QVAdrmFhIvutlPI7kGjUUFi1FD3PCM8wb9YCe5W3QEQGWRo8fFE";
	std::string newSecret;
	bool reencrypt_val = SecUtil::Instance().reencrypt(oldSecret, newSecret);
	EXPECT_FALSE(reencrypt_val);
}

TEST (ReencryptorTest, emptySecret)
{
	std::string oldSecret;
	std::string newSecret;
	bool reencrypt_val = SecUtil::Instance().reencrypt(oldSecret, newSecret);
	EXPECT_FALSE(reencrypt_val);
}

TEST (ReencryptorTest, start)
{
	SaImmAttrValuesT_2* attrVal1 = allocateStringAttrValues("encryptionKeyUuid", "ChildKey");
	SaImmAttrValuesT_2* attrValues1[] = {attrVal1, NULL};
	add2Accessor("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionKeyUuid", (SaImmAttrValuesT_2**)&attrValues1, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal2 = allocateStringAttrValues("encryptionKeyUuid", "ParentKey");
	SaImmAttrValuesT_2* attrValues2[] = {attrVal2, NULL};
	add2Accessor("secEncryptionParticipantMId=1", "encryptionKeyUuid", (SaImmAttrValuesT_2**)&attrValues2, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal3 = allocateStringAttrValues("attr1", "1:heymamamamamma");
	SaImmAttrValuesT_2* attrValues3[] = {attrVal3, NULL};
	add2Accessor("Snmp=1,SnmpTargetV3=1,tag1=Avi", "attr1", (SaImmAttrValuesT_2**)&attrValues3, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal4 = allocateStringAttrValues("attr2", "1:heymamamamammaa");
	SaImmAttrValuesT_2* attrValues4[] = {attrVal4, NULL};
	add2Accessor("Snmp=1,SnmpTargetV3=1,tag1=Avi", "attr2", (SaImmAttrValuesT_2**)&attrValues4, SA_IMM_ATTR_SASTRINGT);

	ObjectQueueT objs1;
	objs1.push(std::string("Snmp=1"));
	add2Search("", "Snmp", objs1);

	ObjectQueueT objs2;
	objs2.push(std::string("Snmp=1,SnmpTargetV3=1"));
	add2Search("Snmp=1", "SnmpTargetV3", objs2);

	ObjectQueueT objs3;
	objs3.push(std::string("Snmp=1,SnmpTargetV3=1,tag1=Avi"));
	add2Search("Snmp=1,SnmpTargetV3=1", "tag1", objs3);

	AutoCreateFile file;

	//Actual Tests...
	Reencryptor::Instance().start(file.fileName.c_str());

	ASSERT_TRUE(assertCcbValue("Snmp=1,SnmpTargetV3=1,tag1=Avi","attr1", "Avi:heymamamamamma"));
	ASSERT_TRUE(assertCcbValue("Snmp=1,SnmpTargetV3=1,tag1=Avi","attr2", "Avi:heymamamamammaa"));

	ASSERT_TRUE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionKeyUuid", "ParentKey"));
	ASSERT_TRUE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionStatus", 0));

	resetAllCaches();
	freeAttrValues(attrVal1); attrVal1 = NULL;
	freeAttrValues(attrVal2); attrVal2 = NULL;
	freeAttrValues(attrVal3); attrVal3 = NULL;
	freeAttrValues(attrVal4); attrVal4 = NULL;
	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, doReencryption_alreadyDone)
{

	SaImmAttrValuesT_2* attrVal1 = allocateStringAttrValues("encryptionKeyUuid", "SameKey");
	SaImmAttrValuesT_2* attrValues1[] = {attrVal1, NULL};
	add2Accessor("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionKeyUuid", (SaImmAttrValuesT_2**)&attrValues1, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal2 = allocateStringAttrValues("encryptionKeyUuid", "SameKey");
	SaImmAttrValuesT_2* attrValues2[] = {attrVal2, NULL};
	add2Accessor("secEncryptionParticipantMId=1", "encryptionKeyUuid", (SaImmAttrValuesT_2**)&attrValues2, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal3 = allocateStringAttrValues("attr1", "1:heymamamamamma");
	SaImmAttrValuesT_2* attrValues3[] = {attrVal3, NULL};
	add2Accessor("Snmp=1,SnmpTargetV3=1,tag1=Avi", "attr1", (SaImmAttrValuesT_2**)&attrValues3, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal4 = allocateStringAttrValues("attr2", "1:heymamamamammaa");
	SaImmAttrValuesT_2* attrValues4[] = {attrVal4, NULL};
	add2Accessor("Snmp=1,SnmpTargetV3=1,tag1=Avi", "attr2", (SaImmAttrValuesT_2**)&attrValues4, SA_IMM_ATTR_SASTRINGT);

	int32_t encStatus[1] = {0};
	SaImmAttrValuesT_2* attrVal5 = allocateIntAttrValues("encryptionStatus", encStatus, 1);
	SaImmAttrValuesT_2* attrValues5[] = {attrVal5, NULL};
	add2Accessor("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionStatus", (SaImmAttrValuesT_2**)&attrValues5, SA_IMM_ATTR_SAINT32T);

	ObjectQueueT objs1;
	objs1.push(std::string("Snmp=1"));
	add2Search("", "Snmp", objs1);

	ObjectQueueT objs2;
	objs2.push(std::string("Snmp=1,SnmpTargetV3=1"));
	add2Search("Snmp=1", "SnmpTargetV3", objs2);

	ObjectQueueT objs3;
	objs3.push(std::string("Snmp=1,SnmpTargetV3=1,tag1=Avi"));
	add2Search("Snmp=1,SnmpTargetV3=1", "tag1", objs3);

	AutoCreateFile file;

	//Actual Tests...
	Reencryptor::Instance().start(file.fileName.c_str());

	ASSERT_FALSE(assertCcbValue("Snmp=1,SnmpTargetV3=1,tag1=Avi","attr1", "Avi:heymamamamamma"));
	ASSERT_FALSE(assertCcbValue("Snmp=1,SnmpTargetV3=1,tag1=Avi","attr2", "Avi:heymamamamammaa"));

	ASSERT_FALSE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionKeyUuid", "SameKey"));
	ASSERT_FALSE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionStatus", 0));

	freeAttrValues(attrVal1); attrVal1 = NULL;
	freeAttrValues(attrVal2); attrVal2 = NULL;
	freeAttrValues(attrVal3); attrVal3 = NULL;
	freeAttrValues(attrVal4); attrVal4 = NULL;
	freeAttrValues(attrVal5); attrVal5 = NULL;
	resetAllCaches();
	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, doReencryption_negative1)
{
	Reencryptor::Instance().setCcbId(6);
	receivedCcbModify = false;

	//CCB IDs don't match
	Reencryptor::Instance().doReencryption(9);
	ASSERT_FALSE(receivedCcbModify);

	resetAllCaches();
}

TEST (ReencryptorTest, doReencryption_negative2)
{
	//TestInfo: Failed reencryptions

	SaImmAttrValuesT_2* attrVal1 = allocateStringAttrValues("encryptionKeyUuid", "ChildKey");
	SaImmAttrValuesT_2* attrValues1[] = {attrVal1, NULL};
	add2Accessor("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionKeyUuid", (SaImmAttrValuesT_2**)&attrValues1, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal2 = allocateStringAttrValues("encryptionKeyUuid", "ParentKey");
	SaImmAttrValuesT_2* attrValues2[] = {attrVal2, NULL};
	add2Accessor("secEncryptionParticipantMId=1", "encryptionKeyUuid", (SaImmAttrValuesT_2**)&attrValues2, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal3 = allocateStringAttrValues("attr1", "1:@failEncrypt@heymamamamamma");
	SaImmAttrValuesT_2* attrValues3[] = {attrVal3, NULL};
	add2Accessor("Snmp=1,SnmpTargetV3=1,tag1=Avi", "attr1", (SaImmAttrValuesT_2**)&attrValues3, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal4 = allocateStringAttrValues("attr2", "1:heymamamamammaa");
	SaImmAttrValuesT_2* attrValues4[] = {attrVal4, NULL};
	add2Accessor("Snmp=1,SnmpTargetV3=1,tag1=Avi", "attr2", (SaImmAttrValuesT_2**)&attrValues4, SA_IMM_ATTR_SASTRINGT);

	ObjectQueueT objs1;
	objs1.push(std::string("Snmp=1"));
	add2Search("", "Snmp", objs1);

	ObjectQueueT objs2;
	objs2.push(std::string("Snmp=1,SnmpTargetV3=1"));
	add2Search("Snmp=1", "SnmpTargetV3", objs2);

	ObjectQueueT objs3;
	objs3.push(std::string("Snmp=1,SnmpTargetV3=1,tag1=Avi"));
	add2Search("Snmp=1,SnmpTargetV3=1", "tag1", objs3);

	AutoCreateFile file;
	Reencryptor::Instance().setCcbId(0);

	//Actual Tests...
	Reencryptor::Instance().start(file.fileName.c_str());

	ASSERT_FALSE(assertCcbValue("Snmp=1,SnmpTargetV3=1,tag1=Avi","attr1", "Avi:@failEncrypt@heymamamamamma"));
	ASSERT_FALSE(assertCcbValue("Snmp=1,SnmpTargetV3=1,tag1=Avi","attr2", "Avi:heymamamamammaa"));

	ASSERT_TRUE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionKeyUuid", "ParentKey"));
	ASSERT_TRUE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionStatus", 2));

	freeAttrValues(attrVal1); attrVal1 = NULL;
	freeAttrValues(attrVal2); attrVal2 = NULL;
	freeAttrValues(attrVal3); attrVal3 = NULL;
	freeAttrValues(attrVal4); attrVal4 = NULL;
	resetAllCaches();
	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, handleOiCcbObjectAbortCallback)
{
	AutoCreateFile file;
	Reencryptor::Instance().setCcbId(9);
	Reencryptor::Instance().start(file.fileName.c_str());

	if (sImmOiApplierCallbacks) {
		sImmOiApplierCallbacks->saImmOiCcbAbortCallback(6, 9);
	}
	Reencryptor::Instance().stop();

	resetAllCaches();
	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, updateParentEncryptionKeyUuid)
{
	char* val = "1";
	char* objectName = "encryptionKeyUuid";
	void* attrValue1 = &val;

	SaImmAttrValuesT_2 val1 = {
			objectName,						// name
			SA_IMM_ATTR_SASTRINGT,			// type
			1,								// number of values
			(SaImmAttrValueT*) &attrValue1	// pointer to the value
	};

	SaNameT* objName = ImmUtil::Instance().makeSaNameT(std::string(SEC_ENCRYPTION_PARTICIPANTM_OBJECT));
	SaUint64T immHandle = 1;
	SaUint64T ccbId = 1;
	SaImmAttrModificationT_2 value;
	value.modType = SA_IMM_ATTR_VALUES_REPLACE;
	value.modAttr = val1;

	const SaImmAttrModificationT_2** attrMods = new const SaImmAttrModificationT_2*[2];
	attrMods[0]	= &value;
	attrMods[1] = NULL;

	Reencryptor::Instance().updateParentEncryptionKeyUuid(attrMods);

	delete objName;
	objName = NULL;

	delete []attrMods;
	attrMods =NULL;

	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, handleOiCcbApplyCallback)
{
	AutoCreateFile file;

	Reencryptor::Instance().setCcbId(9);
	Reencryptor::Instance().start(file.fileName.c_str());
	if (sImmOiApplierCallbacks)
		sImmOiApplierCallbacks->saImmOiCcbApplyCallback(6, 9);
	Reencryptor::Instance().stop();

	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, handleOiCcbObjectCreateCallback)
{
	SaStringT className = "immclass";
	SaNameT* objName = ImmUtil::Instance().makeSaNameT(std::string(SEC_ENCRYPTION_PARTICIPANTM_OBJECT));
	const SaImmAttrValuesT_2** attrValues = NULL;

	AutoCreateFile file;
	Reencryptor::Instance().setCcbId(9);//to stop unnecessary reencryption
	Reencryptor::Instance().start(file.fileName.c_str());

	if (sImmOiApplierCallbacks) {
		sImmOiApplierCallbacks->saImmOiCcbObjectCreateCallback (6, 9, className, objName, attrValues);
	}

	delete objName;
	objName = NULL;
	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, handleOiCcbCompletedCallback)
{
	AutoCreateFile file;
	Reencryptor::Instance().setCcbId(9);//to stop unnecessary reencryption
	Reencryptor::Instance().start(file.fileName.c_str());

	if (sImmOiApplierCallbacks) {
		sImmOiApplierCallbacks->saImmOiCcbCompletedCallback(6, 9);
	}
	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, handleOiCcbObjectDeleteCallback)
{
	SaNameT* objName = ImmUtil::Instance().makeSaNameT(std::string(SEC_ENCRYPTION_PARTICIPANTM_OBJECT));


	AutoCreateFile file;
	Reencryptor::Instance().setCcbId(9);//to stop unnecessary reencryption
	Reencryptor::Instance().start(file.fileName.c_str());

	if (sImmOiApplierCallbacks) {
		sImmOiApplierCallbacks->saImmOiCcbObjectDeleteCallback(6, 9, objName);
	}
	sImmOiApplierCallbacks = NULL;

	delete objName;
	objName = NULL;
}

TEST (ReencryptorTest, errorStrings)
{
	SaImmAttrValuesT_2* attrVal1 = allocateStringAttrValues("encryptionKeyUuid", "ChildKey");
	SaImmAttrValuesT_2* attrValues1[] = {attrVal1, NULL};
	add2Accessor("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionKeyUuid", (SaImmAttrValuesT_2**)&attrValues1, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal2 = allocateStringAttrValues("encryptionKeyUuid", "ParentKey");
	SaImmAttrValuesT_2* attrValues2[] = {attrVal2, NULL};
	add2Accessor("secEncryptionParticipantMId=1", "encryptionKeyUuid", (SaImmAttrValuesT_2**)&attrValues2, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal3 = allocateStringAttrValues("attr1", "1:heymamamamamma");
	SaImmAttrValuesT_2* attrValues3[] = {attrVal3, NULL};
	add2Accessor("Snmp=1,SnmpTargetV3=1,tag1=Avi", "attr1", (SaImmAttrValuesT_2**)&attrValues3, SA_IMM_ATTR_SASTRINGT);

	SaImmAttrValuesT_2* attrVal4 = allocateStringAttrValues("attr2", "1:heymamamamammaa");
	SaImmAttrValuesT_2* attrValues4[] = {attrVal4, NULL};
	add2Accessor("Snmp=1,SnmpTargetV3=1,tag1=Avi", "attr2", (SaImmAttrValuesT_2**)&attrValues4, SA_IMM_ATTR_SASTRINGT);

	ObjectQueueT objs1;
	objs1.push(std::string("Snmp=1"));
	add2Search("", "Snmp", objs1);

	ObjectQueueT objs2;
	objs2.push(std::string("Snmp=1,SnmpTargetV3=1"));
	add2Search("Snmp=1", "SnmpTargetV3", objs2);

	ObjectQueueT objs3;
	objs3.push(std::string("Snmp=1,SnmpTargetV3=1,tag1=Avi"));
	add2Search("Snmp=1,SnmpTargetV3=1", "tag1", objs3);

	AutoCreateFile file;
	ccbModifyRetVal = SA_AIS_ERR_FAILED_OPERATION;
	ccbModifyObj = std::string("Snmp=1,SnmpTargetV3=1,tag1=Avi");

	//Actual Tests...
	Reencryptor::Instance().setCcbId(0);
	Reencryptor::Instance().start(file.fileName.c_str());

	ASSERT_FALSE(assertCcbValue("Snmp=1,SnmpTargetV3=1,tag1=Avi","attr1", "Avi:heymamamamamma"));
	ASSERT_FALSE(assertCcbValue("Snmp=1,SnmpTargetV3=1,tag1=Avi","attr2", "Avi:heymamamamammaa"));

	ASSERT_TRUE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionKeyUuid", "ParentKey"));
	ASSERT_TRUE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "encryptionStatus", 2));
	ASSERT_TRUE(assertCcbValue("secEncryptionParticipantId=COM_APR_9010443,secEncryptionParticipantMId=1", "additionalStatusInfo", "Error from OI: I'm Batman."));

	freeAttrValues(attrVal1); attrVal1 = NULL;
	freeAttrValues(attrVal2); attrVal2 = NULL;
	freeAttrValues(attrVal3); attrVal3 = NULL;
	freeAttrValues(attrVal4); attrVal4 = NULL;
	resetAllCaches();
	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, createObjectSecEncryptionParticipant)
{
	receivedObjectCreate = false;
	ImmUtil::Instance().createObjectSecEncryptionParticipant();
	ASSERT_TRUE(receivedObjectCreate);
}

TEST (ReencryptorTest, handleOiCcbAbortCallback)
{
	AutoCreateFile file;
	Reencryptor::Instance().setCcbId(9);//to stop unnecessary reencryption
	Reencryptor::Instance().start(file.fileName.c_str());

	if (sImmOiApplierCallbacks) {
		sImmOiApplierCallbacks->saImmOiCcbAbortCallback(6, 9);
	}
	sImmOiApplierCallbacks = NULL;
}

TEST (ReencryptorTest, getSeverityFilter)
{
	int32_t sevFilter[2] = {2 , 4};
	SaImmAttrValuesT_2* attrVal = allocateIntAttrValues(LOGM_LOG_SEVERITYFILTER , sevFilter, 2);
	SaImmAttrValuesT_2* attrValues[] = {attrVal, NULL};

	add2Accessor(COMSA_LOGM_DN, LOGM_LOG_SEVERITYFILTER, (SaImmAttrValuesT_2**)&attrValues, SA_IMM_ATTR_SAINT32T);

	EXPECT_EQ(true, Trace::Instance().start());
	EXPECT_EQ(20, Trace::Instance().getLogSeverityFlags());
	sleep(1); // Let the stream thread detach before calling stop
	EXPECT_EQ(true, Trace::Instance().stop());

	freeAttrValues(attrVal); attrVal = NULL;
}
