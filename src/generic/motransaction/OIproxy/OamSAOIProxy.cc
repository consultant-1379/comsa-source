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
 *   File:	OamSAOIProxy.cc
 *
 *   Author: uabjoy
 *
 *   Date:   2011-09-19
 *
 *   This file implements the Object Implementer inside ComSA which shall forward
 *   IMM callbacks to their mapped counterparts in the COM OIs
 *
 *   Modify: emilzor 2012-02-14
 *   Reviewed: efaiami 2012-04-21
 *   Modify: xnikvap, xngangu, xjonbuc, xduncao 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *
 *   Modify: eaparob 2012-10-16  Memory leak fix in OIProxy. Correction added to function releaseModifiedStructAttributes
 *   Modify: eaparob 2012-11-01  Fixed the unfinished transaction in OIProxy_handleRtAttrUpdate
 *   Modify: eaparob 2012-11-22  Modified part: "OIProxy_handleCcbCreateObject" -special translation added which differs from all the other callbacks
 *   Modify: eaparob 2012-12-19  Modified part: Changed the IMM2MO_DN translation to use "ImmRdn2MOTop" which used by "OIProxy_handleCcbCreateObject" and "OIProxy_handleComplexTypeCcbCreateObject" callbacks
 *   Modify: xnikvap, xduncao 2013-01-22  Implemented SDP875 Error strings from admin operations
 *   Modify: eaparob 2013-03-23  Fixed the multi OI handling in "openTransaction" function
 *   Modify: xjonbuc 2013-05-16  Implemented support for any return type from action()
 *	 Modify: uabjoy  2013-09-11  Fixing trouble report HR61815
 *	 Modify: eaparob 2013-12-19: hardcoded test debugs removed
 *	 Modify: xdonngu 2014-03-05: HS37161 - Make OI-Proxy for runtime mulltivalue structure (currently only struct with strings are supported).
 *	 Modify: xdonngu 2014-04-18: Fix wrong index. Fix some potential memory leaks/lacking support for moRef type.
 *	 Modify: xjonbuc 2014-06-23  Implement MR-20275 support for explicit ccb validate() & abort()
 *	 Modify: xadaleg 2014-08-02: MR35347 - increase DN length
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

#include <string>
#include <list>

#include <cstring>
#include <cctype>
#include <unistd.h>

#include "MafMgmtSpiInterface_1.h"
#include "MafOamSpiRegisterObjectImplementer_2.h"
#include "MafOamSpiServiceIdentities_1.h"
#include "MafOamSpiTransactionMaster_2.h"

#include "ComSA.h"
#include "OamSACache.h"
#include "OamSATranslator.h"
#include "trace.h"
#include "OamSAOIProxy.h"
#include "OamSAOIProxyStructCache.h"
#include "ImmCmd.h"
#include "imm_utils.h"
#include "OamSARegisterObjectUtils.h"
#include "DxEtModelConstants.h"
#include <assert.h>

extern OamSATranslator theTranslator;
char _CC_OI_NAME_PREFIX[] = CC_NAME;

//implicit definition

// IMM OI handle for main OI proxy
SaImmOiHandleT immOiHandle = 0;
// IMM OI applier handle for main OI proxy
SaImmOiHandleT immOiApplierHandle = 0;
// IMM OI handle for runtime structs

SaSelectionObjectT immSelectionObject;
SaSelectionObjectT immApplierSelectionObject;
SaSelectionObjectT immOiRTCTSelectionObject;
SaImmHandleT immOmHandle;
// Handle to the object access API
SaImmAccessorHandleT accessorHandleOI;

bool exit_OI_dispatch_thread = false;
pthread_t dispatch_thread;

MafMgmtSpiInterfacePortal_3T* InterfacePortal = NULL;
static MafMgmtSpiThreadContext_2T* threadContext = NULL;
static MafMgmtSpiInterface_1T theThreadContext = MafMgmtSpiThreadContext_2Id;

MafOamSpiTransactionMaster_2T *txMaster = NULL;
const unsigned int TransactionTimeoutValue = 3600;

std::map<std::string, MafOamSpiTransactionHandleT> ccbMap;
typedef std::list<std::string> runtimeStructInstancesT;
typedef runtimeStructInstancesT::iterator runtimeStructInstancesIteratorT;
runtimeStructInstancesT runtimeStructInstances;
typedef std::map<const SaImmAttrNameT, MafMoAttributeValueResult_3T> runtimeAttrMapT;
typedef runtimeAttrMapT::iterator runtimeAttrMapIteratorT;

void OIProxy_handleAdminOperation(SaImmOiHandleT immOiHandle, SaInvocationT invocation, const SaNameT *objectName,
				SaImmAdminOperationIdT operationId, const SaImmAdminOperationParamsT_2 **params);
void OIProxy_handleCcbAbort(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
void OIProxy_handleCcbApply(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
SaAisErrorT OIProxy_handleCcbCompleted(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId);
SaAisErrorT OIProxy_handleCcbCreateObject(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
											const SaImmClassNameT className,
											const SaNameT *parentName,
											const SaImmAttrValuesT_2 **attr);
SaAisErrorT OIProxy_handleCcbDeleteObject(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaNameT *objectName);
SaAisErrorT OIProxy_handleCcbModification(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
											const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods);
SaAisErrorT OIProxy_handleRtAttrUpdate(SaImmOiHandleT immOiHandle, const SaNameT *objectName, const SaImmAttrNameT *attributeNames);

MafOamSpiTransactionMaster_2T *maf_txMaster = NULL;

std::map<std::string, MafOamSpiTransactionHandleT> maf_ccbMap;

//To avoid recursive calls, retry variable for handling SA_AIS_ERR_NO_RESOURCES case
short nRetries = 0;
/**
 * Remove the variable ptr from the environment
 */
void unsetFromEnvironment(const char *ptr) {
        LOG_OIPROXY("unsetFromEnvironment(%s) is called", ptr);
        char *value = getenv(ptr);
        if(NULL != value) {
                LOG_OIPROXY("getenv(%s) returns (%s), hence trying to unsetenv", ptr,value);
                unsetenv(ptr);
                value = getenv(ptr);
                if(NULL != value) {
                        LOG_OIPROXY("getenv(%s) returns (%s) after unsetenv, something is wrong here!!", ptr,value);
                }
                else {
                        LOG_OIPROXY("getenv(%s) succesfully returns NULL after unsetenv", ptr);
                }
        }
}
/**
 * The function  return the basic type for a derived type.
 *
 * @param	moAttr		The MoAttribute definition in the Model Repository version one.
 *
 * @return				the basic type for the attribute
 */
// Correction for TR HR62291
// When the COM MO SPI3 was introduced, the handling of the DERIVED type changed
// COM MO SPI3 use Model repository version 3 and the type DERIVED not defined as a type of it own any more.
// To compensate for this, we convert the DERIVED type here to it's basic type, which are the same
// and since COM SA only cares about the basic type. IT will work.
MafOamSpiMoAttributeTypeT CorrectBaseType(MafOamSpiMoAttributeT *moAttr)
{
	ENTER_OIPROXY();
	MafOamSpiMoAttributeTypeT basetype;
	if (moAttr->type == MafOamSpiMoAttributeType_DERIVED)
	{
		// OK, convert to the basic type here
		if (moAttr->derivedDatatype != NULL)
		{
			basetype = (moAttr->derivedDatatype)->type;
		}
		else
		{
			// OK there is no base type, then keep it as before and fail the handling of this.
			// Issue a warning in the syslog
			ERR_OIPROXY("unknown MafOamSpiMoAttributeType_DERIVED found in the model repository");
			// take the type as is
			basetype = moAttr->type;
		}
	}
	else
	{
		// take the type as is and use it
		basetype = moAttr->type;
	}
	LEAVE_OIPROXY();
	return basetype;
}

/**
 * The function  return the basic type for a derived type.
 *
 * @param	memberType		Type of StructMember or Parameter definition in the Model Repository version one.
 *
 * @return				the basic type for the attribute
 */
// When the COM MO SPI3 was introduced, the handling of the DERIVED type changed
// COM MO SPI3 use Model repository version 3 and the type DERIVED not defined as a type of it own any more.
// To compensate for this, we convert the DERIVED type here to it's basic type, which are the same
// and since COM SA only cares about the basic type. IT will work.
MafOamSpiMoAttributeType_3 CorrectBaseType(MafOamSpiDatatypeContainerT memberType)
{
	ENTER_OIPROXY();
	MafOamSpiMoAttributeType_3 basetype;
	if (memberType.type == MafOamSpiDatatype_DERIVED)
	{
		// OK, convert to the basic type here
		if (memberType.derivedDatatype != NULL)
		{
			basetype = (MafOamSpiMoAttributeType_3) memberType.derivedDatatype->type;
		}
		else
		{
			// OK there is no base type, then keep it as before and fail the handling of this.
			// Issue a warning in the syslog
			ERR_OIPROXY("unknown MafOamSpiMoAttributeType_DERIVED found in the model repository");
			// take the type as is
			basetype = (MafOamSpiMoAttributeType_3) memberType.type;
		}
	}
	else
	{
		// take the type as is and use it
		basetype = (MafOamSpiMoAttributeType_3) memberType.type;
	}
	LEAVE_OIPROXY();
	return basetype;
}

/**
 * Delete memory allocated for this structure
 * NOTE !!! This function assumes that this structure not
 * contains struct, i.e. complex datatypes!
 * It will not work for this case.
 *
 * @param	ctp	attribute value container that will be erased
 */
void emptyAttributeValueContainer(MafMoAttributeValueContainer_3T *ctp) {
	ENTER_OIPROXY();
	if (ctp!=NULL && ctp->nrOfValues>0 && ctp->values!=NULL)
	{
		// OKEJ, loop over the memory and delete it!
		for (unsigned int index=0;index<ctp->nrOfValues;index++)
		{
			// Allocated memory only for string
			if (ctp->type == MafOamSpiMoAttributeType_3_STRING ||
					ctp->type == MafOamSpiMoAttributeType_3_REFERENCE
					)
			{
				delete [] ctp->values[index].value.theString;
			}
		}
	}
	// delete the values field
	if (ctp != NULL) {
		delete [] ctp->values;
	}
	LEAVE_OIPROXY();
}

/**
 * The function that will return value from a certain maps
 *
 * @param	map	the map from which will be returned a values
 * @param	key	key search in the map
 * @param	out	the value of the key
 * @return	if the key exists in the map, true will be returned,
 * 			otherwise the function will return false
 */
template <class Key, class Value, class Comparator, class Alloc>
bool getValue(const std::map<Key, Value, Comparator, Alloc>& map, Key key, Value& out)
{
	ENTER_OIPROXY();
	bool retVal = false;
	typename std::map<Key, Value, Comparator, Alloc>::const_iterator it = map.find(key);
	if (it != map.end())
	{
		out = it->second;
		retVal = true;
	}
	LEAVE_OIPROXY();
	return retVal;
}

/**
 * This method retrieves the parameter type for the parameter provided in paramName
 * It sets the paramType equal to the found type.
 *
 * @param	paramName             parameter name
 * @param	pParameterList        Com SPI parameter list
 * @param	paramType             return parameter type value
 * @param	isMoSpiVersion2       true if MO SPI version 2 is used.
 * @return	true if the parameter exists, otherwise false
 */
bool RetriveComParameterType(char *paramName,
							 MafOamSpiParameterT *pParameterList,
							 MafOamSpiDatatypeT &paramType,
							 bool isMoSpiVersion2 = false)
{
	ENTER_OIPROXY();
	bool retVal = false;
	while(pParameterList!=NULL)
	{
		// Check if we found the parameter search for
		if (!strcmp(paramName, pParameterList->generalProperties.name)) {
			DEBUG_OIPROXY("RetriveComParameterType() : Found parameter type for %s", paramName);
			if (isMoSpiVersion2){
				paramType = (MafOamSpiDatatypeT)pParameterList->parameterType.type;
			}
			else
			{
				paramType = (MafOamSpiDatatypeT) CorrectBaseType(pParameterList->parameterType);
			}
			retVal = true;
			break;
		}
			// get the next parameter in the list
		pParameterList=pParameterList->next;
	}
	LEAVE_OIPROXY();
	return retVal;
}

/**
 * Translate from IMM to MOM format for the full list of all parameters coming from IMM.
 * This function assume that the attributeValue pointer points to already
 * allocated memory.
 *
 * @param	params			parameters
 * @param	attributeValue	return the attribute value
 * @param	parameterOrder	parameter order
 * @param	pParameterList	parameter list
 * @return	return SA_AIS_OK if the translation is successful,
 * 			otherwise return SA_AIS_ERR_BAD_OPERATION
 */
SaAisErrorT OIProxy_translateImm2Mo(const SaImmAdminOperationParamsT_2 **params,
									MafMoNamedAttributeValueContainer_3T *attributeValue,
									const std::map<std::string,int> &parameterOrder,
									MafOamSpiParameterT *pParameterList)
{
	ENTER_OIPROXY();
	// NOTE that this function needs to create the anonymous attributeValue list in the correct order
	// This order is defined in the the parameterOrder map.
	if (params!=NULL)
	{
		// loop over all in parameters and translate them one by one
		for(unsigned int i=0;params[i]!=NULL;i++)
		{
			// Find where to put the data in the array of parameters
			int orderOfParameter=0;
			if (!getValue(parameterOrder, std::string(params[i]->paramName), orderOfParameter))
			{
				// Bad things in, bad things out.
				DEBUG_OIPROXY("OIProxy_translateImm2Mo() : No Parameter Order defined");
				DEBUG_OIPROXY("OIProxy_translateImm2Mo() : Did not found parmeter %s", params[i]->paramName);
				LEAVE_OIPROXY();
				return SA_AIS_ERR_BAD_OPERATION;
			}
			DEBUG_OIPROXY("OIProxy_translateImm2Mo() : orderOfParameter=%i", orderOfParameter);
			// Get the attributeType from the MOM
			MafOamSpiDatatypeT paramType;
#ifndef UNIT_TEST
			if(!RetriveComParameterType(params[i]->paramName, pParameterList, paramType))
			{
#endif
#ifdef UNIT_TEST
			extern bool RetriveComAttributeType(MafOamSpiDatatypeT &paramType);
			{
#endif
				ERR_OIPROXY("OIProxy_translateImm2Mo() : COM OI Failed to find AttributeType");
				LEAVE_OIPROXY();
				return SA_AIS_ERR_BAD_OPERATION;
			}
                        attributeValue[orderOfParameter].name = params[i]->paramName;
			attributeValue[orderOfParameter].value.type = (MafOamSpiMoAttributeType_3T) (paramType);
			// Set the nrOfValues always to 1
			attributeValue[orderOfParameter].value.nrOfValues = 1;
			// Convert the Value Field, a simple cast here.
			attributeValue[orderOfParameter].value.values = (struct MafMoAttributeValue_3 *) (params[i]->paramBuffer);
		}
	}
	else
	{
		// Bad things in, bad things out.
		LEAVE_OIPROXY();
		return SA_AIS_ERR_BAD_OPERATION;
	}
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/**
 * Translate from IMM to MOM format for the full list of all parameters coming from IMM.
 * This function assume that the attributeValue pointer points to already
 * allocated memory.
 *
 * @param	params			parameters
 * @param	attributeValue	return the attribute value
 * @param	parameterOrder	parameter order
 * @param	pParameterList	parameter list
 * @return	return SA_AIS_OK if the translation is successful,
 * 			otherwise return SA_AIS_ERR_BAD_OPERATION
 */
SaAisErrorT OIProxy_translateImm2Mo(const SaImmAdminOperationParamsT_2 **params,
									MafMoNamedAttributeValueContainerT *attributeValue,
									const std::map<std::string,int> &parameterOrder,
									MafOamSpiParameterT *pParameterList)
{
	ENTER_OIPROXY();
	// NOTE that this function needs to create the anonymous attributeValue list in the correct order
	// This order is defined in the the parameterOrder map.
	if (params!=NULL)
	{
		// loop over all in parameters and translate them one by one
		for(unsigned int i=0;params[i]!=NULL;i++)
		{
			// Find where to put the data in the array of parameters
			int orderOfParameter=0;
			if (!getValue(parameterOrder, std::string(params[i]->paramName), orderOfParameter))
			{
				// Bad things in, bad things out.
				DEBUG_OIPROXY("OIProxy_translateImm2Mo() : No Parameter Order defined");
				DEBUG_OIPROXY("OIProxy_translateImm2Mo() : Did not found parmeter %s", params[i]->paramName);
				LEAVE_OIPROXY();
				return SA_AIS_ERR_BAD_OPERATION;
			}
			DEBUG_OIPROXY("OIProxy_translateImm2Mo() : orderOfParameter=%i", orderOfParameter);
			// Get the attributeType from the MOM
			MafOamSpiDatatypeT paramType;
#ifndef UNIT_TEST
			if(!RetriveComParameterType(params[i]->paramName, pParameterList, paramType, true))
			{
#endif
#ifdef UNIT_TEST
			extern bool RetriveComAttributeType(MafOamSpiDatatypeT &paramType);
			{
#endif
				ERR_OIPROXY("OIProxy_translateImm2Mo() : COM OI Failed to find AttributeType");
				LEAVE_OIPROXY();
				return SA_AIS_ERR_BAD_OPERATION;
			}
                        attributeValue[orderOfParameter].name = params[i]->paramName;
			attributeValue[orderOfParameter].value.type = (MafOamSpiMoAttributeType_2T) (paramType);
			// Set the nrOfValues always to 1
			attributeValue[orderOfParameter].value.nrOfValues = 1;
			// Convert the Value Field, a simple cast here.
			attributeValue[orderOfParameter].value.values = (struct MafMoAttributeValue_2*) (params[i]->paramBuffer);
		}
	}
	else
	{
		// Bad things in, bad things out.
		LEAVE_OIPROXY();
		return SA_AIS_ERR_BAD_OPERATION;
	}
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/**
 * Map the COM result to available IMM error codes.
 *
 * @param	maf_rc	Maf error code
 * @return	appropriate SA error code
 */
SaAisErrorT mapComCodeToImmCode(MafReturnT maf_rc) {
	ENTER_OIPROXY();
	SaAisErrorT retVal = SA_AIS_OK;
	if ( maf_rc == MafOk ) {
		retVal = SA_AIS_OK;
	}else if ( maf_rc == MafTryAgain ) {
		retVal = SA_AIS_ERR_TRY_AGAIN;
	}else if ( maf_rc == MafNotActive ) {
		retVal = SA_AIS_ERR_NOT_READY;
	}else if ( maf_rc == MafFailure ) {
		retVal = SA_AIS_ERR_FAILED_OPERATION;
	}else if( maf_rc == MafNotExist ) {
		retVal = SA_AIS_ERR_NOT_EXIST;
	}else if( maf_rc == MafAlreadyExist ) {
		retVal = SA_AIS_ERR_EXIST;
	}else if( maf_rc == MafAborted     ||
			maf_rc == MafObjectLocked  ||
			maf_rc == MafPrepareFailed ||
			maf_rc == MafCommitFailed ) {
		retVal = SA_AIS_ERR_BAD_OPERATION;
	}else if( maf_rc == MafInvalidArgument ) {
		retVal = SA_AIS_ERR_INVALID_PARAM;
	}else if( maf_rc == MafValidationFailed ) {
		retVal = SA_AIS_ERR_BAD_OPERATION;
	}else if( maf_rc == MafNoResources ) {
		retVal = SA_AIS_ERR_NO_RESOURCES;
	}else if( maf_rc == MafTimeOut ) {
		retVal = SA_AIS_ERR_TIMEOUT;
	}else {
		retVal = SA_AIS_ERR_BAD_OPERATION;
	}
	LEAVE_OIPROXY();
	return retVal;
}

////////////////////////////////

/**
 * The function returns all valid attribute values from input attribute value list
 * If icComplexType is set to <b>true</b>, <b>id</b> attribute will not be included
 * in the output list.
 * SaImmAttrAdminOwnerName, SaImmAttrClassName and SaImmAttrImplementerName attributes
 * belong to IMM, and they will be excluded from the output list.
 *
 * @param	attr			attribute value list. The list is NULL terminated
 * @param	className		class name of the object where attribute values belong to
 * @param	isComplexType	is attribute list belongs to the object or complex type attribute
 * @param	attrList		output attribute list of attribute values
 * @return	number of attribute in the output list, on an error return -1
 */
int getImmClassAttributes(SaImmAttrValuesT_2 **attr, bool isComplexType, std::list<SaImmAttrValuesT_2 *> &attrList) {
	ENTER_OIPROXY();
	//	Check input parameters
	if(!attr) {
		LEAVE_OIPROXY();
		return -1;
	}
	//	Go through all attributes...
	int count = 0;
	int index = 0;
	while(attr[index]) {
		//	... and check if they are valid attributes
		if(((strcmp(attr[index]->attrName, "id") && isComplexType) || !isComplexType) &&
				strcmp(attr[index]->attrName, "SaImmAttrAdminOwnerName") &&
				strcmp(attr[index]->attrName, "SaImmAttrClassName") &&
				strcmp(attr[index]->attrName, "SaImmAttrImplementerName")) {

			attrList.push_front(attr[index]);
			count++;
		}
		index++;
	}
	LEAVE_OIPROXY();
	return count;
}

/**
 * Find RDN from the input attribute values, and return its value
 *
 * @param	attr		attribute values that are NULL terminated
 * @param	className	class name of attribute values object
 * @return	attribute value of RDN attribute, on error NULL
 */
SaImmAttrValuesT_2 *getRDNAttribute(SaImmAttrValuesT_2 **attr, char *className) {
	ENTER_OIPROXY();
	SaAisErrorT retVal = SA_AIS_OK;
	//	Check input parameters
	if(!attr || !className) {
		LEAVE_OIPROXY();
		return NULL;
	}

	//	Get all class attributes
	SaImmClassCategoryT classCat;
	SaImmAttrDefinitionT_2 **attrDef = NULL;

	retVal = autoRetry_saImmOmClassDescriptionGet_2(immOmHandle, className, &classCat, &attrDef);

	if (SA_AIS_ERR_BAD_HANDLE == retVal)
	{
		LOG_OIPROXY("saImmOmClassDescriptionGet_2() returned BAD_HANDLE, reinitializing OI handlers before another attempt");

		ObjImp_finalize_imm(true,false);
		ObjImp_init_imm(true);

		retVal = autoRetry_saImmOmClassDescriptionGet_2(immOmHandle, className, &classCat, &attrDef);
	}

	if(retVal != SA_AIS_OK)
	{
		DEBUG_OIPROXY("Cannot get class (%s) description", className);
		LEAVE_OIPROXY();
		return NULL;
	}

	if(!attrDef) {
		DEBUG_OIPROXY("attrDef is NULL");
		LEAVE_OIPROXY();
		return NULL;
	}
	if(!*attrDef) {
		retVal = saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);
		if (retVal != SA_AIS_OK)
		{
			LOG_OIPROXY("saImmOmClassDescriptionMemoryFree_2() failed retVal = %d",(int)retVal);
		}
		DEBUG_OIPROXY("List of attributes is empty");
		LEAVE_OIPROXY();
		return NULL;
	}

	//	Check which attribute contains RDN flag
	int i = 0;
	while(attrDef[i]) {
		if((attrDef[i]->attrFlags & SA_IMM_ATTR_RDN))
			break;
		i++;
	}

	if(!attrDef[i]) {
		retVal = saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);
		if (retVal != SA_AIS_OK)
		{
			LOG_OIPROXY("saImmOmClassDescriptionMemoryFree_2() failed retVal = %d",(int)retVal);
		}
		DEBUG_OIPROXY("Cannot find RDN in attribute definition");
		LEAVE_OIPROXY();
		return NULL;
	}

	//	Check which attribute is RDN, and return its attribute value
	char *rdn = attrDef[i]->attrName;
	i = 0;
	while(attr[i])
	{
		if(!strcasecmp(attr[i]->attrName, rdn))
		{
			retVal = saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);
			if (retVal != SA_AIS_OK)
			{
				LOG_OIPROXY("saImmOmClassDescriptionMemoryFree_2() failed retVal = %d",(int)retVal);
			}
			LEAVE_OIPROXY();
			return attr[i];
		}
		i++;
	}

	//	Release allocated memory by saImmOmClassDescriptionGet_2
	retVal = saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);
	if (retVal != SA_AIS_OK)
	{
		LOG_OIPROXY("saImmOmClassDescriptionMemoryFree_2() failed retVal = %d",(int)retVal);
	}
	//	RDN attribute cannot be found. Then return NULL
	LEAVE_OIPROXY();
	return NULL;
}

/**
 * Open anonymous transaction that will be used in run-time attribute update callback.
 * In run-time update callback, ccbId is not provided, and the already opened transaction
 * cannot be used, so, new anonymous transaction will be opened.
 *
 * @param	spiIf		Common registered struct that contains transaction resource interface
 * @param	txHandle	transaction handle that will contain new open transaction handle
 * @return	if the function successful open new transaction, true will be returned, otherwise false
 */
bool openAnonymousTransaction(RegisteredCommonStructT *spiIf, MafOamSpiTransactionHandleT *txHandle) {
	ENTER_OIPROXY();
	//	Check input parameter
	if(!spiIf || !txHandle || !InterfacePortal) {
		LEAVE_OIPROXY();
		return false;
	}

	//	Get transaction resource interface
	MafReturnT ret = MafOk;
	MafOamSpiTransactionalResource_2T *txResourceIf;
	ret = InterfacePortal->getInterface(spiIf->transactionalResourceId, (MafMgmtSpiInterface_1T**)&txResourceIf);
	if(ret !=MafOk){
		ERR_OIPROXY("Failed to get the MafOamSpiTransactionalResource_2 interface from the portal, rc = %d", ret);
		LEAVE_OIPROXY();
		return false;
	}

	//	Create new master transaction
	if((ret = txMaster->newTransaction(MafLockWrite, TransactionTimeoutValue, txHandle)) != MafOk) {
		ERR_OIPROXY("New transaction couldn't be started rc = %d", ret);
		LEAVE_OIPROXY();
		return false;
	}

	//	Check if the transaction has already been registered
	bool registered=false;
	if((ret = txMaster->isRegistered(*txHandle, txResourceIf, &registered)) != MafOk) {
		ERR_OIPROXY("call to txMaster->isRegistered() failed for the txResource %s, rc = %d", spiIf->transactionalResourceId.interfaceName, ret);
		LEAVE_OIPROXY();
		return false;
	}

	//	If it has not been registered, then join to the master transaction
	if(!registered) {
		if((ret = txResourceIf->join(*txHandle)) != MafOk) {
			ERR_OIPROXY("Transactional Resource %s failed to join transaction rc = %d", spiIf->transactionalResourceId.interfaceName, ret);
			LEAVE_OIPROXY();
			return false;
		}
	}
	LEAVE_OIPROXY();
	return true;
}

/**
 * Open a transaction.
 * If the transaction has already been opened for the same transaction resource and ccbId,
 * the function will return already opened transaction handle from the ccbId transaction cache.
 * If the transaction is not created, new transaction for given transaction resource interface
 * will be created, and saved to ccb transaction cache for later useage.
 *
 * @param	ccbId		ccbId from callback function
 * @param	spiIf		Common registered structure (dn or class) that contains transaction resource interface
 * @param	txHandle	txHandle contains output value of open transaction handle
 * @return	true on successful opening transaction, otherwise false
 */
bool openTransaction(SaImmOiCcbIdT ccbId, RegisteredCommonStructT *spiIf, MafOamSpiTransactionHandleT *txHandle) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("openTransaction(): ENTER with ccbId:(%llu)",ccbId);
	//	Check input parameters
	if(!spiIf || !txHandle || !InterfacePortal)
	{
		LEAVE_OIPROXY();
		return false;
	}

	//	Get transaction resource interface
	MafReturnT ret = MafOk;
	MafOamSpiTransactionalResource_2T *txResourceIf;
	ret = InterfacePortal->getInterface(spiIf->transactionalResourceId, (MafMgmtSpiInterface_1T**)&txResourceIf);
	if(ret != MafOk)
	{
		ERR_OIPROXY("Failed to get the MafOamSpiTransactionalResource_2 interface from the portal, rc = %d", ret);
		LEAVE_OIPROXY();
		return false;
	}

	/* Check if the transaction for the ccbId has already been created
	 * If exists:
	 *                -get txHandle from the map
	 *
	 * If not exists:
	 *                -create new transaction
	 *                -add ccbId and txHandle (as key-value pairs) to the map
	 */
	if(existCcbTransaction(ccbId))
	{
		DEBUG_OIPROXY("openTransaction(): existCcbTransaction(%llu): ccbId exists, calling getCcbTransaction()",ccbId);
		// FIXME: return value not checked.
		getCcbTransaction(ccbId, txHandle);
	}
	else
	{
		DEBUG_OIPROXY("openTransaction(): existCcbTransaction(%llu): ccbId not exists, creating new transaction",ccbId);
		//	Create new master transaction
		if((ret = txMaster->newTransaction(MafLockWrite, TransactionTimeoutValue, txHandle)) != MafOk)
		{
			ERR_OIPROXY("New transaction couldn't be started rc = %d", ret);
			DEBUG_OIPROXY("openTransaction(): RETURN false");
			LEAVE_OIPROXY();
			return false;
		}
		//	Add main transaction to the cache
		// FIXME: return value not checked. Method doesn't called elsewhere: then why does "addNewCcbTransaction" function have return a value?
		addNewCcbTransaction(ccbId, *txHandle);
	}
	//	Check if the transaction resource interface has already been registered
	bool registered = false;
	if((ret = txMaster->isRegistered(*txHandle, txResourceIf, &registered)) != MafOk)
	{
		ERR_OIPROXY("call to txMaster->isRegistered() failed for the txResource %s, rc = %d", spiIf->transactionalResourceId.interfaceName, ret);
		LEAVE_OIPROXY();
		return false;
	}

	//	If it's not registered, then join transaction resource interface to the master transaction
	if(!registered)
	{
		if((ret = txResourceIf->join(*txHandle)) != MafOk)
		{
			ERR_OIPROXY("Transactional Resource %s failed to join transaction rc = %d", spiIf->transactionalResourceId.interfaceName, ret);
			LEAVE_OIPROXY();
			return false;
		}
	}
	DEBUG_OIPROXY("openTransaction(): RETURN true");
	LEAVE_OIPROXY();
	return true;
}

/**
 * getMoIf function return managed object interface for a managed object SPI interface
 *
 * @param	spiIf	Common registered (dn or class) structure that contains managed object SPI interface
 * @param	moIf	output managed object interface
 * @return	true, on any error returns false
 */
bool getMoIf(RegisteredCommonStructT *spiIf, MafOamSpiManagedObject_3T **moIf) {
	ENTER_OIPROXY();
	//	Check input parameters
	if(!spiIf || !InterfacePortal) {
		LEAVE_OIPROXY();
		return false;
	}

	//	Get managed object interface from interface portal
	MafReturnT ret = InterfacePortal->getInterface(spiIf->managedObjectInterfaceId, (MafMgmtSpiInterface_1T**)moIf);
	if(MafOk != ret)
	{
		ERR_OIPROXY("Failed to get the MafOamSpiManagedObject_3T interface from the portal, rc = %d", ret);
		LEAVE_OIPROXY();
		return false;
	}
	LEAVE_OIPROXY();
	return true;
}

/**
 * getMoIfVersion function return managed object interface for a managed object SPI interface
 *
 * @param	spiIf	Common registered (dn or class) structure that contains managed object SPI interface
 * @param	version	version of i/f
 * @return	true, on any error returns false
 */
bool getMoIfVersion(const RegisteredCommonStructT *spiIf, RegisteredMoInterfaceT& version)
{
	if(NULL == spiIf)
	{
		DEBUG_OIPROXY("getMoIfVersion() Cannot access Registered interface");
		return false;
	}

	if (0 == strcmp("2", spiIf->managedObjectInterfaceId.interfaceVersion))
	{
		version = MafOamSpiManagedObject2;
	}
	else
	{
		version = MafOamSpiManagedObject3;
	}
	DEBUG_OIPROXY("getMoIfVersion() Registered MO interface: %d", (int)version);
	return true;
}

/**
 * The function release allocated memory by createAttributeContainerContent function.
 * All memory allocated within attrCnt parameter will be freed.
 * If bAll is set to true (!= 0), the memory of attrCnt will be also deleted,
 * otherwise its allocated memory will remain in memory.
 *
 * @param	attrCnt		attribute container which memory needs to be freed
 * @param	bAll		true if attrCnt memory needs to be freed also, otherwise it will remain in memory
 */
void freeAttributeContainerContent(MafMoAttributeValueContainer_3T *attrCnt, int bAll) {	//	bAll = true (free memory of attrCnt)
	ENTER_OIPROXY();

	//	Check input parameter
	if(!attrCnt) {
		LEAVE_OIPROXY();
		return;
	}

	//	Free allocated memory
	// HS37161
	if(attrCnt->values)
	{
		if(attrCnt->type == MafOamSpiMoAttributeType_3_STRING)
		{
			for(unsigned int i=0; i<attrCnt->nrOfValues; i++)
			{
				if(attrCnt->values[i].value.theString)
				{
					free(const_cast<char *>(attrCnt->values[i].value.theString));
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
				}
			}
		}
		free(attrCnt->values);
	}

	//	Check if the attrCnt memory needs to be freed
	if(bAll)
		free(attrCnt);

	LEAVE_OIPROXY();
}

/**
 * Fill in attribute value container with IMM attribute values. Attribute container will be type of <i>type</i>.
 * attrCnt parameter needs to be allocated before the function call.
 * Memory allocated by this function needs to be freed by freeAttributeContainerContent.
 *
 * @param	immAttr		input IMM attributes
 * @param	type		type of attribute container
 * @param	attrCnt		attribute container that will contain IMM attributes and their values in Com SPI format
 * @return	true, on any error returns false
 */
bool createAttributeContainerContent(SaImmAttrValuesT_2 *immAttr, MafOamSpiMoAttributeType_3T type, MafMoAttributeValueContainer_3T *attrCnt) {
	ENTER_OIPROXY();

	//	Check input parameters
	if(!immAttr || !attrCnt) {
		LEAVE_OIPROXY();
		return false;
	}

	//	Fill in type and attribute value array size
	attrCnt->type = type;
	attrCnt->nrOfValues = immAttr->attrValuesNumber;
	attrCnt->values = (MafMoAttributeValue_3T *)malloc(sizeof(MafMoAttributeValue_3T) * immAttr->attrValuesNumber);

	//	Save every IMM attribute to attribute container in Com SPI format
	SaNameT *t = NULL;
	for(unsigned int i=0; i<immAttr->attrValuesNumber; i++) {
		if(type == MafOamSpiMoAttributeType_3_STRING) {
			if(immAttr->attrValueType == SA_IMM_ATTR_SANAMET) {
				t = (SaNameT *)immAttr->attrValues[i];
				attrCnt->values[i].value.theString = (char *)malloc(saNameLen(t) + 1);
				strncpy(const_cast<char *>(attrCnt->values[i].value.theString), saNameGet(t), saNameLen(t));
				// In COM SPI Ver3. theString is declared as const, so can not be written to.
				// So, we have to cast it this way
				const char* p_str_end = &(attrCnt->values[i].value.theString[saNameLen(t)]);
				*(const_cast<char *>(p_str_end)) = 0;
			} else {
				attrCnt->values[i].value.theString = strdup(*(char **)(immAttr->attrValues[i]));
			}
		} else if (type == MafOamSpiMoAttributeType_3_REFERENCE) {
			t = (SaNameT *)immAttr->attrValues[i];
			// Convert the references from imm RDN to 3GPP DN representation
			std::string immDN(saNameGet(t), saNameLen(t));
			std::string comDN = theTranslator.ImmRdn2MOTop(immDN);
			attrCnt->values[i].value.moRef = (char *)malloc(comDN.length() + 1);
			strncpy(const_cast<char *>(attrCnt->values[i].value.moRef), (char *)comDN.c_str(), comDN.length());
			// In COM SPI Ver3. theString is declared as const, so can not be written to.
			// So, we have to cast it this way
			const char* p_str_end = &(attrCnt->values[i].value.moRef[comDN.length()]);
			*(const_cast<char *>(p_str_end)) = 0;
		} else {
			switch (type)
			{
				case MafOamSpiMoAttributeType_3_INT64:
					attrCnt->values[i].value.i64 	= *(int64_t*)(immAttr->attrValues[i]);
					break;
				case MafOamSpiMoAttributeType_3_INT32:
					attrCnt->values[i].value.i32 	= *(int32_t*)(immAttr->attrValues[i]);
					break;
				case MafOamSpiMoAttributeType_3_INT16:
					attrCnt->values[i].value.i16 	= *(int32_t*)(immAttr->attrValues[i]);
					break;
				case MafOamSpiMoAttributeType_3_INT8:
					attrCnt->values[i].value.i8 	= *(int32_t*)(immAttr->attrValues[i]);
					break;
				case MafOamSpiMoAttributeType_3_UINT64:
					attrCnt->values[i].value.u64 	= *(uint64_t*)(immAttr->attrValues[i]);
					break;
				case MafOamSpiMoAttributeType_3_UINT32:
					attrCnt->values[i].value.u32 	= *(uint32_t*)(immAttr->attrValues[i]);
					break;
				case MafOamSpiMoAttributeType_3_UINT16:
					attrCnt->values[i].value.u16 	= *(uint32_t*)(immAttr->attrValues[i]);
					break;
				case MafOamSpiMoAttributeType_3_UINT8:
					attrCnt->values[i].value.u8 	= *(uint32_t*)(immAttr->attrValues[i]);
					break;
				case MafOamSpiMoAttributeType_3_ENUM:
					if (immAttr->attrValueType == SA_IMM_ATTR_SAUINT32T) {
						attrCnt->values[i].value.theEnum = *(uint32_t*)(immAttr->attrValues[i]);
					}
					else {
						attrCnt->values[i].value.theEnum = *(int32_t*)(immAttr->attrValues[i]);
					}
					break;
				case MafOamSpiMoAttributeType_3_BOOL:
					if (immAttr->attrValueType == SA_IMM_ATTR_SAUINT32T) {
						attrCnt->values[i].value.theBool = *(uint32_t*)(immAttr->attrValues[i]);
					}
					else {
						attrCnt->values[i].value.theBool = *(int32_t*)(immAttr->attrValues[i]);
					}
					break;
				case MafOamSpiMoAttributeType_3_DECIMAL64:
					attrCnt->values[i].value.decimal64     = *(double*)(immAttr->attrValues[i]);
					break;
				default:
					// Invalid data type for this ... do nothing but set it to zero
					attrCnt->values[i].value.i32 	= 0;
					break;
			}
		}
	}

	LEAVE_OIPROXY();
	return true;
}

/**
 * Free IMM attribute memory allocated by copyMoAttrToImm function.
 * If freeRoot is set to true, allocated memory for attr attribute will be also freed.
 *
 * @param	attr		attribute that which allocated memory needs to be freed
 * @param	freeRoot	true is attr parameter need to be freed
 */
void freeImmAttr(SaImmAttrValuesT_2 *attr, bool freeRoot) {
	ENTER_OIPROXY();

	//	Check input parameter
	if(!attr) {
		LEAVE_OIPROXY();
		return;
	}

	//	Check if attribute name exists
	if(attr->attrName) {
		free(attr->attrName);
		attr->attrName = NULL;
	}

	//	Free allocated memory
	if(attr->attrValues) {
		for(SaUint32T i=0; i<attr->attrValuesNumber; i++) {
			if(attr->attrValues[i]) {
				if(attr->attrValueType == SA_IMM_ATTR_SASTRINGT) {
					if(*(char **)attr->attrValues[i])
						free(*(char **)attr->attrValues[i]);
				}

				free(attr->attrValues[i]);
			}
		}
		free(attr->attrValues);
		attr->attrValues = NULL;
	}

	//	Check if attr parameter memory needs to be freed
	if(freeRoot)
		free(attr);

	LEAVE_OIPROXY();
}

/**
 * Copy MO attribute container values to IMM attribute.
 * attr attribute needs to be allocated before the function is called.
 * Memory allocated by this function needs to be freed by freeImmAttr.
 *
 * @param	attrValue	MO attribute container
 * @param	attrDef		IMM attribute definition
 * @param	attr		IMM attribute that will contain MO attribute value(s)
 * @return	true, on error returns false
 */
bool copyMoAttrToImm(MafMoAttributeValueContainer_3T *attrValue, SaImmAttrDefinitionT_2 *attrDef, SaImmAttrValuesT_2 *attr) {
	ENTER_OIPROXY();

	//	Check input parameter
	if(!attrValue || !attrDef || !attr) {
		LEAVE_OIPROXY();
		return false;
	}

	//	Fill in attr basic data
	attr->attrValuesNumber = attrValue->nrOfValues;
	DEBUG_OIPROXY("copyMoAttrToImm(): attr->attrValuesNumber(%u)",attr->attrValuesNumber);
	attr->attrValueType = attrDef->attrValueType;
	DEBUG_OIPROXY("copyMoAttrToImm(): attr->attrValueType(%d)",attr->attrValueType);
	attr->attrName = strdup(attrDef->attrName);
	DEBUG_OIPROXY("copyMoAttrToImm(): attr->attrName(%s)",attr->attrName);
	attr->attrValues = (SaImmAttrValueT *)malloc(attrValue->nrOfValues * sizeof(SaImmAttrValueT));
	DEBUG_OIPROXY("copyMoAttrToImm(): before memset");
	memset(attr->attrValues, 0, attrValue->nrOfValues * sizeof(SaImmAttrValueT));
	DEBUG_OIPROXY("copyMoAttrToImm(): before memset");

	//	Copy attribute values
	if(attr->attrValueType == SA_IMM_ATTR_SASTRINGT) {
		if(attrValue->type != MafOamSpiMoAttributeType_3_STRING) {
			freeImmAttr(attr, false);
			LEAVE_OIPROXY();
			return false;
		}

		for(unsigned int i=0; i<attr->attrValuesNumber; i++) {
			attr->attrValues[i] = malloc(sizeof(char *));
			*(char **)attr->attrValues[i] = strdup(attrValue->values[i].value.theString);
		}
	} else if(attr->attrValueType == SA_IMM_ATTR_SANAMET) {
		if(attrValue->type != MafOamSpiMoAttributeType_3_STRING) {
			freeImmAttr(attr, false);
			LEAVE_OIPROXY();
			return false;
		}

		unsigned len = 0;
		SaNameT *saname = NULL;
		for(unsigned int i=0; i<attr->attrValuesNumber; i++) {
			len = strlen(attrValue->values[i].value.theString);
			if(len > saNameMaxLen()) {
				freeImmAttr(attr, false);
				LEAVE_OIPROXY();
				return false;
			}

			saname = makeSaNameT(attrValue->values[i].value.theString);
			attr->attrValues[i] = saname;
		}
	} else {
		for(unsigned int i=0; i<attrValue->nrOfValues; i++) {
			switch(attr->attrValueType) {
				case SA_IMM_ATTR_SAINT32T :
				case SA_IMM_ATTR_SAUINT32T :
					attr->attrValues[i] = (SaImmAttrValueT)malloc(sizeof(SaUint32T));

					switch(attrValue->type) {
						case MafOamSpiMoAttributeType_3_INT8 :
							*(SaInt32T *)(attr->attrValues[i]) = (SaInt32T)(attrValue->values[i].value.i8);
							break;
						case MafOamSpiMoAttributeType_3_UINT8 :
							*(SaUint32T *)(attr->attrValues[i]) = (SaUint32T)(attrValue->values[i].value.u8);
							break;
						case MafOamSpiMoAttributeType_3_INT16 :
							*(SaInt32T *)(attr->attrValues[i]) = (SaInt32T)attrValue->values[i].value.i16;
							break;
						case MafOamSpiMoAttributeType_3_UINT16 :
							*(SaUint32T *)(attr->attrValues[i]) = (SaUint32T)attrValue->values[i].value.u16;
							break;
						case MafOamSpiMoAttributeType_3_ENUM :
							*(SaInt32T *)(attr->attrValues[i]) = (SaInt32T)attrValue->values[i].value.theEnum;
							break;
						case MafOamSpiMoAttributeType_3_BOOL :
							*(SaUint32T *)(attr->attrValues[i]) = (SaUint32T)attrValue->values[i].value.theBool;
							break;
						case MafOamSpiMoAttributeType_3_DECIMAL64 :
							*(SaUint32T *)(attr->attrValues[i]) = (SaUint32T)attrValue->values[i].value.decimal64;
							break;
						default :
							*(SaUint32T *)(attr->attrValues[i]) = (SaUint32T)attrValue->values[i].value.u32;
					}
					break;
				case SA_IMM_ATTR_SAINT64T :
				case SA_IMM_ATTR_SAUINT64T :
					attr->attrValues[i] = (SaImmAttrValueT)malloc(sizeof(SaUint64T));

					switch(attrValue->type) {
						case MafOamSpiMoAttributeType_3_INT8 :
							*(SaInt64T *)(attr->attrValues[i]) = (SaInt64T)attrValue->values[i].value.i8;
							break;
						case MafOamSpiMoAttributeType_3_UINT8 :
							*(SaUint64T *)(attr->attrValues[i]) = (SaUint64T)attrValue->values[i].value.u8;
							break;
						case MafOamSpiMoAttributeType_3_INT16 :
							*(SaInt64T *)(attr->attrValues[i]) = (SaInt64T)attrValue->values[i].value.i16;
							break;
						case MafOamSpiMoAttributeType_3_UINT16 :
							*(SaUint64T *)(attr->attrValues[i]) = (SaUint64T)attrValue->values[i].value.u16;
							break;
						case MafOamSpiMoAttributeType_3_INT32 :
							*(SaInt64T *)(attr->attrValues[i]) = (SaInt64T)attrValue->values[i].value.i32;
							break;
						case MafOamSpiMoAttributeType_3_UINT32 :
							*(SaUint64T *)(attr->attrValues[i]) = (SaUint64T)attrValue->values[i].value.u32;
							break;
						case MafOamSpiMoAttributeType_3_ENUM :
							*(SaInt64T *)(attr->attrValues[i]) = (SaInt64T)attrValue->values[i].value.theEnum;
							break;
						case MafOamSpiMoAttributeType_3_BOOL :
							*(SaUint64T *)(attr->attrValues[i]) = (SaUint64T)attrValue->values[i].value.theBool;
							break;
						case MafOamSpiMoAttributeType_3_DECIMAL64 :
							*(SaUint64T *)(attr->attrValues[i]) = (SaUint64T)attrValue->values[i].value.decimal64;
							break;
						default :
							*(SaUint64T *)(attr->attrValues[i]) = (SaUint64T)attrValue->values[i].value.u64;
					}
					break;
				case SA_IMM_ATTR_SADOUBLET :
					attr->attrValues[i] = (SaImmAttrValueT)malloc(sizeof(SaDoubleT));

					switch(attrValue->type) {
						case MafOamSpiMoAttributeType_3_INT8 :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.i8;
							break;
						case MafOamSpiMoAttributeType_3_UINT8 :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.u8;
							break;
						case MafOamSpiMoAttributeType_3_INT16 :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.i16;
							break;
						case MafOamSpiMoAttributeType_3_UINT16 :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.u16;
							break;
						case MafOamSpiMoAttributeType_3_INT32 :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.i32;
							break;
						case MafOamSpiMoAttributeType_3_UINT32 :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.u32;
							break;
						case MafOamSpiMoAttributeType_3_BOOL :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.theBool;
							break;
						case MafOamSpiMoAttributeType_3_DECIMAL64 :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.decimal64;
							break;
						default :
							*(SaDoubleT *)(attr->attrValues[i]) = (SaDoubleT)attrValue->values[i].value.i64;
					}
					break;
				case SA_IMM_ATTR_SAFLOATT :
					attr->attrValues[i] = (SaImmAttrValueT)malloc(sizeof(SaFloatT));

					switch(attrValue->type) {
						case MafOamSpiMoAttributeType_3_INT8 :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.i8;
							break;
						case MafOamSpiMoAttributeType_3_UINT8 :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.u8;
							break;
						case MafOamSpiMoAttributeType_3_INT16 :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.i16;
							break;
						case MafOamSpiMoAttributeType_3_UINT16 :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.u16;
							break;
						case MafOamSpiMoAttributeType_3_INT32 :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.i32;
							break;
						case MafOamSpiMoAttributeType_3_UINT32 :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.u32;
							break;
						case MafOamSpiMoAttributeType_3_BOOL :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.theBool;
							break;
						case MafOamSpiMoAttributeType_3_DECIMAL64 :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.decimal64;
							break;
						default :
							*(SaFloatT *)(attr->attrValues[i]) = (SaFloatT)attrValue->values[i].value.i64;
					}
					break;
				default :
					freeImmAttr(attr, false);
					LEAVE_OIPROXY();
					return false;
			}
		}
	}

	LEAVE_OIPROXY();
	return true;
}

/**
 * Free member struct memory allocated by copyImmAttrListToMoAttrStruct
 *
 * @param	firstMemberStruct	first member of member struct list, which is NULL terminated
 */
void freeMoAttrStructList(MafMoAttributeValueStructMember_3T *firstMemberStruct) {
	ENTER_OIPROXY();
	//	Check input parameter
	if(!firstMemberStruct) {
		LEAVE_OIPROXY();
		return;
	}

	//	Go through the member struct linked list, and release allocated memory
	MafMoAttributeValueStructMember_3T *t = firstMemberStruct;
	while(firstMemberStruct) {
		t = firstMemberStruct;

		if(t->memberName)
			free(t->memberName);

		//	Free attribute container memory
		freeAttributeContainerContent(t->memberValue, true);
		firstMemberStruct = t->next;
		free(t);
	}
	LEAVE_OIPROXY();
}

/**
 * Copy IMM attributes to struct member attribute list.
 * Allocated memory by this function needs to be freed by freeMoAttrStructList.
 *
 * @param	moAttr			list of all complex type attributes
 * @param	immAttr			list of IMM attributes (list of complex type simple attributes)
 * @param	memberStruct	member struct which will contain all IMM attribute values from immAttr list
 * @return	true, on any error returns false
 */
bool copyImmAttrListToMoAttrStruct(MafOamSpiStructMemberT *moAttr, std::list<SaImmAttrValuesT_2 *> &immAttr, MafMoAttributeValueStructMember_3T **memberStruct) {
	ENTER_OIPROXY();

	//	Check input parameters
	if(!memberStruct || immAttr.size() == 0) {
		LEAVE_OIPROXY();
		return false;
	}

	*memberStruct = NULL;

	//	Go through all IMM attributes, and if attributes exist in moAtrr member list, copy their values to memberStruct
	MafOamSpiStructMemberT *ma = NULL;
	MafMoAttributeValueStructMember_3T *t = NULL;
	MafMoAttributeValueStructMember_3T *last = NULL;
	MafOamSpiMoAttributeType_3T basetype;
	for(std::list<SaImmAttrValuesT_2 *>::iterator it=immAttr.begin(); it != immAttr.end(); it++) {
		//	Check if IMM attribute exists in moAttr member list
		ma = moAttr;
		while(ma) {
			if(!strcmp(ma->generalProperties.name, (*it)->attrName))
				break;
			ma = ma->next;
		}

		//	If the attribute cannot be found in moAttr member list, continue with next attribute
		if(!ma)
			continue;

		//	Create member struct linked list
		t = (MafMoAttributeValueStructMember_3T *)malloc(sizeof(MafMoAttributeValueStructMember_3T));
		if(!(*memberStruct)) {
			last = *memberStruct = t;
		} else {
			last->next = t;
			last = t;
		}
		//	Copy IMM attribute to memberStruct
		t->memberName = strdup((*it)->attrName);
		t->next = NULL;
		t->memberValue = (MafMoAttributeValueContainer_3T *)malloc(sizeof(MafMoAttributeValueContainer_3T));
		basetype = CorrectBaseType(ma->memberType);
		createAttributeContainerContent(*it, basetype, t->memberValue);
	}

	LEAVE_OIPROXY();
	return true;
}

/**
 * Free attribute value container allocated memory by getModifiedStructAttributes
 *
 * @param	structAttr	attribute value container
 */
void releaseModifiedStructAttributes(MafMoAttributeValueContainer_3T *structAttr) {
	ENTER_OIPROXY();

	//	check input parameters
	if(!structAttr) {
		LEAVE_OIPROXY();
		return;
	}

	//	Check if the attribute container is type of STRUCT
	if(structAttr->type != MafOamSpiMoAttributeType_3_STRUCT) {
		LEAVE_OIPROXY();
		return;
	}

	//	Free allocated memory
	if(structAttr->values)
	{
		MafMoAttributeValueStructMember_3T *member;
		MafMoAttributeValueContainer_3T *value;

		for(unsigned int i=0; i<structAttr->nrOfValues; i++)
		{
			member = structAttr->values[i].value.structMember;
			while (member != NULL)
			{

				value = member->memberValue;
				if(value != NULL)
				{
					if (value->nrOfValues > 0)
					{
						if(value->values != NULL)
						{
							if(value->type == MafOamSpiMoAttributeType_3_STRING)
							{
								for(unsigned int k=0; k<value->nrOfValues; k++)
								{
									if(value->values[k].value.theString)
									{
										free(const_cast<char*>(value->values[k].value.theString));
									}
								}
							}
							else if(value->type == MafOamSpiMoAttributeType_3_REFERENCE)
							{
								for(unsigned int k=0; k<value->nrOfValues; k++)
								{
									if(value->values[k].value.moRef)
									{
										free(const_cast<char*>(value->values[k].value.moRef));
									}
								}
							}
							free(value->values);
							value->values = NULL;
						}
					}
					free(value);
					value = NULL;
				}
				if(member->memberName)
				{
					free(member->memberName);
					member->memberName = NULL;
				}

				MafMoAttributeValueStructMember_3T *tmp_member = member->next;
				free(member);
				member = tmp_member;
			}
		}
		free(structAttr->values);
		structAttr->values = NULL;
	}
	free(structAttr);
	structAttr = NULL;
	LEAVE_OIPROXY();
	return;
}

/**
 * Create complex type attribute value container from modified IMM attributes.
 * Allocated memory needs to be freed by releaseModifiedStructAttributes.
 *
 * @param	attrMods	NULL terminated list of modified attributes
 * @param	moAttr		SPI MO attribute of complex type attribute
 * @return	New created complex type attribute value container. On error NULL is returned.
 */
MafMoAttributeValueContainer_3T *getModifiedStructAttributes(const SaImmAttrModificationT_2 **attrMods, MafOamSpiMoAttributeT *moAttr) {
	ENTER_OIPROXY();
	//	Check input parameters
	if(!attrMods || !moAttr) {
		LEAVE_OIPROXY();
		return NULL;
	}

	//	Check if the attribute container is type of STRUCT
	if(moAttr->type != (MafOamSpiMoAttributeTypeT) MafOamSpiMoAttributeType_3_STRUCT) {
		LEAVE_OIPROXY();
		return NULL;
	}

	//	Take struct data
	MafOamSpiStructT *structData = moAttr->structDatatype;
	if(!structData) {
		LEAVE_OIPROXY();
		return NULL;
	}

	//	First complex type member element
	MafOamSpiStructMemberT *members = structData->members;

	//	Fill in basic data
	MafMoAttributeValueContainer_3T *cnt = (MafMoAttributeValueContainer_3T *)malloc(sizeof(MafMoAttributeValueContainer_3T));
	cnt->type = MafOamSpiMoAttributeType_3_STRUCT;
	cnt->nrOfValues = 1;
	cnt->values = (MafMoAttributeValue_3T *)malloc(sizeof(MafMoAttributeValue_3T));
	cnt->values[0].value.structMember = NULL;

	//	Copy input IMM attributes
	bool ok = true;
	int i;
	MafMoAttributeValueStructMember_3T *firstMember = NULL;
	MafMoAttributeValueStructMember_3T *lastAttr = NULL;
	MafMoAttributeValueStructMember_3T *t;
	members = structData->members;
	//	For each attribute member
	while(members) {
		//	fill in the attribute data
		t = (MafMoAttributeValueStructMember_3T *)malloc(sizeof(MafMoAttributeValueStructMember_3T));
		t->memberName = strdup(members->generalProperties.name);
		t->memberValue = (MafMoAttributeValueContainer_3T *)malloc(sizeof(MafMoAttributeValueContainer_3T));
		t->memberValue->type = CorrectBaseType(members->memberType);
		t->memberValue->nrOfValues = 0;
		t->next = NULL;

		//	Handle the chain of attributes
		if(!firstMember) {
			lastAttr = firstMember = t;
		} else {
			lastAttr->next = t;
			lastAttr = t;
		}

		//	Find the attribute in IMM attributes
		i = 0;
		while(attrMods[i]) {
			if(attrMods[i]->modType == SA_IMM_ATTR_VALUES_ADD || attrMods[i]->modType == SA_IMM_ATTR_VALUES_REPLACE)
				if(!strcmp(members->generalProperties.name, (char *)attrMods[i]->modAttr.attrName)) {
					break;
				}
			i++;
		}

		//	If the attribute is changed, create its attribute container content
		if(attrMods[i])
			if(!createAttributeContainerContent((SaImmAttrValuesT_2 *)&attrMods[i]->modAttr, t->memberValue->type, t->memberValue)) {
				ok = false;
				break;
			}

		//	Go to the next member
		members = members->next;
	}

	cnt->values[0].value.structMember = firstMember;

	if(!ok) {
		releaseModifiedStructAttributes(cnt);
		LEAVE_OIPROXY();
		return NULL;
	}
	LEAVE_OIPROXY();
	return cnt;
}


typedef struct MafMoAttributeValueResult_3_AutoRelease : MafMoAttributeValueResult_3T {
	MafMoAttributeValueResult_3_AutoRelease()
	{
		release = NULL;
		container = NULL;
	}
	~MafMoAttributeValueResult_3_AutoRelease()
	{
		if(release != NULL && container!= NULL)
		{
			release(container);
		}
	}
} MafMoAttributeValueResult_3T_AutoRelease;

/**
 * This function implements action in the COM Object Implementer.
 * It receives a operationId as input that is mapped to an action name by
 * searching the MOM model for an matching action name.
 *
 * The parameter invocation is used to identify the admin session when returning
 * the result of the action by use of the IMM method
 * saImmOiAdminOperationResult().
 *
 * The parameter params provides the values of the parameters provided with
 * the action call to COM OI.
 *
 * The parameter objectName is the owning object for the action called.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	invocation		Used to match this invocation
 * @param	objectName		Pointer to the object name
 * @param	operationId		Identifier of the administrative operation
 * @param	params			Pointer to a NULL-terminated array of pointers to parameter descriptors
 */
void OIProxy_handleAdminOperation(SaImmOiHandleT immOiHandle, SaInvocationT invocation, const SaNameT *objectName,
				SaImmAdminOperationIdT operationId, const SaImmAdminOperationParamsT_2 **params) {

	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleAdminOperation() entered");
	//mapped to MO::action()
	DEBUG_OIPROXY("OIProxy_handleAdminOperation: objectName = %s ", saNameGet(objectName));
	DEBUG_OIPROXY("OIProxy_handleAdminOperation: operationId = %llu ", operationId);

	// Implementation of this:

	std::string immDn(saNameGet(objectName));
	std::string moDn;
	std::string ctAttr;
	int index;
	if(!parseImmDN(immDn.c_str(), moDn, ctAttr, &index)) {
		ERR_OIPROXY("OIProxy_handleAdminOperation: Error in parsing IMM DN");
		// Return failing result to caller of action
		LEAVE_OIPROXY();
		return;
	}

	DEBUG_OIPROXY("OIProxy_handleAdminOperation: after calling parseImmDN()");

	std::string mocPath;
	createMocPath(moDn.c_str(), mocPath);

	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	RegisteredCommonStructT *spiIf;

	SaImmAdminOperationParamsT_2** returnParams = new SaImmAdminOperationParamsT_2*[ImmAdmOpNoReturnParams+1];

	#define ERR_STR_BUF_SIZE 255 // max size for the error strins we create here
	char errStr[] = "errorString";
	char *errorText = new char[ERR_STR_BUF_SIZE + 1];
	returnParams[0] = new SaImmAdminOperationParamsT_2;
	returnParams[0]->paramName = errStr;
	returnParams[0]->paramType = SA_IMM_ATTR_SASTRINGT;
	returnParams[0]->paramBuffer = &errorText;
	returnParams[1] = NULL; // prepare for a single error string for most of the cases below

	if(index == -1) {	//	Not complex type
		if(!existRegisteredDN(moDn.c_str())) {
			if(!existRegisteredClass((*mocPathList.rbegin()).c_str())) {
				ERR_OIPROXY("OIProxy_handleAdminOperation() : %s is not registered", immDn.c_str());
				snprintf(errorText, ERR_STR_BUF_SIZE,
						 "@COMNBI@OIProxy_handleAdminOperation() : %s is not registered", immDn.c_str());
				autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
														 (const SaImmAdminOperationParamsT_2**) returnParams);
				DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
					  returnParams[0]->paramName, returnParams[0]->paramType, errorText);
				delete returnParams[0];
				delete [] returnParams;
				delete [] errorText;
				LEAVE_OIPROXY();
				return;
			} else
				spiIf = (RegisteredCommonStructT *)getRegisteredClass((*mocPathList.rbegin()).c_str());
		} else
			spiIf = (RegisteredCommonStructT *)getRegisteredDN(moDn.c_str());

	} else {
		//	Complex type, this is a faulty case so we make a note in the error log and returns.
		ERR_OIPROXY("OIProxy_handleAdminOperation: %s is registered as struct attribute", immDn.c_str());
		snprintf(errorText, ERR_STR_BUF_SIZE,
				 "@COMNBI@OIProxy_handleAdminOperation: %s is registered as struct attribute", immDn.c_str());
		autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
												 (const SaImmAdminOperationParamsT_2**) returnParams);
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
			  returnParams[0]->paramName, returnParams[0]->paramType, errorText);
		delete returnParams[0];
		delete [] returnParams;
		delete [] errorText;
		LEAVE_OIPROXY();
		return;
	}

	RegisteredMoInterfaceT moIfVersion;
	if (!getMoIfVersion(spiIf, moIfVersion))
	{
		ERR_OIPROXY("OIProxy_handleAdminOperation()::getMoIfVersion() Unable to get Registered interface version.");
		return;
	}

	// 2. Translate params from IMM format to MO format, reuse the setMoAttribute( .. ) translation( ..)
	// Debug printout what we receive in this action
	// Count the number of parameters that we need to translate.
	int	numberOfParameters=0;
	for(unsigned int i=0;params!=NULL && params[i]!=NULL;i++) {
		numberOfParameters++;
		if (params[i]->paramType==SA_IMM_ATTR_SASTRINGT) {
			DEBUG_OIPROXY("OIProxy_handleAdminOperation: adminOperationParams %i, paramName %s, paramType %d, paramBuffer %s",
					i, params[i]->paramName, params[i]->paramType, *(char**)params[i]->paramBuffer);
		}
	}

	// Translate the OperationId to a action name.
	// Basically iterate over the structure with available actions in the Mom using GetComAction( .. ) in the Translator.
	// Create a map with keyValue=parameterName and the order as the second data.
	OamSACache::DNList theSplitDN;
	// Use the global one here because in OIProxy we do not use the COM-SA cache.
//	GlobalSplitDN(the3gppDN,theSplitDN);
	GlobalSplitDN(moDn.c_str(),theSplitDN);
#ifdef UNIT_TEST
	extern MafOamSpiMoActionT* GetComActionList(OamSACache::DNList& mo_name);
	// MafOamSpiMoActionT *moAction = GetComActionList(theSplitDN);
	//TODO: FIXME SO We can execute the action in unit test.
	MafOamSpiMoActionT *moAction = NULL;
#endif
#ifndef UNIT_TEST
	MafOamSpiMoActionT *moAction = theTranslator.GetComActionList(theSplitDN);
#endif
	// Identify the operationId
	bool admOpFound=false;
	// The actions name is stored in this pointer
	const char * name;
	// The map for the order of the parameter list.
	std::map<std::string,int> parameterOrder;
	typedef std::pair<std::string,int> mapNameOrder;
	// Pointer to the parameter list for the found action
	MafOamSpiParameterT *pParameterList=NULL;
	SaAisErrorT returnResult;

	if (moAction != NULL) {
		for (MafOamSpiMoActionT* ap = moAction; ap != NULL; ap = ap->next) {
			MafOamSpiDomainExtensionT* dp = ap->domainExtension;
			if (dp != 0 && dp->domain != 0 && !strcmp(dp->domain, DOMAIN_NAME_COREMW)) {
				// Check for a match of the operationId that we have
				// To identify the action name
				for (MafOamSpiExtensionT* ep = dp->extensions; ep != 0; ep=ep->next) {
					if (ep->name != 0 && !strcmp(ep->name, DOMAIN_EXT_NAME_ADM_OP_ID)) {
						if (ep->value != 0) {
							SaImmAdminOperationIdT tempOperationId = strtoull (ep->value,0,10);
							if (errno != ERANGE || errno == 0) {
								// OK Check if this is our OperationId
								if (operationId == tempOperationId) {
									// Match we have found the action!
									name = ap->generalProperties.name;
									DEBUG_OIPROXY("OIProxy_handleAdminOperation: Found action name %s",name);
									// Store the parameterlist for the found action
									pParameterList = ap->parameters;
									//SDP872 - set moAction to the found action for use later on.
									moAction = theTranslator.GetComAction(theSplitDN, name);
									admOpFound = true;
									break;
								}
							} else {
								ERR_OIPROXY("OIProxy_handleAdminOperation() : Erroneous operationId received");
								snprintf(errorText, ERR_STR_BUF_SIZE,
										 "@COMNBI@OIProxy_handleAdminOperation() : Erroneous operationId received");
								returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
																						(const SaImmAdminOperationParamsT_2**) returnParams);
								DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
									  returnParams[0]->paramName, returnParams[0]->paramType, errorText);
								delete returnParams[0];
								delete [] returnParams;
								delete [] errorText;
								LEAVE_OIPROXY();
								return;
							}
						}
					} // if admOpId
				} // for
			}
			if (admOpFound) {
				// OK, we have found the action we search for.
				// populate the parameterOrder map
				int index = 0;
				for(MafOamSpiParameterT *parameters = ap->parameters; parameters != NULL; parameters = parameters->next) {
					parameterOrder.insert(mapNameOrder(std::string(parameters->generalProperties.name),index));
					DEBUG_OIPROXY("OIProxy_handleAdminOperation: Insert parameter %s Index=%i",
							parameters->generalProperties.name, index);
					index++;
				}
				// We have found what we looks for, now break out.
				break;
			}
		} // for
	} else {
		// Fatal error, return a failure
		ERR_OIPROXY("OIProxy_handleAdminOperation() : Fatal error could not find corresponding action");
		snprintf(errorText, ERR_STR_BUF_SIZE,
				 "@COMNBI@OIProxy_handleAdminOperation() : Fatal error could not find corresponding action");
		returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
																(const SaImmAdminOperationParamsT_2**) returnParams);
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
			  returnParams[0]->paramName, returnParams[0]->paramType, errorText);
		delete returnParams[0];
		delete [] returnParams;
		delete [] errorText;
		LEAVE_OIPROXY();
		return;
	}
	// Final check that everything works as expected.
	if (!admOpFound) {
		ERR_OIPROXY("OIProxy_handleAdminOperation() : Fatal error could not map corresponding action");
		snprintf(errorText, ERR_STR_BUF_SIZE,
				 "@COMNBI@OIProxy_handleAdminOperation() : Fatal error could not map corresponding action");
		returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
																(const SaImmAdminOperationParamsT_2**) returnParams);
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
			  returnParams[0]->paramName, returnParams[0]->paramType, errorText);
		delete returnParams[0];
		delete [] returnParams;
		delete [] errorText;
		LEAVE_OIPROXY();
		return;
	}

	// Allocate memory for the parameters. Use stack here to optimize
	MafMoNamedAttributeValueContainer_3T* myParameters[numberOfParameters + 1];
	MafMoNamedAttributeValueContainer_3T parameterMemory[numberOfParameters];

	// To provide backward compatibility support to LM, do same for v2 datatypes.
	// FIXME: Find a way to optimise both myParameters and myParameters_2 to save memory.
	MafMoNamedAttributeValueContainerT* myParameters_2[numberOfParameters + 1];
	MafMoNamedAttributeValueContainerT parameterMemory_2[numberOfParameters];

	for(int index = 0; index < numberOfParameters; index++) {
		myParameters[index]= &parameterMemory[index];
		myParameters_2[index]= &parameterMemory_2[index];
	}

	myParameters[numberOfParameters] = NULL; // array terminator
	myParameters_2[numberOfParameters] = NULL; // array terminator

	// OK now we have a order of the parameters defined.
	// OK now we have a action name.
	SaAisErrorT returnValue = SA_AIS_ERR_NOT_SUPPORTED;
	// Translate to Mom/3gpp format.
	if (moIfVersion == MafOamSpiManagedObject3)
	{
		returnValue = OIProxy_translateImm2Mo(params, parameterMemory, parameterOrder, pParameterList);
	}
	else if (moIfVersion == MafOamSpiManagedObject2)
	{
		returnValue = OIProxy_translateImm2Mo(params, parameterMemory_2, parameterOrder, pParameterList);
	}

	if(returnValue != SA_AIS_OK) {
		ERR_OIPROXY("OIProxy_handleAdminOperation() : Fatal error could not translate to Mo format");
		snprintf(errorText, ERR_STR_BUF_SIZE,
				 "@COMNBI@OIProxy_handleAdminOperation() : Fatal error could not translate to Mo format");
		returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
																(const SaImmAdminOperationParamsT_2**) returnParams);
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
			  returnParams[0]->paramName, returnParams[0]->paramType, errorText);
		delete returnParams[0];
		delete [] returnParams;
		delete [] errorText;
		LEAVE_OIPROXY();
		return;
	}
	// 3. An action is always implemented as an own transaction that is finished within the scope of this
	//    function, so this must be implemented separately.
	MafOamSpiTransactionHandleT txHandle;
	MafOamSpiManagedObject_3T *moIf;

	// The number 47 below is a magic number that must exist, but the value is not interesting it can be anything as long
	// it is not used by any other transaction running.
	SaImmOiCcbIdT ccbId = 47;
	// Check that we do not have any exciting transaction already running using this ccbId
	while(getCcbTransaction(ccbId, &txHandle)) {
		ccbId++;
	}
	// OK, now we have a unique ccbId
	// Get the interface for the Object implementor
	bool oiInterfaceFound = getMoIf(spiIf, &moIf);
	if(!oiInterfaceFound) {
		ERR_OIPROXY("OIProxy_handleAdminOperation() : Fatal error could not get MO I/F for the OI");
		snprintf(errorText, ERR_STR_BUF_SIZE,
				 "@COMNBI@OIProxy_handleAdminOperation() : Fatal error could not get MO I/F for the OI");
		returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
																(const SaImmAdminOperationParamsT_2**) returnParams);
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
			  returnParams[0]->paramName, returnParams[0]->paramType, errorText);
		delete returnParams[0];
		delete [] returnParams;
		delete [] errorText;
		LEAVE_OIPROXY();
		return;
	}

	if(!openTransaction(ccbId, spiIf, &txHandle)) {
		ERR_OIPROXY("OIProxy_handleAdminOperation: Fatal error could not start transaction");
		snprintf(errorText, ERR_STR_BUF_SIZE,
				 "@COMNBI@OIProxy_handleAdminOperation: Fatal error could not start transaction");
		returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
																(const SaImmAdminOperationParamsT_2**) returnParams);
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
			  returnParams[0]->paramName, returnParams[0]->paramType, errorText);
		delete returnParams[0];
		delete [] returnParams;
		delete [] errorText;
		LEAVE_OIPROXY();
		return;
	}
	// Remove the ccBId from the map since it is not needed any more.
	removeCcbTransaction(ccbId);

	MafMoAttributeValueResult_3T_AutoRelease result;
	MafMoAttributeValueContainer_3T **comParamList;
	MafReturnT maf_rc = MafFailure;
#ifdef UNIT_TEST
//	extern
//	MafReturnT action(MafOamSpiTransactionHandleT txHandle,
//		                 const char * dn,
//		                 const char * name,
//		                 MafMoAttributeValueContainer_3T **parameters,
//		                 MafMoAttributeValueContainer_3T **result);
//	maf_rc=action(txHandle, the3gppDN, name,
//			(MafMoNamedAttributeValueContainer_3T **)(&myParameters),&result);
#endif
#ifndef UNIT_TEST
	if (moIfVersion == MafOamSpiManagedObject3)
	{
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: calling action() with myParameters = %p", myParameters);
		maf_rc = moIf->action(txHandle,
				moDn.c_str(),
				name,
				(MafMoNamedAttributeValueContainer_3T**)(&myParameters),
				&result);
	}
	else if (moIfVersion == MafOamSpiManagedObject2)
	{
		MafOamSpiManagedObject_2T *moIf_2 = reinterpret_cast<MafOamSpiManagedObject_2T*>(moIf);
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: calling action() with myParameters_2 = %p", myParameters);
		maf_rc = moIf_2->action(txHandle,
				moDn.c_str(),
				name,
				(MafMoNamedAttributeValueContainerT**)(&myParameters_2),
				(MafMoAttributeValueContainer_2T**)(&result.container));
	}

#endif
	if (MafOk == maf_rc) {
		// Execute explicit prepare.
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: calling txMaster->explicitPrepare()");
		MafReturnT com_rc_expP = txMaster->explicitPrepare(txHandle);
		// 4. The result from the action calls is the return value from the action call.
		if (com_rc_expP == MafOk) {
			// OK action accepted, return success and commit/finish the transaction
			// commit and finish
			com_rc_expP = txMaster->commit(txHandle);

			// VOID type is not supported by MO SPI v3.
			// If the modeled action does not expect returned parameters we can not handle it in OIProxy.
			if((moAction->returnType.type == MafOamSpiDatatype_VOID) && (moIfVersion == MafOamSpiManagedObject3))
			{
				ERR_OIPROXY("OIProxy_handleAdminOperation() : VOID return type not currently supported by MO SPI v3");
				snprintf(errorText, ERR_STR_BUF_SIZE,
						 "@COMNBI@OIProxy_handleAdminOperation() : VOID return type not currently supported by MO SPI v3");
				returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
																		(const SaImmAdminOperationParamsT_2**) returnParams);
				DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
				returnParams[0]->paramName, returnParams[0]->paramType, errorText);
				delete returnParams[0];
				delete [] returnParams;
				delete [] errorText;
				LEAVE_OIPROXY();
				return;
			}

			if (com_rc_expP == MafOk) {
				DEBUG_OIPROXY("OIProxy_handleAdminOperation: action executed successfully");
				SaImmAdminOperationParamsT_2 **	opPP = NULL;
				bool returnOnMemFree = false;
				if (NULL != result.container) {
					comParamList = new MafMoAttributeValueContainer_3T *[2];
					comParamList[0] = (MafMoAttributeValueContainer_3*)result.container;
					comParamList[1] = NULL;

					// Convert parameters sent to us into a format underestood by the object implementer
					// before passing them on
					maf_rc = theTranslator.ConvertActionReturnToImmParameters(moAction, comParamList, &opPP, (moIfVersion == MafOamSpiManagedObject2));
					delete []comParamList;



































					if (MafOk == maf_rc)
					{
						returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_OK, (const SaImmAdminOperationParamsT_2**) opPP);
						DEBUG_OIPROXY("OIProxy_handleAdminOperation() from autoRetry_saImmOiAdminOperationResult_o2 returnResult: %d", returnResult);
					}
					else
					{
						DEBUG_OIPROXY("OIProxy_handleAdminOperation() : Failed to Convert Admin Op Parameters, rc = %d", maf_rc);
						// Map the COM result to IMM error codes.
						//TODO: check if this is actually needed. if not, remove mapComCodeToImmCode.
						returnResult = mapComCodeToImmCode(maf_rc);
						snprintf(errorText, ERR_STR_BUF_SIZE,
								"@COMNBI@OIProxy_handleAdminOperation() : Failed to convert Admin Op Parameters to IMM, rc=%d", maf_rc);
						returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, returnResult,
								(const SaImmAdminOperationParamsT_2**) returnParams);
						if (SA_AIS_OK != returnResult)
						{
							ERR_OIPROXY("OIProxy_handleAdminOperation: no returned params, saImmOiAdminOperationResult_o2: failed with %d", returnResult);
/*							delete returnParams[0];
							delete [] returnParams;
							delete [] errorText;
							LEAVE_OIPROXY();*/
							returnOnMemFree = true;//return;
						}
						else
						{
							DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
									returnParams[0]->paramName, returnParams[0]->paramType, errorText);
						}
						//result.release(result.container);
					}
					if(opPP[0] != NULL)
					{
						DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2: result OK, returnParams: name: %s, type: %d",
								opPP[0]->paramName, opPP[0]->paramType);

						// opPP is not deleted anywhere else, needs to be delete'd here to avoid memory leak.
						int i;
						for (i = 0; opPP != NULL && opPP[i] != NULL;i++)
						{
							delete [] opPP[i]->paramName;
							switch (opPP[i]->paramType)
							{
							case SA_IMM_ATTR_SAINT32T:
								delete (SaInt32T*) opPP[i]->paramBuffer;
								break;
							case SA_IMM_ATTR_SAINT64T:
								delete (SaInt64T*) opPP[i]->paramBuffer;
								break;
							case SA_IMM_ATTR_SAUINT32T:
								delete (SaUint32T*) opPP[i]->paramBuffer;
								break;
							case SA_IMM_ATTR_SAUINT64T:
								delete (SaUint64T*) opPP[i]->paramBuffer;
								break;
							case SA_IMM_ATTR_SASTRINGT:
							{
								char* cptr = *(char**)(opPP[i]->paramBuffer);
								delete [] cptr;
								delete (char*)opPP[i]->paramBuffer;
							}
							break;
							case SA_IMM_ATTR_SAFLOATT:
								delete (SaFloatT*) opPP[i]->paramBuffer;
								break;
							case SA_IMM_ATTR_SADOUBLET:
								delete (SaDoubleT*) opPP[i]->paramBuffer;
								break;
							case SA_IMM_ATTR_SANAMET:
								saNameDelete((SaNameT*) opPP[i]->paramBuffer, true);
								break;
							default:
								// Should never end up here.
								ERR_OIPROXY("OIProxy_handleAdminOperation(): ERROR: Unknown SA_IMM_ATTR_ paramType: %d", opPP[i]->paramType);
								break;
							}
							if (opPP[i]) delete opPP[i], opPP[i]=0;
						}
						if (opPP) delete [] opPP, opPP=0;

						if (returnOnMemFree)
						{
							if (returnParams[0])  delete returnParams[0];
							if (returnParams)     delete [] returnParams;
							if (errorText)        delete [] errorText;
							LEAVE_OIPROXY();
							return;
						}
					}
					else if (SA_AIS_OK != returnResult)
					{
						ERR_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2: failed with %d", returnResult);
						if (returnParams[0])  delete returnParams[0];
						if (returnParams)     delete [] returnParams;
						if (errorText)        delete [] errorText;
						if (opPP)             delete [] opPP, opPP = 0;

						LEAVE_OIPROXY();
						return;
					}
					else {
						if (opPP) delete [] opPP, opPP = 0;
					}
				}
				else // for backward compatibility when the MAF OI does not return a result, but the model expects a result
				{
					returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_OK, (const SaImmAdminOperationParamsT_2**) opPP);
					if (SA_AIS_OK != returnResult)
					{
						ERR_OIPROXY("OIProxy_handleAdminOperation: no returned params, saImmOiAdminOperationResult_o2: failed with %d", returnResult);
						if (returnParams[0])  delete returnParams[0];
						if (returnParams)     delete [] returnParams;
						if (errorText)        delete [] errorText;
						LEAVE_OIPROXY();
						return;
					}
					else
					{
						DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2: result OK, no params returned.");
					}
				}
			} else {
				ERR_OIPROXY("OIProxy_handleAdminOperation() : Failed to commit after action() returned MafOk");
				// No need to map the COM result to IMM error codes. Here com_rc_expP is OK!
				snprintf(errorText, ERR_STR_BUF_SIZE,
						 "@COMNBI@OIProxy_handleAdminOperation() : Failed to commit after action() returned MafOk");
				returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, SA_AIS_ERR_FAILED_OPERATION,
																		(const SaImmAdminOperationParamsT_2**) returnParams);
				if (SA_AIS_OK != returnResult)
				{
					ERR_OIPROXY("OIProxy_handleAdminOperation: no returned params, saImmOiAdminOperationResult_o2: failed with %d", returnResult);
					if (returnParams[0])  delete returnParams[0];
					if (returnParams)     delete [] returnParams;
					if (errorText)        delete [] errorText;
					LEAVE_OIPROXY();
					return;
				}
				else
				{
					DEBUG_OIPROXY("OIProxy_handleAdminOperation: saImmOiAdminOperationResult_o2 returnParams: name: %s, type: %d, buffer: %s",
						returnParams[0]->paramName, returnParams[0]->paramType, errorText);
					// abort is called by the TX Master in COM
				}
			}
		} else {
			// explicitPrepare not accepted, return failure and abort/finish the transaction
			ERR_OIPROXY("OIProxy_handleAdminOperation: explicitPrepare failed");
			// Map the COM result to IMM error codes.
			returnResult = mapComCodeToImmCode((MafReturnT) com_rc_expP);
			// get error strings from the Thread Context
			const char** errorStrings;
			ThreadContextHandle_2T msgHandle;
			if(MafOk != GetErrMsgsFromThContext(&errorStrings, &msgHandle))
			{
				ERR_OIPROXY("OIProxy_handleAdminOperation() : GetErrMsgsFromThContext() returned error");
				if (returnParams[0])  delete returnParams[0];
				if (returnParams)     delete [] returnParams;
				if (errorText)        delete [] errorText;
				LEAVE_OIPROXY();
				return;
			}
			PrepareErrMsgsForAdmOpResult((char**) errorStrings, returnParams);
			returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, returnResult,
																	(const SaImmAdminOperationParamsT_2**) returnParams);
			com_rc_expP = txMaster->abort(txHandle);
			threadContext->clearMessages(ThreadContextMsgLog_2);
			threadContext->clearMessages(ThreadContextMsgNbi_2);
			threadContext->releaseHandle(msgHandle);
		}
	} else {
		// action Failed
		DEBUG_OIPROXY("OIProxy_handleAdminOperation: action failed");
		// Map the COM result to IMM error codes.
		returnResult = mapComCodeToImmCode(maf_rc);
		// get error strings from the Thread Context
		const char** errorStrings = NULL;
		ThreadContextHandle_2T msgHandle;
		if(MafOk != GetErrMsgsFromThContext(&errorStrings, &msgHandle))
		{
			ERR_OIPROXY("OIProxy_handleAdminOperation() : GetErrMsgsFromThContext() returned error");
			if (returnParams[0])  delete returnParams[0];
			if (returnParams)     delete [] returnParams;
			if (errorText)        delete [] errorText;
			LEAVE_OIPROXY();
			return;
		}
		PrepareErrMsgsForAdmOpResult((char**) errorStrings, returnParams);

		returnResult = autoRetry_saImmOiAdminOperationResult_o2(immOiHandle, invocation, returnResult, (const SaImmAdminOperationParamsT_2**) returnParams);
		MafReturnT com_rc = txMaster->abort(txHandle);
		if (MafOk != com_rc)
		{
			ERR_OIPROXY("OIProxy_handleAdminOperation() : Transaction abort() failed.");
		}
		threadContext->clearMessages(ThreadContextMsgLog_2);
		threadContext->clearMessages(ThreadContextMsgNbi_2);
		threadContext->releaseHandle(msgHandle);
	}
	// OK, we are done, only to return back to caller
	for (int i = 0; returnParams[i] != NULL; i++ ) {
		delete returnParams[i];
		returnParams[i] = NULL;
	}
	delete [] returnParams;
	delete [] errorText;
	LEAVE_OIPROXY();
	return;
}


/**
 * Something failed, and this function is called to abort Com transaction and to clean
 * all used data for a certain ccbId.
 * This function is used by the main implementer.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	ccbId			Ccb identifier
 */
void OIProxy_handleCcbAbort(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleCcbAbort() entered");
	//	Find the transaction handle
	MafOamSpiTransactionHandleT txHandle;
	if(!getCcbTransaction(ccbId, &txHandle)) {
		ERR_OIPROXY("OIProxy_handleCcbAbort(): COM OI Failed to get transaction handle");
		LEAVE_OIPROXY();
		return;
	}

	// Remove ccbId data from the cache
	OIProxy_cache_ccbAbort(ccbId);
	// Remove the ccBId from the map since it is not needed any more.
	removeCcbTransaction(ccbId);

	//	Abort the transaction using the transaction master
	MafReturnT ret = txMaster->abort(txHandle);
	if(ret == MafNotExist)
		ERR_OIPROXY("OIProxy_handleCcbAbort(): The transaction does not exist");
	else if(ret != MafOk)
		ERR_OIPROXY("OIProxy_handleCcbAbort(): Failed to perform abort()");

	LEAVE_OIPROXY();
}

/**
 * This callback is called to commit all changes. Also all used data will be cleaned
 * for a certain ccbId.
 * This function is used by the main implementer.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	ccbId			Ccb identifier
 */
void OIProxy_handleCcbApply(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleCcbApply() entered");
	//	Get the transaction for ccbId
	MafOamSpiTransactionHandleT txHandle;
	if(!getCcbTransaction(ccbId, &txHandle)) {
		ERR_OIPROXY("OIProxy_handleCcbApply(): COM OI Failed to get transaction handle");
		LEAVE_OIPROXY();
		return;
	}

	// Remove the ccBId from the map since it is not needed any more.
	removeCcbTransaction(ccbId);

	//	Commit the transaction using the transaction master
	MafReturnT ret = txMaster->commit(txHandle);
	if(ret == MafNotExist)
		ERR_OIPROXY("OIProxy_handleCcbApply(): the transaction does not exist or no participants are registered");
	else if(ret == MafCommitFailed)
		ERR_OIPROXY("OIProxy_handleCcbApply(): one or more participants failed to commit");
	else if(ret == MafPrepareFailed)
		ERR_OIPROXY("OIProxy_handleCcbApply(): a participant failed to prepare");
	else if(ret == MafValidationFailed)
		ERR_OIPROXY("OIProxy_handleCcbApply(): the validation failed");
	else if(ret != MafOk)
		ERR_OIPROXY("OIProxy_handleCcbApply(): the validation failed");

	LEAVE_OIPROXY();
}

/**
 * This callback is called to validate data. In the function, explicitPrepare will be called
 * to confirm that the validation is successful.
 * This function is used by the main implementer.
 *
 * @param	IMM OI handle
 * @ccbId	Ccb identifier
 * @return	On successful validation, SA_AIS_OK will be returned,
 * 			otherwise the appropriate SA error code will be returned
 */
SaAisErrorT OIProxy_handleCcbCompleted(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleCcbCompleted() entered");
	MafReturnT ret = MafOk;
	MafOamSpiTransactionHandleT txHandle;

	//	Check errors from the multiply applier (complex type attributes)
	SaAisErrorT errCode = getCcbApplierError(ccbId);
	if(errCode != SA_AIS_OK) {
		LEAVE_OIPROXY();
		return errCode;
	}

	//	Get transaction handle
	if(!getCcbTransaction(ccbId, &txHandle)) {
		ERR_OIPROXY("OIProxy_handleCcbCompleted(): COM OI Failed to get transaction handle");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	// Send if something has left in the cache
	if(OIProxy_cache_existCacheData(ccbId) == MafOk) {
		MafOamSpiManagedObject_3T *moIf = OIProxy_cache_getCacheMOIf(ccbId);
		if(!moIf) {
			ERR_OIPROXY("OIProxy_handleCcbApply(): COM OI Failed to get MO Interface");
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}

		if((ret = OIProxy_cache_sendCache(moIf, txHandle, ccbId)) != MafOk) {
			ERR_OIPROXY("OIProxy_handleCcbCompleted(): Failed to send OI Proxy cache. Com error code(%d)", (int)ret);
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}

	//	Call explicit prepare to confirm everything is fine with data
	ret = txMaster->explicitPrepare(txHandle);
	if(ret != MafOk) {
		//	Error messages
		if(GetSet_ErrorMessages(ccbId) != MafOk)
		{
			ERR_OIPROXY("OIProxy_handleCcbCompleted(): COM OI Failed to GetSet_ErrorMessages()");
		}

		//	If explicitPrepare fails, we need to clean OIProxy cache and behave like abort happen
		//	OIProxy cache needs to be cleaned, and transaction needs to be removed
		// Remove ccbId data from the cache
		OIProxy_cache_ccbAbort(ccbId);
		// Remove the ccBId from the map since it is not needed any more.
		removeCcbTransaction(ccbId);

		if(ret == MafNotExist)
		{
			ERR_OIPROXY("OIProxy_handleCcbCompleted(): the transaction does not exist or no participants are registered");
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}
		else if(ret == MafPrepareFailed)
		{
			ERR_OIPROXY("OIProxy_handleCcbCompleted(): one or more participants failed to prepare");
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}
		else
		{
			ERR_OIPROXY("OIProxy_handleCcbCompleted(): Failed to perform explicitPrepare(). COM error code: %d", (int)ret);
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}
	}

	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/*
 * Create object callback for registered classes.
 * This function is used by the main implementer.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	ccbId			Ccb identifier
 * @param	className		Class name for the creating object
 * @param	parentName		Parent name of the creating object
 * @param	attr			List of attributes that belong to the new createing object
 * @return	SA_AIS_OK of successful object creation, otherwise other SA error code
 */
SaAisErrorT OIProxy_handleCcbCreateObject(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
										  const SaImmClassNameT className,
										  const SaNameT *parentName,
										  const SaImmAttrValuesT_2 **attr) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleCcbCreateObject ENTER with immOiHandle=(%llu) ccbId=(%llu) className=(%s)", immOiHandle, ccbId, className);
	if (parentName != NULL) {
		DEBUG_OIPROXY("OIProxy_handleCcbCreateObject ENTER with parentName=(%s)", saNameGet(parentName));
	}

	//	Check errors from the multiply applier (complex type attributes)
	SaAisErrorT errCode = getCcbApplierError(ccbId);
	if(errCode != SA_AIS_OK)
	{
		LEAVE_OIPROXY();
		return errCode;
	}

	//	Get RDN from the attribute list
	SaImmAttrValuesT_2 *rdnAttr = getRDNAttribute((SaImmAttrValuesT_2 **)attr, className);
	if(!rdnAttr)
	{
		ERR_OIPROXY("Cannot find RDN attribute");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	Create IMM DN
	std::string immDn = *(char **)(rdnAttr->attrValues[0]);
	if(parentName != NULL)
	{
		if(saNameLen(parentName) > 0)
		{
			immDn.append(",");
			immDn.append(saNameGet(parentName), saNameLen(parentName));
		}
	}
	DEBUG_OIPROXY("OIProxy_handleCcbCreateObject(): immDn=(%s)",immDn.c_str());

	std::string moDn = theTranslator.ImmRdn2MOTop(immDn);
	DEBUG_OIPROXY("OIProxy_handleCcbCreateObject(): the full moDn=(%s)",moDn.c_str());

	//	Create MOC path from MO dn
	std::string mocPath;
	createMocPath(moDn.c_str(), mocPath);
	DEBUG_OIPROXY("OIProxy_handleCcbCreateObject(): mocPath=(%s)",mocPath.c_str());

	//	split MOC path into MOC path fragments
	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	RegisteredCommonStructT *spiIf;
	//	Check is the new object is registered
	//	This callback is called only for classes regsitered by registerClass,
	//	so, we shall check only class registration
	if(!existRegisteredClass((*mocPathList.rbegin()).c_str()))
	{
		ERR_OIPROXY("OIProxy_handleCcbCreateObject(): %s is not registered", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	else
	{
		spiIf = (RegisteredCommonStructT *)getRegisteredClass((*mocPathList.rbegin()).c_str());
	}

	//	Double check. This case should never happen
	if(!spiIf)
	{
		ERR_OIPROXY("OIProxy_handleCcbCreateObject(): Error while creating %s", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Open transaction
	MafOamSpiTransactionHandleT txHandle;
	if(!openTransaction(ccbId, spiIf, &txHandle))
	{
		ERR_OIPROXY("OIProxy_handleCcbCreateObject(): Cannot open transaction (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get managed object interface
	MafOamSpiManagedObject_3T *moIf;
	if(!getMoIf(spiIf, &moIf))
	{
		ERR_OIPROXY("OIProxy_handleCcbCreateObject(): Cannot retrieve managed object interface (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	RegisteredMoInterfaceT moIfVersion;
	if (!getMoIfVersion(spiIf, moIfVersion))
	{
		ERR_OIPROXY("OIProxy_handleAdminOperation()::getMoIfVersion() Unable to get Registered interface version.");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get all object attributes
	std::list<SaImmAttrValuesT_2 *> attrList;
	getImmClassAttributes((SaImmAttrValuesT_2 **)attr, false, attrList);

	// Remove the tail of the moDn path, since we not should send up the object that we want to create.
	std::string fullParentMoDn(moDn.c_str(), moDn.find_last_of(","));

	//	Get object's key
	std::string key;
	std::string keyValue = *(char**)(rdnAttr->attrValues[0]);
	if(!getFragmentKey(keyValue, key))
	{
		ERR_OIPROXY("OIProxy_handleCcbCreateObject(): Cannot get key attribute (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Call createMo
	MafReturnT com_rc = OIProxy_cache_createMo(moIf, txHandle, ccbId, fullParentMoDn.c_str(), className, key.c_str(), keyValue.c_str(), (moIfVersion == MafOamSpiManagedObject2));
	if(com_rc != MafOk)
	{
		com_rc = GetSet_ErrorMessages(ccbId);
		if(com_rc != MafOk)
		{
			ERR_OIPROXY("OIProxy_handleCcbCreateObject(): COM OI Failed to GetSet_ErrorMessages(), rc = %d", com_rc);
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}
		LEAVE_OIPROXY();
		return SA_AIS_ERR_BAD_OPERATION;
	}

	// If more attributes where provided in this create operation we send them to COM using the setMoAttribute function
	if(attrList.size() > 0)
	{
		//	Get MOC root element
		MafOamSpiMocT *mocRoot;
		DEBUG_OIPROXY("OIProxy_handleCcbCreateObject: calling getModelRepository()");
		getModelRepository()->getTreeRoot(&mocRoot);

		//	Get MOC element from MOC path list
		MafOamSpiMocT *moc = findComMoc(mocRoot, mocPathList);
		if(!moc) {	//	this case should never happen
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}

		MafMoAttributeValueContainer_3T attrCnt;
		// Fix for type DERIVED which do not exist in COM MO SPI3
		MafOamSpiMoAttributeTypeT basetype;
		MafOamSpiMoAttributeT *moAttr;
		// In this loop we set all attributes provided in the attr list.
		// But we must not send up the key/keyvalue, it is already provided in the createMo call.
		for(std::list<SaImmAttrValuesT_2 *>::iterator it = attrList.begin(); it != attrList.end(); it++)
		{
			//	Find the attribute
			moAttr = moc->moAttribute;
			while(moAttr)
			{
				if(!strcmp(moAttr->generalProperties.name, (*it)->attrName))
					break;
				moAttr = moAttr->next;
			}

			//	If the attribute is not found, continue with the next attribute
			if(!moAttr)
				continue;

			//	Complex type are not handled here.
			//	There will be a callback for each created complex type attribute in the applier's callback
			if(moAttr->type == (MafOamSpiMoAttributeTypeT) MafOamSpiMoAttributeType_3_STRUCT)
				continue;

			// Check for the key attribute, if key then skip the attribute.
			if(moAttr->isKey)
				continue;

			// Fix for TR HR62291
			basetype = CorrectBaseType(moAttr);

			//	Create attribute container
			createAttributeContainerContent(*it,
											(MafOamSpiMoAttributeType_3T) ((basetype == (MafOamSpiMoAttributeTypeT) MafOamSpiMoAttributeType_3_STRUCT) ? (MafOamSpiMoAttributeTypeT) MafOamSpiMoAttributeType_3_STRING : basetype),
											&attrCnt);

			//	Call setMoAttribute for the attribute
			com_rc = OIProxy_cache_setMoAttribute(moIf, txHandle, ccbId, moDn.c_str(), (*it)->attrName,
										 (MafOamSpiMoAttributeType_3T) basetype, &attrCnt, (moIfVersion == MafOamSpiManagedObject2));

			//	Free allocated memory
			freeAttributeContainerContent(&attrCnt, false);

			//	Error messages
			if(com_rc != MafOk)
			{
				ERR_OIPROXY("OIProxy_handleCcbCreateObject(): COM OI Failed to setMoAttribute(), rc = %d",com_rc);
				// get error string messages
				com_rc=GetSet_ErrorMessages(ccbId);
				if(com_rc != MafOk)
				{
					ERR_OIPROXY("OIProxy_handleCcbCreateObject(): COM OI Failed to GetSet_ErrorMessages(), rc = %d",com_rc);
					LEAVE_OIPROXY();
					return SA_AIS_ERR_FAILED_OPERATION;
				}
				LEAVE_OIPROXY();
				return SA_AIS_ERR_FAILED_OPERATION;
			}
		}
	}
	DEBUG_OIPROXY("OIProxy_handleCcbCreateObject RETURN with SA_AIS_OK");
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/**
 * This callback is called when a MO instance is deleted from IMM.
 * This function is used by the main implementer.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	ccbId			Ccb identifier
 * @param	objectName		Pointer to the object name which will be deleted
 * @return	SA_AIS_OK, on error other SA error code
 */
SaAisErrorT OIProxy_handleCcbDeleteObject(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaNameT *objectName) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleCcbDeleteObject() entered");
	//	Check errors from the multiply applier (complex type attributes)
	SaAisErrorT errCode = getCcbApplierError(ccbId);
	if(errCode != SA_AIS_OK) {
		LEAVE_OIPROXY();
		return errCode;
	}

	//	Check input parameters
	if(!objectName) {
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Save imm DN to immDn
	std::string immDn(saNameGet(objectName), saNameLen(objectName));

	//	Parse immDn, and get MO dn, complex type attribute and index of complex type attribute
	//	If the object is not complex type attribute, ctAttr will be empty and index will be -1
	std::string moDn;
	std::string ctAttr;
	int index;
	if(!parseImmDN(immDn.c_str(), moDn, ctAttr, &index)) {
		ERR_OIPROXY("Error in parsing IMM DN");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	if(index != -1) {
		DEBUG_OIPROXY("The object is complex type attribute");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	Create MOC path from MO dn
	std::string mocPath;
	createMocPath(moDn.c_str(), mocPath);

	//	Split MOC path into fragments
	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	//	Check if the object is registered
	RegisteredCommonStructT *spiIf;
	if(!existRegisteredDN(moDn.c_str())) {
		if(!existRegisteredClass((*mocPathList.rbegin()).c_str())) {
			ERR_OIPROXY("%s is not registered", immDn.c_str());
			LEAVE_OIPROXY();
			return SA_AIS_OK;
		} else
			spiIf = (RegisteredCommonStructT *)getRegisteredClass((*mocPathList.rbegin()).c_str());
	} else
		spiIf = (RegisteredCommonStructT *)getRegisteredDN(moDn.c_str());

	//	Double check. This case should never happen
	if(!spiIf) {
		ERR_OIPROXY("Error while creating %s", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get transaction handle
	MafOamSpiTransactionHandleT txHandle;
	if(!openTransaction(ccbId, spiIf, &txHandle)) {
		ERR_OIPROXY("Cannot open transaction (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get managed object interface
	MafOamSpiManagedObject_3T *moIf;
	if(!getMoIf(spiIf, &moIf)) {
		ERR_OIPROXY("Cannot retrieve managed object interface (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	RegisteredMoInterfaceT moIfVersion;
	if (!getMoIfVersion(spiIf, moIfVersion))
	{
		ERR_OIPROXY("OIProxy_handleAdminOperation()::getMoIfVersion() Unable to get Registered interface version.");
	}

	//	Delete the object
	MafReturnT com_rc = OIProxy_cache_deleteMo(moIf, txHandle, ccbId, moDn.c_str(), (moIfVersion == MafOamSpiManagedObject2));
	if(com_rc != MafOk)
	{
		ERR_OIPROXY("OIProxy_handleCcbDeleteObject(): COM OI Failed to deleteMo(), rc = %d",com_rc);
		// get error string messages
		com_rc=GetSet_ErrorMessages(ccbId);
		if(com_rc != MafOk)
		{
			ERR_OIPROXY("OIProxy_handleCcbDeleteObject(): COM OI Failed to GetSet_ErrorMessages(), rc = %d",com_rc);
			LEAVE_OIPROXY();
			return SA_AIS_ERR_FAILED_OPERATION;
		}
		LEAVE_OIPROXY();
		return SA_AIS_ERR_FAILED_OPERATION;
	}
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/**
 * This callback is called then some object's attribute is modified. Only MO instances will be handled here.
 * This function is used by the main implementer.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	ccbId			Ccb identifier
 * @param	objectName		Pointer to the object name
 * @param	attrMods		NULL terminated list of modified attributes and their values
 * @return	SA_AIS_OK, on error returns other SA error code
 */
SaAisErrorT OIProxy_handleCcbModification(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
											const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods) {

	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleCcbModification() entered");
	//	Check errors from the multiply applier (complex type attributes)
	SaAisErrorT errCode = getCcbApplierError(ccbId);
	if(errCode != SA_AIS_OK) {
		LEAVE_OIPROXY();
		return errCode;
	}

	//	Check input parameters
	if(!objectName || !attrMods) {
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	MafOamSpiMocT *mocRoot;
	MafOamSpiMocT *moc = NULL;

	//	Save IMM dn to immDn
	std::string immDn(saNameGet(objectName), saNameLen(objectName));

	//	Parse immDn, and get MO dn, complex type attribute and index of complex type attribute
	//	If the object is not complex type attribute, ctAttr will be empty and index will be -1
	std::string moDn;
	std::string ctAttr;
	int index;
	if(!parseImmDN(immDn.c_str(), moDn, ctAttr, &index)) {
		ERR_OIPROXY("Error in parsing IMM DN");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	Check if the object is an MO instance or complex type attribute
	if(index != -1) {
		DEBUG_OIPROXY("Not interested in complex type attributes");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	Create MOC path from MO dn
	std::string mocPath;
	createMocPath(moDn.c_str(), mocPath);

	//	Split MOC path into fragments
	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	//	Check if the object is registered
	RegisteredCommonStructT *spiIf;
	if(!existRegisteredDN(moDn.c_str())) {
		if(!existRegisteredClass((*mocPathList.rbegin()).c_str())) {
			ERR_OIPROXY("%s is not registered", immDn.c_str());
			LEAVE_OIPROXY();
			return SA_AIS_OK;
		} else
			spiIf = (RegisteredCommonStructT *)getRegisteredClass((*mocPathList.rbegin()).c_str());
	} else
		spiIf = (RegisteredCommonStructT *)getRegisteredDN(moDn.c_str());

	//	Double check. This case should never happen
	if(!spiIf) {
		ERR_OIPROXY("Error while creating %s", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get transaction handle
	MafOamSpiTransactionHandleT txHandle;
	if(!openTransaction(ccbId, spiIf, &txHandle)) {
		ERR_OIPROXY("Cannot open transaction (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get managed object interface
	MafOamSpiManagedObject_3T *moIf;
	if(!getMoIf(spiIf, &moIf)) {
		ERR_OIPROXY("Cannot retrieve managed object interface (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	RegisteredMoInterfaceT moIfVersion;
	if (!getMoIfVersion(spiIf, moIfVersion))
	{
		ERR_OIPROXY("OIProxy_handleCcbModification() Unable to get Registered interface version.");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get MOC tree
	DEBUG_OIPROXY("OIProxy_handleCcbModification: calling getModelRepository()");
	getModelRepository()->getTreeRoot(&mocRoot);

	//	Find MOC element for the object
	moc = findComMoc(mocRoot, mocPathList);
	if(!moc) {	//	this case should never happen
		ERR_OIPROXY("Fatal error. Cannot find known MOC path.");
		LEAVE_OIPROXY();
		return SA_AIS_ERR_BAD_OPERATION;
	}

	int i=0;
	MafOamSpiMoAttributeT *moAttr;
	// Fix for type DERIVED which do not exist in COM MO SPI3
	MafOamSpiMoAttributeTypeT basetype;
	MafMoAttributeValueContainer_3T attrContainer;
	//	For each modified attribute...
	while(attrMods[i]) {
		//	check modification types of the modified attribute
		if(attrMods[i]->modType != SA_IMM_ATTR_VALUES_ADD && attrMods[i]->modType != SA_IMM_ATTR_VALUES_DELETE && attrMods[i]->modType != SA_IMM_ATTR_VALUES_REPLACE) {
			ERR_OIPROXY("Unknown modification type (%d)", attrMods[i]->modType);
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}

		//	Find MOC attribute element
		moAttr = moc->moAttribute;
		while(moAttr) {
			if(!strcmp(moAttr->generalProperties.name, attrMods[i]->modAttr.attrName))
				break;
			moAttr = moAttr->next;
		}

		//	if the attribute is not found, continue with the next modified attribute
		if(!moAttr) {
			i++;
			continue;
		}

		// Fix for TR HR62291
		basetype = CorrectBaseType(moAttr);

		//	Struct attribute will be skipped from setMoAttribute for objects
		//	Only deleted (attrValuesNumber == 0) complex type attributes will be handled
		if((basetype == (MafOamSpiMoAttributeTypeT) MafOamSpiMoAttributeType_3_STRUCT)
		   && (attrMods[i]->modAttr.attrValuesNumber > 0)) {
			i++;
			continue;
		}

		memset(&attrContainer, 0, sizeof(attrContainer));

		//	Fill in data
		if(basetype == (MafOamSpiMoAttributeTypeT) MafOamSpiMoAttributeType_3_STRUCT) {
			if(attrMods[i]->modType != SA_IMM_ATTR_VALUES_DELETE && attrMods[i]->modType != SA_IMM_ATTR_VALUES_REPLACE) {
				i++;
				continue;
			}
			attrContainer.type = (MafOamSpiMoAttributeType_3T) basetype;
		} else if(attrMods[i]->modType == SA_IMM_ATTR_VALUES_ADD || attrMods[i]->modType == SA_IMM_ATTR_VALUES_REPLACE) {
			createAttributeContainerContent((SaImmAttrValuesT_2 *)&attrMods[i]->modAttr,
											(MafOamSpiMoAttributeType_3T) basetype, &attrContainer);
		} else if(attrMods[i]->modType == SA_IMM_ATTR_VALUES_DELETE) {
			attrContainer.type = (MafOamSpiMoAttributeType_3T) basetype;
		} else {
			ERR_OIPROXY("Unknown modification type (%d)", attrMods[i]->modType);
			LEAVE_OIPROXY();
			return SA_AIS_ERR_BAD_OPERATION;
		}

		//	Call setMoAttribute for the modified attribute
		MafReturnT com_rc = OIProxy_cache_setMoAttribute(moIf, txHandle, ccbId, moDn.c_str(), attrMods[i]->modAttr.attrName,
														 (MafOamSpiMoAttributeType_3T) basetype, &attrContainer, (moIfVersion == MafOamSpiManagedObject2));

		//	Free allocated memory
		freeAttributeContainerContent(&attrContainer, false);

		//	Error messages
		if(com_rc != MafOk)
		{
			ERR_OIPROXY("OIProxy_handleCcbModification(): COM OI Failed to setMoAttribute(), rc = %d",com_rc);
			// get error string messages
			com_rc=GetSet_ErrorMessages(ccbId);
			if(com_rc != MafOk)
			{
				ERR_OIPROXY("OIProxy_handleCcbModification(): COM OI Failed to GetSet_ErrorMessages(), rc = %d",com_rc);
				LEAVE_OIPROXY();
				return SA_AIS_ERR_FAILED_OPERATION;
			}
			LEAVE_OIPROXY();
			return SA_AIS_ERR_FAILED_OPERATION;
		}

		i++;
	}
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

bool getStructClassNameFromCOM(const char* attrName, OamSACache::MocPathList mocPathList, std::string &className)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("getStructClassNameFromCOM(): ENTER");
	MafOamSpiMocT *mocRoot;
	//	Get MOC root element
	DEBUG_OIPROXY("getStructClassNameFromCOM: calling getModelRepository()");
	getModelRepository()->getTreeRoot(&mocRoot);

	//	Find MOC element of the object from MOC path fragments
	MafOamSpiMocT *moc = NULL;
	moc = findComMoc(mocRoot, mocPathList);
	if(!moc)
	{
		DEBUG_OIPROXY("getStructClassNameFromCOM(): findComMoc failed, return false");
		LEAVE_OIPROXY();
		return false;
	}
	MafOamSpiMoAttributeT *moAttr;
	//	Get MOC element of the complex type attribute
	moAttr = getMocAttribute(moc, attrName);
	if(!moAttr)
	{
		DEBUG_OIPROXY("getStructClassNameFromCOM(): Cannot find MO attribute (%s) in the class, return false", attrName);
		ERR_OIPROXY("Cannot find MO attribute (%s) in the class", attrName);
		LEAVE_OIPROXY();
		return false;
	}

	//	Double check. This case should never happen
	if(moAttr->type != (MafOamSpiMoAttributeTypeT) MafOamSpiMoAttributeType_3_STRUCT)
	{
		DEBUG_OIPROXY("getStructClassNameFromCOM(): Attribute is not a struct(%d)", moAttr->type);
		ERR_OIPROXY("Attribute (%s) is not a struct", attrName);
		LEAVE_OIPROXY();
		return false;
	}

	className = moAttr->structDatatype->generalProperties.name;
	DEBUG_OIPROXY("getStructClassNameFromCOM(): RETURN with true className (%s)", className.c_str());
	LEAVE_OIPROXY();
	return true;
}

void removeStructInstancesFromIMM(runtimeStructInstancesT* instanceList)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("removeStructInstancesFromIMM(): ENTER, size of runtimeStructInstances map(%lu)",(unsigned long)instanceList->size());
	for(runtimeStructInstancesIteratorT it = instanceList->begin(); it != instanceList->end(); it++)
	{
		SaNameT objectName;
		DEBUG_OIPROXY("removeStructInstancesFromIMM(): calling saImmOiRtObjectDelete() with objName(%s)",it->c_str());
		saNameSet(it->c_str(), &objectName);
		SaAisErrorT delRet = saImmOiRtObjectDelete(immOiHandle,	(const SaNameT*)&objectName);
		if (delRet == SA_AIS_OK)
		{
			DEBUG_OIPROXY("removeStructInstancesFromIMM():    saImmOiRtObjectDelete SUCCESS");
		}
		else
		{
			DEBUG_OIPROXY("removeStructInstancesFromIMM():    saImmOiRtObjectDelete FAILED (%d)",delRet);
		}
	}
	// clear the list
	instanceList->clear();
	DEBUG_OIPROXY("removeStructInstancesFromIMM(): RETURN");
	LEAVE_OIPROXY();
}

void removeNewStructInstancesFromIMM(runtimeStructInstancesT *deleteList, std::string value)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("removeNewStructInstancesFromIMM(): ENTER with value(%s)",value.c_str());
	runtimeStructInstancesIteratorT it = deleteList->begin();
	for(; it != deleteList->end(); it++)
	{
		if(*it == value)
		{
			break;
		}
	}
	if(it == deleteList->end())
	{
		SaNameT objectName;
		DEBUG_OIPROXY("removeNewStructInstancesFromIMM(): calling saImmOiRtObjectDelete() with objName(%s)",value.c_str());
		saNameSet(value.c_str(), &objectName);
		SaAisErrorT delRet = saImmOiRtObjectDelete(immOiHandle,	(const SaNameT*)&objectName);
		if (delRet == SA_AIS_OK)
		{
			DEBUG_OIPROXY("removeNewStructInstancesFromIMM():    saImmOiRtObjectDelete SUCCESS");
		}
		else
		{
			// it is not an error since we delete the struct instance (before we create the new) and we don't know if it was there before or not
			DEBUG_OIPROXY("removeNewStructInstancesFromIMM():    saImmOiRtObjectDelete FAILED (%d)",delRet);
		}
	}
	else
	{
		DEBUG_OIPROXY("removeNewStructInstancesFromIMM(): struct object already deleted from IMM");
	}
	// clear the list
	deleteList->clear();
	DEBUG_OIPROXY("removeNewStructInstancesFromIMM(): RETURN");
	LEAVE_OIPROXY();
}

/* Input: container
 * Output: attrValues
 */
//HS37161
void createRuntimeStructAttrMods(std::string rdnValue, MafMoAttributeValueStructMember_3* SM, SaImmAttrValuesT_2*** attrValues)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("createRuntimeStructAttrMods(): ENTER with rdnValue(%s)", rdnValue.c_str());
	if (SM == NULL)
	{
		DEBUG_OIPROXY("createRuntimeStructAttrMods(): SM = NULL, empty list");
	}
	// 1 for RDN value
	unsigned int nrOfMembers = 1;
	std::list<std::string> attrNameList;
	std::list<SaImmValueTypeT> attrTypeList;                    // store the IMM attr type in order

	std::list<std::list<int32_t> > attrInt32ValueList;          // store the boolean, enum, int8_t, int16_t, int32_t attr values
	std::list<std::list<int64_t> > attrInt64ValueList;          // store the int64_t attr values
	std::list<std::list<uint32_t> > attrUInt32ValueList;        // store the uint8_t, uint16_t, uint32_t attr values
	std::list<std::list<uint64_t> > attrUInt64ValueList;        // store the uint64_t attr values
	std::list<std::list<std::string> > attrStringValueList;     // store the string and moRef attr values
	std::list<std::list<double> > attrDec64ValueList;           // store the double attr values

	std::list<int32_t> memAttrInt32ValueList;                   // store the element list boolean, enum, int8_t, int16_t, int32_t attr values
	std::list<int64_t> memAttrInt64ValueList;                   // store the element list int64_t and enumerate attr values
	std::list<uint32_t> memAttrUInt32ValueList;                 // store the element list uint8_t, uint16_t, uint32_t attr values
	std::list<uint64_t> memAttrUInt64ValueList;                 // store the element list uint64_t attr values
	std::list<std::string> memAttrStringValueList;              // store the element list string and moRef attr values
	std::list<double> memAttrDec64ValueList;                    // store the element list double attr values

	// first member always is rdn id
	std::string rdnName = "id";
	attrNameList.push_back(rdnName);
	memAttrStringValueList.push_back(rdnValue);
	attrStringValueList.push_back(memAttrStringValueList);
	attrTypeList.push_back(SA_IMM_ATTR_SASTRINGT);

	// explore MafMoAttributeValueStructMember_3 SM list
	while (SM != NULL)
	{
		if (SM->memberName == NULL)
		{
			DEBUG_OIPROXY("createRuntimeStructAttrMods:    SM->memberName = NULL");
			break;
		}
		else
		{
			if (SM->memberValue->nrOfValues > 0)
			{
				switch (SM->memberValue->type)
				{
				case MafOamSpiMoAttributeType_3_INT8:
					memAttrInt32ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrInt32ValueList.push_back(SM->memberValue->values[i].value.i8);
					}
					if (!memAttrInt32ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAINT32T);
						attrInt32ValueList.push_back(memAttrInt32ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_INT16:
					memAttrInt32ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrInt32ValueList.push_back(SM->memberValue->values[i].value.i16);
					}
					if (!memAttrInt32ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAINT32T);
						attrInt32ValueList.push_back(memAttrInt32ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_INT32:
					memAttrInt32ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrInt32ValueList.push_back(SM->memberValue->values[i].value.i32);
					}
					if (!memAttrInt32ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAINT32T);
						attrInt32ValueList.push_back(memAttrInt32ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_INT64:
					memAttrInt64ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrInt64ValueList.push_back(SM->memberValue->values[i].value.i64);
					}
					if (!memAttrInt64ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAINT64T);
						attrInt64ValueList.push_back(memAttrInt64ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_UINT8:
					memAttrUInt32ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrUInt32ValueList.push_back(SM->memberValue->values[i].value.u8);
					}
					if (!memAttrUInt32ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAUINT32T);
						attrUInt32ValueList.push_back(memAttrUInt32ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_UINT16:
					memAttrUInt32ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrUInt32ValueList.push_back(SM->memberValue->values[i].value.u16);
					}
					if (!memAttrUInt32ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAUINT32T);
						attrUInt32ValueList.push_back(memAttrUInt32ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_UINT32:
					memAttrUInt32ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrUInt32ValueList.push_back(SM->memberValue->values[i].value.u32);
					}
					if (!memAttrUInt32ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAUINT32T);
						attrUInt32ValueList.push_back(memAttrUInt32ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_UINT64:
					memAttrUInt64ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrUInt64ValueList.push_back(SM->memberValue->values[i].value.u64);
					}
					if (!memAttrUInt64ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAUINT64T);
						attrUInt64ValueList.push_back(memAttrUInt64ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_STRING:
					memAttrStringValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						if (SM->memberValue->values[i].value.theString != NULL)
						{
							memAttrStringValueList.push_back(SM->memberValue->values[i].value.theString);
						}
					}
					if (!memAttrStringValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SASTRINGT);
						attrStringValueList.push_back(memAttrStringValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_BOOL:
					memAttrInt32ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrInt32ValueList.push_back(SM->memberValue->values[i].value.theBool ? 1 : 0);
					}
					if (!memAttrInt32ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAINT32T);
						attrInt32ValueList.push_back(memAttrInt32ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_REFERENCE:
					memAttrStringValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						if (SM->memberValue->values[i].value.moRef != NULL)
						{
							char *the3gppName = (char *) malloc(strlen(SM->memberValue->values[i].value.moRef) + 1);
							char *immName;
							strcpy(the3gppName, SM->memberValue->values[i].value.moRef);
							theTranslator.MO2Imm_DN(the3gppName, &immName);
							DEBUG_OIPROXY("createRuntimeStructAttrMods: the3gppName: %s, immName: %s", the3gppName, immName);
							memAttrStringValueList.push_back(immName);
							free(immName);
							free(the3gppName);
						}
					}
					if (!memAttrStringValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SANAMET);
						attrStringValueList.push_back(memAttrStringValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_ENUM:
					memAttrInt32ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrInt32ValueList.push_back((int32_t) SM->memberValue->values[i].value.theEnum);
					}
					if (!memAttrInt32ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SAINT32T);
						attrInt32ValueList.push_back(memAttrInt32ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_DECIMAL64:
					memAttrDec64ValueList.clear();
					for (uint32_t i = 0; i < SM->memberValue->nrOfValues; i++)
					{
						memAttrDec64ValueList.push_back(SM->memberValue->values[i].value.decimal64);
					}
					if (!memAttrDec64ValueList.empty())
					{
						attrNameList.push_back(SM->memberName);
						attrTypeList.push_back(SA_IMM_ATTR_SADOUBLET);
						attrDec64ValueList.push_back(memAttrDec64ValueList);
						nrOfMembers++;
					}
					break;
				case MafOamSpiMoAttributeType_3_STRUCT:
					DEBUG_OIPROXY("createRuntimeStructAttrMods(): memberValue type MafOamSpiMoAttributeType_3_STRUCT is not supported");
					break;
				default:
					ERR_OIPROXY("createRuntimeStructAttrMods(): SM->memberValue->type is invalid");
					break;
				}
			}
		}
		SM = SM->next;
	}
	DEBUG_OIPROXY("createRuntimeStructAttrMods: nrOfMembers(%u)", nrOfMembers);
	SaImmAttrValuesT_2 **attrMods = (SaImmAttrValuesT_2**) malloc((nrOfMembers + 1) * sizeof(SaImmAttrValuesT_2*));
	memset(attrMods, 0, (nrOfMembers + 1) * sizeof(SaImmAttrValuesT_2*));
	for (unsigned int i = 0; i < nrOfMembers; i++)
	{
		//task43204: supports multi-value strings as a member of runtime structs
		attrMods[i] = (SaImmAttrValuesT_2*) malloc(sizeof(SaImmAttrValuesT_2));
		attrMods[i]->attrName = strdup(attrNameList.front().c_str());
		attrNameList.pop_front();
		attrMods[i]->attrValueType = attrTypeList.front();
		attrTypeList.pop_front();

		SaImmAttrValueT *attrV = NULL;

		switch (attrMods[i]->attrValueType)
		{
		case SA_IMM_ATTR_SAINT32T:
			memAttrInt32ValueList = attrInt32ValueList.front();
			attrInt32ValueList.pop_front();
			attrMods[i]->attrValuesNumber = memAttrInt32ValueList.size();
			attrV = (SaImmAttrValueT *) malloc(attrMods[i]->attrValuesNumber * sizeof( SaImmAttrValueT ));
			for (uint32_t j = 0; j < attrMods[i]->attrValuesNumber; j++)
			{
				int32_t* attrValueInt32 = (int32_t*) malloc(sizeof(int32_t));
				*attrValueInt32 = memAttrInt32ValueList.front();
				memAttrInt32ValueList.pop_front();
				attrV[j] = (SaImmAttrValueT) attrValueInt32;
			}
			break;
		case SA_IMM_ATTR_SAUINT32T:
			memAttrUInt32ValueList = attrUInt32ValueList.front();
			attrUInt32ValueList.pop_front();
			attrMods[i]->attrValuesNumber = memAttrUInt32ValueList.size();
			attrV = (SaImmAttrValueT *) malloc(attrMods[i]->attrValuesNumber * sizeof( SaImmAttrValueT ));
			for (uint32_t j = 0; j < attrMods[i]->attrValuesNumber; j++)
			{
				uint32_t* attrValueUInt32 = (uint32_t*) malloc(sizeof(uint32_t));
				*attrValueUInt32 = memAttrUInt32ValueList.front();
				memAttrUInt32ValueList.pop_front();
				attrV[j] = (SaImmAttrValueT) attrValueUInt32;
			}
			break;
		case SA_IMM_ATTR_SAINT64T:
			memAttrInt64ValueList = attrInt64ValueList.front();
			attrInt64ValueList.pop_front();
			attrMods[i]->attrValuesNumber = memAttrInt64ValueList.size();
			attrV = (SaImmAttrValueT *) malloc(attrMods[i]->attrValuesNumber * sizeof( SaImmAttrValueT ));
			for (uint32_t j = 0; j < attrMods[i]->attrValuesNumber; j++)
			{
				int64_t* attrValueInt64 = (int64_t*) malloc(sizeof(int64_t));
				*attrValueInt64 = memAttrInt64ValueList.front();
				memAttrInt64ValueList.pop_front();
				attrV[j] = (SaImmAttrValueT) attrValueInt64;
			}
			break;
		case SA_IMM_ATTR_SAUINT64T:
			memAttrUInt64ValueList = attrUInt64ValueList.front();
			attrUInt64ValueList.pop_front();
			attrMods[i]->attrValuesNumber = memAttrUInt64ValueList.size();
			attrV = (SaImmAttrValueT *) malloc(attrMods[i]->attrValuesNumber * sizeof( SaImmAttrValueT ));
			for (uint32_t j = 0; j < attrMods[i]->attrValuesNumber; j++)
			{
				uint64_t* attrValueUInt64 = (uint64_t*) malloc(sizeof(uint64_t));
				*attrValueUInt64 = memAttrUInt64ValueList.front();
				memAttrUInt64ValueList.pop_front();
				attrV[j] = (SaImmAttrValueT) attrValueUInt64;
			}
			break;
		case SA_IMM_ATTR_SADOUBLET:
			memAttrDec64ValueList = attrDec64ValueList.front();
			attrDec64ValueList.pop_front();
			attrMods[i]->attrValuesNumber = memAttrDec64ValueList.size();
			attrV = (SaImmAttrValueT *) malloc(attrMods[i]->attrValuesNumber * sizeof( SaImmAttrValueT ));
			for (uint32_t j = 0; j < attrMods[i]->attrValuesNumber; j++)
			{
				double *attrValueDec64 = (double*) malloc(sizeof(double));
				*attrValueDec64 = memAttrDec64ValueList.front();
				memAttrDec64ValueList.pop_front();
				attrV[j] = (SaImmAttrValueT) attrValueDec64;
			}
			break;
		case SA_IMM_ATTR_SANAMET:
			memAttrStringValueList = attrStringValueList.front();
			attrStringValueList.pop_front();
			attrMods[i]->attrValuesNumber = memAttrStringValueList.size();
			attrV = (SaImmAttrValueT *) malloc(attrMods[i]->attrValuesNumber * sizeof( SaImmAttrValueT ));
			for (uint32_t j = 0; j < attrMods[i]->attrValuesNumber; j++)
			{
				SaNameT* attrValueNameT = makeSaNameT(memAttrStringValueList.front().c_str());
				memAttrStringValueList.pop_front();
				attrV[j] = (SaImmAttrValueT) attrValueNameT;
			}
			break;
		case SA_IMM_ATTR_SASTRINGT:
			memAttrStringValueList = attrStringValueList.front();
			attrStringValueList.pop_front();
			attrMods[i]->attrValuesNumber = memAttrStringValueList.size();
			attrV = (SaImmAttrValueT *) malloc(attrMods[i]->attrValuesNumber * sizeof( SaImmAttrValueT ));
			for (uint32_t j = 0; j < attrMods[i]->attrValuesNumber; j++)
			{
				char* attrValueCstr = strdup(memAttrStringValueList.front().c_str());
				memAttrStringValueList.pop_front();
				SaStringT *pAttrValueSaStringT = (SaStringT *) malloc(sizeof(SaStringT));
				*pAttrValueSaStringT = attrValueCstr;
				attrV[j] = (SaImmAttrValueT) pAttrValueSaStringT;
			}
			break;
		default:
			ERR_OIPROXY("createRuntimeStructAttrMods(): something is wrong in implementation of this function");
			break;
		}

		attrMods[i]->attrValues = attrV;
	}
	*attrValues = attrMods;
	DEBUG_OIPROXY("createRuntimeStructAttrMods(): RETURN");
	LEAVE_OIPROXY();
}

void freeSaImmAttrValuesT_2(SaImmAttrValuesT_2*** attrValues)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("freeSaImmAttrValuesT_2(): ENTER");
	if(attrValues != NULL)
	{
		if(*attrValues != NULL)
		{
			SaImmAttrValuesT_2** av = *attrValues;
			for(unsigned int i = 0; av[i] != NULL; i++)
			{
				for(unsigned int j = 0; j < av[i]->attrValuesNumber; j++)
				{
					SaAnyT *saAnyTp = NULL;
					SaStringT *saStringTp = NULL;
					switch(av[i]->attrValueType)
					{
					case SA_IMM_ATTR_SASTRINGT:
						saStringTp = (SaStringT *) av[i]->attrValues[j];
						if(*saStringTp)
						{
							DEBUG_OIPROXY("freeSaImmAttrValuesT_2:  av[%u]->attrValues[%u]=(%s):",i, j, *saStringTp);
							free(*saStringTp);
						}
						free(saStringTp);
						break;
					case SA_IMM_ATTR_SAINT32T:
					case SA_IMM_ATTR_SAUINT32T:
					case SA_IMM_ATTR_SAINT64T:
					case SA_IMM_ATTR_SAUINT64T:
					case SA_IMM_ATTR_SAFLOATT:
					case SA_IMM_ATTR_SADOUBLET:
					case SA_IMM_ATTR_SATIMET:
						free(av[i]->attrValues[j]);
						break;
					case SA_IMM_ATTR_SANAMET:
						saNameDelete((SaNameT *)av[i]->attrValues[j], true);
						break;
					case SA_IMM_ATTR_SAANYT:
						saAnyTp = (SaAnyT *)av[i]->attrValues[j];
						if(saAnyTp->bufferAddr)
						{
							free(saAnyTp->bufferAddr);
						}
						free(saAnyTp);
						break;
					default:
						break;
					}
				}
				free(av[i]->attrValues);
				free(av[i]->attrName);
				free(av[i]); // struct
			}
			free(av);
		}
	}
	DEBUG_OIPROXY("freeSaImmAttrValuesT_2(): RETURN");
	LEAVE_OIPROXY();
}


/**
 * This callback is called to read a run-time attribute from managed object interface.
 * This function is used by the main implementer.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	objectName		Pointer to the object name
 * @param	attributeName	List of run-time attributes that need to be read
 * @return	SA_AIS_OK, on error return other SA error code
 */
SaAisErrorT OIProxy_handleRtAttrUpdate(SaImmOiHandleT immOiHandle, const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): ENTER");
	//	Check input parameters
	if(!objectName || !attributeNames)
	{
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): invalid input parameters, returning SA_AIS_ERR_INVALID_PARAM");
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	SaAisErrorT ret = SA_AIS_OK;
	std::string className;

	//	Save IMM dn to immDn
	std::string immDn(saNameGet(objectName), saNameLen(objectName));
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): immOiHandle(%llu) immDn(%s)",immOiHandle,immDn.c_str());
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): attributeNames:");
	for(unsigned int i = 0; attributeNames[i] != NULL; i++)
	{
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate():    (%s)",attributeNames[i]);
	}
	//	Parse immDn, and get MO dn, complex type attribute and index of complex type attribute
	//	If the object is not complex type attribute, ctAttr will be empty and index will be -1
	std::string moDn;
	std::string ctAttr;
	int index;
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): before parseImmDN");
	if(!parseImmDN(immDn.c_str(), moDn, ctAttr, &index)) {
		ERR_OIPROXY("Error in parsing IMM DN");
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): after parseImmDN: moDn(%s) index(%d)",moDn.c_str(), index);
	//	Find MOC path from MO dn
	std::string mocPath;
	createMocPath(moDn.c_str(), mocPath);
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): after createMocPath: mocPath(%s)",mocPath.c_str());

	//	Split MOC path into fragments
	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	//	Check if the object is registered
	//	For now, reading run-time attributes from complex-type attributes are disabled
	RegisteredCommonStructT *spiIf;

	if(!existRegisteredDN(moDn.c_str()))
	{
		if(!existRegisteredClass((*mocPathList.rbegin()).c_str()))
		{
			ERR_OIPROXY("%s is not registered", immDn.c_str());
			LEAVE_OIPROXY();
			return SA_AIS_ERR_FAILED_OPERATION;
		}
		else
		{
			spiIf = (RegisteredCommonStructT *)getRegisteredClass((*mocPathList.rbegin()).c_str());
		}
	}
	else
	{
		spiIf = (RegisteredCommonStructT *)getRegisteredDN(moDn.c_str());
	}
	className = (*mocPathList.rbegin()).c_str();
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): className(%s)", className.c_str());
	//	Double check. This case should never happen
	if(!spiIf)
	{
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): Error while creating %s", immDn.c_str());
		ERR_OIPROXY("Error while creating %s", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Open temporary transaction handle
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): Open temporary transaction handle");
	MafOamSpiTransactionHandleT txHandle;
	if(!openAnonymousTransaction(spiIf, &txHandle))
	{
		ERR_OIPROXY("Cannot open transaction (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get MO interface
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): Get MO interface");
	MafOamSpiManagedObject_3T *moIf;
	if(!getMoIf(spiIf, &moIf))
	{
		ERR_OIPROXY("Cannot retrieve managed object interface (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	RegisteredMoInterfaceT moIfVersion;
	if (!getMoIfVersion(spiIf, moIfVersion))
	{
		ERR_OIPROXY("OIProxy_handleAdminOperation()::getMoIfVersion() Unable to get Registered interface version.");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Count the number of attributes
	int counter = 0;
	while(attributeNames[counter])
		counter++;

	//	Get class description from IMM
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): Get class description from IMM");
	SaImmClassCategoryT classCat;
	SaImmAttrDefinitionT_2 **attrDef = NULL;

	ret = autoRetry_saImmOmClassDescriptionGet_2(immOmHandle, (char *)className.c_str(), &classCat, &attrDef);

	if (SA_AIS_ERR_BAD_HANDLE == ret)
	{
		LOG_OIPROXY("saImmOmClassDescriptionGet_2() returned BAD_HANDLE, reinitializing OI handlers before another attempt");

		ObjImp_finalize_imm(true,false);
		ObjImp_init_imm(true);

		ret = autoRetry_saImmOmClassDescriptionGet_2(immOmHandle, (char *)className.c_str(), &classCat, &attrDef);
	}

	if(ret != SA_AIS_OK)
	{
		DEBUG_OIPROXY("Cannot get class (%s) description", className.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	//	Create output values (input values for saImmOiRtObjectUpdate_2)
	SaImmAttrModificationT_2 **attrMods = (SaImmAttrModificationT_2 **)malloc((counter + 1) * sizeof(SaImmAttrModificationT_2 *));
	memset(attrMods, 0, (counter + 1) * sizeof(SaImmAttrModificationT_2 *));

	int attrIndex;
	int i;
	i = 0;	//	Because of "goto rtend;" :)

	runtimeStructInstancesT deletedRTobjectsList = runtimeStructInstances;
	// removing old struct instances from IMM
	//FIXME: shouldn't be called all the time
	removeStructInstancesFromIMM(&runtimeStructInstances);

	// a map to store the received containers (which can be identified by the attributes)
	runtimeAttrMapT runtimeAttrMap;
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): Search for the attribute");
	while(attributeNames[i])
	{
		//	Search for the attribute in the received IMM description
		for(attrIndex = 0; attrDef[attrIndex]; attrIndex++)
		{
			if(!strcmp(attributeNames[i], attrDef[attrIndex]->attrName))
			{
				break;
			}
		}
		//	Check if the attribute is found in the received IMM description
		if(!attrDef[attrIndex])
		{
			saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): Cannot find attribute %s", attributeNames[i]);
			ret = SA_AIS_ERR_FAILED_OPERATION;
			goto rtend;
		}
		else
		{
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate():    attribute found: (%s)",attributeNames[i]);
		}

		MafMoAttributeValueResult_3T attrCont;
		attrCont.container = NULL;
		attrCont.release = NULL;

		//	Get attribute container for the requested attribute name
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate: [%d] calling getMoAttribute() with (%s) attrName(%s)",i, moDn.c_str(), attributeNames[i]);
		MafReturnT retValue = MafOk;
		if(moIfVersion == MafOamSpiManagedObject3)
		{
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): _3 Search for the attribute");
			retValue = moIf->getMoAttribute(txHandle, moDn.c_str(), attributeNames[i], &attrCont);
		}
		else if (moIfVersion == MafOamSpiManagedObject2)
		{
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): _2 Search for the attribute");
			MafOamSpiManagedObject_2T* moIf_2 = reinterpret_cast<MafOamSpiManagedObject_2T*>(moIf);
			retValue = moIf_2->getMoAttribute(txHandle, moDn.c_str(), attributeNames[i], (MafMoAttributeValueContainer_2T**)(&attrCont.container));
		}

		if(MafOk != retValue)
		{
			saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);
			DEBUG_OIPROXY("getMoAttribute failed for %s", attributeNames[i]);
			ret = SA_AIS_ERR_FAILED_OPERATION;
			goto rtend;
		}
		// save the received container which can be identified by the attribute name
		runtimeAttrMap[attributeNames[i]] = attrCont;

		if(attrCont.container != NULL)
		{
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate: after calling getMoAttribute() with DN: %s, attrName: %s", moDn.c_str(), attributeNames[i]);
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate: Received container:");
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate:    nrOfValues(%u)",attrCont.container->nrOfValues);
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate:    type(%d)",attrCont.container->type);
		}
		else
		{
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate: received attrCont.container = NULL");
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate: returning SA_AIS_ERR_INVALID_PARAM");
			LEAVE_OIPROXY();
			return SA_AIS_ERR_INVALID_PARAM;
		}

		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate():    About to set the attribute data");
		//	Set attribute data (input values for saImmOiRtObjectUpdate_2)
		attrMods[i] = (SaImmAttrModificationT_2 *)malloc(sizeof(SaImmAttrModificationT_2));
		if(attrCont.container->type == MafOamSpiMoAttributeType_3_STRUCT)
		{
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate():    complex type PHASE 4");
			//	Struct attribute. So, we shall fill in return value(s) with its DNs
			attrMods[i]->modType = SA_IMM_ATTR_VALUES_REPLACE;
			attrMods[i]->modAttr.attrName = strdup(attributeNames[i]);
			attrMods[i]->modAttr.attrValueType = SA_IMM_ATTR_SANAMET;
			attrMods[i]->modAttr.attrValuesNumber = attrCont.container->nrOfValues;
			if(attrCont.container->nrOfValues > 0)
			{
				SaNameT *attrVal = NULL;
				std::string tempAttrValue;
				attrMods[i]->modAttr.attrValues = (SaImmAttrValueT *)malloc(sizeof(SaImmAttrValueT) * attrCont.container->nrOfValues);
				for(unsigned int n = 0; n < attrCont.container->nrOfValues; n++)
				{
					// create struct reference string
					tempAttrValue = "id=";
					tempAttrValue.append(attributeNames[i]).append("_");
					char buf[500];
					sprintf(buf,"%u",n);
					tempAttrValue += std::string(buf);
					tempAttrValue.append(",").append(saNameGet(objectName), saNameLen(objectName));
					DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate():       got imm attr value(%s)",tempAttrValue.c_str());

					// save IMM rdn in a list to know what to delete before creating the new struct instances in IMM
					runtimeStructInstances.push_back(tempAttrValue);

					// Try to remove the struct instances from IMM before creating it.
					// The known instances are removed already, because they are stored, but after COMSA restarts the list of the known created instances are lost.
					removeNewStructInstancesFromIMM(&deletedRTobjectsList, tempAttrValue);
					attrVal = makeSaNameT(tempAttrValue.c_str());
					attrMods[i]->modAttr.attrValues[n] = (SaImmAttrValueT)attrVal;
				}
			}
			else
			{
				attrMods[i]->modAttr.attrValues = NULL;
			}
		}
		else
		{
			//	Simple value attribute
			attrMods[i]->modType = SA_IMM_ATTR_VALUES_REPLACE;
			if(!copyMoAttrToImm(attrCont.container, attrDef[attrIndex], &(attrMods[i]->modAttr)))
			{
				saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);
				DEBUG_OIPROXY("Cannot convert data from MO to IMM format for %s", attributeNames[i]);
				ret = SA_AIS_ERR_FAILED_OPERATION;
				goto rtend;
			}
		}
		i++;
	} // end of while

	//	Release memory allocated by saImmOmClassDescriptionGet_2
	saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);

	//	Finally, call saImmOiRtObjectUpdate_2 for the callback
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): call saImmOiRtObjectUpdate_2 for the callback");

	// update attributes in IMM
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): calling saImmOiRtObjectUpdate_2");
	ret = autoRetry_saImmOiRtObjectUpdate_2(immOiHandle, objectName, (const SaImmAttrModificationT_2 **)attrMods);

	// if attribute update was successful then create the struct instances in IMM (if attribute is a struct)
	if(ret == SA_AIS_OK)
	{
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): saImmOiRtObjectUpdate_2 SUCCESS");
		// for each attribute: fill in the data and create struct instance
		for(unsigned int i = 0; attributeNames[i] != NULL; i++)
		{
			runtimeAttrMapIteratorT it = runtimeAttrMap.find(attributeNames[i]);
			// process only structs
			if(it != runtimeAttrMap.end() && (it->second.container->type == MafOamSpiMoAttributeType_3_STRUCT))
			{
				DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate():    get COM className from attr(%s)",attributeNames[i]);
				std::string structClassName;
				if(getStructClassNameFromCOM(attributeNames[i], mocPathList, structClassName))
				{
					DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): struct class name from COM(%s)",structClassName.c_str());
					// find the already stored IMM RDNs (struct reference values)
					for(unsigned int i = 0; attrMods[i] != NULL; i++)
					{
						if(!strcmp(attrMods[i]->modAttr.attrName, attributeNames[i]))
						{
							DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): found attrName(%s)",attributeNames[i]);
							unsigned int nrOfValues = attrMods[i]->modAttr.attrValuesNumber;
							for(unsigned int j = 0; j < nrOfValues; j++)
							{
								SaNameT* attrValueSaNameT = (SaNameT*)attrMods[i]->modAttr.attrValues[j];
								std::string attrValue;
								attrValue.append(saNameGet(attrValueSaNameT), saNameLen(attrValueSaNameT));
								// only the RDN name and value needed together, not the full RDN
								// so remove all characters from first ',' to the end of the string
								size_t pos = attrValue.find(',');
								if(pos != std::string::npos)
								{
									attrValue.erase(pos);
									DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): RDN name-value(%s)",attrValue.c_str());
								}
								else
								{
									DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): not a valid RDN(%s)",attrValue.c_str());
								}
								SaImmAttrValuesT_2 **attrValues;
								createRuntimeStructAttrMods(attrValue, it->second.container->values[j].value.structMember, &attrValues);

								DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): calling saImmOiRtObjectCreate_2 for the struct object in imm");
								const char* structClassNameChar = strdup(structClassName.c_str());
								SaAisErrorT rc = saImmOiRtObjectCreate_2(immOiHandle, (SaImmClassNameT)structClassNameChar, objectName, (const SaImmAttrValuesT_2**)attrValues);
								if (rc == SA_AIS_OK)
								{
									DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): saImmOiRtObjectCreate_2 SUCCESS");
								}
								else
								{
									DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): saImmOiRtObjectCreate_2 FAILED (%d)",rc);
								}

								// free all the data used for saImmOiRtObjectCreate_2() function
								DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): free all the data used for saImmOiRtObjectCreate_2() function");
								if (NULL != structClassNameChar)
								{
									free(const_cast<char *>(structClassNameChar)); // malloc is used in strdup
								}
								freeSaImmAttrValuesT_2(&attrValues);
								DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): attrValues successfully freed");
							}
						}
					}
				}
				else
				{
					DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): failed to get struct class name from COM");
				}

			}
		}
	}
	else
	{
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): saImmOiRtObjectUpdate_2 FAILED(%d)",ret);
	}

rtend:
	//	Release memory allocated for modified attributes
	if(attrMods)
	{
		for(i=0; i<counter; i++)
		{
			if(attrMods[i])
			{
				if(attrMods[i]->modAttr.attrValues)
					{
						freeImmAttr(&attrMods[i]->modAttr, false);
					}
				free(attrMods[i]);
			}
		}
		free(attrMods);
	}
	for(runtimeAttrMapIteratorT i = runtimeAttrMap.begin(); i != runtimeAttrMap.end(); i++)
	{
		if ((i->second.release != NULL) && (i->second.container!= NULL))
		{
			i->second.release(i->second.container);
		}
	}

	MafReturnT comRet = MafOk;
	// if all the previous operations before "rtend" resulted "SA_AIS_OK" then call "explicitPrepare"
	// else call "abort"
	if(ret == SA_AIS_OK)
	{
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): calling explicitPrepare");
		comRet = txMaster->explicitPrepare(txHandle);
		// if "explicitPrepare" returned "MafOk" then call "commit"
		// else do nothing, because "abort" will called automatically
		if (comRet == MafOk)
		{
			DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): calling commit");
			comRet = txMaster->commit(txHandle);
		}
	}
	else
	{
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): calling abort");
		comRet = txMaster->abort(txHandle);
	}

	if(comRet != MafOk)
	{
		DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): operation failed");
		ret = SA_AIS_ERR_FAILED_OPERATION;
	}
	DEBUG_OIPROXY("OIProxy_handleRtAttrUpdate(): RETURN with (%d)",ret);
	LEAVE_OIPROXY();
	return ret;
}

SaAisErrorT OIProxy_RuntimeStructAttrUpdate(SaImmOiHandleT immOiHandle, const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_RuntimeStructAttrUpdate(): ENTER");
	//	Check input parameters
	if(!objectName || !attributeNames)
	{
		DEBUG_OIPROXY("OIProxy_RuntimeStructAttrUpdate(): wrong input data");
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	std::string immDn(saNameGet(objectName), saNameLen(objectName));
	DEBUG_OIPROXY("OIProxy_RuntimeStructAttrUpdate(): immOiHandle(%llu) immDn(%s)",immOiHandle,immDn.c_str());
	DEBUG_OIPROXY("OIProxy_RuntimeStructAttrUpdate(): attributeNames:");
	for(unsigned int i = 0; attributeNames[i] != NULL; i++)
	{
		DEBUG_OIPROXY("OIProxy_RuntimeStructAttrUpdate():    (%s)",attributeNames[i]);
	}
	DEBUG_OIPROXY("OIProxy_RuntimeStructAttrUpdate(): RETURN");
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/***********************************************
 *
 * Complex type (multiply applier) callbacks
 *
 ***********************************************/

/**
 * Abort callback for complex type attributes.
 * In this function complex type attribute error code will be deleted.
 * This function is used by the multiply applier.
 *
 * @param	immOiHandle		immOiHandle
 * @param	ccbId			Ccb identifier
 */
void OIProxy_handleComplexTypeCcbAbort(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbAbort() entered");
	removeCcbApplierError(ccbId);
	LEAVE_OIPROXY();
}

/**
 * Apply callback for complex type attributes.
 * In this function complex type attribute error code will be deleted.
 * This function is used by the multiply applier.
 *
 * @param	immOiHandle		immOiHandle
 * @param	ccbId			Ccb identifier
 */
void OIProxy_handleComplexTypeCcbApply(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbApply() entered");
	removeCcbApplierError(ccbId);
	LEAVE_OIPROXY();
}

/**
 * This function does not do anything, but is here because multiply applier needs to have it.
 * This function is used by the multiply applier.
 *
 * @param	immOiHandle		immOiHandle
 * @param	ccbId			Ccb identifier
 */
SaAisErrorT OIProxy_handleComplexTypeCcbCompleted(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCompleted() entered");
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/**
 * Create object callback for a new complex type attributes.
 * This function is called when an array of a size of a complex type attribute is increased,
 * and new IMM object is created for the complex type attribute.
 * This function is used by the multiply applier.
 *
 * @param	immOiHandle		immOiHandle
 * @param	ccbId			Ccb identifier
 */
SaAisErrorT OIProxy_handleComplexTypeCcbCreateObject(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
											const SaImmClassNameT className,
											const SaNameT *parentName,
											const SaImmAttrValuesT_2 **attr) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject ENTER with immOiHandle=(%llu) ccbId=(%llu) className=(%s)", immOiHandle, ccbId, className);
	if (parentName != NULL) {
		DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject ENTER with parentName=(%s)", saNameGet(parentName));
	}

	//	Get RDN from the attribute list
	SaImmAttrValuesT_2 *rdnAttr = getRDNAttribute((SaImmAttrValuesT_2 **)attr, className);
	if(!rdnAttr)
	{
		ERR_OIPROXY("Cannot find RDN attribute");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	Save RDN value to immDn value
	std::string immDn = *(char **)(rdnAttr->attrValues[0]);

	// save the IMM DN of the object
	std::string objectImmDn;
	if(parentName != NULL)
	{
		if(saNameLen(parentName) > 0)
		{
			objectImmDn.append(saNameGet(parentName), saNameLen(parentName));
		}
	}
	// Create the IMM DN of the struct attribute by appending "," and the parent IMM DN to the RDN
	if(objectImmDn != "")
	{
		immDn.append(",");
		immDn.append(objectImmDn);
	}
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject(): objectImmDn=(%s)",objectImmDn.c_str());
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject(): immDn=(%s)",immDn.c_str());

	//	Get complex type attribute and index of complex type attribute
	//	If the object is not complex type attribute, ctAttr will be empty and index will be -1
	std::string ctAttr;
	int index;
	getCtAttrFromImmDN(immDn.c_str(), ctAttr, &index);
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject(): ctAttr=(%s) index=(%d)",ctAttr.c_str(),index);

	//	Check if the new creating object is MO instance or complex type attribute
	if(index == -1) {
		//	Not interested in for non-complex type attributes
		DEBUG_OIPROXY("Not complex type attribute");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	std::string moDn = theTranslator.ImmRdn2MOTop(objectImmDn);
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject(): the full moDn=(%s)",moDn.c_str());

	//	Create MOC path from MO dn
	std::string mocPath;
	createMocPath(moDn.c_str(), mocPath);
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject(): mocPath=(%s)",mocPath.c_str());
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject(): ctAttr=(%s)",ctAttr.c_str());

	//	Split MOC path into fragments
	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	//	Check if the new complex type attribute IMM class is registered
	RegisteredCommonStructT *spiIf;
	std::string ctPath = moDn + ":" + ctAttr;
	if(!existRegisteredCTClassAttr(className, ctPath.c_str())) {
		ctPath = mocPath + ":" + ctAttr;
		if(!existRegisteredCTClassAttr(className, ctPath.c_str())) {
			ERR_OIPROXY("%s is not registered", immDn.c_str());
			LEAVE_OIPROXY();
			return SA_AIS_OK;
		} else
			spiIf = (RegisteredCommonStructT *)getRegisteredCTClass(className, ctPath.c_str());
	} else
		spiIf = (RegisteredCommonStructT *)getRegisteredCTClass(className, ctPath.c_str());

	//	Double check. This case should never happen
	if(!spiIf) {
		setCcbApplierError(ccbId, SA_AIS_ERR_INVALID_PARAM);
		ERR_OIPROXY("Error while creating %s", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get transaction handle
	MafOamSpiTransactionHandleT txHandle;
	if(!openTransaction(ccbId, spiIf, &txHandle)) {
		setCcbApplierError(ccbId, SA_AIS_ERR_INVALID_PARAM);
		ERR_OIPROXY("Cannot open transaction (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get managed object interface
	MafOamSpiManagedObject_3T *moIf;
	if(!getMoIf(spiIf, &moIf)) {
		setCcbApplierError(ccbId, SA_AIS_ERR_INVALID_PARAM);
		ERR_OIPROXY("Cannot retrieve managed object interface (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	RegisteredMoInterfaceT moIfVersion;
	if (!getMoIfVersion(spiIf, moIfVersion))
	{
		ERR_OIPROXY("OIProxy_handleAdminOperation()::getMoIfVersion() Unable to get Registered interface version.");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get all attributes of the complex type attribute
	std::list<SaImmAttrValuesT_2 *> attrList;
	getImmClassAttributes((SaImmAttrValuesT_2 **)attr, true, attrList);

	//	Get MOC root element
	MafOamSpiMocT *mocRoot;
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject: calling getModelRepository()");
	getModelRepository()->getTreeRoot(&mocRoot);

	//	Get MOC element of the object
	MafOamSpiMocT *moc = findComMoc(mocRoot, mocPathList);
	if(!moc) {	//	this case should never happen
		setCcbApplierError(ccbId, SA_AIS_ERR_BAD_OPERATION);
		LEAVE_OIPROXY();
		return SA_AIS_ERR_BAD_OPERATION;
	}

	//	Find MOC element of the complex type attribute
	MafOamSpiMoAttributeT *structAttribute = moc->moAttribute;
	while(structAttribute) {
		if(!strcmp(structAttribute->generalProperties.name, ctAttr.c_str()))
			break;
		structAttribute = structAttribute->next;
	}

	//	Check if the MOC element is found
	if(!structAttribute) {
		setCcbApplierError(ccbId, SA_AIS_ERR_BAD_OPERATION);
		LEAVE_OIPROXY();
		return SA_AIS_ERR_BAD_OPERATION;
	}

	MafMoAttributeValueContainer_3T cnt;
	MafMoAttributeValue_3T tempAttrValue;
	cnt.nrOfValues = 1;
	cnt.type = MafOamSpiMoAttributeType_3_STRUCT;
	cnt.values = &tempAttrValue;

	//	Copy IMM attributes to MO attributes format
	MafMoAttributeValueStructMember_3 *structMember = NULL;
	copyImmAttrListToMoAttrStruct(structAttribute->structDatatype->members, attrList,  &structMember);

	cnt.values[0].value.structMember = structMember;

	//	Call setMoAttribute for the complex type attribute
	MafReturnT ret = OIProxy_cache_setMoAttribute(moIf, txHandle, ccbId, moDn.c_str(), ctAttr.c_str(), MafOamSpiMoAttributeType_3_STRUCT, &cnt, (moIfVersion == MafOamSpiManagedObject2));

	//	Free allocated memory
	freeMoAttrStructList(cnt.values[0].value.structMember);
	if(ret != MafOk) {
		setCcbApplierError(ccbId, SA_AIS_ERR_BAD_OPERATION);
		LEAVE_OIPROXY();
		return SA_AIS_ERR_BAD_OPERATION;
	}
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbCreateObject LEAVE with SA_AIS_OK");
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/**
 * Delete object callback for complex type attribute.
 * Not used for complex type attributes, but needs to be there because of initialization of the multiply applier
 * This function is used by the multiply applier.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	Ccb identified
 * @param	Pointer to the object name
 * @return	SA_AIS_OK
 */
SaAisErrorT OIProxy_handleComplexTypeCcbDeleteObject(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId, const SaNameT *objectName) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbDeleteObject() entered");
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/**
 * Simple type attribute modification callback for complex type attribute.
 * When some attribute is modified in a complex type attribute, this function will be called.
 * This function is used by the multiply applier.
 *
 * @param	immOiHandle		IMM OI handle
 * @param	Ccb identified
 * @param	Pointer to the object name
 * @return	SA_AIS_OK
 */
SaAisErrorT OIProxy_handleComplexTypeCcbModification(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
											const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbModification() entered");
	//	Check input parameters
	if(!objectName || !attrMods) {
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	MafOamSpiMocT *mocRoot;
	MafOamSpiMocT *moc = NULL;
	MafOamSpiMoAttributeT *moAttr;

	//	Save IMM dn to immDn
	std::string immDn(saNameGet(objectName), saNameLen(objectName));

	//	Parse immDn, and get MO dn, complex type attribute and index of complex type attribute
	//	If the object is not complex type attribute, ctAttr will be empty and index will be -1
	std::string moDn;
	std::string ctAttr;
	int index;
	if(!parseImmDN(immDn.c_str(), moDn, ctAttr, &index)) {
		DEBUG_OIPROXY("Error in parsing IMM DN");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	Check if the IMM object is a MO instance or a complex type attribute
	if(index == -1) {
		DEBUG_OIPROXY("Not interested in non-complex type attributes");
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	create MOC path from MO dn
	std::string mocPath;
	createMocPath(moDn.c_str(), mocPath);

	//	Split MOC path into fragments
	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	//	Get MOC root element
	RegisteredCommonStructT *spiIf;
	DEBUG_OIPROXY("OIProxy_handleComplexTypeCcbModification: calling getModelRepository()");
	getModelRepository()->getTreeRoot(&mocRoot);

	//	Get MOC class element of the MO instance
	moc = findComMoc(mocRoot, mocPathList);
	if(!moc) {	//	this case should never happen
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	Get the complex type MO attribute from the MOC element of MO instance
	moAttr = getMocAttribute(moc, ctAttr.c_str());
	if(!moAttr) {
		DEBUG_OIPROXY("Cannot find MO attribute (%s) in the class", ctAttr.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_OK;
	}

	//	check if the complex type attribute is registered
	std::string ctPath = moDn + ":" + ctAttr;
	if(!existRegisteredCTClassAttr(moAttr->structDatatype->generalProperties.name, ctPath.c_str())) {
		ctPath = mocPath + ":" + ctAttr;
		if(!existRegisteredCTClassAttr(moAttr->structDatatype->generalProperties.name, ctPath.c_str())) {
			DEBUG_OIPROXY("%s is not registered", immDn.c_str());
			LEAVE_OIPROXY();
			return SA_AIS_OK;
		} else
			spiIf = (RegisteredCommonStructT *)getRegisteredCTClass(moAttr->structDatatype->generalProperties.name, ctPath.c_str());
	} else
		spiIf = (RegisteredCommonStructT *)getRegisteredCTClass(moAttr->structDatatype->generalProperties.name, ctPath.c_str());

	//	Double check. This case should never happen
	if(!spiIf) {
		setCcbApplierError(ccbId, SA_AIS_ERR_INVALID_PARAM);
		ERR_OIPROXY("Error while creating %s", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get transaction handle
	MafOamSpiTransactionHandleT txHandle;
	if(!openTransaction(ccbId, spiIf, &txHandle)) {
		setCcbApplierError(ccbId, SA_AIS_ERR_INVALID_PARAM);
		ERR_OIPROXY("Cannot open transaction (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get managed object interface
	MafOamSpiManagedObject_3T *moIf;
	if(!getMoIf(spiIf, &moIf)) {
		setCcbApplierError(ccbId, SA_AIS_ERR_INVALID_PARAM);
		ERR_OIPROXY("Cannot retrieve managed object interface (%s)", immDn.c_str());
		LEAVE_OIPROXY();
		return SA_AIS_ERR_INVALID_PARAM;
	}

	RegisteredMoInterfaceT moIfVersion;
	if (!getMoIfVersion(spiIf, moIfVersion))
	{
		ERR_OIPROXY("OIProxy_handleCcbModification()::getMoIfVersion() Unable to get Registered interface version.");
		return SA_AIS_ERR_INVALID_PARAM;
	}

	//	Get list of all modified attributes
	MafMoAttributeValueContainer_3T *attrValue = getModifiedStructAttributes(attrMods, moAttr);
	if(!attrValue) {
		setCcbApplierError(ccbId, SA_AIS_ERR_FAILED_OPERATION);
		ERR_OIPROXY("Cannot create modified structure");
		LEAVE_OIPROXY();
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	//	Set all modified attributes using setMoAttribute
	MafReturnT com_rc = OIProxy_cache_setMoAttribute(moIf, txHandle, ccbId, moDn.c_str(), ctAttr.c_str(), MafOamSpiMoAttributeType_3_STRUCT, attrValue, (moIfVersion == MafOamSpiManagedObject2));

	//	Free allocated memory by getModifiedStructAttributes
	releaseModifiedStructAttributes(attrValue);

	//	Error messages
	if(com_rc != MafOk)
	{
		ERR_OIPROXY("OIProxy_handleCcbModification(): COM OI Failed to setMoAttribute(), rc = %d",com_rc);
		// get error string messages
		com_rc=GetSet_ErrorMessages(ccbId);
		if(com_rc != MafOk) {
			ERR_OIPROXY("OIProxy_handleCcbModification(): COM OI Failed to GetSet_ErrorMessages(), rc = %d",com_rc);
		}

		setCcbApplierError(ccbId, SA_AIS_ERR_FAILED_OPERATION);

		LEAVE_OIPROXY();
		return SA_AIS_ERR_FAILED_OPERATION;
	}
	LEAVE_OIPROXY();
	return SA_AIS_OK;
}

/******************************************************************************
 *
 *
 *
 *
 ******************************************************************************/

//	Callback functions for the main implementer
SaImmOiCallbacksT_2 callbackFunctions = {
	OIProxy_handleAdminOperation,
	OIProxy_handleCcbAbort,
	OIProxy_handleCcbApply,
	OIProxy_handleCcbCompleted,
	OIProxy_handleCcbCreateObject,
	OIProxy_handleCcbDeleteObject,
	OIProxy_handleCcbModification,
	OIProxy_handleRtAttrUpdate
};

//	Callback functions for the "second OI" for runtime complex types
SaImmOiCallbacksT_2 RTCTcallbackFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	OIProxy_RuntimeStructAttrUpdate
};

//	Callback functions for the multiply apllier (complex type attributes)
SaImmOiCallbacksT_2 applierCallbackFunctions = {
	NULL,
	OIProxy_handleComplexTypeCcbAbort,
	OIProxy_handleComplexTypeCcbApply,
	OIProxy_handleComplexTypeCcbCompleted,
	OIProxy_handleComplexTypeCcbCreateObject,
	OIProxy_handleComplexTypeCcbDeleteObject,
	OIProxy_handleComplexTypeCcbModification,
	NULL
};

/**
 * This method Initialize the InterfacePortal
 *
 * @param	_portal		SPI interface portal
 * @return	always return MafOk
 */
MafReturnT oamSAOiProxyInitialization(
		MafMgmtSpiInterfacePortal_3T* _portal) {
	ENTER_OIPROXY();
	InterfacePortal = _portal;
	LEAVE_OIPROXY();
	return MafOk;
}

/**
 * This function waits for events on both the main implementer and the multiply applier,
 * and invoke their pending callbacks.
 */
void dispatch_to_imm()
{
	ENTER_OIPROXY();
	struct pollfd fds[2];
	fds[0].fd = immSelectionObject; //poll the imm selection object file descriptor that refers to /dev/poll driver
	fds[0].events = POLLIN;
	fds[1].fd = immApplierSelectionObject; //poll the imm selection object file descriptor that refers to /dev/poll driver
	fds[1].events = POLLIN;
	while (!exit_OI_dispatch_thread)
	{
		int res = poll(fds, 2, 20); // 15 msec timeout value. Returns 0 if timed out
		if (res == -1)
		{
			if (errno == EINTR)
				continue;
			else {
				ERR_OIPROXY("dispatch_to_imm(): poll FAILED - %s", strerror(errno));
				pthread_exit(NULL);
			}
		}

		if ( exit_OI_dispatch_thread )
		{
			DEBUG_OIPROXY("Stopping dispatching to IMM\n");
			LEAVE_OIPROXY();
			pthread_exit(NULL);
		}


		if ( fds[0].revents & POLLIN ) //a POLLIN event occurred (i.e. there is data to be read)
		{
			/* There is an IMM OI call back waiting to be be processed. Process it */
			DEBUG_OIPROXY("dispatch_to_imm(): A dispatch from IMM arrived.");
			SaAisErrorT rc = saImmOiDispatch(immOiHandle, SA_DISPATCH_ONE);
			if (SA_AIS_OK != rc)
			{
				LOG_OIPROXY("dispatch_to_imm(): saImmOiDispatch FAILED %s", getOpenSAFErrorString(rc));
				if (SA_AIS_ERR_BAD_HANDLE == rc){
                                        LOG_OIPROXY("dispatch_to_imm():reinitializing OI handles ");
                                        ObjImp_finalize_imm(false,false);
                                        ObjImp_init_imm(false);
                                        SaAisErrorT rc = saImmOiDispatch(immOiHandle, SA_DISPATCH_ONE);
                                        if (SA_AIS_OK != rc){
                                                LOG_OIPROXY("dispatch_to_imm()1: saImmOiDispatch FAILED %s", getOpenSAFErrorString(rc));
                                                pthread_exit(NULL);
					}
				}
                                else{
                                        pthread_exit(NULL);
				}
			}
		}

		if ( exit_OI_dispatch_thread )
		{
			DEBUG_OIPROXY("Stopping dispatching to IMM\n");
			LEAVE_OIPROXY();
			pthread_exit(NULL);
		}


		if( fds[1].revents & POLLIN )
		{
			DEBUG_OIPROXY("dispatch_to_imm(): A dispatch for multiply applier from IMM arrived.");
			SaAisErrorT rc = saImmOiDispatch(immOiApplierHandle, SA_DISPATCH_ONE);
			if (SA_AIS_OK != rc)
			{
				LOG_OIPROXY("dispatch_to_imm(): saImmOiDispatch for multiply applier FAILED %s", getOpenSAFErrorString(rc));
				if (SA_AIS_ERR_BAD_HANDLE == rc){
                                        LOG_OIPROXY("dispatch_to_imm():reinitializing OI handles ");
                                        ObjImp_finalize_imm(false,false);
                                        ObjImp_init_imm(false);
                                        SaAisErrorT rc = saImmOiDispatch(immOiApplierHandle, SA_DISPATCH_ONE);
                                        if (SA_AIS_OK != rc){
                                                LOG_OIPROXY("dispatch_to_imm()1: saImmOiDispatch for multiply applier FAILED %s", getOpenSAFErrorString(rc));
                                                pthread_exit(NULL);
					}
				}
                                else{
                                         pthread_exit(NULL);
				}
			}
		}
	}

	DEBUG_OIPROXY("Stopping dispatching to IMM\n");
	LEAVE_OIPROXY();
	pthread_exit(NULL);
}

/**
 * The main thread function for the IMM OI dispatch function.
 *
 * @param	dispatch_threadid		Dispatch thread identifier (not used)
 */
void *polling_dispatch_thread(void *dispatch_threadid)
{
	ENTER_OIPROXY();
	dispatch_to_imm();
	LEAVE_OIPROXY();
	return NULL;
}

/**
 * Object implementer initialization function
 */
MafReturnT ObjImp_init_imm(bool loadImmOm)
{
	ENTER_OIPROXY();
	SaAisErrorT error = SA_AIS_OK;
	MafReturnT com_rc = MafOk;
	DEBUG_OIPROXY("ObjImp_init_imm(): Initializing IMM");
	if (immOiHandle == 0)
	{
		// Init OI
		DEBUG_OIPROXY("ObjImp_init_imm(): Init OI");
		if ((error = autoRetry_saImmOiInitialize_2(&immOiHandle, &callbackFunctions, &imm_version)) != SA_AIS_OK)
		{
			ERR_OIPROXY("ObjImp_init_imm(): saImmOiInitialize FAILED %s", getOpenSAFErrorString(error));
			com_rc = MafFailure;
		}
		else
		{
			// Init Applier
			DEBUG_OIPROXY("ObjImp_init_imm(): Init Applier");
			if ((error = autoRetry_saImmOiInitialize_2(&immOiApplierHandle, &applierCallbackFunctions, &imm_version)) != SA_AIS_OK)
			{
				ObjImp_finalize_imm(true,true);
				ERR_OIPROXY("ObjImp_init_imm(): saImmOiInitialize FAILED %s", getOpenSAFErrorString(error));
				com_rc = MafFailure;
				LEAVE_OIPROXY();
				return com_rc;
			}
			DEBUG_OIPROXY("autoRetry_saImmOiInitialize_2 : SUCCESS");

			com_rc = InterfacePortal->getInterface(MafOamSpiTransactionMaster_2Id,(MafMgmtSpiInterface_1T**)&txMaster);
			if( com_rc !=MafOk)
			{
				ObjImp_finalize_imm(true,true);
				ERR_OIPROXY("ObjImp_init_imm(): Failed to get MafOamSpiTransactionMaster_2 from the portal");
				LEAVE_OIPROXY();
				return com_rc;
			}

			if(( error = autoRetry_saImmOiSelectionObjectGet(immOiHandle, &immSelectionObject)) != SA_AIS_OK )
			{
				ERR_OIPROXY("ObjImp_init_imm(): saImmOiSelectionObjectGet FAILED %s", getOpenSAFErrorString(error));
				DEBUG_OIPROXY("ObjImp_init_imm(): calling ObjImp_finalize_imm() because of saImmOiSelectionObjectGet() failure");
				ObjImp_finalize_imm(true,true);
				LEAVE_OIPROXY();
				com_rc = MafFailure;
			}
			else
			{
				if(( error = autoRetry_saImmOiSelectionObjectGet(immOiApplierHandle, &immApplierSelectionObject)) != SA_AIS_OK )
				{
					ERR_OIPROXY("ObjImp_init_imm(): saImmOiSelectionObjectGet(multiply applier) FAILED %s", getOpenSAFErrorString(error));
					DEBUG_OIPROXY("ObjImp_init_imm(): calling ObjImp_finalize_imm() for multiply applier because of saImmOiSelectionObjectGet() failure");
					ObjImp_finalize_imm(true,true);
					com_rc = MafFailure;
					LEAVE_OIPROXY();
					return com_rc;
				}
				DEBUG_OIPROXY("autoRetry_saImmOiSelectionObjectGet : SUCCESS");

				std::string oiNameStr = (const char*)(&_CC_OI_NAME_PREFIX);;
				oiNameStr += "SAOiProxy";
				const char* oiName= oiNameStr.c_str();
				DEBUG_OIPROXY("autoRetry_saImmOiImplementerSet OI Name: %s", oiName);
				if ((error = autoRetry_saImmOiImplementerSet(immOiHandle, (char*)oiName)) != SA_AIS_OK)
				{
					if((error == SA_AIS_ERR_NO_RESOURCES) && (nRetries++<3)) {
						LOG_OIPROXY("autoRetry_saImmOiImplementerSet receives SA_AIS_ERR_NO_RESOURCES, hence re-initializing handlers at attempt (%d)", nRetries);
						ObjImp_finalize_imm(true,true);
						unsetFromEnvironment("IMMA_OI_CALLBACK_TIMEOUT");
						ObjImp_init_imm(true);
					}
					else
					{
						ERR_OIPROXY("ObjImp_init_imm(): saImmOiImplementerSet FAILED: %s", getOpenSAFErrorString(error));
						DEBUG_OIPROXY("ObjImp_init_imm(): calling ObjImp_finalize_imm() because of saImmOiImplementerSet() failure");
						ObjImp_finalize_imm(true,true);
						com_rc = MafFailure;
					}
				}
				else
				{
					nRetries=0;
					std::string oiApplierNameStr("@");
					oiApplierNameStr += (const char*)(&_CC_OI_NAME_PREFIX);
					oiApplierNameStr += "SAOiApplierProxy";
					const char *oiApplierName = oiApplierNameStr.c_str();
					DEBUG_OIPROXY("autoRetry_saImmOiImplementerSet OI Applier Name: %s", oiApplierName);
					if ((error = autoRetry_saImmOiImplementerSet(immOiApplierHandle, (char*)oiApplierName)) != SA_AIS_OK)
					{
						if((error == SA_AIS_ERR_NO_RESOURCES) && (nRetries++<3)) {
							LOG_OIPROXY("autoRetry_saImmOiImplementerSet(multiply applier) receives SA_AIS_ERR_NO_RESOURCES, hence re-initializing handlers at attempt (%d)", nRetries);
							ObjImp_finalize_imm(true,true);
							unsetFromEnvironment("IMMA_OI_CALLBACK_TIMEOUT");
							ObjImp_init_imm(true);
						}
						else
						{
							ERR_OIPROXY("ObjImp_init_imm(): saImmOiImplementerSet(multiply applier) FAILED: %s", getOpenSAFErrorString(error));
							DEBUG_OIPROXY("ObjImp_init_imm(): calling ObjImp_finalize_imm() because of saImmOiImplementerSet() failure");
							ObjImp_finalize_imm(true,true);
							com_rc = MafFailure;
							LEAVE_OIPROXY();
							return com_rc;
						}
					}
					else
					{
						DEBUG_OIPROXY("autoRetry_saImmOiImplementerSet : SUCCESS");
						nRetries=0;
					}
					if(loadImmOm){
						if((error = autoRetry_saImmOmInitialize(&immOmHandle, NULL, &imm_version)) != SA_AIS_OK)
						{
							ERR_OIPROXY("ObjImp_init_imm(): saImmOmInitialize failed %s", getOpenSAFErrorString(error));
							DEBUG_OIPROXY("ObjImp_init_imm(): calling ObjImp_finalize_imm() because of saImmOmInitialize() failure");
							ObjImp_finalize_imm(true,true);
							com_rc = MafFailure;
						}
						else
						{
							DEBUG_OIPROXY("autoRetry_saImmOmInitialize : SUCCESS, handle(%llu)",immOmHandle);
							if((autoRetry_saImmOmAccessorInitialize(immOmHandle, &accessorHandleOI)) != SA_AIS_OK)
							{
								ERR_OIPROXY("ObjImp_init_imm(): saImmOmAccessorInitialize failed %s", getOpenSAFErrorString(error));
								DEBUG_OIPROXY("ObjImp_init_imm(): calling ObjImp_finalize_imm() because of saImmOmAccessorInitialize() failure");
								ObjImp_finalize_imm(true,true);
								com_rc = MafFailure;
							}
							else
							{
								exit_OI_dispatch_thread=false;
								pthread_attr_t attr;
								if (pthread_attr_init(&attr) != 0)
								{
									ERR_OIPROXY("ObjImp_init_imm():pthread_attr_init FAILED - %s", strerror(errno));
									com_rc = MafFailure;
								}
								int rc = pthread_create(&dispatch_thread, &attr, polling_dispatch_thread, NULL);
								(void)pthread_attr_destroy(&attr);
								if (rc != 0)
								{
									ERR_OIPROXY("ObjImp_init_imm(): pthread_create FAILED - %s", strerror(errno));
									com_rc = MafFailure; //without the thread waiting for the callbacks, it is useless to be an object implementer
								}
							}
						}
						saNameInit();
					}
					DEBUG_OIPROXY("ObjImp_init_imm(): Successfully initialized IMM\n");
				}
			}
		}
	}
	LEAVE_OIPROXY();
	return com_rc;
}

/**
 * Object implementer finalize function
 */
//TODO:Thread handling should be improved
void ObjImp_finalize_imm(bool unLoadImmOm, bool cancelDispatchThread)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("ObjImp_finalize_imm(): ENTER");
	SaAisErrorT error = SA_AIS_OK;
	if(unLoadImmOm)
	{
		exit_OI_dispatch_thread = true;
	}
	DEBUG_OIPROXY("ObjImp_finalize_imm(): Finalizing the IMM Object Implementer handle..");
	if (dispatch_thread && cancelDispatchThread)
	{
		// Stop the execution of the dispatch thread
		DEBUG_OIPROXY("ObjImp_finalize_imm(): Stop the execution of the dispatch thread");
		pthread_cancel(dispatch_thread);
	}
	if(immOiHandle != 0)
	{
		if ((error = autoRetry_saImmOiFinalize(immOiHandle))!= SA_AIS_OK)
		{
			ERR_OIPROXY("ObjImp_finalize_imm(): saImmOiFinalize FAILED: %s immOiHandle(%llu)", getOpenSAFErrorString(error),immOiHandle);
		}
		else
		{
			DEBUG_OIPROXY("ObjImp_finalize_imm(): Successfully finalized the immOiHandle(%llu)",immOiHandle);
			immOiHandle = 0;
		}
	}
	else
	{
		DEBUG_OIPROXY("ObjImp_finalize_imm(): immOiHandle is null, not calling saImmOiFinalize()");
	}
	if(immOiApplierHandle != 0)
	{
		if ((error = autoRetry_saImmOiFinalize(immOiApplierHandle))!= SA_AIS_OK)
		{
			ERR_OIPROXY("ObjImp_finalize_imm(): saImmOiFinalize(multiply applier) FAILED: %s immOiApplierHandle(%llu)", getOpenSAFErrorString(error),immOiApplierHandle);
		}
		else
		{
			DEBUG_OIPROXY("ObjImp_finalize_imm(): Successfully finalized the immOiApplierHandle(%llu)",immOiApplierHandle);
			immOiApplierHandle = 0;
		}
	}
	else
	{
		DEBUG_OIPROXY("ObjImp_finalize_imm(): immOiHandle is null, not calling saImmOiFinalize()");
	}
	if(unLoadImmOm){
		if (accessorHandleOI != 0)
		{
			error=autoRetry_saImmOmAccessorFinalize(accessorHandleOI);
			if (error != SA_AIS_OK)
			{
				ERR_OIPROXY("ObjImp_finalize_imm(): saImmOmAccessorFinalize FAILED: %s accessorHandleOI(%llu)", getOpenSAFErrorString(error),accessorHandleOI);
			}
			else
			{
				DEBUG_OIPROXY("ObjImp_finalize_imm(): saImmOmAccessorFinalize Successfully finalized (%llu)",accessorHandleOI);
			}
			accessorHandleOI=0;
		}

		if(immOmHandle != 0)
		{
			DEBUG_OIPROXY("ObjImp_finalize_imm(): try saImmOmFinalize(%llu)", immOmHandle);
			error=autoRetry_saImmOmFinalize(immOmHandle);
			if (error != SA_AIS_OK)
			{
				ERR_OIPROXY("ObjImp_finalize_imm(): saImmOmFinalize(%llu) FAILED: %s", immOmHandle, getOpenSAFErrorString(error));
			}
			else
			{
				DEBUG_OIPROXY("ObjImp_finalize_imm(): saImmOmFinalize(%llu), Successfully finalized the immOmHandle", immOmHandle);
			}
			immOmHandle = 0;
		}
	}
	DEBUG_OIPROXY("ObjImp_finalize_imm(): RETURN");
	LEAVE_OIPROXY();
}

/**
 * Get and set error messages for a ccbId
 *
 * @param	ccbId	Ccb identifier
 */
MafReturnT GetSet_ErrorMessages(SaImmOiCcbIdT ccbId)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("GetSet_ErrorMessages()\n");
	// get errorMessages from com
	const char** messages;

	// Same solution as in 1043.
	using namespace CM;
	// Initialize threadContext once.
	if (threadContext == NULL)
	{
		if(ImmCmd::mVersion.releaseCode >= immReleaseCode && ImmCmd::mVersion.majorVersion >= immMajorVersion
				&& ImmCmd::mVersion.minorVersion >= immMinorVersion)
		{
			DEBUG_OIPROXY("GetSet_ErrorMessages: IMM version %c.%u.%u encountered",
						ImmCmd::mVersion.releaseCode, ImmCmd::mVersion.majorVersion, ImmCmd::mVersion.minorVersion);
			MafReturnT com_rc = InterfacePortal->getInterface(
					theThreadContext, (MafMgmtSpiInterface_1T**)&threadContext);
			if( com_rc !=MafOk)
			{
				ERR_OIPROXY("Failed to get MafMgmtSpiThreadContext_2");
				LEAVE_OIPROXY();
				return com_rc;
			}
			else DEBUG_OIPROXY("_portal->getInterface() return OK");
		}
		else
		{
			// WE do not have support for this in the coreMW IMM, just return MafOk
			LEAVE_OIPROXY();
			return MafOk;
		}
	}
	// Try to read the two available categories.
	// ThreadContextMsgLog_2 and ThreadContextMsgNbi_2
	ThreadContextMsgCategory_2T category=ThreadContextMsgLog_2;
	ThreadContextHandle_2T msgHandle;
	bool continueLoop = true;

	while (continueLoop)
	{
		// First get a message Iterator Handle
		MafReturnT rc = threadContext->newMessageIterator(category, &msgHandle);
		if (rc == MafOk)
		{
			rc = threadContext->getMessages(category, &msgHandle, &messages);
		}
		else
		{
			threadContext->clearMessages(category);
			threadContext->releaseHandle(msgHandle);
			ERR_OIPROXY("Error occurred during newMessageIterator()");
			LEAVE_OIPROXY();
			return MafFailure;
		}
		// Forward what we found to IMM
		if (rc == MafOk)
		{
			// send errorMessages to Imm
			for(int i=0;i<ImmAdmOpNoReturnParams && messages[i]!=(char *)'\0';i++)
			{
				const SaStringT errorString=(SaStringT)messages[i];
				DEBUG_OIPROXY("Error string number %d: %s",i,errorString);
				SaAisErrorT RC = saImmOiCcbSetErrorString(immOiHandle, ccbId, errorString);
				if(RC != SA_AIS_OK)
				{
					ERR_OIPROXY("Error occurred during saImmOiCcbSetErrorString");
					threadContext->clearMessages(category);
					threadContext->releaseHandle(msgHandle);
					LEAVE_OIPROXY();
					return MafFailure;
				}
			}
			threadContext->clearMessages(category);
			threadContext->releaseHandle(msgHandle);
		}
		if (category==ThreadContextMsgLog_2)
		{
			category = ThreadContextMsgNbi_2;
		}
		else
		{
			continueLoop = false;
		}
	} // End 	while (continueLoop)
	DEBUG_OIPROXY("GetSet_ErrorMessages: All error strings were forwarded to Imm");
	LEAVE_OIPROXY();
	return MafOk;
}


/**
 * Get the error messages from a Thread Context
 *
 * @param	messages (out) a pointer to the result
 *
 */
MafReturnT GetErrMsgsFromThContext(const char*** messages, ThreadContextHandle_2T* msgHandle)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("GetErrMsgsFromTxContext()\n");

	// Same solution as in 1043.
	using namespace CM;
	// Initialize threadContext once.
	if (threadContext == NULL)
	{
		if(ImmCmd::mVersion.releaseCode >= immReleaseCode && ImmCmd::mVersion.majorVersion >= immMajorVersion
				&& ImmCmd::mVersion.minorVersion >= immMinorVersion)
		{
			DEBUG_OIPROXY("GetErrMsgsFromThContext: IMM version %c.%u.%u encountered",
						ImmCmd::mVersion.releaseCode, ImmCmd::mVersion.majorVersion, ImmCmd::mVersion.minorVersion);
			MafReturnT com_rc = InterfacePortal->getInterface(
					theThreadContext, (MafMgmtSpiInterface_1T**)&threadContext);
			if( com_rc !=MafOk)
			{
				ERR_OIPROXY("GetErrMsgsFromThContext: Failed to get MafMgmtSpiThreadContext_2");
				LEAVE_OIPROXY();
				return com_rc;
			}
			else DEBUG_OIPROXY("GetErrMsgsFromThcontext: _portal->getInterface() return OK");
		}
		else
		{
			// WE do not have support for this in the coreMW IMM, just return MafOk
			LEAVE_OIPROXY();
			return MafOk;
		}
	}
	// Try to read the two available categories.
	// ThreadContextMsgLog_2 and ThreadContextMsgNbi_2
	ThreadContextMsgCategory_2T category = ThreadContextMsgLog_2;
	bool continueLoop = true;

	while (continueLoop)
	{
		// First get a message Iterator Handle
		MafReturnT rc = threadContext->newMessageIterator(category, msgHandle);
		if (rc == MafOk)
		{
			rc = threadContext->getMessages(category, msgHandle, messages);
		}
		else
		{
			threadContext->clearMessages(category);
			threadContext->releaseHandle(*msgHandle);
			ERR_OIPROXY("GetErrMsgsFromThContext: Error occurred during newMessageIterator()");
			LEAVE_OIPROXY();
			return MafFailure;
		}
		if (category == ThreadContextMsgLog_2)
		{
			category = ThreadContextMsgNbi_2;
		}
		else
		{
			continueLoop = false;
		}
	} // End 	while (continueLoop)
	DEBUG_OIPROXY("GetErrMsgsFromThContext: All error strings were retrieved from the Thread Context");
	LEAVE_OIPROXY();
	return MafOk;
}


/**
 * Prepare error messages for admin operation result
 *
 * @param	messages (in) a pointer to array of error strings
 * @param	returnParams (out) a pointer to a SaImmAdminOperationParamsT_2 to use for the result
 *
 */
void PrepareErrMsgsForAdmOpResult(char** messages, SaImmAdminOperationParamsT_2** returnParams)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("PrepareErrMsgsForAdmOpResult()\n");
	int i;
	if (messages != NULL)
	{
		for(i = 0; (i < ImmAdmOpNoReturnParams) && (messages[i] != (char*)'\0'); i++)
		{
			DEBUG_OIPROXY("PrepareErrMsgsForAdmOpResult: Error string number %d: %s", i, messages[i]);
			if (i >= 1) {
				returnParams[i] = new SaImmAdminOperationParamsT_2;
				returnParams[i]->paramName = returnParams[i-1]->paramName;
				returnParams[i]->paramType = SA_IMM_ATTR_SASTRINGT;
			}
			returnParams[i]->paramBuffer = &messages[i];
		}

		if (i == 0) {
			delete returnParams[i];
		}

		returnParams[i] = NULL; // null terminator at the end
	}
	LEAVE_OIPROXY();
}


/******************************************************************************
 *
 *
 *
 *
 ******************************************************************************/
/**
 * This method Initialize the InterfacePortal
 *
 * @param	_portal		SPI interface portal
 * @return	always return MafOk
 */
MafReturnT maf_oamSAOiProxyInitialization(
		MafMgmtSpiInterfacePortal_3T* _portal) {
	ENTER_OIPROXY();
	InterfacePortal = _portal;
	LEAVE_OIPROXY();
	return MafOk;
}
