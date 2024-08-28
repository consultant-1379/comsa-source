/**
* stubs and tests for OamSAImmBridge
*
*   Modify:   xnikvap, xngangu, xjonbuc, xduncao 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
*
*   Modify:   eaparob, ejnolsz 2013-01-31: test cases added: -to test isNotifiable Storage
*                                                            -to test isNotify function together with the Storage
*
*   Modify:   uabjoy   2013-10-01: Added test case for trouble report HR74015, correction of ACtionTest SimpleParameter
*
*   Modify:   eaparob  2013-10-22: test cases added to test "GetClassNameFromImm"
*
*   Modify:   eaparob  2013-10-25: test case corrections: "search" and "delete" sub-TCs from "ImmexistsMo" TC
*
*   Modify:   xdonngu  2014-05-29: usingstd::tr1::shared_ptr for TxContext
*
*   Modify:   xadaleg  2014-08-02:  MR35347 - increase DN length
*/

//#define IMM_A_02_01

/*
#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
*/

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <ctype.h>
#include <fstream>
#include <sstream>

#include <assert.h>
#include <gtest/gtest.h>

// CoreMW
#include "saAis.h"
#include "saImm.h"
#include "saImmOm.h"
#include "saImmOm_A_2_11.h"
// COM
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiComponent_1.h"
#include "MafMgmtSpiInterfacePortal_3.h"
#include "MafMgmtSpiServiceIdentities_1.h"
#include "MafMgmtSpiThreadContext_2.h"
#include "MafOamSpiManagedObject_3.h"
#include "MafOamSpiModelRepository_4.h"
// COM SA
#include "ComSA.h"
#include "OamSATransactionRepository.h"
#include "OamSACache.h"
#include "TxContext.h"
#include "ImmCmd.h"
#include "OamSAImmBridge.h"
#include "OamSATranslator.h"
#include "OamSATransactionRepository.h"
#include "OamSATransactionalResource.h"
#include "DxEtModelConstants.h"
#include <tr1/memory>

bool lock_mo_for_config_change=false;
std::map<std::string, SaImmScopeT> mymap;

using namespace std::tr1;


typedef struct _auto_clean_map
{
	std::map<uint64_t, uint64_t> mapT;
	~_auto_clean_map()
	{
		mapT.clear();
	}
} auto_clean_map;

auto_clean_map attrTypeContMap, attrStructMap;

typedef struct _auto_clean
{
	std::list<void*> listItem;
	~_auto_clean()
	{
		std::list<void*>::iterator it = listItem.begin();
		for(; it != listItem.end(); it++)
		{
			if(*it != NULL)
			{
				delete *it;
				*it = NULL;
			}
		}
	}
} auto_clean;

typedef struct _auto_clean_free
{
	std::list<void*> listItem;
	~_auto_clean_free()
	{
		std::list<void*>::iterator it = listItem.begin();
		for(; it != listItem.end(); it++)
		{
			if(*it != NULL)
			{
				free(*it);
				*it = NULL;
			}
		}
	}
} auto_clean_free;

auto_clean autocleanList;
auto_clean_free autocleanFreeList;

// To make this output DEBUG printouts to std out. Change gLog level variable to LOG_LEVEL_DEBUG and remove the comment below
#define REDIRECT_LOG
#ifdef REDIRECT_LOG
#define DEBUG printf;
#else
#include "trace.h"
#endif

// TESTS
#include "OamSAImmBridge.h"

extern OamSAKeyAttributeRepository theKeyHolder;

typedef CM::ImmCmdOmCcbObjectModify ImmCmdOmCcbObjectModifyT;

// This is used by the OamSATranslator to access IMM data
SaImmAccessorHandleT accessorHandleOI = 1;
static struct MafOamSpiMom* maf_theGlobalMom = NULL;
unsigned int tc_number = 0;
int validate_TC_number = 0;
bool immResourceAbort = false;
SaAisErrorT saAisErr = SA_AIS_OK;
bool ccbValidateFailed = false;
MafOamSpiTransaction_1T* MafOamSpiTransactionStruct_p_v1 = NULL;
MafOamSpiTransaction_2T* MafOamSpiTransactionStruct_p_v2 = NULL;

//////////////////////////////////////////////////////
// Utilities
//////////////////////////////////////////////////////

SaAisErrorT saImmOmClassDescriptionMemoryFree_2(SaImmHandleT immHandle, SaImmAttrDefinitionT_2 **attrDefinitions) {
	return SA_AIS_OK;
}

static bool saImmOmCcbGetErrorStrings_accessed=false;
SaAisErrorT saImmOmCcbGetErrorStrings(SaImmCcbHandleT ccbHandle, const SaStringT **errorStrings)
{
	SaAisErrorT rc=SA_AIS_OK;
	DEBUG("OamSAImmBridge_unittest::saImmOmCcbGetErrorStrings \n");

	SaStringT* eStrings = (SaStringT*) malloc(128 * sizeof(SaStringT));

	static char sKalle[] = "kalle";
	eStrings[0] = sKalle;

	static char sStina1[] = "stina1";
	eStrings[1] = sStina1;

	eStrings[2] = NULL;

	if(immResourceAbort) {
		static char sResourceAbort[] = "IMM: Resource abort";
		eStrings[2] = sResourceAbort;
		eStrings[3] = NULL;
	}

	*errorStrings=eStrings;

	saImmOmCcbGetErrorStrings_accessed=true;

	autocleanFreeList.listItem.push_back(eStrings);

	return rc;
}

void freeAttributeContainerContent(MafMoAttributeValueContainer_3T *attrCnt, int bAll) {		//bAll = true (free memory of attrCnt)
	ENTER_OIPROXY();

	//Check input parameter
	if(!attrCnt) {
		LEAVE_OIPROXY();
		return;
	}

	//Free allocated memory
	if(attrCnt->values)
	{
		if(attrCnt->type == MafOamSpiMoAttributeType_3_STRING)
		{
			for(unsigned int i=0; i<attrCnt->nrOfValues; i++)
			{
				if(attrCnt->values[i].value.theString)
				{
					free(const_cast<char *>(attrCnt->values[i].value.theString));
					attrCnt->values[i].value.theString = NULL;
				}
			}
		}
		else if(attrCnt->type == MafOamSpiMoAttributeType_3_REFERENCE)
		{
			for(unsigned int i=0; i<attrCnt->nrOfValues; i++)
			{
				if(attrCnt->values[i].value.moRef)
				{
					free(const_cast<char *>(attrCnt->values[i].value.moRef));
					attrCnt->values[i].value.moRef = NULL;
				}
			}
		}
		delete []attrCnt->values;
		attrCnt->values = NULL;
	}

	//Check if the attrCnt memory needs to be freed
	if(bAll)
	{
		delete attrCnt;
	}

	LEAVE_OIPROXY();
}


///////////////////////////
// Parameter verifier
///////////////////////////'

class ParameterVerifier {

public:

	ParameterVerifier();
	virtual MafMoAttributeValueContainer_3T **getParams()=0;
	virtual MafOamSpiParameterT* getParameterModelList()=0;
	virtual void checkParameters(SaImmAdminOperationParamsT_2 **params);
	virtual ~ParameterVerifier();

protected:
	MafOamSpiParameter* createParameter(MafOamSpiDatatype dataType, char* name, MafOamSpiParameter* prev);
	void setParam(MafOamSpiMoAttributeType_3 dataType, MafMoAttributeValue_3 *value, int numberValues);
	void checkTypeAndValue(MafOamSpiDatatypeT comType, SaImmValueTypeT immType, MafMoAttributeValue_3 *expectedValue, void *actualValuePt);
	void ReleaseAttributeValueContainer(MafMoAttributeValueContainer_3T* p);

	vector<MafMoAttributeValueContainer_3T*> m_paramList;
	MafOamSpiParameter *m_parameterModel;
};

static ParameterVerifier* sParameterVerifier = 0;

ParameterVerifier::ParameterVerifier() {
	m_parameterModel = 0;
}

void ParameterVerifier::ReleaseAttributeValueContainer(MafMoAttributeValueContainer_3T* p)
{
	if(!p)
	{
		return;
	}
	MafOamSpiMoAttributeType_3 vct = p->type;
	if (vct == MafOamSpiMoAttributeType_3_STRUCT)
	{
		for (unsigned int i = 0; i < p->nrOfValues; i++){
			MafMoAttributeValueStructMember_3T *member = p->values[i].value.structMember;
			while(member != NULL){
				//delete[] (char*)member->memberName;
				ReleaseAttributeValueContainer(member->memberValue);
				MafMoAttributeValueStructMember_3T* tmp = member;
				member = member->next;
				delete tmp;
				tmp = NULL;
			}
		}
	}
	else if (vct == MafOamSpiMoAttributeType_3_STRING)
	{
		for (unsigned int i = 0; i < p->nrOfValues; i++){
			free(const_cast<char *>(p->values[i].value.theString));
			p->values[i].value.theString = NULL;
		}
	}
	else if (vct == MafOamSpiMoAttributeType_3_REFERENCE)
	{
		for (unsigned int i = 0; i < p->nrOfValues; i++){
			free(const_cast<char *>(p->values[i].value.moRef));
			p->values[i].value.moRef = NULL;
		}
	}
	delete [] p->values;
	p->values = NULL;
	delete p;
	p = NULL;
}

ParameterVerifier::~ParameterVerifier() {
	printf("---> ParameterVerifier Destructor called \n");
	vector<MafMoAttributeValueContainer_3T*>::iterator iter = m_paramList.begin();
	while (iter != m_paramList.end()) {
		ReleaseAttributeValueContainer(*iter);
		iter++;
	}
	m_paramList.clear();

	MafOamSpiParameterT* modelParam = m_parameterModel;
	while (modelParam) {
		if (modelParam->parameterType.derivedDatatype != 0) {
			delete modelParam->parameterType.derivedDatatype;
		}
		else if(modelParam->parameterType.structDatatype)
		{
			MafOamSpiStructMemberT *p = modelParam->parameterType.structDatatype->members;
			MafOamSpiStructMemberT *prv = p;
			while(p)
			{
				p = p->next;
				delete prv;
				prv = p;
			}
			delete modelParam->parameterType.structDatatype;
		}
		MafOamSpiParameterT* next = modelParam->next;
		delete modelParam;
		modelParam = next;
	}
	m_parameterModel = 0;
}

///////////////////////////
//STUB IMPL IMM
///////////////////////////'

/////// IMM tracks memory //////////
class ImmMemoryTracker {
public:
	std::vector<SaImmAttrDefinitionT_2**> attrDefs;
	std::vector<SaImmAttrValuesT_2**> attrVals;

	void reg(SaImmAttrDefinitionT_2** defArray) {
		attrDefs.push_back(defArray);
	}

	void reg(SaImmAttrValuesT_2** valArray) {
		attrVals.push_back(valArray);
	}

	void clear(SaImmAttrValuesT_2* v) {
		SaImmAttrValuesT_2* res = v;
		for (int i = 0; i < res->attrValuesNumber; i++) {
			switch (res->attrValueType) {
			case SA_IMM_ATTR_SANAMET:
				delete (SaNameT*) res->attrValues[i];
				break;
			case SA_IMM_ATTR_SASTRINGT:
				delete[] (char*) (*(char**) res->attrValues[i]);
				delete (char**) res->attrValues[i];
				break;
			}
		}
		delete[] res->attrName;
		delete[] res->attrValues;
		delete v;
	}

	void clear(SaImmAttrDefinitionT_2* d) {
		//delete [] (char*)d->attrName;  // why invalid!?!?
		delete d;
	}

	void clear(SaImmAttrValuesT_2** vArray) {
		int i = 0;
		while (vArray[i] != NULL) {
			clear(vArray[i]);
			i = i + 1;
		}
		delete[] vArray;
	}

	void clear(SaImmAttrDefinitionT_2** dArray) {
		int i = 0;
		while (dArray[i] != NULL) {
			clear(dArray[i]);
			i = i + 1;
		}
		delete[] dArray;
	}

	void cleanup() {
		for (int i = 0; i < attrDefs.size(); i++) {
			clear(attrDefs[i]);
		}
		for (int i = 0; i < attrVals.size(); i++) {
			clear(attrVals[i]);
		}
		attrDefs.clear();
		attrVals.clear();
	}
	~ImmMemoryTracker()
	{
		cleanup();
	}
};

// Global Memory tracker!
ImmMemoryTracker immMemoryTrackerG;

// Global variable for switching between different output settings of saImmOmAccessorGet_2
unsigned int saImmOmAccessorGet_2_setting = 0;

/* This class is used for setting back the global variable "saImmOmAccessorGet_2_setting" automatically after the test cases.
* It will set back only if the instance of this class is locally declared in the test case(and not allocated memory for it).
*/
class saImmOmAccessorGet_2_switch
{
public:
	saImmOmAccessorGet_2_switch(unsigned int i){saImmOmAccessorGet_2_setting = i;}
	~saImmOmAccessorGet_2_switch(){saImmOmAccessorGet_2_setting = 0;}
};

bool GetClassNameFromImm_tester_function(unsigned int i)
{
	printf("GetClassNameFromImm_tester_function(): ENTER\n");
	bool result = false;
	// This will set the mocked saImmOmAccessorGet_2() function to use a special setting (not the default which is used by all other test cases)
	saImmOmAccessorGet_2_switch test_case(i);

	// Making an instance of OamSACache to be able to use its functions
	printf("GetClassNameFromImm_tester_function(): Instantiating OamSACache now\n");
	OamSACache cache;
	printf("GetClassNameFromImm_tester_function(): Instantiating OamSACache done\n");

	// Inputs and expected output settings
	const std::string objectName = "someObjectName";
	std::string className;
	std::string expectedClassName = "";

	// Calling the function to test it
	printf("GetClassNameFromImm_tester_function(): Call GetClassNameFromImm() function\n");
	cache.GetClassNameFromImm(objectName, className);
	printf("GetClassNameFromImm_tester_function(): GetClassNameFromImm() returned:\n");
	printf("GetClassNameFromImm_tester_function():    className(%s)\n",className.c_str());

	if(className == expectedClassName)
	{
		result = true;
	}
	printf("GetClassNameFromImm_tester_function(): OamSACache will go out of scope and will destruct itself now\n");
	printf("GetClassNameFromImm_tester_function(): RETURN with (%d)\n",result);
	return result;
}

class ImmItem {
// default constructor and we never copy
public:

	unsigned int type;
	std::vector<std::string> data;

	bool isEmpty() {
		return data.size() == 0;
	}

	std::string toString() {
		std::string outp;
		outp.append("ImmItem( type=");
		switch (type) {
		case 0:
			outp.append("0");
			break;
		case 1:
			outp.append("1");
			break;
		case 2:
			outp.append("2");
			break;
		case 3:
			outp.append("3");
			break;
		case 4:
			outp.append("4");
			break;
		case 5:
			outp.append("5");
			break;
		case 6:
			outp.append("6");
			break;
		case 7:
			outp.append("7");
			break;
		case 8:
			outp.append("8");
			break;
		case 9:
			outp.append("9");
			break;
		default:
			outp.append("n");
			break;
		}
		outp.append(", data=");
		for (int i = 0; i < data.size(); i++) {
			outp.append(data[i]);
			outp.append(" ");
		}
		outp.append(")");
		return outp;
	}
};

class ImmAttrDef {
public:
	unsigned int attrType;
	std::string attrName;
};

class ImmClassDef {
public:
	std::vector<ImmAttrDef> attrDef;
};

class ImmStorage {

	// Storage backend for fake IMM impl
	std::map<std::string, ImmItem> immStorage;
	std::map<std::string, int> createLog;
	std::map<std::string, int> deleteLog;
	std::map<std::string, ImmClassDef> immClassStorage;

public:

	void reset() {
		immStorage.clear();
		createLog.clear();
		deleteLog.clear();
		immClassStorage.clear();
	}

	int getItemCount() {
		return immStorage.size();
	}

	bool isEmpty() {
		return immStorage.empty();
	}

	/**
	* Add a class attribute definition, used by saImmOmClassDecriptionGet_2
	*/
	void addImmClassAttributeDef(std::string className, std::string attrName,
	unsigned int attrType) {
		ImmAttrDef attr;
		attr.attrType = attrType;
		attr.attrName = attrName;
		immClassStorage[className].attrDef.push_back(attr);
	}

	ImmClassDef getClassDef(std::string className) {
		return immClassStorage[className];
	}

	void addToImmStorage(const char* dn, const char* attr, unsigned int type,
	std::vector<std::string> values) {
		std::string key = makeKey(dn, attr);
		ImmItem item;
		item.type = type;
		item.data = values;
		immStorage[key] = item;
	}

	ImmItem readFromImmStorage(const char* dn, const char* attr) {
		std::string key = makeKey(dn, attr);
		if (immStorage.find(key) == immStorage.end()) {
			printf("Error: no such element, dn=%s attribute=%s\n", dn, attr);
			printf("Available elements:\n");
			for (std::map<std::string, ImmItem>::iterator it =
			immStorage.begin(); it != immStorage.end(); it++) {
				printf("   [%s]=>%s\n", (*it).first.c_str(), (*it).second.toString().c_str());
			}
		}
		ImmItem item = immStorage[key]; // creates default value if not found.
		return item;
	}

	ImmItem deleteFromImmStorage(const char* dn, const char* attr) {
		immStorage.erase(makeKey(dn, attr));
	}

	void logCreate(const char* dn) {
		createLog[std::string(dn)] = 1;
	}

	void logDelete(const char* dn) {
		deleteLog[std::string(dn)] = 1;
	}

private:

	std::string makeKey(const char* dn, const char* attr) {
		std::string key(dn);
		key.append(",");
		key.append(attr);
		return key;
	}

};

// Global IMM storage!!
ImmStorage immStorageG;

//////////////////////////////////////////////////////
// Setup model repository
//////////////////////////////////////////////////////

bool existManagedElement = true;

// generalProperties methods defined here
bool isDecorated=false;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// MOCKING FUNCTION FOR SPI MR V4 API  ////////////////////////////////////////////////////////////////////////////////
////////////// ALMOST DUPLICATE MOCKING FUNCTION OF SPI MR V2 /////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////
// Setup model repository
//////////////////////////////////////////////////////

// Support methods for  MafOamSpiModelRepository_4
// Data structure here

// Mocking for MafOamSpiMrEntry_4T
MafOamSpiMrMomHandle_4T Ghandle_V4 = {1};
MafReturnT entry_getMom_V4(MafOamSpiMrMomHandle_4T* handle)
{
	printf("UNITTEST: MafOamSpiMrMomHandle_4T::entry_getMom_V4(): ENTER\n");
	if (existManagedElement)
	{
		handle->handle = Ghandle_V4.handle;
		printf("UNITTEST: MafOamSpiMrMomHandle_4T::entry_getMom_V4(): RETURN MafOk handle(%llu)\n",handle->handle);
		return MafOk;
	}
	return MafNotExist;
}

MafReturnT entry_getRoot_V4(MafOamSpiMrMocHandle_4T* handle)
{
	printf("UNITTEST: MafOamSpiMrMoc_4::entry_getMom_V4(): ENTER\n");
	if (existManagedElement)
	{
		handle->handle = (uint64_t)(((struct MafOamSpiMom*)Ghandle_V4.handle)->rootMoc);
		printf("UNITTEST: MafOamSpiMrMoc_4::entry_getRoot_V3(): RETURN MafOk handle(%llu)\n",handle->handle);
		return MafOk;
	}
	return MafNotExist;
}

MafOamSpiMrEntry_4T entry_V4 = {entry_getMom_V4, 0, 0, entry_getRoot_V4};

// Mocking for MafOamSpiMrGeneralProperties_4T
// Used in test case for GetClassName()

MafReturnT genpro_getStringProperty_V4(MafOamSpiMrGeneralPropertiesHandle_4T handle, MafOamSpiMrGeneralStringProperty_4T property, const char** value)
{
	printf("UNITTEST: genpro_getStringProperty_V4(): ENTER handle(%llu)\n",handle);
	if (handle.handle != 0)
	{
		// OK, this is a pointer to generalproperties, get the name and return MafOk
		*value = ((struct MafOamSpiGeneralProperties*)(handle.handle))->name;
		printf("UNITTEST: genpro_getStringProperty_V4(): RETURN MafOk  with value=%s\n", ((struct MafOamSpiGeneralProperties*)(handle.handle))->name);
		return MafOk;
	}
	printf("UNITTEST: genpro_getStringProperty_V4(): RETURN MafNotExist\n");
	return MafNotExist;
}

//const char* testDomainName = DOMAIN_NAME_COREMW;
const char* testDomainName;// = DOMAIN_NAME_ECIM;
const char* testExtensionName;// = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
const char* testExtensionValue;// = DN_VALUE_MOM_NAME;
MafReturnT genpro_getDomainExtension_V4(MafOamSpiMrGeneralPropertiesHandle_4T handle, const char* domainName, const char* extensionName, const char** value){
	printf("UNITTEST: MafOamSpiMrGeneralProperties_4T::getDomainExtension(): ENTER\n");
	domainName = testDomainName;
	extensionName = testExtensionName;
	*value = testExtensionValue;
	printf("UNITTEST: MafOamSpiMrGeneralProperties_4T::getDomainExtension(): %s \n", value);
	return MafOk;

}

MafOamSpiMrGeneralProperties_4T generalProperties_V4 = {genpro_getStringProperty_V4, 0, 0, genpro_getDomainExtension_V4};

// MafOamSpiMrMom_4 methods defined heres

MafReturnT mom_getGeneralProperties_V4(MafOamSpiMrMomHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle) {
	if (handle.handle != 0) {
		// OK, this is a pointer to generalproperties, get the name and return MafOk
		subHandle->handle = (uint64_t)(&((struct MafOamSpiMom*)(handle.handle))->generalProperties);
		printf("UNITTEST: MafOamSpiMrMom_4::mom_getGeneralProperties_V4(): RETURN MafOk handle(%llu)\n",subHandle->handle);
		return MafOk;
	}
	return MafNotExist;
}

MafReturnT mom_getNext_V4(MafOamSpiMrMomHandle_4T handle, MafOamSpiMrMomHandle_4T* next) {
	// OK, check if there is more MOMS available
	if (((struct MafOamSpiMom*)(handle.handle))->next != NULL) {
		next->handle = (uint64_t)((struct MafOamSpiMom*)(handle.handle))->next;
		return MafOk;
	}
	return MafNotExist;
}

MafOamSpiMrMom_4T mom_V4 = { mom_getGeneralProperties_V4, 0, 0, 0, 0, mom_getNext_V4};

// MafOamSpiMrMoc_4T methods defined here

MafReturnT moc_getGeneralProperties_V4(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle) {
	if (handle.handle != 0) {
		// Assume this is in fact a pointer
		// Return this as a GENERALPROPERTIES pointer for the attribute
		subHandle->handle = (uint64_t)(&(((struct MafOamSpiMoc*)handle.handle)->generalProperties));
		return MafOk;
	}
	return MafNotExist;
}

/**
* Gets the MOM that the MOC belongs to.
*/
MafReturnT moc_getMom_V4(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrMomHandle_4T* subHandle) {
	if (handle.handle != 0)
	{
		// Return this as a attribute pointer for the MOC
		subHandle->handle = (uint64_t)(((struct MafOamSpiMoc*)handle.handle)->mom);
		return MafOk;
	}
	return MafNotExist;
}

MafReturnT moc_getBoolProperty_V4(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrMocBoolProperty_4T property, bool* value)
{
	if (handle.handle != 0)
	{
		// Return this as key moc or not
		*value = ((struct MafOamSpiMoc*)handle.handle)->isRoot;
		printf("UNITTEST: moc_getBoolProperty_V4():  RETURN MafOk\n");
		return MafOk;
	}
	return MafNotExist;
}

/**
* Gets the first attribute.
*/
MafReturnT moc_getAttribute_V4(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrAttributeHandle_4T* subHandle)
{
	printf("UNITTEST: moc_getAttribute_V3(): handle(%llu)\n",handle.handle);
	// OK, check if this is a real moc handle
	if (handle.handle != 0)
	{
		subHandle->handle = (uint64_t)(((struct MafOamSpiMoc*)handle.handle)->moAttribute);
		printf("UNITTEST: getAttribute(): RETURN MafOk subHandle(%llu)\n",subHandle->handle);
		return MafOk;
	}
	printf("UNITTEST: moc_getAttribute_V4(): RETURN MafNotExist\n");
	return MafNotExist;
}

/**
	* Gets the first child containment in the list of child containments for this Moc.
**/
MafReturnT moc_getChildContainment_V4(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrContainmentHandle_4T* child)
{
	printf("UNITTEST: getChildContainment_V4(): handle(%llu)\n",handle.handle);
	//child->handle = (uint64_t)(((struct MafOamSpiMoc*)handle.handle)->childContainment);
	if (handle.handle != 0)
	{
		child->handle = (uint64_t)(((struct MafOamSpiMoc*)handle.handle)->childContainment);
		printf("UNITTEST: moc_getChildContainment_V4(): RETURN MafOk with child->handle(%llu)\n",child->handle);
		return MafOk;
	}
	return MafNotExist;
}
/**
	* Gets the first parent containment in the list if parent containments for this Moc.
**/
MafReturnT moc_getParentContainment_V4(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrContainmentHandle_4T* parent)
{
	printf("UNITTEST: getParentContainment(): ENTER with handle(%llu)\n",handle.handle);
	if (handle.handle != 0)
	{
		// Return this as a attribute pointer for the MOC
		parent->handle = (uint64_t)(((struct MafOamSpiMoc*)handle.handle)->parentContainment);
		if (parent->handle == 0)
		{
			printf("UNITTEST: moc_getParentContainment_V4(): RETURN MafNotExist with parent->handle(%llu)\n",parent->handle);
			return MafNotExist;
		}
		printf("UNITTEST: moc_getParentContainment_V4(): RETURN MafOk with parent->handle(%llu)\n",parent->handle);
		return MafOk;
	}
	return MafNotExist;
}

MafOamSpiMrMoc_4T moc_V4 = {moc_getGeneralProperties_V4, moc_getMom_V4, 0, moc_getBoolProperty_V4, 0, moc_getAttribute_V4, 0, moc_getChildContainment_V4, moc_getParentContainment_V4};


// MafOamSpiMrContainment_4T methods defined here

MafReturnT cont_getGeneralProperties_V4(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle)
{
	printf("UNITTEST: cont_getGeneralProperties_V4(): ENTER with handle(%llu)\n",handle.handle);
	if (handle.handle != 0)
	{
		std::map<uint64_t, uint64_t>::iterator attr_Search = attrTypeContMap.mapT.find(handle.handle);
		if (attr_Search != attrTypeContMap.mapT.end())
		{
			if(((MafOamSpiMoAttribute*)attr_Search->second)->type == MafOamSpiMoAttributeType_STRUCT){
				std::map<uint64_t, uint64_t>::iterator struct_Search = attrStructMap.mapT.find((uint64_t)attr_Search->second);
				if (struct_Search != attrStructMap.mapT.end()){
					subHandle->handle = (uint64_t)&(((MafOamSpiMoAttribute*)struct_Search->second)->generalProperties);
				}
			}else{
				subHandle->handle = (uint64_t)&(((MafOamSpiMoAttribute*)attr_Search->second)->generalProperties);
			}
		}
		printf("UNITTEST: cont_getGeneralProperties_V4():        handle.handle != 0\n");
		subHandle->handle = (uint64_t)(&(((struct MafOamSpiContainment*)(handle.handle))->generalProperties));
		printf("UNITTEST: cont_getGeneralProperties_V4():        generalProperties.name(%s)\n",((struct MafOamSpiContainment*)(handle.handle))->generalProperties.name);
		printf("UNITTEST: cont_getGeneralProperties_V4(): RETURN MafOk subHandle->handle(%llu)\n",subHandle->handle);
		return MafOk;
	}
	printf("UNITTEST: cont_getGeneralProperties_V4():        setting subHandle->handle = 0\n");
	subHandle->handle = 0;
	printf("UNITTEST: cont_getGeneralProperties_V4(): RETURN MafNotExist subHandle->handle(%llu)\n",subHandle->handle);
	return MafNotExist;
}

/**
	* Gets the child MOC.
**/
MafReturnT cont_getChildMoc_V4(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrMocHandle_4T* child)
{
	if (handle.handle != 0) {
		// Assume this is in fact a pointer
		// Return this as a GENERALPROPERTIES pointer for the attribute
		child->handle = (uint64_t)((((struct MafOamSpiContainment*)handle.handle)->childMoc));
		if (child->handle != 0)
		{
			printf("UNITTEST: cont_getChildMoc_V4(): RETURN MafOk parent->handle(%llu)\n",child->handle);
			return MafOk;
		}
	}
	child->handle = 0;
	return MafNotExist;
}

MafReturnT cont_getParentMoc_V4(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrMocHandle_4T* parent)
{
	if (handle.handle != 0) {
		// Return this as a MafOamSpiMocHandleT pointer for the MOC
		parent->handle = (uint64_t)(((struct MafOamSpiContainment*)handle.handle)->parentMoc);
		if (parent->handle != 0)
		{
			printf("UNITTEST: cont_getParentMoc_V4(): RETURN MafOk parent->handle(%llu)\n",parent->handle);
			return MafOk;
		}
	}
	parent->handle = 0;
	return MafNotExist;
}

/**
* Gets the next containment relationship in the list that has the same child MOC.
*/
MafReturnT cont_getNextContainmentSameChildMoc_V4(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrContainmentHandle_4T* next)
{
	printf("UNITTEST: cont_getNextContainmentSameChildMoc_V4(): ENTER with handle(%llu)\n",handle.handle);
	if (handle.handle != 0)
	{
		next->handle = (uint64_t)(((struct MafOamSpiContainment*)handle.handle)->nextContainmentSameChildMoc);
		if (next->handle != 0)
		{
			printf("UNITTEST: cont_getNextContainmentSameChildMoc_V4(): RETURN MafOk next->handle(%llu)\n",next->handle);
			return MafOk;
		}
	}
	next->handle = 0;
	printf("UNITTEST: cont_getNextContainmentSameChildMoc_V4(): RETURN MafNotExist\n");
	return MafNotExist;
}


/**
* Gets the next containment relationship in the list that has the same
* parent MOC.
**/
MafReturnT cont_getNextContainmentSameParentMoc_V4(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrContainmentHandle_4T* next)
{
	if (handle.handle != 0) {
		// Assume this is in fact a pointer
		next->handle = (uint64_t)(((struct MafOamSpiContainment*)handle.handle)->nextContainmentSameParentMoc);
		if (next->handle != 0)
		{
			printf("UNITTEST: cont_getNextContainmentSameParentMoc_V4(): RETURN MafOk next->handle(%llu)\n",next->handle);
			return MafOk;
		}
	}
	next->handle = 0;
	printf("UNITTEST: cont_getNextContainmentSameParentMoc_V4(): RETURN MafNotExist\n");
	return MafNotExist;
}

MafOamSpiMrContainment_4T containment_V4 = {cont_getGeneralProperties_V4, 0, cont_getChildMoc_V4, cont_getParentMoc_V4, cont_getNextContainmentSameChildMoc_V4, cont_getNextContainmentSameParentMoc_V4, 0, 0, 0};


// MafOamSpiMrAttribute_4 method defined here
/**
* Gets the general properties.
*/
MafReturnT attr_getGeneralProperties_V4(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle)
{
	if (handle.handle != 0) {
		// Assume this is in fact a pointer
		// Return this as a GENERALPROPERTIES pointer for the attribute
		subHandle->handle = (uint64_t)(&(((struct MafOamSpiMoAttribute*)handle.handle)->generalProperties));
		return MafOk;
	}
	subHandle->handle = NULL;
	return MafNotExist;
}
/**
 * Gets the MOC an attribute belongs to.
 */
MafReturnT attr_getMoc_V4(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrMocHandle_4T* subHandle)
{
	if (handle.handle != 0) {
		subHandle->handle = (uint64_t)(&(((struct MafOamSpiMoAttribute*)handle.handle)->moc));
		return MafOk;
	}
	subHandle->handle = NULL;
	return MafNotExist;
}

/**
 * Gets the container type
 */
MafReturnT attr_getTypeContainer_V4(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrTypeContainerHandle_4T* subHandle)
{
	if (handle.handle != 0) {
		// Returns this as a type of container
		subHandle->handle = (uint64_t)(&((struct MafOamSpiMrAttribute_4 *)(handle.handle))->getTypeContainer);
		printf("UNITTEST: attr_getTypeContainer_V4():handle =%lu subhandle value: (%llu)\n", handle.handle,(subHandle->handle));
		attrTypeContMap.mapT.insert(std::pair<uint64_t, uint64_t>(subHandle->handle,handle.handle));
		return MafOk;
	}
	subHandle->handle = NULL;
	return MafNotExist;
}

/**
* Gets the next attribute.
*/
MafReturnT attr_getNext_V4(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrAttributeHandle_4T* next)
{
	if (handle.handle != 0) {
		// Assume this is in fact a pointer
		// Return this as a GENERALPROPERTIES pointer for the attribute
		next->handle = (uint64_t)((((MafOamSpiMoAttribute*)handle.handle)->next));
		if (next->handle != NULL)
		{
			return MafOk;
		}
		else
		{
			return MafNotExist;
		}
	}
	return MafNotExist;
}


/**
* Gets an indication if this is a key (naming) attribute of the MOC it
* belongs to. There must be one and only one key attribute per every MOC.
*/
MafReturnT attr_getIsKey_V4(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrAttributeBoolProperty_4T property, bool* value) {
	if (handle.handle != 0) {
		*value = ((MafOamSpiMoAttribute *)(handle.handle))->isKey;
		return MafOk;
	}
	return MafNotExist;
}

/**
 * Get the type of the attribute
 */
MafReturnT container_getType_V4(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrType_4T* attr_type)
{
	if (handle.handle != 0) {
		// Return this as a attribute type
		std::map<uint64_t, uint64_t>::iterator attr_Search = attrTypeContMap.mapT.find(handle.handle);
		if (attr_Search != attrTypeContMap.mapT.end())
		{
			*attr_type = (MafOamSpiMrType_4T)((MafOamSpiMoAttribute*)attr_Search->second)->type;
			return MafOk;
		}
		attr_type = (MafOamSpiMrType_4T*)(&((struct MafOamSpiMrTypeContainer_4 *)handle.handle)->getType);
		return MafOk;
	}
	return MafNotExist;
}

// MafOamSpiMrTypeContainer_4
/**
 * Get the type of the attribute
 */
MafReturnT container_getGeneralProperties_V4(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle)
{
	if (handle.handle != 0) {
		// Assume this is in fact a pointer
		// Return this as a attribute type
		std::map<uint64_t, uint64_t>::iterator attr_Search = attrTypeContMap.mapT.find(handle.handle);
			if (attr_Search != attrTypeContMap.mapT.end())
			{
				if(((MafOamSpiMoAttribute*)attr_Search->second)->type == MafOamSpiMoAttributeType_STRUCT){
					std::map<uint64_t, uint64_t>::iterator struct_Search = attrStructMap.mapT.find((uint64_t)attr_Search->second);
					if (struct_Search != attrStructMap.mapT.end()){
						subHandle->handle = (uint64_t)&(((MafOamSpiMoAttribute*)struct_Search->second)->generalProperties);
					}
				}else{
					subHandle->handle = (uint64_t)&(((MafOamSpiMoAttribute*)attr_Search->second)->generalProperties);
				}
				return MafOk;
			}
		subHandle->handle = (uint64_t)(&((struct MafOamSpiMrTypeContainer_4 *)handle.handle)->getGeneralProperties);
		return MafOk;
	}
	return MafNotExist;
}

// MafOamSpiMrString_4
/**
 * Get the type of the attribute
 */
MafReturnT string_getBoolProperty_V4(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrStringBoolProperty_4T property, bool* value)
{
	if (handle.handle != 0) {
		// Assume this is in fact a pointer
		if (property == MafOamSpiMrTypeContainerBoolProperty_isPassphrase_4) {
			std::map<uint64_t, uint64_t>::iterator attr_Search = attrTypeContMap.mapT.find(handle.handle);
			if (attr_Search != attrTypeContMap.mapT.end())
			{
				std::string attrName((char*)((MafOamSpiMoAttribute*)attr_Search->second)->generalProperties.name);
				//if the attribute name contains passPhrase then consider that as passPhrase attribute

				if(attrName.find("passPhrase")!= std::string::npos){
					*value = true;
				}else{
					*value = false;
				}
				return MafOk;
			}
		}
		*value = false;
	}
	return MafNotExist;
}

MafOamSpiMrString_4 stringIf_V4 = {0,string_getBoolProperty_V4,0};

MafOamSpiMrTypeContainer_4T typeContainer_V4 = { container_getGeneralProperties_V4, container_getType_V4, 0, 0, 0, 0};

MafOamSpiMrAttribute_4T attribute_V4 = { attr_getGeneralProperties_V4, attr_getMoc_V4, attr_getTypeContainer_V4, attr_getNext_V4, 0, attr_getIsKey_V4, 0};

MafOamSpiModelRepository_4T InterfaceStruct_4 = {{ "", "", "4" }, &entry_V4, &generalProperties_V4, &mom_V4, &moc_V4, &containment_V4, 0, 0, 0, &typeContainer_V4, 0, 0, &attribute_V4, 0, 0, 0, &stringIf_V4, 0};

MafOamSpiModelRepository_4T* theModelRepo_v4_p = &InterfaceStruct_4;


// Interface SPI
//

static MafReturnT getMoms(const MafOamSpiMomT ** result)
{
	*result = (MafOamSpiMomT*) maf_theGlobalMom;
	return MafOk;
}

static MafReturnT getMoc(const char* momName, const char* momVersion, const char* mocName, MafOamSpiMocT** result)
{
	return MafOk;
}

MafReturnT getTreeRoot(const MafOamSpiMocT **result)
{
	const struct MafOamSpiMoc* moc = ((struct MafOamSpiMom*)Ghandle_V4.handle)->rootMoc;
	if (moc != NULL){
		*result = (MafOamSpiMocT*)moc;
		return MafOk;
	}
	else {
		return MafNotExist;
	}
}

static MafOamSpiModelRepository_1T InterfaceStruct = { { "", "", "1" }, getMoms, getMoc, getTreeRoot };
MafOamSpiModelRepository_1T* theModelRepo_v1_p = &InterfaceStruct;

/* SDP1694 - support MAF SPI */


// Support only one MOM and ManagedElement in this version.
MafOamSpiMrMocHandle_4T maf_Ghandle = {1};
// Entry methods used defined here
MafReturnT maf_getMom(MafOamSpiMrMocHandle_4T* handle) {
	(*handle).handle = maf_Ghandle.handle;
	return MafOk;
}

/// Builders

struct MafOamSpiMom* maf_makeMom(const char* name, struct MafOamSpiMoc* rootMoc) {
	struct MafOamSpiMom* mom = new struct MafOamSpiMom;
	memset(mom, 0, sizeof(MafOamSpiMom));
	autocleanList.listItem.push_back(mom);
	mom->generalProperties.name = (char*) name;
	mom->next = NULL;
	mom->rootMoc = rootMoc;
	mom->version = (char*) "Unit testing 101";
	mom->release = (char*) "BETA";
	rootMoc->mom = mom;
	rootMoc->isRoot = true;
	if (rootMoc->childContainment != NULL)
	{
		const struct MafOamSpiMoc* childMoc = rootMoc->childContainment->childMoc;
		((struct MafOamSpiMoc*)childMoc)->mom = rootMoc->mom;
	}
	printf("UNITTEST: maf_makeMom(): name(%s) (%llu)\n",mom->generalProperties.name, (uint64_t)mom);
	// Add the MOM to the model repository version two test code
	// FIXME : THIS NEEDS TO BE FIXED IF TO TEST WITH THIS INTERFACE
	//addMOMHandle(mom);
	return mom;
}

struct MafOamSpiMoc* maf_makeMoc(const char* name, struct MafOamSpiMoAttribute* attrList) {
	struct MafOamSpiMoc* moc = new struct MafOamSpiMoc;
	memset(moc, 0, sizeof(MafOamSpiMoc));
	autocleanList.listItem.push_back(moc);
	moc->generalProperties.name = (char*) name;
	moc->isRoot = false;
	moc->moAttribute = attrList;
	moc->parentContainment = NULL;
	moc->childContainment = NULL;
	moc->constraint = NULL;
	moc->moAction = NULL;
	moc->mom = NULL;
	if (attrList != NULL)
	{
		attrList->moc = moc;
		if (attrList->next != NULL)
		{
			const struct MafOamSpiMoAttribute* nextAttr = attrList->next;
			((struct MafOamSpiMoAttribute*)nextAttr)->moc = moc;
		}
	}
	printf("UNITTEST: maf_makeMoc(): name(%s) (%llu) parentContainment(%llu)\n",moc->generalProperties.name, (uint64_t)moc, (uint64_t)(moc->parentContainment));
	return moc;
}

struct MafOamSpiContainment* maf_makeContribution(const char* name, MafOamSpiMoc* parent, MafOamSpiMoc* child, MafOamSpiContainmentT* preSameParent, MafOamSpiContainmentT* preSameChild) {
	struct MafOamSpiContainment* cont = new struct MafOamSpiContainment;
	memset(cont, 0, sizeof(MafOamSpiContainment));
	autocleanList.listItem.push_back(cont);
	cont->generalProperties.name = (char*) name;
	cont->parentMoc = parent;
	cont->childMoc = child;
	if (preSameParent == NULL && preSameChild == NULL)
	{
		cont->nextContainmentSameParentMoc = NULL;
		cont->nextContainmentSameChildMoc = NULL;
		parent->childContainment = cont;
		child->parentContainment = cont;
	}
	else
	{
		cont->nextContainmentSameParentMoc = NULL;
		cont->nextContainmentSameChildMoc = NULL;
		if(preSameParent !=NULL) ((struct MafOamSpiContainment*)preSameParent)->nextContainmentSameParentMoc = cont;
		if(preSameChild !=NULL) ((struct MafOamSpiContainment*)preSameChild)->nextContainmentSameChildMoc = cont;
	}
	printf("UNITTEST: maf_makeContribution(): cont(%llu)\n",(uint64_t)cont);
	return cont;
}

struct MafOamSpiContainment* maf_makeContainment(struct MafOamSpiMoc* parent, struct MafOamSpiMoc* child) {
	struct MafOamSpiContainment* cont = new struct MafOamSpiContainment;
	memset(cont, 0, sizeof(MafOamSpiContainment));
	autocleanList.listItem.push_back(cont);
	cont->generalProperties.name = "C1BMomContainment";
	cont->parentMoc = parent;
	cont->childMoc = child;
	cont->nextContainmentSameParentMoc = NULL;
	cont->nextContainmentSameChildMoc = NULL;

	// update child and parent
	parent->childContainment = cont;
	child->parentContainment = cont;
	//parent->mom = mom;
	child->mom = parent->mom;
	printf("UNITTEST: maf_makeContainment(): cont(%llu)\n",(uint64_t)cont);
	return cont;
}

struct MafOamSpiContainment* maf_makeContainment(struct MafOamSpiMoc* parent, struct MafOamSpiMoc* child, const char* name) {
	struct MafOamSpiContainment* cont = new struct MafOamSpiContainment;
	memset(cont, 0, sizeof(MafOamSpiContainment));
	autocleanList.listItem.push_back(cont);
	cont->generalProperties.name = (char*)name;
	cont->parentMoc = parent;
	cont->childMoc = child;
	cont->nextContainmentSameParentMoc = NULL;
	cont->nextContainmentSameChildMoc = NULL;

	// update child and parent
	parent->childContainment = cont;
	child->parentContainment = cont;
	//parent->mom = mom;
	child->mom = parent->mom;
	printf("UNITTEST: maf_makeContainment(): cont(%llu)\n",(uint64_t)cont);
	return cont;
}

struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, MafOamSpiMoAttributeType_3T type, struct MafOamSpiMoAttribute* next) {
	struct MafOamSpiMoAttribute* attr = new struct MafOamSpiMoAttribute;
	memset(attr, 0, sizeof(MafOamSpiMoAttribute));
	autocleanList.listItem.push_back(attr);
	attr->generalProperties.name = (char*) name;
	attr->type = (MafOamSpiMoAttributeTypeT)type;
	attr->isKey = false;
	//attr->moc = moc;
	attr->next = next;
	return attr;
}

void maf_makeParentContainmentToTop(struct MafOamSpiMoc* childClass) {
	// Set the parent Containment to ManagedElement
	const struct MafOamSpiMoc* ManagedElement = ((const struct MafOamSpiMom*)Ghandle_V4.handle)->rootMoc;
	struct MafOamSpiContainment* cont = new struct MafOamSpiContainment;
	memset(cont, 0, sizeof(MafOamSpiContainment));
	autocleanList.listItem.push_back(cont);
	cont->parentMoc = ManagedElement;
	cont->childMoc = childClass;
	cont->nextContainmentSameParentMoc = NULL;
	cont->nextContainmentSameChildMoc = NULL;
	cont->generalProperties.name = "dummyContainment";
	((struct MafOamSpiMoc*)ManagedElement)->childContainment = cont;
	childClass->parentContainment = cont;
	printf("UNITTEST: maf_makeParentContainmentToTop(): (%llu)\n",(uint64_t)cont);
}


void maf_makeParentContainmentToTop(struct MafOamSpiMoc* childClass1, struct MafOamSpiMoc* childClass2) {
	// Set the parent Containment to ManagedElement
	const struct MafOamSpiMoc* ManagedElement = ((const struct MafOamSpiMom*)Ghandle_V4.handle)->rootMoc;
	struct MafOamSpiContainment* cont1 = new struct MafOamSpiContainment;
	memset(cont1, 0, sizeof(MafOamSpiContainment));
	struct MafOamSpiContainment* cont2 = new struct MafOamSpiContainment;
	memset(cont2, 0, sizeof(MafOamSpiContainment));
	autocleanList.listItem.push_back(cont1);
	autocleanList.listItem.push_back(cont2);

	//1st MOM
	cont1->parentMoc = ManagedElement;
	cont1->childMoc = childClass1;
	cont1->nextContainmentSameParentMoc = cont2;
	cont1->nextContainmentSameChildMoc = NULL;
	cont1->generalProperties.name = "ManageElement_to_C1A";

	//2nd MOM
	cont2->parentMoc = ManagedElement;
	cont2->childMoc = childClass2;
	cont2->nextContainmentSameParentMoc = NULL;
	cont2->nextContainmentSameChildMoc = NULL;
	cont2->generalProperties.name = "ManageElement_to_C2A";

	((struct MafOamSpiMoc*)ManagedElement)-> childContainment = cont1;
	//((struct MafOamSpiMoc*)ManagedElement)-> childContainment->nextContainmentSameParentMoc = cont2;
	childClass1->parentContainment = cont1;
	childClass2->parentContainment = cont2;
	printf("UNITTEST: maf_makeParentContainmentToTop(): cont1=(%llu) cont2=(%llu)\n",(uint64_t)cont1, (uint64_t)cont2);
}

struct MafOamSpiMoAttribute* maf_makeAttribute(char* name, struct MafOamSpiStruct* structData, struct MafOamSpiMoAttribute* next) {
	struct MafOamSpiMoAttribute* attr = new struct MafOamSpiMoAttribute;
	memset(attr, 0, sizeof(MafOamSpiMoAttribute));
	autocleanList.listItem.push_back(attr);
	attr->generalProperties.name = (char*) name;
	attr->generalProperties.description = (char*) "This is an attribute in UnitTesting";
	attr->generalProperties.specification = (char*) "A specification";
	attr->generalProperties.status = MafOamSpiStatus_CURRENT;
	attr->generalProperties.hidden = (char*) "The hidden!!";

	attr->type = MafOamSpiMoAttributeType_STRUCT; // implicit
	attr->structDatatype = structData;
	attrStructMap.mapT.insert(std::pair<uint64_t, uint64_t>((uint64_t)attr,(uint64_t)structData));
	//attr->moc = moc;
	attr->next = next;
	return attr;
}

struct MafOamSpiStruct* maf_makeStruct(char* name, struct MafOamSpiStructMember* memberList) {
	struct MafOamSpiStruct* s = new struct MafOamSpiStruct;
	memset(s, 0, sizeof(MafOamSpiStruct));
	autocleanList.listItem.push_back(s);
	s->generalProperties.name = name;
	s->members = memberList;
	return s;
}

struct MafOamSpiStructMember* maf_makeStructMember(char* name, MafOamSpiDatatype type, struct MafOamSpiStructMember* next) {
	struct MafOamSpiStructMember* m = new struct MafOamSpiStructMember;
	memset(m, 0, sizeof(MafOamSpiStructMember));
	autocleanList.listItem.push_back(m);
	m->generalProperties.name = name;
	m->memberType.type = type;
	m->next = next;
	return m;
}

// Export the global MOM pointer
//
//
void maf_setMom(struct MafOamSpiMom* theMom) {
	maf_theGlobalMom = theMom;

	// Create an entry for ManagedElement
	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"managedElementId", MafOamSpiMoAttributeType_3_STRING, NULL);
	attrs->isKey = true;
	struct MafOamSpiMoc* FileMMoc = maf_makeMoc("ManagedElement", attrs);
	FileMMoc->isRoot = true;
	struct MafOamSpiMom* model = maf_makeMom("ManagedElement",FileMMoc);
	printf("UNITTEST: maf_setMom(): FileMMoc(%llu)\n",(uint64_t)FileMMoc);

	// set up the first primary MOM to this
	Ghandle_V4.handle = (uint64_t)model;
	printf("UNITTEST: maf_setMom(): Ghandle_V4.handle(%llu)\n",Ghandle_V4.handle);
	existManagedElement = true;

	// OK, add the mom to ManagedElement
	((struct MafOamSpiMom*)Ghandle_V4.handle)->next = theMom;
}

//
// Interface SPI
//
static MafReturnT maf_getMoms(const MafOamSpiMomT ** result) {
	*result = (MafOamSpiMomT*) maf_theGlobalMom;
	return MafOk;
}

static MafReturnT maf_getMoc(const char* momName, const char* momVersion, const char* mocName, MafOamSpiMocT** result)
{
	return MafOk;
}

static MafOamSpiModelRepository_1T maf_InterfaceStruct = { { "", "", "1" }, maf_getMoms, maf_getMoc, NULL };
MafOamSpiModelRepository_1T* maf_theModelRepo_v1_p = &maf_InterfaceStruct;



#ifdef REDIRECT_LOG
////////////////////////////////////3//////////////////
// LOG
//////////////////////////////////////////////////////
#ifdef __cplusplus
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
	void coremw_debug_log(int priority, char const* fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		coremw_vlog2(priority, fmt, ap);
		va_end(ap);
	}

#ifdef __cplusplus
}
#endif

#endif //REDIRECT_LOG
//////////////////////////////////////////////////////
// SPI Interface portal exports
//////////////////////////////////////////////////////


// SPI interfaces
MafOamSpiManagedObject_3T* _managedObject;
MafMgmtSpiThreadContext_2T* _threadContext;


static bool get_interface_mock_accessed=false;
// Interface names
const std::string _threadContextId = MafMgmtSpiThreadContext_2Id.interfaceName;

// added for mock portal
MafReturnT get_interface_mock(MafMgmtSpiInterface_1T id, MafMgmtSpiInterface_1T** result)
{
	DEBUG("id.interfaceName:%s\n",id.interfaceName);
	if (_threadContextId == id.interfaceName)
	{
		*result = (MafMgmtSpiInterface_1T*)_threadContext;
		DEBUG("OamSAImmBridge_unittest::get_interface_moc (_threadContextId == id.interfaceName)\n");
		get_interface_mock_accessed=true;
		return MafOk;
	}


	return MafFailure;
} // get_interface_mock

static bool addMessage_mock_accessed=false;
static std::vector<std::string> addMessage_mock_strings;

MafReturnT addMessage_mock(ThreadContextMsgCategory_2T category, const char* message)
{
	DEBUG("OamSAImmBridge_unittest::addMessage_mock\n");
	addMessage_mock_strings.push_back(message);
	addMessage_mock_accessed=true;
	return MafOk;
}

MafMgmtSpiInterfacePortal_3T* portal = NULL;

MafReturnT registerParticipant(MafOamSpiTransactionHandleT txHandle, MafOamSpiTransactionalResource_1T * resp) {
	return MafOk;
}

MafReturnT setContext(MafOamSpiTransactionHandleT txHandle, MafOamSpiTransactionalResource_1T *resource, void *context) {
	return MafOk;
}

MafReturnT getContext(MafOamSpiTransactionHandleT txHandle, MafOamSpiTransactionalResource_1T *resource, void **context) {
	return MafOk;
}

MafReturnT getLockPolicy(MafOamSpiTransactionHandleT txHandle, MafLockPolicyT *result) {
	return MafOk;
}

MafOamSpiTransaction_1T type = { { "", "", "" }, registerParticipant, setContext, getContext, getLockPolicy };

#ifdef __cplusplus
extern "C" {
#endif
	MafReturnT join(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT prepare(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT commit(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT myabort(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT finish(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}

	MafReturnT validate(MafOamSpiTransactionHandleT txHandle, bool *result) {
		return MafOk;
	}
	/*
MafOamSpiTransactionalResource_1T myTxRes = { { "", "", "" }, join, prepare,
		commit, myabort, finish };

MafOamSpiTransactionalResource_1T*
ExportOamSATransactionalResourceInterface(void) {
	return &myTxRes;
}
*/
	MafOamSpiTransactionalResource_2T myTxRes2 = { { "", "", "" }, join, prepare,
		commit, myabort, finish, validate };

	MafOamSpiTransactionalResource_2T*
	ExportOamSATransactionalResourceInterface(void) {
		return &myTxRes2;
	}

#ifdef __cplusplus
}
#endif

/* SDP1694 -support MAF SPI */

// SPI interfaces
MafOamSpiManagedObject_3T* _maf_managedObject;
MafMgmtSpiThreadContext_2T* _maf_threadContext;

static bool maf_get_interface_mock_accessed=false;
// Interface names
const std::string _maf_threadContextId = MafMgmtSpiThreadContext_2Id.interfaceName;

// added for mock portal
MafReturnT maf_get_interface_mock(MafMgmtSpiInterface_1T id, MafMgmtSpiInterface_1T** result)
{
	DEBUG("id.interfaceName:%s\n",id.interfaceName);
	if (_threadContextId == id.interfaceName)
	{
		*result = (MafMgmtSpiInterface_1T*)_maf_threadContext;
		DEBUG("OamSAImmBridge_unittest::get_interface_moc (_threadContextId == id.interfaceName)\n");
		maf_get_interface_mock_accessed=true;
		return MafOk;
	}


	return MafFailure;
} // get_interface_mock

static bool maf_addMessage_mock_accessed=false;
static std::vector<std::string> maf_addMessage_mock_strings;

MafReturnT maf_addMessage_mock(ThreadContextMsgCategory_2T category, const char* message)
{
	DEBUG("OamSAImmBridge_unittest::maf_addMessage_mock\n");
	maf_addMessage_mock_strings.push_back(message);
	maf_addMessage_mock_accessed=true;
	return MafOk;
}

MafMgmtSpiInterfacePortal_3T* maf_portal = NULL;

MafReturnT maf_registerParticipant(MafOamSpiTransactionHandleT txHandle,
		MafOamSpiTransactionalResource_1T * resp) {
	return MafOk;
}

MafReturnT maf_setContext(MafOamSpiTransactionHandleT txHandle,
		MafOamSpiTransactionalResource_1T *resource, void *context) {
	return MafOk;
}

MafReturnT maf_getContext(MafOamSpiTransactionHandleT txHandle,
		MafOamSpiTransactionalResource_1T *resource, void **context) {
	return MafOk;
}

MafReturnT maf_getLockPolicy(MafOamSpiTransactionHandleT txHandle,
		MafLockPolicyT *result) {
	return MafOk;
}

MafOamSpiTransaction_1T maf_type = { { "", "", "" }, maf_registerParticipant,
		maf_setContext, maf_getContext, maf_getLockPolicy };

MafOamSpiTransaction_1* MafOamSpiTransactionStruct_p = &maf_type;

#ifdef __cplusplus
extern "C" {
#endif
	MafReturnT maf_join(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT maf_prepare(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT maf_commit(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT maf_myabort(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT maf_finish(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}

	MafOamSpiTransactionalResource_1T maf_myTxRes = { { "", "", "" }, maf_join, maf_prepare,
		maf_commit, maf_myabort, maf_finish };

	MafOamSpiTransactionalResource_1T*
	maf_ExportOamSATransactionalResourceInterface(void) {
		return &maf_myTxRes;
	}
#ifdef __cplusplus
}
#endif





//////////////////////////////////////////////////////
// IMM API
//////////////////////////////////////////////////////
SaAisErrorT autoRetry_saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle) {
	printf("----> autoRetry_saImmOmAccessorFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOmFinalize(SaImmHandleT immHandle) {
	printf("----> autoRetry_saImmOmFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmSearchFinalize(SaImmSearchHandleT searchHandle) {
	printf("----> saImmOmSearchFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmCcbFinalize(SaImmCcbHandleT ccbHandle) {
	printf("----> saImmOmCcbFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAdminOwnerFinalize(SaImmAdminOwnerHandleT ownerHandle) {
	printf("----> saImmOmAdminOwnerFinalize \n");
	return SA_AIS_OK;
}

// This function is used directly by COMSA MO cache
SaAisErrorT saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
		const SaNameT *objectName, const SaImmAttrNameT *attributeNames,
		SaImmAttrValuesT_2 ***attributes)
{
	if(saImmOmAccessorGet_2_setting == 0)
	{
		printf("----> saImmOmAccessorGet_2 \n");
		char* tmp = new char[saNameLen(objectName) + 1];
		memcpy(tmp, saNameGet(objectName), saNameLen(objectName));
		tmp[saNameLen(objectName)] = 0;

		std::string dn(tmp);
		std::string attrName(attributeNames[0]);
		delete[] tmp;

		printf("saImmOmAccessorGet_2 dn=%s attrName=%s \n", dn.c_str(),	attrName.c_str());
		CM::ImmCmdOmAccessorGet immGet(NULL, dn, attrName, attributes); // no need to run execute() on command, we stubbed this command!
		return SA_AIS_OK;
	}
	else if(saImmOmAccessorGet_2_setting == 1)
	{
		printf("UNITTEST: saImmOmAccessorGet_2(): saImmOmAccessorGet_2_setting case %u ENTER\n",saImmOmAccessorGet_2_setting);
		*attributes = NULL;
		printf("UNITTEST: saImmOmAccessorGet_2(): saImmOmAccessorGet_2_setting case %u RETURN SA_AIS_OK\n",saImmOmAccessorGet_2_setting);
		return SA_AIS_OK;
	}
	else if(saImmOmAccessorGet_2_setting == 2)
	{
		printf("UNITTEST: saImmOmAccessorGet_2(): saImmOmAccessorGet_2_setting case %u ENTER\n",saImmOmAccessorGet_2_setting);
		SaImmAttrValuesT_2 **attributeList = new SaImmAttrValuesT_2*[2];
		attributeList[0] = NULL;
		attributeList[1] = NULL;
		*attributes = attributeList;
		printf("UNITTEST: saImmOmAccessorGet_2(): saImmOmAccessorGet_2_setting case %u RETURN SA_AIS_OK\n",saImmOmAccessorGet_2_setting);
		immMemoryTrackerG.reg(attributeList);
		return SA_AIS_OK;
	}
	else if(saImmOmAccessorGet_2_setting == 3)
	{
		printf("UNITTEST: saImmOmAccessorGet_2(): saImmOmAccessorGet_2_setting case %u ENTER\n",saImmOmAccessorGet_2_setting);
		SaImmAttrValuesT_2 **attributeList = new SaImmAttrValuesT_2*[2];
		SaImmAttrValuesT_2 *theAttribute = new SaImmAttrValuesT_2;
		//theAttribute->attrValuesNumber = 0;
		memset(theAttribute, 0, sizeof(SaImmAttrValuesT_2));
		attributeList[0] = theAttribute;
		attributeList[1] = NULL;
		*attributes = attributeList;
		printf("UNITTEST: saImmOmAccessorGet_2(): saImmOmAccessorGet_2_setting case %u RETURN SA_AIS_OK\n",saImmOmAccessorGet_2_setting);
		immMemoryTrackerG.reg(attributeList);
		return SA_AIS_OK;
	}
	else if(saImmOmAccessorGet_2_setting == 4)
	{
		printf("----> saImmOmAccessorGet_2 \n");
		return SA_AIS_ERR_NOT_EXIST;
	}
}

SaAisErrorT saImmOmFinalize(SaImmHandleT immHandle) {
	printf("----> saImmOmFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmInitialize(SaImmHandleT *immHandle,
		const SaImmCallbacksT *immCallbacks, SaVersionT *version) {
	printf("----> saImmOmInitialize \n");
	(*immHandle) = (SaImmHandleT) 1; // set to value != 0 to inidcate success!
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOmInitialize(SaImmHandleT *immHandle, const SaImmCallbacksT *immCallbacks, const SaVersionT *version)
{
	printf("----> saImmOmInitialize \n");
	*immHandle = (SaImmHandleT) 1; // set to value != 0 to inidcate success!
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOmInitialize_getVersion(SaImmHandleT *immHandle, const SaImmCallbacksT *immCallbacks, SaVersionT *version)
{
	printf("----> saImmOmInitialize \n");
	*immHandle = (SaImmHandleT) 1; // set to value != 0 to inidcate success!
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOmAccessorInitialize(SaImmHandleT immHandle,	SaImmAccessorHandleT *accessorHandle)
{
	printf("----> saImmOmAccessorInitialize \n");
	(*accessorHandle) = (SaImmAccessorHandleT) 1; // set to value != 0 to show success
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAccessorInitialize(SaImmHandleT immHandle,
		SaImmAccessorHandleT *accessorHandle) {
	printf("----> saImmOmAccessorInitialize \n");
	(*accessorHandle) = (SaImmAccessorHandleT) 1; // set to value != 0 to show success
	return SA_AIS_OK;
}

SaAisErrorT saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle) {
	printf("----> saImmOmAccessorFinalize \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmSearchInitialize_2(SaImmHandleT immHandle,
		const SaNameT *rootName,
		SaImmScopeT scope,
		SaImmSearchOptionsT searchOptions,
		const SaImmSearchParametersT_2 *searchParam,
		const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOmSearchInitialize_2(SaImmHandleT immHandle,
		const SaNameT *rootName,
		SaImmScopeT scope,
		SaImmSearchOptionsT searchOptions,
		const SaImmSearchParametersT_2 *searchParam,
		const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
	printf("----> autoRetry_saImmOmSearchInitialize_2 \n");
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOmSearchNext_2(SaImmSearchHandleT searchHandle,
		SaNameT *objectName, SaImmAttrValuesT_2 ***attributes)
{
	printf("----> autoRetry_saImmOmSearchNext_2 \n");
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOmSearchFinalize(SaImmSearchHandleT searchHandle) {
	printf("----> autoRetry_saImmOmSearchFinalize \n");
	return SA_AIS_OK;
}

unsigned int saImmOmClassDescriptionGet_2_setting_OK = true;
SaAisErrorT saImmOmClassDescriptionGet_2(SaImmHandleT immHandle,
		const SaImmClassNameT className, SaImmClassCategoryT *classCategory,
		SaImmAttrDefinitionT_2 ***attrDefinitions) {
	if (saImmOmClassDescriptionGet_2_setting_OK)
	{
		printf("----> saImmOmClassDescriptionGet_2 NORMAL\n");

		ImmClassDef immDef = immStorageG.getClassDef(className);

		SaImmAttrDefinitionT_2** defArray = new SaImmAttrDefinitionT_2*[immDef.attrDef.size() + 1];

		for (int i = 0; i < immDef.attrDef.size(); i++) {

			SaImmAttrDefinitionT_2* def = new SaImmAttrDefinitionT_2;

			def->attrName = (char *) immDef.attrDef[i].attrName.c_str();
			def->attrValueType = (SaImmValueTypeT) immDef.attrDef[i].attrType;
			def->attrFlags = 0;
			def->attrDefaultValue = NULL;
			printf("   attrDef: name=%s type=%d\n", immDef.attrDef[i].attrName.c_str(), immDef.attrDef[i].attrType);
			defArray[i] = def;
		}
		defArray[immDef.attrDef.size()] = NULL;

		(*attrDefinitions) = defArray;
		immMemoryTrackerG.reg(defArray);

		return SA_AIS_OK;
	}
	else {
		printf("----> saImmOmClassDescriptionGet_2 ERROR\n");
		return SA_AIS_ERR_NOT_EXIST;
	}
}


///////////////////////////////////
// ImmCommands
///////////////////////////////////


extern SaVersionT CM::ImmCmd::mVersion;

CM::ImmCmd::ImmCmd(TxContext * txContextIn, std::string cmd, int retries) :
	mTxContextIn(txContextIn), mCmd(cmd), mRetries(retries) {
	ENTER();
	LEAVE();
}

CM::ImmCmd::~ImmCmd() {
	//ENTER();
	//int size = mTmpDns.size();
	//for (int i=0; i<size; i++)
	//{
	//	delete mTmpDns.at(i);
	//}
	//LEAVE();
}

SaAisErrorT CM::ImmCmd::execute() {
	return doExecute();
}

SaNameT *
CM::ImmCmd::toSaNameT(std::string &dnIn) {
	ENTER();
	// temp SaNameTs are deleted in destructor
	//   SaNameT * dn = NULL;
	//  if (dnIn.size()>0) {
	//     dn = new SaNameT();
	//    mTmpDns.push_back(dn);
	//from the root
	//      dn->length = dnIn.length();
	//      snprintf((char*)(dn->value), saNameMaxLen(),"%s", dnIn.c_str());
	//  }
	//LEAVE();
	return NULL;
}

//------------------------ ImmCmdOmInit ----------------------------------------------------
CM::ImmCmdOmInit::ImmCmdOmInit(TxContext * txContextIn) :
	ImmCmd(txContextIn, "saImmOmInitialize") {
	ENTER();
	LEAVE();
}

SaAisErrorT CM::ImmCmdOmInit::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAccessorInit ----------------------------------------------------

CM::ImmCmdOmAccessorInit::ImmCmdOmAccessorInit(TxContext * txContextIn) :
	ImmCmd(txContextIn, "saImmOmAccessorInitialize") {
	ENTER();
	LEAVE();
}

SaAisErrorT CM::ImmCmdOmAccessorInit::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOwnerInit ----------------------------------------------------
CM::ImmCmdOmAdminOwnerInit::ImmCmdOmAdminOwnerInit(TxContext * txContextIn,
		std::string immOwnerNameIn, SaBoolT releaseOnFinalizeIn) :
	ImmCmd(txContextIn, "saImmOmAdminOwnerInitialize"), mImmOwnerNameIn(
		immOwnerNameIn), mReleaseOnFinalizeIn(releaseOnFinalizeIn) {
}

SaAisErrorT ImmCmdOmAdminOwnerInit::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOwnerSet ----------------------------------------------------
CM::ImmCmdOmAdminOwnerSet::ImmCmdOmAdminOwnerSet(TxContext * txContextIn,
		std::vector<std::string> *objectDns, SaImmScopeT scope) :
	ImmCmd(txContextIn, "saImmOmAdminOwnerSet"), mObjectDns(objectDns), mScope(
		scope) {
}

SaAisErrorT CM::ImmCmdOmAdminOwnerSet::doExecute() {
	if(lock_mo_for_config_change){
		std::map<std::string, SaImmScopeT>::iterator it;
		it = mymap.find((*mObjectDns)[0]);
		if(it == mymap.end()){
			mymap.insert(std::pair<std::string, SaImmScopeT>((*mObjectDns)[0],mScope));
			return SA_AIS_OK;
		}else{
			if(it->second >= mScope){
				return SA_AIS_ERR_EXIST;
			}else{
				it->second = mScope;
				return SA_AIS_OK;
			}
		}
	}else{
		return SA_AIS_OK;
	}
}

//------------------------ ImmCmdOmAdminOwnerClear ----------------------------------------------------
CM::ImmCmdOmAdminOwnerClear::ImmCmdOmAdminOwnerClear(TxContext * txContextIn,
		std::string dnIn, SaImmScopeT scope) :
	ImmCmd(txContextIn, "saImmOmAdminOwnerClear"), mDnIn(dnIn), mScope(scope) {
}

SaAisErrorT CM::ImmCmdOmAdminOwnerClear::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOwnerRelease ----------------------------------------------------
ImmCmdOmAdminOwnerRelease::ImmCmdOmAdminOwnerRelease(TxContext * txContextIn,
		std::vector<std::string> *objectDns, SaImmScopeT scope) :
	ImmCmd(txContextIn, "saImmOmAdminOwnerRelease"), mObjectDns(objectDns),
		mScope(scope) {
}

SaAisErrorT ImmCmdOmAdminOwnerRelease::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmClassDescriptionGet ----------------------------------------------------
CM::ImmCmdOmClassDescriptionGet::ImmCmdOmClassDescriptionGet(
		TxContext * txContextIn/*in*/, SaImmClassNameT className/*in*/,
		SaImmClassCategoryT *classCategory/*out*/,
		SaImmAttrDefinitionT_2 *** attrDef/*out*/) :
	ImmCmd(txContextIn, "saImmOmClassDescription_2"), mAttrDef(attrDef),
		mClassCategory(classCategory), mClassName(className) {
}

SaAisErrorT CM::ImmCmdOmClassDescriptionGet::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmKeySearchInit ----------------------------------------------------
CM::ImmCmdOmKeySearchInit::ImmCmdOmKeySearchInit(TxContext *txContextIn,
		std::string dnIn, std::string classNameIn, BridgeImmIterator* biter) :
	ImmCmd(txContextIn, "saImmOmSearchInitialize_2"), mDnIn(dnIn),
		mClassNameIn(classNameIn) {
	printf("-----> ImmCmdOmKeySearchInit: dn=%s classname=%s\n", dnIn.c_str(),
	classNameIn.c_str());
}

SaAisErrorT CM::ImmCmdOmKeySearchInit::doExecute() {
	return SA_AIS_OK;
}

// result string for CM::ImmCmdOmSearchNext::ImmCmdOmSearchNext
std::string GLOBAL_immCmdOmSearchNextDnOut;
std::vector<std::string> GLOBAL_immCmdOmSearchNextDnOut_ptr;
bool isSDP1694 = false;
bool returnBadHandle = false;
short nRetries_BadHandle = 0;
//------------------------ ImmCmdOmKeySearchNext ----------------------------------------------------
CM::ImmCmdOmSearchNext::ImmCmdOmSearchNext(TxContext *txContextIn,
		std::string dnIn, BridgeImmIterator* biter, std::string *dnOut/*out*/,
		SaImmAttrValuesT_2*** attrValsOut/*out*/) :
	ImmCmd(txContextIn, "saImmOmSearchNext_2"), mDnIn(dnIn), mDnOut(dnOut),
		mAttrValsOut(attrValsOut) {
	// returnt the response
	// note that the ImmBridge does not seem to care about attrvalsOut at all, only the dnOut.
	if (isSDP1694)
	{
		if (!GLOBAL_immCmdOmSearchNextDnOut_ptr.empty())
		(*dnOut) = GLOBAL_immCmdOmSearchNextDnOut_ptr.back();
		else
		(*dnOut).clear();
	}
	else {
		(*dnOut) = GLOBAL_immCmdOmSearchNextDnOut;
	}
	printf("-----> ImmCmdOmSearchNext: dn=%s dnOut=%s\n", dnIn.c_str(), (*dnOut).c_str());
}

SaAisErrorT CM::ImmCmdOmSearchNext::doExecute() {
	if (isSDP1694)
	{
		if(!GLOBAL_immCmdOmSearchNextDnOut_ptr.empty())
		{
			GLOBAL_immCmdOmSearchNextDnOut_ptr.pop_back();
			return SA_AIS_OK;
		}
		else {
			return SA_AIS_ERR_NOT_EXIST;
		}
	}
	else if(returnBadHandle) {
		if(nRetries_BadHandle < 3) {
			nRetries_BadHandle++;
			return SA_AIS_ERR_BAD_HANDLE;
		} else {
			return SA_AIS_ERR_TRY_AGAIN;
		}
	}
	else
	{
		if (!GLOBAL_immCmdOmSearchNextDnOut.empty()) {
			return SA_AIS_OK;
		} else {
			return SA_AIS_ERR_NOT_EXIST;
		}
	}
}

//------------------------ ImmCmAccessorGet ----------------------------------------------------

bool throw_exception = false;
CM::ImmCmdOmAccessorGet::ImmCmdOmAccessorGet(TxContext *txContextIn,
		std::string dnIn, std::string attrNameIn,
		SaImmAttrValuesT_2*** attrValsOut /*out*/) :
	ImmCmd(txContextIn, "saImmOmAccessorGet_2"), mDnIn(dnIn), mAttrNameIn(
		attrNameIn), mAttrValsOut(attrValsOut) {
	ImmItem item = immStorageG.readFromImmStorage(dnIn.c_str(),
	attrNameIn.c_str());
	SaImmAttrValuesT_2* res = new SaImmAttrValuesT_2; // allocate value space
	SaImmAttrValuesT_2** resPtr = new SaImmAttrValuesT_2*[2]; // allocate pointer array
	resPtr[0] = res; // add our attribute to the array
	resPtr[1] = NULL; // null terminate array of attributes (no counter!)

	res->attrName = new char[attrNameIn.size() + 1];
	memcpy(res->attrName, attrNameIn.c_str(), attrNameIn.size());
	res->attrName[attrNameIn.size()] = 0;

	int nrofValues = item.data.size();
	res->attrValueType = (SaImmValueTypeT) item.type;
	res->attrValuesNumber = nrofValues;
	if (nrofValues == 0) {
		if (throw_exception) {
			throw_exception = false;
			res->attrValues = NULL;
			immMemoryTrackerG.reg(resPtr);
			throw 1;
		}
		// exit if there are no values!
		res->attrValues = NULL;
		(*attrValsOut) = resPtr;
		immMemoryTrackerG.reg(resPtr);
		return;
	}
	res->attrValues = new void*[nrofValues]; // allocate array of void*
	for (int i = 0; i < nrofValues; i++) {

		switch (res->attrValueType) {
		case SA_IMM_ATTR_SANAMET: {

				// make a SaNameT
				SaNameT* buf = new SaNameT;
				saNameSet(item.data[i].c_str(), buf);
				res->attrValues[i] = (void*) buf;
				break;
			}
		case SA_IMM_ATTR_SASTRINGT: {
				// make a char**
				int size = item.data[i].size();
				char* buf = new char[size + 1];
				char** bufPtr = new char*;
				*bufPtr = buf;
				strcpy(buf, item.data[i].c_str());
				res->attrValues[i] = (void*) bufPtr;
				break;
			}
		default: {
				printf(
				"ERROR in CM::ImmCmdOmAccessorGet::ImmCmdOmAccessorGet, unsupported TYPE %d\n",
				res->attrValueType);

			}
		}
	}
	(*attrValsOut) = resPtr;
	immMemoryTrackerG.reg(resPtr);
}

SaAisErrorT CM::ImmCmdOmAccessorGet::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbInitialize ----------------------------------------------------
CM::ImmCmdOmCcbInit::ImmCmdOmCcbInit(TxContext *txContextIn) :
	ImmCmd(txContextIn, "saImmOmCcbInitialize") {
}

SaAisErrorT CM::ImmCmdOmCcbInit::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbFinalize ----------------------------------------------------
CM::ImmCmdOmCcbFinalize::ImmCmdOmCcbFinalize(TxContext *txContextIn) :
	ImmCmd(txContextIn, "saImmOmCcbFinalize") {
}

SaAisErrorT CM::ImmCmdOmCcbFinalize::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbObjectModify ----------------------------------------------------
CM::ImmCmdOmCcbObjectModify::ImmCmdOmCcbObjectModify(TxContext *txContextIn,
		SaNameT* dnIn, SaImmAttrModificationT_2 **attrModsIn) :
	ImmCmd(txContextIn, "saImmOmCcbObjectModify_2"), mDnIn(dnIn), mAttrModsIn(
		attrModsIn) {
	//	char* dn = new char[dnIn->length+1];
	//	memcpy(dn,dnIn->value,dnIn->length);
	//	dn[dnIn->length]=0; // make a c-string!!

	char* dn = makeCString(dnIn);

	SaImmAttrModificationT_2** modArray = attrModsIn;

	int i = 0;
	while (modArray[i] != NULL) {
		SaImmAttrModificationT_2* mod = modArray[i];
		switch (mod->modType) {

		case SA_IMM_ATTR_VALUES_ADD:
		case SA_IMM_ATTR_VALUES_REPLACE: {
				std::vector<std::string> values;
				for (int a = 0; a < mod->modAttr.attrValuesNumber; a++) {
					// assume string type!!
					switch (mod->modAttr.attrValueType) {
					case SA_IMM_ATTR_SASTRINGT: {
							std::string strval(*((char**) mod->modAttr.attrValues[a]));
							values.push_back(strval);
							printf(
							"ModifyImm: dn=%s modtype=%d attr=%s attrtype=%d attval=%s\n",
							dn, mod->modType, mod->modAttr.attrName,
							mod->modAttr.attrValueType, strval.c_str());
							break;
						}
					case SA_IMM_ATTR_SANAMET: {
							std::string strval(((char*) mod->modAttr.attrValues[a]));
							values.push_back(strval);
							printf(
							"ModifyImm: dn=%s modtype=%d attr=%s attrtype=%d attval=%s\n",
							dn, mod->modType, mod->modAttr.attrName,
							mod->modAttr.attrValueType, strval.c_str());
							break;
						}
					default: {
							printf(
							"ERROR: Unsupported datatype in CM::ImmCmdOmCcbObjectModify::ImmCmdOmCcbObjectModify, currently IMM stub backend only support SA_STRING and SA_NAME\n");
						}

					}
				}
				immStorageG.addToImmStorage(dn, mod->modAttr.attrName,
				mod->modAttr.attrValueType, values);
				break;
			}
		case SA_IMM_ATTR_VALUES_DELETE: {
				for (int a = 0; a < mod->modAttr.attrValuesNumber; a++) {
					immStorageG.deleteFromImmStorage(dn, mod->modAttr.attrName);
				}

				break;
			}

		}// switch

		i = i + 1;
	}// while
	delete[] dn; // remove tmp


}

SaAisErrorT CM::ImmCmdOmCcbObjectModify::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbApply ----------------------------------------------------
CM::ImmCmdOmCcbApply::ImmCmdOmCcbApply(TxContext *txContextIn) :
	ImmCmd(txContextIn, "saImmOmCcbApply") {
}

SaAisErrorT ImmCmdOmCcbApply::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbValidate ----------------------------------------------------
CM::ImmCmdOmCcbValidate::ImmCmdOmCcbValidate(TxContext *txContextIn) :
	ImmCmd(txContextIn, "saImmOmCcbValidate") {
}

SaAisErrorT ImmCmdOmCcbValidate::doExecute() {
	//set the return value of ccbValidate() as SA_AIS_ERR_FAILED_OPERATION for TC7, TC8, TC9, TC10
	if (ccbValidateFailed)
	{
		saAisErr = (saAisErr == SA_AIS_OK) ? SA_AIS_ERR_FAILED_OPERATION : SA_AIS_OK;
		return saAisErr;
	}
	else if(validate_TC_number != 3)
	{
		return SA_AIS_OK;
	}
	else
	{
		return (SaAisErrorT)-1;
	}
}

//------------------------ ImmCmdOmCcbAbort ----------------------------------------------------
CM::ImmCmdOmCcbAbort::ImmCmdOmCcbAbort(TxContext *txContextIn) :
	ImmCmd(txContextIn, "saImmOmCcbAbort") {
}

SaAisErrorT ImmCmdOmCcbAbort::doExecute() {
	if (validate_TC_number != 4){
		return SA_AIS_OK;
	}
	else
	return (SaAisErrorT)-1;
}

/**
* Helper to find the key attribute matching the classname!
*/
std::string findInstanceKey(char* className, SaImmAttrValuesT_2** attrValsIn) {

	std::string keyName(className);
	keyName[0] = tolower(keyName[0]);
	keyName.append("Id");

	int i = 0;
	while (attrValsIn[i] != NULL) {
		if (0 == strcmp(attrValsIn[i]->attrName, keyName.c_str())
				&& attrValsIn[i]->attrValueType == 9) {
			// found key now make instance key!
			std::string instance;
			char* attr = *((char**) attrValsIn[i]->attrValues[0]);
			instance.append(attr);
			return instance;
		}
		i = i + 1;
	}
	return "";
}

//------------------------ ImmCmdOmCcbCreate ----------------------------------------------------
CM::ImmCmdOmCcbObjectCreate::ImmCmdOmCcbObjectCreate(TxContext* txContextIn,
		SaNameT *parentIn, SaImmClassNameT classNameIn,
		SaImmAttrValuesT_2** attrValsIn) :
	ImmCmd(txContextIn, "saImmOmCcbObjectCreate_2"), mParentIn(parentIn),
		mClassNameIn(classNameIn), mAttrValsIn(attrValsIn) {
	char* parentDn = makeCString(parentIn);
	printf("ImmCmdOmCcbObjectCreate parent=%s class=%s\n", parentDn,
	classNameIn);

	// build complete path
	std::string instance = findInstanceKey((char*) classNameIn, attrValsIn);
	std::string dn(instance);
	dn.append(",");
	dn.append(parentDn);

	SaNameT* path = makeSaNameT(dn.c_str());

	SaImmAttrModificationT_2 mod;
	mod.modType = SA_IMM_ATTR_VALUES_REPLACE;
	mod.modAttr.attrValuesNumber = 1;

	SaImmAttrModificationT_2** modArray = new SaImmAttrModificationT_2*[2];
	modArray[0] = &mod;
	modArray[1] = NULL;
	int i = 0;
	while (attrValsIn[i] != NULL) {
		mod.modAttr.attrValueType = attrValsIn[i]->attrValueType;
		mod.modAttr.attrName = attrValsIn[i]->attrName;
		mod.modAttr.attrValuesNumber = attrValsIn[i]->attrValuesNumber;
		mod.modAttr.attrValues = attrValsIn[i]->attrValues;
		//CM::ImmCmdOmCcbObjectModify::ImmCmdOmCcbObjectModify modCmd(txContextIn, path, modArray);
		ImmCmdOmCcbObjectModifyT modCmd(txContextIn, path, modArray);
		i = i + 1;
	}

	modArray[0] = NULL;
	delete[] modArray;
	delete path;
	delete[] parentDn;

}

SaAisErrorT CM::ImmCmdOmCcbObjectCreate::doExecute() {
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmCcbDelete ----------------------------------------------------
CM::ImmCmdOmCcbObjectDelete::ImmCmdOmCcbObjectDelete(TxContext* txContextIn,
		SaNameT* dnIn) :
	ImmCmd(txContextIn, "saImmOmCcbObjectDelete"), mDnIn(dnIn) {
}

SaAisErrorT CM::ImmCmdOmCcbObjectDelete::doExecute() {
	return SA_AIS_OK;
}


//------------------------ ImmCmdOmAdminOperationInvoke ----------------------------------------------------

CM::ImmCmdOmAdminOperationInvoke::ImmCmdOmAdminOperationInvoke(
		TxContext *txContextIn, std::string dnIn,
		SaImmAdminOperationIdT operationId,
		SaImmAdminOperationParamsT_2 **params,
		SaAisErrorT *operationReturnValue /*out*/,
		SaImmAdminOperationParamsT_2 ***returnParams /*out*/) :
	ImmCmd(txContextIn, "saImmOmAdminOperationInvoke_2"), mDnIn(dnIn),
		mOperationId(operationId), mParams(params), mOperationReturnValue(
		operationReturnValue) {
	printf("-----> ImmCmdOmAdminOperationInvoke: dnIn=%s operationId=%i\n",
	dnIn.c_str(), operationId);
}

SaAisErrorT CM::ImmCmdOmAdminOperationInvoke::doExecute() {
	if (sParameterVerifier != 0) {
		printf("-----> ImmCmdOmAdminOperationInvoke:doExecute()\n");
		sParameterVerifier->checkParameters(mParams);
	}
	return SA_AIS_OK;
}

//------------------------ ImmCmdOmAdminOperationMemoryFree ----------------------------------------------------

CM::ImmCmdOmAdminOperationMemoryFree::ImmCmdOmAdminOperationMemoryFree(
		TxContext *txContextIn,
		SaImmAdminOperationParamsT_2 **returnParams) :
	ImmCmd(txContextIn, "saImmOmAdminOperationMemoryFree"), mReturnParams(returnParams) {
	printf("-----> ImmCmdOmAdminOperationMemoryFree\n");
}

SaAisErrorT CM::ImmCmdOmAdminOperationMemoryFree::doExecute() {
	printf("-----> ImmCmdOmAdminOperationMemoryFree:doExecute()\n");
	return SA_AIS_OK;
}

/**
* Allocate c-string on heap
*/
char* allocCstr(const char* str) {
	unsigned int len = strlen(str);
	char* tmp = new char[len + 1]; // the '\0' terminator
	strcpy(tmp, str);
	return tmp;
}

/*
*
* UNIT TESTS
*/


TEST(ImmTest, StorageTest)
{
	std::vector<std::string> values;
	values.push_back(std::string("attrValue1"));
	values.push_back(std::string("attrValue2"));
	immStorageG.addToImmStorage("dn=1,path=2","attrName",9,values);

	ImmItem test = immStorageG.readFromImmStorage("dn=1,path=2","attrName");
	ASSERT_TRUE(test.type==9);
	ASSERT_TRUE(test.data[0]=="attrValue1");
	ASSERT_TRUE(test.data[1]=="attrValue2");
}



TEST(ImmTest, GetImmMoAttribute_V3_TC1)
{
	testDomainName = "";
	testExtensionName = "";
	testExtensionValue = "";
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key2);
	struct MafOamSpiMoc* meMoc = maf_makeMoc("C1A", key);
	struct MafOamSpiMoc* homeMoc = maf_makeMoc("C1B",attrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, homeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("C1Mom",meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	std::vector<std::string> values;
	values.push_back(std::string("tiger"));
	values.push_back(std::string("lion"));
	immStorageG.addToImmStorage("c1BId=2,c1AId=1", "pets", MafOamSpiMoAttributeType_3_STRING, values);

	// setup the call
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *result = NULL;
	MafReturnT ret = testObj.getImmMoAttribute(txh, "ManagedElement=1,C1A=1,C1B=2", "pets", &result);

	// verify the result
	ASSERT_TRUE(ret==MafOk);
	ASSERT_TRUE(result != NULL);
	ASSERT_TRUE(result->type==9);
	ASSERT_TRUE(result->nrOfValues==2);
	ASSERT_TRUE(std::string(result->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result->values[1].value.theString) == std::string("lion"));
}

TEST(ImmTest, GetImmMoAttribute_V3_TC2)
{
	tc_number = 2;
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1key2);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"c2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);
	maf_setMom(C1mom);
	((struct MafOamSpiMom*)C1mom)->next = C2mom;
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc);

	struct MafOamSpiContainment* contribution = maf_makeContribution("C1B_to_C2A", c1BMoc, c2AMoc, NULL, NULL);

	std::vector<std::string> values;
	values.push_back(std::string("tiger"));
	values.push_back(std::string("lion"));
	immStorageG.addToImmStorage("c2BId=1,c2AId=1,c1BId=1,c1AId=1", "c2_pets", MafOamSpiMoAttributeType_3_STRING, values);

	// setup the call
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *result = NULL;
	MafReturnT ret = testObj.getImmMoAttribute(txh, "ManagedElement=1,C1A=1,C1B=1,C2A=1,C2B=1", "c2_pets", &result);


	// verify the result
	ASSERT_TRUE(ret==MafOk);
	ASSERT_TRUE(result != NULL);
	ASSERT_TRUE(result->type==9);
	ASSERT_TRUE(result->nrOfValues==2);
	ASSERT_TRUE(std::string(result->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result->values[1].value.theString) == std::string("lion"));
}

TEST(ImmTest, GetImmMoAttribute_V3_TC3)
{
	tc_number = 3;
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"C1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1attrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"C2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc, c2AMoc);

	struct MafOamSpiContainment* contribution = maf_makeContribution("C2A_to_C1B", c2AMoc,c1BMoc, cont2, cont);
	((struct MafOamSpiMom*)C2mom)->next = C1mom;
	// Set value of attribute of C1B via IMM RDN
	std::vector<std::string> values;
	values.push_back(std::string("tiger"));
	values.push_back(std::string("lion"));
	immStorageG.addToImmStorage("c1BId=1,c1AId=1", "C1_pets", MafOamSpiMoAttributeType_3_STRING, values);

	// Get value of attribute of C1B via 3GPP name
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *result = NULL;
	MafReturnT ret = testObj.getImmMoAttribute(txh, "ManagedElement=1,C1A=1,C1B=1", "C1_pets", &result);

	// verify the result
	ASSERT_TRUE(ret==MafOk);
	ASSERT_TRUE(result != NULL);
	ASSERT_TRUE(result->type==9);
	ASSERT_TRUE(result->nrOfValues==2);
	ASSERT_TRUE(std::string(result->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result->values[1].value.theString) == std::string("lion"));

	// Set value of attribute of C1B via IMM RDN
	immStorageG.addToImmStorage("c1BId=1,c2AId=1", "C1_pets", MafOamSpiMoAttributeType_3_STRING, values);
	// Get value of attribute of C1B via 3GPP name
	MafOamSpiTransactionHandleT txh2=1;
	std::tr1::shared_ptr<TxContext> txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);
	OamSAImmBridge testObj2;
	MafMoAttributeValueContainer_3T *result2 = NULL;
	MafReturnT ret2 = testObj2.getImmMoAttribute(txh2, "ManagedElement=1,C2A=1,C1B=1", "C1_pets", &result2);

	// verify the result
	ASSERT_TRUE(ret2==MafOk);
	ASSERT_TRUE(result2 != NULL);
	ASSERT_TRUE(result2->type==9);
	ASSERT_TRUE(result2->nrOfValues==2);
	ASSERT_TRUE(std::string(result2->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result2->values[1].value.theString) == std::string("lion"));
}

TEST(ImmTest, GetImmMoAttribute_V3_TC4)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"C1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1attrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"C2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc, c2AMoc);
	//((struct MafOamSpiMom*)C2mom)->next = C1mom;
	struct MafOamSpiContainment* contribution = maf_makeContribution("C2B_to_C1B", c2BMoc, c1BMoc, NULL, cont);

	// Set value of attribute of C1B via IMM RDN
	std::vector<std::string> values;
	values.push_back(std::string("tiger"));
	values.push_back(std::string("lion"));
	immStorageG.addToImmStorage("c1BId=1,c1AId=1", "C1_pets", MafOamSpiMoAttributeType_3_STRING, values);

	// Get value of attribute of C1B via 3GPP name
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *result = NULL;
	MafReturnT ret = testObj.getImmMoAttribute(txh, "ManagedElement=1,C1A=1,C1B=1", "C1_pets", &result);

	// verify the result
	ASSERT_TRUE(ret==MafOk);
	ASSERT_TRUE(result != NULL);
	ASSERT_TRUE(result->type==9);
	ASSERT_TRUE(result->nrOfValues==2);
	ASSERT_TRUE(std::string(result->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result->values[1].value.theString) == std::string("lion"));

	// Set value of attribute of C1B via IMM RDN
	immStorageG.addToImmStorage("c1BId=1,c2BId=1,c2AId=1", "C1_pets", MafOamSpiMoAttributeType_3_STRING, values);
	// Get value of attribute of C1B via 3GPP name
	MafOamSpiTransactionHandleT txh2=1;
	std::tr1::shared_ptr<TxContext> txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);
	OamSAImmBridge testObj2;
	MafMoAttributeValueContainer_3T *result2 = NULL;
	MafReturnT ret2 = testObj2.getImmMoAttribute(txh2, "ManagedElement=1,C2A=1,C2B=1,C1B=1", "C1_pets", &result2);

	// verify the result
	ASSERT_TRUE(ret2==MafOk);
	ASSERT_TRUE(result2 != NULL);
	ASSERT_TRUE(result2->type==9);
	ASSERT_TRUE(result2->nrOfValues==2);
	ASSERT_TRUE(std::string(result2->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result2->values[1].value.theString) == std::string("lion"));
}

TEST(ImmTest, GetImmMoAttribute_V3_TC5)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"C1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1attrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"C2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);

	//3rd MOM
	struct MafOamSpiMoAttribute* c3key = maf_makeAttribute((char*)"c3AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c3key->isKey = true;
	struct MafOamSpiMoAttribute* c3a_attrs = maf_makeAttribute((char*)"C3A_pets", MafOamSpiMoAttributeType_3_STRING, c3key);
	// Add the key attribute
	struct MafOamSpiMoAttribute* c3key2 = maf_makeAttribute((char*)"c3BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c3key2->isKey = true;

	struct MafOamSpiMoAttribute* c3b_attrs = maf_makeAttribute((char*)"C3B_pets", MafOamSpiMoAttributeType_3_STRING, c3key2);
	struct MafOamSpiMoc* c3AMoc = maf_makeMoc("C3A", c3a_attrs);
	struct MafOamSpiMoc* c3BMoc = maf_makeMoc("C3B", c3b_attrs);
	struct MafOamSpiMom* C3mom = maf_makeMom("C3Mom",c3AMoc);
	((struct MafOamSpiMoc*)c3BMoc)->mom = C3mom;

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc, c2AMoc);

	// Make relationship
	maf_makeContribution("C2A_to_C1B", c2AMoc,c1BMoc, cont2, cont);
	maf_makeContribution("C1B_to_C3A", c1BMoc, c3AMoc, NULL, NULL);
	maf_makeContribution("C2B_to_C3B", c2BMoc,c3BMoc, NULL, NULL);
	//((struct MafOamSpiMom*)C2mom)->next = C1mom;
	//((struct MafOamSpiMom*)C1mom)->next = C3mom;
	// Set value of attribute of C3B via IMM RDN
	std::vector<std::string> values;
	values.push_back(std::string("tiger"));
	values.push_back(std::string("lion"));

	immStorageG.addToImmStorage("c3BId=1,c2BId=1,c2AId=1", "C3B_pets", MafOamSpiMoAttributeType_3_STRING, values);

	// Get value of attribute of C3B via 3GPP name
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *result = NULL;
	MafReturnT ret = testObj.getImmMoAttribute(txh, "ManagedElement=1,C2A=1,C2B=1,C3B=1", "C3B_pets", &result);


	// verify the result
	ASSERT_TRUE(ret==MafOk);
	ASSERT_TRUE(result != NULL);
	ASSERT_TRUE(result->type==9);
	ASSERT_TRUE(result->nrOfValues==2);
	ASSERT_TRUE(std::string(result->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result->values[1].value.theString) == std::string("lion"));

	// Set value of attribute of C3A via IMM RDN
	std::vector<std::string> values2;
	values2.push_back(std::string("snake"));
	values2.push_back(std::string("fox"));
	immStorageG.addToImmStorage("c3AId=1,c1BId=1,c1AId=1", "C3A_pets", MafOamSpiMoAttributeType_3_STRING, values2);

	// Get value of attribute of C3B via 3GPP name
	MafOamSpiTransactionHandleT txh2=1;
	std::tr1::shared_ptr<TxContext> txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2)	;
	OamSAImmBridge testObj2;
	MafMoAttributeValueContainer_3T *result2 = NULL;
	MafReturnT ret2 = testObj2.getImmMoAttribute(txh2, "ManagedElement=1,C1A=1,C1B=1,C3A=1", "C3A_pets", &result2);

	// verify the result
	ASSERT_TRUE(ret2==MafOk);
	ASSERT_TRUE(result2 != NULL);
	ASSERT_TRUE(result2->type==9);
	ASSERT_TRUE(result2->nrOfValues==2);
	ASSERT_TRUE(std::string(result2->values[0].value.theString) == std::string("snake"));
	ASSERT_TRUE(std::string(result2->values[1].value.theString) == std::string("fox"));

	//std::vector<std::string> values2;
	values2.push_back(std::string("snake"));
	values2.push_back(std::string("fox"));
	// Set value of attribute of C3A via IMM RDN
	immStorageG.addToImmStorage("c3AId=1,c1BId=1,c2AId=1", "C3A_pets", MafOamSpiMoAttributeType_3_STRING, values2);

	// Get value of attribute of C3A via 3GPP name
	MafOamSpiTransactionHandleT txh3=1;
	std::tr1::shared_ptr<TxContext> txContextIn3 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh3);
	OamSAImmBridge testObj3;
	MafMoAttributeValueContainer_3T *result3 = NULL;
	MafReturnT ret3 = testObj3.getImmMoAttribute(txh3, "ManagedElement=1,C2A=1,C1B=1,C3A=1", "C3A_pets", &result3);

	// verify the result
	ASSERT_TRUE(ret3==MafOk);
	ASSERT_TRUE(result3 != NULL);
	ASSERT_TRUE(result3->type==9);
	ASSERT_TRUE(result3->nrOfValues==4);
	ASSERT_TRUE(std::string(result3->values[0].value.theString) == std::string("snake"));
	ASSERT_TRUE(std::string(result3->values[1].value.theString) == std::string("fox"));
}

TEST(ImmTest, GetImmMoAttribute_V3_Abnormal)
{
	testDomainName = DOMAIN_NAME_COREMW;

	testExtensionName = DOMAIN_EXT_NAME_IMM_NAMESPACE;
	testExtensionValue = DN_VALUE_MOM_NAME;
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key2);
	struct MafOamSpiMoc* meMoc = maf_makeMoc("whynot_C1A", key);
	struct MafOamSpiMoc* homeMoc = maf_makeMoc("whynot_C1B",attrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, homeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("C1Mom",meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	std::vector<std::string> values;
	values.push_back(std::string("tiger"));
	values.push_back(std::string("lion"));
	immStorageG.addToImmStorage("c1BId=2,C1Momc1AId=1", "pets", MafOamSpiMoAttributeType_3_STRING, values);

	// setup the call
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *result = NULL;
	MafReturnT ret = testObj.getImmMoAttribute(txh, "ManagedElement=1,whynot_C1A=1,whynot_C1B=2", "pets", &result);

	// verify the result
	ASSERT_TRUE(ret==MafOk);
	ASSERT_TRUE(result != NULL);
	ASSERT_TRUE(result->type==9);
	ASSERT_TRUE(result->nrOfValues==2);
	ASSERT_TRUE(std::string(result->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result->values[1].value.theString) == std::string("lion"));
}


TEST(ImmTest, Populate_V3_TC6)
{
	testDomainName = "";
	testExtensionName = "";
	testExtensionValue = "";
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key2);
	struct MafOamSpiMoc* meMoc = maf_makeMoc("C1A", key);
	struct MafOamSpiMoc* homeMoc = maf_makeMoc("C1B",attrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, homeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("C1Mom",meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);


	// setup the call
	MOMRootRepository testObj;
	testObj.Populate();

	std::map <std::string, MOMRootDataEntry>::iterator theRetVal;
	theRetVal = testObj.myMOMRootMap.find("C1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElement");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

}

TEST(ImmTest, Populate_V3_TC7)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	//struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"c1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1key2);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"c2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);
	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc);
	struct MafOamSpiContainment* contribution = maf_makeContribution("C1B_to_C2A", c1BMoc, c2AMoc, NULL, NULL);
	//struct MafOamSpiContainment* contribution = maf_makeContribution("C1B_to_C2A", c1BMoc, c2AMoc);

	// setup the call
	MOMRootRepository testObj;
	testObj.Populate();

	std::map <std::string, MOMRootDataEntry>::iterator theRetVal;
	theRetVal = testObj.myMOMRootMap.find("C1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C1BC2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElement");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

}



TEST(ImmTest, Populate_V3_TC8)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"C1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1attrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"C2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc, c2AMoc);

	//struct MafOamSpiContainment* contribution = maf_makeContribution("C2A_to_C1B", c2AMoc,c1BMoc);
	struct MafOamSpiContainment* contribution = maf_makeContribution("C2A_to_C1B", c2AMoc,c1BMoc, cont2, cont);

	// setup the call
	MOMRootRepository testObj;
	testObj.Populate();

	std::map <std::string, MOMRootDataEntry>::iterator theRetVal;
	theRetVal = testObj.myMOMRootMap.find("C1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C1B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2AC1B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElement");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

}

TEST(ImmTest, Populate_V3_TC9)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"C1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1attrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"C2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc, c2AMoc);

	//struct MafOamSpiContainment* contribution = maf_makeContribution("C2A_to_C1B", c2AMoc,c1BMoc);
	struct MafOamSpiContainment* contribution = maf_makeContribution("C2B_to_C1B", c2BMoc,c1BMoc, NULL, cont);



	// setup the call
	MOMRootRepository testObj;
	testObj.Populate();

	std::map <std::string, MOMRootDataEntry>::iterator theRetVal;
	theRetVal = testObj.myMOMRootMap.find("C1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C1B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2BC1B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElement");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

}

TEST(ImmTest, Populate_V3_TC10)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"C1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1attrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"C2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);

	//3rd MOM
	struct MafOamSpiMoAttribute* c3key = maf_makeAttribute((char*)"c3AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c3key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c3key2 = maf_makeAttribute((char*)"c3BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c3key2->isKey = true;

	struct MafOamSpiMoAttribute* c3attrs = maf_makeAttribute((char*)"C3_pets", MafOamSpiMoAttributeType_3_STRING, c3key2);
	struct MafOamSpiMoc* c3AMoc = maf_makeMoc("C3A", c3key);
	struct MafOamSpiMoc* c3BMoc = maf_makeMoc("C3B",c3attrs);
	//struct MafOamSpiContainment* cont3 = maf_makeContainment(c3AMoc, c3BMoc, "C3A");
	struct MafOamSpiMom* C3mom = maf_makeMom("C3Mom",c3AMoc);
	((struct MafOamSpiMoc*)c3BMoc)->mom = C3mom;

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc, c2AMoc);

	// Make relationship
	maf_makeContribution("C2A_to_C1B", c2AMoc,c1BMoc, cont2, cont);
	maf_makeContribution("C1B_to_C3A", c1BMoc, c3AMoc, NULL, NULL);
	maf_makeContribution("C2B_to_C3B", c2BMoc,c3BMoc, NULL, NULL);


	// setup the call
	MOMRootRepository testObj;
	testObj.Populate();

	std::map <std::string, MOMRootDataEntry>::iterator theRetVal;
	theRetVal = testObj.myMOMRootMap.find("C1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C1B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C1BC3A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2BC3B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C3A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C3B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElement");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2AC1B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

}
TEST(ImmTest, IsMocChildofEcimContribution_V4_TC11)
{
	testDomainName = "";
	testExtensionName = "";
	testExtensionValue = "";
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key2);
	struct MafOamSpiMoc* C1A_Moc = maf_makeMoc("C1A", key);
	struct MafOamSpiMoc* C1B_Moc = maf_makeMoc("C1B",attrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(C1A_Moc, C1B_Moc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,C1A_Moc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(C1A_Moc);

	// setup the call
	MOMRootRepository testObj;
	MafOamSpiMrMocHandle_4T mocHandle;
	mocHandle.handle = (uint64_t)(C1B_Moc);
	bool ret = testObj.IsMocChildofEcimContribution_V4(mocHandle);

	// verify the result
	ASSERT_TRUE(ret == false);

}
TEST(ImmTest, IsMocChildofEcimContribution_V4_TC12)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository

	//create C1Mom
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key2);

	struct MafOamSpiMoc* C1A_Moc = maf_makeMoc("C1A", key);
	struct MafOamSpiMoc* C1B_Moc = maf_makeMoc("C1B",attrs);
	struct MafOamSpiContainment* cont1 = maf_makeContainment(C1A_Moc, C1B_Moc, "C1A_to_C1B");
	struct MafOamSpiMom* mom1 = maf_makeMom("C1Mom", C1A_Moc);
	maf_setMom(mom1);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(C1A_Moc);

	//create C2Mom
	// Add the key attribute
	struct MafOamSpiMoAttribute* key3 = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key4 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs2 = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key4);
	struct MafOamSpiMoc* C2A_Moc = maf_makeMoc("C2A", key);
	struct MafOamSpiMoc* C2B_Moc = maf_makeMoc("C2B",attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(C2A_Moc, C2B_Moc, "C2A_to_C2B");
	struct MafOamSpiMom* mom2 = maf_makeMom("C2Mom", C2A_Moc);
	// Set the Contribution between C1B and C2A
	struct MafOamSpiContainment* contribution = maf_makeContribution("C1B_to_C2A", C1B_Moc, C2A_Moc, NULL, NULL);
	// setup the call
	MOMRootRepository testObj;
	MafOamSpiMrMocHandle_4T mocHandle;
	mocHandle.handle = (uint64_t)(C2A_Moc);
	bool ret = testObj.IsMocChildofEcimContribution_V4(mocHandle);

	// verify the result
	ASSERT_TRUE(ret == true);

}

TEST(ImmTest, IsMocChildofEcimContribution_V4_TC13)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository

	//create C1Mom
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key2);

	struct MafOamSpiMoc* C1A_Moc = maf_makeMoc("C1A", key);
	struct MafOamSpiMoc* C1B_Moc = maf_makeMoc("C1B",attrs);
	struct MafOamSpiContainment* cont1 = maf_makeContainment(C1A_Moc, C1B_Moc, "C1A_to_C1B");
	struct MafOamSpiMom* mom1 = maf_makeMom("C1Mom",C1A_Moc);

	maf_setMom(mom1);

	//create C2Mom
	// Add the key attribute
	struct MafOamSpiMoAttribute* key3 = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key4 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs2 = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key4);
	struct MafOamSpiMoc* C2A_Moc = maf_makeMoc("C2A", key);
	struct MafOamSpiMoc* C2B_Moc = maf_makeMoc("C2B",attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(C2A_Moc, C2B_Moc, "C2A_to_C2B");
	struct MafOamSpiMom* mom2 = maf_makeMom("C2Mom",C2A_Moc);

	maf_setMom(mom2);

	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(C1A_Moc, C2A_Moc);

	// Set the Contribution between C2A -> C1B
	struct MafOamSpiContainment* contribution = maf_makeContribution("C2A_to_C1B", C2A_Moc, C1B_Moc, cont2, cont1);

	// setup the call
	MOMRootRepository testObj;
	MafOamSpiMrMocHandle_4T mocHandle;
	mocHandle.handle = (uint64_t)(C1B_Moc);
	bool ret = testObj.IsMocChildofEcimContribution_V4(mocHandle);

	// verify the result
	ASSERT_TRUE(ret == true);

}

TEST(ImmTest, IsMocChildofEcimContribution_V4_TC14)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository

	//create C1Mom
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key2);

	struct MafOamSpiMoc* C1A_Moc = maf_makeMoc("C1A", key);
	struct MafOamSpiMoc* C1B_Moc = maf_makeMoc("C1B",attrs);
	struct MafOamSpiContainment* cont1 = maf_makeContainment(C1A_Moc, C1B_Moc, "C1A_to_C1B");
	struct MafOamSpiMom* mom1 = maf_makeMom("C1Mom",C1A_Moc);

	maf_setMom(mom1);

	//create C2Mom
	// Add the key attribute
	struct MafOamSpiMoAttribute* key3 = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key4 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoAttribute* attrs2 = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key4);
	struct MafOamSpiMoc* C2A_Moc = maf_makeMoc("C2A", key);
	struct MafOamSpiMoc* C2B_Moc = maf_makeMoc("C2B",attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(C2A_Moc, C2B_Moc, "C2A_to_C2B");
	struct MafOamSpiMom* mom2 = maf_makeMom("C2Mom",C2A_Moc);

	maf_setMom(mom2);

	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(C1A_Moc, C2A_Moc);

	// Set the Contribution between C2B -> C1B
	struct MafOamSpiContainment* contribution = maf_makeContribution("C2B_to_C1B", C2B_Moc, C1B_Moc, cont2, cont1);

	// setup the call
	MOMRootRepository testObj;
	MafOamSpiMrMocHandle_4T mocHandle;
	mocHandle.handle = (uint64_t)(C2B_Moc);
	bool ret = testObj.IsMocChildofEcimContribution_V4(mocHandle);

	// verify the result
	ASSERT_TRUE(ret == false);

}


TEST(ImmTest, Populate_V3_SplitContainTrue)
{
	tc_number = 3;
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_TRUE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"C1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1attrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"C2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc, c2AMoc);

	struct MafOamSpiContainment* contribution = maf_makeContribution("C2A_to_C1B", c2AMoc,c1BMoc, cont2, cont);
	((struct MafOamSpiMom*)C2mom)->next = C1mom;

	MOMRootRepository testObj;
	testObj.Populate();

}

TEST(ImmTest, checkContainmentValid_V3_TC5)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_TRUE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key2 = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key2->isKey = true;

	struct MafOamSpiMoAttribute* c1attrs = maf_makeAttribute((char*)"C1_pets", MafOamSpiMoAttributeType_3_STRING, c1key2);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1key);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B",c1attrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(c1AMoc, c1BMoc, "C1A_to_C1B");
	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	//2nd MOM
	struct MafOamSpiMoAttribute* c2key = maf_makeAttribute((char*)"c2AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* c2key2 = maf_makeAttribute((char*)"c2BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c2key2->isKey = true;

	struct MafOamSpiMoAttribute* c2attrs = maf_makeAttribute((char*)"C2_pets", MafOamSpiMoAttributeType_3_STRING, c2key2);
	struct MafOamSpiMoc* c2AMoc = maf_makeMoc("C2A", c2key);
	struct MafOamSpiMoc* c2BMoc = maf_makeMoc("C2B",c2attrs);
	struct MafOamSpiContainment* cont2 = maf_makeContainment(c2AMoc, c2BMoc, "C2A_to_C2B");
	struct MafOamSpiMom* C2mom = maf_makeMom("C2Mom",c2AMoc);

	//3rd MOM
	struct MafOamSpiMoAttribute* c3key = maf_makeAttribute((char*)"c3AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c3key->isKey = true;
	struct MafOamSpiMoAttribute* c3a_attrs = maf_makeAttribute((char*)"C3A_pets", MafOamSpiMoAttributeType_3_STRING, c3key);
	// Add the key attribute
	struct MafOamSpiMoAttribute* c3key2 = maf_makeAttribute((char*)"c3BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c3key2->isKey = true;

	struct MafOamSpiMoAttribute* c3b_attrs = maf_makeAttribute((char*)"C3B_pets", MafOamSpiMoAttributeType_3_STRING, c3key2);
	struct MafOamSpiMoc* c3AMoc = maf_makeMoc("C3A", c3a_attrs);
	struct MafOamSpiMoc* c3BMoc = maf_makeMoc("C3B", c3b_attrs);
	struct MafOamSpiMom* C3mom = maf_makeMom("C3Mom",c3AMoc);
	((struct MafOamSpiMoc*)c3BMoc)->mom = C3mom;

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc, c2AMoc);

	// Make relationship
	maf_makeContribution("C2A_to_C1B", c2AMoc,c1BMoc, cont2, cont);
	struct MafOamSpiContainment* cont3 = maf_makeContribution("C1B_to_C3A", c1BMoc, c3AMoc, NULL, NULL);
	maf_makeContribution("C2B_to_C3B", c2BMoc,c3BMoc, NULL, NULL);

	MOMRootRepository testObj;
	//testObj.checkContainmentValid_V3("C1B_to_C3A", (MafOamSpiMrContainmentHandle_3T&)cont3);
	testObj.Populate();

	std::map <std::string, MOMRootDataEntry>::iterator theRetVal;
	theRetVal = testObj.myMOMRootMap.find("C1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C1B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C1BC3A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2BC3B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C3A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C3B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElement");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("ManagedElementC2A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

	theRetVal = testObj.myMOMRootMap.find("C2AC1B");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());

}

TEST(ImmTest, selfNestedContainment_withMultipleChildren)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_TRUE;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	// Add the key attribute

	struct MafOamSpiMoAttribute* c1Akey = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1Akey->isKey = true;
	struct MafOamSpiMoAttribute* c1Bkey = maf_makeAttribute((char*)"c1BId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1Bkey->isKey = true;
	struct MafOamSpiMoAttribute* c1Ckey = maf_makeAttribute((char*)"c1CId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1Ckey->isKey = true;

	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1Akey);
	struct MafOamSpiMoc* c1BMoc = maf_makeMoc("C1B", c1Bkey);
	struct MafOamSpiMoc* c1CMoc = maf_makeMoc("C1C", c1Ckey);

	struct MafOamSpiContainment* cont0 = maf_makeContribution("C1A_to_C1A", c1AMoc, c1AMoc, NULL, NULL);
	struct MafOamSpiContainment* cont1 = maf_makeContribution("C1A_to_C1B", c1AMoc, c1BMoc, cont0, NULL);
	struct MafOamSpiContainment* cont2 = maf_makeContribution("C1A_to_C1C", c1AMoc, c1CMoc, cont1, NULL);

	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);
	c1BMoc->mom = C1mom;
	c1CMoc->mom = C1mom;
	maf_setMom(C1mom);

	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc);

	MOMRootRepository testObj;
	testObj.Populate();

	std::map <std::string, MOMRootDataEntry>::iterator theRetVal;
	theRetVal = testObj.myMOMRootMap.find("C1A");
	ASSERT_TRUE(theRetVal != testObj.myMOMRootMap.end());
}

TEST(ImmTest, getEcimPasswordAttrInfo)
{
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();

	// setup ModelRepository
	// Add the key attribute
	struct MafOamSpiStructMember* myClearText = maf_makeStructMember((char*)"cleartext", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStruct* secretData = maf_makeStruct((char*)"EcimPassword", myClearText);

	// Add the key attribute
	struct MafOamSpiMoAttribute* key1 = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key1->isKey = true;
	struct MafOamSpiMoAttribute* c1attr1 = maf_makeAttribute((char*)"userLabel",MafOamSpiMoAttributeType_3_STRING,key1);
	struct MafOamSpiMoAttribute* c1attr2 = maf_makeAttribute((char*)"secretInfo",secretData, c1attr1);
	c1attr2->isKey=false;

	struct MafOamSpiMoc* myMoc = maf_makeMoc("C1A",c1attr2);
	struct MafOamSpiMom* mom = maf_makeMom("C1MOM",myMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(myMoc);

	MafOamSpiMrAttributeHandle_4T attrHandle;

	attrHandle.handle = (uint64_t)(c1attr2); //attr Handle other then passPhrase attribute
	MOMRootRepository testObj;
	MafOamSpiMrTypeContainerHandle_4T attrTypeContHandle;
	theModelRepo_v4_p->attribute->getTypeContainer(attrHandle, &attrTypeContHandle);
	std::string ecimPasswdAttr = "";

	EXPECT_EQ(testObj.getEcimPasswordAttrInfo(attrTypeContHandle,ecimPasswdAttr), MafOk);
	ASSERT_TRUE(ecimPasswdAttr=="password");
}

TEST(ImmTest, getPassphraseAttrInfo)
{
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository

	//1st MOM
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;

	struct MafOamSpiMoAttribute* c1attr1 = maf_makeAttribute((char*)"userLabel", MafOamSpiMoAttributeType_3_STRING, c1key);
	struct MafOamSpiMoAttribute* c1attr2 = maf_makeAttribute((char*)"passPhraseAttr1", MafOamSpiMoAttributeType_3_STRING, c1attr1);
	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1attr2);

	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc);

	// setup the call
	MOMRootRepository testObj;
	MafOamSpiMrAttributeHandle_4T attrHandle;

	attrHandle.handle = (uint64_t)(c1attr1); //attr Handle other then passPhrase attribute
	MafOamSpiMrTypeContainerHandle_4T attrTypeContHandle;

	theModelRepo_v4_p->attribute->getTypeContainer(attrHandle, &attrTypeContHandle);
	std::string passPhraseAttr="";
	MafReturnT ret = testObj.getPassphraseAttrInfo(attrHandle,attrTypeContHandle,passPhraseAttr);
	EXPECT_EQ(MafOk, ret);
	ASSERT_TRUE(passPhraseAttr.empty());

	attrHandle.handle = (uint64_t)(c1attr2); //passPhraseAttr Handle
	theModelRepo_v4_p->attribute->getTypeContainer(attrHandle, &attrTypeContHandle);
	passPhraseAttr="";
	ret = testObj.getPassphraseAttrInfo(attrHandle,attrTypeContHandle,passPhraseAttr);
	EXPECT_EQ(MafOk, ret);
	ASSERT_TRUE(passPhraseAttr == "passPhraseAttr1");
}

TEST(ImmTest, getSecretAttributesFromModelRepo)
{
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository

	//1st MOM

	struct MafOamSpiStructMember* myClearText = maf_makeStructMember((char*)"cleartext", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStruct* secretData = maf_makeStruct((char*)"EcimPassword", myClearText);

	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;

	struct MafOamSpiMoAttribute* c1attr1 = maf_makeAttribute((char*)"passPhraseAttr1", MafOamSpiMoAttributeType_3_STRING, c1key);

	struct MafOamSpiMoAttribute* c1attr2 = maf_makeAttribute((char*)"secretInfo",secretData, c1attr1);
	c1attr2->isKey=false;
	struct MafOamSpiMoAttribute* c1attr3 = maf_makeAttribute((char*)"passPhraseAttr2", MafOamSpiMoAttributeType_3_STRING, c1attr2);

	struct MafOamSpiMoAttribute* c1attr4 = maf_makeAttribute((char*)"userLabel", MafOamSpiMoAttributeType_3_STRING, c1attr3);

	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1attr4);

	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc);

	// setup the call
	MOMRootRepository testObj;
	MafOamSpiMrMocHandle_4T mocHandle;
	mocHandle.handle = (uint64_t)c1AMoc;

	testObj.mapMocHandler.insert(std::pair< std::string, MafOamSpiMrMocHandle_4T>( "managedElement,c1AMoc", mocHandle));
	testObj.getSecretAttributesFromModelRepo();

	EXPECT_EQ(2, testObj.mapMocpathToAttributes.size());

	std::map<std::string, std::string>::iterator itMap = testObj.mapMocpathToAttributes.begin();
	ASSERT_TRUE("managedElement,c1AMoc"== std::string(itMap->first.c_str()));
	ASSERT_TRUE("passPhraseAttr2,passPhraseAttr1"== std::string(itMap->second.c_str()));
	itMap++;
	ASSERT_TRUE("managedElement,c1AMoc:EcimPassword"== std::string(itMap->first.c_str()));
	ASSERT_TRUE("password"== std::string(itMap->second.c_str()));
}

TEST(ImmTest, ReadStructFromImm)
{
	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//   Me<<EcimClass>,
	//     Employee<<EcimClass>>
	//           title:string
	//           person:PersonData
	//
	//  PersonData<<EcimStruct>>{
	//                firstname:string;
	//                lastname:string;
	//          }
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	struct MafOamSpiStructMember* mLastname = maf_makeStructMember((char*)"lastname", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStructMember* mFirstname = maf_makeStructMember((char*)"firstname", MafOamSpiDatatype_STRING, mLastname);
	struct MafOamSpiStruct* personData = maf_makeStruct((char*)"PersonData", mFirstname);
		// Add the key attribute for class Employee
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoAttribute* attrPerson = maf_makeAttribute((char*)"person",personData, key2);
	attrPerson->isKey=false;
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,attrPerson);
	employeeAttrs->isKey=false;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;

	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", key);

	struct MafOamSpiMoc* employeeMoc = maf_makeMoc("Employee",employeeAttrs);

	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("MEMOM",meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);
	// To be able to add a class to the MOMKeyRepo that is not a key class, the MOM must be linked to
	// The homeMoc, do this manually here.
	employeeMoc->mom = mom;
	// OK, need to trigger the read of the model repository, do this by a fake question
	theTranslator.IsClassNamePresent("READ_MODEL_REPO");

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=1,meId=1","title",9,values);
	}
	{
		// add Employee instance 1 attribute: person (a reference to the struct instance!)
		std::vector<std::string> values;
		values.push_back(std::string("id=person_1,employeeId=1,meId=1"));
		unsigned int IMM_SA_NAME=6;
		immStorageG.addToImmStorage("employeeId=1,meId=1","person",IMM_SA_NAME,values);
	}
	{
		// add personData instance person_1 attribute: firstname
		std::vector<std::string> values;
		values.push_back(std::string("Bob"));
		immStorageG.addToImmStorage("id=person_1,employeeId=1,meId=1","firstname",9,values);
	}
	{
		// add personData instance person_1 attribute: lastname
		std::vector<std::string> values;
		values.push_back(std::string("Dole"));
		immStorageG.addToImmStorage("id=person_1,employeeId=1,meId=1","lastname",9,values);
	}

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("person"), IMM_REF_TYPE);
	}
	{
		// add the class name attribute for PersonData with two attributes "firstname" and "lastname"
		std::vector<std::string> className;
		className.push_back("PersonData");
		unsigned int IMM_STRING_TYPE=6;
		immStorageG.addToImmStorage("id=person_1,employeeId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("firstname"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("lastname"), IMM_STRING_TYPE);
	}
	// add the class name attribute for Me
	{
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}

	// read the Ecim struct value from "Me=1,Employee=1,person"

	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *result = NULL;
	MafReturnT ret = testObj.getImmMoAttribute(txh,"ManagedElement=1,Me=1,Employee=1","person",&result);

	// verify the result
	ASSERT_EQ(MafOk, ret);
	ASSERT_TRUE(result != NULL);
	ASSERT_EQ(14, result->type); // struct
	ASSERT_EQ(1, result->nrOfValues);

	MafMoAttributeValueStructMember_3* member;
	// First struct member
	member = result->values[0].value.structMember;
	ASSERT_TRUE(std::string(member->memberName) == std::string("firstname"));
	ASSERT_EQ(9, member->memberValue->type); // string
	ASSERT_EQ(1, member->memberValue->nrOfValues);
	ASSERT_TRUE(std::string(member->memberValue->values[0].value.theString) == std::string("Bob"));
	ASSERT_TRUE(member->next!=NULL);

	// Second struct member
	member = member->next;
	ASSERT_TRUE(std::string(member->memberName) == std::string("lastname"));
	ASSERT_EQ(9, member->memberValue->type); // string
	ASSERT_EQ(1, member->memberValue->nrOfValues);
	ASSERT_TRUE(std::string(member->memberValue->values[0].value.theString) == std::string("Dole"));
	ASSERT_TRUE(member->next==NULL); // no more members


	// setup the call
	MafOamSpiTransactionHandleT txh2=0;
	//TxContext* txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);
	OamSAImmBridge testObj2;
	MafMoAttributeValueContainer_3T *result2 = NULL;
	MafReturnT ret2 = testObj2.getImmMoAttribute(txh2,"ManagedElement=1,Me=1,Employee=1","person",&result2);
	ASSERT_TRUE(ret2==MafNotExist);

}

TEST(ImmTest, ReadStructArrayFromImm)
{
	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//   Me<<EcimClass>,
	//     Employee<<EcimClass>>
	//           title:string
	//           person[2]:PersonData
	//
	//  PersonData<<EcimStruct>>{
	//                firstname:string;
	//                lastname:string;
	//          }
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiStructMember* mLastname = maf_makeStructMember((char*)"lastname", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStructMember* mFirstname = maf_makeStructMember((char*)"firstname", MafOamSpiDatatype_STRING, mLastname);
	struct MafOamSpiStruct* personData = maf_makeStruct((char*)"PersonData", mFirstname);
	// Add the key attribute for class Employee
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoAttribute* attrPerson = maf_makeAttribute((char*)"person",personData, key2);
	attrPerson->isKey=false;
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,attrPerson);
	employeeAttrs->isKey=false;
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", key);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc("Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("MEMOM",meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);
	// To be able to add a class to the MOMKeyRepo that is not a key class, the MOM must be linked to
	// The homeMoc, do this manually here.
	employeeMoc->mom = mom;
	// OK, need to trigger the read of the model repository, do this by a fake question
	theTranslator.IsClassNamePresent("READ_MODEL_REPO");

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=1,meId=1","title",MafOamSpiMoAttributeType_3_STRING,values);
	}

	// person[0]
	{
		// add personData instance person_0 attribute: firstname
		std::vector<std::string> values;
		values.push_back(std::string("Bob"));
		immStorageG.addToImmStorage("id=person_0,employeeId=1,meId=1","firstname",MafOamSpiMoAttributeType_3_STRING,values);
	}
	{
		// add personData instance person_0 attribute: lastname
		std::vector<std::string> values;
		values.push_back(std::string("Dole"));
		immStorageG.addToImmStorage("id=person_0,employeeId=1,meId=1","lastname",MafOamSpiMoAttributeType_3_STRING,values);
	}

	// person[1]
	{
		// add personData instance person_1 attribute: firstname
		std::vector<std::string> values;
		values.push_back(std::string("Jimmy"));
		immStorageG.addToImmStorage("id=person_1,employeeId=1,meId=1","firstname",MafOamSpiMoAttributeType_3_STRING,values);
	}
	{
		// add personData instance person_1 attribute: lastname
		std::vector<std::string> values;
		values.push_back(std::string("Carter"));
		immStorageG.addToImmStorage("id=person_1,employeeId=1,meId=1","lastname",MafOamSpiMoAttributeType_3_STRING,values);
	}
	// add the person reference array to employee
	{
		// add Employee instance 1 attribute: person (a reference to the struct instance!)
		std::vector<std::string> values;
		values.push_back(std::string("id=person_0,employeeId=1,meId=1"));
		values.push_back(std::string("id=person_1,employeeId=1,meId=1"));
		unsigned int IMM_SA_NAME=6;
		immStorageG.addToImmStorage("employeeId=1,meId=1","person",IMM_SA_NAME,values);
	}

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=MafOamSpiMoAttributeType_3_STRING;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("person"), IMM_REF_TYPE);
	}
	{
		// add the class name attribute for PersonData with two attributes "firstname" and "lastname"
		std::vector<std::string> className;
		className.push_back("PersonData");
		unsigned int IMM_STRING_TYPE=MafOamSpiMoAttributeType_3_STRING;
		immStorageG.addToImmStorage("id=person_0,employeeId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		immStorageG.addToImmStorage("id=person_1,employeeId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("firstname"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("lastname"), IMM_STRING_TYPE);
	}
	// add the class name attribute for Me
	{
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=MafOamSpiMoAttributeType_3_STRING;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}

	// read the Ecim struct value from "Me=1,Employee=1,person"

	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *result = NULL;
	MafReturnT ret = testObj.getImmMoAttribute(txh,"ManagedElement=1,Me=1,Employee=1","person",&result);

	// verify the result
	ASSERT_EQ(MafOk, ret);
	ASSERT_TRUE(result != NULL);
	ASSERT_EQ(MafOamSpiMoAttributeType_3_STRUCT, result->type); // struct
	ASSERT_EQ(2, result->nrOfValues);

	MafMoAttributeValueStructMember_3* member = NULL;
	// person[0] First struct member
	member = result->values[0].value.structMember;
	ASSERT_TRUE(std::string(member->memberName) == std::string("firstname"));
	ASSERT_EQ(MafOamSpiMoAttributeType_3_STRING, member->memberValue->type); // string
	ASSERT_EQ(1, member->memberValue->nrOfValues);
	ASSERT_TRUE(std::string(member->memberValue->values[0].value.theString) == std::string("Bob"));
	ASSERT_TRUE(member->next!=NULL);

	// person[0] Second struct member
	member = member->next;
	ASSERT_TRUE(std::string(member->memberName) == std::string("lastname"));
	ASSERT_EQ(MafOamSpiMoAttributeType_3_STRING, member->memberValue->type); //string
	ASSERT_EQ(1, member->memberValue->nrOfValues);
	ASSERT_TRUE(std::string(member->memberValue->values[0].value.theString) == std::string("Dole"));
	ASSERT_TRUE(member->next==NULL); // no more members


	// person[1] First struct member
	member = result->values[1].value.structMember;
	ASSERT_TRUE(std::string(member->memberName) == std::string("firstname"));
	ASSERT_EQ(MafOamSpiMoAttributeType_3_STRING, member->memberValue->type); //string
	ASSERT_EQ(1, member->memberValue->nrOfValues);
	ASSERT_TRUE(std::string(member->memberValue->values[0].value.theString) == std::string("Jimmy"));
	ASSERT_TRUE(member->next!=NULL);

	// person[1] Second struct member
	member = member->next;
	ASSERT_TRUE(std::string(member->memberName) == std::string("lastname"));
	ASSERT_EQ(MafOamSpiMoAttributeType_3_STRING, member->memberValue->type); // string
	ASSERT_EQ(1, member->memberValue->nrOfValues);
	ASSERT_TRUE(std::string(member->memberValue->values[0].value.theString) == std::string("Carter"));
	ASSERT_TRUE(member->next==NULL); // no more members

}

TEST(ImmTest, ImmIterator)
{
	// First we setup the test so that we can perform a write!

	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//   Me<<EcimClass>,
	//     Employee<<EcimClass>>
	//           title:string
	//           person:PersonData
	//
	//  PersonData<<EcimStruct>>{
	//                firstname:string;
	//                lastname:string;
	//          }

	// COM will read the model from the repository and then it will look for instances for each contained class
	// This means that COM will always iterate over a known class, and since struct classes in IMM are unknown to COM
	// COM will never iterate over them, they are infact invisible to COM without us doing anything!
	//
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiStructMember* mLastname = maf_makeStructMember((char*)"lastname", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStructMember* mFirstname = maf_makeStructMember((char*)"firstname", MafOamSpiDatatype_STRING, mLastname);
	struct MafOamSpiStruct* personData = maf_makeStruct((char*)"PersonData", mFirstname);
	// Add the key attribute for class Employee
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoAttribute* attrPerson = maf_makeAttribute((char*)"person",personData, key2);
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,attrPerson);
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoc* meMoc = maf_makeMoc((char*)"Me", key);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	// setup IMM storage
	immStorageG.reset();

	{
		// add the class name attribute for Employee with two attributes "title" and "person"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("person"), IMM_REF_TYPE);
	}
	{
		// add the class name attribute for PersonData with two attributes "firstname" and "lastname"
		std::vector<std::string> className;
		className.push_back("PersonData");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("personDataId=person_0,employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("firstname"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("lastname"), IMM_STRING_TYPE);
	}
	// add the class name attribute for Me
	{
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}

	// iterator across root, eg DN="" and classname "Me" (COM always uses classname!) (Search directly towards IMM, no cache)
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMoAttributeValueContainer_3T *result = NULL;
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		// perform a root search

		// setup the search result
		GLOBAL_immCmdOmSearchNextDnOut = "meId=1";
		// performt the search

		MafReturnT retGet = testObj.getImmMoIterator(txh,"","Me", p_ith);
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafOk);
		ASSERT_TRUE(searchResult != NULL);
		printf("Search Result=%s\n",searchResult);
		ASSERT_TRUE(searchResult == std::string("1")); // result should be a COM path!
		//delete txContextIn;
	}

	// iterator across root, eg DN="Me=1,Employee=1" and classname Test (Search directly towards IMM, no cache)
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		OamSAImmBridge testObj;
		MafMoAttributeValueContainer_3T *result = NULL;
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		// setup the search result
		GLOBAL_immCmdOmSearchNextDnOut = "testId=66";
		// performt the search
		MafReturnT retGet = testObj.getImmMoIterator(txh,"Me=1,Employee=1","Test", p_ith);
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafOk);
		ASSERT_TRUE(searchResult != NULL);
		printf("Search Result=%s\n",searchResult);
		ASSERT_TRUE(searchResult == std::string("66"));
		delete _portal;
	}

	{
		// create an new employee=2 Richard Nixon, Vice President! :-)
		// This will store the value in the cache and any search should look in the cache!!

		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		OamSAImmBridge testObj;

		// -----------------------------
		// prepare array of named containers with the initial attribute values (COM MO SPI Ver.3)
		MafMoNamedAttributeValueContainer_3T* myInitialAttributes[3];

		myInitialAttributes[0] = new MafMoNamedAttributeValueContainer_3T;
		myInitialAttributes[1] = new MafMoNamedAttributeValueContainer_3T;
		myInitialAttributes[2] = NULL; // NULL terminator for the array

		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont1 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont2 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		memset(cont1.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont2.get(), 0, sizeof(MafMoAttributeValueContainer_3T));

		uint32_t myValuesInt[10] = { 41, 42, 43, 44, 45, 46, 47, 48, 49, 50 };

		myInitialAttributes[0]->name = "myContainerInt32s";
		cont1->type = MafOamSpiMoAttributeType_3_UINT32;
		cont1->nrOfValues = 5;
		MafMoAttributeValue_3* cont1_value = new MafMoAttributeValue_3[cont1->nrOfValues];
		cont1->values = cont1_value;

		int i = 0;
		for (i = 0; i < cont1->nrOfValues; i++) {
			cont1->values[i].value.u32 = (uint32_t)myValuesInt[i];
		}
		myInitialAttributes[0]->value = *cont1;
		// this works fine on a 64-bit build server, but on a 32-bit machine every second
		// element is skipped, so "myString1" and "myString3" are used instead of
		// "myString1" and "myString2"
		char* myStrings[5] = {"myString1", "myString2", "myString3", "myString4", NULL};
		myInitialAttributes[1]->name = "myContainerStrings";
		cont2->type = MafOamSpiMoAttributeType_3_STRING;
		cont2->nrOfValues = 2; // 2;
		cont2->values = (MafMoAttributeValue_3T*) &myStrings;
		myInitialAttributes[1]->value = *cont2;
		// -----------------------------

		//Create an instance to prime the cache
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","Employee","2", &myInitialAttributes[0]);
		ASSERT_TRUE(retCre==MafOk);

		// NOW we try to search and expect to get a cache hit
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		GLOBAL_immCmdOmSearchNextDnOut = "errorId=555"; // this value should NOT be returned cause of the cache!

		// perform the search, we search for "Me=1" subinstances and should get "Employee=2" back
		MafReturnT retGet = testObj.getImmMoIterator(txh,"ManagedElement=1,Me=1","Employee", p_ith); // COM only ever iterates over known classes!
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafOk);
		ASSERT_TRUE(searchResult != NULL);
		printf("Search Result=%s\n",searchResult);
		ASSERT_TRUE(searchResult == std::string("2")); //Only the instance value, not the class...COM style!
		delete _portal;
		delete []cont1_value;
		delete myInitialAttributes[0];
		delete myInitialAttributes[1];
	}

	// iterator across root, eg DN="" and classname "Me" (COM always uses classname!) (Search directly towards IMM, no cache)
	// This testcase is to test reinitializing the bad handle
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMoAttributeValueContainer_3T *result = NULL;
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		// perform a root search

		// setup the search result
		GLOBAL_immCmdOmSearchNextDnOut = "meId=1";
		// perform the search

		MafReturnT retGet = testObj.getImmMoIterator(txh,"","Me", p_ith);
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		returnBadHandle = true;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafFailure);
		ASSERT_TRUE(searchResult == NULL);
		ASSERT_TRUE(nRetries_BadHandle == 3);
		returnBadHandle = false;
		nRetries_BadHandle = 0;
	}
}

TEST(ImmTest, Release)
{
	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"homeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets", MafOamSpiMoAttributeType_3_STRING, key);
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoAttribute* attrs_me = maf_makeAttribute((char*)"rooms", MafOamSpiMoAttributeType_3_STRING, key2);
	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", attrs_me);
	struct MafOamSpiMoc* homeMoc = maf_makeMoc("Home",attrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, homeMoc);
	struct MafOamSpiMom* mom = maf_makeMom( "meMom",meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	{
		std::vector<std::string> values;
		values.push_back(std::string("tiger"));
		values.push_back(std::string("lion"));
		immStorageG.addToImmStorage("homeId=2,meId=1", "pets", MafOamSpiMoAttributeType_3_STRING, values);
	}

	{
		std::vector<std::string> values;
		values.push_back(std::string("bedroom"));
		values.push_back(std::string("bathroom"));
		immStorageG.addToImmStorage("meId=1", "rooms", MafOamSpiMoAttributeType_3_STRING, values);
	}

	// setup the first call
	MafOamSpiTransactionHandleT txh_1=1;
	std::tr1::shared_ptr<TxContext> txContextIn_1 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh_1);
	OamSAImmBridge testObj_1;
	MafMoAttributeValueContainer_3T *result_1 = NULL;
	MafReturnT ret = testObj_1.getImmMoAttribute(txh_1, "ManagedElement=1,Me=1,Home=2", "pets", &result_1);

	// verify the result
	EXPECT_EQ(ret, MafOk);
	ASSERT_TRUE(result_1 != NULL);
	EXPECT_EQ(result_1->type, 9);
	EXPECT_EQ(result_1->nrOfValues, 2);
	ASSERT_TRUE(result_1->nrOfValues==2);
	ASSERT_TRUE(std::string(result_1->values[0].value.theString) == std::string("tiger"));
	ASSERT_TRUE(std::string(result_1->values[1].value.theString) == std::string("lion"));

	// setup the second call
	MafOamSpiTransactionHandleT txh_2=2;
	std::tr1::shared_ptr<TxContext> txContextIn_2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh_2);
	OamSAImmBridge testObj_2;
	MafMoAttributeValueContainer_3T *result_2 = NULL;
	ret = testObj_2.getImmMoAttribute(txh_2, "ManagedElement=1,Me=1", "rooms", &result_2);

	// verify the result
	EXPECT_EQ(ret, MafOk);
	ASSERT_TRUE(result_2 != NULL);
	EXPECT_EQ(result_2->type, 9);
	ASSERT_TRUE(result_2->nrOfValues==2);
	ASSERT_TRUE(std::string(result_2->values[0].value.theString) == std::string("bedroom"));
	ASSERT_TRUE(std::string(result_2->values[1].value.theString) == std::string("bathroom"));

	// call release() and check the result
	int valList1_size_before = txContextIn_1->GetCache().GetAttValCList().size();
	int valList2_size_before = txContextIn_2->GetCache().GetAttValCList().size();
	OamSATransactionRepository::getOamSATransactionRepository()->ReleaseAttValContfromCache(result_1);
	int valList1_size_after = txContextIn_1->GetCache().GetAttValCList().size();
	int valList2_size_after = txContextIn_2->GetCache().GetAttValCList().size();
	ASSERT_TRUE(valList1_size_before==(valList1_size_after + 1));
	ASSERT_TRUE(valList2_size_before==valList2_size_after);

	valList1_size_before = txContextIn_1->GetCache().GetAttValCList().size();
	valList2_size_before = txContextIn_2->GetCache().GetAttValCList().size();
	OamSATransactionRepository::getOamSATransactionRepository()->ReleaseAttValContfromCache(result_2);
	valList1_size_after = txContextIn_1->GetCache().GetAttValCList().size();
	valList2_size_after = txContextIn_2->GetCache().GetAttValCList().size();
	ASSERT_TRUE(valList1_size_before==valList1_size_after);
	ASSERT_TRUE(valList2_size_before==(valList2_size_after + 1));

	// call for release in case give attribute value not in list
	valList1_size_before = txContextIn_1->GetCache().GetAttValCList().size();
	OamSATransactionRepository::getOamSATransactionRepository()->ReleaseAttValContfromCache(result_1);
	valList1_size_after = txContextIn_1->GetCache().GetAttValCList().size();
	ASSERT_TRUE(valList1_size_before==valList1_size_after);

	// set up call for release multi attribute value
	// setup the first call
	MafOamSpiTransactionHandleT txh_3=1;
	std::tr1::shared_ptr<TxContext> txContextIn_3 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh_3);
	OamSAImmBridge testObj_3;
	MafMoAttributeValueContainer_3T *result_3 = NULL;
	ret = testObj_3.getImmMoAttribute(txh_3, "ManagedElement=1,Me=1,Home=2", "pets", &result_3);
	// setup the second call
	MafOamSpiTransactionHandleT txh_4=2;
	std::tr1::shared_ptr<TxContext> txContextIn_4 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh_4);
	OamSAImmBridge testObj_4;
	MafMoAttributeValueContainer_3T *result_4 = NULL;
	ret = testObj_4.getImmMoAttribute(txh_4, "ManagedElement=1,Me=1", "rooms", &result_4);

	// call release multi attribute value
	MafMoAttributeValueContainer_3T **result = new MafMoAttributeValueContainer_3T*[3]{result_3, result_4,NULL};

	int valList3_size_before = txContextIn_3->GetCache().GetAttValCList().size();
	int valList4_size_before = txContextIn_4->GetCache().GetAttValCList().size();
	OamSATransactionRepository::getOamSATransactionRepository()->ReleaseMultipleAttValContfromCache(result);
	int valList3_size_after = txContextIn_3->GetCache().GetAttValCList().size();
	int valList4_size_after = txContextIn_4->GetCache().GetAttValCList().size();
	ASSERT_TRUE(valList3_size_before==(valList3_size_after + 1));
	ASSERT_TRUE(valList4_size_before==(valList4_size_after + 1));

}

TEST(ImmTest, FinalizeMoIterator)
{
	// First we setup the test so that we can perform a write!

	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//   Me<<EcimClass>,
	//     Employee<<EcimClass>>
	//           title:string
	//           person:PersonData
	//
	//  PersonData<<EcimStruct>>{
	//                firstname:string;
	//                lastname:string;
	//          }

	// COM will read the model from the repository and then it will look for instances for each contained class
	// This means that COM will always iterate over a known class, and since struct classes in IMM are unknown to COM
	// COM will never iterate over them, they are infact invisible to COM without us doing anything!
	//
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiStructMember* mLastname = maf_makeStructMember((char*)"lastname", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStructMember* mFirstname = maf_makeStructMember((char*)"firstname", MafOamSpiDatatype_STRING, mLastname);
	struct MafOamSpiStruct* personData = maf_makeStruct((char*)"PersonData", mFirstname);
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoAttribute* attrPerson = maf_makeAttribute((char*)"person",personData, key);
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,attrPerson);
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoc* meMoc = maf_makeMoc((char*)"Me", NULL);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	// setup IMM storage
	immStorageG.reset();

	{
			// add the class name attribute for Employee with two attributes "title" and "person"
			std::vector<std::string> className;
			className.push_back("Employee");
			unsigned int IMM_STRING_TYPE=9;
			unsigned int IMM_REF_TYPE = 6;
			immStorageG.addToImmStorage("employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
			// add attribute defintions to class
			immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
			immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("person"), IMM_REF_TYPE);
	}
	{
			// add the class name attribute for PersonData with two attributes "firstname" and "lastname"
			std::vector<std::string> className;
			className.push_back("PersonData");
			unsigned int IMM_STRING_TYPE=9;
			immStorageG.addToImmStorage("personDataId=person_0,employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
			// add attribute defintions to class
			immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("firstname"), IMM_STRING_TYPE);
			immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("lastname"), IMM_STRING_TYPE);
	}
	// add the class name attribute for Me
	{
			std::vector<std::string> className;
			className.push_back("Me");
			unsigned int IMM_STRING_TYPE=9;
			immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}

	{
		MafOamSpiTransactionHandleT txh_1 = 1;
		std::tr1::shared_ptr<TxContext> txContextIn_1 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh_1);

		MafOamSpiTransactionHandleT txh_2 = 2;
		std::tr1::shared_ptr<TxContext> txContextIn_2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh_2);

		{
			OamSAImmBridge testObj;

			// iterator across root, eg DN="" and classname "Me" (COM always uses classname!) (Search directly towards IMM, no cache) on txh_1
			MafOamSpiMoIteratorHandle_3T ith_1;
			MafOamSpiMoIteratorHandle_3T* p_ith = &ith_1;

			// setup the search result
			GLOBAL_immCmdOmSearchNextDnOut = "meId=1";
			// performt the search
			MafReturnT retGet = testObj.getImmMoIterator(txh_1,"ManagedElement=1","Me", p_ith);
			ASSERT_TRUE(retGet==MafOk);

			char* searchResult = NULL;
			MafReturnT retNext = testObj.getImmNextMo (ith_1, &searchResult);
			ASSERT_TRUE(retNext==MafOk);
			ASSERT_TRUE(searchResult != NULL);
			printf("Search Result=%s\n",searchResult);
			ASSERT_TRUE(searchResult == std::string("1")); // result should be a COM path!

			for (int i = 0; i < 10; i++)
			{
				searchResult = NULL;
				retNext = testObj.getImmNextMo (ith_1, &searchResult);
				ASSERT_TRUE(retNext==MafOk);
				ASSERT_TRUE(searchResult != NULL);
				printf("another Search Result=%s\n",searchResult);
			}

			// iterator across root, eg DN="Me=1,Employee=1" and classname Test (Search directly towards IMM, no cache) on txh_1
			MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
			_portal->getInterface = &get_interface_mock;
			MafOamSpiMoIteratorHandle_3T ith_2;
			p_ith = &ith_2;
			// setup the search result
			GLOBAL_immCmdOmSearchNextDnOut = "";
			// performt the search
			retGet = testObj.getImmMoIterator(txh_1,"ManagedElement=1,Me=1,Employee=1","Test", p_ith);
			ASSERT_TRUE(retGet==MafOk);
			searchResult = NULL;
			retNext = testObj.getImmNextMo (ith_2, &searchResult);
			ASSERT_TRUE(retNext==MafOk);
			ASSERT_TRUE(searchResult == NULL);
			printf("Search Result=%s\n",searchResult);

			// create an new employee=2 Richard Nixon, Vice President! :-) on txh_1
			// This will store the value in the cache and any search should look in the cache!!
			_portal->getInterface = &get_interface_mock;
			// prepare container with the initial attribute values (COM MO SPI Ver.3)
			MafMoNamedAttributeValueContainer_3T myInitialAttributes;
			MafMoNamedAttributeValueContainer_3T* p_myInitialAttributes = NULL;

			//Create an instance to prime the cache
			MafReturnT retCre = testObj.createImmMo(txh_1,"ManagedElement=1,Me=1","Employee","Employee","2", &p_myInitialAttributes);
			ASSERT_TRUE(retCre==MafOk);
			// NOW we try to search and expect to get a cache hit
			MafOamSpiMoIteratorHandle_3T ith_3;
			p_ith = &ith_3;
			// setup the search result
			GLOBAL_immCmdOmSearchNextDnOut = "errorId=555"; // this value should NOT be returned cause of the cache!
			// perform the search, we search for "Me=1" subinstances and should get "Employee=2" back
			retGet = testObj.getImmMoIterator(txh_1,"ManagedElement=1,Me=1","Employee", p_ith); // COM only ever iterates over known classes!
			ASSERT_TRUE(retGet==MafOk);
			searchResult = NULL;
			retNext = testObj.getImmNextMo (ith_3, &searchResult);
			ASSERT_TRUE(retNext==MafOk);
			ASSERT_TRUE(searchResult != NULL);
			printf("Search Result=%s\n",searchResult);
			ASSERT_TRUE(searchResult == std::string("2")); //Only the instance value, not the class...COM style!

			// iterator across root, eg DN="" and classname "Me" on txh_2
			MafOamSpiMoIteratorHandle_3T ith_4;
			p_ith = &ith_4;
			// setup the search result
			GLOBAL_immCmdOmSearchNextDnOut = "meId=1";
			// performt the search
			retGet = testObj.getImmMoIterator(txh_2,"","Me", p_ith);
			ASSERT_TRUE(retGet==MafOk);

			searchResult = NULL;
			retNext = testObj.getImmNextMo (ith_4, &searchResult);
			ASSERT_TRUE(retNext==MafOk);
			ASSERT_TRUE(searchResult != NULL);
			printf("Search Result=%s\n",searchResult);
			ASSERT_TRUE(searchResult == std::string("1")); // result should be a COM path!

			// Call finalizeImmMoIterator() and check the result
			int iter_size_1 = txContextIn_1->GetCache().GetBridgeImmIter().size();
			int mit_size_1 = txContextIn_1->GetCache().GetTheBridgeImmIterMap().size();
			MafReturnT retfinal = testObj.finalizeImmMoIterator(ith_1);
			ASSERT_TRUE(retfinal==MafOk);
			int iter_size_2 = txContextIn_1->GetCache().GetBridgeImmIter().size();
			int mit_size_2 = txContextIn_1->GetCache().GetTheBridgeImmIterMap().size();
			ASSERT_TRUE(iter_size_1==(iter_size_2 + 1));
			ASSERT_TRUE(mit_size_1==(mit_size_2 + 1));

			retfinal = testObj.finalizeImmMoIterator(ith_3);
			ASSERT_TRUE(retfinal==MafOk);
			int iter_size_3 = txContextIn_1->GetCache().GetBridgeImmIter().size();
			int mit_size_3 = txContextIn_1->GetCache().GetTheBridgeImmIterMap().size();
			ASSERT_TRUE(iter_size_2==(iter_size_3 + 1));
			ASSERT_TRUE(mit_size_2==(mit_size_3 + 1));

			int iter_size_5 = txContextIn_2->GetCache().GetBridgeImmIter().size();
			int mit_size_5 = txContextIn_2->GetCache().GetTheBridgeImmIterMap().size();
			retfinal = testObj.finalizeImmMoIterator(ith_4);
			ASSERT_TRUE(retfinal==MafOk);
			int iter_size_6 = txContextIn_2->GetCache().GetBridgeImmIter().size();
			int mit_size_6 = txContextIn_2->GetCache().GetTheBridgeImmIterMap().size();
			ASSERT_TRUE(iter_size_5==(iter_size_6 + 1));
			ASSERT_TRUE(mit_size_5==(mit_size_6 + 1));

			retfinal = testObj.finalizeImmMoIterator(ith_2);
			ASSERT_TRUE(retfinal==MafOk);
			int iter_size_4 = txContextIn_1->GetCache().GetBridgeImmIter().size();
			int mit_size_4 = txContextIn_1->GetCache().GetTheBridgeImmIterMap().size();
			ASSERT_TRUE(iter_size_3==(iter_size_4 + 1));
			ASSERT_TRUE(mit_size_3==mit_size_4);
			delete _portal;
		}
	}
}

TEST(ImmTest, setImmMo_error_Locked)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,key);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee", employeeAttrs);

	struct MafOamSpiMom* mom = maf_makeMom(NULL,employeeMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(employeeMoc);

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=1","title",9,values);
	}

	{
		MafOamSpiTransactionHandleT txh1 = 1;
		MafOamSpiTransactionHandleT txh2 = 2;
		std::tr1::shared_ptr<TxContext> txContextIn1 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh1);
		std::tr1::shared_ptr<TxContext> txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);


		MafMoAttributeValueContainer_3T* cont1 = new MafMoAttributeValueContainer_3T;
		char* myStrings[1] = {"SeniorCleaner"};
		cont1->type = MafOamSpiMoAttributeType_3_STRING;
		cont1->nrOfValues = 1;
		cont1->values = (MafMoAttributeValue_3T*) &myStrings;

		OamSAImmBridge testObj;
		lock_mo_for_config_change = true;
		MafReturnT ret3 = testObj.setImmMoAttribute(txh1,"ManagedElement=1,Employee=1","title", cont1);
		ASSERT_TRUE(ret3==MafOk);

		MafReturnT ret4 = testObj.setImmMoAttribute(txh2,"ManagedElement=1,Employee=1","title", cont1);
		ASSERT_TRUE(ret4==MafObjectLocked);
		lock_mo_for_config_change  = false;
		mymap.clear();

		delete cont1;
	}
}

TEST(ImmTest, createImmMo_error_Locked)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,key);
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoc* meMoc = maf_makeMoc((char*)"Me", key2);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=1,meId=1","title",9,values);
	}

	{
		// search in cache - created list
		// create an new employee=2 Richard Nixon, Vice President!
		// This will store the value in the cache and any search should look in the cache!!

		MafOamSpiTransactionHandleT txh1 = 1;
		MafOamSpiTransactionHandleT txh2 = 2;
		std::tr1::shared_ptr<TxContext> txContextIn1 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh1);
		std::tr1::shared_ptr<TxContext> txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);

		// -----------------------------
		// prepare array of named containers with the initial attribute values (COM MO SPI Ver.3)
		MafMoNamedAttributeValueContainer_3T* myInitialAttributes[2];

		myInitialAttributes[0] = new MafMoNamedAttributeValueContainer_3T;
		myInitialAttributes[1] = NULL; // NULL terminator for the array

		MafMoAttributeValueContainer_3T* cont1 = new MafMoAttributeValueContainer_3T;
		char* myStrings[1] = {"SeniorCleaner"};
		myInitialAttributes[0]->name = "title";
		cont1->type = MafOamSpiMoAttributeType_3_STRING;
		cont1->nrOfValues = 1;
		cont1->values = (MafMoAttributeValue_3T*) &myStrings;
		myInitialAttributes[0]->value = *cont1;

		OamSAImmBridge testObj;

		lock_mo_for_config_change = true;
		//Create an instance to prime the cache
		MafReturnT retCre = testObj.createImmMo(txh1,"ManagedElement=1,Me=1","Employee","employeeId","2", &myInitialAttributes[0]);
		ASSERT_TRUE(retCre==MafOk);

		retCre = testObj.createImmMo(txh2,"ManagedElement=1,Me=1","Employee","employeeId","2", NULL);
		ASSERT_TRUE(retCre==MafObjectLocked);
		lock_mo_for_config_change = false;
		mymap.clear();

		delete cont1;
		delete myInitialAttributes[0];
	}
}

TEST(ImmTest, deleteImmMo_error_Locked)
{
	testDomainName = DOMAIN_NAME_ECIM;
	testExtensionName = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	testExtensionValue = DOMAIN_EXT_VALUE_FALSE;
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,key);
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoc* meMoc = maf_makeMoc((char*)"Me", key2);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=1,meId=1","title",9,values);
	}

	// deleting Me=1,Employee=1 to see if it returns error ot not
	{
		MafOamSpiTransactionHandleT txh1=1;
		MafOamSpiTransactionHandleT txh2=2;
		std::tr1::shared_ptr<TxContext> txContextIn1 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh1);
		std::tr1::shared_ptr<TxContext> txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);
		OamSAImmBridge testObj;

		lock_mo_for_config_change = true;
		//Delete an instance to prime the cache
		MafReturnT retDel = testObj.deleteImmMo(txh1,"ManagedElement=1,Me=1,Employee=1");
		ASSERT_TRUE(retDel==MafOk);

		MafMoAttributeValueContainer_3T* cont1 = new MafMoAttributeValueContainer_3T;
		char* myStrings[1] = {"SeniorCleaner"};
		cont1->type = MafOamSpiMoAttributeType_3_STRING;
		cont1->nrOfValues = 1;
		cont1->values = (MafMoAttributeValue_3T*) &myStrings;

		retDel = testObj.setImmMoAttribute(txh2,"ManagedElement=1,Me=1,Employee=1","title", cont1);
		ASSERT_TRUE(retDel==MafObjectLocked);

		retDel = testObj.deleteImmMo(txh2,"ManagedElement=1,Me=1,Employee=1");
		ASSERT_TRUE(retDel==MafObjectLocked);
		lock_mo_for_config_change = false;
		mymap.clear();
		delete cont1;
	}
}

TEST(ImmTest, createImmMo_error)
{
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,key);
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoc* meMoc = maf_makeMoc((char*)"Me", key2);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=1,meId=1","title",9,values);
	}
	{
		// add the class name attribute for Me
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}
	{
		// add the class name attribute for Employee with two attributes "title"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
	}
	{
	// search in cache - created list
	// create an new employee=2 Richard Nixon, Vice President! :-)
		// This will store the value in the cache and any search should look in the cache!!

		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		OamSAImmBridge testObj;

		// -----------------------------
		// prepare array of named containers with the initial attribute values (COM MO SPI Ver.3)
		MafMoNamedAttributeValueContainer_3T* myInitialAttributes[2];

		myInitialAttributes[0] = new MafMoNamedAttributeValueContainer_3T;
		myInitialAttributes[1] = NULL; // NULL terminator for the array

		MafMoAttributeValueContainer_3T* cont1 = new MafMoAttributeValueContainer_3T;
		char* myStrings[1] = {"SeniorCleaner"};
		myInitialAttributes[0]->name = "title";
		cont1->type = MafOamSpiMoAttributeType_3_STRING;
		cont1->nrOfValues = 1;
		cont1->values = (MafMoAttributeValue_3T*) &myStrings;
		myInitialAttributes[0]->value = *cont1;


		// -----------------------------
		saImmOmClassDescriptionGet_2_setting_OK = false;
		//Create an instance to prime the cache
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","Employee","2", &myInitialAttributes[0]);
		ASSERT_TRUE(retCre==MafOk);

		// NOW we try to search and expect to get a cache hit
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		GLOBAL_immCmdOmSearchNextDnOut = "errorId=555"; // this value should NOT be returned cause of the cache!

		// perform the search, we search for "Me=1" sub instances and should get "Employee=2" back
		MafReturnT retGet = testObj.getImmMoIterator(txh,"ManagedElement=1,Me=1","Employee", p_ith); // COM only ever iterates over known classes!
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafOk);
		ASSERT_TRUE(searchResult != NULL);
		printf("Search Result=%s\n",searchResult);
		ASSERT_TRUE(searchResult == std::string("2")); //Only the instance value, not the class...COM style!
		//delete txContextIn;

		// setup the call
		MafOamSpiTransactionHandleT txh2=0;
		//TxContext* txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);
		OamSAImmBridge testObj2;
		MafMoAttributeValueContainer_3T *result2 = NULL;
		MafReturnT ret2 = testObj2.createImmMo(txh2,"ManagedElement=1,Me=1","Employee","Employee","2", &myInitialAttributes[0]);
		ASSERT_TRUE(ret2==MafNotExist);

		//MafOamSpiTransactionHandleT txh2=0;
		//TxContext* txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);
		//OamSAImmBridge testObj2;
		//MafMoAttributeValueContainer_3T *result2 = NULL;
		MafReturnT ret3 = testObj2.setImmMoAttribute(txh2,"ManagedElement=1,Me=1","Employee", cont1);
		ASSERT_TRUE(ret3==MafNotExist);
		delete cont1;
		delete myInitialAttributes[0];
		delete _portal;
	}
}

TEST(ImmTest, createImmMo)
{
	// First we setup the test so that we can perform a write!

	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//   Me<<EcimClass>,
	//     Employee<<EcimClass>>
	//           position:string
	//           employeedata:PersonData
	//
	//  PersonData<<EcimStruct>>{
	//                firstname:string;
	//                lastname:string;
	//          }

	// COM will read the model from the repository and then it will look for instances for each contained class
	// This means that COM will always iterate over a known class, and since struct classes in IMM are unknown to COM
	// COM will never iterate over them, they are infact invisible to COM without us doing anything!
	//
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiStructMember* mLastname = maf_makeStructMember((char*)"lastname", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStructMember* mFirstname = maf_makeStructMember((char*)"firstname", MafOamSpiDatatype_STRING, mLastname);
	struct MafOamSpiStruct* personData = maf_makeStruct((char*)"PersonData", mFirstname);
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;

	struct MafOamSpiMoAttribute* gender = maf_makeAttribute((char*)"employee_gender",MafOamSpiMoAttributeType_3_BOOL, key);
	struct MafOamSpiMoAttribute* age = maf_makeAttribute((char*)"employee_age",MafOamSpiMoAttributeType_3_INT8, gender);
	struct MafOamSpiMoAttribute* attrPerson = maf_makeAttribute((char*)"employeedata",personData, age);
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"position",MafOamSpiMoAttributeType_3_STRING,attrPerson);

	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoAttribute* val_int16 = maf_makeAttribute((char*)"me_int16",MafOamSpiMoAttributeType_3_INT16, key2);
	struct MafOamSpiMoAttribute* val_int32 = maf_makeAttribute((char*)"me_int32",MafOamSpiMoAttributeType_3_INT32, val_int16);
	struct MafOamSpiMoAttribute* val_int64 = maf_makeAttribute((char*)"me_int64",MafOamSpiMoAttributeType_3_INT64, val_int32);
	struct MafOamSpiMoAttribute* val_uint8 = maf_makeAttribute((char*)"me_uint8",MafOamSpiMoAttributeType_3_UINT8, val_int64);
	struct MafOamSpiMoAttribute* val_uint16 = maf_makeAttribute((char*)"me_uint16",MafOamSpiMoAttributeType_3_UINT16, val_uint8);
	struct MafOamSpiMoAttribute* val_uint32 = maf_makeAttribute((char*)"me_uint32",MafOamSpiMoAttributeType_3_UINT32, val_uint16);
	struct MafOamSpiMoAttribute* val_uint64 = maf_makeAttribute((char*)"me_uint64",MafOamSpiMoAttributeType_3_UINT64, val_uint32);
	struct MafOamSpiMoAttribute* val_decimal = maf_makeAttribute((char*)"me_decimal",MafOamSpiMoAttributeType_3_DECIMAL64, val_uint64);
	struct MafOamSpiMoAttribute* val_enum = maf_makeAttribute((char*)"me_enum",MafOamSpiMoAttributeType_3_ENUM, val_decimal);
	struct MafOamSpiMoc* meMoc = maf_makeMoc((char*)"Me", val_enum);

	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	// setup IMM storage
	immStorageG.reset();

	{
		// add the class name attribute for Employee with two attributes "position" and "employeedata"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;

		immStorageG.addToImmStorage("employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("position"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("employeedata"), IMM_REF_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("employee_age"), SA_IMM_ATTR_SAINT32T);
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("employee_gender"), SA_IMM_ATTR_SAUINT32T);
	}
	{
		// add the class name attribute for PersonData with two attributes "firstname" and "lastname"
		std::vector<std::string> className;
		className.push_back("PersonData");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("personDataId=person_0,employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("firstname"), IMM_STRING_TYPE);
		immStorageG.addImmClassAttributeDef(std::string("PersonData"), std::string("lastname"), IMM_STRING_TYPE);
	}
	// add the class name attribute for Me
	{
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_int16"), SA_IMM_ATTR_SAINT32T);
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_int32"), SA_IMM_ATTR_SAINT32T);
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_int64"), SA_IMM_ATTR_SAINT64T);
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_uint8"), SA_IMM_ATTR_SAUINT32T);
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_uint16"), SA_IMM_ATTR_SAUINT32T);
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_uint32"), SA_IMM_ATTR_SAUINT32T);
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_uint64"), SA_IMM_ATTR_SAUINT64T);
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_decimal"), SA_IMM_ATTR_SADOUBLET);
		immStorageG.addImmClassAttributeDef(std::string("Me"), std::string("me_enum"), SA_IMM_ATTR_SAUINT32T);
	}

	// iterator across root, eg DN="" and classname "Me" (COM always uses classname!) (Search directly towards IMM, no cache)
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMoAttributeValueContainer_3T *result = NULL;
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		// perform a root search

		// setup the search result
		GLOBAL_immCmdOmSearchNextDnOut = "meId=1";
		// performt the search
		MafReturnT retGet = testObj.getImmMoIterator(txh,"ManagedElement=1","Me", p_ith);
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafOk);
		ASSERT_TRUE(searchResult != NULL);
		printf("Search Result=%s\n",searchResult);
		ASSERT_TRUE(searchResult == std::string("1")); // result should be a COM path!
		//delete txContextIn;
	}

	// iterator across root, eg DN="Me=1,Employee=1" and classname Test (Search directly towards IMM, no cache)
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		OamSAImmBridge testObj;
		MafMoAttributeValueContainer_3T *result = NULL;
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		// setup the search result
		GLOBAL_immCmdOmSearchNextDnOut = "testId=66";
		// performt the search
		MafReturnT retGet = testObj.getImmMoIterator(txh,"ManagedElement=1,Me=1,Employee=1","Test", p_ith);
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafOk);
		ASSERT_TRUE(searchResult != NULL);
		printf("Search Result=%s\n",searchResult);
		ASSERT_TRUE(searchResult == std::string("66"));
		delete _portal;
	}

	{
		// create an new employee=2 Richard Nixon, Vice President! :-)
		// This will store the value in the cache and any search should look in the cache!!

		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		OamSAImmBridge testObj;

		// -----------------------------
		// prepare array of named containers with the initial attribute values (COM MO SPI Ver.3)
		MafMoNamedAttributeValueContainer_3T* myInitialAttributes[5];

		myInitialAttributes[0] = new MafMoNamedAttributeValueContainer_3T;
		myInitialAttributes[1] = new MafMoNamedAttributeValueContainer_3T;
		myInitialAttributes[2] = new MafMoNamedAttributeValueContainer_3T;
		myInitialAttributes[3] = new MafMoNamedAttributeValueContainer_3T;
		myInitialAttributes[4] = NULL; // NULL terminator for the array

		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont1 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont2 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont3 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont4 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		memset(cont1.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont2.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont3.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont4.get(), 0, sizeof(MafMoAttributeValueContainer_3T));

		char* myStrings[1] = {"SeniorCleaner"};
		myInitialAttributes[0]->name = "position";
		cont1->type = MafOamSpiMoAttributeType_3_STRING;
		cont1->nrOfValues = 1;
		cont1->values = (MafMoAttributeValue_3T*) &myStrings;
		myInitialAttributes[0]->value = *cont1;

		char* myOtherStrings[1] = {"PersonData"};
		myInitialAttributes[1]->name = "employeedata";

		cont2->type = MafOamSpiMoAttributeType_3_REFERENCE;
		cont2->nrOfValues = 1; // 2;
		cont2->values = (MafMoAttributeValue_3T*) &myOtherStrings;
		myInitialAttributes[1]->value = *cont2;

		int value[1] = {32};
		myInitialAttributes[2]->name = "employee_age";
		cont3->type = MafOamSpiMoAttributeType_3_INT8;
		cont3->nrOfValues = 1; // 2;
		cont3->values = (MafMoAttributeValue_3T*) &value;
		myInitialAttributes[2]->value = *cont3;

		unsigned int gender[1] = {0}; //male
		myInitialAttributes[3]->name = "employee_gender";
		cont4->type = MafOamSpiMoAttributeType_3_BOOL;
		cont4->nrOfValues = 1; // 2;
		cont4->values = (MafMoAttributeValue_3T*) &gender;
		myInitialAttributes[3]->value = *cont4;

		// -----------------------------

		//Create an instance to prime the cache
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","Employee","2", &myInitialAttributes[0]);
		ASSERT_TRUE(retCre==MafOk);

		// NOW we try to search and expect to get a cache hit
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		GLOBAL_immCmdOmSearchNextDnOut = "errorId=555"; // this value should NOT be returned cause of the cache!
		// perform the search, we search for "Me=1" sub instances and should get "Employee=2" back
		MafReturnT retGet = testObj.getImmMoIterator(txh,"ManagedElement=1,Me=1","Employee", p_ith); // COM only ever iterates over known classes!
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafOk);
		ASSERT_TRUE(searchResult != NULL);
		printf("Search Result=%s\n",searchResult);
		ASSERT_TRUE(searchResult == std::string("2")); //Only the instance value, not the class...COM style!
		delete _portal;
		delete myInitialAttributes[0];
		delete myInitialAttributes[1];
		delete myInitialAttributes[2];
		delete myInitialAttributes[3];
	}
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext>txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		OamSAImmBridge testObj;

		// -----------------------------
		// prepare array of named containers with the initial attribute values (COM MO SPI Ver.3)
		MafMoNamedAttributeValueContainer_3T* myInitialAttributes[10];

		myInitialAttributes[0] = new MafMoNamedAttributeValueContainer_3T;//int8
		myInitialAttributes[1] = new MafMoNamedAttributeValueContainer_3T;//int32
		myInitialAttributes[2] = new MafMoNamedAttributeValueContainer_3T;//int64
		myInitialAttributes[3] = new MafMoNamedAttributeValueContainer_3T;//uint8
		myInitialAttributes[4] = new MafMoNamedAttributeValueContainer_3T;//uint16
		myInitialAttributes[5] = new MafMoNamedAttributeValueContainer_3T;//uint32
		myInitialAttributes[6] = new MafMoNamedAttributeValueContainer_3T;//uint64
		myInitialAttributes[7] = new MafMoNamedAttributeValueContainer_3T;//decimal
		myInitialAttributes[8] = new MafMoNamedAttributeValueContainer_3T;//enum
		myInitialAttributes[9] = NULL; // NULL terminator for the array

		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont1 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont2 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont3 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont4 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont5 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont6 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont7 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont8 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		std::tr1::shared_ptr<MafMoAttributeValueContainer_3T> cont9 =std::tr1::shared_ptr<MafMoAttributeValueContainer_3T>(new MafMoAttributeValueContainer_3T);
		memset(cont1.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont2.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont3.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont4.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont5.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont6.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont7.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont8.get(), 0, sizeof(MafMoAttributeValueContainer_3T));
		memset(cont9.get(), 0, sizeof(MafMoAttributeValueContainer_3T));

		int val_int16[1] = {-3232};
		myInitialAttributes[0]->name = "me_int16";
		cont1->type = MafOamSpiMoAttributeType_3_INT16;
		cont1->nrOfValues = 1; // 2;
		cont1->values = (MafMoAttributeValue_3T*) &val_int16;
		myInitialAttributes[0]->value = *cont1;

		int val_int32[1] = {-123456789};
		myInitialAttributes[1]->name = "me_int32";
		cont2->type = MafOamSpiMoAttributeType_3_INT32;
		cont2->nrOfValues = 1; // 2;
		cont2->values = (MafMoAttributeValue_3T*) &val_int32;
		myInitialAttributes[1]->value = *cont2;

		long val_int64[1] = {-1234567891234};
		myInitialAttributes[2]->name = "me_int64";
		cont3->type = MafOamSpiMoAttributeType_3_INT64;
		cont3->nrOfValues = 1; // 2;
		cont3->values = (MafMoAttributeValue_3T*) &val_int64;
		myInitialAttributes[2]->value = *cont3;

		unsigned int val_uint8[1] = {12};
		myInitialAttributes[3]->name = "me_uint8";
		cont4->type = MafOamSpiMoAttributeType_3_UINT8;
		cont4->nrOfValues = 1; // 2;
		cont4->values = (MafMoAttributeValue_3T*) &val_uint8;
		myInitialAttributes[3]->value = *cont4;

		unsigned int val_uint16[1] = {6400};
		myInitialAttributes[4]->name = "me_uint16";
		cont5->type = MafOamSpiMoAttributeType_3_UINT16;
		cont5->nrOfValues = 1; // 2;
		cont5->values = (MafMoAttributeValue_3T*) &val_uint16;
		myInitialAttributes[4]->value = *cont5;

		unsigned int val_uint32[1] = {6400000};
		myInitialAttributes[5]->name = "me_uint32";
		cont6->type = MafOamSpiMoAttributeType_3_UINT32;
		cont6->nrOfValues = 1; // 2;
		cont6->values = (MafMoAttributeValue_3T*) &val_uint32;
		myInitialAttributes[5]->value = *cont6;

		unsigned long val_uint64[1] = {640000000000};
		myInitialAttributes[6]->name = "me_uint64";
		cont7->type = MafOamSpiMoAttributeType_3_UINT64;
		cont7->nrOfValues = 1; // 2;
		cont7->values = (MafMoAttributeValue_3T*) &val_uint64;
		myInitialAttributes[6]->value = *cont7;

		double val_decimal[1] = {640000000000};
		myInitialAttributes[7]->name = "me_decimal";
		cont8->type = MafOamSpiMoAttributeType_3_DECIMAL64;
		cont8->nrOfValues = 1; // 2;
		cont8->values = (MafMoAttributeValue_3T*) &val_decimal;
		myInitialAttributes[7]->value = *cont8;


		unsigned int val_enum[1] = {5};
		myInitialAttributes[8]->name = "me_enum";
		cont9->type = MafOamSpiMoAttributeType_3_ENUM;
		cont9->nrOfValues = 1; // 2;
		cont9->values = (MafMoAttributeValue_3T*) &val_enum;
		myInitialAttributes[8]->value = *cont9;
		// -----------------------------

		//Create an instance to prime the cache
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1","Me","meId","3", &myInitialAttributes[0]);
		ASSERT_TRUE(retCre==MafOk);

		// NOW we try to search and expect to get a cache hit
		MafOamSpiMoIteratorHandle_3T ith;
		MafOamSpiMoIteratorHandle_3T* p_ith = &ith;

		GLOBAL_immCmdOmSearchNextDnOut = "errorId=555"; // this value should NOT be returned cause of the cache!

		// perform the search, we search for "Me=1" sub instances and should get "Employee=2" back
		MafReturnT retGet = testObj.getImmMoIterator(txh,"ManagedElement=1","Me", p_ith); // COM only ever iterates over known classes!
		ASSERT_TRUE(retGet==MafOk);
		char* searchResult = NULL;
		MafReturnT retNext = testObj.getImmNextMo (ith, &searchResult);
		ASSERT_TRUE(retNext==MafOk);
		ASSERT_TRUE(searchResult != NULL);
		printf("Search Result=%s\n",searchResult);
		ASSERT_TRUE(searchResult == std::string("3")); //Only the instance value, not the class...COM style!
		delete _portal;
		delete myInitialAttributes[0];
		delete myInitialAttributes[1];
		delete myInitialAttributes[2];
		delete myInitialAttributes[3];
		delete myInitialAttributes[4];
		delete myInitialAttributes[5];
		delete myInitialAttributes[6];
		delete myInitialAttributes[7];
		delete myInitialAttributes[8];
	}
}


TEST(ImmTest, ImmexistsMo)
{
	// First we setup the test so that we can perform a write!

	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//   Me<<EcimClass>,
	//     Employee<<EcimClass>>
	//           title:string
	//


	// COM will read the model from the repository and then it will look for instances for each contained class
	// This means that COM will always iterate over a known class, and since struct classes in IMM are unknown to COM
	// COM will never iterate over them, they are infact invisible to COM without us doing anything!
	//
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();

	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,key);
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoc* meMoc = maf_makeMoc((char*)"Me", key2);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);

	// setup IMM storage
	immStorageG.reset();
	{
		// add Employee instance 1 attribute: title
		std::vector<std::string> values;
		values.push_back(std::string("President"));
		immStorageG.addToImmStorage("employeeId=1,meId=1","title",9,values);
	}
	{
		// add the class name attribute for Me
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}
	{
		// add the class name attribute for Employee with two attributes "title"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=9;
		unsigned int IMM_REF_TYPE = 6;
		immStorageG.addToImmStorage("employeeId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
	}

	// search in IMM for "Me=1,Employee=1"
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		// setup the search result
		isSDP1694 = true;
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("managedElementId=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("meId=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=1");
		// performt the search
		bool result;
		MafReturnT retGet = testObj.existsImmMo(txh,"ManagedElement=1,Me=1,Employee=1", &result);
		printf("Result %d \n",result);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(result==true);
		//delete txContextIn;
	}
	// search in IMM for "Me=1,Employee=2"
	{
		unsigned int temp_for_saImmOmAccessorGet_2_setting = saImmOmAccessorGet_2_setting;
		unsigned int saImmOmAccessorGet_4_testcase = 4;
		saImmOmAccessorGet_2_switch test_case_imm_exist_mo(saImmOmAccessorGet_4_testcase);
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		// setup the search result
		GLOBAL_immCmdOmSearchNextDnOut_ptr.clear();
		// performt the search
		bool result;
		MafReturnT retGet = testObj.existsImmMo(txh,"ManagedElement=1,Me=1,Employee=2", &result);
		saImmOmAccessorGet_2_switch set_to_prev_value(temp_for_saImmOmAccessorGet_2_setting);
		printf("Result %d \n",result);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(result!=true);
		//delete txContextIn;
	}
	// search in cache - created list
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		// prepare a named array of containers with the initial attribute values (COM MO SPI Ver.3)
		MafMoNamedAttributeValueContainer_3T myInitialAttributes;
		MafMoNamedAttributeValueContainer_3T* p_myInitialAttributes = NULL;

		//Create an instance to prime the cache
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","employeeId","2", &p_myInitialAttributes);
		ASSERT_TRUE(retCre==MafOk);

		// performt the search
		bool result;
		MafReturnT retGet = testObj.existsImmMo(txh,"ManagedElement=1,Me=1,Employee=2", &result);
		printf("Result %d \n",result);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(result==true);
		delete _portal;
	}

	// search in cache - deleted list
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		//Delete an instance to prime the cache
		MafReturnT retDle = testObj.deleteImmMo(txh,"ManagedElement=1,Me=1,Employee=1");
		ASSERT_TRUE(retDle==MafOk);

		// performt the search
		bool result;
		MafReturnT retGet = testObj.existsImmMo(txh,"ManagedElement=1,Me=1,Employee=1", &result);
		printf("Result %d \n",result);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(result!=true);
		delete _portal;
	}

	{
		MafOamSpiTransactionHandleT txh=0;
		//TxContext* txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		//Delete an instance to prime the cache
		MafReturnT retDle = testObj.deleteImmMo(txh,"ManagedElement=1,Me=1,Employee=1");
		ASSERT_TRUE(retDle==MafNotExist);

		delete _portal;
	}

	// delete Me=1,Employee=1 then create again and check again it exist in system or not (expected it still exit)
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		//Delete an instance to prime the cache
		MafReturnT retDle = testObj.deleteImmMo(txh,"ManagedElement=1,Me=1,Employee=1");
		ASSERT_TRUE(retDle==MafOk);

		MafMoNamedAttributeValueContainer_3T myInitialAttributes;
		MafMoNamedAttributeValueContainer_3T* p_myInitialAttributes = NULL;
		//Create an instance to prime the cache
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","employeeId","1", &p_myInitialAttributes);
		ASSERT_TRUE(retCre==MafOk);

		// performt the search
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("managedElementId=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("meId=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=1");
		bool result;
		MafReturnT retGet = testObj.existsImmMo(txh,"ManagedElement=1,Me=1,Employee=1", &result);
		printf("Result %d \n",result);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(result==true);
		delete _portal;
	}

	// create Me=1,Employee=2 then delete and check again it exist in system or not (expected it not exist)
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		//Create an instance to prime the cache
		MafMoNamedAttributeValueContainer_3T myInitialAttributes;
		MafMoNamedAttributeValueContainer_3T* p_myInitialAttributes = NULL;
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","Employee","2", &p_myInitialAttributes);
		ASSERT_TRUE(retCre==MafOk);

		//Delete an instance to prime the cache
		MafReturnT retDle = testObj.deleteImmMo(txh,"ManagedElement=1,Me=1,Employee=2");
		ASSERT_TRUE(retDle==MafOk);

		// performt the search
		GLOBAL_immCmdOmSearchNextDnOut_ptr.clear();
		bool result;
		MafReturnT retGet = testObj.existsImmMo(txh,"ManagedElement=1,Me=1,Employee=2", &result);
		printf("Result %d \n",result);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(result!=true);
		delete _portal;
	}
	isSDP1694 = false;
}


TEST(ImmTest, CountImmMoChildren)
{
	// Setup ModelRepository with Me base class and Sub class Employee containing a struct type PersonData

	//   Me=1,
	//     Employee=1
	//           title:string
	//     Employee=2
	//           title:string
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"employeeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	struct MafOamSpiMoAttribute* employeeAttrs = maf_makeAttribute((char*)"title",MafOamSpiMoAttributeType_3_STRING,key);
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;
	struct MafOamSpiMoc* meMoc = maf_makeMoc((char*)"Me", key2);
	struct MafOamSpiMoc* employeeMoc = maf_makeMoc((char*)"Employee",employeeAttrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, employeeMoc);
	struct MafOamSpiMom* mom = maf_makeMom(NULL,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);
	// Clean the immStorage so we do not have any data left.
	immStorageG.reset();

	//set up IMM storage
	{
		// add the class name attribute for Employee with two attributes "title"
		std::vector<std::string> className;
		className.push_back("Employee");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("employeeId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		immStorageG.addToImmStorage("employeeId=2,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
		// add attribute defintions to class
		immStorageG.addImmClassAttributeDef(std::string("Employee"), std::string("title"), IMM_STRING_TYPE);
	}
	// add the class name attribute for Me
	{
		std::vector<std::string> className;
		className.push_back("Me");
		unsigned int IMM_STRING_TYPE=9;
		immStorageG.addToImmStorage("meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	}
		// search in IMM for className = "Employee"
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;

		// performt the search
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=2,Me=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=1,Me=1");
		isSDP1694 = true;
		uint64_t num_mo;
		MafReturnT retGet = testObj.countImmMoChildren(txh,"ManagedElement=1,Me=1", "Employee", &num_mo);
		printf("num_mo %d \n",num_mo);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(num_mo==2);
		//delete txContextIn;
	}
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;

		// performt the search
		GLOBAL_immCmdOmSearchNextDnOut_ptr.clear();
		uint64_t num_mo;
		MafReturnT retGet = testObj.countImmMoChildren(txh,"ManagedElement=1,Me=1", "Employee", &num_mo);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(num_mo==0);
		//delete txContextIn;
	}
	// search in cache (with createImmMo())
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		//Create an instance to prime the cache
		// Note: In version 3 have change in createImmMo(). pay attention here when it is implemented
		// prepare a named array of containers with the initial attribute values (COM MO SPI Ver.3)
				MafMoNamedAttributeValueContainer_3T myInitialAttributes;
				MafMoNamedAttributeValueContainer_3T* p_myInitialAttributes = NULL;

		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","employeeId","3", &p_myInitialAttributes);
		ASSERT_TRUE(retCre==MafOk);

		// performt the search
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=2,meId=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=1,meId=1");

		uint64_t num_mo;
		MafReturnT retGet = testObj.countImmMoChildren(txh,"ManagedElement=1,Me=1", "Employee", &num_mo);
		printf("num_mo %d \n",num_mo);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(num_mo==3);
		delete _portal;
	}

	// search in cache (with deleteImmMo())
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		//Delete an instance to prime the cache
		MafReturnT retDle = testObj.deleteImmMo(txh,"ManagedElement=1,Me=1,Employee=2");
		ASSERT_TRUE(retDle==MafOk);

		// performt the search
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=2,meId=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=1,meId=1");

		uint64_t num_mo;
		MafReturnT retGet = testObj.countImmMoChildren(txh,"ManagedElement=1,Me=1", "Employee", &num_mo);
		printf("num_mo %d \n",num_mo);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(num_mo==1);
		delete _portal;
	}
	// search in cache (with deleteImmMo() and createImmMo())
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		//Delete an instance to prime the cache
		MafReturnT retDle = testObj.deleteImmMo(txh,"ManagedElement=1,Me=1,Employee=2");
		ASSERT_TRUE(retDle==MafOk);
		//Create an instance to prime the cache
		MafMoNamedAttributeValueContainer_3T myInitialAttributes;
		MafMoNamedAttributeValueContainer_3T* p_myInitialAttributes = NULL;
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","employeeId","2", &p_myInitialAttributes);
		ASSERT_TRUE(retCre==MafOk);

		// performt the search
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=2,meId=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=1,meId=1");

		uint64_t num_mo;
		MafReturnT retGet = testObj.countImmMoChildren(txh,"ManagedElement=1,Me=1", "Employee", &num_mo);
		printf("num_mo %d \n",num_mo);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(num_mo==2);
		delete _portal;
	}
	// search in cache (with createImmMo() and deleteImmMo())
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;

		//Create an instance to prime the cache
		MafMoNamedAttributeValueContainer_3T myInitialAttributes;
		MafMoNamedAttributeValueContainer_3T* p_myInitialAttributes = NULL;
		MafReturnT retCre = testObj.createImmMo(txh,"ManagedElement=1,Me=1","Employee","employeeId","3", NULL);
		ASSERT_TRUE(retCre==MafOk);
		//Delete an instance to prime the cache
		MafReturnT retDle = testObj.deleteImmMo(txh,"ManagedElement=1,Me=1,Employee=3");
		ASSERT_TRUE(retDle==MafOk);

		// performt the search
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=2,meId=1");
		GLOBAL_immCmdOmSearchNextDnOut_ptr.push_back("employeeId=1,meId=1");

		uint64_t num_mo;
		MafReturnT retGet = testObj.countImmMoChildren(txh,"ManagedElement=1,Me=1", "Employee", &num_mo);
		printf("num_mo %d \n",num_mo);
		ASSERT_TRUE(retGet==MafOk);
		ASSERT_TRUE(num_mo==2);
		delete _portal;
	}

	isSDP1694 = false;
}

class ActionTestHelper {
public:
	ActionTestHelper() {
		init();
	}
	void init() {
		actionName = "backup";
		actionClassName = "ActionClass";
		admOpId = DOMAIN_EXT_NAME_ADM_OP_ID;
		admOpValue = "3";
		domain = DOMAIN_NAME_COREMW;
	}
	void setupMom(bool containsAction = true, bool containsAdmOpValue = true, bool errorCase = false) {
		printf("ActionTestHelper::setupMom(): ENTER\n");
		setupActionObject();
		if (containsAction)
		setupAction(containsAdmOpValue, errorCase);
		maf_setMom(mom);
		// Set the parent Containment to ManagedElement
		maf_makeParentContainmentToTop((struct MafOamSpiMoc*)(mom->rootMoc));
		// OK, need to trigger the read of the model repository, do this by a fake question
		// A bit ugly, but works, it Populates the structures
		extern OamSATranslator theTranslator;
		theTranslator.IsClassNamePresent("READ_MODEL_REPO");
		printf("ActionTestHelper::setupMom(): RETURN\n");
	}
	;
	void setupAction(bool containsAdmOpValue, bool errorCase = false) {

		action = new MafOamSpiMoAction;
		dext = new MafOamSpiDomainExtension;
		ext = new MafOamSpiExtension;
		ext->name = admOpId.c_str();
		ext->value = (containsAdmOpValue ? admOpValue.c_str() : NULL);
		ext->next = NULL;
		dext->domain = domain.c_str();
		dext->extensions = ext;
		action->domainExtension = dext;
		amoc->moAction = action;
		action->generalProperties.name = (char*) actionName.c_str();
		action->parameters = 0;
		action->next = NULL;
		action->returnType.type = MafOamSpiDatatype_BOOL;
		if (errorCase){
			amoc->moAction = NULL;
		}
	}
	void setupActionObject() {
		extern OamSATranslator theTranslator;
		theTranslator.ResetMOMRoot();
		struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
		key->isKey = true;

		memoc = maf_makeMoc("Me", key);
		std::string temp = actionClassName;
		temp[0] = ::tolower(temp[0]);
		keyAttributeName = temp + "Id";
		struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)keyAttributeName.c_str(), MafOamSpiMoAttributeType_3_STRING, NULL);
		key2->isKey = true;
		amoc = maf_makeMoc(actionClassName.c_str(), key2);
		cont = maf_makeContainment(memoc, amoc);
		mom = maf_makeMom("MEMOM", memoc);
		amoc->mom = mom;
		amoc->moAction = NULL;
	}

	void setActionParameters(MafOamSpiParameterT* parameterList) {
		if (action != 0) {
			action->parameters = parameterList;
		}
	}
	void setActionReturnType(MafOamSpiDatatypeT returnType) {
		if (action != 0) {
			action->returnType.type = returnType;
		}
	}

	void clearMom() {
		if (action) {
			delete ext;
			delete dext;
			delete action;
		}
		extern OamSATranslator theTranslator;
		theTranslator.ResetMOMRoot();
		//Remove linked model from ManagedElement
		ext = NULL;
		dext = NULL;
		action = NULL;
		amoc = NULL;
		memoc = NULL;
		cont = NULL;
		mom = NULL;
	}
	;
	std::string actionName;
	std::string actionClassName;
	std::string admOpId;
	std::string admOpValue;
	std::string domain;
	//private:
	struct MafOamSpiMoc* memoc;
	struct MafOamSpiMoc *amoc;
	struct MafOamSpiContainment *cont;
	struct MafOamSpiMoAction *action;
	struct MafOamSpiDomainExtension *dext;
	struct MafOamSpiExtension *ext;
	struct MafOamSpiMom *mom;
	std::string keyAttributeName;
};


TEST(ActionTest, CallAction)
{
	ActionTestHelper helper;
	helper.setupMom();

	// setup IMM storage
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE=9;
	immStorageG.addToImmStorage("actionClassId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);

	// Normal call
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		MafMoAttributeValueContainer_3T *params = NULL;

		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		MafReturnT ret = maf_comSAMgmtSpiThreadContInit(_portal); // set _portal to testObject

		// mock for errorCode=saImmOmCcbGetErrorStrings(txContextIn->mCcbHandle,&errorStrings);
		// txConmtextIn->mCcbHandle is of type SaImmHandleT
		txContextIn->mCcbHandle = 8;
		SaImmHandleT immHandle = txContextIn->mCcbHandle;

		//com_rc=threadContext->addMessage(category,errorString);
		//MafMgmtSpiThreadContext_2T* threadContext = new MafMgmtSpiThreadContext_2T;
		_threadContext = new MafMgmtSpiThreadContext_2T;
		_threadContext->addMessage = &addMessage_mock;

		const SaStringT* errorStrings;
		get_interface_mock_accessed=false;
		saImmOmCcbGetErrorStrings_accessed=false;
		addMessage_mock_accessed=false;

		OamSAImmBridge testObj;
		ret = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", &params, &result);

		// Verify result
		ASSERT_TRUE(result->nrOfValues == 1);
		ASSERT_TRUE(result->values->value.theBool == true);
		ASSERT_TRUE(result->type == 10);
		ASSERT_TRUE(ret==MafOk);
		//delete txContextIn;

		// setup the call
		MafOamSpiTransactionHandleT txh2=0;
		//TxContext* txContextIn2 = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh2);
		OamSAImmBridge testObj2;
		MafMoAttributeValueContainer_3T *result2 = NULL;
		MafReturnT ret2 = testObj2.action(txh2,"ManagedElement=1,Me=1,ActionClass=1","backup",&params,&result2);
		ASSERT_TRUE(ret2==MafNotExist);
		delete _portal;
		delete _threadContext;
	}
	helper.clearMom();
	addMessage_mock_strings.clear();
}

TEST(ActionTest, ParamErrors)
{
	ActionTestHelper helper;
	helper.setupMom();

	immStorageG.reset();
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE=9;
	immStorageG.addToImmStorage("actionClassId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);

	// setup the call
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	MafMoAttributeValueContainer_3T *result = NULL;
	MafMoAttributeValueContainer_3T *params = NULL;

	MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
	_portal->getInterface = &get_interface_mock;
	MafReturnT ret = maf_comSAMgmtSpiThreadContInit(_portal); // set _portal to testObject

	// mock for errorCode=saImmOmCcbGetErrorStrings(txContextIn->mCcbHandle,&errorStrings);
	// txConmtextIn->mCcbHandle is of type SaImmHandleT
	txContextIn->mCcbHandle = 8;
	SaImmHandleT immHandle = txContextIn->mCcbHandle;

	//com_rc=threadContext->addMessage(category,errorString);
	//MafMgmtSpiThreadContext_2T* threadContext = new MafMgmtSpiThreadContext_2T;
	_threadContext = new MafMgmtSpiThreadContext_2T;
	_threadContext->addMessage = &addMessage_mock;

	const SaStringT* errorStrings;
	get_interface_mock_accessed=false;
	saImmOmCcbGetErrorStrings_accessed=false;
	addMessage_mock_accessed=false;

	OamSAImmBridge testObj;
	ret = testObj.action(txh, "ManagedElement=1,NothingHere=1,ActionClass=1", "backup", &params, &result);

	// Verify result
	ASSERT_TRUE(result != NULL);
	ASSERT_TRUE(result->nrOfValues == 0);
	ASSERT_TRUE(result->values == NULL);
	ASSERT_TRUE(ret == MafFailure);
	//delete txContextIn;
	addMessage_mock_strings.clear();
	helper.clearMom();
	delete _portal;
	delete _threadContext;
}

TEST(ActionTest, MomErrors)
{
	// setup IMM storage
	immStorageG.reset();
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE=9;
	immStorageG.addToImmStorage("actionClassId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);

	// No domain
	{
		ActionTestHelper helper;
		helper.setupMom();
		helper.domain = "";

		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		MafMoAttributeValueContainer_3T *params = NULL;
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		MafReturnT ret = maf_comSAMgmtSpiThreadContInit(_portal); // set _portal to testObject
		ret = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", &params, &result);

		// Verify result
		ASSERT_TRUE(result->nrOfValues == 0);
		ASSERT_TRUE(ret==MafFailure);
		//delete txContextIn;
		addMessage_mock_strings.clear();
		helper.clearMom();
		delete _portal;
	}
	// No admOpId
	{
		ActionTestHelper helper;
		helper.setupMom();
		helper.admOpId = "";

		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		MafMoAttributeValueContainer_3T *params = NULL;
		OamSAImmBridge testObj;
		MafReturnT ret = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", &params, &result);

		// Verify result
		ASSERT_TRUE(result->nrOfValues == 0);
		ASSERT_TRUE(ret==MafFailure);
		//delete txContextIn;
		addMessage_mock_strings.clear();
		helper.clearMom();
	}
	// No admOpId value
	{
		ActionTestHelper helper;
		helper.setupMom(true, false);

		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		MafMoAttributeValueContainer_3T *params = NULL;
		OamSAImmBridge testObj;
		MafReturnT ret = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", &params, &result);

		// Verify result
		ASSERT_TRUE(result->nrOfValues == 0);
		ASSERT_TRUE(ret==MafFailure);
		//delete txContextIn;
		addMessage_mock_strings.clear();
		helper.clearMom();
	}
	// No action on object
	{
		ActionTestHelper helper;
		helper.setupMom(false);

		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		MafMoAttributeValueContainer_3T *params = NULL;
		OamSAImmBridge testObj;
		MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
		_portal->getInterface = &get_interface_mock;
		MafReturnT ret = maf_comSAMgmtSpiThreadContInit(_portal); // set _portal to testObject
		ret = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", &params, &result);

		// Verify result
		ASSERT_TRUE(result->nrOfValues == 0);
		ASSERT_TRUE(ret==MafFailure);
		//delete txContextIn;
		addMessage_mock_strings.clear();
		helper.clearMom();
		delete _portal;
	}

	// Must be in last test or create a gtest main.
	immMemoryTrackerG.cleanup();
}

//////////////////////////////////////////////////////
// Check Simple Parameters
//////////////////////////////////////////////////////

MafOamSpiParameter* ParameterVerifier::createParameter(
MafOamSpiDatatype dataType, char* name, MafOamSpiParameter* prev) {
	MafOamSpiParameter* p = new MafOamSpiParameter;
	memset(p, 0, sizeof(MafOamSpiParameter));

	p->parameterType.type = dataType;
	p->generalProperties.name = name;
	p->parameterType.derivedDatatype = 0;
	p->generalProperties.description = 0;
	p->generalProperties.specification = 0;
	p->next = 0;
	if (prev) prev->next = p;
	return p;
}


void ParameterVerifier::setParam(MafOamSpiMoAttributeType_3 dataType,
								 MafMoAttributeValue_3 *value,
								 int numberValues = 1) {
	MafMoAttributeValueContainer_3T* p = new MafMoAttributeValueContainer_3T();
	memset(p, 0, sizeof(MafMoAttributeValueContainer_3T));
	p->type = dataType;
	p->nrOfValues = numberValues;
	p->values = value;
	m_paramList.push_back(p);
}

void ParameterVerifier::checkTypeAndValue(MafOamSpiDatatypeT mafType,
SaImmValueTypeT immType, MafMoAttributeValue_3 *expectedValue,
void *actualValuePt) {
	switch (mafType) {
	case MafOamSpiDatatype_INT8:
		EXPECT_EQ(SA_IMM_ATTR_SAINT32T, immType) << "Wrong parameter type i8";
		EXPECT_EQ(expectedValue->value.i8, *((int*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_INT16:
		EXPECT_EQ(SA_IMM_ATTR_SAINT32T, immType) << "Wrong parameter type i16";
		EXPECT_EQ(expectedValue->value.i16, *((int*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_INT32:
		EXPECT_EQ(SA_IMM_ATTR_SAINT32T, immType) << "Wrong parameter type i32";
		EXPECT_EQ(expectedValue->value.i32, *((int*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_INT64:
		EXPECT_EQ(SA_IMM_ATTR_SAINT64T, immType) << "Wrong parameter type i64";
		EXPECT_EQ(expectedValue->value.i64, *((int64_t*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_UINT8:
		EXPECT_EQ(SA_IMM_ATTR_SAUINT32T, immType) << "Wrong parameter type u8";
		EXPECT_EQ(expectedValue->value.u8, *((int*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_UINT16:
		EXPECT_EQ(SA_IMM_ATTR_SAUINT32T, immType) << "Wrong parameter type u16";
		EXPECT_EQ(expectedValue->value.u16, *((int*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_UINT32:
		EXPECT_EQ(SA_IMM_ATTR_SAUINT32T, immType) << "Wrong parameter type u32";
		EXPECT_EQ(expectedValue->value.u32, *((int*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_UINT64:
		EXPECT_EQ(SA_IMM_ATTR_SAUINT64T, immType) << "Wrong parameter type u64";
		EXPECT_EQ(expectedValue->value.u64, *((uint64_t*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_STRING:
		EXPECT_EQ(SA_IMM_ATTR_SASTRINGT, immType) << "Wrong parameter type string";
		EXPECT_STREQ(expectedValue->value.theString, *((SaStringT *) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_BOOL:
		EXPECT_EQ(SA_IMM_ATTR_SAINT32T, immType) << "Wrong parameter type bool";
		EXPECT_EQ(expectedValue->value.i32, *((int*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_ENUM:
		EXPECT_EQ(SA_IMM_ATTR_SAINT32T, immType) << "Wrong parameter type enum";
		EXPECT_EQ(expectedValue->value.i32, *((int*) (actualValuePt))) << "Wrong value";
		break;
	case MafOamSpiDatatype_REFERENCE:
		{
			ASSERT_EQ(SA_IMM_ATTR_SANAMET, immType) << "Wrong parameter type reference";
			SaNameT *actual = (SaNameT *) actualValuePt;
			int expectedLen = strlen(expectedValue->value.theString);
			ASSERT_EQ(expectedLen, saNameLen(actual)) << "Wrong length";

			for (int i=0; i< expectedLen;i++) {
				ASSERT_EQ(expectedValue->value.theString[i], saNameGet(actual)[i]) << "Wrong character on pos " + i;
			}
		}
		break;
	default:
		FAIL() << "Parameter handling error";
	}
}

void ParameterVerifier::checkParameters(SaImmAdminOperationParamsT_2 **pParams) {
	// Calc number of expected parameters
	int ep = 0;
	vector<MafMoAttributeValueContainer_3T*>::iterator iter = m_paramList.begin();
	while (iter != m_paramList.end()) {
		MafMoAttributeValueContainer_3T* param = *iter;
		if (param) {
			if (param->type == MafOamSpiMoAttributeType_3_STRUCT) {
				MafMoAttributeValueStructMember_3 *m = param->values->value.structMember;
				while (m) {
					ep += m->memberValue->nrOfValues;
					m = m->next;
				}
			}
			else {
				if (param->nrOfValues > 1) {
					ep += param->nrOfValues;
				} else {
					ep++;
				}
			}
		}
		iter++;
	}

	printf("-----> Expecting %d number of paramters\n", ep);

	// Assume they come in the same order as the vector
	SaImmAdminOperationParamsT_2 *p = *pParams;
	int i = 0;
	int j = 0;
	int k = 0;
	bool isMultiValue = false;
	MafOamSpiParameterT* modelParam = m_parameterModel;

	// go through the parameters
	while (p) {
		printf("-----> Receiving parameter %s\n", p->paramName);
		ASSERT_LE(i, ep) << "Too many parameters";

		MafMoAttributeValueContainer_3T* origP = m_paramList[j];
		if (!modelParam) {
			FAIL() << "Modelling error";
		}
		if (origP->nrOfValues > 1) {
			isMultiValue = true;
		} else {
			isMultiValue = false;
		}
		MafMoAttributeValue_3T* origP_values;
		printf("----->modelParam->generalProperties.name: %s\n",modelParam->generalProperties.name);
		printf("----->p->paramName:  %s\n",p->paramName);
		if (origP->type != MafOamSpiMoAttributeType_3_STRUCT && isMultiValue) {
			std::string memberName = modelParam->generalProperties.name;
			char	numbuf[32];
			sprintf(numbuf,"%u",k + 1);
			std::string str(numbuf);
			memberName += '_' + str;
			ASSERT_STREQ(memberName.c_str(), p->paramName) << "Parameter name is wrong";
			origP_values = & origP->values[k];
		}
		else if (origP->type != MafOamSpiMoAttributeType_3_STRUCT) {
			ASSERT_STREQ(modelParam->generalProperties.name, p->paramName) << "Parameter name is wrong";
			origP_values = origP->values;
		} else {
			origP_values = origP->values;
		}

		MafOamSpiMoAttributeType_3 t = origP->type;
		//		if (t == MafOamSpiMoAttributeType_3_DERIVED) {
		//	t = modelParam->parameterType.derivedDatatype->type;
		//}

		switch (t) {
		case MafOamSpiMoAttributeType_3_INT8:
			EXPECT_EQ(SA_IMM_ATTR_SAINT32T, p->paramType) << "Wrong parameter type i8";
			EXPECT_EQ(origP_values->value.i8, *((int*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_INT16:
			EXPECT_EQ(SA_IMM_ATTR_SAINT32T, p->paramType) << "Wrong parameter type i16";
			EXPECT_EQ(origP_values->value.i16, *((int*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_INT32:
			EXPECT_EQ(SA_IMM_ATTR_SAINT32T, p->paramType) << "Wrong parameter type i32";
			EXPECT_EQ(origP_values->value.i32, *((int*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_INT64:
			EXPECT_EQ(SA_IMM_ATTR_SAINT64T, p->paramType) << "Wrong parameter type i64";
			EXPECT_EQ(origP_values->value.i64, *((int64_t*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_UINT8:
			EXPECT_EQ(SA_IMM_ATTR_SAUINT32T, p->paramType) << "Wrong parameter type u8";
			EXPECT_EQ(origP_values->value.u8, *((int*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_UINT16:
			EXPECT_EQ(SA_IMM_ATTR_SAUINT32T, p->paramType) << "Wrong parameter type u16";
			EXPECT_EQ(origP_values->value.u16, *((int*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_UINT32:
			EXPECT_EQ(SA_IMM_ATTR_SAUINT32T, p->paramType) << "Wrong parameter type u32";
			EXPECT_EQ(origP_values->value.u32, *((int*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_UINT64:
			EXPECT_EQ(SA_IMM_ATTR_SAUINT64T, p->paramType) << "Wrong parameter type u64";
			EXPECT_EQ(origP_values->value.u64, *((uint64_t*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_STRING:
			EXPECT_EQ(SA_IMM_ATTR_SASTRINGT, p->paramType) << "Wrong parameter type string";
			EXPECT_STREQ(origP_values->value.theString, *((SaStringT *) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_BOOL:
			EXPECT_EQ(SA_IMM_ATTR_SAINT32T, p->paramType) << "Wrong parameter type bool";
			EXPECT_EQ(origP_values->value.i32, *((int*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_ENUM:
			EXPECT_EQ(SA_IMM_ATTR_SAINT32T, p->paramType) << "Wrong parameter type enum";
			EXPECT_EQ(origP_values->value.i32, *((int*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_DECIMAL64:
			EXPECT_EQ(SA_IMM_ATTR_SADOUBLET, p->paramType) << "Wrong parameter type double";
			EXPECT_EQ(origP_values->value.decimal64, *((double*) (p->paramBuffer))) << "Wrong value";
			break;
		case MafOamSpiMoAttributeType_3_REFERENCE:
			{
				ASSERT_EQ(SA_IMM_ATTR_SANAMET, p->paramType) << "Wrong parameter type reference";
				struct MafMoAttributeValue_3 *orgValue = origP_values;
				orgValue++; // Contains the answer

				SaNameT *actual = (SaNameT *) p->paramBuffer;
				int expectedLen = strlen(orgValue->value.theString);
				ASSERT_EQ(expectedLen, saNameLen(actual)) << "Wrong length";

				for (int i=0; i< expectedLen;i++) {
					ASSERT_EQ(orgValue->value.theString[i], saNameGet(actual)[i]) << "Wrong character on pos " + i;
				}
			}

			break;

		case MafOamSpiMoAttributeType_3_STRUCT: {
				MafOamSpiStructMember* modelMember =
				modelParam->parameterType.structDatatype->members;
				MafMoAttributeValueStructMember_3* parMember =
				origP_values->value.structMember;

				int l = 0;
				while (modelMember && p && parMember) {

					for (l =0; l < parMember->memberValue->nrOfValues; l++) {
						string expectedParamName(modelParam->generalProperties.name);
						//					expectedParamName += ".";
						expectedParamName += "_"; // SDP975
						expectedParamName += modelMember->generalProperties.name;
						char	num[32];
						sprintf(num,"%u",l + 1);
						std::string str(num);
						if (parMember->memberValue->nrOfValues > 1)
						expectedParamName += '_' + str;
						ASSERT_STREQ(expectedParamName.c_str(), p->paramName)
						<< "Parameter Name";
						checkTypeAndValue(modelMember->memberType.type, p->paramType,
						&parMember->memberValue->values[l], p->paramBuffer);
						if ( l == parMember->memberValue->nrOfValues - 1)
						break;
						pParams++;
						p = *pParams;
						if (p) {
							printf("-----> Receiving parameter 1 %s\n", p->paramName);
							i++;
						}

					}

					modelMember = modelMember->next;
					parMember = parMember->next;

					if (modelMember) {
						pParams++;
						p = *pParams; // check next parameter in structure
						if (p) {
							printf("-----> Receiving parameter 2 %s\n", p->paramName);
							i++;
						}
					}
				}
			}

			break;
		default:
			//FAIL() << "Wrong type : " << t;
			printf("\n Unsupported p->paramName:  %s with data type = %d \n\n", p->paramName, t);
		}
		if (isMultiValue) {
			k++;
			if ( k == origP->nrOfValues) {
				k = 0;
				modelParam = modelParam->next;
				j++;
			}
		}
		else {
			j++;
			modelParam = modelParam->next;
		}
		i++;
		pParams++;
		p = *pParams;

	}
	ASSERT_EQ(ep, i) << "Too few parameters";
}

//-------------------------
//  SimpleParameterVerifier
//-------------------------

class SimpleParameterVerifier: public ParameterVerifier {
public:
	SimpleParameterVerifier();
	MafMoAttributeValueContainer_3T ** getParams();
	MafOamSpiParameterT* getParameterModelList();
	~SimpleParameterVerifier();

private:
	MafOamSpiParameter* createDerivedParameter(MafOamSpiMoAttributeType_3T dataType, char* name, MafOamSpiParameter* prev);

	char *m_myString;
	char *m_myClass3gpp;
	char *m_myClassDn;
};

SimpleParameterVerifier::SimpleParameterVerifier() {
	m_myString = NULL;
	m_myClass3gpp = NULL;
	m_myClassDn = NULL;
}

SimpleParameterVerifier::~SimpleParameterVerifier() {
	if (m_myClassDn != NULL) {
		free(m_myClassDn), m_myClassDn = 0;
	}
}

MafOamSpiParameter* SimpleParameterVerifier::createDerivedParameter(MafOamSpiMoAttributeType_3T dataType, char* name, MafOamSpiParameter* prev)
{
	MafOamSpiParameter* p = createParameter (MafOamSpiDatatype_DERIVED, name, prev);
	MafOamSpiDerivedDatatype* derivedDataType = new MafOamSpiDerivedDatatype;
	derivedDataType->type = (MafOamSpiMoAttributeTypeT) dataType;
	p->parameterType.derivedDatatype = derivedDataType;
	return p;
}

// Model
MafOamSpiParameterT* SimpleParameterVerifier::getParameterModelList() {
	MafOamSpiParameter* p = 0;
	p = createParameter(MafOamSpiDatatype_INT8, "pint8", 0);
	m_parameterModel = p;
	p = createParameter(MafOamSpiDatatype_INT16, "pint16", p);
	p = createParameter(MafOamSpiDatatype_INT32, "pint32", p);
	p = createParameter(MafOamSpiDatatype_INT64, "pint64", p);
	p = createParameter(MafOamSpiDatatype_UINT8, "puint8", p);
	p = createParameter(MafOamSpiDatatype_UINT16, "puint16", p);
	p = createParameter(MafOamSpiDatatype_UINT32, "puint32", p);
	p = createParameter(MafOamSpiDatatype_UINT64, "puint64", p);
	p = createParameter(MafOamSpiDatatype_STRING, "pstring", p);
	p = createParameter(MafOamSpiDatatype_BOOL, "pboolTrue", p);
	p = createParameter(MafOamSpiDatatype_BOOL, "pboolFalse", p);
	p = createParameter(MafOamSpiDatatype_ENUM, "penum", p);
	p = createParameter(MafOamSpiDatatype_REFERENCE, "pref", p);
	p = createParameter((MafOamSpiDatatype)MafOamSpiMoAttributeType_3_DECIMAL64, "pdouble", p);
	return (MafOamSpiParameterT*) m_parameterModel;
}

MafMoAttributeValueContainer_3T** SimpleParameterVerifier::getParams() {
	m_myString = (char*) malloc(20);
	strcpy(m_myString, "A new string");

	m_myClass3gpp = (char*) malloc(30);
	strcpy(m_myClass3gpp, "Me=1,ActionClass=1");

	m_myClassDn = (char*) malloc(30);
	strcpy(m_myClassDn, "actionClassId=1,meId=1");

	MafMoAttributeValue_3 *v;
	v = new MafMoAttributeValue_3[1];
	v->value.i8 = 12;
	setParam(MafOamSpiMoAttributeType_3_INT8, v);
	v = new MafMoAttributeValue_3[1];
	v->value.i16 = 1234;
	setParam(MafOamSpiMoAttributeType_3_INT16, v);
	v = new MafMoAttributeValue_3[1];
	v->value.i32 = 232323;
	setParam(MafOamSpiMoAttributeType_3_INT32, v);
	v = new MafMoAttributeValue_3[1];
	v->value.i64 = 1, 147, 483, 647;
	setParam(MafOamSpiMoAttributeType_3_INT64, v);
	v = new MafMoAttributeValue_3[1];
	v->value.u8 = 134;
	setParam(MafOamSpiMoAttributeType_3_UINT8, v);
	v = new MafMoAttributeValue_3[1];
	v->value.u16 = 45678;
	setParam(MafOamSpiMoAttributeType_3_UINT16, v);
	v = new MafMoAttributeValue_3[1];
	v->value.u32 = 4, 147, 483, 647;
	setParam(MafOamSpiMoAttributeType_3_UINT32, v);
	v = new MafMoAttributeValue_3[1];
	v->value.u64 = 10, 223, 372, 036, 854, 775, 808;
	setParam(MafOamSpiMoAttributeType_3_UINT64, v);
	v = new MafMoAttributeValue_3[1];
	v->value.theString = m_myString;
	setParam(MafOamSpiMoAttributeType_3_STRING, v);
	v = new MafMoAttributeValue_3[1];
	memset(v, 0, sizeof(MafMoAttributeValue_3));
	v->value.theBool = true;
	setParam(MafOamSpiMoAttributeType_3_BOOL, v);
	v = new MafMoAttributeValue_3[1];
	memset(v, 0, sizeof(MafMoAttributeValue_3));
	v->value.theBool = false;
	setParam(MafOamSpiMoAttributeType_3_BOOL, v);
	v = new MafMoAttributeValue_3[1];
	memset(v, 0, sizeof(MafMoAttributeValue_3));
	v->value.theEnum = 31;
	setParam(MafOamSpiMoAttributeType_3_ENUM, v);

	MafMoAttributeValue_3 *p = new MafMoAttributeValue_3[2];
	v = p;
	v->value.theString = m_myClass3gpp;
	setParam(MafOamSpiMoAttributeType_3_REFERENCE, v);
	(++p)->value.theString = m_myClassDn;
	v = new MafMoAttributeValue_3[1];
	v->value.decimal64 = 6622.22222222;
	setParam(MafOamSpiMoAttributeType_3_DECIMAL64, v);

	m_paramList.push_back(0); // Finish with a zero
	return &m_paramList[0];
}

TEST(ActionTest, SimpleParameters)
{
	SimpleParameterVerifier parameterVerifier;

	ActionTestHelper helper;
	helper.setupMom();
	helper.setActionParameters (parameterVerifier.getParameterModelList());

	immStorageG.reset();
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE=9;
	immStorageG.addToImmStorage("actionClassId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);

	// setup the call
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	MafMoAttributeValueContainer_3T *result = NULL;
	sParameterVerifier = &parameterVerifier;
	OamSAImmBridge testObj;
	MafReturnT ret = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", parameterVerifier.getParams(), &result);
	sParameterVerifier=0;

	// Verify result
	ASSERT_EQ(1, result->nrOfValues);
	ASSERT_EQ(true, result->values->value.theBool);
	ASSERT_EQ(10, result->type);
	ASSERT_EQ(MafOk, ret) << "Wrong return value";

	//delete txContextIn;
	addMessage_mock_strings.clear();
	helper.clearMom();
}


TEST(ActionTest, NULLParameters)
{
	SimpleParameterVerifier parameterVerifier;

	ActionTestHelper helper;
	helper.setupMom();
	helper.setActionParameters (NULL);

	immStorageG.reset();
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE=9;
	immStorageG.addToImmStorage("actionClassId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);

	// setup the call
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	MafMoAttributeValueContainer_3T *result = NULL;
	sParameterVerifier = &parameterVerifier;
	OamSAImmBridge testObj;
	MafReturnT ret = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", parameterVerifier.getParams(), &result);
	sParameterVerifier=0;

	// Verify result
	//ASSERT_EQ(1, result->nrOfValues);
	//ASSERT_EQ(true, result->values->value.theBool);
	//ASSERT_EQ(10, result->type);
	ASSERT_EQ(MafInvalidArgument, ret) << "Wrong return value";

	//delete txContextIn;
	addMessage_mock_strings.clear();
	helper.clearMom();
}

//////////////////////////////////////////////////////
// Check complex parameters
//////////////////////////////////////////////////////

class StructParameterVerifier: public ParameterVerifier {
public:
	MafMoAttributeValueContainer_3T ** getParams() {
		MafMoAttributeValue_3 *v = new MafMoAttributeValue_3[1];

		MafMoAttributeValueStructMember_3 * structMember = new MafMoAttributeValueStructMember_3;
		memset(v, 0, sizeof(MafMoAttributeValue_3));
		memset(structMember, 0, sizeof(MafMoAttributeValueStructMember_3));
		v->value.structMember = structMember;
		structMember->memberName = (char*)"a";
		structMember->memberValue = new MafMoAttributeValueContainer_3;
		structMember->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
		structMember->memberValue->nrOfValues = 1;
		structMember->memberValue->values = new MafMoAttributeValue_3[1];
		structMember->memberValue->values->value.decimal64 = 6622.22222222;
		structMember->next = 0;

		setParam(MafOamSpiMoAttributeType_3_STRUCT, v);
		m_paramList.push_back(0); // Finish with a zero
		return &m_paramList[0];
	}

	MafMoAttributeValueContainer_3T ** getParams_OneParamTwoMember() {
		MafMoAttributeValue_3 *v = new MafMoAttributeValue_3[1];

		MafMoAttributeValueStructMember_3 * structMember = new MafMoAttributeValueStructMember_3;
		memset(v, 0, sizeof(MafMoAttributeValue_3));
		memset(structMember, 0, sizeof(MafMoAttributeValueStructMember_3));

		v->value.structMember = structMember;
		structMember->memberName = (char*)"a";
		structMember->memberValue = new MafMoAttributeValueContainer_3;
		structMember->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
		structMember->memberValue->nrOfValues = 1;
		structMember->memberValue->values = new MafMoAttributeValue_3[1];
		structMember->memberValue->values->value.i16 = 6622;

		MafMoAttributeValueStructMember_3 * structMemberNext = new MafMoAttributeValueStructMember_3;
		structMemberNext->memberName = (char*)"b";
		structMemberNext->memberValue = new MafMoAttributeValueContainer_3;
		structMemberNext->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
		structMemberNext->memberValue->nrOfValues = 1;
		structMemberNext->memberValue->values = new MafMoAttributeValue_3[1];
		structMemberNext->memberValue->values->value.theString = strdup("hello");
		structMemberNext->next = 0;

		structMember->next = structMemberNext;

		setParam(MafOamSpiMoAttributeType_3_STRUCT, v);
		m_paramList.push_back(0); // Finish with a zero
		return &m_paramList[0];
	}

	MafMoAttributeValueContainer_3T ** getParams_OneParamTwoMemberStruct() {
		MafMoAttributeValue_3 *v = new MafMoAttributeValue_3[1];

		MafMoAttributeValueStructMember_3 * structMember = new MafMoAttributeValueStructMember_3;
		memset(v, 0, sizeof(MafMoAttributeValue_3));
		memset(structMember, 0, sizeof(MafMoAttributeValueStructMember_3));

		v->value.structMember = structMember;
		structMember->memberName = (char*)"membsimple";
		structMember->memberValue = new MafMoAttributeValueContainer_3;
		structMember->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
		structMember->memberValue->nrOfValues = 1;
		structMember->memberValue->values = new MafMoAttributeValue_3[1];
		structMember->memberValue->values->value.i16 = 6622;

		MafMoAttributeValueStructMember_3 * structElem = new MafMoAttributeValueStructMember_3;
		memset(structElem, 0, sizeof(MafMoAttributeValueStructMember_3));
		structElem->memberName = (char*)"membstrelem";
		structElem->memberValue = new MafMoAttributeValueContainer_3;
		structElem->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
		structElem->memberValue->nrOfValues = 1;
		structElem->memberValue->values = new MafMoAttributeValue_3[1];
		structElem->memberValue->values->value.i32 = 2345;
		structElem->next = 0;

		MafMoAttributeValueStructMember_3 * structMemberNext =	new MafMoAttributeValueStructMember_3;
		memset(structMemberNext, 0, sizeof(MafMoAttributeValueStructMember_3));
		structMemberNext->memberName = (char*)"membstruct";
		structMemberNext->memberValue = new MafMoAttributeValueContainer_3;
		structMemberNext->memberValue->type = MafOamSpiMoAttributeType_3_STRUCT;
		structMemberNext->memberValue->nrOfValues = 1;
		structMemberNext->memberValue->values = new MafMoAttributeValue_3[1];
		structMemberNext->memberValue->values->value.structMember = structElem;
		structMemberNext->next = 0;

		structMember->next = structMemberNext;

		setParam(MafOamSpiMoAttributeType_3_STRUCT, v);
		m_paramList.push_back(0); // Finish with a zero
		return &m_paramList[0];
	}

	MafOamSpiParameterT* getParameterModelList() {
		MafOamSpiParameter* p = createParameter(MafOamSpiDatatype_STRUCT, "pstruct", 0);
		m_parameterModel = p;
		MafOamSpiStructMemberT* member = new MafOamSpiStructMemberT();
		memset(member, 0, sizeof(MafOamSpiStructMemberT));
		member->generalProperties.name = (char*)"a";
		member->memberType.type = MafOamSpiDatatype_INT16;

		MafOamSpiStructMemberT* memberNext = new MafOamSpiStructMemberT();
		memset(memberNext, 0, sizeof(MafOamSpiStructMemberT));
		memberNext->generalProperties.name = (char*)"b";
		memberNext->memberType.type = MafOamSpiDatatype_STRING;
		memberNext->next = 0;
		member->next = memberNext;

		MafOamSpiStruct * structDataType = new MafOamSpiStruct;
		memset(structDataType, 0, sizeof(MafOamSpiStruct));
		structDataType->members = member;
		p->parameterType.structDatatype = structDataType;
		return (MafOamSpiParameterT*) m_parameterModel;
	}

	MafOamSpiParameterT* getParameterModelList_1() {
		MafOamSpiParameter* p = createParameter(MafOamSpiDatatype_STRUCT, "pstruct", 0);
		m_parameterModel = p;
		MafOamSpiStructMemberT* member = new MafOamSpiStructMemberT();
		memset(member, 0, sizeof(MafOamSpiStructMemberT));

		member->generalProperties.name = (char*)"membsimple";
		member->memberType.type = MafOamSpiDatatype_INT16;

		MafOamSpiStructMemberT* memberNext = new MafOamSpiStructMemberT();
		memset(memberNext, 0, sizeof(MafOamSpiStructMemberT));
		memberNext->generalProperties.name = (char*)"membstruct";
		memberNext->memberType.type = MafOamSpiDatatype_STRUCT;
		memberNext->next = 0;
		member->next = memberNext;

		MafOamSpiStruct * structDataType = new MafOamSpiStruct;
		memset(structDataType, 0, sizeof(MafOamSpiStruct));
		structDataType->members = member;
		p->parameterType.structDatatype = structDataType;
		return (MafOamSpiParameterT*) m_parameterModel;
	}

	MafOamSpiParameterT* getParameterModelList_2() {
		MafOamSpiParameter* p = createParameter(MafOamSpiDatatype_STRUCT, "pstruct", 0);
		m_parameterModel = p;
		MafOamSpiStructMemberT* member = new MafOamSpiStructMemberT();
		memset(member, 0, sizeof(MafOamSpiStructMemberT));
		member->generalProperties.name = (char*)"a";
		member->memberType.type = MafOamSpiDatatype_INT16;

		MafOamSpiStructMemberT* memberNext = new MafOamSpiStructMemberT();
		memset(memberNext, 0, sizeof(MafOamSpiStructMemberT));
		memberNext->generalProperties.name = (char*)"b";
		memberNext->memberType.type = MafOamSpiDatatype_STRING;
		memberNext->next = 0;
		member->next = memberNext;

		MafOamSpiStruct * structDataType = new MafOamSpiStruct;
		memset(structDataType, 0, sizeof(MafOamSpiStruct));
		structDataType->members = member;
		p->parameterType.structDatatype = structDataType;

		p = createParameter(MafOamSpiDatatype_INT16, "pint16", p);
		return (MafOamSpiParameterT*) m_parameterModel;
	}

	MafOamSpiParameterT* getParameterModelList_3() {
		MafOamSpiParameter* p = createParameter(MafOamSpiDatatype_INT16, "pint16", 0);
		m_parameterModel = p;
		p = createParameter(MafOamSpiDatatype_STRING, "pstring", p);
		return (MafOamSpiParameterT*) m_parameterModel;
	}

	MafMoAttributeValueContainer_3T ** getParams_TwoParam() {
		MafMoAttributeValue_3 *v = new MafMoAttributeValue_3[1];

		MafMoAttributeValueStructMember_3 * structMember =
		new MafMoAttributeValueStructMember_3;
		memset(v, 0, sizeof(MafMoAttributeValue_3));
		memset(structMember, 0, sizeof(MafMoAttributeValueStructMember_3));
		v->value.structMember = structMember;
		structMember->memberName = (char*)"a";
		structMember->memberValue = new MafMoAttributeValueContainer_3;
		structMember->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
		structMember->memberValue->nrOfValues = 1;
		structMember->memberValue->values = new MafMoAttributeValue_3[1];
		structMember->memberValue->values->value.i16 = 6622;

		MafMoAttributeValueStructMember_3 * structMemberNext =	new MafMoAttributeValueStructMember_3;
		memset(structMemberNext, 0, sizeof(MafMoAttributeValueStructMember_3));
		structMemberNext->memberName = (char*)"b";
		structMemberNext->memberValue = new MafMoAttributeValueContainer_3;
		structMemberNext->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
		structMemberNext->memberValue->nrOfValues = 1;
		structMemberNext->memberValue->values = new MafMoAttributeValue_3[1];
		structMemberNext->memberValue->values->value.theString = strdup("hello");
		structMemberNext->next = 0;

		structMember->next = structMemberNext;

		setParam(MafOamSpiMoAttributeType_3_STRUCT, v);
		v = new MafMoAttributeValue_3[1];
		memset(v, 0, sizeof(MafMoAttributeValue_3));
		v->value.i16 = 1234;
		setParam(MafOamSpiMoAttributeType_3_INT16, v);

		m_paramList.push_back(0); // Finish with a zero
		return &m_paramList[0];
	}

	MafMoAttributeValueContainer_3T ** getParams_TwoParamMultiValue() {
		MafMoAttributeValue_3 *v = new MafMoAttributeValue_3[1];

		MafMoAttributeValueStructMember_3 * structMember = new MafMoAttributeValueStructMember_3;
		memset(v, 0, sizeof(MafMoAttributeValue_3));
		memset(structMember, 0, sizeof(MafMoAttributeValueStructMember_3));
		v->value.structMember = structMember;
		structMember->memberName = (char*)"a";
		structMember->memberValue = new MafMoAttributeValueContainer_3;
		structMember->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
		structMember->memberValue->nrOfValues = 2;
		structMember->memberValue->values = new MafMoAttributeValue_3[2];
		structMember->memberValue->values[0].value.i16 = 6622;
		structMember->memberValue->values[1].value.i16 = 5511;

		MafMoAttributeValueStructMember_3 * structMemberNext =	new MafMoAttributeValueStructMember_3;
		memset(structMemberNext, 0, sizeof(MafMoAttributeValueStructMember_3));
		structMemberNext->memberName = "b";
		structMemberNext->memberValue = new MafMoAttributeValueContainer_3;
		structMemberNext->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
		structMemberNext->memberValue->nrOfValues = 3;
		structMemberNext->memberValue->values = new MafMoAttributeValue_3[3];
		structMemberNext->memberValue->values[0].value.theString = strdup("hello");
		structMemberNext->memberValue->values[1].value.theString = strdup("my");
		structMemberNext->memberValue->values[2].value.theString = strdup("world");
		structMemberNext->next = 0;

		structMember->next = structMemberNext;

		setParam(MafOamSpiMoAttributeType_3_STRUCT, v);
		v = new MafMoAttributeValue_3[1];
		memset(v, 0, sizeof(MafMoAttributeValue_3));
		v->value.i16 = 1234;
		setParam(MafOamSpiMoAttributeType_3_INT16, v);

		m_paramList.push_back(0); // Finish with a zero
		return &m_paramList[0];
	}

	MafMoAttributeValueContainer_3T ** getParams_SimpleMultiValue() {
		MafMoAttributeValue_3 *values;
		values = new MafMoAttributeValue_3[3];
		values[0].value.i16 = 123;
		values[1].value.i16 = 456;
		values[2].value.i16 = 789;


		setParam(MafOamSpiMoAttributeType_3_INT16, values, 3);
		printf("m_paramList: type(%d), nrOfValues(%d)", m_paramList[0]->type, m_paramList[0]->nrOfValues);

		values = new MafMoAttributeValue_3 [2];
		values[0].value.theString = strdup("Hello");
		values[1].value.theString = strdup("world");


		setParam(MafOamSpiMoAttributeType_3_STRING, values, 2);
		printf("m_paramList: type(%d), nrOfValues(%d)", m_paramList[1]->type, m_paramList[1]->nrOfValues);

		m_paramList.push_back(0); // Finish with a zero

		return &m_paramList[0];
	}

};

void testFailure(ParameterVerifier &parameterVerifier, ActionTestHelper &helper, MafReturnT expectedReturn) {

	helper.setActionParameters(parameterVerifier.getParameterModelList());

	immStorageG.reset();
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE = 9;
	immStorageG.addToImmStorage("actionClassId=1,meId=1", "SaImmAttrClassName", IMM_STRING_TYPE, className);

	// setup the call
	MafOamSpiTransactionHandleT txh = 1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	MafMoAttributeValueContainer_3T *result = NULL;
	sParameterVerifier = &parameterVerifier;
	OamSAImmBridge testObj;
	MafReturnT actual = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", parameterVerifier.getParams(), &result);
	sParameterVerifier = 0;

	// Verify result
	ASSERT_TRUE(result->nrOfValues == 0);
	ASSERT_EQ(expectedReturn, actual) << "Wrong return value";

	//delete txContextIn;
	addMessage_mock_strings.clear();
	helper.clearMom();

}

TEST(ActionTest, UnsupportedStructParameters)
{
	ActionTestHelper helper;
	helper.setupMom();

	StructParameterVerifier parameterVerifier;

	testFailure (parameterVerifier, helper, MafValidationFailed);
}

TEST(ActionTest, ComplexParameters)
{
	immStorageG.reset();
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE = 9;
	immStorageG.addToImmStorage("actionClassId=1,meId=1", "SaImmAttrClassName",
			IMM_STRING_TYPE, className);

	ActionTestHelper helper;
	helper.setupMom();
	{
		printf("===> Test Action with a struct parameter with two members (int and string)\n");
		// pstruct.a = 6622
		// pstruct.b = hello
		StructParameterVerifier parameterVerifier;
		helper.setActionParameters(parameterVerifier.getParameterModelList());
		// setup the call
		MafOamSpiTransactionHandleT txh = 1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		sParameterVerifier = &parameterVerifier;
		OamSAImmBridge testObj;
		MafReturnT actual = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", parameterVerifier.getParams_OneParamTwoMember(), &result);
		sParameterVerifier = 0;

		// Verify result
		printf("----> Result->nrOfValues: %d\n", result->nrOfValues);
		ASSERT_TRUE(result->nrOfValues == 1);
		ASSERT_EQ(MafOk, actual) << "Wrong return value";

		//delete txContextIn;
	}

	{
		printf("===> Test Action with a struct parameter with two members (int and struct) - not supported, should fail\n");
		// pstruct.a = 6622
		// pstruct.b = struct {int32}
		StructParameterVerifier parameterVerifier;
		helper.setActionParameters(parameterVerifier.getParameterModelList_1());
		// setup the call
		MafOamSpiTransactionHandleT txh = 1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		sParameterVerifier = &parameterVerifier;
		OamSAImmBridge testObj;
		MafReturnT actual = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", parameterVerifier.getParams_OneParamTwoMemberStruct(), &result);
		sParameterVerifier = 0;

		// Verify result
		printf("----> Result->nrOfValues: %d\n", result->nrOfValues);
		ASSERT_TRUE(result->nrOfValues == 0);
		ASSERT_EQ(MafInvalidArgument, actual) << "Wrong return value";

		//delete txContextIn;
	}

	{
		printf("===> Test Action with two parameters (a struct with two members and a simple parameter)\n");
		StructParameterVerifier parameterVerifier;
		helper.setActionParameters(parameterVerifier.getParameterModelList_2());
		// setup the call
		MafOamSpiTransactionHandleT txh = 1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		sParameterVerifier = &parameterVerifier;
		OamSAImmBridge testObj;
		MafReturnT actual = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", parameterVerifier.getParams_TwoParam(), &result);
		sParameterVerifier = 0;

		// Verify result
		printf("----> Result->nrOfValues: %d\n", result->nrOfValues);
		ASSERT_TRUE(result->nrOfValues == 1);
		ASSERT_TRUE(MafOk==actual) << "Wrong return value";

		//delete txContextIn;
	}

	{
		printf("===> Test Action with a struct with two multivalue members (2 ints, 3 strings)\n");
		StructParameterVerifier parameterVerifier;
		helper.setActionParameters(parameterVerifier.getParameterModelList_2());
		// setup the call
		MafOamSpiTransactionHandleT txh = 1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		sParameterVerifier = &parameterVerifier;
		OamSAImmBridge testObj;
		MafReturnT actual = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", parameterVerifier.getParams_TwoParamMultiValue(), &result);
		sParameterVerifier = 0;

		// Verify result
		printf("----> Result->nrOfValues: %d\n", result->nrOfValues);
		ASSERT_TRUE(result->nrOfValues == 1);
		ASSERT_TRUE(MafOk==actual) << "Wrong return value";

		//delete txContextIn;
	}

	{
		printf("===> Test Action with Simple but multi-valued parameter (e.g. integer with 3 values)\n");
		StructParameterVerifier parameterVerifier;
		helper.setActionParameters(parameterVerifier.getParameterModelList_3());
		// setup the call
		MafOamSpiTransactionHandleT txh = 1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
		MafMoAttributeValueContainer_3T *result = NULL;
		sParameterVerifier = &parameterVerifier;
		OamSAImmBridge testObj;
		MafReturnT actual = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", parameterVerifier.getParams_SimpleMultiValue(), &result);
		sParameterVerifier = 0;

		// Verify result
		printf("----> Result->nrOfValues: %d\n", result->nrOfValues);
		ASSERT_TRUE(result->nrOfValues == 1);
		ASSERT_TRUE(MafOk==actual) << "Wrong return value";

		//delete txContextIn;
	}

	helper.clearMom();
}
/*
TEST(ActionTest, ReturnTypes)
{
	ActionTestHelper helper;
	helper.setupMom();
	helper.setActionReturnType(MafOamSpiDatatype_UINT32);

	SimpleParameterVerifier parameterVerifier;

	testFailure (parameterVerifier, helper, MafFailure);
}
*/

// Test that the return type is void
TEST(ActionTest, ReturnTypeVoid)
{
	ActionTestHelper helper;
	helper.setupMom();
	helper.setActionReturnType(MafOamSpiDatatype_VOID);

	immStorageG.reset();
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE=9;
	immStorageG.addToImmStorage("actionClassId=1,meId=1","SaImmAttrClassName",IMM_STRING_TYPE,className);

	// setup the call
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	MafMoAttributeValueContainer_3T *result = NULL;
	OamSAImmBridge testObj;
	MafMoAttributeValueContainer_3T *params = NULL;
	MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
	_portal->getInterface = &get_interface_mock;
	MafReturnT ret = maf_comSAMgmtSpiThreadContInit(_portal); // set _portal to testObject
	ret = testObj.action(txh, "ManagedElement=1,Me=1,ActionClass=1", "backup", &params, &result);

	// Verify result
	ASSERT_EQ(MafOk, ret);
	ASSERT_EQ(0, result->nrOfValues);
	ASSERT_EQ(10, result->type) << "Wrong return type";

	//delete txContextIn;
	helper.clearMom();
	delete _portal;
}


// class OamSAImmBridgeChild is to have access to the protected member "parseAndCategorizeErrorString"

class OamSAImmBridgeChild : public OamSAImmBridge
{
	public :
	OamSAImmBridgeChild();
	~OamSAImmBridgeChild();
	const char* parseAndCategorizeErrorStrings(const char* errorString,ThreadContextMsgCategory_2 &category);
	MafReturnT getAndForwardErrorString(TxContext* txContext, bool& immResourceAbort, bool checkImmResAbort=false);
};
// constructor
OamSAImmBridgeChild::OamSAImmBridgeChild(): OamSAImmBridge()
{

}
// destructor
OamSAImmBridgeChild::~OamSAImmBridgeChild()
{

}
MafReturnT OamSAImmBridgeChild::getAndForwardErrorString(TxContext* txContext, bool& immResourceAbort, bool checkImmResAbort)
{
	MafReturnT ret=getAndForwardErrorStrings(txContext, immResourceAbort);
	return ret;
}

const char* OamSAImmBridgeChild::parseAndCategorizeErrorStrings(const char* errorString,ThreadContextMsgCategory_2 &category)
{
	return parseAndCategorizeErrorString(errorString, category);
}

TEST(OamSAImmBridgeTest, parseAndCategorizeErrorString_no_formatting_in_front_of_the_error_string )
{
	const char * errorString="Kalle";
	string res;
	ThreadContextMsgCategory_2 category;
	MafOamSpiDerivedDatatype* derivedDataType = new MafOamSpiDerivedDatatype;
	OamSAImmBridgeChild* bridge = new OamSAImmBridgeChild();
	const char* ret=bridge->parseAndCategorizeErrorStrings(errorString, category);
	res = ret;
	EXPECT_EQ("Kalle",res);
	ASSERT_TRUE(category==ThreadContextMsgLog_2);
	delete bridge;
	delete derivedDataType;
}


TEST(OamSAImmBridgeTest, parseAndCategorizeErrorString_Empty_error_string )
{
	const char * errorString="";

	ThreadContextMsgCategory_2 category;
	MafOamSpiDerivedDatatype* derivedDataType = new MafOamSpiDerivedDatatype;
	OamSAImmBridgeChild* bridge = new OamSAImmBridgeChild();
	const char* ret=bridge->parseAndCategorizeErrorStrings(errorString, category);
	ASSERT_TRUE(ret=="");
	ASSERT_TRUE(category==ThreadContextMsgLog_2);
	delete bridge;
	delete derivedDataType;
}

TEST(OamSAImmBridgeTest, parseAndCategorizeErrorString_comlog_error_string )
{
	const char * errorString="@comlog@89";
	string res;
	ThreadContextMsgCategory_2 category;
	MafOamSpiDerivedDatatype* derivedDataType = new MafOamSpiDerivedDatatype;
	OamSAImmBridgeChild* bridge = new OamSAImmBridgeChild();
	const char* ret=bridge->parseAndCategorizeErrorStrings(errorString, category);
	res = ret;
	EXPECT_EQ("89",res);
	ASSERT_TRUE(category==ThreadContextMsgLog_2);
	delete bridge;
	delete derivedDataType;
}


TEST(OamSAImmBridgeTest, parseAndCategorizeErrorString_comnbi_error_string )
{
	const char * errorString="@comnbi@89";
	string res;
	ThreadContextMsgCategory_2 category;
	MafOamSpiDerivedDatatype* derivedDataType = new MafOamSpiDerivedDatatype;
	OamSAImmBridgeChild* bridge = new OamSAImmBridgeChild();
	const char* ret=bridge->parseAndCategorizeErrorStrings(errorString, category);
	res = ret;
	EXPECT_EQ("89",res);
	ASSERT_TRUE(category==ThreadContextMsgNbi_2);
	delete bridge;
	delete derivedDataType;
}

TEST(OamSAImmBridgeTest, parseAndCategorizeErrorString_error_error_string )
{
	const char * errorString="@comnbo@89";
	string res;
	ThreadContextMsgCategory_2 category;
	MafOamSpiDerivedDatatype* derivedDataType = new MafOamSpiDerivedDatatype;
	OamSAImmBridgeChild* bridge = new OamSAImmBridgeChild();
	const char* ret=bridge->parseAndCategorizeErrorStrings(errorString, category);
	res = ret;
	EXPECT_EQ("89",res);
	ASSERT_TRUE(category==ThreadContextMsgLog_2);
	delete bridge;
	delete derivedDataType;
}


TEST(OamSAImmBridgeTest, getAndForwardErrorStrings )
{
	// setup ModelRepository
	struct MafOamSpiMoAttribute* attrs = maf_makeAttribute((char*)"pets",MafOamSpiMoAttributeType_3_STRING,NULL);
	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", NULL);
	struct MafOamSpiMoc* homeMoc = maf_makeMoc("Home",attrs);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, homeMoc);
	struct MafOamSpiMom* mom = maf_makeMom("Me",meMoc);
	maf_setMom(mom);
	CM::ImmCmd::mVersion = { immReleaseCode, immMajorVersion, immMinorVersion };

	// setup IMM storage
	immStorageG.reset();
	std::vector<std::string> values;
	values.push_back(std::string("tiger"));
	values.push_back(std::string("lion"));
	immStorageG.addToImmStorage("homeId=2,meId=1","pets",9,values);


	// Instanciate OamSAImmBridge
	OamSAImmBridgeChild child;

	// provide the mock portal
	MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
	_portal->getInterface = &get_interface_mock;
	maf_comSAMgmtSpiThreadContInit(_portal); // set _portal to testObject

	// mock for errorCode=saImmOmCcbGetErrorStrings(txContextIn->mCcbHandle,&errorStrings);
	// txConmtextIn->mCcbHandle is of type SaImmHandleT
	MafOamSpiTransactionHandleT txh=1;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	txContextIn->mCcbHandle = 7;
	SaImmHandleT immHandle = txContextIn->mCcbHandle;

	_threadContext = new MafMgmtSpiThreadContext_2T;
	_threadContext->addMessage = &addMessage_mock;

	const SaStringT* errorStrings;
	get_interface_mock_accessed=false;
	saImmOmCcbGetErrorStrings_accessed=false;
	addMessage_mock_accessed=false;
	bool immResAbort=false;

	child.getAndForwardErrorString(txContextIn.get() , immResAbort);

	delete _portal;
	delete _threadContext;

	// if (get_interface_mock_accessed==false){ DEBUG("bool get_interface_mock_accessed=false");}
	EXPECT_TRUE(get_interface_mock_accessed==true);
	EXPECT_TRUE(saImmOmCcbGetErrorStrings_accessed==true);
	EXPECT_TRUE(addMessage_mock_accessed==true);
	EXPECT_EQ(2,addMessage_mock_strings.size());
	EXPECT_EQ("kalle",addMessage_mock_strings[0]);
	EXPECT_EQ("stina1",addMessage_mock_strings[1]);
}

TEST(OamSAImmBridgeTest, translateImmStrings)
{
	// Test to very that a IMM fragment that not is found in the IMM database is not translated
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);

		OamSATranslator theTestObject;
		std::string theImmString = "Hostname=PL-6";
		std::string objectName = "hostname=PL-6";;
		std::string theTranslatedString = theTestObject.ConvertImmNameFragmentTo3GPPNameFragment(txh, theImmString, objectName);
		printf("expected string: (%s)\n",theImmString.c_str());
		printf("received string: (%s)\n",theTranslatedString.c_str());
		ASSERT_TRUE(theTranslatedString == theImmString);
	}
	// Set up IMM
	immStorageG.reset();
	std::vector<std::string> className;
	className.push_back("ActionClass");
	unsigned int IMM_STRING_TYPE=9;
	immStorageG.addToImmStorage("namingAttribute=1","SaImmAttrClassName",IMM_STRING_TYPE,className);
	// Test to check that a class with a naming attribute that not ends with Id is translated to
	// a 3GPP format where the class name is prefixed to the key attribute.
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);

		OamSATranslator theTestObject;
		std::string theImmString = "namingAttribute=1";
		std::string objectName = "namingAttribute=1";
		std::string theTranslatedString = theTestObject.ConvertImmNameFragmentTo3GPPNameFragment(txh, theImmString, objectName);
		printf("received string: (%s)\n",theTranslatedString.c_str());
		ASSERT_TRUE(theTranslatedString == "ActionClass.namingAttribute=1");
	}

	// Set up IMM
	std::vector<std::string> className_1;
	className_1.push_back("TypeAttribute");
	immStorageG.addToImmStorage("typeAttributeId=1","SaImmAttrClassName",IMM_STRING_TYPE,className_1);
	// Test to verify that a IMM fragment ending with Id is translated according to rule one for naming attributes
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);

		OamSATranslator theTestObject;
		std::string theImmString = "typeAttributeId=1";
		std::string objectName = "typeAttributeId=1";
		std::string theTranslatedString = theTestObject.ConvertImmNameFragmentTo3GPPNameFragment(txh, theImmString, objectName);
		printf("received string: (%s)\n",theTranslatedString.c_str());
		ASSERT_TRUE(theTranslatedString == "TypeAttribute=1");
	}

	// Set up IMM
	std::vector<std::string> className_2;
	className_2.push_back("ProxyAttribute");
	immStorageG.addToImmStorage("proxyAttributeId=1","SaImmAttrClassName",IMM_STRING_TYPE,className_2);
	// Test to translate a imm fragment from the OIProxy code that ends with Id
	{
		OamSATranslator theTestObject;
		std::string theImmString = "proxyAttributeId=1";
		std::string objectName = "proxyAttributeId=1";
		std::string theTranslatedString = theTestObject.ConvertImmNameFragmentTo3GPPNameFragment(0, theImmString, objectName);
		printf("received string: (%s)\n",theTranslatedString.c_str());
		ASSERT_TRUE(theTranslatedString == "ProxyAttribute=1");
	}

	// For the OIProxy code
	// Test to check that a class with a naming attribute that not ends with Id is translated to
	// a 3GPP format where the class name is prefixed to the key attribute.
	{
		OamSATranslator theTestObject;
		std::string theImmString = "namingAttribute=1";
		std::string objectName = "namingAttribute=1";
		std::string theTranslatedString = theTestObject.ConvertImmNameFragmentTo3GPPNameFragment(0, theImmString, objectName);
		printf("received string: (%s)\n",theTranslatedString.c_str());
		ASSERT_TRUE(theTranslatedString == "ActionClass.namingAttribute=1");
	}
	// For the OIProxy code
	// Test to very that a IMM fragment that not is found in the IMM database is not translated
	{
		OamSATranslator theTestObject;
		std::string theImmString = "Hostname=PL-6";
		std::string objectName = "hostname=PL-6";;
		std::string theTranslatedString = theTestObject.ConvertImmNameFragmentTo3GPPNameFragment(0, theImmString, objectName);
		printf("expected string: (%s)\n",theImmString.c_str());
		printf("received string: (%s)\n",theTranslatedString.c_str());
		ASSERT_TRUE(theTranslatedString == theImmString);
	}
	// Test to very that a CRAZY IMM fragment is not translated, the coredump test :-)
	{
		OamSATranslator theTestObject;
		std::string theImmString = "GUCKA-MOJ_PROXY::?#";
		std::string objectName = "GUCKA-MOJ_PROXY::?#";
		throw_exception = true;
		std::string theTranslatedString = theTestObject.ConvertImmNameFragmentTo3GPPNameFragment(0, theImmString, objectName);
		printf("expected string: (%s)\n",theImmString.c_str());
		printf("received string: (%s)\n",theTranslatedString.c_str());
		ASSERT_TRUE(theTranslatedString == theImmString);
	}
}

TEST(OamSAImmBridgeTest, translate3gPPStrings)
{
	// Test to very that a IMM fragment that not is found in the IMM database is not translated
	{
		MafOamSpiTransactionHandleT txh=1;
		std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);

		OamSATranslator theTestObject;
		std::string theImmString = "Hostname=PL-6";
		std::string objectName = "hostname=PL-6";;
		std::string theTranslatedString = theTestObject.ConvertImmNameFragmentTo3GPPNameFragment(txh, theImmString, objectName);
		ASSERT_TRUE(theTranslatedString == theImmString);
	}
}

TEST(OamSAImmBridgeTest, ConvertSimpleToMoAttribute)
{
	MafOamSpiTransactionHandleT txh=1;
	MafReturnT ret;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);

	OamSATranslator theTestObject;
	{
	MafMoAttributeValueContainer_3T **theComParameterList_pp;
	SaImmAdminOperationParamsT_2	***theAdminOpParams_ppp;


	MafOamSpiMoActionT				*theAction_p = NULL;
	ret = theTestObject.ConvertToAdminOpParameters(theAction_p,theComParameterList_pp, theAdminOpParams_ppp);
	EXPECT_EQ(ret, MafInvalidArgument);

	}
	{
		MafOamSpiDatatypeContainerT		*returnType_p = NULL;
		SaImmAdminOperationParamsT_2	*theAdminOpParams_pp[2];
		theAdminOpParams_pp[0]= new SaImmAdminOperationParamsT_2 ;
		theAdminOpParams_pp[1] = NULL;
		MafMoAttributeValueContainer_3T *theResult_pp = new MafMoAttributeValueContainer_3T;
		memset(theResult_pp, 0, sizeof(MafMoAttributeValueContainer_3T));
		char * errorText = new char[255];

		ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
		EXPECT_EQ(ret, MafInvalidArgument);
		printf("errorText: %s\n\n", errorText);
		freeAttributeContainerContent(theResult_pp, true);
		delete theAdminOpParams_pp[0];
		delete []errorText;
	}
	MafOamSpiDatatypeContainerT		*returnType_p = new MafOamSpiDatatypeContainerT;
	SaImmAdminOperationParamsT_2	*theAdminOpParams_pp[2];
	theAdminOpParams_pp[0]= new SaImmAdminOperationParamsT_2 ;
	theAdminOpParams_pp[1] = NULL;
	MafMoAttributeValueContainer_3T *theResult_pp = new MafMoAttributeValueContainer_3T;
	memset(theResult_pp, 0, sizeof(MafMoAttributeValueContainer_3T));
	char * errorText = new char[255];

	//1st case
	returnType_p->multiplicity.min = 0;
	returnType_p->multiplicity.max = 5;
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeInt8_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;
	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//2nd case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeInt16_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//3rd case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeInt64_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//4th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeUint8_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//5th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeUint16_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//6th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeUint32_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//7th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeUint64_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//8th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeEnum_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//9th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeBool_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//10th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeString_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//11th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeReference_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//12th case for MafOamSpiDatatype_VOID. Non-support.

	//13th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeDouble_4;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//14th case
	returnType_p->type = (MafOamSpiDatatypeT)20;

	theAdminOpParams_pp[0]->paramName = "do_something";
	theAdminOpParams_pp[0]->paramType = SA_IMM_ATTR_SAUINT32T;
	theAdminOpParams_pp[0]->paramBuffer = (SaImmAttrValueT)8;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh,returnType_p, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//15th case
	MafOamSpiDatatypeContainerT		*returnType_p2 = NULL;
	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p2, &theAdminOpParams_pp[0] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	//16th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeUint16_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_UINT16);
	freeAttributeContainerContent(theResult_pp, false);

	//17th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeInt8_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_INT8);
	freeAttributeContainerContent(theResult_pp, false);

	//18th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeInt16_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_INT16);
	freeAttributeContainerContent(theResult_pp, false);

	//19th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeInt32_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_INT32);
	freeAttributeContainerContent(theResult_pp, false);

	//20th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeInt64_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_INT64);
	freeAttributeContainerContent(theResult_pp, false);

	//21th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeUint8_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_UINT8);

	freeAttributeContainerContent(theResult_pp, false);

	//22th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeUint32_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_UINT32);

	freeAttributeContainerContent(theResult_pp, false);

	//23th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeUint64_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_UINT64);

	freeAttributeContainerContent(theResult_pp, false);

	//24th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeEnum_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_INT32);

	freeAttributeContainerContent(theResult_pp, false);

	//25th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeBool_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_BOOL);

	freeAttributeContainerContent(theResult_pp, false);

	//26th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeReference_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_REFERENCE);
	freeAttributeContainerContent(theResult_pp, false);

	//26th case for MafOamSpiDatatype_VOID. Non-support.

	//25th case
	returnType_p->type = (MafOamSpiDatatypeT)MafOamSpiMrTypeDouble_4;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafOk);
	EXPECT_EQ(theResult_pp->nrOfValues, 0);
	ASSERT_TRUE(theResult_pp->values == NULL);
	EXPECT_EQ(theResult_pp->type, MafOamSpiMoAttributeType_3_DECIMAL64);

	freeAttributeContainerContent(theResult_pp, false);

	//26th case
	returnType_p->type = (MafOamSpiDatatypeT)20;

	ret = theTestObject.ConvertSimpleToMoAttribute(txh, returnType_p, &theAdminOpParams_pp[1] , &theResult_pp,errorText);
	EXPECT_EQ(ret, MafInvalidArgument);
	printf("errorText: %s\n\n", errorText);
	freeAttributeContainerContent(theResult_pp, false);

	delete returnType_p;
	delete []errorText;
	delete theAdminOpParams_pp[0];
	delete theResult_pp;
}



/* *******************************************************************************
*  Tests of the "isNotifiable" data structure (created for CM notifications)
*/

// Test of: isNotifiable-data-structure
// Basic add-get test case
TEST (isNotifiableStorageTest, Test1)
{
	bool res;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass","testAttr");
	EXPECT_EQ(res, 1);
}

// Test of: isNotifiable-data-structure
// Multiple add-get test case
TEST (isNotifiableStorageTest, Test2)
{
	bool res;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("abc","cba");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.addAttributeToIsNotifiableMap("theClass","theAttribute");
	momKeyRepo.addAttributeToIsNotifiableMap("crazyClass","crazyAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass","testAttr");
	EXPECT_EQ(res, 1);
}

// Test of: isNotifiable-data-structure
// Multiple add-get test case
TEST (isNotifiableStorageTest, Test3)
{
	bool res;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("abc","cba");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.addAttributeToIsNotifiableMap("theClass","theAttribute");
	momKeyRepo.addAttributeToIsNotifiableMap("crazyClass","crazyAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass","cba");
	EXPECT_EQ(res, 0);
}

// Test of: isNotifiable-data-structure
// Expect false when class name is not provided
TEST (isNotifiableStorageTest, Test4)
{
	bool res;
	char *empty = NULL;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable(empty,"cba");
	EXPECT_EQ(res, 0);
}

// Test of: isNotifiable-data-structure
// Test with no attribute specified
TEST (isNotifiableStorageTest, Test5)
{
	bool res;
	char *empty = NULL;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass",empty);
	EXPECT_EQ(res, 1);
}

// Test of: isNotifiable-data-structure
// Test with unexisting class specified
TEST (isNotifiableStorageTest, Test6)
{
	bool res;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass2","testAttr");
	EXPECT_EQ(res, 0);
}

// Test of: isNotifiable-data-structure
// Test with no attribute specified
TEST (isNotifiableStorageTest, Test7)
{
	bool res;
	char *empty = NULL;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass");
	EXPECT_EQ(res, 1);
}

// Test of: isNotifiable-data-structure
// Expect false when neither class name nor attribute name is provided
TEST (isNotifiableStorageTest, Test8)
{
	bool res;
	char *empty = NULL;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable(empty,empty);
	EXPECT_EQ(res, 0);
}

// Test of: isNotifiable-data-structure
// Add multiple attributes to same class. Search for existing attribute
TEST (isNotifiableStorageTest, Test9)
{
	bool res;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","cba");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","theAttribute");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","crazyAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass","theAttribute");
	EXPECT_EQ(res, 1);
}

// Test of: isNotifiable-data-structure
// Add multiple attributes to same class. Search for non-existing attribute
TEST (isNotifiableStorageTest, Test10)
{
	bool res;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","cba");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","theAttribute");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","crazyAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass","theAttribute0");
	EXPECT_EQ(res, 0);
}

// Test of: isNotifiable-data-structure
// Add multiple classes and multiple attributes. Search for non-existing attribute
TEST (isNotifiableStorageTest, Test11)
{
	bool res;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","cba");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","theAttribute");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass2","cba");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass2","testAttr");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass2","theAttribute");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass2","crazyAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass","crazyAttr");
	EXPECT_EQ(res, 0);
}


// Test of: isNotifiable-data-structure
// Add multiple classes and multiple attributes. Search for existing attribute
TEST (isNotifiableStorageTest, Test12)
{
	bool res;
	OamSAKeyMOMRepo momKeyRepo;
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","cba");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","testAttr");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass","theAttribute");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass2","cba");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass2","testAttr");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass2","theAttribute");
	momKeyRepo.addAttributeToIsNotifiableMap("testClass2","crazyAttr");
	momKeyRepo.printClassAttributeNotifiableMap();
	res = momKeyRepo.isNotifiable("testClass2","testAttr");
	EXPECT_EQ(res, 1);
}

// *****************************************************************************

// Test of: isNotify for classes and attributes under classes
// No matching MOM
TEST (isNotify_with_StorageTest, Test1)
{
	OamSATranslator	theTranslator;
	bool res = theTranslator.isNotified("CmwPmpmId=1","CmwPmPm","thresholdJobSupport");
	EXPECT_EQ(res, 0);
}

// Test isNotify
// decorated : yes
// root class: yes
// Note: the searchKey must be different then the previous testcases
TEST (isNotify_with_StorageTest, Test2)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSATranslator	theTranslator;

	std::string classNameParent = "SystemFunctions2";
	std::string classNameRoot = "Pm";
	std::string keyAttributeRoot = "CmwPmpmId";

	char *momName = "CmwPm";
	bool decorated = true;

	std::string notifiedClass = "Pm";
	std::string notifiedAttr = "thresholdJobSupport";

	const char *test_immRdn = "CmwPmpmId=1";
	const char *test_immClassName = "CmwPmPm";
	const char *test_attrName = "thresholdJobSupport";

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(momName);
	if(decorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);
	momKeyRepo->addAttributeToIsNotifiableMap(notifiedClass,notifiedAttr);

	// STEP 2: Test "isNotified" function
	momKeyRepo->printClassAttributeNotifiableMap();
	res = theTranslator.isNotified(test_immRdn, test_immClassName, test_attrName);
	EXPECT_EQ(res, 1);
}

// Test isNotify
// decorated : no
// root class: yes
// Note: the searchKey must be different then the previous testcases
TEST (isNotify_with_StorageTest, Test3)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSATranslator	theTranslator;

	std::string classNameParent = "SystemFunctions3";
	std::string classNameRoot = "Pm";
	std::string keyAttributeRoot = "PmpmId";

	char *momName = "CmwPm";
	bool decorated = false;

	std::string notifiedClass = "Pm";
	std::string notifiedAttr = "thresholdJobSupport";

	const char *test_immRdn = "PmpmId=1";
	const char *test_immClassName = "Pm";
	const char *test_attrName = "thresholdJobSupport";

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(momName);
	if(decorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);
	momKeyRepo->addAttributeToIsNotifiableMap(notifiedClass,notifiedAttr);

	// STEP 2: Test "isNotified" function
	momKeyRepo->printClassAttributeNotifiableMap();
	res = theTranslator.isNotified(test_immRdn, test_immClassName, test_attrName);
	EXPECT_EQ(res, 1);
}

// Test isNotify
// decorated : no
// root class: no
// Note: the searchKey must be different then the previous testcases
TEST (isNotify_with_StorageTest, Test4)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSATranslator	theTranslator;

	std::string classNameParent = "SystemFunctions4";
	std::string classNameRoot = "Pm";
	std::string keyAttributeRoot = "PmpmId";

	char *momName = "CmwPm";
	bool decorated = false;

	std::string notifiedClass = "TestClass";
	std::string notifiedAttr = "TestAttr";

	const char *test_immRdn = "testClassId=5,PmpmId=1";
	const char *test_immClassName = "TestClass";
	const char *test_attrName = "TestAttr";

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(momName);
	if(decorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);
	momKeyRepo->addAttributeToIsNotifiableMap(notifiedClass,notifiedAttr);

	// STEP 2: Test "isNotified" function
	momKeyRepo->printClassAttributeNotifiableMap();
	res = theTranslator.isNotified(test_immRdn, test_immClassName, test_attrName);
	EXPECT_EQ(res, 1);
}

// Test isNotify
// decorated : yes
// root class: no
// Note: the searchKey must be different then the previous testcases
TEST (isNotify_with_StorageTest, Test5)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSATranslator	theTranslator;

	std::string classNameParent = "SystemFunctions5";
	std::string classNameRoot = "Pm";
	std::string keyAttributeRoot = "PmpmId";

	char *momName = "CmwPm";
	bool decorated = true;

	std::string notifiedClass = "TestClass";
	std::string notifiedAttr = "TestAttr";

	const char *test_immRdn = "testClassId=5,PmpmId=1";
	const char *test_immClassName = "CmwPmTestClass";
	const char *test_attrName = "TestAttr";

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(momName);
	if(decorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);
	momKeyRepo->addAttributeToIsNotifiableMap(notifiedClass,notifiedAttr);

	// STEP 2: Test "isNotified" function
	momKeyRepo->printClassAttributeNotifiableMap();
	res = theTranslator.isNotified(test_immRdn, test_immClassName, test_attrName);
	EXPECT_EQ(res, 1);
}

// Test isNotify
// decorated : yes
// root class: no
// Note: the searchKey must be different then the previous testcases
TEST (isNotify_with_StorageTest, Test6)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSATranslator	theTranslator;

	std::string classNameParent = "SystemFunctions6";
	std::string classNameRoot = "Pm";
	std::string keyAttributeRoot = "PmpmId";

	char *momName = "CmwPm";
	bool decorated = true;

	std::string notifiedClass = "TestClass";
	std::string notifiedAttr = "TestAttr_7623562738";

	const char *test_immRdn = "testClassId=5,PmpmId=1";
	const char *test_immClassName = "TestClass";
	const char *test_attrName = "TestAttr";

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(momName);
	if(decorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);
	momKeyRepo->addAttributeToIsNotifiableMap(notifiedClass,notifiedAttr);

	// STEP 2: Test "isNotified" function
	momKeyRepo->printClassAttributeNotifiableMap();
	res = theTranslator.isNotified(test_immRdn, test_immClassName, test_attrName);
	EXPECT_EQ(res, 0);
}

// Test isNotify
// decorated : yes
// root class: no
// Note: the searchKey must be different then the previous testcases
TEST (isNotify_with_StorageTest, Test7)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSATranslator	theTranslator;

	std::string classNameParent = "SystemFunctions7";
	std::string classNameRoot = "Pm";
	std::string keyAttributeRoot = "PmpmId";

	char *momName = "CmwPm";
	bool decorated = true;

	std::string notifiedClass = "TestClass";
	std::string notifiedAttr = "TestAttr_7623562738";

	const char *test_immRdn = "testClassId=5,PmpmId=1";
	const char *test_immClassName = "CmwPmTestClass";
	const char *test_attrName = NULL;

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(momName);
	if(decorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);
	momKeyRepo->addAttributeToIsNotifiableMap(notifiedClass,notifiedAttr);

	// STEP 2: Test "isNotified" function
	momKeyRepo->printClassAttributeNotifiableMap();
	res = theTranslator.isNotified(test_immRdn, test_immClassName, test_attrName);
	EXPECT_EQ(res, 1);
}

// Test isNotify
// decorated : no
// root class: no
// Note: the searchKey must be different then the previous testcases
TEST (isNotify_with_StorageTest, Test8)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSATranslator	theTranslator;

	std::string classNameParent = "SystemFunctions8";
	std::string classNameRoot = "Pm";
	std::string keyAttributeRoot = "PmpmId";

	char *momName = "CmwPm";
	bool decorated = false;

	std::string notifiedClass = "FunnyCl";
	std::string notifiedAttr = "FunnyAttr";

	const char *test_immRdn = "funnyClId=13,testClassId=5,PmpmId=1";
	const char *test_immClassName = "FunnyCl";
	const char *test_attrName = "FunnyAttr";

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(momName);
	if(decorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);
	momKeyRepo->addAttributeToIsNotifiableMap(notifiedClass,notifiedAttr);

	// STEP 2: Test "isNotified" function
	momKeyRepo->printClassAttributeNotifiableMap();
	res = theTranslator.isNotified(test_immRdn, test_immClassName, test_attrName);
	EXPECT_EQ(res, 1);
	theTranslator.ResetMOMRoot();
}

// Test isNotify
// decorated : yes
// root class: no
// Note: the searchKey must be different then the previous testcases
TEST (isNotify_with_StorageTest, Test9)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSATranslator	theTranslator;

	std::string classNameParent = "SystemFunctions9";
	std::string classNameRoot = "Pm";
	std::string keyAttributeRoot = "CmwPmpmId";

	char *momName = "CmwPm";
	bool decorated = true;

	std::string notifiedClass = "FunnyCl";
	std::string notifiedAttr = "FunnyAttr";

	const char *test_immRdn = "funnyClId=1,testClassId=5,CmwPmpmId=1";
	const char *test_immClassName = "CmwPmFunnyCl";
	const char *test_attrName = "FunnyAttr";

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(momName);
	if(decorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);
	momKeyRepo->addAttributeToIsNotifiableMap(notifiedClass,notifiedAttr);

	// STEP 2: Test "isNotified" function
	momKeyRepo->printClassAttributeNotifiableMap();
	res = theTranslator.isNotified(test_immRdn, test_immClassName, test_attrName);
	EXPECT_EQ(res, 1);
}
// Test destructor of Key Repo class
TEST (StorageTest, Test10)
{
	// STEP 1: Set up input data for the storage
	bool res;
	OamSAKeyMOMRepo *pointerToObject = new OamSAKeyMOMRepo();

	pointerToObject->setRootClass("FirstRootClass");
	//pointerToObject->setKeyAttributeForClass("FirstRootClass", "keyFirstRootClass");
	pointerToObject->setKeyAttributeForClass("Class1", "keyAttribute1");
	pointerToObject->setKeyAttributeForClass("Class2", "keyAttribute2");
	pointerToObject->setKeyAttributeForClass("Class3", "keyAttribute3");
	pointerToObject->setDecoratedKeyAttributeForClass("FirstRootClass", "Tjooho");
	delete pointerToObject;
	// Destroy the translator object

}

// Test case for testing function "OamSACache::GetClassNameFromImm()"
// when saImmOmAccessorGet_2() returns NULL for attributes
TEST (OamSACacheUtilityFunction, Test1)
{
	bool result = false;
	unsigned int saImmOmAccessorGet_2_testcase = 1;
	result = GetClassNameFromImm_tester_function(saImmOmAccessorGet_2_testcase);
	EXPECT_EQ(result, 1);
}

// Test case for testing function "OamSACache::GetClassNameFromImm()"
// when saImmOmAccessorGet_2() returns attributes with empty list (a list which has 1 element and that is NULL)
TEST (OamSACacheUtilityFunction, Test2)
{
	bool result = false;
	unsigned int saImmOmAccessorGet_2_testcase = 2;
	result = GetClassNameFromImm_tester_function(saImmOmAccessorGet_2_testcase);
	EXPECT_EQ(result, 1);
}

// Test case for testing function "OamSACache::GetClassNameFromImm()"
// when saImmOmAccessorGet_2() returns an attribute which has attrValuesNumber=0
TEST (OamSACacheUtilityFunction, Test3)
{
	bool result = false;
	unsigned int saImmOmAccessorGet_2_testcase = 3;
	result = GetClassNameFromImm_tester_function(saImmOmAccessorGet_2_testcase);
	EXPECT_EQ(result, 1);
}

// Test case for method OamSATranslator::GetClassName( ... )
// Test different types of input imm RDN
/**
* The test cases are:
*
* Test1 calls the GetClassName() for a undecorated MOM with the following IMM RDN's
* 		- meId=1             -----> className=Me,ComClassName=Me
* 		- homeId=2,meId=1    -----> className=Home,ComClassName=Home
* Test2 calls the GetClassName for a decorated MOM with the following IMM RDN's
* 		- DECORATEDmeId=1
* 		- homeId=2,DECORATEDmeId=1
*/
TEST (OamSATranslatorGetClassName, Test1)
{
	bool result = false;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup ModelRepository
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"homeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", key);
	struct MafOamSpiMoc* homeMoc = maf_makeMoc("Home",key2);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, homeMoc);
	char *MOMNAME = "MEMOM";
	struct MafOamSpiMom* mom = maf_makeMom(MOMNAME,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);
	// To be able to add a class to the MOMKeyRepo that is not a key class, the MOM must be linked to
	// The homeMoc, do this manually here.
	homeMoc->mom = mom;

	const char* immRdn = "meId=1";
	std::string className;
	std::string comclassName;
	// OK, need to trigger the read of the model repository, do this by a fake question
	theTranslator.IsClassNamePresent("READ_MODEL_REPO");
	theTranslator.GetClassName(immRdn, className, comclassName);

	ASSERT_TRUE(className == "Me");
	ASSERT_TRUE(comclassName == "Me");

	const char* immRdn2 = "homeId=2,meId=1";
	className.clear();
	comclassName.clear();
	theTranslator.GetClassName(immRdn2, className, comclassName);

	ASSERT_TRUE(className == "Home");
	ASSERT_TRUE(comclassName == "Home");

	const char* immRdn3 = "id=r_1,homeId=2,meId=1";
	className.clear();
	comclassName.clear();
	theTranslator.GetClassName(immRdn2, className, comclassName);

	ASSERT_TRUE(className == "Home");
	ASSERT_TRUE(comclassName == "Home");
}

TEST (OamSATranslatorGetClassName, Test2)
{
	bool result = false;

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	std::string classNameParent = "ManagedElement";
	std::string classNameRoot = "Me";
	std::string keyAttributeRoot = "MEMOMmeId";


	// setup ModelRepository
	// Add the key attribute
	struct MafOamSpiMoAttribute* key = maf_makeAttribute((char*)"meId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key->isKey = true;
	// Add the key attribute
	struct MafOamSpiMoAttribute* key2 = maf_makeAttribute((char*)"homeId", MafOamSpiMoAttributeType_3_STRING, NULL);
	key2->isKey = true;

	struct MafOamSpiMoc* meMoc = maf_makeMoc("Me", key);
	struct MafOamSpiMoc* homeMoc = maf_makeMoc("Home",key2);
	struct MafOamSpiContainment* cont = maf_makeContainment(meMoc, homeMoc);
	char *MOMNAME = "MEMOM";
	struct MafOamSpiMom* mom = maf_makeMom(MOMNAME,meMoc);
	maf_setMom(mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(meMoc);
	// To be able to add a class to the MOMKeyRepo that is not a key class, the MOM must be linked to
	// The homeMoc, do this manually here.
	homeMoc->mom = mom;
	isDecorated = true;

	// setup the call
	MOMRootRepository testObj;
	testObj.Populate();

	std::string searchKey = classNameParent + "_" + classNameRoot;
	theKeyHolder.setMOMRepo(searchKey);
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);
	momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);

	momKeyRepo->setMomName(MOMNAME);
	if(isDecorated) {momKeyRepo->setMomIsDecoratedFlag();}
	momKeyRepo->setRootClass(classNameRoot);

	const char* immRdn = "MEMOMmeId=1";
	std::string className;
	std::string comclassName;
	// OK, need to trigger the read of the model repository, do this by a fake question
	theTranslator.IsClassNamePresent("READ_MODEL_REPO");
	theTranslator.GetClassName(immRdn, className, comclassName);

	EXPECT_EQ(className, "MEMOMMe");
	ASSERT_TRUE(className == "MEMOMMe");
	EXPECT_EQ(comclassName, "Me");
	ASSERT_TRUE(comclassName == "Me");

	const char* immRdn2 = "homeId=2,MEMOMmeId=1";
	className.clear();
	comclassName.clear();
	theTranslator.GetClassName(immRdn2, className, comclassName);
	ASSERT_TRUE(className == "MEMOMHome");
	ASSERT_TRUE(comclassName == "Home");

	isDecorated = false;
}


/* MR20275 - Validate Command Support */


MafReturnT OamSAPrepareTest(MafOamSpiTransactionHandleT txHandle)
{
	if (validate_TC_number == 2)
	{
		return MafNotExist;
	}
	else if(saAisErr == SA_AIS_ERR_FAILED_OPERATION)
	{
		switch(validate_TC_number) {
		case 9:
			return MafFailure;
			break;
		case 10:
			return MafCommitFailed;
			break;
		default:
			return MafOk;
		}
	}
	else
	{
		return MafOk;
	}
}

TEST(OamSAImmBridge, Validate_Command)
{

	MafOamSpiTransactionHandleT txh=1;
	MafReturnT ret;
	bool result;
	OamSAImmBridge testObject;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);

	// provide the mock portal
	MafMgmtSpiInterfacePortal_3T* _portal = new MafMgmtSpiInterfacePortal_3T;
	_portal->getInterface = &get_interface_mock;
	ret = maf_comSAMgmtSpiThreadContInit(_portal); // set _portal to testObject

	_threadContext = new MafMgmtSpiThreadContext_2T;
	_threadContext->addMessage = &addMessage_mock;

	//TC2: Spi version is 2
	printf("\nVALIDATE TC2: Tx SPI version is 2, Fail to prepare CCB\n");
	validate_TC_number = 2;
	ret = testObject.OamSAValidate(txh, &result);
	ASSERT_EQ(ret, MafNotExist);

	//TC3: Spi version is 2
	printf("\nVALIDATE TC3: Tx SPI version is 2, Fail to validate CCB\n");
	validate_TC_number = 3;
	txh = 1;
	ret = testObject.OamSAValidate(txh, &result);
	ASSERT_EQ(ret, MafOk);
	ASSERT_EQ(result, false);

	//TC4: Spi version is 2
	printf("\nVALIDATE TC4: Tx SPI version is 2, Fail to abort CCB\n");
	validate_TC_number = 4;
	txh = 1;
	ret = testObject.OamSAValidate(txh, &result);
	ASSERT_EQ(ret, MafFailure);
	ASSERT_EQ(result, true);

	//TC5: Spi version is 2
	printf("\nVALIDATE TC5: Tx SPI version is 2, Success case\n");
	validate_TC_number = 5;
	txh = 1;
	ret = testObject.OamSAValidate(txh, &result);
	ASSERT_EQ(ret, MafOk);
	ASSERT_EQ(result, true);

	//TC6: Spi version is 2
	printf("\nVALIDATE TC6: Tx SPI version is 2, Null IMM Handle\n");
	validate_TC_number = 6;
	txh = 0;
	ret = testObject.OamSAValidate(txh, &result);
	ASSERT_EQ(ret, MafNotExist);

	printf("\nVALIDATE TC7: Tx SPI version is 2, Fail to validate CCB\n");
	txh = 1;
	ccbValidateFailed = true;
	ret = testObject.OamSAValidate(txh, &result);
	ASSERT_EQ(ret, MafOk);
	ASSERT_EQ(result, false);
	saAisErr = SA_AIS_OK;

	printf("\nVALIDATE TC8: Tx SPI version is 2, retry ccbValidate on IMM Resource abort\n");
	txh = 1;
	immResourceAbort = true;
	ret = testObject.OamSAValidate(txh, &result);
	//Now ImmCcbCreatedstatus got updated with "ImmCcbNotCreated" which means ccb is empty
	ASSERT_EQ(ret, MafOk);
	ASSERT_EQ(result, true);

	printf("\nVALIDATE TC9: Tx SPI version is 2, Failed to prepare CCB while retrying "
			"ccbValidate on IMM resource abort\n");
	validate_TC_number = 9;
	txh = 1;
	//set the ImmCcbCreatedstatus to "ImmCcbCreated" as the ccb is empty because of TC8
	txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->newTxContext(txh);
	txContextIn->setImmCcbCreatedStatus(ImmCcbCreated);
	ret = testObject.OamSAValidate(txh, &result);
	ASSERT_EQ(ret, MafFailure);
	ASSERT_EQ(result, false);
	saAisErr = SA_AIS_OK;

	printf("\nVALIDATE TC10: Tx SPI version is 2, Failed to prepare CCB due to exceeding "
			"IMM_ABORT_RETRIES_COUNT on MafCommitFailed\n");
	validate_TC_number = 10;
	txh = 1;
	ret = testObject.OamSAValidate(txh, &result);
	ASSERT_EQ(ret, MafFailure);
	ASSERT_EQ(result, false);

	delete _portal;
	delete _threadContext;
}

TEST(ImmTest, writeSecretsToFile)
{
	testDomainName = "";
	testExtensionName = "";
	testExtensionValue = "";

	// Clear the Model Repository
	extern OamSATranslator theTranslator;
	theTranslator.ResetMOMRoot();
	// setup IMM storage
	immStorageG.reset();
	// setup ModelRepository

	//1st MOM

	struct MafOamSpiStructMember* myClearText = maf_makeStructMember((char*)"cleartext", MafOamSpiDatatype_STRING,NULL);
	struct MafOamSpiStruct* secretData = maf_makeStruct((char*)"EcimPassword", myClearText);

	// Add the key attribute
	struct MafOamSpiMoAttribute* c1key = maf_makeAttribute((char*)"c1AId", MafOamSpiMoAttributeType_3_STRING, NULL);
	c1key->isKey = true;

	struct MafOamSpiMoAttribute* c1attr1 = maf_makeAttribute((char*)"passPhraseAttr1", MafOamSpiMoAttributeType_3_STRING, c1key);

	struct MafOamSpiMoAttribute* c1attr2 = maf_makeAttribute((char*)"secretInfo",secretData, c1attr1);
	c1attr2->isKey=false;
	struct MafOamSpiMoAttribute* c1attr3 = maf_makeAttribute((char*)"passPhraseAttr2", MafOamSpiMoAttributeType_3_STRING, c1attr2);

	struct MafOamSpiMoAttribute* c1attr4 = maf_makeAttribute((char*)"userLabel", MafOamSpiMoAttributeType_3_STRING, c1attr3);

	struct MafOamSpiMoc* c1AMoc = maf_makeMoc("C1A", c1attr4);

	struct MafOamSpiMom* C1mom = maf_makeMom("C1Mom",c1AMoc);

	maf_setMom(C1mom);
	// Set the parent Containment to ManagedElement
	maf_makeParentContainmentToTop(c1AMoc);

	// setup the call
	MOMRootRepository testObj;
	MafOamSpiMrMocHandle_4T mocHandle;
	mocHandle.handle = (uint64_t)c1AMoc;

	testObj.mapMocHandler.insert(std::pair< std::string, MafOamSpiMrMocHandle_4T>( "managedElement,c1AMoc", mocHandle));

	testObj.writeSecretsToFile();

	int count = 0;
	std::ifstream fileS;
	fileS.open (REENCRYPTOR_MODEL_DATA_FILE_PATH);
	if (fileS.is_open()) {

		std::string tmpLine;
		while (fileS.good()) {

			std::getline(fileS, tmpLine);
			printf ("Content: [%s]\n", tmpLine.c_str());
			if ((0 == strcmp(tmpLine.c_str(), "C1A,EcimPassword;password")) || (0 == strcmp(tmpLine.c_str(), "C1A;passPhraseAttr2,passPhraseAttr1")))
			{
				count++;
			}
		}
		fileS.close();
	}
	EXPECT_EQ(2, count);

	remove(REENCRYPTOR_MODEL_DATA_FILE_PATH);
	testObj.mapMocHandler.clear();
	testObj.mapMocpathToAttributes.clear();
}
