/**
* stubs and tests for OamSAOiProxy
*
*   Modify:   xadaleg  2014-08-02:  MR35347 - increase DN length
*/
#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE

#include <gtest/gtest.h>
#include <stdarg.h>
// COM
#include "MafMgmtSpiInterfacePortal_1.h"
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiComponent_1.h"
#include "MafMgmtSpiThreadContext_2.h"
#include "MafMgmtSpiServiceIdentities_1.h"
#include "MafMgmtSpiThreadContext_2.h"
#include "MafOamSpiServiceIdentities_1.h"
#include "MafOamSpiTransactionMaster_2.h"
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiModelRepository_4.h"
#include "MafOamSpiManagedObject_3.h"
// COM SA
#include "OamSAOIProxy.h"
#include "OamSACache_dummy.h"
#include "OamSATranslator_dummy.h"
#include "ImmCmd.h"

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

////////////////////////////////////3//////////////////
// LOG
//////////////////////////////////////////////////////
#ifdef REDIRECT_LOG
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

MafMgmtSpiThreadContext_2T* threadContext;
MafOamSpiTransactionMaster_2T* transactionMaster;

bool threadContext_interface_accessed=false;
bool transactionMaster_interface_accessed=false;

bool returnMafOk_threadContext_interface=true;
bool returnMafOk_transactionMaster_interface=true;

extern bool MafOamSpiTransactionMaster_1_available, MafOamSpiTransactionMaster_2_available;
extern bool MafOamSpiTransactionMaster_1_selected, MafOamSpiTransactionMaster_2_selected;
extern bool saImmOiInitialize_2_return_OK, saImmOiInitialize_2_accessed, saImmOiSelectionObjectGet_return_OK, saImmOmInitialize_return_OK;
extern bool saImmOiImplementerSet_accessed, saImmOiImplementerSet_return_OK, saImmOiSelectionObjectGet_accessed, saImmOmInitialize_accessed;

//Interface names
std::string threadContextId = MafMgmtSpiThreadContext_2Id.interfaceName;
std::string transactionMasterId = MafOamSpiTransactionMaster_2Id.interfaceName;

// added for mock portal
MafReturnT get_interface_mock(MafMgmtSpiInterface_1T id, MafMgmtSpiInterface_1T** result)
{
	// DEBUG("id.interfaceName:%s\n",id.interfaceName);
	if (threadContextId == id.interfaceName)
	{
		*result = (MafMgmtSpiInterface_1T*)threadContext;
		DEBUG("OamSAOiProxy_unittest::get_interface_moc (threadContextId == id.interfaceName)\n");
		threadContext_interface_accessed=true;
		if (returnMafOk_threadContext_interface==true) return MafOk;
		return MafFailure;
	}
	if (transactionMasterId == id.interfaceName)
	{
		*result = (MafMgmtSpiInterface_1T*)transactionMaster;
		DEBUG("OamSAOiProxy_unittest::get_interface_moc (transactionMasterId == id.interfaceName)\n");
		transactionMaster_interface_accessed=true;
		if (returnMafOk_transactionMaster_interface==true) return MafOk;
		return MafFailure;
	}
	return MafFailure;
} // get_interface_mock


bool saImmOiInitialize_2_return_OK = true;
bool saImmOiInitialize_2_accessed = false;
bool saImmOiSelectionObjectGet_return_OK=true;
bool saImmOiSelectionObjectGet_accessed=false;
bool saImmOiImplementerSet_return_OK=true;
bool saImmOiImplementerSet_accessed=false;
bool saImmOmInitialize_return_OK=true;
bool saImmOmInitialize_accessed=false;

#if 0 // does not work
TEST(OamSAOiProxy, ObjImp_init_imm_OK)
{
	// INITIALIZE
	saImmOiInitialize_2_return_OK=true;
	MafOamSpiTransactionMaster_1_available= true;
	MafOamSpiTransactionMaster_2_available= true;
	saImmOiInitialize_2_accessed=false;
	saImmOiSelectionObjectGet_return_OK=true;
	saImmOiImplementerSet_return_OK=true;

	//EXECUTE TEST & VALIDATE RESULT
	EXPECT_TRUE(ObjImp_init_imm()==MafOk);
	EXPECT_TRUE(saImmOiInitialize_2_accessed);
	EXPECT_TRUE(!MafOamSpiTransactionMaster_1_selected);
	EXPECT_TRUE(MafOamSpiTransactionMaster_2_selected);
	EXPECT_TRUE(saImmOiSelectionObjectGet_accessed);

	//RETURN ORIGINAL INITIALIZE
}

TEST(OamSAOiProxy, ObjImp_init_imm_FatalError_saImmOiInitialize_2)
{
	// INITIALIZE
	saImmOiInitialize_2_return_OK=false;
	MafOamSpiTransactionMaster_1_available= true;
	MafOamSpiTransactionMaster_2_available= true;
	saImmOiInitialize_2_accessed=false;
	saImmOiSelectionObjectGet_return_OK=true;
	saImmOiImplementerSet_return_OK=true;

	//EXECUTE TEST & VALIDATE RESULT
	EXPECT_TRUE(ObjImp_init_imm()==MafFailure);
	EXPECT_TRUE(saImmOiInitialize_2_accessed);

	//RETURN ORIGINAL INITIALIZE
	saImmOiInitialize_2_return_OK=true;
}

TEST(OamSAOiProxy, ObjImp_init_imm_FatalError_MafOamSpiTransactionMaster_2)
{
	// INITIALIZE
	saImmOiInitialize_2_return_OK=true;
	MafOamSpiTransactionMaster_1_available= true;
	MafOamSpiTransactionMaster_2_available= false;
	saImmOiSelectionObjectGet_return_OK=true;

	//EXECUTE TEST & VALIDATE RESULT
	EXPECT_TRUE(ObjImp_init_imm()==MafFailure);

	//RETURN ORIGINAL INITIALIZE
	MafOamSpiTransactionMaster_2_available=true;
}

TEST(OamSAOiProxy, ObjImp_init_imm_FatalError_MafOamSpiTransactionMaster_1and2)
{
	// INITIALIZE
	saImmOiInitialize_2_return_OK=true;
	MafOamSpiTransactionMaster_1_available= false;
	MafOamSpiTransactionMaster_2_available= false;
	saImmOiSelectionObjectGet_return_OK=true;

	//EXECUTE TEST & VALIDATE RESULT
	EXPECT_TRUE(ObjImp_init_imm()==MafFailure);
	EXPECT_TRUE(!MafOamSpiTransactionMaster_1_selected);
	EXPECT_TRUE(!MafOamSpiTransactionMaster_2_selected);

	//RETURN ORIGINAL INITIALIZE
	MafOamSpiTransactionMaster_1_available=true;
	MafOamSpiTransactionMaster_2_available=true;
}

TEST(OamSAOiProxy, ObjImp_init_imm_FatalError_saImmOiSelectionObjectGet)
{
	// INITIALIZE
	saImmOiInitialize_2_return_OK=true;
	MafOamSpiTransactionMaster_1_available= true;
	MafOamSpiTransactionMaster_2_available= true;
	saImmOiSelectionObjectGet_return_OK=false;
	saImmOiSelectionObjectGet_accessed=false;

	//EXECUTE TEST & VALIDATE RESULT
	EXPECT_TRUE(ObjImp_init_imm()==MafFailure);
	EXPECT_TRUE(saImmOiSelectionObjectGet_accessed);

	//RETURN ORIGINAL INITIALIZE
	saImmOiSelectionObjectGet_return_OK=true;
}

TEST(OamSAOiProxy, ObjImp_init_imm_FatalError_saImmOiImplementerSet)
{
	// INITIALIZE
	saImmOiInitialize_2_return_OK=true;
	MafOamSpiTransactionMaster_1_available= true;
	MafOamSpiTransactionMaster_2_available= true;
	saImmOiSelectionObjectGet_return_OK=true;
	saImmOiImplementerSet_return_OK=false;
	saImmOiImplementerSet_accessed=false;

	//EXECUTE TEST & VALIDATE RESULT
	EXPECT_TRUE(ObjImp_init_imm()==MafFailure);
	EXPECT_TRUE(saImmOiImplementerSet_accessed);

	//RETURN ORIGINAL INITIALIZE
	saImmOiImplementerSet_return_OK=true;

}

TEST(OamSAOiProxy, ObjImp_init_imm_FatalError_saImmOmInitialize)
{
	// INITIALIZE
	saImmOiInitialize_2_return_OK=true;
	MafOamSpiTransactionMaster_1_available= true;
	MafOamSpiTransactionMaster_2_available= true;
	saImmOiSelectionObjectGet_return_OK=true;
	saImmOiImplementerSet_return_OK=true;
	saImmOmInitialize_return_OK=false;
	saImmOmInitialize_accessed=false;

	//EXECUTE TEST & VALIDATE RESULT
	EXPECT_TRUE(ObjImp_init_imm()==MafFailure);
	EXPECT_TRUE(saImmOmInitialize_accessed);

	//RETURN ORIGINAL INITIALIZE
	saImmOmInitialize_return_OK=true;
}
#endif

extern void createRuntimeStructAttrMods(std::string rdnValue, MafMoAttributeValueStructMember_3* SM, SaImmAttrValuesT_2*** attrValues);
extern void freeSaImmAttrValuesT_2(SaImmAttrValuesT_2*** attrValues);
extern void freeMoAttrStructList(MafMoAttributeValueStructMember_3T *firstMemberStruct);
int trace_flag = 0;

TEST(OamSAOiProxy, createRuntimeStructAttrMods_SM_is_NULL)
{
	MafMoAttributeValueStructMember_3* SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);
	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// rdn
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);

	// null-terminated
	attrValues++;
	//std::cout << attrValues << std::endl;
	EXPECT_TRUE(*attrValues == NULL);

	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_is_invalid)
{
	MafMoAttributeValueStructMember_3* SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT8_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3T(-1);
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->next = NULL;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);
	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// rdn
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);

	// null-terminated
	attrValues++;
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_INT8_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT8_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i8 = 100;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT8_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i8 = -100;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT8_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT8_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 100);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT8_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT8_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == -100);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_INT8_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT8_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i8 = 100;
	SM->memberValue->values[1].value.i8 = 101;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT8_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i8 = -100;
	SM->memberValue->values[1].value.i8 = -101;
	SM->memberValue->values[2].value.i8 = -102;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT8_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT8_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 100);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 101);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT8_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT8_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == -100);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == -101);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == -102);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_INT16_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT16_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i16 = 0x1234;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT16_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i16 = 0x1235;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT16_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i16 = 0x1236;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT16_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT16_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x1234);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT16_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT16_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x1235);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT16_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT16_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x1236);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_INT16_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT16_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i16 = 0x1234;
	SM->memberValue->values[1].value.i16 = 0x1244;
	SM->memberValue->values[2].value.i16 = 0x1254;
	SM->memberValue->values[3].value.i16 = 0x1264;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT16_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i16 = 0x1235;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT16_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i16 = 0x1236;
	SM->memberValue->values[1].value.i16 = 0x1246;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT16_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT16_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x1234);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x1244);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0x1254);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[3])) == 0x1264);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT16_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT16_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x1235);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT16_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT16_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x1236);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x1246);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_INT32_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12341234;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12341235;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12341236;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12341234);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12341235);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12341236);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_INT32_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12341234;
	SM->memberValue->values[1].value.i32 = 0x12341235;
	SM->memberValue->values[2].value.i32 = 0x12341236;
	SM->memberValue->values[3].value.i32 = 0x12341237;
	SM->memberValue->values[4].value.i32 = 0x12341238;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12341235;
	SM->memberValue->values[1].value.i32 = 0x12341236;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12341236;
	SM->memberValue->values[1].value.i32 = 0x12341237;
	SM->memberValue->values[2].value.i32 = 0x12341238;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_multi_4");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12341236;
	SM->memberValue->values[1].value.i32 = 0x12341237;
	SM->memberValue->values[2].value.i32 = 0x12341238;
	SM->memberValue->values[3].value.i32 = 0x12341239;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12341234);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x12341235);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0x12341236);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[3])) == 0x12341237);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[4])) == 0x12341238);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12341235);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x12341236);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12341236);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x12341237);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0x12341238);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_multi_4
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_multi_4") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12341236);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x12341237);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0x12341238);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[3])) == 0x12341239);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_INT64_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT64_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i64 = 0x1234123412341234LL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT64_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i64 = 0x1234123412341235LL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT64_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i64 = 0x1234123412341236LL;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT64_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT64_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[0])) == 0x1234123412341234LL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT64_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT64_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[0])) == 0x1234123412341235LL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT64_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT64_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[0])) == 0x1234123412341236LL);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_INT64_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT64_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i64 = 0x1234123412341234LL;
	SM->memberValue->values[1].value.i64 = 0x1234123412341235LL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT64_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i64 = 0x1234123412341235LL;
	SM->memberValue->values[1].value.i64 = 0x1234123412341236LL;
	SM->memberValue->values[2].value.i64 = 0x1234123412341237LL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT64_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i64 = 0x1234123412341236LL;
	SM->memberValue->values[1].value.i64 = 0x1234123412341237LL;
	SM->memberValue->values[2].value.i64 = 0x1234123412341238LL;
	SM->memberValue->values[3].value.i64 = 0x1234123412341239LL;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT64_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT64_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[0])) == 0x1234123412341234LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[1])) == 0x1234123412341235LL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT64_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT64_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[0])) == 0x1234123412341235LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[1])) == 0x1234123412341236LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[2])) == 0x1234123412341237LL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT64_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT64_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[0])) == 0x1234123412341236LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[1])) == 0x1234123412341237LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[2])) == 0x1234123412341238LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[3])) == 0x1234123412341239LL);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_UINT8_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT8_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u8 = 0x12;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT8_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u8 = 0x13;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT8_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT8_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT8_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT8_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x13);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_UINT8_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT8_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u8 = 0x12;
	SM->memberValue->values[1].value.u8 = 0x13;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT8_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u8 = 0x12;
	SM->memberValue->values[1].value.u8 = 0x13;
	SM->memberValue->values[2].value.u8 = 0x14;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT8_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT8_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x13);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT8_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT8_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x13);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[2])) == 0x14);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_UINT16_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT16_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u16 = 0x1234;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT16_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u16 = 0x1235;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT16_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u16 = 0x1236;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT16_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT16_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x1234);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT16_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT16_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x1235);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT16_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT16_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x1236);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_UINT16_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT16_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u16 = 0x1234;
	SM->memberValue->values[1].value.u16 = 0x1244;
	SM->memberValue->values[2].value.u16 = 0x1254;
	SM->memberValue->values[3].value.u16 = 0x1264;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT16_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u16 = 0x1235;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT16_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u16 = 0x1236;
	SM->memberValue->values[1].value.u16 = 0x1246;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT16_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT16_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x1234);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x1244);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[2])) == 0x1254);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[3])) == 0x1264);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT16_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT16_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x1235);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT16_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT16_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x1236);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x1246);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_UINT32_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12341234;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12341235;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12341236;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12341234);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12341235);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12341236);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_UINT32_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12341234;
	SM->memberValue->values[1].value.u32 = 0x12341235;
	SM->memberValue->values[2].value.u32 = 0x12341236;
	SM->memberValue->values[3].value.u32 = 0x12341237;
	SM->memberValue->values[4].value.u32 = 0x12341238;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12341235;
	SM->memberValue->values[1].value.u32 = 0x12341236;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12341236;
	SM->memberValue->values[1].value.u32 = 0x12341237;
	SM->memberValue->values[2].value.u32 = 0x12341238;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_multi_4");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12341236;
	SM->memberValue->values[1].value.u32 = 0x12341237;
	SM->memberValue->values[2].value.u32 = 0x12341238;
	SM->memberValue->values[3].value.u32 = 0x12341239;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12341234);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x12341235);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[2])) == 0x12341236);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[3])) == 0x12341237);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[4])) == 0x12341238);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12341235);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x12341236);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12341236);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x12341237);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[2])) == 0x12341238);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_multi_4
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_multi_4") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12341236);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x12341237);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[2])) == 0x12341238);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[3])) == 0x12341239);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_UINT64_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT64_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u64 = 0x1234123412341234ULL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT64_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u64 = 0x1234123412341235ULL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT64_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u64 = 0x1234123412341236ULL;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT64_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT64_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[0])) == 0x1234123412341234ULL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT64_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT64_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[0])) == 0x1234123412341235ULL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT64_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT64_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[0])) == 0x1234123412341236ULL);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_UINT64_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT64_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u64 = 0x1234123412341234ULL;
	SM->memberValue->values[1].value.u64 = 0x1234123412341235ULL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT64_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u64 = 0x1234123412341235ULL;
	SM->memberValue->values[1].value.u64 = 0x1234123412341236ULL;
	SM->memberValue->values[2].value.u64 = 0x1234123412341237ULL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT64_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u64 = 0x1234123412341236ULL;
	SM->memberValue->values[1].value.u64 = 0x1234123412341237ULL;
	SM->memberValue->values[2].value.u64 = 0x1234123412341238ULL;
	SM->memberValue->values[3].value.u64 = 0x1234123412341239ULL;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT64_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT64_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[0])) == 0x1234123412341234ULL);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[1])) == 0x1234123412341235ULL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT64_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT64_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[0])) == 0x1234123412341235ULL);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[1])) == 0x1234123412341236ULL);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[2])) == 0x1234123412341237ULL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT64_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT64_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[0])) == 0x1234123412341236ULL);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[1])) == 0x1234123412341237ULL);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[2])) == 0x1234123412341238ULL);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[3])) == 0x1234123412341239ULL);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_BOOL_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_BOOL_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_BOOL;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theBool = true;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_BOOL_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_BOOL;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theBool = false;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_BOOL_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_BOOL;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theBool = true;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_BOOL_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_BOOL_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1);
	attrValues++;

	// MafOamSpiMoAttributeType_3_BOOL_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_BOOL_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_BOOL_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_BOOL_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_BOOL_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_BOOL_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 7;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_BOOL;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theBool = true;
	SM->memberValue->values[1].value.theBool = true;
	SM->memberValue->values[2].value.theBool = false;
	SM->memberValue->values[3].value.theBool = true;
	SM->memberValue->values[4].value.theBool = true;
	SM->memberValue->values[5].value.theBool = false;
	SM->memberValue->values[6].value.theBool = true;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_BOOL_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_BOOL;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theBool = false;
	SM->memberValue->values[1].value.theBool = true;
	SM->memberValue->values[2].value.theBool = false;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_BOOL_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 10;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_BOOL;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theBool = true;
	SM->memberValue->values[1].value.theBool = true;
	SM->memberValue->values[2].value.theBool = true;
	SM->memberValue->values[3].value.theBool = false;
	SM->memberValue->values[4].value.theBool = true;
	SM->memberValue->values[5].value.theBool = false;
	SM->memberValue->values[6].value.theBool = false;
	SM->memberValue->values[7].value.theBool = true;
	SM->memberValue->values[8].value.theBool = true;
	SM->memberValue->values[9].value.theBool = true;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_BOOL_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_BOOL_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 7);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[3])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[4])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[5])) == 0);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[6])) == 1);
	attrValues++;

	// MafOamSpiMoAttributeType_3_BOOL_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_BOOL_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_BOOL_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_BOOL_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 10);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[3])) == 0);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[4])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[5])) == 0);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[6])) == 0);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[7])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[8])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[9])) == 1);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_STRING_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString1");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString2");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString3");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_single_4");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString4");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_single_5");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString5");
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString1") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString2") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString3") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_single_4
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_single_4") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString4") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_single_5
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_single_5") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString5") == 0);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_STRING_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString1");
	SM->memberValue->values[1].value.theString = strdup("theString11");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString2");
	SM->memberValue->values[1].value.theString = strdup("theString21");
	SM->memberValue->values[2].value.theString = strdup("theString22");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString3");
	SM->memberValue->values[1].value.theString = strdup("theString31");
	SM->memberValue->values[2].value.theString = strdup("theString32");
	SM->memberValue->values[3].value.theString = strdup("theString33");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_multi_4");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString4");
	SM->memberValue->values[1].value.theString = strdup("theString41");
	SM->memberValue->values[2].value.theString = strdup("theString42");
	SM->memberValue->values[3].value.theString = strdup("theString43");
	SM->memberValue->values[4].value.theString = strdup("theString44");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_multi_5");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 6;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString5");
	SM->memberValue->values[1].value.theString = strdup("theString51");
	SM->memberValue->values[2].value.theString = strdup("theString52");
	SM->memberValue->values[3].value.theString = strdup("theString53");
	SM->memberValue->values[4].value.theString = strdup("theString54");
	SM->memberValue->values[5].value.theString = strdup("theString55");
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString1") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[1])), "theString11") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString2") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[1])), "theString21") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[2])), "theString22") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString3") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[1])), "theString31") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[2])), "theString32") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[3])), "theString33") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_multi_4
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_multi_4") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString4") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[1])), "theString41") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[2])), "theString42") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[3])), "theString43") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[4])), "theString44") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_multi_5
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_multi_5") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 6);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString5") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[1])), "theString51") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[2])), "theString52") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[3])), "theString53") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[4])), "theString54") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[5])), "theString55") == 0);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_ENUM_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_ENUM_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_ENUM;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theEnum = 1000;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_ENUM_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_ENUM;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theEnum = 2000;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_ENUM_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_ENUM;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theEnum = 3000;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_ENUM_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_ENUM_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1000);
	attrValues++;

	// MafOamSpiMoAttributeType_3_ENUM_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_ENUM_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 2000);
	attrValues++;

	// MafOamSpiMoAttributeType_3_ENUM_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_ENUM_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 3000);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_ENUM_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_ENUM_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_ENUM;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theEnum = 1000;
	SM->memberValue->values[1].value.theEnum = 1001;
	SM->memberValue->values[2].value.theEnum = 1002;
	SM->memberValue->values[3].value.theEnum = 1003;
	SM->memberValue->values[4].value.theEnum = 1004;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_ENUM_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_ENUM;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theEnum = 2000;
	SM->memberValue->values[1].value.theEnum = 2001;
	SM->memberValue->values[2].value.theEnum = 2002;
	SM->memberValue->values[3].value.theEnum = 2003;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_ENUM_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_ENUM;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theEnum = 3000;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_ENUM_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_ENUM_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1000);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 1001);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 1002);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[3])) == 1003);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[4])) == 1004);
	attrValues++;

	// MafOamSpiMoAttributeType_3_ENUM_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_ENUM_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 2000);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 2001);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 2002);
	attrValues++;

	// MafOamSpiMoAttributeType_3_ENUM_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_ENUM_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 3000);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_DECIMAL64_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_DECIMAL64_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.decimal64 = 1234.56780;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_DECIMAL64_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.decimal64 = 1234.56781;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_DECIMAL64_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.decimal64 = 1234.56782;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_DECIMAL64_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_DECIMAL64_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SADOUBLET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[0])) == 1234.56780);
	attrValues++;

	// MafOamSpiMoAttributeType_3_DECIMAL64_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_DECIMAL64_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SADOUBLET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[0])) == 1234.56781);
	attrValues++;

	// MafOamSpiMoAttributeType_3_DECIMAL64_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_DECIMAL64_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SADOUBLET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[0])) == 1234.56782);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_DECIMAL64_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_DECIMAL64_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.decimal64 = 1234.56780;
	SM->memberValue->values[1].value.decimal64 = 1234.56781;
	SM->memberValue->values[2].value.decimal64 = 1234.56782;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_DECIMAL64_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.decimal64 = 1234.56781;
	SM->memberValue->values[1].value.decimal64 = 1234.56782;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_DECIMAL64_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.decimal64 = 1234.56782;
	SM->memberValue->values[1].value.decimal64 = 1234.56783;
	SM->memberValue->values[2].value.decimal64 = 1234.56784;
	SM->memberValue->values[3].value.decimal64 = 1234.56785;
	SM->memberValue->values[4].value.decimal64 = 1234.56786;
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_DECIMAL64_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_DECIMAL64_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SADOUBLET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[0])) == 1234.56780);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[1])) == 1234.56781);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[2])) == 1234.56782);
	attrValues++;

	// MafOamSpiMoAttributeType_3_DECIMAL64_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_DECIMAL64_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SADOUBLET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[0])) == 1234.56781);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[1])) == 1234.56782);
	attrValues++;

	// MafOamSpiMoAttributeType_3_DECIMAL64_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_DECIMAL64_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SADOUBLET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[0])) == 1234.56782);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[1])) == 1234.56783);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[2])) == 1234.56784);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[3])) == 1234.56785);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[4])) == 1234.56786);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_REFERENCE_single)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_REFERENCE_single_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_REFERENCE;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.moRef = strdup("MoDnString00");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_REFERENCE_single_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_REFERENCE;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.moRef = strdup("MoDnString10");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_REFERENCE_single_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_REFERENCE;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.moRef = strdup("MoDnString20");
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_REFERENCE_single_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_REFERENCE_single_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SANAMET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[0])), "toImmMoDnString00") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_REFERENCE_single_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_REFERENCE_single_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SANAMET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[0])), "toImmMoDnString10") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_REFERENCE_single_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_REFERENCE_single_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SANAMET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[0])), "toImmMoDnString20") == 0);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_type_MafOamSpiMoAttributeType_3_REFERENCE_multi)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_REFERENCE_multi_1");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_REFERENCE;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.moRef = strdup("MoDnString00");
	SM->memberValue->values[1].value.moRef = strdup("MoDnString01");
	SM->memberValue->values[2].value.moRef = strdup("MoDnString02");
	SM->memberValue->values[3].value.moRef = strdup("MoDnString03");
	SM->memberValue->values[4].value.moRef = strdup("MoDnString04");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_REFERENCE_multi_2");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 6;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_REFERENCE;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.moRef = strdup("MoDnString10");
	SM->memberValue->values[1].value.moRef = strdup("MoDnString11");
	SM->memberValue->values[2].value.moRef = strdup("MoDnString12");
	SM->memberValue->values[3].value.moRef = strdup("MoDnString13");
	SM->memberValue->values[4].value.moRef = strdup("MoDnString14");
	SM->memberValue->values[5].value.moRef = strdup("MoDnString15");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_REFERENCE_multi_3");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_REFERENCE;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.moRef = strdup("MoDnString20");
	SM->memberValue->values[1].value.moRef = strdup("MoDnString21");
	SM->memberValue->values[2].value.moRef = strdup("MoDnString22");
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_REFERENCE_multi_1
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_REFERENCE_multi_1") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SANAMET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[0])), "toImmMoDnString00") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[1])), "toImmMoDnString01") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[2])), "toImmMoDnString02") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[3])), "toImmMoDnString03") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[4])), "toImmMoDnString04") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_REFERENCE_multi_2
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_REFERENCE_multi_2") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SANAMET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 6);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[0])), "toImmMoDnString10") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[1])), "toImmMoDnString11") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[2])), "toImmMoDnString12") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[3])), "toImmMoDnString13") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[4])), "toImmMoDnString14") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[5])), "toImmMoDnString15") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_REFERENCE_multi_3
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_REFERENCE_multi_3") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SANAMET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[0])), "toImmMoDnString20") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[1])), "toImmMoDnString21") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[2])), "toImmMoDnString22") == 0);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_multi_type_single_value)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT8_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i8 = 0x12;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT16_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i16 = 0x1234;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12345678;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT64_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i64 = 0x1234567812345678LL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT8_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u8 = 0x12;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT16_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u16 = 0x1234;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12345678;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT64_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u64 = 0x1234567812345678ULL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_BOOL_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_BOOL;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theBool = true;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_ENUM_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_ENUM;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theEnum = 1000;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_DECIMAL64_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.decimal64 = 12345.06789;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_REFERENCE_single");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 1;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_REFERENCE;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.moRef = strdup("MoDnString");
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT8_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT8_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT16_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT16_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x1234);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12345678);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT64_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT64_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[0])) == 0x1234567812345678LL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT8_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT8_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT16_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT16_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x1234);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12345678);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT64_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT64_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[0])) == 0x1234567812345678ULL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_BOOL_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_BOOL_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1);
	attrValues++;

	// MafOamSpiMoAttributeType_3_ENUM_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_ENUM_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1000);
	attrValues++;

	// MafOamSpiMoAttributeType_3_DECIMAL64_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_DECIMAL64_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SADOUBLET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[0])) == 12345.06789);
	attrValues++;

	// MafOamSpiMoAttributeType_3_REFERENCE_single
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_REFERENCE_single") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SANAMET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[0])), "toImmMoDnString") == 0);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

TEST(OamSAOiProxy, createRuntimeStructAttrMods_multi_type_multi_value)
{
	MafMoAttributeValueStructMember_3 * SM = NULL;
	SaImmAttrValuesT_2** attrValues = NULL;

	// construct SM
	SM = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	MafMoAttributeValueStructMember_3 *head = SM;
	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT8_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i8 = 0x12;
	SM->memberValue->values[1].value.i8 = 0x13;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT16_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i16 = 0x1234;
	SM->memberValue->values[1].value.i16 = 0x1235;
	SM->memberValue->values[2].value.i16 = 0x1236;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT32_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i32 = 0x12345678;
	SM->memberValue->values[1].value.i32 = 0x12345679;
	SM->memberValue->values[2].value.i32 = 0x12345670;
	SM->memberValue->values[3].value.i32 = 0x12345671;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_INT64_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_INT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.i64 = 0x1234567812345678LL;
	SM->memberValue->values[1].value.i64 = 0x1234567812345679LL;
	SM->memberValue->values[2].value.i64 = 0x1234567812345670LL;
	SM->memberValue->values[3].value.i64 = 0x1234567812345671LL;
	SM->memberValue->values[4].value.i64 = 0x1234567812345672LL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT8_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 6;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT8;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u8 = 0x12;
	SM->memberValue->values[1].value.u8 = 0x13;
	SM->memberValue->values[2].value.u8 = 0x14;
	SM->memberValue->values[3].value.u8 = 0x15;
	SM->memberValue->values[4].value.u8 = 0x16;
	SM->memberValue->values[5].value.u8 = 0x17;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT16_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT16;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u16 = 0x1234;
	SM->memberValue->values[1].value.u16 = 0x1235;
	SM->memberValue->values[2].value.u16 = 0x1236;
	SM->memberValue->values[3].value.u16 = 0x1237;
	SM->memberValue->values[4].value.u16 = 0x1238;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT32_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT32;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u32 = 0x12345678;
	SM->memberValue->values[1].value.u32 = 0x12345679;
	SM->memberValue->values[2].value.u32 = 0x12345670;
	SM->memberValue->values[3].value.u32 = 0x12345671;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_UINT64_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_UINT64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.u64 = 0x1234567812345678ULL;
	SM->memberValue->values[1].value.u64 = 0x1234567812345679ULL;
	SM->memberValue->values[2].value.u64 = 0x1234567812345670ULL;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_STRING_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 2;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_STRING;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theString = strdup("theString");
	SM->memberValue->values[1].value.theString = strdup("theString1");
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_BOOL_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 3;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_BOOL;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theBool = true;
	SM->memberValue->values[1].value.theBool = false;
	SM->memberValue->values[2].value.theBool = false;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_ENUM_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 4;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_ENUM;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.theEnum = 1000;
	SM->memberValue->values[1].value.theEnum = 1001;
	SM->memberValue->values[2].value.theEnum = 1002;
	SM->memberValue->values[3].value.theEnum = 1003;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_DECIMAL64_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 5;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_DECIMAL64;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.decimal64 = 12345.06789;
	SM->memberValue->values[1].value.decimal64 = 12345.16789;
	SM->memberValue->values[2].value.decimal64 = 12345.26789;
	SM->memberValue->values[3].value.decimal64 = 12345.36789;
	SM->memberValue->values[4].value.decimal64 = 12345.46789;
	SM->next = (MafMoAttributeValueStructMember_3 *) malloc(sizeof(MafMoAttributeValueStructMember_3));
	SM = SM->next;

	SM->memberName = strdup("MafOamSpiMoAttributeType_3_REFERENCE_multi");
	SM->memberValue = (MafMoAttributeValueContainer_3 *) malloc(sizeof(MafMoAttributeValueContainer_3));
	SM->memberValue->nrOfValues = 6;
	SM->memberValue->type = MafOamSpiMoAttributeType_3_REFERENCE;
	SM->memberValue->values = (MafMoAttributeValue_3T *) malloc(SM->memberValue->nrOfValues * sizeof(MafMoAttributeValue_3T));
	SM->memberValue->values[0].value.moRef = strdup("MoDnString");
	SM->memberValue->values[1].value.moRef = strdup("MoDnString1");
	SM->memberValue->values[2].value.moRef = strdup("MoDnString2");
	SM->memberValue->values[3].value.moRef = strdup("MoDnString3");
	SM->memberValue->values[4].value.moRef = strdup("MoDnString4");
	SM->memberValue->values[5].value.moRef = strdup("MoDnString5");
	SM->next = NULL;

	SM = head;

	createRuntimeStructAttrMods("rdn", SM, &attrValues);

	SaImmAttrValuesT_2** bak_attrValues = attrValues;

	EXPECT_TRUE(attrValues != NULL);
	// id
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "id") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 1);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "rdn") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT8_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT8_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x13);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT16_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT16_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x1234);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x1235);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0x1236);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT32_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT32_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 0x12345678);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0x12345679);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0x12345670);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[3])) == 0x12345671);
	attrValues++;

	// MafOamSpiMoAttributeType_3_INT64_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_INT64_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[0])) == 0x1234567812345678LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[1])) == 0x1234567812345679LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[2])) == 0x1234567812345670LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[3])) == 0x1234567812345671LL);
	EXPECT_TRUE(*((SaInt64T *)((*attrValues)->attrValues[4])) == 0x1234567812345672LL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT8_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT8_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 6);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x13);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[2])) == 0x14);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[3])) == 0x15);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[4])) == 0x16);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[5])) == 0x17);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT16_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT16_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x1234);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x1235);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[2])) == 0x1236);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[3])) == 0x1237);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[4])) == 0x1238);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT32_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT32_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[0])) == 0x12345678);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[1])) == 0x12345679);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[2])) == 0x12345670);
	EXPECT_TRUE(*((SaUint32T *)((*attrValues)->attrValues[3])) == 0x12345671);
	attrValues++;

	// MafOamSpiMoAttributeType_3_UINT64_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_UINT64_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAUINT64T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[0])) == 0x1234567812345678ULL);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[1])) == 0x1234567812345679ULL);
	EXPECT_TRUE(*((SaUint64T *)((*attrValues)->attrValues[2])) == 0x1234567812345670ULL);
	attrValues++;

	// MafOamSpiMoAttributeType_3_STRING_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_STRING_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SASTRINGT);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 2);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[0])), "theString") == 0);
	EXPECT_TRUE(strcmp(*((SaStringT *)((*attrValues)->attrValues[1])), "theString1") == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_BOOL_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_BOOL_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 3);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 0);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 0);
	attrValues++;

	// MafOamSpiMoAttributeType_3_ENUM_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_ENUM_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SAINT32T);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 4);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[0])) == 1000);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[1])) == 1001);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[2])) == 1002);
	EXPECT_TRUE(*((SaInt32T *)((*attrValues)->attrValues[3])) == 1003);
	attrValues++;

	// MafOamSpiMoAttributeType_3_DECIMAL64_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_DECIMAL64_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SADOUBLET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 5);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[0])) == 12345.06789);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[1])) == 12345.16789);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[2])) == 12345.26789);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[3])) == 12345.36789);
	EXPECT_TRUE(*((SaDoubleT *)((*attrValues)->attrValues[4])) == 12345.46789);
	attrValues++;

	// MafOamSpiMoAttributeType_3_REFERENCE_multi
	EXPECT_TRUE(strcmp((*attrValues)->attrName, "MafOamSpiMoAttributeType_3_REFERENCE_multi") == 0);
	EXPECT_TRUE((*attrValues)->attrValueType == SA_IMM_ATTR_SANAMET);
	EXPECT_TRUE((*attrValues)->attrValuesNumber == 6);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[0])), "toImmMoDnString") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[1])), "toImmMoDnString1") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[2])), "toImmMoDnString2") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[3])), "toImmMoDnString3") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[4])), "toImmMoDnString4") == 0);
	EXPECT_TRUE(strcmp(saNameGet((SaNameT *)((*attrValues)->attrValues[5])), "toImmMoDnString5") == 0);
	attrValues++;

	// null-terminated
	EXPECT_TRUE(*attrValues == NULL);

	freeMoAttrStructList(SM);
	freeSaImmAttrValuesT_2(&bak_attrValues);
}

#if 1
// Dummy cmw api function
SaAisErrorT saImmOmClassDescriptionMemoryFree_2(SaImmHandleT immHandle, SaImmAttrDefinitionT_2 **attrDefinitions) {
	return SA_AIS_OK;
}

SaAisErrorT saImmOiRtObjectDelete(SaImmOiHandleT immOiHandle, const SaNameT *objectName)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiRtObjectCreate_2(SaImmOiHandleT immOiHandle, const SaImmClassNameT className, const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	return SA_AIS_OK;
}

SaAisErrorT
saImmOiDispatch(SaImmOiHandleT immOiHandle, SaDispatchFlagsT dispatchFlags) {
	printf("----> saImmOiDispatch \n");
	return SA_AIS_OK;
}

SaAisErrorT saImmOmSearchInitialize_2(SaImmHandleT immHandle,
		const SaNameT *rootName,
		SaImmScopeT scope,
		SaImmSearchOptionsT searchOptions,
		const SaImmSearchParametersT_2 *searchParam,
		const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle){
	return SA_AIS_OK;
}

SaAisErrorT saImmOmSearchNext_2_returnValue = SA_AIS_OK;
SaAisErrorT saImmOmSearchNext_2(SaImmSearchHandleT searchHandle, SaNameT *objectName, SaImmAttrValuesT_2 ***attributes){
	printf("----> saImmOmSearchNext_2 \n");
	return saImmOmSearchNext_2_returnValue;
}

SaAisErrorT saImmOmSearchFinalize(SaImmSearchHandleT searchHandle) {
	printf("----> saImmOmSearchFinalize \n");
	return SA_AIS_OK;
}

// Received error strings saved here
std::list<std::string>    ReceivedErrorStrings;

SaAisErrorT
saImmOiCcbSetErrorString(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaStringT errorString) {
	printf("----> saImmOiCcbSetErrorString \n");

	ReceivedErrorStrings.push_front(std::string(errorString));
	return SA_AIS_OK;
}

const SaVersionT imm_version = { immReleaseCode, immMajorVersion, immMinorVersion };
SaVersionT CM::ImmCmd::mVersion = { immReleaseCode, immMajorVersion, immMinorVersion };

#endif
