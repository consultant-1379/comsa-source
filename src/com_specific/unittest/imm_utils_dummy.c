/******************************************************************************
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
*   File:   imm_utils_dummy.cc
*
*   Author: xdonngu
*
*   Date:   2014-03-05
*
*   This file implements the helper functions to use when contacting IMM.
*   The design is heavily influenced by this file's previous and larger version
*   which was implemented by CoreMw.
*
*   Mainly, the changes are:
*   Unnecessary functions have been removed, and the characteristics of Auto Retry
*   have been changed to (by default) continue indefinitely, instead of aborting;
*   as was decided by the ComSa DO team. (The duration of retry will be limited
*   by the timeout set for AMF CSI call-backs in this case.)
*   This characteristic can be reverted back to its previous version by making
*   errorsAreNonFatal 0.
*
*   Modify: xdonngu 2014-03-05: dummy file for UT of HS37161
*           xadaleg 2014-08-02  MR35347 - increase DN length
*
*****************************************************************************/

#include <unistd.h>
#include "imm_utils.h"
#include "saAis.h"


struct ImmAutoRetryProfile immAutoRetryProfile = { 1, 25, 400 };

/* ----------------------------------------------------------------------
* IMM call wrappers; This wrapper interface off loads the burden to
* handle return values and retries for each and every IMM-call. It
* makes the code cleaner.
*/
SaAisErrorT saImmOiInitialize_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOiInitialize_2(SaImmOiHandleT *immOiHandle,
	const SaImmOiCallbacksT_2 *immOiCallbacks,
	const SaVersionT *version)
{
	return saImmOiInitialize_rc;
}

SaAisErrorT autoRetry_saImmOiSelectionObjectGet(SaImmOiHandleT immOiHandle, SaSelectionObjectT *selectionObject)
{
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	return saImmOiClassImplementerSet(immOiHandle, className);
}

SaAisErrorT autoRetry_saImmOiClassImplementerRelease(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	SaAisErrorT retVal = SA_AIS_OK;
	do {
		retVal = saImmOiClassImplementerRelease(immOiHandle, className);
	} while (retVal == SA_AIS_ERR_TRY_AGAIN);
	return retVal;
}

SaAisErrorT autoRetry_saImmOiObjectImplementerSet(SaImmOiHandleT immOiHandle, const SaNameT *objectName, SaImmScopeT scope)
{
	return saImmOiObjectImplementerSet(immOiHandle, objectName, scope);
}

SaAisErrorT autoRetry_saImmOiObjectImplementerRelease(SaImmOiHandleT immOiHandle, const SaNameT *objectName, SaImmScopeT scope)
{
	SaAisErrorT retVal = SA_AIS_OK;
	do {
		retVal = saImmOiObjectImplementerRelease(immOiHandle, objectName, scope);
	} while (retVal == SA_AIS_ERR_TRY_AGAIN);
	return retVal;
}

SaAisErrorT autoRetry_saImmOiImplementerSet(SaImmOiHandleT immOiHandle, const SaImmOiImplementerNameT implementerName)
{
	return SA_AIS_OK;
}

SaAisErrorT autoRetry_saImmOiImplementerClear(SaImmOiHandleT immOiHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiRtObjectDelete_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOiRtObjectDelete(SaImmOiHandleT immOiHandle, const SaNameT *objectName)

{
	return saImmOiRtObjectDelete_rc;
}

SaAisErrorT saImmOiRtObjectUpdate_2_rc = SA_AIS_OK;
extern SaImmAttrValuesT_2** attrValues_Returned ;
SaAisErrorT autoRetry_saImmOiRtObjectUpdate_2(SaImmOiHandleT immOiHandle,
		const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	char strRlistLastId[] = "comsaRlistLastId";
	char strRlistNumElements[] = "comsaRlistNumElements";
	int index = 0;
	unsigned int * tmp;
	if (NULL != attrValues_Returned)
	{
		if(strcmp(attrMods[0]->modAttr.attrName, strRlistLastId) == 0)
		{
			index=0;
			while(attrValues_Returned[index] != NULL)
			{
				if( 0 == strcmp(attrValues_Returned[index]->attrName, strRlistLastId))
				{

					tmp = attrValues_Returned[index]->attrValues[0];
					*tmp = *((unsigned int *)attrMods[0]->modAttr.attrValues[0]);
				}
				index++;
			}
		}
		if(strcmp(attrMods[0]->modAttr.attrName, strRlistNumElements) == 0)
		{
			index=0;
			while(attrValues_Returned[index] != NULL)
			{
				if( 0 == strcmp(attrValues_Returned[index]->attrName, strRlistNumElements))
				{

					tmp = attrValues_Returned[index]->attrValues[0];
					*tmp = *((unsigned int *)attrMods[0]->modAttr.attrValues[0]);
				}
				index ++;
			}
		}
	}
	return saImmOiRtObjectUpdate_2_rc;
}

SaAisErrorT autoRetry_saImmOiAdminOperationResult_o2(SaImmOiHandleT immOiHandle,
		SaInvocationT invocation,
		SaAisErrorT result,
		const SaImmAdminOperationParamsT_2 **returnParams)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOiFinalize_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOiFinalize(SaImmOiHandleT immOiHandle)
{
	return saImmOiFinalize_rc;
}

SaAisErrorT saImmOmInitialize_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOmInitialize(SaImmHandleT *immHandle,
		const SaImmCallbacksT *immCallbacks, const SaVersionT *version)
{
	return saImmOmInitialize_rc;
}

SaAisErrorT autoRetry_saImmOmInitialize_getVersion(SaImmHandleT *immHandle,
		const SaImmCallbacksT *immCallbacks, SaVersionT *version)
{
	return saImmOmInitialize_rc;
}

SaAisErrorT saImmOmSearchInitialize_2_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOmSearchInitialize_2(SaImmHandleT immHandle,
		const SaNameT *rootName,
		SaImmScopeT scope,
		SaImmSearchOptionsT searchOptions,
		const SaImmSearchParametersT_2 *searchParam,
		const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
	return saImmOmSearchInitialize_2_rc;
}

SaAisErrorT saImmOmSearchNext_2_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOmSearchNext_2(SaImmSearchHandleT searchHandle,
		SaNameT *objectName, SaImmAttrValuesT_2 ***attributes)
{
	return saImmOmSearchNext_2_rc;
}

SaAisErrorT autoRetry_saImmOmSearchFinalize(SaImmSearchHandleT searchHandle)
{
	return SA_AIS_OK;
}

SaAisErrorT saImmOmFinalize_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOmFinalize(SaImmHandleT immHandle)
{
	return saImmOmFinalize_rc;
}


SaAisErrorT saImmAccessFinalize_rc = SA_AIS_OK;
/*
* Finalize the Object Access API towards the IMM database.
*
* @return	A SaAisErrorT value that in turn is from the IMM, see the IMM documentation for details.
*/
SaAisErrorT autoRetry_saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle)
{
	return saImmAccessFinalize_rc;
}


SaAisErrorT saImmAccessInitialize_rc = SA_AIS_OK;
/*
* Initialize the Object Access API towards the IMM database.
*
* @return	A SaAisErrorT value that in turn is from the IMM, see the IMM documentation for details.
*/
SaAisErrorT autoRetry_saImmOmAccessorInitialize(SaImmHandleT immHandle,
		SaImmAccessorHandleT *accessorHandle) {
	return saImmAccessInitialize_rc;
}


/*
* Request the IMM for a value of a specific attribute of a special object.
*
* @return	A SaAisErrorT enum that in turn is from the IMM, see the IMM documentation for details.
*/
SaAisErrorT saImmOmAccessorGet_2_rc = SA_AIS_OK;
SaImmAttrValuesT_2** attrValues_Returned = NULL;
SaAisErrorT autoRetry_saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
		const SaNameT *objectName,
		const SaImmAttrNameT *attributeNames,
		SaImmAttrValuesT_2 ***attributes) {
	if (attrValues_Returned != NULL) *attributes = attrValues_Returned;
	return saImmOmAccessorGet_2_rc;

}


/*
*  Register for a particular Mo instance
*
*  @return SA_AIS_OK if the operation succeeded and the OamSa is
* responsible for this dn, ComNotExist if the OamSa doesn't
* handle this dn, otherwise one of the other
* ComReturnT error codes
*/

/*
* Search for a class description in IMM. The function takes a immHandle and a className as input.
* It returns a pointer to a classCategory and a pointer to a pointer of an array of attribute definitions.
* The attrDefinitions must after use be release by a call to saImmOmClassDescriptionMemoryFree_2( .. ) function.
*/
SaAisErrorT saImmOmClassDescriptionGet_2_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOmClassDescriptionGet_2(SaImmHandleT immHandle,
		const SaImmClassNameT className,
		SaImmClassCategoryT *classCategory,
		SaImmAttrDefinitionT_2 ***attrDefinitions) {
	return saImmOmClassDescriptionGet_2_rc;
}

SaAisErrorT saImmOmClassCreate_2_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOmClassCreate_2(SaImmHandleT immHandle,
		const SaImmClassNameT className,
		SaImmClassCategoryT classCategory,
		const SaImmAttrDefinitionT_2 **attrDefinitions)
{
	return saImmOmClassCreate_2_rc;
}

SaAisErrorT saImmOmClassDelete_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOmClassDelete(SaImmHandleT immHandle, const SaImmClassNameT className)
{
	return saImmOmClassDelete_rc;
}

SaAisErrorT saImmOiRtObjectCreate_2_rc = SA_AIS_OK;
SaAisErrorT autoRetry_saImmOiRtObjectCreate_2(SaImmOiHandleT immOiHandle, const SaImmClassNameT className,
		const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	return saImmOiRtObjectCreate_2_rc;
}


static const char *saf_error_name[] = {
	"OUT_OF_RANGE",
	"SA_AIS_OK (1)",
	"SA_AIS_ERR_LIBRARY (2)",
	"SA_AIS_ERR_VERSION (3)",
	"SA_AIS_ERR_INIT (4)",
	"SA_AIS_ERR_TIMEOUT (5)",
	"SA_AIS_ERR_TRY_AGAIN (6)",
	"SA_AIS_ERR_INVALID_PARAM (7)",
	"SA_AIS_ERR_NO_MEMORY (8)",
	"SA_AIS_ERR_BAD_HANDLE (9)",
	"SA_AIS_ERR_BUSY (10)",
	"SA_AIS_ERR_ACCESS (11)",
	"SA_AIS_ERR_NOT_EXIST (12)",
	"SA_AIS_ERR_NAME_TOO_LONG (13)",
	"SA_AIS_ERR_EXIST (14)",
	"SA_AIS_ERR_NO_SPACE (15)",
	"SA_AIS_ERR_INTERRUPT (16)",
	"SA_AIS_ERR_NAME_NOT_FOUND (17)",
	"SA_AIS_ERR_NO_RESOURCES (18)",
	"SA_AIS_ERR_NOT_SUPPORTED (19)",
	"SA_AIS_ERR_BAD_OPERATION (20)",
	"SA_AIS_ERR_FAILED_OPERATION (21)",
	"SA_AIS_ERR_MESSAGE_ERROR (22)",
	"SA_AIS_ERR_QUEUE_FULL (23)",
	"SA_AIS_ERR_QUEUE_NOT_AVAILABLE (24)",
	"SA_AIS_ERR_BAD_FLAGS (25)",
	"SA_AIS_ERR_TOO_BIG (26)",
	"SA_AIS_ERR_NO_SECTIONS (27)",
	"SA_AIS_ERR_NO_OP (28)",
	"SA_AIS_ERR_REPAIR_PENDING (29)",
	"SA_AIS_ERR_NO_BINDINGS (30)",
	"SA_AIS_ERR_UNAVAILABLE (31)",
	"SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED (32)",
	"SA_AIS_ERR_CAMPAIGN_PROC_FAILED (33)",
	"SA_AIS_ERR_CAMPAIGN_CANCELED (34)",
	"SA_AIS_ERR_CAMPAIGN_FAILED (35)",
	"SA_AIS_ERR_CAMPAIGN_SUSPENDED (36)",
	"SA_AIS_ERR_CAMPAIGN_SUSPENDING (37)",
	"SA_AIS_ERR_ACCESS_DENIED (38)",
	"SA_AIS_ERR_NOT_READY (39)",
	"SA_AIS_ERR_DEPLOYMENT (40)"
};

const char *getOpenSAFErrorString(SaAisErrorT error)
{
	if ((int)error < 0 || error > SA_AIS_ERR_DEPLOYMENT)
	{
		return (char *)saf_error_name[0]; /* out of range */
	}

	return ((char *)saf_error_name[error]);
}
