/******************************************************************************
 *	 Copyright (C) 2010 by Ericsson AB
 *	 S - 125 26	 STOCKHOLM
 *	 SWEDEN, tel int + 46 10 719 0000
 *
 *	 The copyright to the computer program herein is the property of
 *	 Ericsson AB. The program may be used and/or copied only with the
 *	 written permission from Ericsson AB, or in accordance with the terms
 *	 and conditions stipulated in the agreement/contract under which the
 *	 program has been supplied.
 *
 *	 All rights reserved.
 *
 *
 *	 File:	 imm_utils.h
 *
 *	 Author: eozasaf
 *
 *	 Date:	 2011-09-20
 *
 *	 This file defines the helper functions to use when contacting IMM
 *
 *	 Reviewed: efaiami 2012-04-21
 *	 Modified: xnikvap 2013-01-22 SDP875 support
 *	 Modified: xjonbuc 2014-12-12 MR-37637 Adapt IMM for replicated list service instead of CKPT
 *
 *****************************************************************************/
#ifndef IMMUTILS_h
#define IMMUTILS_h

#include "saAis.h"
#include "saImmOi.h"
#include "saImmOm.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ImmCallWrappers IMM call wrappers
 *
 *	  This wrapper interface offloads the burden to handle return
 *	  values and retries for each and every IMM-call. It makes the
 *	  code cleaner.
 */
extern const SaVersionT imm_version;

/**
 * Controls the behavior of the wrapper functions.
 */
	struct ImmAutoRetryProfile {
		int errorsAreNonFatal;
		unsigned int nTries;
		unsigned int retryInterval; // Interval in milli-seconds between tries.
	};

/**
 * Default: errorsAreNonFatal=1, nTries=25, retryInterval=400.
 */
	extern SaAisErrorT autoRetry_saImmOiInitialize_2(SaImmOiHandleT *immOiHandle,
													 const SaImmOiCallbacksT_2 *immOiCallbacks,
													 const SaVersionT *version);

	extern SaAisErrorT autoRetry_saImmOiSelectionObjectGet(SaImmOiHandleT immOiHandle,
														   SaSelectionObjectT *selectionObject);

	extern SaAisErrorT autoRetry_saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle,
															const SaImmClassNameT className);

	extern SaAisErrorT autoRetry_saImmOiClassImplementerRelease(SaImmOiHandleT immOiHandle,
																const SaImmClassNameT className);

	extern SaAisErrorT autoRetry_saImmOiObjectImplementerSet(SaImmOiHandleT immOiHandle,
															 const SaNameT *objectName,
															 SaImmScopeT scope);

	extern SaAisErrorT autoRetry_saImmOiObjectImplementerRelease(SaImmOiHandleT immOiHandle,
																 const SaNameT *objectName,
																 SaImmScopeT scope);

	extern SaAisErrorT autoRetry_saImmOiImplementerSet(SaImmOiHandleT immOiHandle,
													   const SaImmOiImplementerNameT implementerName);

	extern SaAisErrorT autoRetry_saImmOiImplementerClear(SaImmOiHandleT immOiHandle);


	extern SaAisErrorT autoRetry_saImmOiRtObjectCreate_2(SaImmOiHandleT immOiHandle,
														const SaImmClassNameT className,
														const SaNameT *parentName,
														const SaImmAttrValuesT_2 **attrValues);
	extern SaAisErrorT autoRetry_saImmOiRtObjectDelete(SaImmOiHandleT immOiHandle,
														const SaNameT *objectName);

	extern SaAisErrorT autoRetry_saImmOiRtObjectUpdate_2(SaImmOiHandleT immOiHandle,
														 const SaNameT *objectName,
														 const SaImmAttrModificationT_2 **attrMods);

	extern SaAisErrorT autoRetry_saImmOiAdminOperationResult_o2(SaImmOiHandleT immOiHandle,
																SaInvocationT invocation,
																SaAisErrorT result,
																const SaImmAdminOperationParamsT_2 **returnParams);
	extern SaAisErrorT autoRetry_saImmOiFinalize(SaImmOiHandleT immOiHandle);

	extern SaAisErrorT autoRetry_saImmOmInitialize(SaImmHandleT *immHandle,
												   const SaImmCallbacksT *immCallbacks,
												   const SaVersionT *version);

	extern SaAisErrorT autoRetry_saImmOmInitialize_getVersion(SaImmHandleT *immHandle,
												   const SaImmCallbacksT *immCallbacks,
												   SaVersionT *version);

	extern SaAisErrorT autoRetry_saImmOmSearchInitialize_2(SaImmHandleT immHandle,
														   const SaNameT *rootName,
														   SaImmScopeT scope,
														   SaImmSearchOptionsT searchOptions,
														   const SaImmSearchParametersT_2 *searchParam,
														   const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle);

	extern SaAisErrorT autoRetry_saImmOmSearchNext_2(SaImmSearchHandleT searchHandle,
													 SaNameT *objectName,
													 SaImmAttrValuesT_2 ***attributes);

	extern SaAisErrorT autoRetry_saImmOmSearchFinalize(SaImmSearchHandleT searchHandle);

	extern SaAisErrorT autoRetry_saImmOmFinalize(SaImmHandleT immHandle);

	extern const char *getOpenSAFErrorString(SaAisErrorT error);

	extern SaAisErrorT autoRetry_saImmOmClassDescriptionGet_2(SaImmHandleT immHandle,
															  const SaImmClassNameT className,
															  SaImmClassCategoryT *classCategory,
															  SaImmAttrDefinitionT_2 ***attrDefinitions);

	extern SaAisErrorT autoRetry_saImmOmAccessorInitialize(SaImmHandleT immHandle,
														   SaImmAccessorHandleT *accessorHandle);

	extern SaAisErrorT autoRetry_saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle);

	extern SaAisErrorT autoRetry_saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
													  const SaNameT *objectName,
													  const SaImmAttrNameT *attributeNames,
													  SaImmAttrValuesT_2 ***attributes);

	extern SaAisErrorT autoRetry_saImmOmClassCreate_2(SaImmHandleT immHandle,
													  const SaImmClassNameT className,
													  SaImmClassCategoryT classCategory,
													  const SaImmAttrDefinitionT_2 **attrDefinitions);

	extern SaAisErrorT autoRetry_saImmOmClassDelete(SaImmHandleT immHandle,
													const SaImmClassNameT className);
	extern SaImmAttrModificationT_2* allocateUint32AttrMod(const char* attrName, uint32_t value);
	extern SaImmAttrModificationT_2* allocateUint64AttrMod(const char* attrName, uint64_t value);
	extern SaImmAttrModificationT_2* allocateStringAttrMod(const char* attrName, const char* value);
	extern void freeAttrMod(SaImmAttrModificationT_2* attrMod);
	extern void om_imm_finalize(SaImmHandleT immOmHandle, SaImmAccessorHandleT accessorHandle);
#ifdef __cplusplus
}
#endif
#endif
