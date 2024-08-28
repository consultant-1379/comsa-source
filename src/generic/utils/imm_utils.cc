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
 *	 File:	 imm_utils.cc
 *
 *	 Author: eozasaf
 *
 *	 Date:	 2011-09-20
 *
 *	 This file implements the helper functions to use when contacting IMM.
 *	 The design is heavily influenced by this file's previous and larger version
 *	 which was implemented by CoreMw.
 *
 *	 Mainly, the changes are:
 *	 Unnecessary functions have been removed, and the characteristics of Auto Retry
 *	 have been changed to (by default) continue indefinitely, instead of aborting;
 *	 as was decided by the ComSa DO team. (The duration of retry will be limited
 *	 by the timeout set for AMF CSI call-backs in this case.)
 *	 This characteristic can be reverted back to its previous version by making
 *	 errorsAreNonFatal 0.
 *
 *	Modify: uabjoy	 2012-02-14 added auto retry support for function saImmOmAccessorGet_2()
 *
 *	Reviewed: efaiami 2012-04-21
 *
 *	Modified: xnikvap 2013-01-04 changed to use saImmOiAdminOperationResult_o2() for SDP875
 *	Modified: xadaleg 2014-08-02 MR35347 - increase DN length
 *	Modified: xjonbuc 2014-12-12 MR-37637 Adapt IMM for replicated list service instead of CKPT
 *  Modified: xnikvap 2015-08-02 MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *****************************************************************************/

#include <unistd.h>
#include "imm_utils.h"
#include "trace.h"
#include "ComSA.h"


const SaVersionT imm_version = { immReleaseCode, immMajorVersion, immMinorVersion };
struct ImmAutoRetryProfile immAutoRetryProfile = { 1, 25, 400 };

/* ----------------------------------------------------------------------
 * IMM call wrappers; This wrapper interface off loads the burden to
 * handle return values and retries for each and every IMM-call. It
 * makes the code cleaner.
 */
SaAisErrorT autoRetry_saImmOiInitialize_2(SaImmOiHandleT *immOiHandle,
	const SaImmOiCallbacksT_2 *immOiCallbacks,
	const SaVersionT *version)
{
	ENTER_IMM_OI();
	/* Version parameter is in/out i.e. must be mutable and should not be
	   re-used from previous call in a retry loop. */
	SaVersionT localVer = *version;
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiInitialize_2(immOiHandle, immOiCallbacks, &localVer);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiInitialize_2() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		localVer = *version;
		rc = saImmOiInitialize_2(immOiHandle, immOiCallbacks, &localVer);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiInitialize_2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiInitialize FAILED, rc = %d", (int)rc);
	}
	imm_version_latest = localVer;
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiSelectionObjectGet(SaImmOiHandleT immOiHandle, SaSelectionObjectT *selectionObject)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiSelectionObjectGet(immOiHandle, selectionObject);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiSelectionObjectGet() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiSelectionObjectGet(immOiHandle, selectionObject);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiSelectionObjectGet() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiSelectionObjectGet FAILED, rc = %d", (int)rc);
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiClassImplementerSet(immOiHandle, className);
	while ( (rc == SA_AIS_ERR_TRY_AGAIN || rc == SA_AIS_ERR_EXIST) && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		if (rc == SA_AIS_ERR_TRY_AGAIN)
			LOG_IMM_OI("saImmOiClassImplementerSet() returned SA_AIS_ERR_TRY_AGAIN");
		else
			LOG_IMM_OI("saImmOiClassImplementerSet() returned SA_AIS_ERR_EXIST");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiClassImplementerSet(immOiHandle, className);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiClassImplementerSet() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiClassImplementerSet FAILED, rc = %d", (int)rc);
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiClassImplementerRelease(SaImmOiHandleT immOiHandle, const SaImmClassNameT className)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiClassImplementerRelease(immOiHandle, className);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiClassImplementerRelease() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiClassImplementerRelease(immOiHandle, className);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiClassImplementerRelease() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiClassImplementerRelease FAILED, rc = %d", (int)rc);
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiObjectImplementerSet(SaImmOiHandleT immOiHandle, const SaNameT *objectName, SaImmScopeT scope)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiObjectImplementerSet(immOiHandle, objectName, scope);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiObjectImplementerSet() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiObjectImplementerSet(immOiHandle, objectName, scope);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiObjectImplementerSet() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiObjectImplementerSet FAILED, rc = %d", (int)rc);
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiObjectImplementerRelease(SaImmOiHandleT immOiHandle, const SaNameT *objectName, SaImmScopeT scope)
{
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiObjectImplementerRelease(immOiHandle, objectName, scope);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiObjectImplementerSet() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiObjectImplementerRelease(immOiHandle, objectName, scope);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiObjectImplementerRelease() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiObjectImplementerRelease FAILED, rc = %d", (int)rc);
	}
	return rc;
}

SaAisErrorT autoRetry_saImmOiImplementerSet(SaImmOiHandleT immOiHandle, const SaImmOiImplementerNameT implementerName)
{
	ENTER_IMM_OI();
	DEBUG_IMM_OI("saImmOiImplementerSet : %s", (char*) implementerName);
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiImplementerSet(immOiHandle, implementerName);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiImplementerSet() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiImplementerSet(immOiHandle, implementerName);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiImplementerSet() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if ( rc != SA_AIS_OK )
	{
		if ( rc == SA_AIS_ERR_NO_RESOURCES )
		{
			LOG_IMM_OI("saImmOiImplementerSet receives SA_AIS_ERR_NO_RESOURCES, rc = %d", (int)rc);
		}
		else
		{
			ERR_IMM_OI("saImmOiImplementerSet FAILED, rc = %d", (int)rc);
		}
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiImplementerClear(SaImmOiHandleT immOiHandle)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiImplementerClear(immOiHandle);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiImplementerClear() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiImplementerClear(immOiHandle);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiImplementerClear() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiImplementerClear FAILED, rc = %d", (int)rc);
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiRtObjectCreate_2(SaImmOiHandleT immOiHandle, const SaImmClassNameT className,
			const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiRtObjectCreate_2(immOiHandle, className, parentName, attrValues);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiImplementerSet() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiRtObjectCreate_2(immOiHandle, className, parentName, attrValues);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiRtObjectCreate_2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		if (rc == SA_AIS_ERR_EXIST) {//Since object already exists, don't consider this as high-level log.
			DEBUG_IMM_OI("saImmOiRtObjectCreate_2 FAILED, rc = SA_AIS_ERR_EXIST");
		}
		else {
			ERR_IMM_OI("saImmOiRtObjectCreate_2 FAILED, rc = %d", (int)rc);
		}
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiRtObjectDelete(SaImmOiHandleT immOiHandle, const SaNameT *objectName)

{
        ENTER_IMM_OI();
        SaAisErrorT rc = SA_AIS_OK;
        unsigned int nTries = 1;
        rc = saImmOiRtObjectDelete(immOiHandle, objectName);
        while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
        {
                LOG_IMM_OI("saImmOiImplementerSet() returned SA_AIS_ERR_TRY_AGAIN");
                usleep(immAutoRetryProfile.retryInterval * 1000);
                rc = saImmOiRtObjectDelete(immOiHandle, objectName);
                nTries++;
                if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
                {
                        LOG_IMM_OI("Retried saImmOiRtObjectDelete() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
                }
        }
        if (rc != SA_AIS_OK)
        {
                ERR_IMM_OI("saImmOiRtObjectDelete FAILED, rc = %d", (int)rc);
        }
        LEAVE_IMM_OI();
        return rc;
}

SaAisErrorT autoRetry_saImmOiRtObjectUpdate_2(SaImmOiHandleT immOiHandle,
			const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiRtObjectUpdate_2() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiRtObjectUpdate_2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiRtObjectUpdate_2 FAILED, rc = %d", (int)rc);
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiAdminOperationResult_o2(SaImmOiHandleT immOiHandle,
													 SaInvocationT invocation,
													 SaAisErrorT result,
													 const SaImmAdminOperationParamsT_2 **returnParams)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiAdminOperationResult_o2(immOiHandle, invocation, result, returnParams);
	while (rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiAdminOperationResult_o2() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiAdminOperationResult_o2(immOiHandle, invocation, result, returnParams);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiAdminOperationResult_o2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiAdminOperationResult_o2 FAILED, rc = %d", (int)rc);
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOiFinalize(SaImmOiHandleT immOiHandle)
{
	ENTER_IMM_OI();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOiFinalize(immOiHandle);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_IMM_OI("saImmOiFinalize() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOiFinalize(immOiHandle);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_IMM_OI("Retried saImmOiFinalize() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_IMM_OI("saImmOiFinalize FAILED, rc = %d", (int)rc);
	}
	LEAVE_IMM_OI();
	return rc;
}

SaAisErrorT autoRetry_saImmOmInitializeAttempt(SaImmHandleT *immHandle,
						const SaImmCallbacksT *immCallbacks, SaVersionT *version)
{
	SaVersionT localVersion = *version;
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOmInitialize(immHandle, immCallbacks, &localVersion);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmInitialize() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmInitialize(immHandle, immCallbacks, &localVersion);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmInitialize() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA("saImmOmInitialize FAILED, rc = %d", (int)rc);
	} else {
		*version = localVersion;
	}
	return rc;
}
SaAisErrorT autoRetry_saImmOmInitialize(SaImmHandleT *immHandle,
						const SaImmCallbacksT *immCallbacks, const SaVersionT *version)
{
	ENTER_OAMSA();
	DEBUG_OAMSA("Enter autoRetry_saImmOmInitialize");
	SaVersionT localVersion = *version;
	SaAisErrorT rc = SA_AIS_OK;
	rc = autoRetry_saImmOmInitializeAttempt(immHandle, immCallbacks, &localVersion);
	LEAVE_OAMSA();
	return rc;
}

SaAisErrorT autoRetry_saImmOmInitialize_getVersion(SaImmHandleT *immHandle,
						const SaImmCallbacksT *immCallbacks, SaVersionT *version)
{
	ENTER_OAMSA();
	DEBUG_OAMSA("Enter autoRetry_saImmOmInitialize_getVersion");
	SaVersionT localVersion = *version;
	SaAisErrorT rc = SA_AIS_OK;
	rc = autoRetry_saImmOmInitializeAttempt(immHandle, immCallbacks, &localVersion);
	if (rc == SA_AIS_OK)
	{
		*version = localVersion;
	}
	LEAVE_OAMSA();
	return rc;
}

SaAisErrorT autoRetry_saImmOmSearchInitialize_2(SaImmHandleT immHandle,
		const SaNameT *rootName,
		SaImmScopeT scope,
		SaImmSearchOptionsT searchOptions,
		const SaImmSearchParametersT_2 *searchParam,
		const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOmSearchInitialize_2(immHandle,rootName,scope, searchOptions, searchParam,attributeNames,searchHandle);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmSearchInitialize_2() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmSearchInitialize_2(immHandle,rootName,scope, searchOptions,
				searchParam,attributeNames,searchHandle);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmSearchInitialize_2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA("saImmOmSearchInitialize_2 FAILED, rc = %d", (int)rc);
	}
	LEAVE_OAMSA();
	return rc;
}

SaAisErrorT autoRetry_saImmOmSearchNext_2(SaImmSearchHandleT searchHandle,
		SaNameT *objectName, SaImmAttrValuesT_2 ***attributes)
{
	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOmSearchNext_2(searchHandle,objectName,attributes);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmSearchNext_2() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmSearchNext_2(searchHandle,objectName,attributes);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmSearchNext_2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		if (rc == SA_AIS_ERR_NOT_EXIST) {
			DEBUG_OAMSA("saImmOmSearchNext_2, rc = SA_AIS_ERR_NOT_EXIST");
		}
		else {
			ERR_OAMSA("saImmOmSearchNext_2 FAILED, rc = %d", (int)rc);
		}
	}
	LEAVE_OAMSA();
	return rc;
}

SaAisErrorT autoRetry_saImmOmSearchFinalize(SaImmSearchHandleT searchHandle)
{
	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOmSearchFinalize(searchHandle);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmSearchFinalize() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmSearchFinalize(searchHandle);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmSearchFinalize() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA("saImmOmSearchFinalize FAILED, rc = %d", (int)rc);
	}
	LEAVE_OAMSA();
	return rc;
}

SaAisErrorT autoRetry_saImmOmFinalize(SaImmHandleT immHandle)
{
	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOmFinalize(immHandle);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmFinalize() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmFinalize(immHandle);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmFinalize() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA("saImmOmFinalize(%llu) FAILED, rc = %d", immHandle, (int)rc);
	}
	else
	{
		DEBUG_OAMSA("saImmOmFinalize(%llu) SUCCESS, rc = %d", immHandle, (int)rc);
	}
	LEAVE_OAMSA();
	return rc;
}
/*
 * Finalize the Object Access API towards the IMM database.
 *
 * @return	A SaAisErrorT value that in turn is from the IMM, see the IMM documentation for details.
 */
SaAisErrorT autoRetry_saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle) {
	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOmAccessorFinalize(accessorHandle);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmAccessorFinalize() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmAccessorFinalize(accessorHandle);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmAccessorFinalize() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA("saImmOmAccessorFinalize(%llu) FAILED, rc = %d", accessorHandle, (int)rc);
	}
	else
	{
		DEBUG_OAMSA("saImmOmAccessorFinalize(%llu) SUCCESS, rc = %d", accessorHandle, (int)rc);
	}
	LEAVE_OAMSA();
	return rc;
}
/*
 * Initialize the Object Access API towards the IMM database.
 *
 * @return	A SaAisErrorT value that in turn is from the IMM, see the IMM documentation for details.
 */
SaAisErrorT autoRetry_saImmOmAccessorInitialize(SaImmHandleT immHandle,
															SaImmAccessorHandleT *accessorHandle) {
	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOmAccessorInitialize(immHandle, accessorHandle);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmAccessorInitialize() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmAccessorInitialize(immHandle, accessorHandle);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmAccessorInitialize() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA("saImmOmAccessorInitialize FAILED, rc = %d", (int)rc);
	}
	LEAVE_OAMSA();
	return rc;
}
/*
 * Request the IMM for a value of a specific attribute of a special object.
 *
 * @return	A SaAisErrorT enum that in turn is from the IMM, see the IMM documentation for details.
 */
SaAisErrorT autoRetry_saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
										   const SaNameT *objectName,
										   const SaImmAttrNameT *attributeNames,
										   SaImmAttrValuesT_2 ***attributes) {

	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;
	rc = saImmOmAccessorGet_2(accessorHandle, objectName, attributeNames, attributes);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmAccessorGet_2() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmAccessorGet_2(accessorHandle, objectName, attributeNames, attributes);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmClassDescriptionGet_2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		if (rc == SA_AIS_ERR_NOT_EXIST)
		{
			DEBUG_OAMSA("saImmOmAccessorGet_2 return rc = SA_AIS_ERR_NOT_EXIST");
		}
		else
		{
			ERR_OAMSA("saImmOmAccessorGet_2 FAILED, rc = %d", (int)rc);
		}
	}
	LEAVE_OAMSA();
	return rc;
}

/*
 * Search for a class description in IMM. The function takes a immHandle and a className as input.
 * It returns a pointer to a classCategory and a pointer to a pointer of an array of attribute definitions.
 * The attrDefinitions must after use be release by a call to saImmOmClassDescriptionMemoryFree_2( .. ) function.
*/
SaAisErrorT autoRetry_saImmOmClassDescriptionGet_2(SaImmHandleT immHandle,
												   const SaImmClassNameT className,
												   SaImmClassCategoryT *classCategory,
												   SaImmAttrDefinitionT_2 ***attrDefinitions) {

	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;

	rc = saImmOmClassDescriptionGet_2(immHandle, className, classCategory, attrDefinitions);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmClassDescriptionGet_2() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmClassDescriptionGet_2(immHandle, className, classCategory, attrDefinitions);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmClassDescriptionGet_2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		if ((rc == SA_AIS_ERR_NOT_EXIST) || (rc == SA_AIS_ERR_BAD_HANDLE))
		{
			//search function returning 'not exist' can be excluded from high level logging.
			DEBUG_OAMSA("saImmOmClassDescriptionGet_2 return rc = %d", (int)rc);
		}
		else
		{
			ERR_OAMSA("saImmOmClassDescriptionGet_2 FAILED, rc = %d", (int)rc);
		}
	}
	LEAVE_OAMSA();
	return rc;
}


/*
 * creates a new object class with the name className. The new object
 * class can be a configuration or runtime object class, depending on the
 * classCategory parameter setting
*/
SaAisErrorT autoRetry_saImmOmClassCreate_2(SaImmHandleT immHandle,
										   const SaImmClassNameT className,
										   SaImmClassCategoryT classCategory,
										   const SaImmAttrDefinitionT_2 **attrDefinitions)
{
	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;

	DEBUG_OAMSA("Calling saImmOmClassCreate_2()...");
	rc = saImmOmClassCreate_2(immHandle, className, classCategory, attrDefinitions);
	DEBUG_OAMSA("After the 1-st call to saImmOmClassCreate_2() ... returned %d", rc);

	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmClassCreate_2() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmClassCreate_2(immHandle, className, classCategory, attrDefinitions);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmClassCreate_2() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		if (rc == SA_AIS_ERR_NOT_EXIST)
		{
			LOG_OAMSA("saImmOmClassCreate_2 FAILED, rc = %d", (int)rc);
		}
		else
		{
			ERR_OAMSA("saImmOmClassCreate_2 FAILED, rc = %d", (int)rc);
		}
	}
	LEAVE_OAMSA();
	return rc;
}


/*
* This function deletes the object class whose name is className, provided no
* objects of this class exist.
*/
SaAisErrorT autoRetry_saImmOmClassDelete(SaImmHandleT immHandle,
										 const SaImmClassNameT className) {

	ENTER_OAMSA();
	SaAisErrorT rc = SA_AIS_OK;
	unsigned int nTries = 1;

	rc = saImmOmClassDelete(immHandle, className);
	while(rc == SA_AIS_ERR_TRY_AGAIN && (nTries < immAutoRetryProfile.nTries || immAutoRetryProfile.errorsAreNonFatal) )
	{
		LOG_OAMSA("saImmOmClassDelete() returned SA_AIS_ERR_TRY_AGAIN");
		usleep(immAutoRetryProfile.retryInterval * 1000);
		rc = saImmOmClassDelete(immHandle, className);
		nTries++;
		if(nTries == immAutoRetryProfile.nTries && immAutoRetryProfile.errorsAreNonFatal)
		{
			LOG_OAMSA("Retried saImmOmClassDelete() %d times, continuing indefinitely",immAutoRetryProfile.retryInterval);
		}
	}
	if (rc != SA_AIS_OK)
	{
		ERR_OAMSA("saImmOmClassDelete FAILED, rc = %d", (int)rc);
	}
	LEAVE_OAMSA();
	return rc;
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
		return (char *)saf_error_name[0]; /* out of range */

	return ((char *)saf_error_name[error]);
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

void om_imm_finalize(SaImmHandleT immOmHandle, SaImmAccessorHandleT accessorHandle)
{
	ENTER_IMM_OM();
	SaAisErrorT error = SA_AIS_OK;
	DEBUG_IMM_OM("om_imm_finalize(): Finalizing the IMM Object Implementer handle %llu.", immOmHandle);
	if(immOmHandle != (SaImmHandleT)0)
	{
		if (accessorHandle != (SaImmAccessorHandleT)0)
		{
			if ((error = saImmOmAccessorFinalize(accessorHandle)) != SA_AIS_OK)
			{
				WARN_IMM_OM("om_imm_finalize(): saImmOmAccessorFinalize(%llu) FAILED: %s", accessorHandle, getOpenSAFErrorString(error));
			}
		}
		if ((error = autoRetry_saImmOmFinalize(immOmHandle))!= SA_AIS_OK)
		{
			WARN_IMM_OM("om_imm_finalize(): saImmOmFinalize FAILED: %s", getOpenSAFErrorString(error));
		}
		else
		{
			DEBUG_IMM_OM("om_imm_finalize(): Successfully finalized the immOmHandle");
		}
	}
	else
	{
		DEBUG_IMM_OM("om_imm_finalize(): immOmHandle is null, not calling saImmOmFinalize()");
	}
	LEAVE_IMM_OM();
}
