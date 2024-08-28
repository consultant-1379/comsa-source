/**
 * Include files   Copyright (C) 2010 by Ericsson AB
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
 *   File:   OamSAManagedObjects.cc
 *
 *   Author: egorped & efaiami
 *
 *   Date:   2010-05-21
 *
 *   This file implements the need functions for Managed Objects.
 *
 *  Reviewed: efaiami 2010-06-17
 *
 *  Reviewed: efaiami 2011-01-25  code review for CM Action
 *
 *  Modify: efaiami 2011-02-23   for log and trace function
 *  Modify: xnikvap, xngangu, xjonbuc, xduncao 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *  Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *  Modify: uabjoy  2014-03-14  Adding support for Trace CC.
 *  Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 */

#include <ComSA.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <poll.h>
#include "saImm.h"
#include "saImmOm.h"
#include "OamSAManagedObjects.h"
#include "OamSAImmBridge.h"
#include "OamSATransactionRepository.h"
#include <MafOamSpiServiceIdentities_1.h>
#include "trace.h"

/**
 * Forward declaration of functions
 */

static MafReturnT setMoAttribute(MafOamSpiTransactionHandleT txHandle,
								 const char * dn,
								 const char * attributeName,
								 const MafMoAttributeValueContainer_3T * attributeValue);

static MafReturnT getMoAttribute(MafOamSpiTransactionHandleT txHandle,
								 const char * dn,
								 const char * attributeName,
								 MafMoAttributeValueResult_3T * result);

static MafReturnT getMoAttributes(MafOamSpiTransactionHandleT txHandle,
								  const char * dn,
								  const char ** attributeNames,
								  MafMoAttributeValuesResult_3T * result);

static MafReturnT newMoIterator(MafOamSpiTransactionHandleT txHandle,
								const char * dn,
								const char * className,
								MafOamSpiMoIteratorHandle_3T * result);

static MafReturnT nextMo(MafOamSpiMoIteratorHandle_3T itHandle,
						 char **result);

static MafReturnT createMo(MafOamSpiTransactionHandleT txHandle,
						   const char * parentDn,
						   const char * className,
						   const char * keyAttributeName,
						   const char * keyAttributeValue,
						   MafMoNamedAttributeValueContainer_3T ** initialAttributes);

static MafReturnT deleteMo(MafOamSpiTransactionHandleT txHandle,
						   const char * dn);

static MafReturnT action(MafOamSpiTransactionHandleT txHandle,
						 const char * dn,
						 const char * name,
						 MafMoNamedAttributeValueContainer_3T **parameters,
						 MafMoAttributeValueResult_3T * result);

static MafReturnT finalizeMoIterator(MafOamSpiMoIteratorHandle_3T itHandle);

static MafReturnT existsMo(MafOamSpiTransactionHandleT txHandle,
						   const char * dn,
						   bool * result);

static MafReturnT countMoChildren(MafOamSpiTransactionHandleT txHandle,
								  const char * dn,
								  const char * className,
								  uint64_t * result);

static void releaseAttributeValueContainerCallback(MafMoAttributeValueContainer_3T * container);

static void releaseAttributeValueContainersCallback(MafMoAttributeValueContainer_3T ** containers);


/******* TO DO!!!!! Get the real string values in here **************************************/

static MafOamSpiManagedObject_3 InterfaceStruct = {
	MafOamSpiCmRouterService_3Id,
	setMoAttribute,
	getMoAttribute,
	getMoAttributes,
	newMoIterator,
	nextMo,
	createMo,
	deleteMo,
	action,
	finalizeMoIterator,
	existsMo,
	countMoChildren};

/**
 *	Global i/f
 */
MafOamSpiManagedObject_3T* ExportOamManagedObjectInterface(void)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return &InterfaceStruct;
}

MafReturnT OamManagedObjectOpen(void)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}

MafReturnT OamManagedObjectClose(void)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOk;
}

 /**
  *  setMoAttribute
  */
static MafReturnT setMoAttribute (MafOamSpiTransactionHandleT txHandle,
								  const char * dn,
								  const char * attributeName,
								  const MafMoAttributeValueContainer_3T * attributeValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.setMoAttribute : Called with DN: %s", dn);
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->setImmMoAttribute(txHandle, dn, attributeName,
																				  const_cast<MafMoAttributeValueContainer_3T*>(attributeValue));
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  getMoAttribute
 */
static MafReturnT getMoAttribute(MafOamSpiTransactionHandleT txHandle, const char *dn,
                                  const char *attributeName, MafMoAttributeValueResult_3T *result)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.getMoAttribute : Called with DN: %s", dn);
	MafMoAttributeValueContainer_3T** conv_result = &(result->container);

	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->getImmMoAttribute(txHandle, dn, attributeName, conv_result);

	if (myRetVal == MafOk)
	{
	    result->release = &(releaseAttributeValueContainerCallback);
	}
	else
	{
	    result->release= NULL;
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}


/**
 *  getMoAttributes
 */
static MafReturnT getMoAttributes(MafOamSpiTransactionHandleT txHandle,
								  const char * dn,
								  const char ** attributeNames,
								  MafMoAttributeValuesResult_3T * result)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.getMoAttributes : Called with DN: %s", dn);
	const char **Attribute = attributeNames;
	MafMoAttributeValueContainer_3T** conv_result;

	// For HS96956 - COMSA will allocate memory for MafMoAttributeValueContainer_3T pointer
	int attributeCount = 0;
	while( *Attribute )
	{
		attributeCount++;
		Attribute++;
	}
	result->containers = new MafMoAttributeValueContainer_3T*[attributeCount + 1];
	// End HS96956

	conv_result = (result->containers);
	MafReturnT	myRetVal = MafFailure;

	result->release = &(releaseAttributeValueContainersCallback);

	Attribute = attributeNames;
	while( *Attribute )
	{
		myRetVal = OamSAImmBridge::getOamSAImmBridge()->getImmMoAttribute(txHandle, dn, *Attribute, conv_result);

		if (myRetVal != MafOk)
		{
			result->release= NULL;
			*conv_result = NULL;
			releaseAttributeValueContainersCallback(result->containers);
			result->containers = NULL;
			return myRetVal;
		}

		Attribute++;
		conv_result++;
	}
	//Set the last element in the array to NULL
	*conv_result = NULL;

	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}


/**
 *  newMoIterator
 */
static MafReturnT newMoIterator(MafOamSpiTransactionHandleT txHandle, const char *dn,
								const char *className, MafOamSpiMoIteratorHandle_3T *result)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.newMoIterator : Called with txhandle: %lu, DN: %s, classMame: %s ", txHandle, dn, className);
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->getImmMoIterator(txHandle, dn, className, result);
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.newMoIterator : Leaving with retVal  %d, itHandle %lu", myRetVal, (unsigned long)*result);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  nextMo
 */
static MafReturnT nextMo(MafOamSpiMoIteratorHandle_3T itHandle, char **result)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.nextMo : Called with itHandle: %u", (unsigned int) itHandle);
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->getImmNextMo(itHandle, result);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  createMo
 */
static MafReturnT createMo(MafOamSpiTransactionHandleT txHandle,
						   const char * parentDn,
						   const char * className,
						   const char * keyAttributeName,
						   const char * keyAttributeValue,
						   MafMoNamedAttributeValueContainer_3T ** initialAttributes)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.createMo : Called with parentDN: %s", parentDn);
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->createImmMo(txHandle, parentDn, className, keyAttributeName, keyAttributeValue, initialAttributes);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  deleteMo
 */
static MafReturnT deleteMo(MafOamSpiTransactionHandleT txHandle, const char *dn)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.deleteMo : Called with DN: %s", dn);
	MafReturnT	myRetVal = 	OamSAImmBridge::getOamSAImmBridge()->deleteImmMo(txHandle, dn);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  action
 */
static MafReturnT action(MafOamSpiTransactionHandleT txHandle,
						 const char * dn,
						 const char * name,
						 MafMoNamedAttributeValueContainer_3T **parameters,
						 MafMoAttributeValueResult_3T * result)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.action : Called with DN: %s", dn);
	// for the new COM SPI Ver. 3.0 need to convert from
	// MafMoNamedAttributeValueContainer_3T** to MafMoAttributeValueContainer_3T**
	MafMoAttributeValueContainer_3T **conv_parameters;
	MafMoNamedAttributeValueContainer_3T **pP = parameters;
 	int i = 0;

	while (*pP) {
		pP++;
		i++;
	}

	conv_parameters = new MafMoAttributeValueContainer_3T*[i+1];

	int j = 0;
	pP = parameters;

	for (j = 0; j < i; j++)
	{
		conv_parameters[j] = &((*pP)->value);
		pP++;
	}

	conv_parameters[j] = NULL;

	MafReturnT	myRetVal = 	OamSAImmBridge::getOamSAImmBridge()->action(txHandle, dn, name, conv_parameters, &(result->container));

	delete [] conv_parameters;

	result->release = &(releaseAttributeValueContainerCallback);

	if (MafOk != myRetVal)
	{
		result->container = NULL;
		result->release   = NULL;
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  finalizeMoIterator
 */
static MafReturnT finalizeMoIterator(MafOamSpiMoIteratorHandle_3T itHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.finalizeMoIterator : Called with itHandle: %u", (unsigned int) itHandle);
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->finalizeImmMoIterator(itHandle);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
	//return MafOk;
}

/**
 *  existsMo
 */
static MafReturnT existsMo(MafOamSpiTransactionHandleT txHandle,
						   const char * dn,
						   bool * result)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.existsMo : Called with DN: %s", dn);
	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->existsImmMo(txHandle, dn, result);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

/**
 *  countMoChildren
 */
static MafReturnT countMoChildren(MafOamSpiTransactionHandleT txHandle,
								  const char * dn,
								  const char * className,
								  uint64_t * result)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.countMoChildren : Called with DN: %s", dn);
	MafReturnT      myRetVal = OamSAImmBridge::getOamSAImmBridge()->countImmMoChildren(txHandle, dn, className, result);
	LEAVE_OAMSA_TRANSLATIONS();
	return myRetVal;
}

static void releaseAttributeValueContainerCallback(MafMoAttributeValueContainer_3T *container)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.releaseAttributeValueContainerCallback : Called with container: %p", container);
	OamSATransactionRepository::getOamSATransactionRepository()->ReleaseAttValContfromCache(container);
	LEAVE_OAMSA_TRANSLATIONS();
}

static void releaseAttributeValueContainersCallback(MafMoAttributeValueContainer_3T **containers)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("COM_MO_SPI.releaseAttributeValueContainersCallback : Called with containers: %p", containers);
	OamSATransactionRepository::getOamSATransactionRepository()->ReleaseMultipleAttValContfromCache(containers);
	LEAVE_OAMSA_TRANSLATIONS();
}
