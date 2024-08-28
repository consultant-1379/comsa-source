/**
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
 *	 File:	 OamSATranslator.cc
 *
 *	 Author: egorped
 *
 *	 Date:	 2010-05-21
 *
 *	 This file implementing translations between Imm and MO.
 *
 *	 Reviewed: efaiami 2010-07-05
 *	 Reviewed: efaiami 2011-01-26  Com_SA Action
 *	 Modify: efaiami 2011-02-23	 for log and trace function
 *			 ejonajo 2012-01-11	 improved //DEBUG printouts
 *	 Reviewed: eaparob 2011-09-19
 *	 Reviewed: efaiami 2012-03-26  sanityCheckAdminParameters()
 *
 *	 Modify: uabjoy  2012-04-10	 Added support for MOM value prefixing in the IMM class names.
 *	 Modify: uabjoy  2012-05-04	 Correction for TR HP81651 and a fault found in function test in the populate method.
 *	 Modify: xnikvap, xngangu, xjonbuc, xduncao 2012-08-30	support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *	 Modify: xjonbuc 2012-09-06	 Implement SDP1694 - support MAF SPI
 *	 Modify: uabjoy  2012-09-04	 Correction for TR HQ23564.
 *	 Modify: uabjoy  2012-11-09	 Correcting imm to 3GPP	translation so free key attribute format is allowed.
 *	 Modify: uabjoy  2012-11-09	 Correcting 3GPP to imm	translation so free key attribute format is allowed.
 *	 Modify: eaparob 2013-01-28	 Added functionality to get isNotifiable data
 *	 Modify: eaparob 2013-01-31	 Implemented "isNotified" functionality under OamSATranslator class
 *	 Modify: xduncao 2013-05-16	 Implemented support for any return type from action()
 *	 Modify: xadaleg 2013-07-16	 MR26712 - Support option to not split IMM DN at EcimContribution
 *	 Modify: eaparob 2013-10-22	 "GetClassNameFromImm()" function modified to be able to use as a common function (e.g. from MO Cache)
 *	 Modify: xdonngu 2013-11-01	 Fix HR82371 - Prevent segment fault caused by invalid handle.
 *	 Modify: xanhdao 2013-10-15	 MR 24146  -	 Support Floating point
 *	 Modify: xjonbuc 2013-12-05	 Fix HR94824 - Configured struct member can't be shown in COM
 *	 Modify: xdonngu 2013-12-12	 run cppcheck 1.62 and fix errors and warnings
 *	 Modify: xngangu 2014-02-19	 Fix HS30458 conflict immhandle/sub_immhandle between alarm notify and register OI class thread while node lock/unlock
 *	 Modify: xjonbuc 2014-03-01	 Implement MR29333 No root classes (including use of MAF MR SPI V3)
 *	 Modify: xnikvap 2014-03-06	 classes classMOMRootDataEntry and MOMRootRepository taken out in separate file
 *	 Modify: xdonngu 2014-05-29  using std::tr1::shared_ptr for TxContext
 *	 Modify: xadaleg 2014-08-02  MR35347 - increase DN length
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <map>
#include <cctype>
#include <queue>
#include "OamSATranslator.h"
#include "MafOamSpiModelRepository_4.h"
#include <tr1/memory>

using namespace std::tr1;
// Repository for the existing key attributes in the model repository
// Only visible inside OamSATranslator
OamSAKeyAttributeRepository theKeyHolder;

MOMRootRepository theMOMRepository;
const std::string containmentSeparator = CONTAINMENT_SEPARATOR;

extern MafOamSpiModelRepository_1T* theModelRepo_v1_p;
extern MafOamSpiModelRepository_4T* theModelRepo_v4_p;
extern bool non_root_mo_notifications;
extern "C" void createSecretAttrFile() {
	theMOMRepository.writeSecretsToFile();
}

/**
 *	Default constructor
 */
OamSATranslator::OamSATranslator()
{
	ENTER_OAMSA_TRANSLATIONS();
	immOmHandleLocal = 0;
	OamSAaccessorHandleT = 0;
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *	Default destructor
 */
OamSATranslator::~OamSATranslator()
{
	ENTER_OAMSA_TRANSLATIONS();
	finalizeImmHandle();
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *	Member functions
 **/

#ifdef UNIT_TEST
/**
 *	ResetMOMRoot -
 *	Unit test support method.
 **/
void
OamSATranslator::ResetMOMRoot()
{
	ENTER_OAMSA_TRANSLATIONS();
	// Clear the internal data structure.
	theMOMRepository.ResetRootRepository();
	theKeyHolder.ResetRepo();
	LEAVE_OAMSA_TRANSLATIONS();
}
#endif

/**
 *	IsClassNamePresent -
 *	Return True if ClassName is present in the MOM Repository.
 *	Return False if ClassName is not present in the MOM Repository.
 **/
bool
OamSATranslator::IsClassNamePresent(const std::string& ClassName)
{
	ENTER_OAMSA_TRANSLATIONS();
	MOMRootRepository::MOMRootDataEntryMapRetVal theMOMRetVal;
	// Here we only want to check that the class exist or not in MOM.
	// WE do not need to know if it is a rootclass.
	theMOMRetVal = theMOMRepository.Find(ClassName);
	LEAVE_OAMSA_TRANSLATIONS();
	return theMOMRetVal.second;
}

/**
 *	isImmKeyAttribute -
 *	return the key attribute name for the class. This can be prefixed or not.
 **/
bool OamSATranslator::isImmKeyAttribute(const char *parentClass,
										const char *className,
										const char *AttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string AttributeNameString=AttributeName;
	std::string parentClassString = parentClass;
	std::string classNameString = className;
	std::string searchKey = parentClassString + "_" + classNameString;
	// Check if this is the top class
	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	std::string keyAttributeName = momKeyRepo->getKeyAttributeForClass(className);
	if (keyAttributeName == AttributeNameString)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}
	else
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
}


/**
 *	getImmKeyAttribute -
 *	return the key attribute name for the class. This can be prefixed or not.
 **/
std::string OamSATranslator::getImmKeyAttribute(const char *parentClass,
												const char *className,
												const char *keyAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string returnValue=keyAttributeName;
	std::string parentClassString = parentClass;
	std::string classNameString = className;
	std::string searchKey = parentClassString + "_" + classNameString;
	// Check if this is the top class
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::getImmKeyAttribute check attribute=%s", returnValue.c_str());

	OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
	std::string decoratedkeyAttributeName = momKeyRepo->getDecoratedKeyAttributeForClass(className);
	if (decoratedkeyAttributeName!="")
	{
		// Ok, this is a decorated MOM
		returnValue = decoratedkeyAttributeName;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::getImmKeyAttribute return %s", returnValue.c_str());
	return returnValue;
}

/**
 *	RetrieveComAttributeType -
 *	This METHOD must only receive attributes name coming from IMM, since they always are unique,
 *	the MOM names are not uniqe any more....
 *	Return true if attribute Name and Type is present in the MOM Repository.
 *	Return false if attribute Name and Type is not present in the MOM Repository.
 **/
bool
OamSATranslator::RetrieveComAttributeType(const char *the3gppDN, const char *attributeName,
										 MafOamSpiMoAttributeType_3 &attrType)
{
	ENTER_OAMSA_TRANSLATIONS();
	//std::string AttrNameString(attributeName); // Unused variable
	//std::string the3gppDNString(the3gppDN); // Unused variable

	OamSACache::DNList theSplitDN;
	// Use the global one here because in OIProxy we do not use the cache.
	GlobalSplitDN(the3gppDN,theSplitDN);
	// since this value comes directly from the IMM
	attrType = GetComAttributeType(theSplitDN, attributeName);
	// FIXME: Below a cast to 0 is done, this because it is made in the original code,
	// however, it is not defined in the struct so it is a bit undefined
	if(attrType == (MafOamSpiMoAttributeType_3)0)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}

/**
 *	imm2MO_AttrValue - converts an imm data value to a MO value
 */
bool OamSATranslator::Imm2MO_AttrValue(MafOamSpiTransactionHandleT txHandle,
										const SaImmAttrValuesT_2 *attr,
										MafOamSpiMoAttributeType_3 attrType,
										MafMoAttributeValueContainer_3T **result)
{
	bool RetVal = true;
	ENTER_OAMSA_TRANSLATIONS();
	// Check so that we haven't passed any nasty NULL pointers here
	if (NULL == result || NULL == attr)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	// It's OK to have zero values returned from IMM

	MafMoAttributeValueContainer_3T* resptr = *result;
	resptr->type = attrType;
	if (0 == attr->attrValuesNumber)
	{
		resptr->nrOfValues = 0;
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}

#ifndef UNIT_TEST
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Imm2MO_AttrValue");
	DEBUG_OAMSA_TRANSLATIONS("attr->attrName			%s",attr->attrName);
	DEBUG_OAMSA_TRANSLATIONS("attr->attrValueType		%d",attr->attrValueType);
	DEBUG_OAMSA_TRANSLATIONS("attr->attrValuesNumber	%d",attr->attrValuesNumber);
	DEBUG_OAMSA_TRANSLATIONS("attr->attrValues			%p",attr->attrValues);
	DEBUG_OAMSA_TRANSLATIONS("attrType					%d",attrType);
#endif

	if (SA_IMM_ATTR_SASTRINGT == attr->attrValueType)
	{
		DEBUG_OAMSA_TRANSLATIONS("attr->attrValues %s",*(char**)attr->attrValues[0]);
	}

	// Check so that there is a valid pointer
	if (0 == attr->attrValuesNumber || NULL == attr->attrValues)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	// Check so that we wern't passed garbage
	if (NULL == resptr)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	resptr->nrOfValues = attr->attrValuesNumber;
	resptr->values = new MafMoAttributeValue_3T[attr->attrValuesNumber];
	// Check allocation
	if (NULL == resptr->values)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	memset(resptr->values,0,sizeof(MafMoAttributeValue_3T) * attr->attrValuesNumber);

	for (unsigned int i = 0; i < attr->attrValuesNumber && attr->attrValues != NULL && attr->attrValues[i]!=NULL; i++)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Imm2MO_AttrValue loop nr:%d",i);
		switch (attr->attrValueType)
		{
		case SA_IMM_ATTR_SAINT32T:
			switch (attrType)
			{
			case MafOamSpiMoAttributeType_3_INT32:
				resptr->values[i].value.i32		= *(int32_t*)(attr->attrValues[i]);
				break;
			case MafOamSpiMoAttributeType_3_INT16:
				resptr->values[i].value.i16		= *(int32_t*)(attr->attrValues[i]);
				break;
			case MafOamSpiMoAttributeType_3_INT8:
				resptr->values[i].value.i8	= *(int32_t*)(attr->attrValues[i]);
				break;
			case MafOamSpiMoAttributeType_3_ENUM:
				resptr->values[i].value.theEnum = *(int32_t*)(attr->attrValues[i]);
				break;
			case MafOamSpiMoAttributeType_3_BOOL:
				resptr->values[i].value.theBool = *(int32_t*)(attr->attrValues[i]);
				break;
			default:
				// Invalid data type for this ... do nothing but set it to zero
				ERR_OAMSA_TRANSLATIONS(" ERROR: Unknown MafOamSpiMoAttributeType_3: Replacing it with type INT32 with value 0");
				resptr->values[i].value.i32		= 0;
				RetVal = false;
				break;
			}
			break;


		case SA_IMM_ATTR_SAUINT32T:
			switch (attrType)
			{
			case MafOamSpiMoAttributeType_3_UINT32:
				resptr->values[i].value.u32		= *(uint32_t*)(attr->attrValues[i]);
				break;
			case MafOamSpiMoAttributeType_3_UINT16:
				resptr->values[i].value.u16		= *(uint32_t*)(attr->attrValues[i]);
				break;
			case MafOamSpiMoAttributeType_3_UINT8:
				resptr->values[i].value.u8	= *(uint32_t*)(attr->attrValues[i]);
				break;
			case MafOamSpiMoAttributeType_3_ENUM:
				resptr->values[i].value.theEnum = *(uint32_t*)(attr->attrValues[i]);
				break;
			case MafOamSpiMoAttributeType_3_BOOL:
				resptr->values[i].value.theBool = *(uint32_t*)(attr->attrValues[i]);
				break;
			default:
				// Invalid data type for this ... do nothing but set it to zero
				ERR_OAMSA_TRANSLATIONS(" ERROR -- Unknown MafOamSpiMoAttributeType_3 -- Replacing it with type INT32 with value 0");
				resptr->values[i].value.u32		= 0;
				RetVal = false;
				break;
			}
			break;

		case SA_IMM_ATTR_SATIMET:
			// SA_TIME_T is int64 in imm while confd_time is a struct
			// user must use int64 if attr is SA_TIME_T
			// So intentionally fall through to the 64 bit section here
		case SA_IMM_ATTR_SAINT64T:
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::imm2MO_AttrValue case SA_IMM_ATTR_SAINT64T");
			if (attrType == MafOamSpiMoAttributeType_3_INT64)
			{
				resptr->values[i].value.i64		= *(int64_t*)(attr->attrValues[i]);
			}
			else
			{
				resptr->values[i].value.i64		= 0;
			}
			break;

		case SA_IMM_ATTR_SAUINT64T:
			if (attrType == MafOamSpiMoAttributeType_3_UINT64)
			{
				resptr->values[i].value.u64		= *(uint64_t*)(attr->attrValues[i]);
			}
			else
			{
				resptr->values[i].value.u64		= 0;
			}
			break;

		case SA_IMM_ATTR_SANAMET:
			// This is COM concept always a result of a REFERENCE, i.e. a Dn and in need of translation from IMM to 3gpp format
			if ( MafOamSpiMoAttributeType_3_REFERENCE )
			{
				SaNameT* atp = (SaNameT*) attr->attrValues[i];
				char *the3gppName = NULL;
				char *immName = makeCString(atp);
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Imm2MO_AttrValue : Translating a reference %s", immName);
				Imm2MO_DN(txHandle,immName,&the3gppName);
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Imm2MO_AttrValue : Result %s", the3gppName);
				resptr->values[i].value.moRef = new char[strlen(the3gppName)+1];
				if (resptr->values[i].value.moRef != NULL)
				{
					strcpy(const_cast<char*>(resptr->values[i].value.moRef),the3gppName);
				}
				else
				{
					RetVal = false;
				}
				delete [] immName;
				delete [] the3gppName;
			}
			else
			{
				RetVal = false;
			}


			break;
		case SA_IMM_ATTR_SAANYT:
			// treated as string, user must represent any as string ,eg hex,
			// otherwise the result can not be sent via Netconf and to cli
			if (attrType == MafOamSpiMoAttributeType_3_STRING)
			{
				SaAnyT* atp = (SaAnyT*) attr->attrValues[i];
				resptr->values[i].value.theString = new char[atp->bufferSize + 1];
				if (resptr->values[i].value.theString != NULL)
				{
					// adding the NULL terminator this way as it is now const in SPI Ver.3
					memset(const_cast<char*>(resptr->values[i].value.theString),
							'\0',
							atp->bufferSize + 1);
					memcpy(const_cast<char*>(resptr->values[i].value.theString),
							atp->bufferAddr,
							atp->bufferSize);
				}
				else
				{
					RetVal = false;
				}
			}
			else
			{
				RetVal = false;
			}
			break;
		case SA_IMM_ATTR_SASTRINGT:
			if (attrType == MafOamSpiMoAttributeType_3_STRING)
			{
				if(*(char**)attr->attrValues[i])
				{
					resptr->values[i].value.theString	= new char[strlen(*(char**)attr->attrValues[i])+1];
				}
				else
				{
					resptr->values[i].value.theString = new char[1];
				}
				if (NULL == resptr->values[i].value.theString)
				{
					LEAVE_OAMSA_TRANSLATIONS();
					return false;
				}
				if(*(char**)attr->attrValues[i])
				{
					//Do not parse string, use original string
					strcpy(const_cast<char*>(resptr->values[i].value.theString),*(char**)attr->attrValues[i]);
				}
				else
				{
					strcpy(const_cast<char*>(resptr->values[i].value.theString),"");
				}
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("Imm2MO case SA_IMM_ATTR_SASTRINGT RETURN FALSE");
				RetVal = false;
			}			break;
		case SA_IMM_ATTR_SAFLOATT:
			if (attrType == MafOamSpiMoAttributeType_3_DECIMAL64)
			{
				resptr->values[i].value.decimal64	= *(SaFloatT*)(attr->attrValues[i]);
			}
			else
			{
				resptr->values[i].value.decimal64	= 0;
			}
			break;
		case SA_IMM_ATTR_SADOUBLET:
			if (attrType == MafOamSpiMoAttributeType_3_DECIMAL64)
			{
				resptr->values[i].value.decimal64	= *(SaDoubleT*)(attr->attrValues[i]);
			}
			else
			{
				resptr->values[i].value.decimal64	= 0;
			}
			break;
		default:
			ERR_OAMSA_TRANSLATIONS("Unknown SA_IMM_ATTR Type: %d", attr->attrValueType);
			RetVal = false;
			break;
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}


// method that converts a decorated imm dn to the name format known by the MOM
bool OamSATranslator::unDecorateRootDn(std::string& immRootDn, std::string& originalImmRootDn)
{
	// Search to see if a entry exist for this IMM dn in the converted map
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("unDecorateRootDn : enter %s / %s", immRootDn.c_str(), originalImmRootDn.c_str());
	theMOMRepository.FindUnDecoratedImmDN(immRootDn, originalImmRootDn);
	if (originalImmRootDn != "") {
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}else {
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}


// method to check if this root class needs to be decorated to access the class in the IMM
// Input value : TheDNName for the root instantiation
bool OamSATranslator::isDecorated(std::string theRootName, std::string theParent)
{
	// Search the map with key attributes to see if we find one that maps
	// Convert to the key attribute for expected for this root
	// The format is : PmTjoho=TR OR PmTjoho=2
	ENTER_OAMSA_TRANSLATIONS();
	std::string keyTheDnRootName = theRootName;
	std::string keyTheDnParentName = theParent;
	// Remove everything after equal sign
	size_t positionOfEqualsSign = keyTheDnRootName.find("=");
	if (positionOfEqualsSign == std::string::npos) {
		// OK, this is probably the /ManagedElement that is search for.
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}else {
		removeEqualSignAndDotPartOfDN(keyTheDnRootName);
	}
	removeEqualSignAndDotPartOfDN(keyTheDnParentName);
	LEAVE_OAMSA_TRANSLATIONS();
	return theMOMRepository.FindIfRootMOMIsDecorated(keyTheDnRootName, keyTheDnParentName);
}

/**
 * MO2Imm_DN
 *
 * Translates an MO (3gpp) name into the format used by IMM
 *
 */
bool OamSATranslator::MO2Imm_DN(OamSACache::DNList& mo_name, std::string& imm_name)
{
	/* Contrary to what is said in the IP we must start with converting the
	 * individual parts to IMM format, otherwise we won't be able to find the
	 * name in the MOM database.
	 *
	 * The conversion consists of adding Id to the name, if not already there and
	 * the first character changed to lower case.
	 */
	ENTER_OAMSA_TRANSLATIONS();

	std::string TheRootName = "";
	std::string TheParent = "";
	std::string theTempString;

	OamSACache::DNListReverseIterator theOuterIter;
	OamSACache::DNListReverseIterator theOuterIterParent;
	OamSACache::DNListReverseIterator theIterRoot;
	OamSACache::DNListReverseIterator theIterParent;

	theIterParent = mo_name.rbegin();
	++theIterParent;

	// STEP1 - iterate to the next root, store the root class and parent name
	if (theIterParent != mo_name.rend())
	{
		for (theIterRoot = mo_name.rbegin(); theIterRoot != mo_name.rend(); ++theIterRoot)
		{
			if (theIterParent != mo_name.rend())
			{
				if (theMOMRepository.IsRoot(*theIterRoot, *theIterParent))
				{
					// This was the root, so let's leave. Our work is done
					TheRootName = *theIterRoot;
					TheParent = *theIterParent;
					DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN current root MOC = %s", TheRootName.c_str());
					DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN current root MOC Parent = %s", TheParent.c_str());
					++theIterRoot;
					++theIterParent;
					break;
				}
				++theIterParent;
			}else
			{
				// This is the case of ManagedElement and its MOM (SystemFunctions etc ...),
				// like ManagedElement=1,SystemFunctions=1
				// empty string is parent
				DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN rootParent = ");
				TheParent = "";
				TheRootName = *theIterRoot;
				break;
			}
		}
	}
	else
	{
		// This is the case of ManagedElement, empty string is parent
		// In this case someone translate ManagedElement=1
		TheParent = "";
		TheRootName = *(mo_name.rbegin());
		DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN root = %s", TheRootName.c_str());
	}

	// STEP2 - check if current MOM exists in key holder
	std::string rootClass = TheRootName;
	removeEqualSignAndDotPartOfDN(rootClass);
	std::string parentClass = TheParent;
	removeEqualSignAndDotPartOfDN(parentClass);
	std::string searchKey = parentClass + "_" + rootClass;
	OamSAKeyMOMRepo* momKeyRep = NULL;
	bool useTheOldMethod = false;

	if (theKeyHolder.momNameExist(searchKey))
	{
		// The MOM exists
		DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN Found key holder %s", searchKey.c_str());
		momKeyRep = theKeyHolder.getMOMRepo(searchKey);
	}
	else
	{
		// OK, No MOM, two cases here:
		// 1. Translating empty string
		// 2. Not empty, then it is a special situation
		// translate with the old method
		DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN ERROR no MOMKEYREPO key %s", searchKey.c_str());
		useTheOldMethod = true;
	}


	// STEP 3 - translate the entire MO name to IMM format piece by piece.
	// If we cross the MOM boundary due to non-singleton root, we need to recalculate Step1 and Step2 above
	imm_name = "";
	theOuterIterParent = mo_name.rbegin();
	++theOuterIterParent;

	for (theOuterIter = mo_name.rbegin(); theOuterIter != mo_name.rend(); ++theOuterIter, ++ theOuterIterParent)
	{
		// If we need to decorate check that we now translate a child of an EcimContribution, all are stored in theKeyHolder
		rootClass = (*theOuterIter);
		removeEqualSignAndDotPartOfDN(rootClass);

		if (theOuterIterParent != mo_name.rend())
		{
			parentClass = (*theOuterIterParent);
			removeEqualSignAndDotPartOfDN(parentClass);
		}
		else
		{
			parentClass="";
		}
		searchKey = parentClass + "_" + rootClass;

		if ( (theKeyHolder.momNameExist(searchKey)) && (isDecorated(TheRootName, TheParent)))
		{
			DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN : Prefixing Active");
			//Decorate this root class
			std::string prefixedtheIter;

                        theMOMRepository.FindDecorationForMOMDN(TheRootName, TheParent, prefixedtheIter);
			if (useTheOldMethod)
				theTempString = Convert3GPPNameFragmentToImmNameFragment(*theOuterIter);
			else
				theTempString = Convert3GPPNameFragmentToImmNameFragment(*theOuterIter, momKeyRep);
			theTempString = prefixedtheIter + theTempString;
		}else
		{
			if (useTheOldMethod)
				theTempString = Convert3GPPNameFragmentToImmNameFragment(*theOuterIter);
			else
				theTempString = Convert3GPPNameFragmentToImmNameFragment(*theOuterIter, momKeyRep);
		}

		imm_name = imm_name + theTempString;
		if ((!theTempString.empty()) && (TheRootName != *theOuterIter))
		{
			imm_name = imm_name +',';
		}
		DEBUG_OAMSA_TRANSLATIONS("Imm name: <%s>, temp string: <%s>", imm_name.c_str(), theTempString.c_str());

		// Check if we have reached a root
		if (TheRootName == *theOuterIter)
		{
			if (theMOMRepository.IsSingletonRoot(TheRootName, TheParent))
			{
				// This was the final singleton root, so let's leave. Our work is done
				break;
			}
			else
			{
				// Not a singleton root, we are at the border about to cross into a new MOM on the next iteration.
				// Re-compute Step1 and Step2 so we have the new Root & Parent names for the next MOM
				// then proceed as normal.
				if (!theTempString.empty()) {
					imm_name = imm_name +',';
				}

				// STEP1 - find the next root, store the root class and parent name
				if (theIterParent != mo_name.rend())
				{
					for ( ; theIterRoot != mo_name.rend(); ++theIterRoot)
					{
						if (theIterParent != mo_name.rend())
						{
							if (theMOMRepository.IsRoot(*theIterRoot, *theIterParent))
							{
								// This was the root, so let's leave. Our work is done
								TheRootName = *theIterRoot;
								TheParent = *theIterParent;
								DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN current root MOC = %s", TheRootName.c_str());
								DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN current root MOC Parent = %s", TheParent.c_str());
								++theIterRoot;
								++theIterParent;
								break;
							}
							++theIterParent;
						}else
						{
							// This is the case of ManagedElement and its MOM (SystemFunctions etc ...),
							// like ManagedElement=1,SystemFunctions=1
							// empty string is parent
							DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN rootParent = ");
							TheParent = "";
							TheRootName = *theIterRoot;
							break;
						}
					}
				}
				else
				{
					// This is the case of ManagedElement, empty string is parent
					// In this case someone translate ManagedElement=1
					TheParent = "";
					TheRootName = *(mo_name.rbegin());
					DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN root = %s", TheRootName.c_str());
				}

				// STEP2 - check if current MOM exists in key holder
				rootClass = TheRootName;
				removeEqualSignAndDotPartOfDN(rootClass);
				parentClass = TheParent;
				removeEqualSignAndDotPartOfDN(parentClass);
				searchKey = parentClass + "_" + rootClass;

				if (theKeyHolder.momNameExist(searchKey))
				{
					// The MOM exists
					DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN Found key holder %s", searchKey.c_str());
					momKeyRep = theKeyHolder.getMOMRepo(searchKey);
				}
				else
				{
					// OK, No MOM, two cases here:
					// 1. Translating empty string
					// 2. Not empty, then it is a special situation
					// translate with the old method
					DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN ERROR no MOMKEYREPO key %s", searchKey.c_str());
					useTheOldMethod = true;
				}
			}
		}
	}
	size_t immDnSize=imm_name.size();
	 //if imm_name has comma(,) appending in its last position,we need to erase it in order to align with the immDn format.
	 if(immDnSize > 0 &&  imm_name[immDnSize-1] == ',')
	 {
		imm_name = imm_name.erase(immDnSize-1);
	 }
	DEBUG_OAMSA_TRANSLATIONS("MO2Imm_DN: Returning TRUE, imm_name=<%s>", imm_name.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}


/*
 *	As above but with different parameters
 */
bool OamSATranslator::MO2Imm_DN(const char* mo_name, char** imm_name)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string moNameStr(mo_name);
	std::string immNameStr;
	OamSACache::DNList moNameList;
	bool result;

	GlobalSplitDN(moNameStr, moNameList);
	result = MO2Imm_DN(moNameList, immNameStr);

	*imm_name = new char[immNameStr.length()+1];
	strcpy(*imm_name, immNameStr.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return result;
}

/*
 * retriveMoTop
 *
 * Creates the DN from CompTop down to the root class.
 * Input : theMOMRetVal
 * Return: parentName
 */
std::string OamSATranslator::retriveMoTop(MOMRootRepository::MOMRootDataEntryMapRetVal theMOMRetVal)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string n3gpp_name;
	// OK here we build the path from top down to the root class
	if (theMOMRetVal.second)
	{
		std::string parentName = "";
		std::string searchKey = "";
		// This return a empty repository
		OamSAKeyMOMRepo* MomKeyRepo = theKeyHolder.getMOMRepo(searchKey);
		std::string keyAttribute = "";
		// Here we iterate over the grandparent to the root class up to ManagedElement=1
		MOMRootDataEntry::MOMRootParentListReverseIterator theMOMRevIter;
		for (theMOMRevIter = theMOMRetVal.first->second.GetLastRoot();
			 theMOMRevIter != theMOMRetVal.first->second.GetReverseEnd();
			 theMOMRevIter++)
		{
			// OK get the MOMkeyrepo and check the format of the key attribute
			// ComTop is always on top, so get that first
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::retriveMoTop : Parent <%s>", (*theMOMRevIter).c_str());
			if (!MomKeyRepo->classNameInMOM(*theMOMRevIter))
			{
				// Get the first/next MOM repository
				searchKey = parentName + "_" + (*theMOMRevIter);
				MomKeyRepo = theKeyHolder.getMOMRepo(searchKey);
			}
			keyAttribute = MomKeyRepo->getKeyAttributeForClass((*theMOMRevIter));
			// Check if this is case A or B
			// Example Case A			// Example B:
			// Class name = MyClass		// Class name = MyClass
			// RDN : MYCLASSID=12		// RDN : MAID=13
			// DN  : MyClass=12			// DN  : MyClass.MAID=13
			if ( ConvertToLowerCase(keyAttribute) != ConvertToLowerCase((*theMOMRevIter)+STRUCT_CLASS_KEY_ATTRIBUTE) )
			{
				// Case B
				n3gpp_name = n3gpp_name + *theMOMRevIter + "." + keyAttribute + "=1,";
			}
			else
			{
				// Case A, default case
				n3gpp_name = n3gpp_name + *theMOMRevIter + "=1,";
			}
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::retriveMoTop %s", n3gpp_name.c_str());
			// Set this to the next parent
			parentName = (*theMOMRevIter);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return n3gpp_name;
}

// HElper method retrieving the first piece of a immRdn and removes it.
std::string OamSATranslator::nextFragment(std::string& immRdnHead)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string immFragment = immRdnHead;
	try {
		// Remove everything before last ,
		if (immFragment.find_last_of(',') != std::string::npos)
		{
			immFragment = immFragment.substr(immFragment.find_last_of(',') + 1);
			immRdnHead = immRdnHead.substr(0, immRdnHead.find_last_of(','));
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::nextFragment return immRdnHead=%s", immRdnHead.c_str());
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::nextFragment return immFragment=%s", immFragment.c_str());

		}
		else
		{	// OK, only one element left, return that and empty input string
			immRdnHead.clear();
			LEAVE_OAMSA_TRANSLATIONS();
			return immFragment;
		}
		LEAVE_OAMSA_TRANSLATIONS();
		return immFragment;
	}catch (...) {
		// Only to not core dump
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::nextFragment failed to clean string");
		return immRdnHead;
	}
}

// Helper method that retrieves everything before the "="
std::string OamSATranslator::retriveKey(const std::string nextpiece)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string immFragment = nextpiece;
	try {
		// Remove everything before last ,
		if (immFragment.find_last_of('=') != std::string::npos)
		{
			immFragment = immFragment.substr(0, immFragment.find_last_of('='));
		}
		LEAVE_OAMSA_TRANSLATIONS();
		return immFragment;
	}catch (...) {
		// Only to not core dump
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::retriveKey failed to clean string");
		LEAVE_OAMSA_TRANSLATIONS();
		return nextpiece;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return nextpiece;
}
// Helper method that retrieves everything after the "="
std::string OamSATranslator::retiveValue(const std::string nextpiece)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string immFragment = nextpiece;
	try {
		// Remove everything before last ,
		if (immFragment.find_last_of('=') != std::string::npos)
		{
			immFragment = immFragment.substr(immFragment.find_last_of('=')+1);
		}
		LEAVE_OAMSA_TRANSLATIONS();
		return immFragment;
	}catch (...) {
		// Only to not core dump
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::retiveValue failed to clean string");
		LEAVE_OAMSA_TRANSLATIONS();
		return nextpiece;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return nextpiece;
}

// Helper Method creating the DN below the Root
std::string OamSATranslator::ImmBuilder(const std::string immRdn, OamSAKeyMOMRepo* tempRepo)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string immRdnHead = immRdn;
	std::string nextpiece;
	std::string n3gpp_name;
	std::queue<std::string> dnList; // Build the order and then retrieve the result FIFO-basis.
	DEBUG_OAMSA_TRANSLATIONS("immRdn = %s", immRdn.c_str());

	while ( !immRdnHead.empty() )
	{
		nextpiece = nextFragment(immRdnHead);
		std::string keyAttributeFragment = retriveKey(nextpiece);
		std::string KeyValue = retiveValue(nextpiece);
		std::string className = tempRepo->getClassNameForKeyAttribute(keyAttributeFragment);
		if ( className.empty() && ConvertToLowerCase(keyAttributeFragment) != ConvertToLowerCase(STRUCT_CLASS_KEY_ATTRIBUTE) )
		{
			// OK, so we might be just looking in the wrong MOM here...
			// Try to re-acquire the MOM for the fragment
			// This happens when two root MOCs in EcimContainment belong to different MOMs.
			tempRepo = theKeyHolder.getMOMRepoImmKey(keyAttributeFragment);
			className = tempRepo->getClassNameForKeyAttribute(keyAttributeFragment);
		}
		// Check for prefixing here
		if (tempRepo->isMomDecorated())
		{
			// Remove the decoration from the keyAttributeFragment
			std::string momPrefix = tempRepo->getMomName();
			size_t position = keyAttributeFragment.find(momPrefix);
			if (position != std::string::npos)
			{
				keyAttributeFragment = keyAttributeFragment.substr(momPrefix.length());
			}
		}
//		DEBUG_OAMSA_TRANSLATIONS("keyAttributeFragment = %s", keyAttributeFragment.c_str());
//		DEBUG_OAMSA_TRANSLATIONS("KeyValue = %s", KeyValue.c_str());
//		DEBUG_OAMSA_TRANSLATIONS("className = %s", className.c_str());
		// Check if this is case A or B
		// Example Case A			// Example B:
		// Class name = MyClass		// Class name = MyClass
		// RDN : MYCLASSID=12		// RDN : MAID=13
		// DN  : MyClass=12			// DN  : MyClass.MAID=13
		if ( className.empty() && (ConvertToLowerCase(keyAttributeFragment) == ConvertToLowerCase(STRUCT_CLASS_KEY_ATTRIBUTE)))
		{
			std::string temp = keyAttributeFragment + "=" + KeyValue;
			dnList.push(temp);
		}
		else
		{
			if ( ConvertToLowerCase(keyAttributeFragment) != ConvertToLowerCase(className+STRUCT_CLASS_KEY_ATTRIBUTE) )
			{
				// Case B
				std::string temp = className + "." + keyAttributeFragment + "=" + KeyValue;
				dnList.push(temp);
			}
			else
			{
				// Case A, default case
				//DEBUG_OAMSA_TRANSLATIONS("Case A");

				std::string temp = className + "=" + KeyValue;
				dnList.push(temp);
			}
		}
	}
	// Build the Dn
	while (!dnList.empty())
	{
		n3gpp_name = n3gpp_name + dnList.front();
		dnList.pop();
		if (!dnList.empty())
			n3gpp_name = n3gpp_name + ",";
		//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ImmBuilder n3gpp_name:%s", n3gpp_name.c_str());
	}
	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ImmBuilder RETRUNS ImmRdn2MOTop:%s", n3gpp_name.c_str());
	return n3gpp_name;
}
// This method builds the DN for a provided immRDN
/*
 * ImmClass2MOTop
 *
 * Creates the DN for a ImmRDN
 * INPUT : SomeThing like : foo=2,MAID=1
 * OUTPUT: ManagedElement=1, SystemFunctions=1,ObjImp.MAID=1,TestClass.Foo=2
 */
std::string OamSATranslator::ImmRdn2MOTop(const std::string immRdn)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ImmRdn2MOTop translate immRdn <%s>", immRdn.c_str());
	std::string n3gpp_name;
	// First find the mom for the root class, to do this we need the immKey attribute for the
	// Root of this MOM, that is the last element in the immRnd.
	std::string keyAttributeName = immRdn;
	try {
		// Remove everything after	=
		if (keyAttributeName.find_last_of('=') != std::string::npos)
		{
			keyAttributeName = keyAttributeName.substr(0,keyAttributeName.find_last_of('='));
		}
		// Remove everything before last ,
		if (keyAttributeName.find_last_of(',') != std::string::npos)
		{
			keyAttributeName = keyAttributeName.substr(keyAttributeName.find_last_of(',')+1);
		}
	}catch (...) {
		// Only to not core dump
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ImmRdn2MOTop failed to clean string");
		return "invalidDn";
	}
	//DEBUG_OAMSA_TRANSLATIONS("CALL getMOMRepoImmKey %s", keyAttributeName.c_str());
	OamSAKeyMOMRepo* tempRepo = theKeyHolder.getMOMRepoImmKey(keyAttributeName);
	MOMRootRepository::MOMRootDataEntryMapRetVal theMOMRetVal = theMOMRepository.Find(tempRepo->getRootClass());
	// Check for decorated MOMS
	if (!theMOMRetVal.second)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ImmRdn2MOTop unable to find Root Class in the MOMRootDataEntry. Looking for decorated imm classes.");
		// OK, we might have a decorated class Here, search the repository of decorated imm classes
		// Use the key attribute to find our class name
		std::string decoratedClassName = theMOMRepository.FindDecoratedClassName(keyAttributeName);
		// Search again with possible candidate hope we are lucky
                theMOMRetVal = theMOMRepository.Find(decoratedClassName);
		if (!theMOMRetVal.second) {
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ImmRdn2MOTop unable to find Root Class for decoratedClassName:%s in the MOMRootDataEntry", decoratedClassName.c_str());
			return "invalidDn" ;
		}
	}
	// OK here we build the path from top down to the root class
	n3gpp_name = retriveMoTop(theMOMRetVal);
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ImmRdn2MOTop after retriveMoTop %s", n3gpp_name.c_str());
	// Now create the rest of the DN down to the last element of the immRdn
	// This is left: immRdnHead
	// Translate piece by piece
	//std::string nextpiece; // Unused variable
	try {
		n3gpp_name = n3gpp_name + ImmBuilder(immRdn, tempRepo);
		//DEBUG_OAMSA_TRANSLATIONS("OK DONE %s", n3gpp_name.c_str());
	}catch (...) {
		// Only to not core dump
		//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ImmRdn2MOTop failed to clean string");
		return n3gpp_name;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("Returning OamSATranslator::ImmRdn2MOTop %s", n3gpp_name.c_str());
	return n3gpp_name;
}

/**
 * buildImmObjectPath
 *
 * Input :
 * std::string objectPath
 * std::string objectFragment
 * Output:
 * std::string containing a imm object name that is the concatenation of the input strings.
 */
std::string OamSATranslator::buildImmObjectPath(std::string objectPath, std::string objectFragment)
{
	std::string finalPath;

	ENTER_OAMSA_TRANSLATIONS();
	if ((objectPath.length()!=0) && (objectFragment.length()!=0))
	{
		finalPath = objectFragment + "," + objectPath;
	}
	else
	{
		if ((objectPath.length()!=0))
		{
			finalPath = objectPath;
		}
		else
		{
			finalPath = objectFragment;
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return finalPath;
}
// QUICK FIX OF STRING PROBLEM FOUND 4 DAYS BEFORE PRA COMSA3.2
// FIXME : REWRITE THIS FROM SCRATCH AS SOON AS POSSIBLE
// The only reason that this method was created is that
// the construction
// n3gpp_name = n3gpp_name + ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
// *theDNRevIter,
// *theDNRevIter);
// DO NOT WORK FOR THESE TWO STRINGS!
// <ManagedElement=1,SystemFunctions=1,SwM=1,> + <UpgradePackage=ERIC-Installation-UP1122-P1A29>
// FURTHER ANALYZE IS NEEDED
std::string OamSATranslator::addTwoString(std::string stringOne, std::string stringTwo)
{
	ENTER_OAMSA_TRANSLATIONS();
	char buff1[stringOne.length()+2];
	strcpy(buff1, stringOne.c_str());
	DEBUG_OAMSA_TRANSLATIONS("buff1 <%s>",buff1);
	char buff2[stringTwo.length()+2];
	strcpy(buff2, stringTwo.c_str());
	DEBUG_OAMSA_TRANSLATIONS("buff2 <%s>",buff2);
	LEAVE_OAMSA_TRANSLATIONS();
	return (std::string(buff1) + std::string(buff2));
}
/**
 * Imm2MO_DN
 *
 * Translates an Imm name to a 3gpp name. This might be only partially successful
 * because some information is thrown away regarding parents to the sub tree
 * represented in IMM. It works if parent classes are singletons with the value =1
 *
 */
bool OamSATranslator::Imm2MO_DN(MafOamSpiTransactionHandleT txHandle,
								OamSACache::DNList& imm_name,
								std::string& n3gpp_name)
{
	//FIXME : UABJOY This Code is really complicated, needs to be refactored.
	ENTER_OAMSA_TRANSLATIONS();
	MOMRootDataEntry::MOMRootParentListReverseIterator theMOMRevIter;
	OamSACache::DNListReverseIterator theDNRevIter;
	MOMRootRepository::MOMRootDataEntryMapRetVal theMOMRetVal;
	std::string theTempString;
	// This flag is valid only for the MOM root DN, and when it has been check it is toggled to false
	bool checkForTheRootOnly = true;
	// USed to indicate if the MOM is decorated
	bool decoratedRoot = false;
	std::string converted_root_dn;
	//std::string decorationOfMOM; // Unused variable

	n3gpp_name = "";
	theDNRevIter = imm_name.rbegin();
	if ((theDNRevIter != imm_name.rend()) && ((*theDNRevIter).length()!=0))
	{	// OK, this is the root for the MOM since it always comes last in the IMM DN.
		// If this is a decorated root, we will find it here since it is stored under it's decorated IMM name.
		theTempString	= *theDNRevIter;
		// This is a IMM name and in theMOMRepository the class name is stored
		// Now we need to find the corresponding class for this key attribute
		std::string finalSearchKey = ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																			  theTempString,
																			  theTempString);
		// Now remove the everything after and including the '=' and we have the class name.
		std::string theSearchStr = finalSearchKey.substr(0, finalSearchKey.find('='));
		//DEBUG_OAMSA_TRANSLATIONS("Imm2MO_DN search for root class %s", theSearchStr.c_str());
		theMOMRetVal = theMOMRepository.Find(theSearchStr);
		// OK here we build the path from top down to the root class
		n3gpp_name = retriveMoTop(theMOMRetVal);
		// And here we append the rest of the path from rootClass of MOM down to the bottom.
		std::string currentImmDn;
		// This value will contain the position where the prefix ends of the class name.
		size_t positionOfEndOfDecoration = 0;
		for (;;)
		{
			// But here we need to translate the root class that is prefix.
			//char* currentImmDn[MaxStringLength];
			if (checkForTheRootOnly)
			{
				converted_root_dn.clear();
				if (unDecorateRootDn(*theDNRevIter, converted_root_dn))
				{
					// Only for the root of the IMM DN. I.e. something like CmwPmpmId=1 is expected here and
					// that must be converted to pmId=1 (OR rather Pm=1)
					// OR something like CmwPmTricky=1
					// That shall be converted to Pm.Tricky=1
					decoratedRoot = true;
					currentImmDn = buildImmObjectPath(currentImmDn, (*theDNRevIter));
					//n3gpp_name = n3gpp_name + ConvertImmNameFragmentTo3GPPNameFragment(txHandle, converted_root_dn, *theDNRevIter);
					std::string temporaryNameFragment = ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																								 converted_root_dn,
																								 currentImmDn);
					// Here we have one of two cases.
					// case 1: temporaryNameFragment = "className=99"
					// In this case nothing more is needed
					// OR
					// case 2: temporaryNameFragment = "classNameDecorated.blopp=99"
					// In this case we have to convert the classNameDecorated TO className AND
					// check if this new classname means that we need to add className.blopp=99 to the 3GPP name
					// OR className=99
					// And continue do so for all coming parts of the RDN.
					size_t positionOfDot= temporaryNameFragment.find_first_of('.');
					if (positionOfDot!=std::string::npos)
					{
						// Case 2
						std::string classNameDecorated = temporaryNameFragment.substr(0, positionOfDot);
						std::string nameAfterDot = temporaryNameFragment.substr(positionOfDot, temporaryNameFragment.length());
						// Find the decoration by removing converted key attribute from decorated key attribute.
						positionOfEndOfDecoration = (*theDNRevIter).find(converted_root_dn.c_str());
						bool removeEveryThingAfterDot = false;
						try {
							// Remove the decorated part from the classNameDecorated
							classNameDecorated.erase(0, positionOfEndOfDecoration);
							// Now with the new fragment looking like this ClassName.blopp we must ensure that
							// we do not have case 1 active again after this translation.
							// REmove value part of nameAfterDot and check if we need to remove everything after the dot.
							std::string temporaryValue = nameAfterDot;
							// OK, there is a dot left here that we do not want, remove it.
							temporaryValue.erase(0,1);
							removeEqualSignAndDotPartOfDN(temporaryValue);
							std::string keyAttributeName = ConvertToLowerCase(temporaryValue);
							// REmove Id at the end, if there is one
							if (keyAttributeName.length() > 2)
								keyAttributeName.erase(keyAttributeName.length()-2, 2);
							if (ConvertToLowerCase(classNameDecorated) == keyAttributeName)
							{
								removeEveryThingAfterDot = true;
							}
							// Now build the 3GPP name
							if (removeEveryThingAfterDot)
							{
								std::string valuePart = nameAfterDot;
								if (nameAfterDot.find_first_of('=') != std::string::npos)
								{
									valuePart = nameAfterDot.substr(nameAfterDot.find_first_of('='));
								}
								n3gpp_name = n3gpp_name + classNameDecorated + valuePart;
							}
							else
							{
								n3gpp_name = n3gpp_name + classNameDecorated + nameAfterDot;
							}
						}catch(...) {
							// Ops here we have something unexpected
							ERR_OAMSA_TRANSLATIONS("Unexpected COM_SA ERROR in OamSATranslator::Imm2MO_DN");
						}

					}
					else
					{
						// Case 1
						n3gpp_name = n3gpp_name + temporaryNameFragment;
					}
					++theDNRevIter;

					// TR HR94824 fix .
					if ((theDNRevIter != imm_name.rend()) && (unDecorateRootDn(*theDNRevIter, converted_root_dn)))
					{
						checkForTheRootOnly = true;
					}
					else
					{
						checkForTheRootOnly = false;
					}

				}
				else
				{
					currentImmDn = buildImmObjectPath(currentImmDn, (*theDNRevIter));
					n3gpp_name = n3gpp_name + ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																						*theDNRevIter,
																						currentImmDn);
					theDNRevIter++;
					// TR HR94824 fix .
					if ((theDNRevIter != imm_name.rend()) && (unDecorateRootDn(*theDNRevIter, converted_root_dn)))
					{
						checkForTheRootOnly = true;
					}
					else
					{
						checkForTheRootOnly = false;
					}
				}
			}
			else
			{
				// Normal case
				currentImmDn = buildImmObjectPath(currentImmDn, (*theDNRevIter));
				if (decoratedRoot)
				{
					// Check if we need to remove the decoration from the translation
					std::string temporaryNameFragment = ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																								 *theDNRevIter++,
																								 currentImmDn);
					// TR HR94824 fix .
					if ((theDNRevIter != imm_name.rend()) && (unDecorateRootDn(*theDNRevIter, converted_root_dn)))
					{
						checkForTheRootOnly = true;
					}

					size_t positionOfDot= temporaryNameFragment.find_first_of('.');
					if (positionOfDot!=std::string::npos)
					{
						std::string nameAfterDot = temporaryNameFragment.substr(positionOfDot, temporaryNameFragment.length());
						std::string classNameDecorated;
						try {
							classNameDecorated = temporaryNameFragment.substr(0, positionOfDot);
							// Remove the decorated part from the classNameDecorated
							classNameDecorated.erase(0, positionOfEndOfDecoration);
						}catch(...) {
							// Ops here we have something unexpected
							ERR_OAMSA_TRANSLATIONS("Unexpected COM_SA ERROR in OamSATranslator::Imm2MO_DN");
						}
						size_t positionOfEqual = temporaryNameFragment.find_first_of('=');
						try {
							std::string nameAfterEqual = temporaryNameFragment.substr(positionOfEqual, temporaryNameFragment.length());
							std::string stringId = "";
							if (positionOfEqual - positionOfDot == 3)
							{
								stringId = temporaryNameFragment.substr(positionOfDot + 1, 2);
								DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Imm2MO_DN stringId %s",stringId.c_str());
							}
							if (stringId != "" && stringId == STRUCT_CLASS_KEY_ATTRIBUTE)
								n3gpp_name = n3gpp_name + classNameDecorated + nameAfterDot;
							else
								n3gpp_name = n3gpp_name + classNameDecorated + nameAfterEqual;
						}catch(...) {
							// we landed here as there is equalto missing in the dn received
							n3gpp_name = n3gpp_name + temporaryNameFragment;
						}
					}
					else
					{
						n3gpp_name = n3gpp_name + temporaryNameFragment;
					}
				}
				else
				{
					n3gpp_name = n3gpp_name + ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																						*theDNRevIter++,
																						currentImmDn);
					// TR HR94824 fix .
					if ((theDNRevIter != imm_name.rend()) && (unDecorateRootDn(*theDNRevIter, converted_root_dn)))
					{
						checkForTheRootOnly = true;
					}
				}
			}
			if (theDNRevIter == imm_name.rend())
			{
				break;
			}
			//n3gpp_name = n3gpp_name + ",";
			n3gpp_name = addTwoString(n3gpp_name, ",");
		}
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Imm2MO_DN : Leave with value %s", n3gpp_name.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}

/*
 *	As above but with different parameters
 */
bool OamSATranslator::Imm2MO_DN(MafOamSpiTransactionHandleT txHandle,
								const char* imm_name,
								char** mo_name)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string immNameStr(imm_name);
	std::string moNameStr;
	OamSACache::DNList immNameList;
	bool result;

	GlobalSplitDN(immNameStr, immNameList);

	result = Imm2MO_DN(txHandle, immNameList, moNameStr);

	*mo_name = new char[moNameStr.length()+1];
	strcpy(*mo_name, moNameStr.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return result;
}

/**
 * BuildObjectName
 * This method create a 3GPP DN from the input variables.
 * Input:
 * ParentName		: The 3GPP DN for the parent instance of the searched object
 * ClassNameMOM		: The class name of the created object
 * KeyAttributeName : The name of the key attribute of the class
 * AttributeValue	: The instance value for the new object
 * Output:
 * ObjectName		: The 3GPP DN for the new object
 *
 */
void OamSATranslator::BuildObjectName(const std::string& ParentName,
									  const std::string& ClassNameMOM,
									  const std::string& KeyAttributeName,
									  const std::string& KeyAttributeValue,
									  std::string& ObjectName)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (ParentName != "")
	{
		ObjectName = ParentName;
	}

	if (ObjectName != "")
	{
		ObjectName += ',';
	}

	// OK Build the DN for the new object
	std::string classNameLowerCase = ConvertToLowerCase(ClassNameMOM);
	classNameLowerCase += STRUCT_CLASS_KEY_ATTRIBUTE;
	if (classNameLowerCase == ConvertToLowerCase(KeyAttributeName))
	{
		ObjectName += ClassNameMOM;
	}
	else
	{
		ObjectName += ClassNameMOM + "." + KeyAttributeName;
	}
	// Check if there is an = sign in the Attribute value, otherwise insert it
	if (KeyAttributeValue[0] != '=')
	{
		ObjectName += "=";
	}

	ObjectName += KeyAttributeValue;
	//DEBUG_OAMSA_TRANSLATIONS("BuildObjectName:  build %s", ObjectName.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return ;
}

/*
 *
 *	OamSATranslator::GetComAction
 *
 *	Fetches the action for name in a specified MO from the repository
 *
 */

MafOamSpiMoActionT* OamSATranslator::GetComAction(OamSACache::DNList& mo_name, const std::string& actionName)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (actionName.size() != 0) {
		for (MafOamSpiMoActionT* ap = GetComActionList(mo_name); ap!=NULL; ap=ap->next) {
			if (!strcmp(ap->generalProperties.name, actionName.c_str())) {
				//DEBUG_OAMSA_TRANSLATIONS("MoAction found for %s", actionName.c_str());
				LEAVE_OAMSA_TRANSLATIONS();
				return ap;
			}
		} // for
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return 0;
}

/*
 *
 *	OamSATranslator::GetComActionList
 *
 *	Fetches the action list for a specified MO from the repository
 *
 */

MafOamSpiMoActionT* OamSATranslator::GetComActionList(OamSACache::DNList& mo_name)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafOamSpiMocT* parentMoc = GetComMoc(mo_name);
	if (parentMoc != NULL) {
		MafOamSpiMoActionT* ap = parentMoc->moAction;
		LEAVE_OAMSA_TRANSLATIONS();
		return ap;
	} else {
		LEAVE_OAMSA_TRANSLATIONS();
		return NULL;
	}
}

/*
 *
 *	OamSATranslator::GetComAttributeList
 *
 *	Fetches the attribute list for a specified MO from the repository
 *
 */

MafOamSpiMoAttributeT* OamSATranslator::GetComAttributeList(OamSACache::DNList& mo_name)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafOamSpiMocT* parentMoc = GetComMoc(mo_name);
	if (parentMoc != NULL) {
		MafOamSpiMoAttributeT* ap = parentMoc->moAttribute;
		LEAVE_OAMSA_TRANSLATIONS();
		return ap;
	} else {
		LEAVE_OAMSA_TRANSLATIONS();
		return NULL;
	}
}

/*
 *
 *	OamSATranslator::GetComMoc
 *
 *	Fetches the MO from the repository
 *
 */
MafOamSpiMocT* OamSATranslator::GetComMoc(OamSACache::DNList& mo_name)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafOamSpiMocT*	theMoc_p;
	//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMoc : Enter");
	GetComMocAndMom(mo_name, &theMoc_p, NULL);
	LEAVE_OAMSA_TRANSLATIONS();
	return theMoc_p;
}
/*
 *
 *	OamSATranslator::GetComMom
 *
 *	Fetches the MOM from the repository
 *
 */
MafOamSpiMomT* OamSATranslator::GetComMom(OamSACache::DNList& mo_name)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafOamSpiMomT*	theMom_p;
	GetComMocAndMom(mo_name, NULL, &theMom_p);
	LEAVE_OAMSA_TRANSLATIONS();
	return theMom_p;
}
/*
 *
 *	OamSATranslator::convertDnToClass
 *	Assumes 3GPP format of the input!
 *	Removes equal sign and dot to convert name to class name
 *
 */
std::string& OamSATranslator::convertDnToClass(std::string& dnName)
{
	////DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::convertDnToClass : Enter");
	ENTER_OAMSA_TRANSLATIONS();
	std::string temporary = dnName.substr(0, dnName.find('='));
	size_t equalSignPosition = temporary.find('.');
	if (equalSignPosition != std::string::npos) {
		temporary = temporary.erase(equalSignPosition);
	}
	dnName = temporary;
	////DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::convertDnToClass : Return %s", dnName.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return dnName;
}
/*
 *
 *	OamSATranslator::removeAttributePartOfDN
 *	Helper function that remove all '.' and '=' from a 3GPP DN
 *	so the strings becomes a set of class names.
 *
 */
void OamSATranslator::removeEqualSignAndDotPartOfDN(std::string &theString)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string theWorkingStr = theString;
	// Trim off = and anything beyond
	if ( theWorkingStr.find('=') != std::string::npos) {
		theWorkingStr = theString.substr(0, theString.find('='));
		// Remove everything after '.'
		if ( theWorkingStr.find('.') != std::string::npos) {
			theWorkingStr = theWorkingStr.erase(theWorkingStr.find_first_of('.'));
		}
		// Assign back the result
		theString = theWorkingStr;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}
/*
 *
 *	OamSATranslator::getParentName
 *	Traverse the MOOM structure to find a parent to the rootClass pointer provided
 *
 */
std::list<std::string> OamSATranslator::getParentName(MafOamSpiMocT* rootClass)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string> theMocParentList;
	std::string rootParentClassName;
	if (rootClass != NULL)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::getParentName : Enter with %s", rootClass->generalProperties.name);
		MafOamSpiContainmentT *parentContainment = rootClass->parentContainment;
		while (parentContainment != NULL)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::getParentName : Found %s", parentContainment->parentMoc->generalProperties.name);
			rootParentClassName =  parentContainment->parentMoc->generalProperties.name;
			if (!rootParentClassName.empty())
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::getParentName : push back parentName : %s", rootParentClassName.c_str());
				theMocParentList.push_back(rootParentClassName);
			}
			parentContainment = parentContainment->nextContainmentSameChildMoc;
		}
	}
	////DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::getParentName : No Parent found, returns NULL");
	LEAVE_OAMSA_TRANSLATIONS();
	return theMocParentList;
}

// This method search the MOM repository to find the root class and the MOM data
// for the mom that list mo_name points to.
void OamSATranslator::GetComMocAndMom(OamSACache::DNList mo_name,
									  MafOamSpiMocT** theMoc_pp, MafOamSpiMomT** theMom_pp)
{


	ENTER_OAMSA_TRANSLATIONS();
	OamSACache::DNListIterator						theIter;
	OamSACache::DNListIterator						theTempIter;
	OamSACache::DNListIterator						theIterParent;
	std::string										theTempString;
	MOMRootRepository::MOMRootDataEntryMapRetVal	theMOMRetVal;

	DEBUG_OAMSA_TRANSLATIONS("Enter GetComMocAndMom");
	//-----------------------
	// Step 1
	// Iterate through the entire MO, and save the last root class and its parent to theIter and parentToMOCName
	//-----------------------
	theIter = mo_name.end();	// Just in case nothing is found, otherwise it will crash, and that it did.....
	theIterParent = mo_name.begin();
	std::string parentToMOCName = "";
	bool firstLoop = true;
	for (theTempIter = mo_name.begin(); theTempIter != mo_name.end(); ++theTempIter)
	{
		if (!firstLoop) {
			if (theMOMRepository.IsRoot(*theTempIter, *theIterParent)) {
				// Save the iterator, it's the last root we want.(the root for the MOM)
				theIter = theTempIter;
				parentToMOCName = *theIterParent;
			}
			++theIterParent;
		}else {
			// Here we start with no Parent! That is ManagedElement=1
			std::string emptyString = "";
			if (theMOMRepository.IsRoot(*theTempIter, emptyString)) {
				// Save the iterator, it's the last root we want.(the root for the MOM)
				theIter = theTempIter;
				parentToMOCName = "";
			}
			firstLoop = false;
		}
	}
	// Clean the DN name of the parentToMOCName to become a class name, that is remove everything after '='
	// and '.'
	parentToMOCName = convertDnToClass(parentToMOCName);
	theTempString = convertDnToClass(*theIter);
	DEBUG_OAMSA_TRANSLATIONS(" GetComMocAndMom has parentToMocName [%s], TempString [%s]", parentToMOCName.c_str(), theTempString.c_str());

	//-----------------------
	// Step2
	// Search MR SPI for each MOM and each MOM root class to find a match for the root class in theIter
	//-----------------------
	if (theIter != mo_name.end())
	{
		MafOamSpiMocT *TopRootMoc;

		theModelRepo_v1_p->getTreeRoot(&TopRootMoc);
		if (NULL == TopRootMoc)
		{
			DEBUG_OAMSA_TRANSLATIONS("GetComMocAndMom() could not get Tree Root");
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}
		else
		{
			GetComMocAndMom_loopRootClasses(TopRootMoc, mo_name, parentToMOCName, theTempString, theMoc_pp, theMom_pp);
		}
	}

	LEAVE_OAMSA_TRANSLATIONS();
}



void OamSATranslator::GetComMocAndMom_loopRootClasses(MafOamSpiMocT* parentMoc,
													  OamSACache::DNList mo_name,
													  const std::string parentName,
													  const std::string className,
													  MafOamSpiMocT** theMoc_pp,
													  MafOamSpiMomT** theMom_pp)
{

	ENTER_OAMSA_TRANSLATIONS();
	MafOamSpiMomT* parentMom = NULL;
	//DEBUG_OAMSA_TRANSLATIONS("GetComMocAndMom_loopRootClasses(): enter with class [%s]", parentMoc->generalProperties.name);

	if (theMOMRepository.IsMocChildofEcimContribution_V1(parentMoc))
	{
		// ---------------
		// Step 3
		// Inspect each root class or each child of EcimContribution class
		// -------------------

		parentMom = parentMoc->mom;
		std::string theClassName = parentMoc->generalProperties.name;

		size_t pos;
		pos = theClassName.find('.');
		if (pos != std::string::npos)
		{
			// The name can be in the format Class.namingattribute=blopp
			theClassName.erase(0, pos + 1);
		}
		// OK, add on , here we need to check that the parent to the parentName (the root class name)
		// is equal to parentToMOCName, since the root not is unique any more.
		// The parent is named		: parentToMOCName
		// The root class is named	: theTempString
		// NOTE, this string becomes empty if it is called with ManagedElement
		std::list<std::string> theMocParentList = getParentName(parentMoc);
		if (theMocParentList.empty())
			theMocParentList.push_back("");
		std::list<std::string>::iterator theMocParentList_it;

		//DEBUG_OAMSA_TRANSLATIONS("GetComMocAndMom_loopRootClasses(): className [%s] / theClassName [%s] / parentName [%s]", className.c_str(), theClassName.c_str(), parentName.c_str());

		if (className==theClassName)
		{
			for (theMocParentList_it = theMocParentList.begin(); theMocParentList_it != theMocParentList.end(); ++theMocParentList_it)
			{
			 	// ----------------------
			 	// Step 4
			 	// When root class and a parent are found from MR SPI that match the results of Step1, process the class
			 	// ----------------------
				std::string										theTempString;
				OamSACache::DNListIterator  theIter;
				for (theIter = mo_name.begin(); theIter != mo_name.end(); ++theIter)
				{
					theTempString = convertDnToClass(*theIter);
					if (theTempString == className)
					{
						break;
					}
				}

				if ((*theMocParentList_it) == parentName)
				{
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom_loopRootClasses found root %s ", className.c_str());

					MafOamSpiContainmentT *theChild_p = parentMoc->childContainment;

					theIter++;
					if (theIter != mo_name.end() && theChild_p == NULL)
					{
						// Break if struct, otherwise the caller will recurse forever
						goto end_exit;
					}

					for (;theIter != mo_name.end() && theChild_p != NULL;)
					{
						// Now we found the root, chew along the model until we have found the entire chain
						theTempString = *theIter;
						theTempString = theTempString.substr(0, theTempString.find('='));
						if (theTempString.find('.') != std::string::npos)
						{
							theTempString = theTempString.erase(theTempString.find_first_of('.'));
						}
						//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom_loopRootClasses looking for %s ", theTempString.c_str());
						for (;;)
						{
							std::string childName = theChild_p->childMoc->generalProperties.name;
							size_t pos;
							pos = childName.find('.');
							if (pos != std::string::npos)
							{
								// The name could be in the format Class.namingattribute=blopp
								childName.erase(0, pos + 1);
							}
							//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom_loopRootClasses found child %s ", childName.c_str());
							if (childName == theTempString)
							{
								DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom_loopRootClasses found child %s breaking...", theTempString.c_str());
								// Found what we are looking for
								parentMoc = theChild_p->childMoc;
								parentMom = theChild_p->mom;
								break;
							}

							theChild_p = theChild_p->nextContainmentSameParentMoc;
							if (NULL == theChild_p)
							{
								//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom no match for %s exiting", theTempString.c_str());
								parentMom = NULL;
								goto end_exit;
							}
						}
						//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom after for(;;)...");
						theIter++;
						if (theIter != mo_name.end())
						{
							//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom after for(;;) iterrator not at end...");
							if (theChild_p->childMoc != NULL)
							{
								theChild_p = theChild_p->childMoc->childContainment;
								//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom after for(;;) updatning theChild_p %p", theChild_p);
								if (theChild_p == NULL)
								{
									//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom, chain broken before object found..:!!");
									parentMom = NULL;
									goto end_exit;
								}
							}
							else
							{
								// Strange error, chain broken before object in question found
								//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom, chain broken before object found..:!!");
								parentMom = NULL;
								goto end_exit;
							}
						}
					} // for
					if (theMoc_pp != NULL)
						*theMoc_pp = parentMoc;
					if (theMom_pp != NULL)
						*theMom_pp = parentMom;
					LEAVE_OAMSA_TRANSLATIONS();
					return;
				}

			}
		} // if

	}


	// Recursively call GetComMocAndMom_loopRootClasses() with next class.
	// First look for a child containment from this class
	// else look for the next child containment with same parent

	MafOamSpiContainmentT *currentContainment;
	MafOamSpiContainmentT *nextContainment;

	currentContainment = parentMoc->childContainment;

	if (NULL == currentContainment)
	{
		//DEBUG_OAMSA_TRANSLATIONS("GetComMocAndMom_loopRootClasses() could not get Child containment");
	}
	else
	{
		MafOamSpiMocT *nextMoc;
		MafOamSpiMocT *currentMoc = parentMoc;
		nextContainment = currentContainment;

		// Loop over all neighboring siblings with the same parent
		while (nextContainment != NULL )
		{
			nextMoc = currentContainment->childMoc;

			if (NULL != nextMoc)
			{
				//DEBUG_OAMSA_TRANSLATIONS("GetComMocAndMom_loopRootClasses() next child class [%s] current [%s]", nextMoc->generalProperties.name, currentMoc->generalProperties.name);
				if (nextMoc != currentMoc)
				{
					GetComMocAndMom_loopRootClasses(nextMoc, mo_name, parentName, className, theMoc_pp, theMom_pp);
				}

				//DEBUG_OAMSA_TRANSLATIONS("GetComMocAndMom_loopRootClasses()  Looking for next child containment with same parent");
				nextContainment = currentContainment->nextContainmentSameParentMoc;

				if (NULL != nextContainment)
				{
					currentContainment = nextContainment;
				}
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return ;

end_exit:
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComMocAndMom_loopRootClasses : leaves without any data found");
	if (theMoc_pp != NULL)
		*theMoc_pp = NULL;
	if (theMom_pp != NULL)
		*theMom_pp = parentMom;
	LEAVE_OAMSA_TRANSLATIONS();
	return;

}

/**
 * Convert3GPPNameFragmentToImmNameFragment
 *
 * Translates a 3GPP name fragment to an Imm Name fragment by
 * 1. Looking to see if the name contains a '.'. In that case remove the class name in front of the '.' , the rest is the RDN
 * 2. If there is no '.' then we must retrieve the key attribute name from the map that use the parent_classname as key
 * 2. Making certain that the last two characters before the equal (=) sign is "Id"
 */
std::string OamSATranslator::Convert3GPPNameFragmentToImmNameFragment(const std::string& the3GPPString)
{
	ENTER_OAMSA_TRANSLATIONS();
	size_t pos;
	size_t dotpos;
	std::string theString = the3GPPString;

	//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Convert3GPPNameFragmentToImmNameFragment : %s", the3GPPString.c_str());

	dotpos = theString.find('.');
	pos = theString.find('=');
	if(pos != std::string::npos && dotpos > pos)
		dotpos = std::string::npos;

	if (dotpos != std::string::npos)
	{
		// The name should be in the format Class.namingattribute=blopp
		theString.erase(0, dotpos + 1);
	}
	else
	{
		// If we end up here, the name is on the form :
		// 1. xxxId=blipp
		// OR
		// 2. id=blipp // Id=blipp
		// Check what we have

		// If the egual sign is not found it's very odd and we just ignore this
		if (pos != std::string::npos)
		{
			if ((pos == 2) &&
				((theString.substr(0, 2) == STRUCT_CLASS_KEY_ATTRIBUTE) || (theString.substr(0, 2) == "Id")))
			{
				// Case 2 here
				theString[0]=tolower(theString[0]);
			}
			else
			{
				// Case 1 here
				theString[0]=tolower(theString[0]);
				theString.insert(pos,"Id");
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();

	//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Convert3GPPNameFragmentToImmNameFragment : %s", theString.c_str());

	return theString;
}
/**
 * Convert3GPPNameFragmentToImmNameFragment
 *
 * Translates a 3GPP name fragment to an Imm Name fragment by
 * 1. Looking to see if the name contains a '.'. In that case remove the class name in front of the '.' , the rest is the RDN
 *	  We are done and return.
 * 2. If there is no '.' then we must retrieve the key attribute name from the map that use the parent_classname as key
 * 2. Making certain that the last two characters before the equal (=) sign is "Id"
 */
std::string OamSATranslator::Convert3GPPNameFragmentToImmNameFragment(const std::string& the3GPPString,
																	  OamSAKeyMOMRepo* momRepository)
{
	ENTER_OAMSA_TRANSLATIONS();
	size_t pos;
	size_t dotpos;
	std::string theString = the3GPPString;

	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Convert3GPPNameFragmentToImmNameFragment : %s", the3GPPString.c_str());
	try {
		// Be careful with string handling issues
		dotpos = theString.find('.');
		pos = theString.find('=');
		std::string valueString = theString.substr(pos);
		if(pos != std::string::npos && dotpos > pos)
			dotpos = std::string::npos;

		if (dotpos != std::string::npos)
		{
			// The name should be in the format Class.namingattribute=blopp
			// converted to namingattribute=blopp and that is the RDN format
			theString.erase(0, dotpos + 1);
		}
		else
		{
			// OK, this is the format Class=blopp
			// We need to fetch the key attribute from the model repository
			removeEqualSignAndDotPartOfDN(theString);
			//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Convert3GPPNameFragmentToImmNameFragment search with %s", theString.c_str());
			// Check if this is a struct fragment
			std::string theStringLower = ConvertToLowerCase(theString);
			if (theStringLower == STRUCT_CLASS_KEY_ATTRIBUTE)
			{
				// The struct case here
				theString[0]=tolower(theString[0]);
				theString = theString + valueString;
			}
			else
			{
				if (momRepository->classNameInMOM(theString))
				{
					// Get the key attribute and return
					theString = momRepository->getKeyAttributeForClass(theString);
					// OK, add the value part of the string, that is everything after '='
					theString = theString + valueString;
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Convert3GPPNameFragmentToImmNameFragment key attribute %s", theString.c_str());
				}
				else
				{
					// No key exists, file and error and return
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Convert3GPPNameFragmentToImmNameFragment ERROR NO Key attribute found");
				}
			}
		}
	} catch (...) {

	}
	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::Convert3GPPNameFragmentToImmNameFragment RETURNS %s", theString.c_str());
	return theString;
}


/**
 * ConvertImmNameFragmentTo3GPPNameFragment
 *
 * Translates a imm name fragment to an 3gpp Name fragment by
 *
 * 1. Making certain that the first character of the fragment is in upper case
 * 2. Removing any "Id" before the equal (=) sign.
 *
 */
std::string OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragment(const std::string& theImmString)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Check if t
	std::string theString = theImmString;
	if (theString != STRUCT_CLASS_KEY_ATTRIBUTE) // To handle structs, rather fishy ..... keep eyes on this
	{
		theString[0] = toupper(theString[0]);
		size_t pos = theString.find("Id");
		if (pos != std::string::npos)
		{
			theString.erase(pos,2);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return theString;
}

/**
 * ConvertImmNameFragmentTo3GPPNameFragment
 *
 * Translates a imm name fragment to an 3gpp Name fragment by
 *
 * 0. Check if the name is id, if so then assume that this is in fact a struct type.
 * 1. Use the transaction ID txHandlee to look up the class name from the IMM database.
 * Case a
 * The class name is equal to the naming attribute except for the lower and bigger case plus id at the end
 * 2. Replace everything before the = sign with the class name.
 * Case B
 * 3. I case of naming attribute not is equal to the class name plus id at the end, add this before the naming attribute together with a dot.
 *
 * If the method fails to translate according to the above algorithm, then it tries the best effort and translate according to legacy behaviour
 * and sets an warning in the SYSLOG
 *
 * Example Case A
 * Class name = MyClass
 * RDN : MYCLASSID=12
 * DN  : MyClass=12
 *
 * Example B:
 * Class name = MyClass
 * RDN : MAID=13
 * DN  : MyClass.MAID=13
 */
std::string OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragment(MafOamSpiTransactionHandleT txHandle,
																	  const std::string& theImmString,
																	  const std::string objectName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string theString = theImmString;
	try {
		std::string nameBeforeEqualSign = theString.substr(0, theString.find('='));
		// To handle structs, do not convert when a struct naming attribute appears
		// FIXME : Change this so we can handle structs as well, maybe...
		std::string nameBeforeEqualSignLowerLetters = ConvertToLowerCase(nameBeforeEqualSign);
		// Name without POSSIBLE id at the end
		std::string nameBeforeEqualSignLowerLettersNoID = nameBeforeEqualSignLowerLetters.erase(nameBeforeEqualSignLowerLetters.length()-2, 2);
		if ( nameBeforeEqualSignLowerLetters != STRUCT_CLASS_KEY_ATTRIBUTE )
		{
			// Get the class name from the IMM, one case is when we have access to the cache
			// the else case is when this code not have a cache handle.
			std::string className;
			if ((txHandle != 0))
			{
				std::tr1::shared_ptr<TxContext> txContextIn;
				txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
				if (NULL == txContextIn.get())
				{
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragment the context associated with handle %d was not found in the repository", (int)txHandle);
				}
				else
				{
					if (txContextIn->GetCache().GetClassNameFromImm(objectName, className))
					{
						// Build 3GPP name fragment
						// Check if this is case A or B
						if ( nameBeforeEqualSignLowerLettersNoID == ConvertToLowerCase(className) )
						{
							// Case A
							theString = className + theString.substr(theString.find('='));
						}
						else
						{
							// Case B
							theString = className + "." + nameBeforeEqualSign + theString.substr(theString.find('='));
						}
					}
					else
					{
						// OK, this is something that do not exist in the IMM, therefore we can not translate it
						// return it as it is.
						theString.clear();
						//theString = theImmString;
						// To correct the TR HR51069, we now revert the error behavior so that we do the following
						// 1. Change the first letter to a big letter
						// 2. remove any ID at the end of the string
						theString = ConvertImmNameFragmentTo3GPPNameFragmentErrorCase(theImmString);
					}
				}
			}
			else
			{
				// OK, call from OIProxy and other parts that do not have a cache prepared
				//HS30458: init immHandle in case Txhandle = 0 to avoid conflict immhandle/sub_immhandle among multi threads
				SaImmHandleT immOmHandleTxZero = 0;
				SaImmAccessorHandleT OamSAaccessorHandleTxZero = 0;
				if (initImmHandleForTxhandleZero(&immOmHandleTxZero, &OamSAaccessorHandleTxZero))
				{
					if (GetClassNameFromImm(objectName, className, immOmHandleTxZero, OamSAaccessorHandleTxZero))
					{
						// Check if this is case A or B
						if ( nameBeforeEqualSignLowerLettersNoID == ConvertToLowerCase(className) )
						{
							// Case A
							theString = className + theString.substr(theString.find('='));
						}
						else
						{
							// Case B
							theString = className + "." + nameBeforeEqualSign + theString.substr(theString.find('='));
						}
					}
					else
					{
						// OK, this is something that do not exist in the IMM, therefore we can not translate it
						// return it as it is.
						theString.clear();
						//theString = theImmString;
						// To correct the TR HR51069, we now revert the error behavior so that we do the following
						// 1. Change the first letter to a big letter
						// 2. remove any ID at the end of the string
						theString = ConvertImmNameFragmentTo3GPPNameFragmentErrorCase(theImmString);
					}
					finalizeImmHandleForTxhandleZero(immOmHandleTxZero, OamSAaccessorHandleTxZero);
				}
				else
				{
					ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragment: failed to init imm handle");
					// OK, this is something that do not exist in the IMM, therefore we can not translate it
					// return it as it is.
					theString.clear();
					//theString = theImmString;
					// To correct the TR HR51069, we now revert the error behavior so that we do the following
					// 1. Change the first letter to a big letter
					// 2. remove any ID at the end of the string
					theString = ConvertImmNameFragmentTo3GPPNameFragmentErrorCase(theImmString);
				}
			}
		}
	}catch (...) {
		// Someone calls us with a faulty formed string, no equal sign available...
		//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragment ERROR in string %s", theImmString.c_str());
		LEAVE_OAMSA_TRANSLATIONS();
		return theImmString;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragment returns %s", theString.c_str());
	return theString;
}

std::string OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragmentWoMomPre(MafOamSpiTransactionHandleT txHandle,
																			  const std::string& theImmString,
																			  const std::string objectName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string theString;
	// This flag is valid only for the MOM root DN, and when it has been check it is toggled to false
	bool checkForTheRootOnly = true;
	// USed to indicate if the MOM is decorated
	bool decoratedRoot = false;
	std::string converted_root_dn;
	OamSACache::DNListReverseIterator theDNRevIter;

	OamSACache::DNList theImmObjectList;
	GlobalSplitDN(objectName, theImmObjectList);

	// This value will contain the position where the prefix ends of the class name.
	size_t positionOfEndOfDecoration = 0;
	for (theDNRevIter = theImmObjectList.rbegin();(theDNRevIter != theImmObjectList.rend()) && ((*theDNRevIter).length()!=0);)
	{
		// But here we need to translate the root class that is prefix.
		if (checkForTheRootOnly)
		{
			converted_root_dn.clear();
			if (unDecorateRootDn(*theDNRevIter, converted_root_dn))
			{
				decoratedRoot = true;
				positionOfEndOfDecoration = (*theDNRevIter).find(converted_root_dn.c_str());
				if ( (*theDNRevIter).compare(theImmObjectList.front()) == 0 )
				{
					std::string temporaryNameFragment = ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																								 converted_root_dn,
																								 objectName);
					// Here we have one of two cases.
					// case 1: temporaryNameFragment = "className=99"
					// In this case nothing more is needed
					// OR
					// case 2: temporaryNameFragment = "classNameDecorated.blopp=99"
					// In this case we have to convert the classNameDecorated TO className AND
					// check if this new classname means that we need to add className.blopp=99 to the 3GPP name
					// OR className=99
					// And continue do so for all coming parts of the RDN.
					size_t positionOfDot= temporaryNameFragment.find_first_of('.');
					if (positionOfDot!=std::string::npos)
					{
						// Case 2
						std::string classNameDecorated = temporaryNameFragment.substr(0, positionOfDot);
						std::string nameAfterDot = temporaryNameFragment.substr(positionOfDot, temporaryNameFragment.length());
						// Find the decoration by removing converted key attribute from decorated key attribute.
						bool removeEveryThingAfterDot = false;
						try {
							// Remove the decorated part from the classNameDecorated
							classNameDecorated.erase(0, positionOfEndOfDecoration);
							// Now with the new fragment looking like this ClassName.blopp we must ensure that
							// we do not have case 1 active again after this translation.
							// REmove value part of nameAfterDot and check if we need to remove everything after the dot.
							std::string temporaryValue = nameAfterDot;
							// OK, there is a dot left here that we do not want, remove it.
							temporaryValue.erase(0,1);
							removeEqualSignAndDotPartOfDN(temporaryValue);
							std::string keyAttributeName = ConvertToLowerCase(temporaryValue);
							// REmove Id at the end, if there is one
							if (keyAttributeName.length() > 2)
								keyAttributeName.erase(keyAttributeName.length()-2, 2);
							if (ConvertToLowerCase(classNameDecorated) == keyAttributeName)
							{
								removeEveryThingAfterDot = true;
							}
							// Now build the 3GPP name
							if (removeEveryThingAfterDot)
							{
								std::string valuePart = nameAfterDot;
								if (nameAfterDot.find_first_of('=') != std::string::npos)
								{
									valuePart = nameAfterDot.substr(nameAfterDot.find_first_of('='));
								}
								theString = theString + classNameDecorated + valuePart;
							}
							else
							{
								theString = theString + classNameDecorated + nameAfterDot;
							}
						}catch(...) {
							// Ops here we have something unexpected
							ERR_OAMSA_TRANSLATIONS("Unexpected COM_SA ERROR in OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragmentWoMomPre");
						}
					}
					else
					{
						// Case 1
						theString = theString + temporaryNameFragment;
					}
				}
				++theDNRevIter;
				converted_root_dn.clear();
				if ((theDNRevIter != theImmObjectList.rend()) && (unDecorateRootDn(*theDNRevIter, converted_root_dn)))
				{
					checkForTheRootOnly = true;
				}
				else
				{
					checkForTheRootOnly = false;
				}
			}
			else
			{
				if ( (*theDNRevIter).compare(theImmObjectList.front()) == 0 )
				{
					theString = theString + ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																					 *theDNRevIter,
																					 objectName);
				}
				++theDNRevIter;
				converted_root_dn.clear();
				if ((theDNRevIter != theImmObjectList.rend()) && (unDecorateRootDn(*theDNRevIter, converted_root_dn)))
				{
					checkForTheRootOnly = true;
				}
				else
				{
					checkForTheRootOnly = false;
				}
			}
		}
		else
		{
			// Normal case
			if (decoratedRoot)
			{
				if ( (*theDNRevIter).compare(theImmObjectList.front()) == 0 )
				{
					// Check if we need to remove the decoration from the translation
					std::string temporaryNameFragment = ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																								 *theDNRevIter,
																								 objectName);
					size_t positionOfDot= temporaryNameFragment.find_first_of('.');
					if (positionOfDot!=std::string::npos)
					{
						std::string nameAfterDot = temporaryNameFragment.substr(positionOfDot, temporaryNameFragment.length());
						std::string classNameDecorated;
						try {
							classNameDecorated = temporaryNameFragment.substr(0, positionOfDot);
							// Remove the decorated part from the classNameDecorated
							classNameDecorated.erase(0, positionOfEndOfDecoration);
						}catch(...) {
							// Ops here we have something unexpected
							ERR_OAMSA_TRANSLATIONS("Unexpected COM_SA ERROR in OamSATranslator::Imm2MO_DN");
						}
						size_t positionOfEqual = temporaryNameFragment.find_first_of('=');
						std::string nameAfterEqual = temporaryNameFragment.substr(positionOfEqual, temporaryNameFragment.length());
						std::string stringId = "";
						if (positionOfEqual - positionOfDot == 3)
						{
							stringId = temporaryNameFragment.substr(positionOfDot + 1, 2);
							DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragmentWoMomPre stringId %s",stringId.c_str());
						}
						if (stringId != "" && stringId == STRUCT_CLASS_KEY_ATTRIBUTE)
							theString = theString + classNameDecorated + nameAfterDot;
						else
							theString = theString + classNameDecorated + nameAfterEqual;
					}
					else
					{
						theString = theString + temporaryNameFragment;
					}
				}

				++theDNRevIter;
				// TR HR94824 fix .
				converted_root_dn.clear();
				if ((theDNRevIter != theImmObjectList.rend()) && (unDecorateRootDn(*theDNRevIter, converted_root_dn)))
				{
					checkForTheRootOnly = true;
				}
			}
			else
			{
				if ( (*theDNRevIter).compare(theImmObjectList.front()) == 0 )
				{
					theString = theString + ConvertImmNameFragmentTo3GPPNameFragment(txHandle,
																					 *theDNRevIter,
																					 objectName);
				}

				++theDNRevIter;
				// TR HR94824 fix .
				converted_root_dn.clear();
				if ((theDNRevIter != theImmObjectList.rend()) && (unDecorateRootDn(*theDNRevIter, converted_root_dn)))
				{
					checkForTheRootOnly = true;
				}
			}
		}
	}

	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragmentWoMomPre returns %s", theString.c_str());
	return theString;
}

/**
 * ConvertImmNameFragmentTo3GPPNameFragmentErrorCase
 * If the translation fails, this method translates
 * a imm name fragment to an 3gpp Name fragment by
 *
 * 1. Making certain that the first character of the fragment is in upper case
 * 2. Removing any "Id" before the equal (=) sign.
 * 3. Set an WARNING in the SYSLOG for the translated imm name fragment. From TR HT28548: Don't set an ERROR in the SYSLOG for the translated imm name fragment.
 */
std::string OamSATranslator::ConvertImmNameFragmentTo3GPPNameFragmentErrorCase(const std::string& theImmString)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string theString = theImmString;
	if ((theString != STRUCT_CLASS_KEY_ATTRIBUTE)&&(theString.substr(0,3) != "id=")) // To handle structs
	{
		theString[0] = toupper(theString[0]);
		size_t pos = theString.find("Id=");
		if (pos != std::string::npos)
		{
			theString.erase(pos,2);
		}
	}
	// Set an ERROR in the SYSLOG. From TR HT28548: Don't set an ERROR in the SYSLOG.
	DEBUG_OAMSA_TRANSLATIONS("WARNING Received IMM relative distinguished name to not existing IMM object <%s>", theString.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return theString;
}

/**
 * Get Class name for a IMM object
 *
 * @param	objectName	 Name of Object the class name is required
 * @param	className	 Return parameter containing the class name of
 *						 the Object.
 * @return true if the class Name exists, otherwise false
 *
 */
bool OamSATranslator::GetClassNameFromImm(const std::string& objectName, std::string& className, SaImmHandleT immHandle, SaImmAccessorHandleT accessorHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	static const char* const ClassNameAttribute = "SaImmAttrClassName";
	static const char* const AttributeArray[2] = {ClassNameAttribute, NULL};
	bool RetVal = false;

	SaNameT					theObjectName;
	SaImmAttrValuesT_2**	theAttrVals = NULL;

	saNameSet(objectName.data(), &theObjectName);
	if (accessorHandle != 0)
	{
		// Access the IMM
		SaAisErrorT err = saImmOmAccessorGet_2(accessorHandle,
												&theObjectName,
												(char**)AttributeArray,
												&theAttrVals);
		//DEBUG_OAMSA_TRANSLATIONS("saImmOmAccessorGet_2 returns <%i>", err);
		if(SA_AIS_ERR_BAD_HANDLE == err)
		{
			LOG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassNameFromImm returns SA_AIS_ERR_BAD_HANDLE, hence re-initializing handlers");
			if((err = reInitializeBadHandler(immHandle, accessorHandle)) == SA_AIS_OK)
			{
				LOG_OAMSA_TRANSLATIONS("reInitializeBadHandler returned SUCCESS, calling saImmOmAccessorGet_2 again with new handlers");
				err = saImmOmAccessorGet_2(accessorHandle,
						&theObjectName,
						(char**)AttributeArray,
						&theAttrVals);
				LOG_OAMSA_TRANSLATIONS("saImmOmAccessorGet_2, called after re-initializing handlers, returned retCode=%d",err);
				RetVal = (err == SA_AIS_OK) ? true : false;
			}
			else
			{
				LOG_OAMSA_TRANSLATIONS("reInitializeBadHandler FAILED and returned retCode=%d",err);
				RetVal = false;
			}
		}

		if (SA_AIS_OK == err)
		{
			if(theAttrVals != NULL)
			{
				if(theAttrVals[0] != NULL)
				{
					if(theAttrVals[0]->attrValuesNumber != 0)
					{
						if (theAttrVals[0]->attrValueType == SA_IMM_ATTR_SASTRINGT)
						{
							char**		name_p = (char**) theAttrVals[0]->attrValues[0];
							className	= *name_p;
							RetVal		= true;
						}
						else if (theAttrVals[0]->attrValueType == SA_IMM_ATTR_SANAMET)
						{
							className	 = "";
							SaNameT* ntp = (SaNameT*)theAttrVals[0]->attrValues[0];
							className.append(saNameGet(ntp), saNameLen(ntp));
							RetVal		 = true;
						}
					}
					else
					{
						ERR_OAMSA_TRANSLATIONS("Failed to get class description by saImmOmAccessorGet_2()");
					}
				}
				else
				{
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassNameFromImm(): saImmOmAccessorGet_2() returned SA_AIS_OK with empty list");
				}
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassNameFromImm(): saImmOmAccessorGet_2() returned attributes = NULL");
			}
		}
	}
	saNameDelete(&theObjectName, false);
	LEAVE_OAMSA_TRANSLATIONS();
	//DEBUG_OAMSA_TRANSLATIONS("GetClassNameFromImm returns <%i> ", RetVal);
	return RetVal;
}


std::string OamSATranslator::RemoveClassPart(const std::string& theDn)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string theString = theDn;
	size_t dotpos, equalpos;
	dotpos = theString.find('.');
	equalpos = theString.find('=');

	if (equalpos != std::string::npos && dotpos > equalpos)
		dotpos = std::string::npos;
	if (dotpos != std::string::npos)
	{
		// The name should be in the format Class.namingattribute=blopp
		theString.erase(0, dotpos + 1);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return theString;
}

char* OamSATranslator::BuildAttributeValue(const char* theAttributeName, const char* theAttributeValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string NameStr(theAttributeName);
	std::string ValueStr(theAttributeValue);
	char* ret_p = NULL;

	size_t equalpos = ValueStr.find('=');

	if (equalpos == std::string::npos)
	{
		// No equal sign at all. Build it all from all of the strings
		ValueStr = NameStr + "=" + ValueStr;
	}
	else if (equalpos == 0)
	{
		// value starts with =, add name in front
		ValueStr = NameStr + ValueStr;
	}
	// If neither of the two above do no concatenation
	ret_p = new char[ValueStr.length()+1];
	strcpy(ret_p, ValueStr.c_str());
	printf("Build attribute value %s\n", ret_p);
	LEAVE_OAMSA_TRANSLATIONS();
	return ret_p;
}

bool OamSATranslator::IsImmRoot(const std::string& className, const std::string& undecoratedclassName, const std::string &parentDn)
{
	ENTER_OAMSA_TRANSLATIONS();

	// Step1 - verify that this is a normal class with parent containment split=true, before searching the map.
	// If it has split=false, return.

	// This is done to handle scenarios where there are 2 or more parents, one with split=true, the other where split=false,
	// which will result in a decorated classname being added to the root map, causing confusion when calculating unique IMM names and
	// interfacing with IMM.
	// In order not to confuse the 2 instances of a child, one which is written with decorated class name as a root (split = true)
	// and the other which is written to IMM underneath a parent (split = false),  need to inspect the parent as well as the class name.

	OamSACache::DNList theSplitDN;
	GlobalSplitDN(parentDn,theSplitDN);
	OamSACache::DNListReverseIterator	theDNIter;
	std::string TheParentName;

	theDNIter = theSplitDN.rbegin();
	TheParentName = *theDNIter;

	if (theMOMRepository.IsSingletonRoot(undecoratedclassName, TheParentName))
	{
		DEBUG_OAMSA_TRANSLATIONS("IsImmRoot: Class is a singleton root, searching decorated classname in map");
		//Step2 - check if the decorated root class is found in the map.
		MOMRootRepository::MOMRootDataEntryMapRetVal	theMOMRetVal;
		theMOMRetVal = theMOMRepository.Find(className);
		LEAVE_OAMSA_TRANSLATIONS();
		return theMOMRetVal.second;
	}

	DEBUG_OAMSA_TRANSLATIONS("IsImmRoot: Class is not a singleton root, returning false");
	LEAVE_OAMSA_TRANSLATIONS();
	return false;

}


/*
 *
 *	Get the attribute type for an attribute. This name must always be MOM name, the IMM name is sometimes not the same as in the MOM.
 *
 */
MafOamSpiMoAttributeType_3 OamSATranslator::GetComAttributeType(OamSACache::DNList& mo_name, const std::string& AttrName)
{
	ENTER_OAMSA_TRANSLATIONS();
	//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComAttributeType - attr name <%s>", AttrName.c_str());
	MafOamSpiMoAttributeType_3 retVal = (MafOamSpiMoAttributeType_3)0; // 0 is not part of the ENUM.
	for (MafOamSpiMoAttributeT* ap = GetComAttributeList(mo_name); ap != NULL; ap = ap->next)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComAttributeType, attr list name %s",ap->generalProperties.name);
		if (!strcmp(ap->generalProperties.name, AttrName.c_str()))
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComAttributeType, attr name found %s ap->type = %d",AttrName.c_str(), ap->type);
			if(ap->type != MafOamSpiMoAttributeType_DERIVED)
			{
				retVal = (MafOamSpiMoAttributeType_3)ap->type;
			}
			else
			{
				retVal = (MafOamSpiMoAttributeType_3)ap->derivedDatatype->type;
			}
			LEAVE_OAMSA_TRANSLATIONS();
			return retVal;
		} // if
	} // for
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

bool OamSATranslator::isExclusiveStruct (const std::string& dn, const char* structName)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool isExclusive = true; //can't risk unwanted modifications!
	MafReturnT maf_retval;
	MafOamSpiMrMocHandle_4T mocHandle;
	MafOamSpiMrAttributeHandle_4T attrHandle, nextAttrHandle;
	MafOamSpiMrTypeContainerHandle_4T attrTpH;
	MafOamSpiMrGeneralPropertiesHandle_4T attrGpH;


	maf_retval = theModelRepo_v4_p->entry->getMocFromDn(dn.c_str(), false, &mocHandle);
	if ( MafOk == maf_retval) {
		maf_retval = theModelRepo_v4_p->moc->getAttribute(mocHandle, &attrHandle);
		while ( MafOk == maf_retval ) {
			maf_retval = theModelRepo_v4_p->attribute->getGeneralProperties(attrHandle, &attrGpH);
			if (maf_retval == MafOk) {
				const char* name = NULL;
				maf_retval = theModelRepo_v4_p->generalProperties->getStringProperty(attrGpH, MafOamSpiMrGeneralProperty_name_4, &name);
				if (maf_retval == MafOk) {
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isExclusiveStruct getStringProperty Name:%s", name);
					if (!strcmp(structName,name)) {
						maf_retval = theModelRepo_v4_p->attribute->getTypeContainer(attrHandle, &attrTpH);
						if (maf_retval == MafOk) {
							maf_retval = theModelRepo_v4_p->structType->getBoolProperty(attrTpH, MafOamSpiMrStructBoolProperty_isExclusive_4, &isExclusive);
							if (maf_retval == MafOk) {
								DEBUG_OAMSA_TRANSLATIONS(">>>getBoolProperty returned %d",isExclusive);
							}
						}
						break;
					}
				}
			}
			maf_retval = theModelRepo_v4_p->attribute->getNext(attrHandle, &nextAttrHandle);
			attrHandle = nextAttrHandle;
		}
	}
	return isExclusive;
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * This function will accept the DN in 3GPP format and the name of an attribute and will
 * return true if the attributes type is a struct.
 */
bool OamSATranslator::isStructAttribute(OamSACache::DNList& dn, const std::string& attributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return (GetComAttributeType(dn, attributeName) == MafOamSpiMoAttributeType_3_STRUCT);
}


/**
 * Retrieve a list of the members in a com struct attribute
 */
std::list<std::string> OamSATranslator::getStructMembers(OamSACache::DNList& dn, const std::string& attributeName){
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string> res;
	for (MafOamSpiMoAttributeT* ap = GetComAttributeList(dn); ap != NULL; ap = ap->next)
	{
		////DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetComAttributeType looking at attribute %s",ap->generalProperties.name);
		if (!strcmp(ap->generalProperties.name, attributeName.c_str()))
		{
			if(ap->type == MafOamSpiMoAttributeType_STRUCT && ap->structDatatype != NULL){
				// if correct type then traverse members and build a list!
				for(MafOamSpiStructMemberT* member=ap->structDatatype->members; member != NULL; member=member->next){
					std::string attrName(member->generalProperties.name);
					res.push_back(attrName);
				}
				// done!
				goto end_exit;
			}
		}
	}
end_exit:
	LEAVE_OAMSA_TRANSLATIONS();
	return res;
}


/**
 * Find the ECIM type for a structure member
 * result is written into the res value
 * returns true if member was found, false if not found
 */
bool OamSATranslator::getStructMemberComDatatype(OamSACache::DNList& dn,
												 const std::string& structAttributeName,
												 const std::string& structMemberName,
												 MafOamSpiMoAttributeType_3T* res)
{
	ENTER_OAMSA_TRANSLATIONS();
	for (MafOamSpiMoAttributeT* ap = GetComAttributeList(dn); ap != NULL; ap = ap->next)
	{
		if (strcmp(ap->generalProperties.name, structAttributeName.c_str()) == 0)
		{
			if(ap->type == MafOamSpiMoAttributeType_STRUCT && ap->structDatatype != NULL){
				for(MafOamSpiStructMemberT* member=ap->structDatatype->members; member != NULL; member=member->next){
					if(strcmp(member->generalProperties.name, structMemberName.c_str()) == 0){
						// TODO here we need to cast since the datatypes are dirrefent enums for attributes and members.
						//		if COM decides to change this we can perform conversion here!
						if (member->memberType.type != MafOamSpiDatatype_DERIVED) {
							*res = (MafOamSpiMoAttributeType_3T)member->memberType.type;
						}
						else {
							*res = (MafOamSpiMoAttributeType_3T)member->memberType.derivedDatatype->type;
						}
						//DEBUG_OAMSA_TRANSLATIONS("Data type of member %s is %d",member->generalProperties.name,(MafOamSpiMoAttributeType_3T)member->memberType.type);
						LEAVE_OAMSA_TRANSLATIONS();
						return true;
					}
				}
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}


/**
 * Lookup the name of the class for an attribute.
 */
std::string OamSATranslator::findAttributeTypeName(OamSACache::DNList& dn, const std::string& attributeName){
	ENTER_OAMSA_TRANSLATIONS();
	std::string res;

	for (MafOamSpiMoAttributeT* ap = GetComAttributeList(dn); ap!=NULL; ap=ap->next)
	{
		////DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::findAttributeTypeName looking at attribute %s",ap->generalProperties.name);
		if (!strcmp(ap->generalProperties.name, attributeName.c_str()))
		{
			switch(ap->type){
			case MafOamSpiMoAttributeType_STRUCT:
			{
				if(ap->structDatatype != NULL){
					res = std::string(ap->structDatatype->generalProperties.name);
					goto end_exit;
				}
				break;
			}
			case MafOamSpiMoAttributeType_DERIVED:
			{
				WARN_OAMSA_TRANSLATIONS("WARNING: OamSATranslator::findAttributeTypeName: MafOamSpiMoAttributeType_DERIVED: not supported in MAF MO SPI v3");
				if(ap->derivedDatatype != NULL){
					res = std::string(ap->derivedDatatype->generalProperties.name);
					goto end_exit;
				}
				break;
			}
			case MafOamSpiMoAttributeType_REFERENCE:
			{
				if(ap->referencedMoc != NULL){
					res = std::string(ap->referencedMoc->generalProperties.name);
					goto end_exit;
				}
				break;
			}
			case MafOamSpiMoAttributeType_ENUM:
			{
				if(ap->enumDatatype != NULL){
					res = std::string(ap->enumDatatype->generalProperties.name);
					goto end_exit;
				}
				break;
			}

			default:
			{
				// all other types are "basic" types and have no class name
				res = std::string("");
				goto end_exit;
			}

			}
		} // if
	} // for

end_exit:
	////DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::findAttributeTypeName attribute found %s, typeName='%s'",ap->generalProperties.name, res.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return res;
}


SaImmValueTypeT OamSATranslator::ConvertComAttributeTypeToImmType(MafOamSpiMoAttributeType_3T type)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaImmValueTypeT RetVal = (SaImmValueTypeT)0;

	switch (type)
	{
	case MafOamSpiMoAttributeType_3_UINT8:
	case MafOamSpiMoAttributeType_3_UINT16:
	case MafOamSpiMoAttributeType_3_UINT32:
	case MafOamSpiMoAttributeType_3_BOOL:
	case MafOamSpiMoAttributeType_3_ENUM:
		RetVal = SA_IMM_ATTR_SAUINT32T;
		break;

	case MafOamSpiMoAttributeType_3_INT8:
	case MafOamSpiMoAttributeType_3_INT16:
	case MafOamSpiMoAttributeType_3_INT32:
		RetVal = SA_IMM_ATTR_SAINT32T;
		break;

	case MafOamSpiMoAttributeType_3_UINT64:
		RetVal = SA_IMM_ATTR_SAUINT64T;
		break;

	case MafOamSpiMoAttributeType_3_INT64:
		RetVal = SA_IMM_ATTR_SAINT64T;
		break;

	case MafOamSpiMoAttributeType_3_STRING:
		RetVal = SA_IMM_ATTR_SASTRINGT;
		break;

	case MafOamSpiMoAttributeType_3_REFERENCE:
		RetVal = SA_IMM_ATTR_SANAMET;
		break;
	case MafOamSpiMoAttributeType_3_DECIMAL64:
		RetVal = SA_IMM_ATTR_SADOUBLET;
		break;
	default:
		ERR_OAMSA_TRANSLATIONS("ERROR: Unknown MafOamSpiMoAttributeType_3: %d", type);
		break;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}


/*
 *	TransformImmClassName
 *
 *	To keep IMM class names unique. the class names should be prefixed with the name of the MOM where the class
 *	is defined. An exception is made if the unaltered class name is found in IMM, when it is assumed that it is
 *	an existing class which for backwards compatibility should be left as is,
 *
 */
std::string	 OamSATranslator::TransformImmClassName(OamSACache::DNList& ParentName,
													const  std::string& ClassName)
{
	ENTER_OAMSA_TRANSLATIONS();
	//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::TransformImmClassName : enter");
	std::string thePrefixMom("");
	// Check if we have an alternative name for the class in IMM.
	// First find our root class
	OamSACache::DNListReverseIterator				theIter;
	OamSACache::DNListReverseIterator				theIterOneBefore;
	MOMRootRepository::MOMRootDataEntryMapRetVal	theMOMRetVal;
	std::string TheRootName = "";
	std::string TheParentName = "";
	theIterOneBefore = ParentName.rbegin();
	++theIterOneBefore;
	// Search for the root class name
	if (theIterOneBefore != ParentName.rend())
	{
		for (theIter = ParentName.rbegin(); theIter != ParentName.rend(); ++theIter)
		{
			// In the first loop if we get a hit
			if (theIterOneBefore != ParentName.rend())
			{
				//DEBUG_OAMSA_TRANSLATIONS("Iterate over list %s / %s", (*theIterOneBefore).c_str(), (*theIter).c_str());
				if (theMOMRepository.IsRoot(*theIter, *theIterOneBefore))
				{
					// This was the root, so let's leave. Our work is done
					TheRootName = *theIter;
					TheParentName = *theIterOneBefore;
					break;
				}
				++theIterOneBefore;
			}else
			{
				// OK, we have reached the end of the list and found no root, return no decoration
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::TransformImmClassName -- NewClassName = %s",ClassName.c_str());
				return ClassName;
			}
		}
	}else
	{
		// OK, we have empty string or ManagedElement
		DEBUG_OAMSA_TRANSLATIONS("MANAGEDELEMENT EXPECTED %s", (*(ParentName.rbegin())).c_str() );
		TheRootName = *(ParentName.rbegin());
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::TransformImmClassName : parent = %s, rootname = %s", TheParentName.c_str(), TheRootName.c_str());
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::TransformImmClassName -- ClassName = %s",ClassName.c_str());
	// Then find out if this calls needs to be prefix
	// First we need the MOM class name, remove everything after equal sign
	std::string theSearchStrParent = TheParentName.substr(0, TheParentName.find('='));
	std::string theSearchStrRootName = TheRootName.substr(0, TheRootName.find('='));
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::TransformImmClassName calls with : parent = %s, rootname = %s", theSearchStrParent.c_str(), theSearchStrRootName.c_str());
	if (theMOMRepository.FindIfRootMOMIsDecorated(theSearchStrRootName, theSearchStrParent))
	{
                theMOMRepository.FindDecorationForMOMDN(TheRootName, TheParentName, thePrefixMom);
		std::string temp = thePrefixMom + ClassName;
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::TransformImmClassName -- NewClassName = %s",temp.c_str());
		LEAVE_OAMSA_TRANSLATIONS();
		return thePrefixMom + ClassName;
	}else
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::TransformImmClassName -- NewClassName = %s",ClassName.c_str());
		LEAVE_OAMSA_TRANSLATIONS();
		return ClassName;
	}
}

MafReturnT	OamSATranslator::ConvertToAdminOpParameters(MafOamSpiMoActionT				*theAction_p,
														MafMoAttributeValueContainer_3T **theComParameterList_pp,
														SaImmAdminOperationParamsT_2	***theAdminOpParams_ppp)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	RetVal = MafOk;

	// Parameter check
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertToAdminOpParameters() entered");
	if ((NULL == theAction_p) || (NULL == theComParameterList_pp) || (NULL == theAdminOpParams_ppp))  {
		ERR_OAMSA_TRANSLATIONS("ConvertToAdminOpParameters: NULL parameters, returning ComInvalidArgument");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}

	// Loop through the parameters in the Action to create an apropriate list
	MafOamSpiParameterT*			  par_p			= theAction_p->parameters;
	MafMoAttributeValueContainer_3T*  AttrValC_p	= *theComParameterList_pp;
	OamSATransAdmOpParList			  theAdmOpParList;

	if (NULL == par_p)	{
		ERR_OAMSA_TRANSLATIONS("ConvertToAdminOpParameters: NULL par_p, returning ComInvalidArgument");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}

	if (NULL == AttrValC_p)	 {
		ERR_OAMSA_TRANSLATIONS("ConvertToAdminOpParameters: NULL AttrValC_p, returning ComInvalidArgument");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}

	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertToAdminOpParameters() number of values %d", AttrValC_p->nrOfValues);

	for (;par_p != NULL && (AttrValC_p = *theComParameterList_pp) != NULL; par_p = par_p->next, theComParameterList_pp++)
	{
		if (sanityCheckAdminParameters(par_p, (MafOamSpiDatatypeT)AttrValC_p->type))
		{
			for(unsigned int i = 0; i < AttrValC_p->nrOfValues; i++ )
			{
				std::string ParamName(par_p->generalProperties.name);

				// if number of elements > 1 this is an array, so append [i] to the name

				if (AttrValC_p->nrOfValues > 1)
				{
					char	numbuf[32];
					sprintf(numbuf,"%u", i + 1);
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertToAdminOpParameters() numbuf is : %s", numbuf);
					std::string str(numbuf);
					ParamName += '_' + str;
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertToAdminOpParameters() simple multivalue parameter: %s", ParamName.c_str());
				}
				if (par_p->parameterType.type == MafOamSpiDatatype_STRUCT)
				{
					// If this is a struct handle it in a special way.
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertToAdminOpParameters(): MafOamSpiDatatype_STRUCT");
					RetVal = (MafReturnT) HandleStructToAdminOpPar(ParamName,par_p->parameterType.structDatatype->members,theAdmOpParList,&(AttrValC_p->values[i]) );
				}
				else
				{
					// Convert a single parameter
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertToAdminOpParameters(): single value of type %d",par_p->parameterType.type);
					SaImmAdminOperationParamsT_2*	AdmOpParam_p = ConvertASingleAdmOpParam(ParamName, (MafOamSpiDatatypeContainerT*)&par_p->parameterType, AttrValC_p, i);
					if (AdmOpParam_p != NULL)
						theAdmOpParList.push_back(AdmOpParam_p);
				}
			} // for i
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("Type mismatch in Action =\"%s\" Parameter=\"%s\" Action param type=%d passed type=%d", theAction_p->generalProperties.name,
				  par_p->generalProperties.name,
				  par_p->parameterType.type,
				  AttrValC_p->type);
			RetVal = MafValidationFailed;
			break;
		}
	} // end for par_p

	// If everything is OK, copy it all to the result array
	if (MafOk == RetVal)
	{
		SaImmAdminOperationParamsT_2** theAdminOpParams_pp = new  SaImmAdminOperationParamsT_2*[theAdmOpParList.size() + 1];
		int i = 0;
		for (OamSATransAdmOpParListIter iter = theAdmOpParList.begin(); iter != theAdmOpParList.end(); iter++)
			theAdminOpParams_pp[i++] = *iter;
		theAdminOpParams_pp[i] = NULL;
		*theAdminOpParams_ppp = theAdminOpParams_pp;
	}
	else
	{
		// Fix memory leaks: cleanup theAdmOpParList before assign NULL to theAdminOpParams_ppp.
		if(theAdmOpParList.size() != 0)
		{
			for (OamSATransAdmOpParListIter iter = theAdmOpParList.begin(); iter != theAdmOpParList.end(); iter++)
			{
				SaImmAdminOperationParamsT_2* p = *iter;
				if (p)
				{
					delete []p->paramName;
					switch (p->paramType)
					{
					case SA_IMM_ATTR_SAINT32T:
						delete (SaInt32T*) p->paramBuffer;
						break;
					case SA_IMM_ATTR_SAINT64T:
						delete (SaInt64T*) p->paramBuffer;
						break;
					case SA_IMM_ATTR_SAUINT32T:
						delete (SaUint32T*) p->paramBuffer;
						break;
					case SA_IMM_ATTR_SAUINT64T:
						delete (SaUint64T*) p->paramBuffer;
						break;
					case SA_IMM_ATTR_SASTRINGT:
					{
						char* cptr = *(char**)(p->paramBuffer);
						delete [] cptr;
						delete (char*)p->paramBuffer;
					}
						break;
					case SA_IMM_ATTR_SAFLOATT:
						delete (SaFloatT*) p->paramBuffer;
						break;
					case SA_IMM_ATTR_SADOUBLET:
						delete (SaDoubleT*) p->paramBuffer;
						break;
					case SA_IMM_ATTR_SANAMET:
						saNameDelete((SaNameT*) p->paramBuffer, true);
						break;
					default:
						// Should never end up here.
						ERR_OAMSA_TRANSLATIONS("OamSAImmBridge::ConvertToAdminOpParameters(): ERROR: Unknown SA_IMM_ATTR_ paramType: %d", p->paramType);
						break;
					}
					delete p;
				}
			}
		}
		*theAdminOpParams_ppp = NULL;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}


//SDP872: Called by OIProxy to convert moIf->action() return parameters from COM to IMM format before delivering to AdminOperationResult().
MafReturnT OamSATranslator::ConvertActionReturnToImmParameters(MafOamSpiMoActionT*               theAction_p,
                                                                MafMoAttributeValueContainer_3T** theComParameterList_pp,
                                                                SaImmAdminOperationParamsT_2***   theAdminOpParams_ppp,
                                                                bool                              isMoSpiVersion2)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	RetVal = MafOk;

	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertActionReturnToImmParameters() entered");
	// Parameter check
	if (NULL == theAction_p)
	{
		ERR_OAMSA_TRANSLATIONS("ConvertActionReturnToImmParameters: NULL theAction_p, returning ComInvalidArgument");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}
	if (NULL == theComParameterList_pp)
	{
		ERR_OAMSA_TRANSLATIONS("ConvertActionReturnToImmParameters: NULL COM parameter list, returning ComInvalidArgument");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}
	if (NULL == theAdminOpParams_ppp)
	{
		ERR_OAMSA_TRANSLATIONS("ConvertActionReturnToImmParameters: NULL Admin Oper Parameters, returning ComInvalidArgument");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}

	// Loop through the parameters in the Action to create an apropriate list

	MafOamSpiDatatypeContainerT		  retPar_p	 = theAction_p->returnType;
	MafMoAttributeValueContainer_3T*  AttrValC_p = *theComParameterList_pp;
	OamSATransAdmOpParList			  theAdmOpParList;

	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertActionReturnToImmParameters() number of values %d", AttrValC_p->nrOfValues);

	for (;(AttrValC_p = *theComParameterList_pp) != NULL; theComParameterList_pp++)
	{
		if (sanityCheckReturnComParameters(retPar_p, (MafOamSpiDatatypeT)AttrValC_p->type, isMoSpiVersion2))
		{
			for(unsigned int i = 0; i < AttrValC_p->nrOfValues; i++ )
			{
				if ((retPar_p.type == MafOamSpiDatatype_VOID) && isMoSpiVersion2)
				{
					*theAdminOpParams_ppp = NULL;
					LEAVE_OAMSA_TRANSLATIONS();
					return MafOk;
				}
				std::string ParamName("");
				if (retPar_p.type == MafOamSpiDatatype_STRUCT)
				{
					if( theAction_p->returnType.structDatatype->generalProperties.name != NULL)
					{
						std::string ModelName(theAction_p->returnType.structDatatype->generalProperties.name);
						ParamName += ModelName;
					}
					else
					{
						ERR_OAMSA_TRANSLATIONS("ConvertActionReturnToImmParameters: NULL structDatatype parameter name, returning ComInvalidArgument");
						LEAVE_OAMSA_TRANSLATIONS();
						return MafInvalidArgument;
					}
				}

				// if the number of elements > 1 this is an array, so append [i] to the name
				if (AttrValC_p->nrOfValues > 1)
				{
					char	numbuf[32];
					sprintf(numbuf,"%u", i + 1);
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertActionReturnToImmParameters() numbuf is : %s", numbuf);
					std::string str(numbuf);
					ParamName += '_' + str;
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertActionReturnToImmParameters() simple multivalue parameter: %s", ParamName.c_str());
				}
				if (retPar_p.type == MafOamSpiDatatype_STRUCT)
				{

					// If this is a struct handle it in a special way.
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertActionReturnToImmParameters(): MafOamSpiDatatype_STRUCT");
					RetVal = (MafReturnT) HandleStructToAdminOpPar(ParamName, retPar_p.structDatatype->members,theAdmOpParList,&(AttrValC_p->values[i]), isMoSpiVersion2);
				}
				else
				{
					// Convert a single parameter
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertActionReturnToImmParameters(): single value of type %d",retPar_p.type);
					SaImmAdminOperationParamsT_2*	AdmOpParam_p = ConvertASingleAdmOpParam(ParamName, (MafOamSpiDatatypeContainerT*)&retPar_p, AttrValC_p, i, isMoSpiVersion2);
					if (AdmOpParam_p != NULL)
					{
						theAdmOpParList.push_back(AdmOpParam_p);
					}
				}
			} // for i
		}
		else
		{
			ERR_OAMSA_TRANSLATIONS("Type mismatch in Action =\"%s\" Action param type=%d passed type=%d",
			                       theAction_p->generalProperties.name,
			                       retPar_p.type,
			                       AttrValC_p->type);
			//Enable this debug if theAction_p->parameters is not NULL. //DEBUG_OAMSA_TRANSLATIONS("Parameter=\"%s\"", theAction_p->parameters->generalProperties.name);

			RetVal = MafValidationFailed;
			break;
		}
	} // end for par_p

	// If everything is OK, copy it all to the result array
	if (MafOk == RetVal)
	{
		SaImmAdminOperationParamsT_2** theAdminOpParams_pp = new  SaImmAdminOperationParamsT_2*[theAdmOpParList.size() + 1];
		int i = 0;
		for (OamSATransAdmOpParListIter iter = theAdmOpParList.begin(); iter != theAdmOpParList.end(); iter++)
		{
			theAdminOpParams_pp[i++] = *iter;
		}
		theAdminOpParams_pp[i] = NULL;
		*theAdminOpParams_ppp = theAdminOpParams_pp;
	}
	else
	{
		*theAdminOpParams_ppp = NULL;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}


MafReturnT OamSATranslator::HandleStructToAdminOpPar(const std::string&      theStructName,
                                                     MafOamSpiStructMemberT* member_p,
                                                     OamSATransAdmOpParList& theAdmOpParList,
                                                     MafMoAttributeValue_3T* theComParameterValue_p,
                                                     bool                    isMoSpiVersion2)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	RetVal = MafOk;
	MafMoAttributeValueStructMember_3* structMember = theComParameterValue_p->value.structMember;
	MafMoAttributeValueContainer_3T* ParameterList_p = structMember->memberValue;

	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToAdminOpPar() number of values %d", ParameterList_p->nrOfValues);

	for (;member_p != NULL && structMember != NULL; member_p = member_p->next, structMember = structMember->next )
	{
		ParameterList_p = structMember->memberValue;
		if ((member_p->memberType.type == (MafOamSpiDatatypeT)ParameterList_p->type) || (member_p->memberType.type == MafOamSpiDatatype_DERIVED))
		{
			for(unsigned int i = 0; i < ParameterList_p->nrOfValues; i++ )
			{
				std::string memberName = theStructName + '_' + member_p->generalProperties.name;

				// if the number of elements > 1 this is an array, so append [i] to the name
				if (ParameterList_p->nrOfValues > 1)
				{
					//DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToAdminOpPar(): Handling multivalue parameter, value number %d", i);
					char	numbuf[32];
					sprintf(numbuf,"%u", i + 1);
					std::string numbufStr(numbuf);
					memberName += '_' + numbufStr;
				}
				if (member_p->memberType.type == MafOamSpiDatatype_STRUCT)
				{
					// Also disallow structs within structs for now
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToAdminOpPar(): Structs with struct elements not supported. %s", memberName.c_str());
					RetVal = MafInvalidArgument;
					// RetVal = HandleStructToAdminOpPar(memberName, member_p->memberType.structDatatype->members, theAdmOpParList, ParameterList_p);
					if (RetVal != MafOk)
						goto end_exit;
				}
				else
				{
					// Convert a single parameter
					SaImmAdminOperationParamsT_2*	AdmOpParam_p = ConvertASingleAdmOpParam(memberName, &member_p->memberType, ParameterList_p, i, isMoSpiVersion2);
					if (AdmOpParam_p != NULL)
						theAdmOpParList.push_back(AdmOpParam_p);
				}
			}
		}
		else
		{
			ERR_OAMSA_TRANSLATIONS("Type mismatch in Struct =\"%s\" Parameter=\"%s\" Action param type=%d passed type=%d",
				theStructName.c_str(),
				member_p->generalProperties.name,
				member_p->memberType.type,
				ParameterList_p->type);
			RetVal = MafValidationFailed;
			break;
		}
	} // for member_p
end_exit:
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}


/*
 * In this function we use the MAF MR SPI Ver.4 header file MafOamSpiModelRepository_4.h
 * This is done as we need to support floating point numbers as part of MR 24146
 * And we also use MAF MR SPI Ver.1 for backward compatibility to support DERIVED type
 */
SaImmAdminOperationParamsT_2* OamSATranslator::ConvertASingleAdmOpParam(const std::string&                theParamName,
                                                                        MafOamSpiDatatypeContainerT*      parameterType_p,
                                                                        MafMoAttributeValueContainer_3T*  AttrValC_p,
                                                                        int                               i,
                                                                        bool                              isMoSpiVersion2)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaImmAdminOperationParamsT_2*	AdmOpParam_p = new SaImmAdminOperationParamsT_2;
	MafOamSpiMrType_4T				AttrType;

	/* To provide support for DERIVED type here we are mixing MAF MR SPI Ver.1 and MAF MR SPI Ver.4 !!! */
	if ((parameterType_p->type == MafOamSpiDatatype_DERIVED) && !isMoSpiVersion2)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertASingleAdmOpParam  MafOamSpiDatatype_DERIVED type = \"%d\"", (MafOamSpiDatatypeT)parameterType_p->derivedDatatype->type);
		AttrType = (MafOamSpiMrType_4T)parameterType_p->derivedDatatype->type;
	}
	else
	{
		AttrType = (MafOamSpiMrType_4T)parameterType_p->type;
	}

	AdmOpParam_p->paramName = new char[theParamName.size()+1];
	strcpy(AdmOpParam_p->paramName, theParamName.c_str());

	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertASingleAdmOpParam  AdmOpParam_p->paramName = \"%s\" AttrType = %d", AdmOpParam_p->paramName, AttrType);

	switch (AttrType)
	{
	case MafOamSpiMrTypeInt8_4:
		AdmOpParam_p->paramBuffer = new SaInt32T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAINT32T;
		*(SaInt32T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.i8;
		break;

	case MafOamSpiMrTypeInt16_4:
		AdmOpParam_p->paramBuffer = new SaInt32T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAINT32T;
		*(SaInt32T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.i16;
		break;

	case MafOamSpiMrTypeInt32_4:
		AdmOpParam_p->paramBuffer = new SaInt32T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAINT32T;
		*(SaInt32T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.i32;
		break;

	case MafOamSpiMrTypeInt64_4:
		AdmOpParam_p->paramBuffer = new SaInt64T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAINT64T;
		*(SaInt64T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.i64;
		break;

	case MafOamSpiMrTypeUint8_4:
		AdmOpParam_p->paramBuffer = new SaUint32T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAUINT32T;
		*(SaInt32T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.u8;
		break;

	case MafOamSpiMrTypeUint16_4:
		AdmOpParam_p->paramBuffer = new SaUint32T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAUINT32T;
		*(SaInt32T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.u16;
		break;

	case MafOamSpiMrTypeUint32_4:
		AdmOpParam_p->paramBuffer = new SaUint32T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAUINT32T;
		*(SaInt32T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.u32;
		break;

	case MafOamSpiMrTypeUint64_4:
		AdmOpParam_p->paramBuffer = new SaUint64T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAUINT64T;
		*(SaInt64T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.u64;
		break;

	case MafOamSpiMrTypeEnum_4:
		AdmOpParam_p->paramBuffer = new SaInt32T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAINT32T;
		*(SaInt32T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.theEnum;
		break;

	case MafOamSpiMrTypeString_4:
	{
		char *sptr = new char[strlen(AttrValC_p->values[i].value.theString)+1];
		strcpy(sptr, AttrValC_p->values[i].value.theString);
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SASTRINGT;
		AdmOpParam_p->paramBuffer = new (char*);
		*(SaStringT*)(AdmOpParam_p->paramBuffer) = sptr;
	}
	break;

	case MafOamSpiMrTypeBool_4:
		AdmOpParam_p->paramBuffer = new SaInt32T;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SAINT32T;
		*(SaInt32T*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.theBool;
		break;

	case MafOamSpiMrTypeDouble_4:
		AdmOpParam_p->paramBuffer = new SaDoubleT;
		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SADOUBLET;
		*(SaDoubleT*)(AdmOpParam_p->paramBuffer) = AttrValC_p->values[i].value.decimal64;
		break;

	case MafOamSpiMrTypeReference_4:
	{
		std::string theDN(AttrValC_p->values[i].value.moRef);
		std::string theIMMName;
		OamSACache::DNList theSplitDN;
		GlobalSplitDN(theDN,theSplitDN);
		MO2Imm_DN(theSplitDN, theIMMName);

		AdmOpParam_p->paramType	  = SA_IMM_ATTR_SANAMET;
		AdmOpParam_p->paramBuffer = new SaNameT;
		saNameSet(theIMMName.c_str(), (SaNameT *)AdmOpParam_p->paramBuffer);
	}
	break;

	default:
		// This error message covers VOID and any unknown types.
		// STRUCT is processed in another function, so if it comes here then this is an error!
		ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertASingleAdmOpParam: Unexpected or unknown action input MafOamSpiDatatype %d", AttrType);
		// These data types are not handeled here.
		// MafOamSpiDatatype_VOID - should only appear in return values, never as arguments
		// in these cases, deallocate the already allocated parts
		delete [] AdmOpParam_p->paramName;
		delete AdmOpParam_p;
		AdmOpParam_p = NULL;
		break;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return AdmOpParam_p;
}


bool OamSATranslator::sanityCheckAdminParameters(MafOamSpiParameterT* par_p, MafOamSpiDatatypeT type)
{

	// Sanity check!
	// Here we check that the storage class for the provided parameters are correct.
	// This is straight forward for all cases except for the derived types.
	// If we find a derived type in the parameter list we will receive its base type from COM.
	// Exampel: We have a type derived from INT32 that allows the following alfabeth [1..5],
	// action is invoked with the value 3
	// then in the received parameters list from COM we will have the number 3 of type INT32!
	// We need to check that it is of type INT32.
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::sanityCheckAdminParameters() Entering.. ");
	bool rc = true;
	if (par_p->parameterType.type == MafOamSpiDatatype_DERIVED)
	{
		// This is the special case for derived types, here we check the base type
		// The enum that compare between below should have the same content, lets hope so!
		if ((par_p->parameterType.derivedDatatype)->type == (MafOamSpiMoAttributeTypeT)type)
		{
			rc = true;
		}
		else
		{
			rc = false;
		}
	}
	else
	{
		// As before
		if (par_p->parameterType.type == type)
		{
			rc = true;
		}
		else
		{
			rc = false;
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::sanityCheckAdminParameters() leaving with rc = %d", rc);
	return rc;
}


// SDP 872: Compare COM return parameters against model for expected type
bool OamSATranslator::sanityCheckReturnComParameters(MafOamSpiDatatypeContainerT returnParameters_p,
                                                     MafOamSpiDatatypeT type,
                                                     bool isMoSpiVersion2)
{

	// Sanity check!
	// Here we check that the storage class for the provided parameters are correct.
	// This is straight forward for all cases except for the derived types.
	// If we find a derived type in the parameter list we will receive its base type from COM.
	// Exampel: We have a type derived from INT32 that allows the following alphabet [1..5],
	// action is invoked with the value 3
	// then in the received parameters list from COM we will have the number 3 of type INT32!
	// We need to check that it is of type INT32.
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::sanityCheckReturnAdminParameters() Entering.. ");
	bool rc = true;
	if ((returnParameters_p.type == MafOamSpiDatatype_DERIVED) && !isMoSpiVersion2)
	{
		// This is the special case for derived types, here we check the base type
		// The enum that compare between below should have the same content, lets hope so!
		if ((returnParameters_p.derivedDatatype)->type == (MafOamSpiMoAttributeTypeT)type)
		{
			rc = true;
		}
		else
		{
			rc = false;
		}
	}
	else
	{
		if ((MafOamSpiDatatype_VOID == type) && isMoSpiVersion2)
		{
			if (MafOamSpiDatatype_BOOL == returnParameters_p.type)
			{
				rc = true;
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::sanityCheckReturnAdminParameters() returnParam = BOOL type");
				rc = false;
			}
		}
		else
		{
			// As before
			if (returnParameters_p.type == type)
			{
				rc = true;
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::sanityCheckReturnAdminParameters() ck1");

			}
			else
			{
				rc = false;
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::sanityCheckReturnAdminParameters() ck2 %d, %d", returnParameters_p.type, type);
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::sanityCheckReturnAdminParameters() leaving with rc = %d", rc);
	return rc;
}


/*
 * OamSATranslator:: ConvertSimpleToMoAttribute method
 *
 * This method converts a simple parameter (single or multivalue) returned by an
 * administrative operation from IMM type to COM type
 * It also checks if a provided parameter is of correct type as specified by the model
 *
 * input txHandle
 * input returnType_p		 - the model repository type
 * input theAdminOpParams_pp - the returned parameters from the admin operation
 * output theResult_pp		 - the converted parameters for COM
 * returns MafOk if succesful, otherwise some error code
 *
 * In this function we use the MAF MR SPI Ver.4 header file MafOamSpiModelRepository_4.h
 * This is done as we need to support floating point numbers as part of MR 24146
 * And we also use MAF MR SPI Ver.1 for backward compatibility to support DERIVED type
 */
MafReturnT	OamSATranslator::ConvertSimpleToMoAttribute(MafOamSpiTransactionHandleT		txHandle,
														MafOamSpiDatatypeContainerT		*returnType_p,
														SaImmAdminOperationParamsT_2	**theAdminOpParams_pp,
														MafMoAttributeValueContainer_3T **theResult_pp,
														char * errorText)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	RetVal = MafOk;
	MafOamSpiMrType_4T type;
	// Parameter check
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() entered");
	if ((NULL == returnType_p) || (NULL == theResult_pp) || (NULL == theAdminOpParams_pp))
	{
		ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() received a NULL pointer");
		snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() received a NULL pointer");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}

	int nrOfValues = 0;
	int i = 0;
	SaImmAdminOperationParamsT_2 **adminOpParams_pp = theAdminOpParams_pp;
	MafOamSpiMultiplicityRangeT cardinality;
	cardinality.min = returnType_p->multiplicity.min;
	cardinality.max = returnType_p->multiplicity.max;

	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() The model expects return parameter of type %d, cardinality min=%d, max=%d",
		  returnType_p->type, cardinality.min, cardinality.max);

	while (*adminOpParams_pp != NULL)
	{
		nrOfValues++;
		adminOpParams_pp++;
	}

	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() admin op result nrOfValues = %d", nrOfValues);
	// check against the array range specified by the model
	if ((nrOfValues < cardinality.min) || (nrOfValues > cardinality.max))
	{
		ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Multivalue %d out of the model specified range [%d..%d]",
			nrOfValues, cardinality.min, cardinality.max);
		snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Multivalue %d out of the model specified range [%d..%d]",
				 nrOfValues, cardinality.min, cardinality.max);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}
	// check and reorder param for multivalue
	std::list<SaImmAdminOperationParamsT_2**> adminOpParamsList;
	SaImmAdminOperationParamsT_2 **tempParam_pp = NULL;
	size_t found;
	std::ostringstream s;
	std::string nameStr, tmpStr;
	bool isInvalidName = true;
	if (nrOfValues > 1)
	{
		tempParam_pp = new SaImmAdminOperationParamsT_2*[nrOfValues + 1];
		tempParam_pp[nrOfValues] = NULL;
		for (i = 0; i < nrOfValues; i++)
		{
			tmpStr.clear();
			s.str("");
			s << (i + 1);
			tmpStr = s.str();
			adminOpParams_pp = theAdminOpParams_pp;
			while ((*adminOpParams_pp) != NULL)
			{
				nameStr = (*adminOpParams_pp)->paramName;
				found = nameStr.find('_');
				if (found == std::string::npos)
				{
					ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute(): Invalid paramName: %s", nameStr.c_str());
					snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute(): Invalid paramName: %s", nameStr.c_str());
					RetVal = MafInvalidArgument;
					goto end_exit;
				}
				nameStr = nameStr.substr(found + 1);
				if (strcmp(tmpStr.c_str(), nameStr.c_str()) == 0)
				{
					isInvalidName = false;
					tempParam_pp[i] = *adminOpParams_pp;
					break;
				}

				adminOpParams_pp++;
			}
			if (isInvalidName)
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute(): Invalid paramName: %s", nameStr.c_str());
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute(): Invalid paramName: %s", nameStr.c_str());
				RetVal = MafInvalidArgument;
				goto end_exit;
			}
			isInvalidName = true;
		}
	}
	else
	{
		tempParam_pp = theAdminOpParams_pp;
	}

	(*theResult_pp)->nrOfValues = nrOfValues;
	if(nrOfValues)
	{
		(*theResult_pp)->values = new MafMoAttributeValue_3[nrOfValues];
	}
	else
	{
		(*theResult_pp)->values = NULL;
	}

	/*
	 * convert to MafMoAttributeValue_3
	 */

	/* To provide support for DERIVED type here we are converting to the base type
	 * The cast from MAF MR SPI Ver.1 type to MAF MR SPI Ver.3 type relies on the
	 * fact that the numbering in the enum matches for all other types
	 */
	if (returnType_p->type == MafOamSpiDatatype_DERIVED)
	{
		type = (MafOamSpiMrType_4T)returnType_p->derivedDatatype->type;
	}
	else
	{
		type = (MafOamSpiMrType_4T)returnType_p->type;
	}

	if (nrOfValues == 0)
	{
		(*theResult_pp)->values = NULL;
		switch(type)
		{
		case MafOamSpiMrTypeInt8_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT8;
			break;
		case MafOamSpiMrTypeInt16_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT16;
			break;
		case MafOamSpiMrTypeInt32_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT32;
			break;
		case MafOamSpiMrTypeInt64_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT64;
			break;
		case MafOamSpiMrTypeUint8_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_UINT8;
			break;
		case MafOamSpiMrTypeUint16_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_UINT16;
			break;
		case MafOamSpiMrTypeUint32_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_UINT32;
			break;
		case MafOamSpiMrTypeUint64_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_UINT64;
			break;
		case MafOamSpiMrTypeEnum_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT32;
			break;
		case MafOamSpiMrTypeString_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_STRING;
			break;
		case MafOamSpiMrTypeBool_4:
                case MafOamSpiMrTypeVoid_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_BOOL;
			break;
		case MafOamSpiMrTypeReference_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_REFERENCE;
			break;
		case MafOamSpiMrTypeDouble_4:
			(*theResult_pp)->type = MafOamSpiMoAttributeType_3_DECIMAL64;
			break;

		default:
			ERR_OAMSA_TRANSLATIONS("ERROR: OamSATranslator::ConvertSimpleToMoAttribute: Unknown action return MafOamSpiDatatype %d", type);
			snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute: Unknown action return MafOamSpiDatatype %d", type);
			RetVal = MafInvalidArgument;
			break;
		}
		goto end_exit;
	}

	for (i = 0, adminOpParams_pp = tempParam_pp; (i < nrOfValues) && (*adminOpParams_pp != NULL); i++, adminOpParams_pp++)
	{
		switch(type)
		{
		case MafOamSpiMrTypeInt8_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAINT32T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_INT8, value: %d", i, *(SaInt8T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT8;
				(*theResult_pp)->values[i].value.i8 = *(SaInt8T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_INT8 [%d]. Expected SA_IMM_ATTR_SAINT32T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_INT8 [%d]. Expected SA_IMM_ATTR_SAINT32T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeInt16_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAINT32T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_INT16, value: %d", i, *(SaInt16T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT16;
				(*theResult_pp)->values[i].value.i16 = *(SaInt16T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_INT16 [%d]. Expected SA_IMM_ATTR_SAINT32T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_INT16 [%d]. Expected SA_IMM_ATTR_SAINT32T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeInt32_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAINT32T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_INT32, value: %d", i, *(SaInt32T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT32;
				(*theResult_pp)->values[i].value.i32 = *(SaInt32T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_INT32 [%d]. Expected SA_IMM_ATTR_SAINT32T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_INT32 [%d]. Expected SA_IMM_ATTR_SAINT32T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeInt64_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAINT64T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_INT64, value: %lld", i, *(SaInt64T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_INT64;
				(*theResult_pp)->values[i].value.i64 = *(SaInt64T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_INT64 [%d]. Expected SA_IMM_ATTR_SAINT64T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_INT64 [%d]. Expected SA_IMM_ATTR_SAINT64T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeUint8_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAUINT32T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_UINT8, value: %u", i, *(SaUint8T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_UINT8;
				(*theResult_pp)->values[i].value.u8 = *(SaUint8T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_UINT8 [%d]. Expected SA_IMM_ATTR_SAUINT32T",(*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_UINT8 [%d]. Expected SA_IMM_ATTR_SAUINT32T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeUint16_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAUINT32T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_UINT16, value: %u", i, *(SaUint16T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_UINT16;
				(*theResult_pp)->values[i].value.u16 = *(SaUint16T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argumenttype %d for MafOamSpiDatatype_UINT16 [%d]. Expected SA_IMM_ATTR_SAUINT32T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argumenttype %d for MafOamSpiDatatype_UINT16 [%d]. Expected SA_IMM_ATTR_SAUINT32T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeUint32_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAUINT32T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_UINT32, value %u", i, *(SaUint32T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_UINT32;
				(*theResult_pp)->values[i].value.u32 = *(SaUint32T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_UINT32 [%d]. Expected SA_IMM_ATTR_SAUINT32T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_UINT32 [%d]. Expected SA_IMM_ATTR_SAUINT32T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeUint64_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAUINT64T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_UINT64, value: %llu", i, *(SaUint64T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_UINT64;
				(*theResult_pp)->values[i].value.u64 = *(SaUint64T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_UINT64 [%d]. Expected SA_IMM_ATTR_SAUINT64T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_UINT64 [%d]. Expected SA_IMM_ATTR_SAUINT64T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeEnum_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAINT32T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_ENUM, value: %d" , i, *(SaInt32T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_ENUM;
				(*theResult_pp)->values[i].value.theEnum = *(SaInt32T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_ENUM [%d]. Expected SA_IMM_ATTR_SAINT32T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_ENUM [%d]. Expected SA_IMM_ATTR_SAINT32T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeString_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SASTRINGT)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_STRING, value: %s" , i, *(SaStringT*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_STRING;
				(*theResult_pp)->values[i].value.theString = new char[strlen(*(SaStringT*)((*adminOpParams_pp)->paramBuffer))+1];
				strcpy(const_cast<char*>((*theResult_pp)->values[i].value.theString), *(SaStringT*)((*adminOpParams_pp)->paramBuffer));
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_STRING [%d]. Expected SA_IMM_ATTR_SASTRINGT", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_STRING [%d]. Expected SA_IMM_ATTR_SASTRINGT",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeBool_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAINT32T)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_BOOL, value %d", i, *(SaInt32T*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_BOOL;
				(*theResult_pp)->values[i].value.theBool = *(SaInt32T*)((*adminOpParams_pp)->paramBuffer);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_BOOL [%d]. Expected SA_IMM_ATTR_SAINT32T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_BOOL [%d]. Expected SA_IMM_ATTR_SAINT32T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeReference_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SANAMET)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_REFERENCE, input value: %s", i, saNameGet( ((SaNameT *)(*adminOpParams_pp)->paramBuffer)));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_REFERENCE;
				char *theCOMName;
				SaNameT* atp = (SaNameT*) (*adminOpParams_pp)->paramBuffer;
				Imm2MO_DN(txHandle, saNameGet(atp), &theCOMName);
				int len = strlen(theCOMName);
				(*theResult_pp)->values[i].value.moRef = new char[len + 1];
				strcpy(const_cast<char*>((*theResult_pp)->values[i].value.moRef), theCOMName);
				delete []theCOMName;
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_REFERENCE, output value: %s", i, (*theResult_pp)->values[i].value.moRef);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_REFERENCE [%d]. Expected SA_IMM_ATTR_SANAMET", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_REFERENCE [%d]. Expected SA_IMM_ATTR_SANAMET",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;

		case MafOamSpiMrTypeDouble_4:
		{
			if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SAFLOATT)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_DECIMAL64, value: %f", i, *(SaFloatT*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_DECIMAL64;
				(*theResult_pp)->values[i].value.decimal64 = *(float*)((*adminOpParams_pp)->paramBuffer);
			}
			else if ((*adminOpParams_pp)->paramType == SA_IMM_ATTR_SADOUBLET){
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMoAttributeType_3_DECIMAL64, value: %f", i, *(SaDoubleT*)((*adminOpParams_pp)->paramBuffer));
				(*theResult_pp)->type = MafOamSpiMoAttributeType_3_DECIMAL64;
				(*theResult_pp)->values[i].value.decimal64 = *(double*)((*adminOpParams_pp)->paramBuffer);

			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_UINT64 [%d]. Expected SA_IMM_ATTR_SAUINT64T", (*adminOpParams_pp)->paramType, i);
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute() Invalid Argument type %d for MafOamSpiDatatype_UINT64 [%d]. Expected SA_IMM_ATTR_SAUINT64T",
						 (*adminOpParams_pp)->paramType, i);
				RetVal = MafInvalidArgument;
			}
		}
		break;
                case MafOamSpiMrTypeVoid_4:
                {
                        DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute() type [%d] MafOamSpiMrTypeVoid_3", i);
                        (*theResult_pp)->nrOfValues = 0;
                        if((*theResult_pp)->values)
                        {
                                 delete [](*theResult_pp)->values;
                        }
                        (*theResult_pp)->values = NULL;
                        (*theResult_pp)->type = MafOamSpiMoAttributeType_3_BOOL;
                }
                break;

		default:
			ERR_OAMSA_TRANSLATIONS("OamSATranslator::ConvertSimpleToMoAttribute: Unknown action return MafOamSpiDatatype %d", type);
			snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::ConvertSimpleToMoAttribute: Unknown action return MafOamSpiDatatype %d", type);
			RetVal = MafInvalidArgument;
			break;
		}
	}
end_exit:
	//free memory
	if ((NULL != tempParam_pp) && (nrOfValues > 1))
	{
		delete[] tempParam_pp;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}


/* OamSATranslator::HandleStructToMoAttribute method
 *
 * This method converts struct parameters (single or multivalue) returned by an
 * administrative operation from IMM type to COM type
 * It also checks if a provided parameter has the correct name as specified by the model
 * and by the naming convention for structure and multivalue parameters
 *
 * input txHandle
 * input returnType_p		 - the model repository type
 * input theAdminOpParams_pp - the returned parameters from the admin operation
 * output theResult_pp		 - the converted parameters for COM
 * returns MafOk if succesful, otherwise some error code
 */
MafReturnT	OamSATranslator::HandleStructToMoAttribute(MafOamSpiTransactionHandleT txHandle,
													   MafOamSpiDatatypeContainerT *returnType_p,
													   SaImmAdminOperationParamsT_2 **theAdminOpParams_pp,
													   MafMoAttributeValueContainer_3T **theResult_pp,
													   char *errorText)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT	RetVal = MafOk;
	// Parameter check
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute() entered");
	if ((NULL == returnType_p) || (NULL == theResult_pp) || (NULL == theAdminOpParams_pp))
	{
		ERR_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): NULL parameters, returning ComInvalidArgument");
		snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::HandleStructToMoAttribute(): NULL parameters, returning ComInvalidArgument");
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}
	SaImmAdminOperationParamsT_2 **adminOpParams_pp = theAdminOpParams_pp;
	MafOamSpiStructT *structDatatype_p = returnType_p->structDatatype;
	unsigned int nrOfValues = 1;
	unsigned int nrOfParams = 0;
	unsigned int count = 0;
	unsigned int i = 0;
	bool isMultiStructure = false;
	size_t underScorePos;

	std::string paramNameStr, tmpStr;
	std::list<std::string> memberNameList;
	std::list<std::string> multiValueList;
	std::list<std::string>::iterator it_memberNameList;

	MafOamSpiMultiplicityRangeT cardinality;
	cardinality.min = returnType_p->multiplicity.min;
	cardinality.max = returnType_p->multiplicity.max;
	bool modelTypeMultiValue = true;
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute() The model expects return parameter of type %d, cardinality min=%d, max=%d",
		  returnType_p->type, cardinality.min, cardinality.max);
	if ((1 == cardinality.min) && (1 == cardinality.max))
	{
		modelTypeMultiValue = false;
	}

	(*theResult_pp)->type = MafOamSpiMoAttributeType_3_STRUCT;
	/*Identify:
	  - nrOfValues:		  number of multivalue structure
	  - memberNameList:	  the list of member names
	  - isMultiStructure: check if structure is multivalue or not
	*/
	/* There are 4 cases of VALID paramName(s):
	   - case 1: par1_memb1		   (simple structure with simple member)
	   - case 2: par1_memb1_1	   (simple structure with multivalue member)
	   - case 3: par1_1_memb1	   (multivalue structure with simple member)
	   - case 4: par1_1_memb1_1	   (multivalue structure with multivalue member)

	   And there are many cases of INVALID paramName(s), some examples here:
	   - case I1:  1_par1, 12_par1	  (parameter name can not start with a digit, must be a letter)
	   - case I2:  1par1, 123par1	  (parameter name can not start with a digit, or is this valid if after the number there is a letter?)
	   - case I3:  par1_1memb		  (structure member name can not start with a digit, must be a letter)
	   - case I4:  par1_1_2			  (after the first number another number is not allowed, must be alphanumeric)
	   - case I5:  par1_memb1_1_2	  (after the number for a member multivalue another number is not allowed, or maybe is OK?)
	   - case I6:  par1_1_memb1_1_2	  (after the number for a member multivalue another number is not allowed, or maybe is OK?)
	   - case I7:  par1_1_memb1_memb2 (structure elements can not be structures themselves.
	   Alternatively 'memb1_memb2' could be a valid member name,
	   but this will complicate the algorithm to handle names with underscore allowed)
	   - case I8:  _par1			  (a structure name can not start with underscore)
	   - case I9:  par1_memb1_		  (a structure member name can not end with underscore)
	   - case I10: par1__memb1		  (double underscore is not allowed anywhere in the name)
	   - case I11: par1_memb1__1	  (double underscore is not allowed anywhere in the name)
	   - case I12: par1__1_memb1	  (double underscore is not allowed anywhere in the name)
	   - case I13: par1_1__memb1	  (double underscore is not allowed anywhere in the name)
	   - case I14: par1_1_memb1__	  (double underscore is not allowed anywhere in the name)
	   - case I15: par1_1_memb1__1	  (double underscore is not allowed anywhere in the name)
	   - ....

	   There could be also combinations that are invalid:
	   - a parameter name repeated
	   - parameter names not forming a proper sequence, for example: "par1_1_memb1, par1_3_memb1" (par1_2_memb1 is missing).
	   - number '0' used
	   - sequence not starting with '1'
	   - ....
	*/

	while (*adminOpParams_pp != NULL)
	{
		paramNameStr = (*adminOpParams_pp)->paramName;
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): paramName is : %s", paramNameStr.c_str());
		underScorePos = paramNameStr.find('_');
		if (underScorePos != std::string::npos)
		{
			tmpStr = paramNameStr.substr(0,underScorePos);
			paramNameStr = paramNameStr.substr(underScorePos + 1);
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): tmStr is %s", tmpStr.c_str());
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): paramNameStr is %s", paramNameStr.c_str());
			// check struct name if necessary
			if (strcmp(structDatatype_p->generalProperties.name, tmpStr.c_str()) != 0)
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Invalid struct name. Expected [%s], found [%s]",
					structDatatype_p->generalProperties.name, tmpStr.c_str());
				snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::HandleStructToMoAttribute(): Invalid struct name. Expected [%s], found [%s]",
						 structDatatype_p->generalProperties.name, tmpStr.c_str());
				RetVal = MafInvalidArgument;
				LEAVE_OAMSA_TRANSLATIONS();
				return RetVal;
			}
			underScorePos = paramNameStr.find('_');
			if (underScorePos != std::string::npos)
			{
				tmpStr = paramNameStr.substr(0,underScorePos);
				paramNameStr = paramNameStr.substr(underScorePos + 1);
				//check tmpStr is number or not
				bool isNumber = true;
				for (i = 0; i < strlen(tmpStr.c_str()); i++)
				{
					if( !isdigit(tmpStr.c_str()[i]))
					{
						isNumber = false;
						break;
					}
				}
				if (isNumber)
					//case 3 and 4
				{
					insert_nameList(multiValueList, tmpStr);
					isMultiStructure = true;
					underScorePos = paramNameStr.find('_');
					if (underScorePos != std::string::npos)
					{
						DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Multivalue structure with multivalue member");
						paramNameStr = paramNameStr.substr(0,underScorePos);
					}
					else
						DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Multivalue structure with simple member");
					insert_nameList(memberNameList, paramNameStr);
				}
				else
				{
					//case 2
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Simple structure with multivalue member");

					if(modelTypeMultiValue)
					{
						ERR_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Received single structure, but the model expects multivalue");
						snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::HandleStructToMoAttribute(): Received single structure, but the model expects multivalue");
						RetVal = MafInvalidArgument;
						LEAVE_OAMSA_TRANSLATIONS();
						return RetVal;
					}

					insert_nameList(memberNameList, tmpStr);
				}
			}
			else
			{
				//case 1
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Simple structure with simple member");

				if(modelTypeMultiValue)
				{
					ERR_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Received single structure with simple member, but the model expects multivalue");
					snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::HandleStructToMoAttribute(): Received single structure with simple member, but the model expects multivalue");
					RetVal = MafInvalidArgument;
					LEAVE_OAMSA_TRANSLATIONS();
					return RetVal;
				}

				insert_nameList(memberNameList, paramNameStr);
			}
		}
		else
		{
			ERR_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Invalid paramName: '%s', no '_' found", paramNameStr.c_str());
			snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::HandleStructToMoAttribute(): Invalid paramName: '%s', no '_' found", paramNameStr.c_str());
			RetVal = MafInvalidArgument;
			LEAVE_OAMSA_TRANSLATIONS();
			return RetVal;
		}
		nrOfParams++;
		adminOpParams_pp++;
	}

	// check that a struct member name is matching the defined name in the Model Repository
	MafOamSpiStructMemberT* member_p;
	bool isMatch = false;
	for (it_memberNameList = memberNameList.begin(); it_memberNameList != memberNameList.end(); ++it_memberNameList)
	{
		member_p = structDatatype_p->members;
		for (;member_p != NULL; member_p = member_p->next)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): comparing struct member name: '%s' to model: '%s'",
				  (*it_memberNameList).c_str(), member_p->generalProperties.name);
			if (strcmp((*it_memberNameList).c_str(), member_p->generalProperties.name) == 0)
				isMatch = true;
		}
		if (!isMatch)
		{
			ERR_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): Invalid struct member name. Expected %s", (*it_memberNameList).c_str());
			snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::HandleStructToMoAttribute(): Invalid struct member name");
			RetVal = MafInvalidArgument;
			LEAVE_OAMSA_TRANSLATIONS();
			return RetVal;
		}
		isMatch = false;
	}

	if (isMultiStructure)
	{
		count = multiValueList.size();
		nrOfValues = count;
	}
	else
	{
		count = 1;
	}
	// check if the model also expects multivalue struct
	if (isMultiStructure && !modelTypeMultiValue)
	{
		ERR_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): the model return type is single structure, received multivalue of count %d", count);
		snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::HandleStructToMoAttribute(): the model return type is single structure, received multivalue of count %u", count);
		RetVal = MafInvalidArgument;
		LEAVE_OAMSA_TRANSLATIONS();
		return RetVal;
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): nrOfValues = %d, count = %d, nrOfParams = %d", nrOfValues, count, nrOfParams);
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): the number of multi struct elements is %d", nrOfValues);

	// check against the array range specified by the model
	if (((int) count < cardinality.min) || ((int) count > cardinality.max))
	{
		ERR_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute() Multivalue %d out of the model specified range [%d..%d]",
			count, cardinality.min, cardinality.max);
		snprintf(errorText, ERR_STR_BUF_SIZE, "@COMNBI@OamSATranslator::HandleStructToMoAttribute() Multivalue %u out of the model specified range [%d..%d]",
				 count, cardinality.min, cardinality.max);
		LEAVE_OAMSA_TRANSLATIONS();
		return MafInvalidArgument;
	}

	for (it_memberNameList = memberNameList.begin(); it_memberNameList != memberNameList.end(); ++it_memberNameList)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::HandleStructToMoAttribute(): struct member name is %s", (*it_memberNameList).c_str());
	}
	std::ostringstream s;

	StructMemberMapList structMemberMapList;
	StructMemberMapListIterator it_structMemberMapList;
	StructMemberMapIterator it_structMemberMap;
	SaImmAdminOperationParamsT_2** tempParam_pp;
	/*
                                                        |---<memb1,NULL-terminated array>            |--------------|
                                    |---------------|   |                        |---------------+-->|par1_1_memb1_1|
                                |---|structMemberMap|---|                                        |   |--------------|
                                |   |---------------|   |                                        |-->|par1_1_memb1_2|
        |-------------------|   |                       |---<memb2,NULL-terminated array>            |--------------|
        |structMemberMapList|---|                                                |------------------>|par1_1_memb2  |
        |-------------------|   |                       |---<memb1,NULL-terminated array>            |--------------|
                                |   |---------------|   |                        |---------------+-->|par1_2_memb1_1|
                                |---|structMemberMap|---|                                        |   |--------------|
                                    |---------------|   |                                        |-->|par1_2_memb1_2|
                                                        |---<memb2,NULL-terminated array>            |--------------|
                                                                                |------------------->|par1_2_memb2  |
                                                                                                     |--------------|
	*/
	StructMemberMap* structMemberMap_p = new StructMemberMap[nrOfValues];
	for (i = 0; i < nrOfValues; i++)
	{
		for (it_memberNameList = memberNameList.begin(); it_memberNameList != memberNameList.end(); ++it_memberNameList)
		{
			if (isMultiStructure)
			{
				s.str("");
				s << (i + 1);
				tmpStr = "_" + s.str() + "_" + (*it_memberNameList);
			}
			else
				tmpStr = *it_memberNameList;
			tempParam_pp = create_tempParam(theAdminOpParams_pp, tmpStr);
			structMemberMap_p[i].insert(std::pair<std::string, SaImmAdminOperationParamsT_2**>(*it_memberNameList, tempParam_pp));
			DEBUG_OAMSA_TRANSLATIONS("--> inserting in structMemberMap: %s", tmpStr.c_str());
		}
		DEBUG_OAMSA_TRANSLATIONS("--> Adding to structMemberMapList");
		structMemberMapList.push_back(structMemberMap_p[i]);
		DEBUG_OAMSA_TRANSLATIONS("--> in 1-st loop after push back: structMemberMap.size = %d", (int) structMemberMap_p[i].size());
	}

	DEBUG_OAMSA_TRANSLATIONS("--> structMemberMapList.size = %d", (int) structMemberMapList.size());

	//create struct member content
	(*theResult_pp)->nrOfValues = nrOfValues;
	(*theResult_pp)->values = new MafMoAttributeValue_3[nrOfValues];
	it_structMemberMapList = structMemberMapList.begin();
	for (i = 0; i < nrOfValues; i++)
	{
		DEBUG_OAMSA_TRANSLATIONS("--> calling create_structMember i = %d", i);
		(*theResult_pp)->values[i].value.structMember = create_structMember(txHandle, returnType_p->structDatatype->members, *it_structMemberMapList, RetVal, errorText);
		it_structMemberMapList++;
	}

	//free memory
	//TBD
	for( it_structMemberMapList = structMemberMapList.begin(); it_structMemberMapList != structMemberMapList.end(); it_structMemberMapList++)
	{
		for (it_structMemberMap = (*it_structMemberMapList).begin(); it_structMemberMap != (*it_structMemberMapList).end(); it_structMemberMap++)
		{
			if (it_structMemberMap->second != NULL)
			{
				delete[] (it_structMemberMap->second);
			}
		}
	}
	delete[] structMemberMap_p;
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}


void OamSATranslator::insert_nameList(std::list<std::string>& memberNameList, std::string& paramNameStr)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string>::iterator it;
	bool isExist = false;
	if (memberNameList.empty())
	{
		memberNameList.push_back(paramNameStr);
	}
	else
	{
		for (it = memberNameList.begin(); it != memberNameList.end(); ++it)
		{
			if (paramNameStr.compare(*it) == 0)
				isExist = true;
		}
		if (!isExist)
			memberNameList.push_back(paramNameStr);
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

MafMoAttributeValueStructMember_3T* OamSATranslator::create_structMember(MafOamSpiTransactionHandleT txHandle, MafOamSpiStructMemberT* member_p, StructMemberMap& structMemberMap, MafReturnT& RetVal, char* errorText)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafMoAttributeValueStructMember_3T *structMember;
	MafMoAttributeValueStructMember_3T *head = NULL;
	MafMoAttributeValueStructMember_3T *temp = NULL;
	std::map < std::string, SaImmAdminOperationParamsT_2**>::iterator it;
	MafOamSpiStructMemberT* tmpMember_p;
	MafOamSpiDatatypeContainerT *returnType_p = NULL;
	std::tr1::shared_ptr<TxContext> txContextIn = OamSATransactionRepository::getOamSATransactionRepository()->getTxContext(txHandle);
	if (NULL == txContextIn.get())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::create_structMember the context associated with handle %d was not found in the repository", (int)txHandle);
	}
	else
	{
		for (it = structMemberMap.begin(); it != structMemberMap.end(); ++it)
		{
			structMember = new MafMoAttributeValueStructMember_3;
			structMember->memberName = new char[strlen((it->first).c_str())+1];
			strcpy(structMember->memberName,(it->first).c_str());
			structMember->memberValue = new MafMoAttributeValueContainer_3;
			txContextIn->GetCache().RegisterAttributeValueContainer(structMember->memberValue);
			structMember->next = NULL;
			//
			tmpMember_p = member_p;
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::create_structMember: member name is %s", (it->first).c_str());
			for (;tmpMember_p != NULL; tmpMember_p = tmpMember_p->next)
			{
				if (strcmp((it->first).c_str(), tmpMember_p->generalProperties.name) == 0)
				{
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::create_structMember: type %d", tmpMember_p->memberType.type);
					returnType_p = &tmpMember_p->memberType;
				}
			}
			RetVal = ConvertSimpleToMoAttribute(txHandle, returnType_p, it->second, &(structMember->memberValue), errorText);
			returnType_p = NULL;
			if (head == NULL)
			{
				head = structMember;
				// DEBUG_OAMSA_TRANSLATIONS("--->head was NULL");
			}
			else
			{
				temp = head;
				while(temp->next != NULL)
				{
					temp = temp->next;
					// DEBUG_OAMSA_TRANSLATIONS("---> temp = temp->next;");
				}
				temp->next = structMember;
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return head;
}


SaImmAdminOperationParamsT_2** OamSATranslator::create_tempParam(SaImmAdminOperationParamsT_2 **adminOpParams_pp, std::string& name)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaImmAdminOperationParamsT_2 **tempParam_pp;
	SaImmAdminOperationParamsT_2 **adminOp_pp;
	std::string tempStr;
	size_t found, underscore;
	int count = 0;
	adminOp_pp = adminOpParams_pp;
	while (*adminOp_pp != NULL)
	{
		tempStr = (*adminOp_pp)->paramName;
		found = tempStr.find(name);
		if (found != std::string::npos)
			count++;
		adminOp_pp++;
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::create_tempParam: count is %d", count);
	tempParam_pp = new SaImmAdminOperationParamsT_2*[count + 1];
	tempParam_pp[count] = NULL;
	int i = 0;
	adminOp_pp = adminOpParams_pp;
	while (*adminOp_pp != NULL)
	{
		tempStr = (*adminOp_pp)->paramName;
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::create_tempParam: the original param name is '%s'", (*adminOp_pp)->paramName);
		found = tempStr.find(name);
		if (found != std::string::npos)
		{
			tempParam_pp[i] = *adminOp_pp;
			tempStr = tempStr.substr(found);
			underscore = name.find('_');
			if (underscore != std::string::npos)
			{
				found = tempStr.find('_');
				tempStr = tempStr.substr(found + 1);
				found = tempStr.find('_');
				tempStr = tempStr.substr(found + 1);
			}
			free((void*)(*adminOp_pp)->paramName);
			(*adminOp_pp)->paramName = (char *) malloc(strlen(tempStr.c_str()) +1);
			strcpy((*adminOp_pp)->paramName, tempStr.c_str());
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::create_tempParam: the param name after cut is '%s'", (*adminOp_pp)->paramName);
			i++;
		}
		adminOp_pp++;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return tempParam_pp;
}


/* OamSATranslator::convertToMafValue method
 *
 * This method convert a MafMoAttributeValue_3T to MafMoAttributeValue_3T
 * It will allocate memory for the string types
 * No return value
 */
void OamSATranslator::convertToMafValue(MafOamSpiMoAttributeType_3T type, MafMoAttributeValue_3T *valuesM, MafMoAttributeValue_3T *valuesC)
{
	ENTER_OAMSA_TRANSLATIONS();
	switch (type)
	{
		/**
		 * An 8-bit integer.
		 */
	case MafOamSpiMoAttributeType_3_INT8 :
		valuesM->value.i8 = valuesC->value.i8;
		break;
		/**
		 * A 16-bit integer.
		 */
	case MafOamSpiMoAttributeType_3_INT16 :
		valuesM->value.i16 = valuesC->value.i16;
		break;
		/**
		 * A 32-bit integer.
		 */
	case MafOamSpiMoAttributeType_3_INT32 :
		valuesM->value.i32 = valuesC->value.i32;
		break;
		/**
		 * A 64-bit integer.
		 */
	case MafOamSpiMoAttributeType_3_INT64 :
		valuesM->value.i64 = valuesC->value.i64;
		break;
		/**
		 * An 8-bit unsigned integer.
		 */
	case MafOamSpiMoAttributeType_3_UINT8 :
		valuesM->value.u8 = valuesC->value.u8;
		break;
		/**
		 * A 16-bit unsigned integer.
		 */
	case MafOamSpiMoAttributeType_3_UINT16:
		valuesM->value.u16 = valuesC->value.u16;
		break;
		/**
		 * A 32-bit unsigned integer.
		 */
	case MafOamSpiMoAttributeType_3_UINT32 :
		valuesM->value.u32 = valuesC->value.u32;
		break;
		/**
		 * A 64-bit unsigned integer.
		 */
	case MafOamSpiMoAttributeType_3_UINT64 :
		valuesM->value.u64 = valuesC->value.u64;
		break;
		/**
		 * A string value.
		 */
	case MafOamSpiMoAttributeType_3_STRING :
		valuesM->value.theString = new char[strlen(valuesC->value.theString)+1];
		strcpy((char*)valuesM->value.theString, valuesC->value.theString);
		break;
		/**
		 * A boolean.
		 */
	case MafOamSpiMoAttributeType_3_BOOL :
		valuesM->value.theBool = valuesC->value.theBool;
		break;
		/**
		 * A reference to another Managed Object (MO) class.
		 */
	case MafOamSpiMoAttributeType_3_REFERENCE :
		valuesM->value.moRef = new char[strlen(valuesC->value.moRef)+1];
		strcpy((char*)valuesM->value.moRef, valuesC->value.moRef);
		break;
		/**
		 * An enumeration.
		 */
	case MafOamSpiMoAttributeType_3_ENUM :
		valuesM->value.theEnum = valuesC->value.theEnum;
		break;
		/**
		 * A struct or aggregated data type.
		 */
	case MafOamSpiMoAttributeType_3_STRUCT :
		// Not supported!! What to do?
		ERR_OAMSA_TRANSLATIONS("OamSATranslator::fillStructMembers Struct in Struct unsupported");
		break;
		/**
		 * A 64 bits floating point value
		 */
	case MafOamSpiMoAttributeType_3_DECIMAL64 :
		valuesM->value.decimal64 = valuesC->value.decimal64;
		break;

	default:
		ERR_OAMSA_TRANSLATIONS("OamSATranslator::convertToMafValue invalid MafOamSpiMoAttributeType_3T type %d", type);
		break;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


/* OamSATranslator::initializeImmHandle method
 *
 * This method initialize the handles to the imm that is needed to access data in the IMM
 * return true : if success
 * return false: if no success
 */
bool OamSATranslator::initializeImmHandle()
{
	ENTER_OAMSA_TRANSLATIONS();
	SaAisErrorT Errt;
	if((0 == immOmHandleLocal) && ((Errt = (autoRetry_saImmOmInitialize(&immOmHandleLocal, NULL, &imm_version))) != SA_AIS_OK))
	{
		ERR_OAMSA_TRANSLATIONS("OamSATranslator(): saImmOmInitialize failed %s", getOpenSAFErrorString(Errt));
		finalizeImmHandle();
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	if((0 == OamSAaccessorHandleT) && ((Errt = (autoRetry_saImmOmAccessorInitialize(immOmHandleLocal, &OamSAaccessorHandleT))) != SA_AIS_OK)) {
		ERR_OAMSA_TRANSLATIONS("OamSATranslator(): saImmOmAccessorInitialize failed %s", getOpenSAFErrorString(Errt));
		finalizeImmHandle();
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator(): saImmOmAccessorInitialize SUCCESS (%llu)",OamSAaccessorHandleT);
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}

bool OamSATranslator::initImmHandleForTxhandleZero(SaImmHandleT *immOmHandleTxZero, SaImmAccessorHandleT *OamSAaccessorHandleTxZero)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaAisErrorT Errt;
	if((Errt = autoRetry_saImmOmInitialize(immOmHandleTxZero, NULL, &imm_version)) != SA_AIS_OK)
	{
		ERR_OAMSA_TRANSLATIONS("OamSATranslator()::initImmHandleForTxhandleZero saImmOmInitialize failed %s", getOpenSAFErrorString(Errt));
		finalizeImmHandleForTxhandleZero(*immOmHandleTxZero, (SaImmAccessorHandleT)NULL);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	if((autoRetry_saImmOmAccessorInitialize(*immOmHandleTxZero, OamSAaccessorHandleTxZero)) != SA_AIS_OK) {
		ERR_OAMSA_TRANSLATIONS("OamSATranslator()::initImmHandleForTxhandleZero saImmOmAccessorInitialize failed");
		finalizeImmHandleForTxhandleZero(*immOmHandleTxZero, *OamSAaccessorHandleTxZero);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}


void OamSATranslator::finalizeImmHandleForTxhandleZero(SaImmHandleT immOmHandleTxZero, SaImmAccessorHandleT OamSAaccessorHandleTxZero)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaAisErrorT error;
	if ((OamSAaccessorHandleTxZero) != 0)
	{
		error=autoRetry_saImmOmAccessorFinalize(OamSAaccessorHandleTxZero);
		if (error != SA_AIS_OK)
		{
			ERR_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandleForTxhandleZero(): saImmOmFinalize FAILED: %s", getOpenSAFErrorString(error));
		}
		OamSAaccessorHandleTxZero = 0;
	}
	if(immOmHandleTxZero != 0)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandleForTxhandleZero(): try saImmOmFinalize(%llu)", immOmHandleTxZero);
		error=autoRetry_saImmOmFinalize(immOmHandleTxZero);
		if (error != SA_AIS_OK)
		{
			ERR_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandleForTxhandleZero(): saImmOmFinalize(%llu) FAILED: %s", immOmHandleTxZero, getOpenSAFErrorString(error));
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandleForTxhandleZero(): saImmOmFinalize(%llu) SUCCESS: %s", immOmHandleTxZero, getOpenSAFErrorString(error));
		}
		immOmHandleTxZero = 0;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/* OamSATranslator::getStructRefName method
 *
 * This method returns the matching struct attribute name in the parent class if one exist!
 * It also checks the provided attribute is a member of the union.
 * return attribute name if it exists
 * return empty string if not exists
 */
std::string OamSATranslator::getStructRefName(std::string& stringimmRdn, std::string& parentClassName,
											  std::string& comParenClassName, const char *attributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	// OK, is a struct immRdn object
	// Read the parent object and search for the struct reference
	// We have something like id=s1_1,AId=2,rootHappy=1 and we would like to have AId=2,rootHappy=1
	// This might throw an exception
	std::string SimmRdn = stringimmRdn;
	SimmRdn = SimmRdn.substr((SimmRdn.find(',')+1), SimmRdn.length());
	// Read all attributes for the DN:
	// Translate to 3Gpp format, without a a cache available, to avoid the create case of a struct instance.
	std::string moDn = ImmRdn2MOTop(SimmRdn);
	if (!GetClassName(SimmRdn.c_str(), parentClassName, comParenClassName))
	{
		// OK, fail and return previous result
		LEAVE_OAMSA_TRANSLATIONS();
		return "";
	}
	OamSACache::DNList mo_name;
	GlobalSplitDN(moDn, mo_name);

	MafOamSpiMoAttributeT* attributeList = GetComAttributeList(mo_name);

	// Get the possible struct attributes, iterate over all attributes and match them one at the time
	//std::list<std::string> structAttributes; // Unused variable
	while( attributeList != NULL  )
	{
		if (isStructAttribute(mo_name, attributeList->generalProperties.name))
		{
			// check if it is our attribute
			// Read the value in IMM
			SaImmAttrValuesT_2 **AttrValue = NULL;

			std::string tempattribute = attributeList->generalProperties.name;
			TxContext myPrivate;
			if (!initializeImmHandle())
			{
				ERR_OAMSA_TRANSLATIONS("OamSATranslator::getStructRefName() failed to initialize imm accesorhandle");
				// We can not check this so then we return the result we have.
				LEAVE_OAMSA_TRANSLATIONS();
				return "";
			}

			myPrivate.mAccessorHandle = OamSAaccessorHandleT;
			ImmCmdOmAccessorGet immCmdOmAccessorGet(&myPrivate, SimmRdn, tempattribute, &AttrValue);
			SaAisErrorT Errt;
			Errt = immCmdOmAccessorGet.execute();

			if(SA_AIS_ERR_BAD_HANDLE == Errt)
			{
				WARN_OAMSA_TRANSLATIONS("immCmdOmAccessorGet returns SA_AIS_ERR_BAD_HANDLE, re-initializing IMM handlers");
				finalizeImmHandle();
				if(initializeImmHandle())
				{
					myPrivate.mAccessorHandle = OamSAaccessorHandleT;
					ImmCmdOmAccessorGet immCmdOmAccessorGetRetry(&myPrivate, SimmRdn, tempattribute, &AttrValue);
					Errt = immCmdOmAccessorGetRetry.execute();
				}
				else
				{
					WARN_OAMSA_TRANSLATIONS("re-initialize of IMM handlers failed");
				}
			}
			if ( (Errt == SA_AIS_OK) && ((*AttrValue)->attrValuesNumber != 0) )
			{
				// OK we have values here (can be more than one and all have to be checked),
				// convert it and check if we found the attribute
				int numberOfValues = (*AttrValue)->attrValuesNumber;
				// Index for where we check for a value
				int checkPosition = 0;
				char* thereference = NULL;
				char immName[saNameMaxLen()];
				std::string referenceValue;
				bool foundValue = false;
				while ( (numberOfValues != checkPosition) && (!foundValue) )
				{
					if (SA_IMM_ATTR_SASTRINGT == (*AttrValue)->attrValueType)
					{
						thereference = *(char**)(*AttrValue)->attrValues[checkPosition];
					}else if (SA_IMM_ATTR_SANAMET == (*AttrValue)->attrValueType)
					{
						SaNameT* atp = (SaNameT*)((*AttrValue)->attrValues[checkPosition]);
						memcpy(immName, saNameGet(atp), saNameLen(atp));
						immName[saNameLen(atp)] = '\0';
						thereference = immName;
					}
					referenceValue = thereference;
					if (stringimmRdn == referenceValue)
					{
						foundValue = true;
					}
					++checkPosition;
				}
				// Check the value
				if (foundValue)
				{
					// OK, this is a candidate for our reference
					if (attributeName != NULL)
					{
						// Check if the attribute is a member of the found struct
						std::list<string> memberNames = getStructMembers(mo_name,tempattribute);
						std::list<std::string>::reverse_iterator it;
						for( it = memberNames.rbegin(); it != memberNames.rend(); ++it)
						{
							std::string stringAttributeName = attributeName;
							if ( *it == stringAttributeName )
							{
								// OK this is our attribute name

								// myPrivate will go out from scope(and that time it will finalize the accessor handle), check this: "myPrivate.mAccessorHandle = OamSAaccessorHandleT"
								// so reset the accessor handle of the Translator to not double finalize the handle when Translator is destructed
								OamSAaccessorHandleT = 0;
								LEAVE_OAMSA_TRANSLATIONS();
								return tempattribute;
							}
						}
						// If we reach this, we have not found our attribute as a member

						// myPrivate will go out from scope(and that time it will finalize the accessor handle), check this: "myPrivate.mAccessorHandle = OamSAaccessorHandleT"
						// so reset the accessor handle of the Translator to not double finalize the handle when Translator is destructed
						OamSAaccessorHandleT = 0;
						LEAVE_OAMSA_TRANSLATIONS();
						return "";
					}
					else
					{
						// REturn what we have found without any checking

						// myPrivate will go out from scope(and that time it will finalize the accessor handle), check this: "myPrivate.mAccessorHandle = OamSAaccessorHandleT"
						// so reset the accessor handle of the Translator to not double finalize the handle when Translator is destructed
						OamSAaccessorHandleT = 0;
						LEAVE_OAMSA_TRANSLATIONS();
						return tempattribute;
					}
				}
			}
			// myPrivate will go out from scope(and that time it will finalize the accessor handle), check this: "myPrivate.mAccessorHandle = OamSAaccessorHandleT"
			// so reset the accessor handle of the Translator to not double finalize the handle when Translator is destructed
			OamSAaccessorHandleT = 0;
		}
		// Get the next attribute in the list
		attributeList = attributeList->next;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return "";
}

/* OamSATranslator::finalizeImmHandle method
 *
 * This method finalize the handles to the imm that is needed to access data in the IMM
 * return true : if success
 * return false: if no success
 */
void OamSATranslator::finalizeImmHandle()
{
	ENTER_OAMSA_TRANSLATIONS();
	SaAisErrorT error;
	if (OamSAaccessorHandleT != 0)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandle(): try saImmOmAccessorFinalize(%llu)", OamSAaccessorHandleT);
		error=autoRetry_saImmOmAccessorFinalize(OamSAaccessorHandleT);
		if (error != SA_AIS_OK)
		{
			ERR_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandle(): saImmOmAccessorFinalize FAILED: %s", getOpenSAFErrorString(error));
		}
		OamSAaccessorHandleT = 0;
	}

	if(immOmHandleLocal != 0)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandle(): try saImmOmFinalize(%llu)", immOmHandleLocal);
		error=autoRetry_saImmOmFinalize(immOmHandleLocal);
		if (error != SA_AIS_OK)
		{
			ERR_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandle(): saImmOmFinalize(%llu) FAILED: %s", immOmHandleLocal, getOpenSAFErrorString(error));
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::finalizeImmHandle(): saImmOmFinalize(%llu) SUCCESS: %s", immOmHandleLocal, getOpenSAFErrorString(error));
		}
		immOmHandleLocal = 0;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}
/* OamSATranslator::getStructName method
 *
 * This method returns the name of the struct attribute that the saAttributeName/immRdn is a member/referee to.
 *
 */
std::string OamSATranslator::getStructName(char *immRdn, char *saAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::getStructName: ENTER");
	std::string stringimmRdn = immRdn;
	std::string subimmRdn = stringimmRdn.substr(0, 2);
	// Get the reference value
	std::string referenceValue = saAttributeName;
	if (ConvertToLowerCase(subimmRdn) == STRUCT_CLASS_KEY_ATTRIBUTE)
	{
		// Get the reference value, from the imm, if that is not provided
		std::string className, comClassName;
		referenceValue = getStructRefName(stringimmRdn, className, comClassName);
		// Remove the starting id part.
		stringimmRdn = stringimmRdn.substr((stringimmRdn.find_first_of(',')+1), stringimmRdn.length());
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::getStructName: RETURN <%s>", referenceValue.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return referenceValue;
}
/* OamSATranslator::fillStructMembers method
 *
 * This method read the value of the struct saAttributeName in IMM,
 * then it converts the values to a MafMoAttributeValueContainer_3T value.
 *
 * The method handles the case when the immRdn points to the struct object
 * OR the case when the immRdn points to the parent object with the reference to the struct
 * return true : if object exist
 * return false: if no object exist
 */
bool OamSATranslator::fillStructMembers(MafMoAttributeValueContainer_3T **structAttributePointer,
										char *immRdn, char *saAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Never used value I hope, must check this..
	MafOamSpiTransactionHandleT txHandle = 0;

	MafMoAttributeValueContainer_3T* container = NULL;
	MafMoAttributeValueContainer_3T** conv_result = &(container);
	//std::string my3GPPName = ImmRdn2MOTop(immRdn); // Unused variable
	//std::string				immDn; // Unused variable
	OamSACache::DNList		theDNList;

	// Create the 3GPP dn MafMoNamedAttributeValueContainer_3T
	// Strip of any initial id string
	std::string stringimmRdn = immRdn;
	// Get the reference value
	std::string referenceValue = getStructName( immRdn, saAttributeName);
	// Mother object of the struct attribute, remove all parts belonging to the struct object here
	std::string subimmRdn = stringimmRdn.substr(0, 2);
	if (ConvertToLowerCase(subimmRdn) == STRUCT_CLASS_KEY_ATTRIBUTE)
	{
		size_t position = stringimmRdn.find_first_of(',');
		if (position != std::string::npos)
		{
			stringimmRdn = stringimmRdn.substr(stringimmRdn.find_first_of(',')+1);
		}
	}

	std::string moDn = ImmRdn2MOTop(stringimmRdn);

	MafReturnT	myRetVal = OamSAImmBridge::getOamSAImmBridge()->OamSAJoin(txHandle);
	if (myRetVal != MafOk)
	{
		(*structAttributePointer)->nrOfValues = 0;
		(*structAttributePointer)->type = MafOamSpiMoAttributeType_3_STRUCT;
		(*structAttributePointer)->values = NULL;
		return false;
	}
	MafReturnT myMafRetVal = OamSAImmBridge::getOamSAImmBridge()->getImmMoAttribute(txHandle, moDn.c_str(), referenceValue.c_str(), conv_result);
	if (MafOk == myMafRetVal)
	{
		if (container != NULL)
		{
			// allocate memory and copy all values over to the new Maf structure
			(*structAttributePointer)->nrOfValues = container->nrOfValues;
			(*structAttributePointer)->type = MafOamSpiMoAttributeType_3T(container->type);
			if (container->nrOfValues != 0) {
				(*structAttributePointer)->values = (MafMoAttributeValue_3T*)calloc(container->nrOfValues, sizeof(MafMoAttributeValue_3T));
			}
			else {
				(*structAttributePointer)->values = NULL;
			}

			// From COM
			MafMoAttributeValue_3T *pointerValueCom = container->values;
			// TO MAF
			MafMoAttributeValue_3T *pointerValueMaf = (*structAttributePointer)->values;
			// Number of values to copy over
			uint32_t nrOfValues = container->nrOfValues;
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::fillStructMembers nrOfValues <%i>", nrOfValues);
			int index = 0;
			while ((pointerValueCom != NULL) && (nrOfValues != 0))
			{
				// allocate memory
				if(pointerValueCom->value.structMember != NULL)
				{
					pointerValueMaf[index].value.structMember = (struct MafMoAttributeValueStructMember_3*)calloc(1, sizeof(struct MafMoAttributeValueStructMember_3));
				}
				else
				{
					pointerValueMaf[index].value.structMember = NULL;
				}
				struct MafMoAttributeValueStructMember_3 * structMemberMAf = pointerValueMaf[index].value.structMember;
				bool firstTime = true;
				// Loop over the struct members and copy them.
				struct MafMoAttributeValueStructMember_3 * structMember = pointerValueCom->value.structMember;
				while (structMember != NULL)
				{
					if (firstTime)
						firstTime = false;
					else
					{
						structMemberMAf->next = (struct MafMoAttributeValueStructMember_3*)calloc(1, sizeof(struct MafMoAttributeValueStructMember_3));
						structMemberMAf = structMemberMAf->next;
						structMemberMAf->next = NULL;
					}
					// Copy Data
					// Member Name
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::fillStructMembers copy memberName <%s>", structMember->memberName);
					structMemberMAf->memberName = (char*)calloc(strlen(structMember->memberName)+1, sizeof(char));
					strcpy(structMemberMAf->memberName, structMember->memberName);
					// Member Value
					structMemberMAf->memberValue = (struct MafMoAttributeValueContainer_3*)calloc(1, sizeof(struct MafMoAttributeValueContainer_3));
					structMemberMAf->memberValue->nrOfValues = structMember->memberValue->nrOfValues;
					structMemberMAf->memberValue->type = MafOamSpiMoAttributeType_3T(structMember->memberValue->type);
					if (structMember->memberValue->nrOfValues != 0)
					{
						structMemberMAf->memberValue->values = (MafMoAttributeValue_3T*)calloc(structMember->memberValue->nrOfValues, sizeof(MafMoAttributeValue_3T));
						MafMoAttributeValue_3T *valuesM = structMemberMAf->memberValue->values;
						MafMoAttributeValue_3T *valuesC = structMember->memberValue->values;
						uint32_t nrOfValuesInner = structMember->memberValue->nrOfValues;
						while (nrOfValuesInner != 0)
						{
							// Convert MafMoAttributeValue_3T values to MafMoAttributeValue_3T
							convertToMafValue(structMemberMAf->memberValue->type, valuesM, valuesC);
							--nrOfValuesInner;
							++valuesM;
							++valuesC;
						}
					}
					else
					{
						structMemberMAf->memberValue->values = NULL;
					}
					// Set next pointer to NULL
					structMemberMAf->next = NULL;
					structMember = structMember->next;
				}
				++pointerValueCom;
				//++pointerValueMaf;
				--nrOfValues;
				++index;
			}
		}
		else
		{
			// If NULL out then we set NULL to MAf structure as well
			(*structAttributePointer)->nrOfValues = 0;
			(*structAttributePointer)->type = MafOamSpiMoAttributeType_3_STRUCT;
			(*structAttributePointer)->values = NULL;
			return false;
		}
	}
	// Release resource and finish of nicely
	myRetVal = OamSAImmBridge::getOamSAImmBridge()->OamSAFinish(txHandle);
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}

/* OamSATranslator::isNotified method
 *
 * This method extract the class name for an immRdn object,
 * if the class name is a COM class.
 * The object need not to exist in the IMM.
 * Limitation of this function is that it do not work for a immRdn ending with id (a struct class)
 *
 * return className as the prefixed IMM name of the class
 * return comclassName as the name of class without any prefixing
 *
 * return true : if class exist
 * return false: if no class exist
 */
bool OamSATranslator::GetClassName(const char* immRdn,
									std::string& className,
									std::string& comclassName)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (immRdn == NULL)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassName:immRdn is NULL returning false");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassName:<immRdn><className><comclassName><%s><%s><%s>",immRdn,className.c_str(),comclassName.c_str());
	std::string immRdnTmp=immRdn;
	std::string rootImmKey = immRdn;
	std::string firstKey = immRdn;
	// Get the root imm key from immRdn by:
	// remove everything after the last ','
	// if no ',' in the given string then leave it as it is
	try {
		size_t pos_last_comma = rootImmKey.find_last_of(',');
		if (pos_last_comma != std::string::npos)
		{
			rootImmKey = rootImmKey.substr(pos_last_comma + 1);
			// remove everything after "="
			rootImmKey = rootImmKey.substr(0,rootImmKey.find('='));
		}
		else
		{
			// OK, we have this case : keyAttr=1 to retrive key from
			rootImmKey = rootImmKey.substr(0,rootImmKey.find('='));
		}
		// Retrieve the first element of Rdn
		size_t pos_first_comma = firstKey.find_first_of(',');
		if (pos_first_comma != std::string::npos)
		{
			firstKey = firstKey.substr(0,pos_first_comma + 1);
			firstKey = firstKey.substr(0,firstKey.find('='));
		}
		else
		{
			// OK, we have this case : keyAttr=1 to retrive key from
			firstKey = firstKey.substr(0,firstKey.find('='));
		}
	}catch (...) {
		// Ops core dump?
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassName:<rootImmKey><firstKey> <%s><%s>",rootImmKey.c_str(),firstKey.c_str());
	// get the MOM repo
	OamSAKeyMOMRepo* tempRepo = theKeyHolder.getMOMRepoImmKey(rootImmKey);
	// get the root class of the MOM
	std::string rootClass = tempRepo->getRootClass();
	comclassName = rootClass;
	if(rootClass == "")
	{
		// :if non_root_mo_notifications is enabled in coremw-com-sa.cfg NBI notifications for nonroot mos will be sent
		if(non_root_mo_notifications){
			if(rootImmKey == firstKey) // To Avoid infinite Looping of getclassName function call.
			{
				DEBUG_OAMSA_TRANSLATIONS("COM_DEBUG:GetClassName rootImmKey == firstKey returning false");
				return false;
			}
			std::string immRdnHead = nextDn(immRdnTmp);
			if(GetClassName(immRdnHead.c_str(),className,comclassName)){
				return true;
			}
			else{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassName returns false");
				return false;
			}
		}
		else{
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
	}
	if (rootImmKey == firstKey)
	{
		// Return the root class
		// But check if it needs to be prefixed
		if (tempRepo->isMomDecorated())
		{
			// Add the prefix here
			className = tempRepo->getMomName() + rootClass;
		}
		else
		{
			className = rootClass;
		}
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassName: RETURN<className><comclassName><%s><%s>",className.c_str(),comclassName.c_str());
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}
	// Get the class for the first element, add decoration if it is decorated
	className = tempRepo->getClassNameForKeyAttribute(firstKey);
	comclassName = className;
	// Not root class and then not is prefixed
	if (tempRepo->isMomDecorated())
	{
		// Add the prefix here
		className = tempRepo->getMomName() + className;
		DEBUG_OAMSA_TRANSLATIONS("COM_DEBUG:OamSATranslator::MOM is Decorated and classname is %s",className.c_str());
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassName: <className><comclassName><%s><%s>",className.c_str(),comclassName.c_str());

	if((!non_root_mo_notifications) && className.empty()){ //Legacy Behavior for non root MO notifications skipped if non_root_mo_notifictions is false or empty in coremw-com-sa.cfg
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): no MOM found, return false");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	else if (className.empty() || (comclassName.empty() && non_root_mo_notifications)){ //non root mo NBI notifications sent if non_root_mo_notifictions is set to true.
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator:: non_root_mo_notifictions set to true immRdnTmp is =%s",immRdnTmp.c_str());
		std::string immRdnHead = nextDn(immRdnTmp);
		if(GetClassName(immRdnHead.c_str(),className,comclassName)){
		   return true;
		}
		else{
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::GetClassName returns false");
			return false;
		}
	}
	else
		return true;
}

/* OamSATranslator::nextDn method
+ *
+ * This method parse the immRdn object, modifies and returns immRdnHead by removing last MOC
+ *
+ * If only one element left in the ImmRdn then that MOC will be returned.
+ */

std::string OamSATranslator::nextDn(std::string& immRdnHead)
{
	try {
		// Remove everything before last ,
		if (immRdnHead.find_last_of(',') != std::string::npos)
		{
			immRdnHead = immRdnHead.substr(0, immRdnHead.find_last_of(','));
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::nextDn returns immRdnHead=%s", immRdnHead.c_str());
		}
		return immRdnHead;
	}catch (...) {
		// Only to not core dump
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::nextFragment failed to clean string");
		return immRdnHead;
	}
}

/* OamSATranslator::isNotified method
 *
 * This method searches the storages if the given:
 *	  -attribute under the given class is notified (1.)
 *	  -or the given class (no attribute given) is notified (2.)
 *
 * possible calls:
 *	  1.: isNotified(immRdn, immClassName, attrName)
 *	  2.: isNotified(immRdn, immClassName)
 *
 * return true : if class/attr is notified
 * return false: if class/attr is not notified
 */
bool OamSATranslator::isNotified(const char *immRdn, const char *immClassName, const char *attrName)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): enter with immRdn (%s) className (%s) attrName (%s)",immRdn, immClassName, attrName);
	if(immRdn == NULL || immClassName == NULL)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): immRdn or className is NULL, return false");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	std::string rootImmKey;
	std::string imm_ClassName = immClassName;
	std::string  stringImmRdn = immRdn;
	OamSAKeyMOMRepo* tempRepo = NULL;
	bool res = false;

	// Try to acquire the correct MOM for the fragment
	// This happens when two root MOCs in EcimContainment belong to different MOMs.
	while(!stringImmRdn.empty())
	{
		// Get the root imm key from immRdn by:
		// remove everything after the last ','
		// if no ',' in the given string then leave it as it is
		rootImmKey = nextFragment(stringImmRdn);
		// remove everything before "="
		rootImmKey = retriveKey(rootImmKey);
		if(rootImmKey.empty())
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): faulty immRdn, return false");
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		// get the MOM repo
		tempRepo = theKeyHolder.getMOMRepoImmKey(rootImmKey);
		// get the root class of the MOM
		std::string rootClass = tempRepo->getRootClass();
		if(rootClass.empty())
		{
			// :if non_root_mo_notifications is enabled in coremw-com-sa.cfg NBI notifications for nonroot mos will be sent
		if(non_root_mo_notifications){
		    DEBUG_OAMSA_TRANSLATIONS("COM_DEBUG:OamSATranslator::isNotified(): non_root_mo_notifications is set true");
		    continue;
		}
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
		}
		DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): rootClass of the MOM: (%s)",rootClass.c_str());

		// by default set the class as non-decorated, so the undecoratedClass name is the imm_ClassName

		/**
		 * Check if the MOM is decorated:
		 * 1. If the MOM is decorated, then we need to search the key repo with the imm_Classname without the decoration
		 *
		 */
		if(tempRepo->isMomDecorated())
		{
			// get MOM name
			std::string momName = tempRepo->getMomName();
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): MOM is decorated, MOM name is (%s)",momName.c_str());
			try {
					// Remove the decorated part
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): imm_ClassName <%s>", imm_ClassName.c_str());
					size_t found = imm_ClassName.find(momName);
					if (found==std::string::npos)
					{
						// Ok, something is wrong here
						DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): undecorated class sent to a decorated MOM, try with another decorated MOM.");
						continue;
					}
					else
					{
						// the undecorated version will be
						imm_ClassName = imm_ClassName.substr(momName.length());
						DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): undecoratedClass is (%s)",imm_ClassName.c_str());
						res = tempRepo->isNotifiable(imm_ClassName.c_str(), attrName);
						if (!res)
						{
							res = isStructClassNotifiable(immRdn,attrName,tempRepo);
						}
						if(res)
						{
							DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): MOM is decorated and Notifiable, return true");
							break;
						}
						continue;
					}
				}catch (...)
				{
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): faulty imm_ClassName");
				}
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): MOM is not decorated");
			DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): undecoratedClass is (%s)",imm_ClassName.c_str());
			res = tempRepo->isNotifiable(imm_ClassName.c_str(), attrName);
			if(!res)
			{
				res = isStructClassNotifiable(immRdn,attrName,tempRepo);
			}
			if(res)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified(): MOM is not decorated but Notifiable, return true");
				break;
			}
		}
	}//while loop

	LEAVE_OAMSA_TRANSLATIONS();
	return res;
}

/**
 * OamSATranslator::isStructClassNotifiable method
 * This method Checks if this is a struct class:
 *   1. Check if the immRdn ends with id
 *	 If this is the case, then check the parent class for:
 *	   A. The referring struct reference and if that one is notified marked.
 *	   How to find the struct reference? I have to read it from IMM directly and
 *	   find the matching one using the value of the reference (the immRdn)
 *
 * If struct class is notified it will return true else false.
 */
bool OamSATranslator::isStructClassNotifiable(const char *immRdn, const char *attrName, OamSAKeyMOMRepo* tempRepo)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool res = false;
	std::string stringImmRdn = immRdn;
		try {
				if (ConvertToLowerCase(stringImmRdn.substr(0, 2)) == STRUCT_CLASS_KEY_ATTRIBUTE)
				{
					std::string classNameString = "";
					std::string comClassNameString = "";
					std::string structAttributeName = getStructRefName(stringImmRdn, classNameString,
																		comClassNameString, attrName);
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified: immRdn=<%s>", stringImmRdn.c_str());
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified: classNameString=<%s>", classNameString.c_str());
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified: comClassNameString=<%s>", comClassNameString.c_str());
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified: attrName=<%s>", attrName);
					DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::isNotified: structAttributeName=<%s>", structAttributeName.c_str());
					if (structAttributeName.empty())
					{
						// No not part of the struct
						LEAVE_OAMSA_TRANSLATIONS();
					}
					else
					{
						// OK, this is our reference, lets check if it is notified,
						// use the not prefixed com class name here
						LEAVE_OAMSA_TRANSLATIONS();
						res = tempRepo->isNotifiable(comClassNameString.c_str(), structAttributeName.c_str());
					}
				}
			}catch (...) {
				// OK, not a struct
				LEAVE_OAMSA_TRANSLATIONS();
			}
			return res;
}

/* OamSATranslator::IsDnListValid method
 *
 * This method searches the distinguished name for any
 * invalid comtainments.
 *
 * return true : if DN is valid
 * return false: if DN is invalid
 */
bool OamSATranslator::IsDnListValid(OamSACache::DNList& imm_name, const char *className)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::IsDnListValid className=%s", className);
	bool retVal = true;

	std::string childName(className);
	std::string parentName;
	std::string contName;

	OamSACache::DNListReverseIterator theIterRoot;

	for (theIterRoot = imm_name.rbegin(); theIterRoot != imm_name.rend(); ++theIterRoot)
	{
		parentName = *theIterRoot;
		parentName = parentName.substr(0, parentName.find("="));
		contName = parentName + containmentSeparator + childName;
		if (!theMOMRepository.isSplitImmDnValid(contName)) {
			retVal = false;
			break;
		}
		childName = parentName;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

MafReturnT OamSATranslator::checkObjectExistInImm(const std::string& objectName, SaImmHandleT immHandle, SaImmAccessorHandleT accessorHandle) {
	ENTER_OAMSA_TRANSLATIONS();
	static const char* const ClassNameAttribute = "SaImmAttrClassName";
	static const char* const AttributeArray[2] = {ClassNameAttribute, NULL};
	MafReturnT RetVal = MafFailure;
	SaNameT theObjectName;
	SaImmAttrValuesT_2** theAttrValues = NULL;
	saNameSet(objectName.data(), &theObjectName);
	if (accessorHandle != 0) {
		SaAisErrorT err = saImmOmAccessorGet_2(accessorHandle,
									&theObjectName,
									(char**)AttributeArray,
									&theAttrValues);
		if (SA_AIS_OK != err) {
			if (SA_AIS_ERR_NOT_EXIST == err) {
				RetVal = MafNotExist;
			}
			else {
				if(SA_AIS_ERR_BAD_HANDLE == err)
				{
					LOG_OAMSA_TRANSLATIONS("checkObjectExistInImm::saImmOmAccessorGet_2 returns SA_AIS_ERR_BAD_HANDLE, hence re-initializing handlers");
					if((err = reInitializeBadHandler(immHandle, accessorHandle)) == SA_AIS_OK)
					{
						LOG_OAMSA_TRANSLATIONS("reInitializeBadHandler returned SUCCESS, calling saImmOmAccessorGet_2 again with new handlers");
						err = saImmOmAccessorGet_2(accessorHandle,
								&theObjectName,
								(char**)AttributeArray,
								&theAttrValues);
						LOG_OAMSA_TRANSLATIONS("saImmOmAccessorGet_2, called after re-initializing handlers, returned retCode=%d",err);
						RetVal = (err == SA_AIS_OK) ? MafOk : MafFailure;
					}
					else
					{
						LOG_OAMSA_TRANSLATIONS("reInitializeBadHandler FAILED and returned retCode=%d",err);
						RetVal = MafFailure;
					}
				}
				else {
					RetVal = MafFailure;
				}
			}
		}
		else {
			RetVal = MafOk;
		}
	}
	saNameDelete(&theObjectName, false);
	return RetVal;
	LEAVE_OAMSA_TRANSLATIONS();
}
/**
 * Re-Initializes IMM handlers
 *
 * @param[in] immOmHandle               IMM handler to be re-initialized
 * @param[in] immOmAccessorHandle       Accessor handler to be re-initialized
 *
 * @return  The result of IMM re-initialization
 * @retval  SA_AIS_OK      if re-initialization went fine
 */
SaAisErrorT OamSATranslator::reInitializeBadHandler(SaImmHandleT immOmHandle, SaImmAccessorHandleT immOmAccessorHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
        DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::reInitializeBadHandler ENTER");

        SaVersionT immVersion = { immReleaseCode, immMajorVersion, immMinorVersion };

        /* Try to clean up before re-initializing */
        (void)saImmOmAccessorFinalize(immOmAccessorHandle);
        (void)saImmOmFinalize(immOmHandle);


        /* Try to initialize IMM handler */
        SaAisErrorT retCode = saImmOmInitialize(&immOmHandle, NULL, &immVersion);

        if(SA_AIS_OK == retCode)
        {
                /* Try to initialize Accessor handler */
		DEBUG_OAMSA_TRANSLATIONS("saImmOmInitialize reInitialize SUCCESS");
		retCode = saImmOmAccessorInitialize(immOmHandle, &immOmAccessorHandle);
        }
        DEBUG_OAMSA_TRANSLATIONS("OamSATranslator::reInitializeBadHandler LEAVE with retCode=%d", retCode);
	LEAVE_OAMSA_TRANSLATIONS();
        return retCode;
}
