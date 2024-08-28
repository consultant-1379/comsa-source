/*
 * ImmCmd_unittest.cc
 *
 *  Created on: Dec 18, 2014
 *      Author: Nguyen Tan Phat
 */



#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <ctype.h>
#include <unistd.h>

#include <assert.h>
#include <gtest/gtest.h>

//CoreMW
#include "saAis.h"
#include "saImm.h"
#include "saImmOm.h"
#include "saImmOm_A_2_11.h"
//COM
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiComponent_1.h"
#include "MafMgmtSpiInterfacePortal_1.h"
#include "MafMgmtSpiServiceIdentities_1.h"
#include "MafMgmtSpiThreadContext_2.h"
#include "MafOamSpiManagedObject_3.h"
#include "MafOamSpiModelRepository_1.h"
// COM SA
#include "ComSA.h"
#include "DxEtModelConstants.h"
#include "ImmCmd.h"
#include "OamSATransactionRepository.h"
#include "OamSATransactionalResource.h"
#include "OamSATranslator.h"
#include "TxContext.h"
// Unittest
#include "OamSACache_dummy.h"

#define REDIRECT_LOG
#ifdef REDIRECT_LOG
    #define DEBUG printf;
#else
    #include "trace.h"
#endif

int trace_flag = 0;

#ifdef REDIRECT_LOG
////////////////////////////////////3//////////////////
// LOG
//////////////////////////////////////////////////////
#ifdef  __cplusplus
extern "C" {
#endif
#define LOG_PREFIX " "

//FILE* fpLog = NULL;
int numErr = 0;

static void coremw_vlog2(int priority, char const* fmt, va_list ap) {
    char buffer[256];
    int len = strlen(LOG_PREFIX);
    strcpy(buffer, LOG_PREFIX);
    buffer[len] = ' ';
    len++;
    vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);
    if(LOG_ERR == priority)
    {
        printf("ERR: %s\n", buffer);
        numErr++;
        //fprintf(fpLog, "ERR: %s\n", buffer);
    }
    else
    {
        printf("DEBUG: %s\n", buffer);
        //fprintf(fpLog, "DEBUG: %s\n", buffer);
    }
}

void coremw_log(int priority, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    coremw_vlog2(priority, fmt, ap);
    va_end(ap);
}
void coremw_debug_log(int priority, const char* fmt, ...) {
    if(trace_flag)
    {
        va_list ap;
        va_start(ap, fmt);
        coremw_vlog2(priority, fmt, ap);
        va_end(ap);
    }
}

#ifdef  __cplusplus
}
#endif

#endif //REDIRECT_LOG

SaAisErrorT saImmOmCcbFinalize(SaImmCcbHandleT ccbHandle) {
    printf("----> saImmOmCcbFinalize \n");
    return SA_AIS_ERR_TRY_AGAIN;
}

SaAisErrorT saImmOmCcbValidate(SaImmCcbHandleT ccbHandle) {
    printf("----> saImmOmCcbValidate \n");
    return SA_AIS_ERR_TRY_AGAIN;
}

SaAisErrorT saImmOmCcbApply(SaImmCcbHandleT ccbHandle) {
    printf("----> saImmOmCcbApply \n");
    return SA_AIS_ERR_TRY_AGAIN;
}

SaAisErrorT saImmOmCcbAbort(SaImmCcbHandleT ccbHandle) {
    printf("----> saImmOmCcbAbort \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAdminOwnerInitialize(SaImmHandleT immHandle,
                     const SaImmAdminOwnerNameT adminOwnerName,
                     SaBoolT releaseOwnershipOnFinalize, SaImmAdminOwnerHandleT *ownerHandle)
{
    printf("----> saImmOmAdminOwnerInitialize \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAdminOwnerSet(SaImmAdminOwnerHandleT ownerHandle, const SaNameT **objectNames, SaImmScopeT scope)
{
    printf("----> saImmOmAdminOwnerSet \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAdminOwnerClear(SaImmHandleT immHandle, const SaNameT **objectNames, SaImmScopeT scope)
{
    printf("----> saImmOmAdminOwnerClear \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmSearchInitialize_2(SaImmHandleT immHandle,
               const SaNameT *rootName,
               SaImmScopeT scope,
               SaImmSearchOptionsT searchOptions,
               const SaImmSearchParametersT_2 *searchParam,
               const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
    printf("----> saImmOmSearchInitialize_2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmSearchNext_2(SaImmSearchHandleT searchHandle, SaNameT *objectName, SaImmAttrValuesT_2 ***attributes)
{
    printf("----> saImmOmSearchNext_2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAdminOperationInvoke_o2(
                SaImmAdminOwnerHandleT ownerHandle,
                const SaNameT *objectName,
                SaImmContinuationIdT continuationId,
                SaImmAdminOperationIdT operationId,
                const SaImmAdminOperationParamsT_2 **params,
                SaAisErrorT *operationReturnValue,
                SaTimeT timeout,
                SaImmAdminOperationParamsT_2 ***returnParams)
{
    printf("----> saImmOmAdminOperationInvoke_o2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAdminOperationInvoke_2(SaImmAdminOwnerHandleT ownerHandle,
                   const SaNameT *objectName,
                   SaImmContinuationIdT continuationId,
                   SaImmAdminOperationIdT operationId,
                   const SaImmAdminOperationParamsT_2 **params,
                   SaAisErrorT *operationReturnValue, SaTimeT timeout)
{
    printf("----> saImmOmAdminOperationInvoke_2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAdminOperationMemoryFree(
                 SaImmAdminOwnerHandleT ownerHandle,
                 SaImmAdminOperationParamsT_2 **returnParams)
{
    printf("----> saImmOmAdminOperationMemoryFree \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmCcbInitialize(SaImmAdminOwnerHandleT ownerHandle, SaImmCcbFlagsT ccbFlags, SaImmCcbHandleT *ccbHandle)
{
    printf("----> saImmOmCcbInitialize \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmCcbObjectModify_2(SaImmCcbHandleT ccbHandle,
              const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
    printf("----> saImmOmCcbObjectModify_2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmCcbObjectCreate_2(SaImmCcbHandleT ccbHandle,
              const SaImmClassNameT className,
              const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
    printf("----> saImmOmCcbObjectCreate_2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmCcbObjectDelete(SaImmCcbHandleT ccbHandle, const SaNameT *objectName)
{
    printf("----> saImmOmCcbObjectDelete \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmInitialize(SaImmHandleT *immHandle, const SaImmCallbacksT *immCallbacks, SaVersionT *version)
{
    printf("----> saImmOmInitialize \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAccessorInitialize(SaImmHandleT immHandle, SaImmAccessorHandleT *accessorHandle)
{
    printf("----> saImmOmAccessorInitialize \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAdminOwnerRelease(SaImmAdminOwnerHandleT ownerHandle, const SaNameT **objectNames, SaImmScopeT scope)
{
    printf("----> saImmOmAdminOwnerRelease \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmClassDescriptionGet_2(SaImmHandleT immHandle,
                  const SaImmClassNameT className,
                  SaImmClassCategoryT *classCategory, SaImmAttrDefinitionT_2 ***attrDefinitions)
{
    printf("----> saImmOmClassDescriptionGet_2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
              const SaNameT *objectName,
              const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes)
{
    printf("----> saImmOmAccessorGet_2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
SaAisErrorT saImmOmClassDescriptionMemoryFree_2(SaImmHandleT immHandle, SaImmAttrDefinitionT_2 **attrDefinitions)
{
    printf("----> saImmOmClassDescriptionMemoryFree_2 \n");
    return SA_AIS_ERR_TRY_AGAIN;
}
void PrintAttributeValues(SaImmAttrValuesT_2& AttrValues)
{
}

TEST(ImmCmdTryAgainLog, ImmCmdOmInit)
{
    //fpLog = fopen("/tmp/UT.log", "w");
    shared_ptr<TxContext> txContext(new TxContext(false));
    ImmCmdOmInit testObject(txContext.get());
    numErr = 0;
    SaAisErrorT ret = testObject.execute();
    // Expect the return code is SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(SA_AIS_ERR_TRY_AGAIN, ret);
    // Expect that we have only one err log for SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(1, numErr);
    //fclose(fpLog);
}

TEST(ImmCmdTryAgainLog, ImmCmdOmCcbValidate)
{
    //fpLog = fopen("/tmp/UT.log", "w");
    shared_ptr<TxContext> txContext(new TxContext(false));
    ImmCmdOmCcbValidate testObject(txContext.get());
    numErr = 0;
    SaAisErrorT ret = testObject.execute();
    // Expect the return code is SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(SA_AIS_ERR_TRY_AGAIN, ret);
    // Expect that we have only one err log for SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(1, numErr);
    //fclose(fpLog);
}

TEST(ImmCmdTryAgainLog, ImmCmdOmCcbAbort)
{
    //fpLog = fopen("/tmp/UT.log", "w");
    shared_ptr<TxContext> txContext(new TxContext(false));
    ImmCmdOmCcbAbort testObject(txContext.get());
    numErr = 0;
    SaAisErrorT ret = testObject.execute();
    // Expect the return code is SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(SA_AIS_ERR_TRY_AGAIN, ret);
    // Expect that we have only one err log for SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(1, numErr);
    //fclose(fpLog);
}

TEST(ImmCmdTryAgainLog, ImmCmdOmCcbApply)
{
    //fpLog = fopen("/tmp/UT.log", "w");
    shared_ptr<TxContext> txContext(new TxContext(false));
    ImmCmdOmCcbApply testObject(txContext.get());
    numErr = 0;
    SaAisErrorT ret = testObject.execute();
    // Expect the return code is SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(SA_AIS_ERR_TRY_AGAIN, ret);
    // Expect that we have only one err log for SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(1, numErr);
    //fclose(fpLog);
}

TEST(ImmCmdTryAgainLog, ImmCmdOmCcbObjectCreate)
{
    //fpLog = fopen("/tmp/UT.log", "w");
    shared_ptr<TxContext> txContext(new TxContext(false));
    SaNameT* parentIn = NULL;
    SaImmClassNameT classNameIn = NULL;
    SaImmAttrValuesT_2* attrValsIn = NULL;
    ImmCmdOmCcbObjectCreate testObject(txContext.get(),
            parentIn,
            classNameIn,
            &attrValsIn);
    numErr = 0;
    SaAisErrorT ret = testObject.execute();
    // Expect the return code is SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(SA_AIS_ERR_TRY_AGAIN, ret);
    // Expect that we have only one err log for SA_AIS_ERR_TRY_AGAIN
    ASSERT_EQ(1, numErr);
    //fclose(fpLog);
}
