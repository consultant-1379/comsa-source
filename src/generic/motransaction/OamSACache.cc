/**
 *   OamSACache   Copyright (C) 2010 by Ericsson AB
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
 *   File:   OamSACache.cc
 *
 *   Author: egorped
 *
 *   Date:   2010-05-21
 *
 *   This file to implement the per transaction storage.
 *
 *   Reviewed: efaiami 2010-06-29
 *
 *   Modify: efaiami 2011-02-22 for log and trace function
 *   Modify: eaparob 2012-05-23 new function added: convertToImm
 *   Modify: uabjoy  2012-05-25 corrected task29690
 *   Modify: xnikvap, xngangu, xjonbuc, xduncao 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify: uabjoy  2012-09-18 Correcting trouble report HQ23654
 *   Modify: xjonbuc 2013-08-20 Correcting cache for splitImmDn for MR-26712
 *   Modify: eaparob 2013-10-22 Duplicate function "GetClassNameFromImm()" removed, common function (from the translator code) called instead
 *   Modify: xadaleg 2014-08-02 MR35347 - increase DN length
 *   Modify: xnikvap 2015-08-02 MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 ****************************************************************************************/

#include <stdlib.h>
#include <cassert>
#include "ComSA.h"
#include "OamSAImmBridge.h"
#include "OamSACache.h"
#include "OamSADataClasses.h"
#include "OamSATranslator.h"
#include "OamSATranslateImmName.h"

/**
 * Preprocessor constants.
 */

/**
 *  External references
 */
extern OamSATranslator	theTranslator;

/**
 *  Global declarations
 */
OamSADataFactory	theDataFactory;

void handleCeaseAlarms(std::string& my3GPPName);

/* Local */
/**
 * 	Prints the value of an attribute value. For debugging purposes.
 */
void PrintAttributeValues(SaImmAttrValuesT_2& AttrValues)
{
	unsigned int Index;
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("Attribute Name %s number of values %d", AttrValues.attrName, AttrValues.attrValuesNumber);
	switch(AttrValues.attrValueType)
	{
	case SA_IMM_ATTR_SAINT32T:
		DEBUG_OAMSA_TRANSLATIONS(" type SAINT32");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Value no = %u value = %d", Index,*(SaInt32T*)AttrValues.attrValues[Index] );
		}
		break;

	case SA_IMM_ATTR_SAUINT32T:
		DEBUG_OAMSA_TRANSLATIONS(" type SAUINT32");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Value no = %u value = %u", Index,*(SaUint32T*)AttrValues.attrValues[Index] );
		}
		break;

	case SA_IMM_ATTR_SAINT64T:
		DEBUG_OAMSA_TRANSLATIONS(" type SAINT64");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Value no = %u value = %lld", Index,*(SaInt64T*)AttrValues.attrValues[Index] );
		}
		break;

	case SA_IMM_ATTR_SAUINT64T:
		DEBUG_OAMSA_TRANSLATIONS(" type SAUINT64");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Value no = %u value = %llu", Index,*(SaUint64T*)AttrValues.attrValues[Index] );
		}
		break;

	case SA_IMM_ATTR_SATIMET:
		DEBUG_OAMSA_TRANSLATIONS(" type SATIME");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			// TBD DEBUG_OAMSA_TRANSLATIONS("Value no = %d value = %d", Index,*(SaInt32T*)AttrValues.attrValues[Index] );
		}
		break;

	case SA_IMM_ATTR_SANAMET:
		DEBUG_OAMSA_TRANSLATIONS(" type SANAMET");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			SaNameT*	namet_p = (SaNameT*)AttrValues.attrValues[Index];
			unsigned len = saNameLen(namet_p);
			char* namestr = new char[len + 1];
			memcpy(namestr, saNameGet(namet_p), len);
			namestr[len] = '\0';
			DEBUG_OAMSA_TRANSLATIONS("Value no = %u value = %s", Index, namestr);
			delete [] namestr;
		}
		break;

	case SA_IMM_ATTR_SAFLOATT:
		DEBUG_OAMSA_TRANSLATIONS(" type SAFLOAT");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Value no = %u value = %f", Index,*(SaFloatT*)AttrValues.attrValues[Index] );
		}
		break;

	case SA_IMM_ATTR_SADOUBLET:
		DEBUG_OAMSA_TRANSLATIONS(" type SADOUBLE");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Value no = %u value = %lf", Index,*(SaDoubleT*)AttrValues.attrValues[Index] );
		}
		break;

	case SA_IMM_ATTR_SASTRINGT:
		DEBUG_OAMSA_TRANSLATIONS(" type SASTRING");
		for (Index = 0; Index < AttrValues.attrValuesNumber; Index++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Value no = %u value = %s", Index,*(char**)AttrValues.attrValues[Index] );
		}

		break;

	case SA_IMM_ATTR_SAANYT:
		DEBUG_OAMSA_TRANSLATIONS(" type SAANYT");
		break;

	default:
		ERR_OAMSA_TRANSLATIONS("Unknown SA_IMM_ATTR type %d", AttrValues.attrValueType);
		break;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  Helper function to delete values pointged to by the universal void*
 */
static void DeleteVoidPointerValue(void* ptr, SaImmValueTypeT value_t)
{
	ENTER_OAMSA_TRANSLATIONS();
	switch(value_t)
	{
	case SA_IMM_ATTR_SAINT32T:
		delete (SaInt32T*) ptr;
		break;

	case SA_IMM_ATTR_SAUINT32T:
		delete (SaUint32T*) ptr;
		break;

	case SA_IMM_ATTR_SAINT64T:
		delete (SaInt64T*) ptr;
		break;

	case SA_IMM_ATTR_SAUINT64T:
		delete (SaUint64T*) ptr;
		break;

	case SA_IMM_ATTR_SATIMET:
		delete (SaTimeT*) ptr;
		break;

	case SA_IMM_ATTR_SANAMET:
		saNameDelete((SaNameT*) ptr, true);
		break;

	case SA_IMM_ATTR_SAFLOATT:
		delete (SaFloatT*) ptr;
		break;

	case SA_IMM_ATTR_SADOUBLET:
		delete (SaDoubleT*) ptr;
		break;

	case SA_IMM_ATTR_SASTRINGT:
		{
			SaStringT strp = *(SaStringT*)ptr;
			delete [] strp;
			delete (SaStringT**)ptr;
		}
		break;

	case SA_IMM_ATTR_SAANYT:
		{
			SaAnyT*	anyp = (SaAnyT*)ptr;
			delete  anyp->bufferAddr;
			delete  anyp;
		}
		break;

	default:
		ERR_OAMSA_TRANSLATIONS("Unknown SA_IMM_ATTR type %d", value_t);
		break;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  Copies and allocates memory for the target data
 */
static void** CopyAllocAttributeValueArray(const void* source[], const SaImmValueTypeT value_t, const unsigned int NoOfValues)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (0 == NoOfValues)
	{
		return NULL;
	}
	void** vp = new void*[NoOfValues+1];
	for (unsigned int i = 0; i < NoOfValues; i++)
	{
		if (NULL == source[i])
			continue;
		switch(value_t)
		{
		case SA_IMM_ATTR_SAINT32T:
			{
				SaInt32T*	tp = new SaInt32T;
				SaInt32T*	sp = (SaInt32T*)source[i];
				*tp = *sp;
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SAUINT32T:
			{
				SaUint32T*	tp = new SaUint32T;
				SaUint32T*	sp = (SaUint32T*)source[i];
				*tp = *sp;
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SAINT64T:
			{
				SaInt64T*	tp = new SaInt64T;
				SaInt64T*	sp = (SaInt64T*)source[i];
				*tp = *sp;
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SAUINT64T:
			{
				SaUint64T*	tp = new SaUint64T;
				SaUint64T*	sp = (SaUint64T*)source[i];
				*tp = *sp;
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SATIMET:
			{
				SaTimeT*	tp = new SaTimeT;
				SaTimeT*	sp = (SaTimeT*)source[i];
				*tp = *sp;
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SANAMET:
			{
				SaNameT*	tp = new SaNameT;
				SaNameT*	sp = (SaNameT*)source[i];
				saNameSet(saNameGet(sp), tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SAFLOATT:
			{
				SaFloatT*	tp = new SaFloatT;
				SaFloatT*	sp = (SaFloatT*)source[i];
				*tp = *sp;
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SADOUBLET:
			{
				SaDoubleT*	tp = new SaDoubleT;
				SaDoubleT*	sp = (SaDoubleT*)source[i];
				*tp = *sp;
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SASTRINGT:
			// Assume that this really is SaStringType i.e. *char
			{
				SaStringT*	strp = new SaStringT;
				SaStringT	sp = *(SaStringT*)source[i];
				SaStringT	tp = new char[strlen(sp)+1];
				if (tp != NULL)
				{
					strcpy(tp,sp);
				}
				*strp = tp;
				vp[i] = strp;
			}
			break;

		case SA_IMM_ATTR_SAANYT:
			{
				SaAnyT*	tp = new SaAnyT;
				SaAnyT*	sp = (SaAnyT*)source[i];
				tp->bufferAddr = new SaUint8T[sp->bufferSize];
				if (NULL != tp->bufferAddr)
				{
					memcpy(tp->bufferAddr, sp->bufferAddr, 	sp->bufferSize);
					tp->bufferSize = sp->bufferSize;
				}
				else
				{
					delete tp;
					tp = NULL;
				}
				vp[i] = tp;
			}
			break;

		default:
			ERR_OAMSA_TRANSLATIONS("Unknown SA_IMM_ATTR type %d", value_t);
			break;
		}
	}
	vp[NoOfValues] = NULL;
	LEAVE_OAMSA_TRANSLATIONS();
	return vp;
}

/**
 * 	Creates an array of data elements from a Data container. Output in something that's understandable to IMM
 *  and the input is really something from COM
 */
static void** CreateAttributeValueArray(OamSADataContainer& doc, SaImmValueTypeT& value_t, unsigned int &NoOfValues)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSADataContainer::DataPointerListIterator	dpli;
	NoOfValues = doc.NoOfDataElements();

	// New code to handle the case where a multi value attribute is written with 0 values in order to
	// empty it. Wasn't handled OK before

	if (0 == NoOfValues)
	{
		value_t = doc.GetImmValueType();
		LEAVE_OAMSA_TRANSLATIONS();
		return NULL;
	}

	void** vp = new void*[doc.NoOfDataElements()+1];
	bool 	NotDone = doc.GetFirstData(dpli);

	for (int i = 0;NotDone; NotDone = doc.GetNextData(dpli), i++)
	{
		OamSADataClass* dcp = *dpli;
		value_t = dcp->GetSaImmValueType();
		switch(value_t)
		{
		case SA_IMM_ATTR_SAINT32T:
			{
				SaInt32T*	tp = new SaInt32T;
				dcp->GetImmValue(tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SAUINT32T:
			{
				SaUint32T*	tp = new SaUint32T;
				dcp->GetImmValue(tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SAINT64T:
			{
				SaInt64T*	tp = new SaInt64T;
				dcp->GetImmValue(tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SAUINT64T:
			{
				SaUint64T*	tp = new SaUint64T;
				dcp->GetImmValue(tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SATIMET:
			{
				SaTimeT*	tp = new SaTimeT;
				dcp->GetImmValue(tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SANAMET:
			{
				SaNameT*	tp = new SaNameT;
				dcp->GetImmValue(tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SAFLOATT:
			{
				SaFloatT*	tp = new SaFloatT;
				dcp->GetImmValue(tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SADOUBLET:
			{
				SaDoubleT*	tp = new SaDoubleT;
				dcp->GetImmValue(tp);
				vp[i] = tp;
			}
			break;

		case SA_IMM_ATTR_SASTRINGT:
			// Assume that this really is SaStringType i.e. *char
			{
				SaStringT*	strp = new SaStringT;
				SaStringT	tp = new char[dcp->Size()+1];
				if (tp != NULL)
				{
					dcp->GetImmValue(tp);
					DEBUG_OAMSA_TRANSLATIONS("CreateAttributeValueArray stringtype %s length %d ",tp, dcp->Size());
				}
				*strp = tp;
				vp[i] = strp;
			}
			break;

		case SA_IMM_ATTR_SAANYT:
			{
				// Not used
				DEBUG_OAMSA_TRANSLATIONS("CreateAttributeValueArray: SA_IMM_ATTR_SAANYT not handled");
			}

			break;

		default:
			ERR_OAMSA_TRANSLATIONS("Unknown SA_IMM_ATTR type %d", value_t);
			break;

		}
	}
	vp[NoOfValues] = NULL;
	LEAVE_OAMSA_TRANSLATIONS();
	return vp;
}

void GlobalSplitDN(const std::string& theDN, OamSACache::DNList& theSplitDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	size_t pos;
	size_t oldpos = 0;


	DEBUG_OAMSA_TRANSLATIONS("Split DN input string %s", theDN.c_str());


	for (pos = theDN.find_first_of(',',oldpos) ;pos != std::string::npos; pos = theDN.find_first_of(',',oldpos))
	{
		theSplitDN.push_back(theDN.substr(oldpos,pos-oldpos));
		oldpos = pos + 1;
	}
	theSplitDN.push_back(theDN.substr(oldpos,theDN.size()-oldpos));

	LEAVE_OAMSA_TRANSLATIONS();
}
// This is a helper function handling string manipulations
std::string removeIdInIMM(char* immRdn)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string dnCleanFromStruct = immRdn;
	std::string tempString;
	// Search first part of IMM RDN for struct references and remove it.
	// id=measuremenSpecification=MR_1,aid=2,rootId=1
	// this case id=1 WILL return id=1
	try {
		// check for 'id='
		tempString = dnCleanFromStruct.substr(0, dnCleanFromStruct.find_first_of(','));
		std::string Identifier = std::string(STRUCT_CLASS_KEY_ATTRIBUTE) + "=";
		size_t position = tempString.find(Identifier.c_str());
		if ( position != std::string::npos && position == 0)
		{
			// Return string without first part included
			dnCleanFromStruct = immRdn;
			dnCleanFromStruct = dnCleanFromStruct.substr(dnCleanFromStruct.find_first_of(',')+1);
		}
	}catch (...) {
		LEAVE_OAMSA_TRANSLATIONS();
		return dnCleanFromStruct;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return dnCleanFromStruct;
}

bool getClassName(char* immRdn, char** className)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string classNameString;
	std::string dummy;
	bool returnValue = false;
	// First remove any occurrences of id in the immRdn, because this function shall NOT return the class name of the struct class!
	std::string tempimmRdn = removeIdInIMM(immRdn);
	DEBUG_OAMSA_TRANSLATIONS("getClassName: tempimmRdn=<%s>", tempimmRdn.c_str());
	if (theTranslator.GetClassName(tempimmRdn.c_str(), classNameString, dummy))
	{
		*className = (char*)calloc((classNameString.length() + 1), sizeof(char));
		strcpy(*className, classNameString.c_str());
		returnValue = true;
	}
	else
	{
		*className = NULL;
		returnValue = false;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return returnValue;
}

bool  fillStructMembers(MafMoAttributeValueContainer_3T **structAttributePointer, char *immRdn, char *saAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("fillStructMembers: ENTER <%p><%s><%s>", structAttributePointer, immRdn, saAttributeName);
	LEAVE_OAMSA_TRANSLATIONS();
	return theTranslator.fillStructMembers(structAttributePointer, immRdn, saAttributeName);
}
// Clean a dn from struct references at the end of the 3GPP dn.
bool removeStructPartOfDn(char **out, const char **dn)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string dnCleanFromStruct = *dn;
	// Search last part of DN for struct reference.
	// A=1,test.id=1
	// OR
	// B=1,id=lll
	try {
		dnCleanFromStruct = dnCleanFromStruct.substr(dnCleanFromStruct.find_last_of(','));
		size_t lengthOfReminder = dnCleanFromStruct.length();
		// check for id or .id
		size_t positionType1 = dnCleanFromStruct.find(".id=");
		size_t positionType2 = dnCleanFromStruct.find("id=");
		if ( positionType1 != std::string::npos || positionType2 != std::string::npos)
		{
			// Return string without last part included
			dnCleanFromStruct = *dn;
			dnCleanFromStruct = dnCleanFromStruct.substr(0, (dnCleanFromStruct.length()-lengthOfReminder));
			*out = (char*)calloc(dnCleanFromStruct.length()+1, sizeof(char));
			strcpy(*out, dnCleanFromStruct.c_str());
			LEAVE_OAMSA_TRANSLATIONS();
			return true;
		}
	}catch (...) {
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}

char *getStructAttributeName(char *immRdn, char *saAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("getStructAttributeName: ENTER <%s><%s>", immRdn, saAttributeName);
	// Get the reference value
	std::string structName = theTranslator.getStructName(immRdn, saAttributeName);
	if ( structName.empty() )
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return NULL;
	}
	else
	{
		char *returnValue = (char*) calloc(structName.length()+1,sizeof(char));
		strcpy(returnValue, structName.c_str());
		LEAVE_OAMSA_TRANSLATIONS();
		return returnValue;
	}
}

bool  isStructAttribute(char *immRdn, char *saAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("isStructAttribute: ENTER <%s><%s>", saAttributeName, immRdn);
	std::string moDn = theTranslator.ImmRdn2MOTop(immRdn);
	OamSACache::DNList mo_name;
	GlobalSplitDN(moDn, mo_name);
	bool returnValue = theTranslator.isStructAttribute(mo_name, saAttributeName);
	DEBUG_OAMSA_TRANSLATIONS("isStructAttribute: LEAVE <%i>", returnValue);
	LEAVE_OAMSA_TRANSLATIONS();
	return returnValue;
}

bool  isNotified(char* attributename, char *classname, char* immRdn)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("isNotified: ENTER <%s><%s><%s>", attributename, classname, immRdn);
	bool returnValue;
	if (attributename == NULL)
	{
		returnValue = theTranslator.isNotified(immRdn, classname);
	}
	else
	{
		returnValue = theTranslator.isNotified(immRdn, classname, attributename);
	}
	DEBUG_OAMSA_TRANSLATIONS("isNotified: LEAVE <%i>", returnValue);
	LEAVE_OAMSA_TRANSLATIONS();
	return returnValue;
}

MafOamSpiMoAttributeType_3T  getTypeForStructMemberAttribute(char *attributename, char* membername, char *immRdn)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string immRdnString = immRdn;
	OamSACache::DNList	splitImmName;
	// FIXME :  How shall this we handled in the future?
	// OK, the translation must have the id object removed to work!
	try {
		// Remove remove the first element (the id object)
		if (immRdnString.find_first_of(',') != std::string::npos)
		{
			immRdnString = immRdnString.substr((immRdnString.find_first_of(',') + 1));
		}
	}catch (...) {
		// Only to not core dump, fail this return
		return MafOamSpiMoAttributeType_3T(0);
	}
	DEBUG_OAMSA_TRANSLATIONS("getTypeForStructMemberAttribute <%s><%s><%s>", attributename, membername, immRdn);
	std::string my3GPPName = theTranslator.ImmRdn2MOTop(immRdnString);
	std::string attributeNameString = attributename;
	std::string membernameString = membername;

	OamSACache::DNList the3GPPDn;
	GlobalSplitDN(my3GPPName, the3GPPDn);

	MafOamSpiMoAttributeType_3 attrType;
	bool success = theTranslator.getStructMemberComDatatype( the3GPPDn, attributeNameString, membernameString, &attrType);
	//MafOamSpiMoAttributeType_3 attrType = theTranslator.GetComAttributeType(the3GPPDn, attributeNameString);
	// Convert to the Maf type that is the same integer values of the enum.
	MafOamSpiMoAttributeType_3T returnValue;
	if (success)
		returnValue = MafOamSpiMoAttributeType_3T(attrType);
	else
		returnValue = MafOamSpiMoAttributeType_3T(0);

	LEAVE_OAMSA_TRANSLATIONS();
	return returnValue;

}

MafOamSpiMoAttributeType_3T  getTypeForAttribute(char* attributename, char* immRdn)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string immRdnString = immRdn;
	MafOamSpiMoAttributeType_3 attrType = (MafOamSpiMoAttributeType_3)0;
	OamSACache::DNList	splitImmName;
	DEBUG_OAMSA_TRANSLATIONS("getTypeForAttribute <%s><%s>", attributename, immRdn);
	std::string my3GPPName = theTranslator.ImmRdn2MOTop(immRdnString);
	if (strcmp(my3GPPName.c_str(), "invalidDn") != 0) {
		std::string attributeNameString = attributename;
		OamSACache::DNList the3GPPDn;
		GlobalSplitDN(my3GPPName, the3GPPDn);
		attrType = theTranslator.GetComAttributeType(the3GPPDn, attributeNameString);
	}
	else {  //Incase the Translator is not able to retrieve MOTop.
		DEBUG_OAMSA_TRANSLATIONS("getTypeForAttribute my3GPPName received invalidDn. returning 0 as attribuite type");
	}
        // Convert to the Maf type that is the same integer values of the enum.
	MafOamSpiMoAttributeType_3T returnValue = MafOamSpiMoAttributeType_3T(attrType);
	LEAVE_OAMSA_TRANSLATIONS();
	return returnValue;
}

void GlobalSplitMocPath(const std::string& mocPath, OamSACache::MocPathList& theSplitPath) {
	ENTER_OAMSA_TRANSLATIONS();

	if(mocPath.length() <= 0) {
		DEBUG_OAMSA_TRANSLATIONS("MocPath is empty");
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}

	//	Check if mocPath starts with '/'
	if(mocPath[0] != '/') {
		ERR_OAMSA_TRANSLATIONS("MocPath does not start with '/'");
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}

	DEBUG_OAMSA_TRANSLATIONS("Split MocPath input string %s", mocPath.c_str());

	size_t pos;
	size_t oldpos = 1;

	for(pos=mocPath.find_first_of('/',oldpos); pos!=std::string::npos; pos=mocPath.find_first_of('/',oldpos)) {
		theSplitPath.push_back(mocPath.substr(oldpos,pos-oldpos));
		oldpos = pos + 1;
	}

	theSplitPath.push_back(mocPath.substr(oldpos,mocPath.size()-oldpos));

	LEAVE_OAMSA_TRANSLATIONS();
}

/* Globals */
char* convertTo3GPPDNCOMCASE(char *immRdn)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("convertTo3GPPDNCOMCASE: ENTER <%s>", immRdn);
	std::string stringImmRdn = immRdn;
	std::string dn = theTranslator.ImmRdn2MOTop(stringImmRdn);
	char* charPointer = (char*) calloc(dn.length()+1,sizeof(char));
	if (charPointer!=NULL)
	{
		strcpy(charPointer, dn.c_str());
		DEBUG_OAMSA_TRANSLATIONS("convertTo3GPPDNCOMCASE: LEAVE <%s>", charPointer);
		LEAVE_OAMSA_TRANSLATIONS();
		return charPointer;
	}
	DEBUG_OAMSA_TRANSLATIONS("convertTo3GPPDNCOMCASE: LEAVE <NULL>");
	LEAVE_OAMSA_TRANSLATIONS();
	return NULL;
}

void modifyDN(std::string& str2replace, std::string& newStr, std::string& my3GPPName)
{
	size_t startPos = my3GPPName.find(str2replace);
	if (startPos != std::string::npos)
	{
		my3GPPName.replace(startPos, str2replace.length(), newStr);
		DEBUG_OAMSA_TRANSLATIONS("Modified DN = %s", my3GPPName.c_str());
	}
}

void handleCeaseAlarms(std::string& my3GPPName)
{
	DEBUG_OAMSA_TRANSLATIONS("handleCeaseAlarms: Input string %s", my3GPPName.c_str());
	std::string str2replace = ",SafSi=";
	if(my3GPPName.find(str2replace)!= std::string::npos)
	{
		std::string newStr = ",SaAmfSI.safSi=";
		// Special case to handle cease alarm when the safSi object is deleted
		modifyDN(str2replace,newStr, my3GPPName);
		str2replace ="SafApp=";
		newStr = "SaAmfApplication.safApp=";
		// Special case to handle cease alarm when the safApp object is deleted
		modifyDN(str2replace, newStr, my3GPPName);
	}
	str2replace = ",SafNode=";
	std::string newStr = ",SaClmNode.safNode=";
	// Special case to handle cease alarm when the safNode object is deleted
	modifyDN(str2replace,newStr, my3GPPName);
}

char* convertTo3Gpp(SaNameT*	theImmName_p)
{
	ENTER_OAMSA_TRANSLATIONS();

	char* retStr_p = NULL;
	if (theImmName_p != NULL)
	{
		std::string 		myImmName(saNameGet(theImmName_p), saNameLen(theImmName_p));
		OamSACache::DNList	mySplitImmName;
		std::string			my3GPPName;
		DEBUG_OAMSA_TRANSLATIONS("convertTo3Gpp: ENTER <%s>", myImmName.c_str());


		GlobalSplitDN(myImmName, mySplitImmName);

		if (theTranslator.Imm2MO_DN(0, mySplitImmName, my3GPPName))
		{
			handleCeaseAlarms(my3GPPName);
			if(!strstr(my3GPPName.c_str(),"ManagedElement="))
			{
				retStr_p = (char*) calloc(my3GPPName.length()+18,sizeof(char));
			}
			else
			{
				retStr_p = (char*) calloc(my3GPPName.length()+1,sizeof(char));

			}
			if (retStr_p != NULL)
			{
				if(!strstr(my3GPPName.c_str(),"ManagedElement="))
				{
					strncpy(retStr_p,"ManagedElement=1,", 17);
					strncat(retStr_p, my3GPPName.c_str(), my3GPPName.length());
					DEBUG_OAMSA_TRANSLATIONS("retStr_p = %s", retStr_p);
				}
				else
				{
					// OK, copy source as it is into the source field
					strncpy(retStr_p, my3GPPName.c_str(),my3GPPName.length());
				}
			}
		}
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return retStr_p;
}

/* Globals */
bool convertToImm(const char *the3GppDN, SaNameT *immDn)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool retVal = false;

	if(the3GppDN && immDn) {
		char *dn = NULL;

		theTranslator.MO2Imm_DN(the3GppDN, &dn);
		unsigned int dnLen = strlen(dn);
		if(dnLen <= saNameMaxLen()) {
			saNameSet(dn, immDn);
			retVal = true;
		}
		else {
			ERR_OAMSA_TRANSLATIONS("convertToImm() DN length too long:  %d > max(%u) %s", dnLen, saNameMaxLen(), dn);
		}

		delete[] dn;
	}
	DEBUG_OAMSA_TRANSLATIONS("convertToImm() retVal=%d", int(retVal));
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 * 	Local classes
 */
/**
 *  OamSAModified Entry
 *  This class is one of the keystones in this software module. It's a leaf in the three structure which comprise the
 *  cache. There is one for each name element (in the names of objects stored in the cache),
 *  regardless if there is a corresponding object for this specific leaf. The member myEntryKind indicates what kind of
 *  node each leaf is.
 *
 *  The map myNameMap contains a list of descendants [objects] (which leads to the correct conclusion that the tree structure
 *  isn't binary and might be totally unbalanced)
 *
 *  The map myAttributeMap contains a list of attributes to the node and is present (non empty) only in those nodes
 *  which actually refers to objects modified or created.
 */
class OamSAModifiedEntry
{
public:
	typedef enum {eModifiedEntryNoKind	= 0,
					eModifiedEntryDeletedObject = 1,
					eModifiedEntryAlteredObject = 2,
					eModifiedEntryNewObject = 3}
					eModifiedEntryKind;

	typedef std::list <std::string> OamSAModifiedList;
	typedef std::list <std::string>::iterator OamSAModifiedListIterator;
	typedef std::map <std::string, OamSAModifiedEntry> OamSAModifiedMap;
	typedef std::map <std::string, OamSAModifiedEntry>::iterator OamSAModifiedMapIterator;
	typedef std::pair < OamSAModifiedMapIterator, bool> OamSAModifiedMapRetVal;
	typedef std::map <std::string, OamSADataContainer> OamSAAttributeMap;
	typedef std::map <std::string, OamSADataContainer>::iterator OamSAAttributeMapIterator;
	typedef std::pair < OamSAAttributeMapIterator, bool> OamSAAttributeMapRetVal;

	// Constructors
	OamSAModifiedEntry() : myEntryKind(eModifiedEntryNoKind){ ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); }

	// Destructor
	~OamSAModifiedEntry() { ENTER_OAMSA_TRANSLATIONS(); myNameMap.clear(); myNameMapInsertionOrdered.clear(); myAttributeMap.clear();myImmName.clear();myParentName.clear();myClass.clear(); LEAVE_OAMSA_TRANSLATIONS();}


	// Public methods
	OamSAModifiedMapRetVal 	AddName(const std::string& dnsubpart);
	OamSAAttributeMapRetVal AddAttribute(const std::string& dnsubpart);
	bool					AddAttributeValue(const std::string& dnsubpart, OamSADataClass* datac_p);
	bool					AddAttributeValue(const std::string& dnsubpart, OamSADataClass* datac_p, SaImmValueTypeT theValueType);
	bool					AddEmptyAttributeValue(const std::string& dnsubpart,MafOamSpiMoAttributeType_3T type,SaImmValueTypeT theValueType,bool defaultvalue);
	bool					AddEmptyAttributeValue(const std::string& dnsubpart,MafOamSpiMoAttributeType_3T type,bool defaultvalue);

	OamSAAttributeMapRetVal		GetAttributeValue(const std::string& attrname);
	OamSAModifiedMapRetVal  Find(const std::string& dnsubpart);
	bool						IsEmpty();
	void						Clear();
	unsigned int			NoOfAttributes()  { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myAttributeMap.size();}
	OamSAAttributeMap&		GetAttributeMap() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myAttributeMap; }
	OamSAModifiedMap&		GetNameMap() 		{ ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myNameMap; }
	OamSAModifiedList&		GetOrderedNameList() 		{ ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myNameMapInsertionOrdered; }
	OamSAModifiedEntry&		GetModifiedEntry(const std::string& className) 		{ ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myNameMap[className]; }
	void					SetClassName(const std::string& theClassName) { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); myClass = theClassName; }
	std::string&			GetClassName() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myClass; }
	void					SetImmName(const std::string& theImmName) { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); myImmName = theImmName;}
	std::string&			GetImmName() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myImmName; }
	void					SetParentName(const std::string& theParentName) { ENTER_OAMSA_TRANSLATIONS(); myParentName = theParentName; LEAVE_OAMSA_TRANSLATIONS();}
	std::string&			GetParentName() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myParentName; }
	void					SetEntryKind(eModifiedEntryKind theKind) { ENTER_OAMSA_TRANSLATIONS(); if (theKind > myEntryKind) myEntryKind = theKind; LEAVE_OAMSA_TRANSLATIONS(); }
	void					ClearEntryKind() { ENTER_OAMSA_TRANSLATIONS(); myEntryKind = eModifiedEntryNoKind; LEAVE_OAMSA_TRANSLATIONS(); }
	bool					IsNewObject() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return (myEntryKind == eModifiedEntryNewObject); }
	bool					IsModifiedObject() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return (myEntryKind == eModifiedEntryAlteredObject); }
	bool					IsInvalidObject() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return (myEntryKind == eModifiedEntryNoKind); }


private:
	eModifiedEntryKind	myEntryKind;
	OamSAModifiedMap	myNameMap;
	OamSAModifiedList	myNameMapInsertionOrdered;    // A list of names in myNameMap ordered by the insertion order.
	OamSAAttributeMap	myAttributeMap;
	std::string			myParentName;
	std::string			myClass;
	std::string			myImmName;
};

/**
 * OamSAModifiedEntry::AddName(const std::string& dnsubpart)
 */
OamSAModifiedEntry::OamSAModifiedMapRetVal OamSAModifiedEntry::AddName(const std::string& dnsubpart)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSAModifiedEntry	theEntry;
	OamSAModifiedEntry::OamSAModifiedMapRetVal retVal;

	retVal =  myNameMap.insert(std::pair<std::string, OamSAModifiedEntry>(dnsubpart,theEntry));

	if (retVal.second==true)
	{
		// A unique new element was added to myNameMap, so insert into front of ordered list
		myNameMapInsertionOrdered.push_front(dnsubpart);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

/**
 * OamSAModifiedEntry::AddAttribute(const std::string& dnsubpart)
 */
OamSAModifiedEntry::OamSAAttributeMapRetVal OamSAModifiedEntry::AddAttribute(const std::string& dnsubpart)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSADataContainer	theEntry;
	LEAVE_OAMSA_TRANSLATIONS();
	return myAttributeMap.insert(std::pair<std::string, OamSADataContainer>(dnsubpart,theEntry));
}

/**
 * OamSAModifiedEntry::AddAttributeValue(const std::string& dnsubpart, OamSADataClass* datac_p)
 */
bool OamSAModifiedEntry::AddAttributeValue(const std::string& dnsubpart, OamSADataClass* datac_p)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	OamSAAttributeMapIterator iter = myAttributeMap.find(dnsubpart);
	if (iter != myAttributeMap.end())
	{
		iter->second.Add(datac_p);
		RetVal = true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

/**
 * OamSAModifiedEntry::AddAttributeValue(const std::string& dnsubpart, OamSADataClass* datac_p, SaImmValueTypeT theValueType)
 */
bool OamSAModifiedEntry::AddAttributeValue(const std::string& dnsubpart, OamSADataClass* datac_p, SaImmValueTypeT theValueType)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	OamSAAttributeMapIterator iter = myAttributeMap.find(dnsubpart);
	if (iter != myAttributeMap.end())
	{
		datac_p->SetSaImmValueType(theValueType);
		iter->second.Add(datac_p);
		RetVal = true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}
/**
 * OamSAModifiedEntry::AddEmptyAttributeValue(const std::string& dnsubpart,MafOamSpiMoAttributeType_3T type,SaImmValueTypeT theValueType)
 */

bool OamSAModifiedEntry::AddEmptyAttributeValue(const std::string& dnsubpart,MafOamSpiMoAttributeType_3T type,SaImmValueTypeT theValueType,bool defaultvalue)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	OamSAAttributeMapIterator iter = myAttributeMap.find(dnsubpart);
	if (iter != myAttributeMap.end())
	{
		iter->second.SetAttributeTypes(type,theValueType,defaultvalue);
		RetVal = true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

/**
 * OamSAModifiedEntry::AddEmptyAttributeValue(const std::string& dnsubpart,MafOamSpiMoAttributeType_3T type)
 */

bool OamSAModifiedEntry::AddEmptyAttributeValue(const std::string& dnsubpart,MafOamSpiMoAttributeType_3T type, bool defaultvalue)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaImmValueTypeT theValueType = 	theTranslator.ConvertComAttributeTypeToImmType(type);
	LEAVE_OAMSA_TRANSLATIONS();
	return AddEmptyAttributeValue(dnsubpart, type, theValueType,defaultvalue);

}
/**
 * OamSAModifiedEntry::GetAttributeValue(const std::string& attrname)
 */
OamSAModifiedEntry::OamSAAttributeMapRetVal	OamSAModifiedEntry::GetAttributeValue(const std::string& attrname)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSAAttributeMapRetVal RetVal;
	OamSAAttributeMapIterator iter = myAttributeMap.find(attrname);
	if (iter != myAttributeMap.end())
	{
		RetVal.first = iter;
		RetVal.second = true;
	}
	else
	{
		RetVal.second = false;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

/**
 * OamSAModifiedEntry::Find(const std::string& dnsubpart)
 */
OamSAModifiedEntry::OamSAModifiedMapRetVal OamSAModifiedEntry::Find(const std::string& dnsubpart)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSAModifiedMapRetVal	RetVal;
	RetVal.first = myNameMap.find(dnsubpart);
	RetVal.second= (RetVal.first != myNameMap.end());
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

/**
 * OamSAModifiedEntry::IsEmpty()
 */
bool OamSAModifiedEntry::IsEmpty()
{
	ENTER_OAMSA_TRANSLATIONS();
	bool rt = myNameMap.empty();
	LEAVE_OAMSA_TRANSLATIONS();
	return rt;
}

/**
 * OamSAModifiedEntry::Clear()
 */
void OamSAModifiedEntry::Clear()
{
	ENTER_OAMSA_TRANSLATIONS();
	myNameMap.clear();
	myNameMapInsertionOrdered.clear();
	myAttributeMap.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}



/**
 * OamSAModifiedObjectsList - This class contains a tree structure built from OamSAModifiedEntry leaves.
 *
 * Despite it's name it contains information both for modified and created objects. It's internal to
 * the cache structure.
 *
 * Example:
 *
 * We put in two created objects and one modified object.
 *
 * A=1,B=1,C=2 (new)
 * A=1,B=1,C=3 (new)
 * A=1,B=1,X=4 (mod)
 *
 *   |--------|    |--------|    |--------|
 *   |  A=1   |    |  B=1   |    |  C=2   |
 *   |        |----|        |----|        |
 *   |invalid |    |invalid |    | new    |
 *   |--------|    |--------|    |--------|
 *                                    |
 *                               |--------|
 *                               |  C=3   |
 *                               |        |
 *                               | new    |
 *                               |--------|
 *                                    |
 *                               |--------|
 *                               |  X=4   |  ----------  ----------  ---------
 *                               |        |--| Attr 1 |--| Attr 2 |--| Attr 3 |
 *                               |  mod   |  ----------  ----------  ---------
 *                               |--------|       |           |          |
 *                                           ----------  ----------  ----------
 *                                           |Value1.1|  |Value2.1|  |Value3.1|
 *                                           ----------  ----------  ----------
 *                                                |                      |
 *                                           ----------              ----------
 *                                           |Value1.2|              |Value3.2|
 *                                           ----------              ----------
 *                                                |
 *                                           ----------
 *                                           |Value1.3|
 *                                           ----------
 */
class OamSAModifiedObjectsList
{
public:

	// Constructors
	OamSAModifiedObjectsList(OamSACache* theOwnerCache){ ENTER_OAMSA_TRANSLATIONS(); myOwnerCache = theOwnerCache; LEAVE_OAMSA_TRANSLATIONS(); }

	// Destructor
	~OamSAModifiedObjectsList(){ ENTER_OAMSA_TRANSLATIONS(); theRootEntry.Clear(); LEAVE_OAMSA_TRANSLATIONS();}

		// Public members

	bool	Find(std::list<std::string>&	theDisassembledDN);

	bool 	Find(const std::string& ObjectDN,
					const std::string& AttributeName,
					MafMoAttributeValueContainer_3T ** attributeValue_pp);

	bool 	Find(std::list<std::string>&	theDisassembledDN,
					const std::string& AttributeName,
					MafMoAttributeValueContainer_3T ** attributeValue_pp);

	void	Insert(const std::string& ObjectDN,
					const std::string& theImmName,
					const std::string& AttributeName,
					const MafMoAttributeValueContainer_3T * attributeValue);

	void	Insert(std::list<std::string>&	theDisassembledDN,
					const std::string& theImmName,
					const std::string& AttributeName,
					const MafMoAttributeValueContainer_3T * attributeValue);

	void	InsertNewObject(const std::string& ObjectDN,
							const std::string& theImmName,
							const std::string& theClassName,
							const std::string& theParentName,
							const std::string& AttributeName,
							const MafMoAttributeValueContainer_3T * attributeValue);

	void	InsertNewObject(std::list<std::string>&	theDisassembledDN,
							const std::string& theImmName,
							const std::string& theClassName,
							const std::string& theParentName,
							const std::string& AttributeName,
							const MafMoAttributeValueContainer_3T * attributeValue);

	bool	RemoveObject(const std::string& ObjectDN);
	bool	RemoveObject(std::list<std::string>&	theDisassembledDN);
	void 	GetModifiedData(OamSACacheModifiedObjectsList& mol);
	void 	GetCreatedData(OamSACacheCreatedObjectsList& col);

	std::string GetNextChildObject(OamSACache::DNList& theObjectList,
									const std::string& theObjectClass,
									const std::string& thePreviousSibling);

	void Dump();


private:

	// Private methods
	void 	TraverseModifiedObjTree(OamSACacheModifiedObjectsList& 	mol,
											std::string&					ObjName,
										const std::string&				PartName,
										OamSAModifiedEntry&				delEntry);

	void 	TraverseCreatedObjTree(OamSACacheCreatedObjectsList& 		col,
									std::string						ObjName,
									const std::string&				PartName,
									OamSAModifiedEntry&				delEntry,
									OamSACache&						theCache);

	void	RemoveSubTree(OamSAModifiedEntry&				delEntry);

	void 	DumpItem(OamSAModifiedEntry& theEntry, int level);

	OamSAModifiedObjectsList() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); }

	// Private members
	OamSAModifiedEntry			theRootEntry;
	OamSACache*					myOwnerCache;
};

/**
 * Member functions  OamSAModifiedObjectsList
 */
/**
 * OamSAModifiedObjectsList::Dump()
 */
void OamSAModifiedObjectsList::Dump()
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Dump()");
	DumpItem(theRootEntry, 1);
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * DumpItem(OamSAModifiedEntry& theEntry, int level)
 */
void OamSAModifiedObjectsList::DumpItem(OamSAModifiedEntry& theEntry, int level)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSAModifiedEntry::OamSAModifiedMapIterator mapIter;
	OamSAModifiedEntry::OamSAAttributeMapIterator attIter;
	for (mapIter = theEntry.GetNameMap().begin(); mapIter != theEntry.GetNameMap().end(); mapIter++)
	{
		for (int i = 0; i < level; i++)
			DEBUG_OAMSA_TRANSLATIONS("  ");
		DEBUG_OAMSA_TRANSLATIONS("Level %d Name %s ", level, mapIter->first.c_str());
		for (attIter = mapIter->second.GetAttributeMap().begin(); attIter != mapIter->second.GetAttributeMap().end(); attIter++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Attribute Name %s ",  attIter->first.c_str());
		}
		DumpItem(mapIter->second, level + 1);
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * GetCreatedData(OamSACacheCreatedObjectsList& col)
 */
void OamSAModifiedObjectsList::GetCreatedData(OamSACacheCreatedObjectsList& col)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string	NameStr;
	std::string NullStr = "";
	TraverseCreatedObjTree(col, NameStr, NullStr, theRootEntry, *myOwnerCache);
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * TraverseCreatedObjTree(OamSACacheCreatedObjectsList& 		col,
 *																std::string						ObjName,
 *																const std::string&				PartName,
 *																OamSAModifiedEntry&				delEntry,
 *																OamSACache&						theCache)
 */
//	To recursively traverse the deleted objects tree and gather all the created objects together
//  in the list

void OamSAModifiedObjectsList::TraverseCreatedObjTree(OamSACacheCreatedObjectsList& 		col,
																std::string						ObjName,
																const std::string&				PartName,
																OamSAModifiedEntry&				delEntry,
																OamSACache&						theCache)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSAModifiedEntry::OamSAModifiedListIterator	ListIterator;

	DEBUG_OAMSA_TRANSLATIONS("TraverseCreatedObjTree ObjName [%s] PartName [%s] / ImmName [%s]", ObjName.c_str(), PartName.c_str(), delEntry.GetImmName().c_str());
	DEBUG_OAMSA_TRANSLATIONS("TraverseCreatedObjTree ParentName [%s] / ClassName [%s]", delEntry.GetParentName().c_str(), delEntry.GetClassName().c_str());
	if (ObjName.length() > 0)
	{
		ObjName = ObjName + ',' + PartName;
	}
	else
	{
		ObjName = PartName;
	}
	for (ListIterator = delEntry.GetOrderedNameList().begin(); ListIterator != delEntry.GetOrderedNameList().end(); ListIterator++)
	{
		TraverseCreatedObjTree(col,ObjName,  (*ListIterator), delEntry.GetModifiedEntry(*ListIterator), theCache);
	}
	if (delEntry.IsNewObject())
	{
		DEBUG_OAMSA_TRANSLATIONS("Putting in data in created list, ObjName = %s, delEntry.ImmName = %s" , ObjName.c_str(), delEntry.GetImmName().c_str());
		OamSACacheCreatedObjectData od(ObjName, delEntry, theCache);
		col.Insert(od);
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/*
 * Member functions  OamSAModifiedObjectsList
 */
void OamSAModifiedObjectsList::GetModifiedData(OamSACacheModifiedObjectsList& mol)
{
	ENTER_OAMSA_TRANSLATIONS();

	std::string	NameStr;
	std::string NullStr = "";
	DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::GetModifiedData Traversing modified objects tree----");
	TraverseModifiedObjTree(mol, NameStr, NullStr, theRootEntry);
	LEAVE_OAMSA_TRANSLATIONS();
}

//	To recursively traverse the deleted objects tree and gather all the modified objects together
//  in the list
void OamSAModifiedObjectsList::TraverseModifiedObjTree(OamSACacheModifiedObjectsList& 	mol,
																std::string&						ObjName,
																const std::string&				PartName,
																OamSAModifiedEntry&				delEntry)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSAModifiedEntry::OamSAModifiedListIterator	ListIterator;
	std::string tempObjName;
	if (ObjName.length() > 0)
	{
		tempObjName = ObjName + ',' + PartName;
	}
	else
	{
		tempObjName = PartName;
	}
	for (ListIterator = delEntry.GetOrderedNameList().begin(); ListIterator != delEntry.GetOrderedNameList().end(); ListIterator++)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::TraverseModifiedObjTree Object = \"%s\" ",tempObjName.c_str() );
		TraverseModifiedObjTree(mol,tempObjName, (*ListIterator), delEntry.GetModifiedEntry(*ListIterator));
	}
	if (delEntry.IsModifiedObject())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::TraverseModifiedObjTree INSERTING Object = \"%s\" in list",tempObjName.c_str() );
		OamSACacheModifiedObjectData od(delEntry.GetImmName(), delEntry);
		mol.Insert(od);
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * RemoveSubTree(OamSAModifiedEntry&	delEntry)
 */
void OamSAModifiedObjectsList::RemoveSubTree(OamSAModifiedEntry&	delEntry)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSAModifiedEntry::OamSAModifiedMapIterator	theIter;
	for (theIter = delEntry.GetNameMap().begin(); theIter != delEntry.GetNameMap().end(); theIter++)
	{
		RemoveSubTree(theIter->second);
	}
	delEntry.Clear();
	delEntry.ClearEntryKind();
	LEAVE_OAMSA_TRANSLATIONS();
}
/**
 * OamSAModifiedObjectsList::InsertNewObject(const std::string& ObjectDN,
												const std::string& theImmName,
												const std::string& theClassName,
												const std::string& theParentName,
												const std::string& AttributeName,
												const MafMoAttributeValueContainer_3T * attributeValue)
 *
 *
 */
void OamSAModifiedObjectsList::InsertNewObject(const std::string& ObjectDN,
												const std::string& theImmName,
												const std::string& theClassName,
												const std::string& theParentName,
												const std::string& AttributeName,
												const MafMoAttributeValueContainer_3T * attributeValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string>		theDisassembledDN;
	myOwnerCache->SplitDN(ObjectDN,theDisassembledDN);
	InsertNewObject(theDisassembledDN, 	theImmName,  theClassName,  theParentName, 	AttributeName, attributeValue);
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * OamSAModifiedObjectsList::InsertNewObject(std::list<std::string>&	theDisassembledDN,
												const std::string& theImmName,
												const std::string& theClassName,
												const std::string& theParentName,
												const std::string& AttributeName,
												const MafMoAttributeValueContainer_3T * attributeValue)
 */
void OamSAModifiedObjectsList::InsertNewObject(std::list<std::string>&	theDisassembledDN,
												const std::string& theImmName,
												const std::string& theClassName,
												const std::string& theParentName,
												const std::string& AttributeName,
												const MafMoAttributeValueContainer_3T * attributeValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string>::iterator 				theListIterator;
	OamSAModifiedEntry::OamSAModifiedMapIterator 	MapIterator;
	OamSAModifiedEntry::OamSAModifiedMapRetVal		ModifiedRetVal;
	OamSAModifiedEntry::OamSAAttributeMapRetVal		AttributeRetVal;
	theListIterator = theDisassembledDN.begin();
	ModifiedRetVal = theRootEntry.AddName((*theListIterator));

	DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::InsertNewObject");
	DEBUG_OAMSA_TRANSLATIONS("First name element %s", theListIterator->c_str());

	for (theListIterator++; theListIterator != theDisassembledDN.end(); theListIterator++)
	{
		DEBUG_OAMSA_TRANSLATIONS("Next name element %s", theListIterator->c_str());

		MapIterator		= ModifiedRetVal.first;
		ModifiedRetVal = MapIterator->second.AddName((*theListIterator));
	}
	ModifiedRetVal.first->second.SetEntryKind(OamSAModifiedEntry::eModifiedEntryNewObject);
	ModifiedRetVal.first->second.SetImmName(theImmName);
	ModifiedRetVal.first->second.SetClassName(theClassName);
	ModifiedRetVal.first->second.SetParentName(theParentName);
	AttributeRetVal = ModifiedRetVal.first->second.AddAttribute(AttributeName);

	// Get the IMM data type from IMM, don't trust the default value
	SaImmClassCategoryT		theCategory;
	SaImmValueTypeT			theValueType;
	SaImmAttrFlagsT			theFlags;

	bool ImmDataTypeFound = myOwnerCache->GetImmAttributeData((char*)theClassName.c_str(),
																(char*)AttributeName.c_str(),
																theCategory,
																theValueType,
																theFlags);

	// If add returns false the the attribute already did exist, erase the old data.
	if (!AttributeRetVal.second)
		AttributeRetVal.first->second.Clear();

	for (unsigned int i = 0; i < attributeValue->nrOfValues; i++)
	{
		// Add each and every data item. If found in imm use the IMM data type gotten from there
		// otherwise use the default value. This is especially important for the COM data type
		// string which can map to either SASTRINGT or SANEMET
		OamSADataClass* dc_p = theDataFactory.CreateOamSAData(attributeValue->type, attributeValue->values[i]);
		if (dc_p != NULL)
		{
			if (ImmDataTypeFound)
			{
				ModifiedRetVal.first->second.AddAttributeValue(AttributeName, dc_p, theValueType);
			}
			else
			{
				ModifiedRetVal.first->second.AddAttributeValue(AttributeName, dc_p);
			}
		}
	}

	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * 	OamSAModifiedObjectsList::Insert(const std::string& ObjectDN,
										const std::string& theImmName,
										const std::string& AttributeName,
										const MafMoAttributeValueContainer_3T * attributeValue )
 */
void OamSAModifiedObjectsList::Insert(const std::string& ObjectDN,
										const std::string& theImmName,
										const std::string& AttributeName,
										const MafMoAttributeValueContainer_3T * attributeValue )
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string>	theDisassembledDN;
	myOwnerCache->SplitDN(ObjectDN,theDisassembledDN);
	Insert(theDisassembledDN, theImmName, AttributeName, attributeValue);
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * OamSAModifiedObjectsList::Insert(std::list<std::string>&	theDisassembledDN,
 *										const std::string& theImmName,
 *										const std::string& AttributeName,
 *										const MafMoAttributeValueContainer_3T * attributeValue )
 */
void OamSAModifiedObjectsList::Insert(std::list<std::string>&	theDisassembledDN,
										const std::string& theImmName,
										const std::string& AttributeName,
										const MafMoAttributeValueContainer_3T * attributeValue )
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string>::iterator 				theListIterator;
	OamSAModifiedEntry::OamSAModifiedMapIterator 	MapIterator;
	OamSAModifiedEntry::OamSAModifiedMapRetVal		ModifiedRetVal;
	OamSAModifiedEntry::OamSAAttributeMapRetVal		AttributeRetVal;

	DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Insert");
	DEBUG_OAMSA_TRANSLATIONS("Imm name %s AttributeName %s ",theImmName.c_str(), AttributeName.c_str());

	theListIterator = theDisassembledDN.begin();
	ModifiedRetVal = theRootEntry.AddName((*theListIterator));


	DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Insert. first name part %s", theListIterator->c_str());


	for (theListIterator++; theListIterator != theDisassembledDN.end(); theListIterator++)
	{
		MapIterator		= ModifiedRetVal.first;

		DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Insert. next name part %s", theListIterator->c_str());
		ModifiedRetVal = MapIterator->second.AddName((*theListIterator));
	}
	ModifiedRetVal.first->second.SetEntryKind(OamSAModifiedEntry::eModifiedEntryAlteredObject);
	ModifiedRetVal.first->second.SetImmName(theImmName);

	// Now retrieve the attribute type in IMM. If the classname is missing from the cached data
	// we need to fetch it from IMM using the object name

	bool AttributeTypeFound = true;
	bool DefaultAttributevalueFound = false;
	SaImmClassCategoryT		theCategory;
	SaImmValueTypeT			theValueType;
	SaImmAttrFlagsT			theFlags;

	std::string ClassName = ModifiedRetVal.first->second.GetClassName();
	if (0 == ClassName.length())
	{
		// No class name found, get it from IMM using the object name
		AttributeTypeFound = myOwnerCache->GetClassNameFromImm(theImmName, ClassName);
	}
	if (AttributeTypeFound)
	{
		// Get the IMM data type from IMM, don't trust the default value
		AttributeTypeFound = myOwnerCache->GetImmAttributeData((char*)ClassName.c_str(),
																(char*)AttributeName.c_str(),
																theCategory,
																theValueType,
																theFlags);
	}


	AttributeRetVal = ModifiedRetVal.first->second.AddAttribute(AttributeName);

	// If add returns false the the attribute already did exist, erase the old data.
	if (!AttributeRetVal.second)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Insert. already Exists");
		OamSAModifiedEntry::OamSAAttributeMapRetVal	attret = ModifiedRetVal.first->second.GetAttributeValue(AttributeName);
		if (attret.second)
		{
			MafMoAttributeValueContainer_3T *attributeValue_pp = theDataFactory.CreateMafMoAttrValCont(attret.first->second);
			if(NULL != attributeValue_pp)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Insert. nrOfValues for already existed Attrname: %d ", (attributeValue_pp)->nrOfValues);
				if(0 != (attributeValue_pp)->nrOfValues)
				{
					//if nrovalues if zero for existed attrname , then there is no data to delete.
					//Updating the flag to check is there any data available before setting the new value.
					DefaultAttributevalueFound = true;
				}
				else
				{
					DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Insert. No value is present for already Existed AttrName");
				}
			}
		}
		AttributeRetVal.first->second.Clear();
	}

	if (0 == attributeValue->nrOfValues)
	{
		if (AttributeTypeFound)
		{
			ModifiedRetVal.first->second.AddEmptyAttributeValue(AttributeName, attributeValue->type, theValueType, DefaultAttributevalueFound);
		}
		else
		{
			ModifiedRetVal.first->second.AddEmptyAttributeValue(AttributeName, attributeValue->type, DefaultAttributevalueFound);
		}
	}
	else
	{
		for (unsigned int i = 0; i < attributeValue->nrOfValues; i++)
		{

			if (AttributeTypeFound)
			{

				DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Insert attribute name %s com type %d imm value type %d ", AttributeName.c_str(),
																											attributeValue->type,
																											theValueType);
				ModifiedRetVal.first->second.AddAttributeValue(AttributeName,
																theDataFactory.CreateOamSAData(attributeValue->type,
																attributeValue->values[i]),
																theValueType);
			}
			else
			{
				ModifiedRetVal.first->second.AddAttributeValue(AttributeName,
																	theDataFactory.CreateOamSAData(attributeValue->type,
																	attributeValue->values[i]));
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}
/**
 * OamSAModifiedObjectsList::Find(std::list<std::string>&		theDisassembledDN,
 *										const std::string& AttributeName,
 *										MafMoAttributeValueContainer_3T ** attributeValue_pp)
 */
bool OamSAModifiedObjectsList::Find(std::list<std::string>&		theDisassembledDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	std::list<std::string>::iterator 				theListIterator;
	OamSAModifiedEntry::OamSAModifiedMapIterator 	MapIterator;
	OamSAModifiedEntry::OamSAModifiedMapRetVal		ModifiedRetVal;
	theListIterator = theDisassembledDN.begin();
	ModifiedRetVal  = theRootEntry.Find((*theListIterator));
	DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Find Looking for object in cache");
	DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::Find looking for first part %s", (*theListIterator).c_str());
	if (ModifiedRetVal.second)
	{
		for (theListIterator++ ; theListIterator !=  theDisassembledDN.end(); theListIterator++)
		{
			DEBUG_OAMSA_TRANSLATIONS("Looking for name part \"%s\" ",(*theListIterator).c_str());
			ModifiedRetVal = ModifiedRetVal.first->second.Find((*theListIterator));
			// If not found, the value is not in here. But it mighth be in IMM
			if (!ModifiedRetVal.second)
			{
				goto end_exit;
			}
		}
		RetVal = true;
	}
end_exit:
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}
/**
 * 	OamSAModifiedObjectsList::Find(const std::string& ObjectDN,
 *										const std::string& AttributeName,
 *										MafMoAttributeValueContainer_3T ** attributeValue_pp)
 */
bool OamSAModifiedObjectsList::Find(const std::string& ObjectDN,
										const std::string& AttributeName,
										MafMoAttributeValueContainer_3T ** attributeValue_pp)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	std::list<std::string>		theDisassembledDN;
	myOwnerCache->SplitDN(ObjectDN,theDisassembledDN);
	RetVal = Find(theDisassembledDN, AttributeName, attributeValue_pp);
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

/**
 * OamSAModifiedObjectsList::Find(std::list<std::string>&		theDisassembledDN,
 *										const std::string& AttributeName,
 *										MafMoAttributeValueContainer_3T ** attributeValue_pp)
 */
bool OamSAModifiedObjectsList::Find(std::list<std::string>&		theDisassembledDN,
										const std::string& AttributeName,
										MafMoAttributeValueContainer_3T ** attributeValue_pp)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	std::list<std::string>::iterator 				theListIterator;
	OamSAModifiedEntry::OamSAModifiedMapIterator 	MapIterator;
	OamSAModifiedEntry::OamSAModifiedMapRetVal		ModifiedRetVal;
	theListIterator = theDisassembledDN.begin();
	ModifiedRetVal  = theRootEntry.Find((*theListIterator));
	if (ModifiedRetVal.second)
	{
		for (theListIterator++ ; theListIterator !=  theDisassembledDN.end(); theListIterator++)
		{
			ModifiedRetVal = ModifiedRetVal.first->second.Find((*theListIterator));
			// If not found, the value is not in here. But it mighth be in IMM
			if (!ModifiedRetVal.second)
				goto end_exit;
		}
		// If we end up here, the object name has been found. Now look for the attribute and the value.
		OamSAModifiedEntry::OamSAAttributeMapRetVal	attret = ModifiedRetVal.first->second.GetAttributeValue(AttributeName);
		if (attret.second)
		{
			*attributeValue_pp = theDataFactory.CreateMafMoAttrValCont(attret.first->second);
			RetVal = true;
		}
	}
end_exit:
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

/**
 * OamSAModifiedObjectsList::RemoveObject(const std::string& ObjectDN)
 */
bool OamSAModifiedObjectsList::RemoveObject(const std::string& ObjectDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	std::list<std::string>	theDisassembledDN;
	myOwnerCache->SplitDN(ObjectDN,theDisassembledDN);
	RetVal =  RemoveObject(theDisassembledDN);
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

/**
 * OamSAModifiedObjectsList::RemoveObject(std::list<std::string>&	theDisassembledDN)
 */
bool	OamSAModifiedObjectsList::RemoveObject(std::list<std::string>&	theDisassembledDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Removes the object and all below it, the cannot continue existing
	// without the parent.
	bool RetVal = false;
	std::list<std::string>::iterator 				theListIterator;
	OamSAModifiedEntry::OamSAModifiedMapIterator 	MapIterator;
	OamSAModifiedEntry::OamSAModifiedMapRetVal		ModifiedRetVal;
	OamSAModifiedEntry::OamSAModifiedMapRetVal		ModifiedRetVal_pre;
	theListIterator = theDisassembledDN.begin();
	ModifiedRetVal  = theRootEntry.Find((*theListIterator));
	DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::RemoveObject looking for %s", (*theListIterator).c_str());
	if (ModifiedRetVal.second)
	{
		for (theListIterator++ ; theListIterator !=  theDisassembledDN.end(); theListIterator++)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::RemoveObject looking for \"%s\"", (*theListIterator).c_str());
			ModifiedRetVal_pre = ModifiedRetVal;
			ModifiedRetVal = ModifiedRetVal.first->second.Find((*theListIterator));
			// If not found, that's OK , just get out with nothing to do
			if (!ModifiedRetVal.second)
				goto end_exit;
		}
		// If we end up here, the object name has been found. Remove it and all below it
		// First the thingies below
		DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::RemoveObject removing ");
		//fix TR HQ83048, TR HR29426
		if (ModifiedRetVal.first->second.IsNewObject())
			RetVal = true;
		//fix TR HQ68880
		RemoveSubTree(ModifiedRetVal.first->second);
		// Fix invalid access to element that already deleted after erasing from map (myNameMap): ModifiedRetVal
		ModifiedRetVal_pre.first->second.GetOrderedNameList().remove(ModifiedRetVal.first->first);
		ModifiedRetVal_pre.first->second.GetNameMap().erase(ModifiedRetVal.first);

	}
end_exit:;
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

/**
 * OamSAModifiedObjectsList::GetNextChildObject(OamSACache::DNList& theObjectList, const std::string& theObjectClass, const std::string& thePreviousSibling)
 */
std::string OamSAModifiedObjectsList::GetNextChildObject(OamSACache::DNList& theObjectList, const std::string& theObjectClass, const std::string& thePreviousSibling)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string>::iterator 				theListIterator;
	OamSAModifiedEntry::OamSAModifiedMapIterator 	MapIterator;
	OamSAModifiedEntry::OamSAModifiedMapRetVal		ModifiedRetVal;
	std::string	theRetString;
	theRetString = "";

	theListIterator = theObjectList.begin();
	ModifiedRetVal  = theRootEntry.Find((*theListIterator));
	if (ModifiedRetVal.second)
	{
		for (theListIterator++ ; theListIterator !=  theObjectList.end(); theListIterator++)
		{
			ModifiedRetVal = ModifiedRetVal.first->second.Find((*theListIterator));
			// If not found, the value is not in here. But it mighth be in IMM
			if (!ModifiedRetVal.second)
				goto end_exit;
		}
		// If we end up here, the object name has been found. Now look for any children of the specifed class

		// Well if there are no children......... goodbye
		if (!ModifiedRetVal.first->second.IsEmpty())
		{
			OamSAModifiedEntry::OamSAModifiedMapRetVal		ModifiedMapIter;
			OamSAModifiedEntry::OamSAModifiedMapIterator 	mapiter;
			// Look if there has been a previous sibling found, then we need to find that and look for the next
			if (!thePreviousSibling.empty())
			{
				ModifiedMapIter = ModifiedRetVal.first->second.Find(thePreviousSibling);
				// Should be found, otherwise there has been some fishy deletions going on. But check anyway
				if (ModifiedMapIter.second)
				{
					mapiter = ModifiedMapIter.first;
					mapiter++;
				}
				else
				{
					// if not found, just exit
					goto end_exit;
				}
			}
			else
			{
				// Just grab the first child and see if it matches
				mapiter = ModifiedRetVal.first->second.GetNameMap().begin();
			}
			for (; mapiter !=  ModifiedRetVal.first->second.GetNameMap().end(); mapiter++)
			{
				// COM will always iterate using classname filter!
				if (mapiter->second.GetClassName() == theObjectClass && (!mapiter->second.IsInvalidObject()))
				{
					// Now check so that the object isn't in the deleted list
					OamSACache::DNList	theTempObjList = theObjectList;
					theTempObjList.push_back(mapiter->first);
					if (!myOwnerCache->IsDeleted(theTempObjList))
					{
						theRetString = mapiter->first;
						break;
					}
					else
					{
						DEBUG_OAMSA_TRANSLATIONS("OamSAModifiedObjectsList::GetNextChildObject found \"%s\" in the deleted list continuing", mapiter->first.c_str());
					}
				}
			}
		}
	}
end_exit:
	LEAVE_OAMSA_TRANSLATIONS();
	return theRetString;
}


 /**
  * OamSADeletedEntry - class used to describe a node in the deleted objects tree. The trees structure is similar to
  * the modified objects tree, although it lacks the attribute list.
  *
  * There is also an interesting trick regarding the removal of deleted objects due to the fact that the same object
  * is created again by a call to create MO. Since a delete of an object also means that objects below should be removed
  * (makes sense since they cannot be orphans)
  */
class OamSADeletedEntry
{
public:
	typedef std::map <std::string, OamSADeletedEntry> OamSADeletedMap;
	typedef std::map <std::string, OamSADeletedEntry>::iterator OamSADeletedMapIterator;
	typedef std::pair < OamSADeletedMapIterator, bool> OamSADeletedMapRetVal;

	OamSADeletedEntry(){ ENTER_OAMSA_TRANSLATIONS(); myDeletedKind = eDeletedEntryNoDeletePoint; LEAVE_OAMSA_TRANSLATIONS(); }
	~OamSADeletedEntry() { ENTER_OAMSA_TRANSLATIONS(); myMap.clear(); myImmName.clear(); LEAVE_OAMSA_TRANSLATIONS(); }

	OamSADeletedMapRetVal Add(const std::string& dnsubpart);
	OamSADeletedMapRetVal Find(const std::string& dnsubpart);
	bool					IsEmpty();
	void					Clear();
	OamSADeletedMap&		GetMap() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myMap; }
	bool 					IsActive() 	{ ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return (myDeletedKind == eDeletedEntryActiveDeletePoint); }
	bool					IsDormant()	{ ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return (myDeletedKind == eDeletedEntryDormantDeletePoint); }
	void					SetActive() 	{ ENTER_OAMSA_TRANSLATIONS(); myDeletedKind = eDeletedEntryActiveDeletePoint; LEAVE_OAMSA_TRANSLATIONS(); }
	void					SetDormant() 	{ ENTER_OAMSA_TRANSLATIONS(); myDeletedKind = eDeletedEntryDormantDeletePoint; LEAVE_OAMSA_TRANSLATIONS(); }
	void					SetImmName(const std::string& theImmName)  {ENTER_OAMSA_TRANSLATIONS(); myImmName = theImmName; LEAVE_OAMSA_TRANSLATIONS(); }
	std::string&			GetImmName() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return myImmName; }

private:
	typedef enum {eDeletedEntryNoDeletePoint		= 0,
					eDeletedEntryDormantDeletePoint	= 1,
					eDeletedEntryActiveDeletePoint	= 2
	} eDeletedEntryKind;

	eDeletedEntryKind	myDeletedKind;
	OamSADeletedMap 	myMap;
	std::string		myImmName;
};

OamSADeletedEntry::OamSADeletedMapRetVal OamSADeletedEntry::Add(const std::string& dnsubpart)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSADeletedEntry	theEntry;
	LEAVE_OAMSA_TRANSLATIONS();
	return myMap.insert(std::pair<std::string, OamSADeletedEntry>(dnsubpart,theEntry));
}

OamSADeletedEntry::OamSADeletedMapRetVal OamSADeletedEntry::Find(const std::string& dnsubpart)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSADeletedMapRetVal	RetVal;

	RetVal.first = myMap.find(dnsubpart);
	RetVal.second= (RetVal.first != myMap.end());
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

bool OamSADeletedEntry::IsEmpty()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myMap.empty();
}

void OamSADeletedEntry::Clear()
{
	ENTER_OAMSA_TRANSLATIONS();
	myMap.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  OamSADeletedObjectsList
 *
 *  This contains all the deleted objects and implicitly all their sub-trees too. It's built up from a
 *  multilevel map. The top level map contains all the toplevel dn parts and below each maps containing
 *  the following dn parts etc in a tree structure. As an exampel let's say we have the following
 *  three deleted objects
 *
 *  A=1,B=2,C=3
 *  A=1,B=2,E=4
 *  L=7,B=2,X=19
 *
 *  This is then represented as
 *
 *  Toplevel map			A=1             L=7
 * 							 |				 |
 *  2nd level map		    B=2				B=2
 *  					|----------|		 |
 *  3rd level map	  C=3         E=4       X=19
 *
 *
 * NB! The two B=2 are different disjunct entities
 *
 */

class OamSADeletedObjectsList
{
public:
	OamSADeletedObjectsList(OamSACache* theOwnerCache){ ENTER_OAMSA_TRANSLATIONS(); myOwnerCache = theOwnerCache;  myNumberOfObjects = 0; LEAVE_OAMSA_TRANSLATIONS(); }
	~OamSADeletedObjectsList(){ ENTER_OAMSA_TRANSLATIONS(); myMap.clear(); LEAVE_OAMSA_TRANSLATIONS(); }

	bool	IsDeleted(std::list<std::string>&	theDisassembledDN);
	bool	IsDeleted(const std::string& ObjectDN);
	void	Insert(std::list<std::string>&	theDisassembledDN, const std::string& theImmName);
	void	Insert(const std::string& ObjectDN, const std::string& theImmName);
	void	Remove(std::list<std::string>&	theDisassembledDN);
	void	Remove(const std::string& ObjectDN);
	void 	GetDeletedData(OamSACacheDeletedObjectsList& dol);
private:
	OamSADeletedObjectsList() {ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS();}
	void TraverseObjTree(OamSACacheDeletedObjectsList& 	dol,
							std::string					ObjName,
							const std::string&				PartName,
							OamSADeletedEntry&				delEntry);

	void SetActiveDPsDormant(OamSADeletedEntry& del);
	void SetDormantDPsActive(OamSADeletedEntry& del);
	OamSADeletedEntry::OamSADeletedMap  myMap;
	OamSACache*							myOwnerCache;
	unsigned int						myNumberOfObjects;
};

void OamSADeletedObjectsList::GetDeletedData(OamSACacheDeletedObjectsList& dol)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSADeletedEntry::OamSADeletedMapIterator	MapIterator;

	for (MapIterator = myMap.begin(); MapIterator != myMap.end(); MapIterator++)
	{
		std::string	NameStr;
		TraverseObjTree(dol, NameStr, MapIterator->first, MapIterator->second);
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

//	To recursively traverse the deleted objects tree and gather all the deleted objects
//  together in the list
void OamSADeletedObjectsList::TraverseObjTree(OamSACacheDeletedObjectsList& 	dol,
													std::string						ObjName,
													const std::string&				PartName,
													OamSADeletedEntry&				delEntry)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSADeletedEntry::OamSADeletedMapIterator	MapIterator;

	DEBUG_OAMSA_TRANSLATIONS("TraverseObjTree (deleted) ObjName %s PartName %s", ObjName.c_str(), PartName.c_str());
	ObjName = ObjName + PartName;
	for (MapIterator = delEntry.GetMap().begin(); MapIterator != delEntry.GetMap().end(); MapIterator++)
	{
		TraverseObjTree(dol,ObjName, MapIterator->first, MapIterator->second);
	}
	if (delEntry.IsActive())
	{
		DEBUG_OAMSA_TRANSLATIONS("TraverseObjTree found deleted object ObjName %s PartName %s", ObjName.c_str(), delEntry.GetImmName().c_str());
		OamSACacheDeletedObjectData od(delEntry.GetImmName());
		dol.Insert(od);
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

// Set all active Deletion points dormant
void OamSADeletedObjectsList::SetActiveDPsDormant(OamSADeletedEntry& del)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (del.IsActive())
		del.SetDormant();
	if (!del.GetMap().empty())
	{
		OamSADeletedEntry::OamSADeletedMapIterator theIter;
		for (theIter = del.GetMap().begin(); theIter != del.GetMap().end(); theIter++)
		{
			SetActiveDPsDormant(theIter->second);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return;
}

// Recursevely traverses a tree and sets the topmost dormant deletion point active in each branch
// of the sub tree, leaving those below still dormant
void OamSADeletedObjectsList::SetDormantDPsActive(OamSADeletedEntry& del)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (del.IsDormant())
	{
		del.SetActive();
	}
	else if (!del.GetMap().empty())
	{
		OamSADeletedEntry::OamSADeletedMapIterator theIter;
		for (theIter = del.GetMap().begin(); theIter != del.GetMap().end(); theIter++)
		{
			SetDormantDPsActive(theIter->second);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return;
}

// Member function IsDeleted
bool OamSADeletedObjectsList::IsDeleted(const std::string& ObjectDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	std::list<std::string>						theDisassembledDN;
	myOwnerCache->SplitDN(ObjectDN,theDisassembledDN);
	RetVal = IsDeleted(theDisassembledDN);
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

bool OamSADeletedObjectsList::IsDeleted(std::list<std::string>&	theDisassembledDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool 										RetVal = false;
	std::list<std::string>::iterator 			theListIterator;
	OamSADeletedEntry::OamSADeletedMapIterator	MapIterator;

	// Now check to see if this object is deleted. It may explicitly be specified in the map, or
	// it might be part of a sub-tree in which case it must be matched. This needs a tounge in cheek
	// aproach to get it right. For example, let's say that the DN of the object to check for deletion
	// is "A=1,B=2,C=3"
	// "A=1,B=2,C=3" 		will of course match because that is exactly the object name
	// "A=1,B=2" 			will match because that is a branch above the object
	// "A=1,B=3,D=4" 		will not match because that is a different branch
	// "A=1,B=2,C=3,E=5"  	will not match because the deletion point is below the object
	//
	// A deletion point also has to be active to get a hit. Why? Because another deletion point may have
	// been inserted above this one in the tree, in case those below goes dormant. The cannot just be removed
	// (which actually was the first aproach) because the higher ranking deletion point may be removed
	// by a create object operation on the same object, in which case the dormant deletion points need to be
	// reactivated
	theListIterator = theDisassembledDN.begin();
	DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::IsDeleted looking for first part");
	if ((MapIterator = myMap.find((*theListIterator))) != myMap.end())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::IsDeleted looking for part \"%s\" ", (*theListIterator).c_str());

		// Check if the first element is the only one, then we have a hit
		if (MapIterator->second.IsActive())
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::IsDeleted hit active delete point for  \"%s\" ", (*theListIterator).c_str());
			RetVal = true;
			goto end_exit;
		}

		// Assume that it will match. If we find out it won't, we'll terminate the loop
		for (unsigned int i = 1; i < theDisassembledDN.size(); i++)
		{
			RetVal = true;
			theListIterator++;
			// If we have come to the end of the object name, then it is not in the list, ergo it is not deleted
			// exit and return false
			if (theListIterator == theDisassembledDN.end())
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::IsDeleted failed to find object in deleted objects list , list at end, exiting.....");
				RetVal = false;
				goto end_exit;
			}
			DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::IsDeleted checking if \"%s\" is in the deleted list",(*theListIterator).c_str());
			// If it isn't in the list, it isn't deleted, exit and return false
			OamSADeletedEntry::OamSADeletedMapRetVal	MapRetVal;
			MapRetVal = MapIterator->second.Find((*theListIterator));
			if (!MapRetVal.second) // Not found if false
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::IsDeleted failed to find object in deleted objects list ,\"%s\", exiting.....",(*theListIterator).c_str() );
				RetVal = false;
				goto end_exit;
			}
			MapIterator = MapRetVal.first;
			if (MapIterator->second.IsActive())
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::IsDeleted found object in deleted objects list ,\"%s\", exiting.....",(*theListIterator).c_str() );
				DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::IsDeleted map name found \"%s\" ", MapIterator->first.c_str());
				// The stored list ended before or with the object name, so we have a hit. The object is deleted
				goto end_exit;
			}
			else
			{
				RetVal=false;
			}
		}
	}
end_exit:
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

void OamSADeletedObjectsList::Insert(const std::string& ObjectDN, const std::string& theImmName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string> theDisassembledDN;
	myOwnerCache->SplitDN(ObjectDN,theDisassembledDN);
	Insert(theDisassembledDN, theImmName);
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSADeletedObjectsList::Insert(std::list<std::string>& theDisassembledDN, const std::string& theImmName)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Now let's check if this is an object that
	// 1. Is a completely new object with no decendants in there already.
	//		Then it should be inserted in the tree
	// 2. This object is already there or it's a descendant of an object there.
	//		In that case the object doesn't need to be inserted
	// 3. It's an ancestor of an object or subtree of objects already there.
	//		Then it should be the new active deletion point and all other below
	//		should be set dormant.

	std::list<std::string>::iterator 			theListIterator;

	OamSADeletedEntry::OamSADeletedMapIterator	MapIterator;

	theListIterator = theDisassembledDN.begin();
	MapIterator = myMap.find((*theListIterator));

	DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert (deleted object) first part \"%s\" ", (*theListIterator).c_str());

	if (MapIterator == myMap.end())
	{
		// The list is totaly empty, or this is branch that isn't there before
		// so just put it all in
		OamSADeletedEntry	theEntry;
		OamSADeletedEntry::OamSADeletedMapRetVal	EntryRetVal;
		EntryRetVal = myMap.insert(std::pair<std::string, OamSADeletedEntry>((*theListIterator),theEntry));
		for (theListIterator++; theListIterator != theDisassembledDN.end(); theListIterator++)
		{
			EntryRetVal = EntryRetVal.first->second.Add((*theListIterator));
			DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert (deleted object) next part \"%s\" ", (*theListIterator).c_str());
		}
		EntryRetVal.first->second.SetActive();
 DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert: Setting ImmName [%s]", theImmName.c_str());
		EntryRetVal.first->second.SetImmName(theImmName);
	}
	else
	{
		// OK, the first part is there so look further to see where it goes
		// if an active deletion point is hit, the rest of it should be put in and the last
		// should be set as a dormant deletion point. Otherwise the last node goes active.

		bool 	ActiveDPFound = false;

		if (!MapIterator->second.IsActive())
		{
			for (theListIterator++; theListIterator != theDisassembledDN.end(); theListIterator++)
			{
				OamSADeletedEntry::OamSADeletedMapRetVal	MapRetVal;
				DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert (deleted object) searching for \"%s\" ", (*theListIterator).c_str());
				MapRetVal = MapIterator->second.Find((*theListIterator));
				if (!MapRetVal.second)
				{
					// Not found, then insert the rest of the list because it's new
					for (;;)
					{
						DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert (deleted object) adding part \"%s\" ", (*theListIterator).c_str());
						MapRetVal = MapIterator->second.Add((*theListIterator++));
						MapIterator = MapRetVal.first;
						if (theListIterator == theDisassembledDN.end())
						{
							DEBUG_OAMSA_TRANSLATIONS("amSADeletedObjectsList::Insert (deleted object) setting active....");
							MapIterator->second.SetActive();
							MapIterator->second.SetImmName(theImmName);
							DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert(): Setting ImmName [%s]", theImmName.c_str());
							break;
						}

					}
					goto end_exit;
				}
				else
				{
					// Found. Then let's see if there is more
					DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert (deleted object) found");
					MapIterator = MapRetVal.first;
					if (MapIterator->second.IsActive())
					{
						DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert (deleted object) active delete point found");
						// There is already a branch so exit this
						ActiveDPFound = true;
					}
				}
			} // end for
			// Now check if we found an active deletion point above, in case this is a new dormant one.
			// If not, this one goes active and we need to go below to inactivate any active DP:s
			if (ActiveDPFound)
			{
				MapIterator->second.SetDormant();
			}
			else
			{
				SetActiveDPsDormant(MapIterator->second);
				MapIterator->second.SetActive();
				MapIterator->second.SetImmName(theImmName);
				DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Insert(): setting ImmName [%s]", theImmName.c_str());
			}
		} // end if (!MapIterator->second.IsActive())
	}
end_exit:;
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  OamSADeletedObjectsList::Remove(const std::string& ObjectDN)
 */
void OamSADeletedObjectsList::Remove(const std::string& ObjectDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::list<std::string>		theDisassembledDN;
	myOwnerCache->SplitDN(ObjectDN,theDisassembledDN);
	Remove(theDisassembledDN);
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSADeletedObjectsList::Remove(std::list<std::string>&		theDisassembledDN)
{
	ENTER_OAMSA_TRANSLATIONS();

//	Find the object, i.e active deletion point and remove it by
//  setting the corresponding deletion points below active.
//  If it's not in here, just ignore it, it's OK
	std::list<std::string>::iterator 			theListIterator;
	OamSADeletedEntry::OamSADeletedMapIterator	MapIterator;

	theListIterator = theDisassembledDN.begin();
	MapIterator = myMap.find((*theListIterator));
	DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Remove entered, first part %s",(*theListIterator).c_str());

	if (MapIterator != myMap.end())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Remove, first part is FOUND %s",(*theListIterator).c_str());
		// OK, the first part is there so look further to see where it goes
		// if an active deletion point is hit, look for the first dormant
		// one below and set it active.
		for (theListIterator++; theListIterator != theDisassembledDN.end(); theListIterator++)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Remove, next part is %s",(*theListIterator).c_str());
			OamSADeletedEntry::OamSADeletedMapRetVal	MapRetVal;
			MapRetVal = MapIterator->second.Find((*theListIterator));
			if (!MapRetVal.second)
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Remove part %s is NOT FOUND exiting NOT REMOVED",(*theListIterator).c_str());
				// Not in here so just exit the loop
				goto end_exit;
			}
			else
				MapIterator = MapRetVal.first;
		}
		// Now we have found the node. If it's active disable and det
		//  and we need to go below to activate any inactive DP:s
		theListIterator--; // to avoid segmentation fault in the debug print
		DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::last part %s ",(*theListIterator).c_str());
		if (MapIterator->second.IsActive())
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSADeletedObjectsList::Remove last part %s is FOUND, SET DORMANT",(*theListIterator).c_str());
			SetDormantDPsActive(MapIterator->second);
			MapIterator->second.SetDormant();
		}
	}
end_exit:;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACache::OamSACache()
{
	SaVersionT mVersion = { immReleaseCode, immMajorVersion, immMinorVersion };
	ENTER_OAMSA_TRANSLATIONS();
	SaAisErrorT err = saImmOmInitialize(&(myImmHandle)/*out*/,
										NULL,
										&mVersion);
	if(mVersion.minorVersion != immMinorVersion)
	{
		mVersion = (SaVersionT) {immReleaseCode, immMajorVersion, immMinorVersion};
	}
#if 0
	if (err == SA_AIS_ERR_LIBRARY)
	{
		abort();
	}
#endif
	if (err != SA_AIS_OK)
	{
		myImmHandle 		= 0;
		myAccessorHandle 	= 0;
	}
	else
	{
		err = saImmOmAccessorInitialize(myImmHandle, &myAccessorHandle);
		if (err != SA_AIS_OK)
		{
			myAccessorHandle = 0;
		}
	}
	myDeletedObjectsList_p = NULL;
	myModifiedObjectsList_p= NULL;
	myDeletedObjectsList_p = new OamSADeletedObjectsList(this);
	myModifiedObjectsList_p= new OamSAModifiedObjectsList(this);
	assert(myDeletedObjectsList_p  != NULL);
	assert(myModifiedObjectsList_p != NULL);
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACache::~OamSACache()
{
	ENTER_OAMSA_TRANSLATIONS();

	std::list<MafMoAttributeValueContainer_3T*>::iterator ValCListIter;
//	std::list<MafMoAttributeValueContainer_3T*>::iterator maf_ValCListIter;

	DEBUG_OAMSA_TRANSLATIONS("OamSACache::~OamSACache before deleting  ObjectsLists");
	delete myDeletedObjectsList_p;
	delete myModifiedObjectsList_p;

	for (ValCListIter = myAttValCList.begin() ;ValCListIter != myAttValCList.end();ValCListIter++)
	{
		MafMoAttributeValueContainer_3T* vcp = *ValCListIter;
		MafOamSpiMoAttributeType_3 vct = vcp->type;
		if (vct == MafOamSpiMoAttributeType_3_STRUCT)
		{
			for (unsigned int i = 0; i < vcp->nrOfValues; i++){
				MafMoAttributeValueStructMember_3T *member = vcp->values[i].value.structMember;
				while(member != NULL){
					delete[] (char*)member->memberName;
					MafMoAttributeValueStructMember_3T* tmp = member;
					member = member->next;
					delete tmp;
				}
			}
		}
		else if (vct == MafOamSpiMoAttributeType_3_STRING)
		{
			for (unsigned int i = 0; i < vcp->nrOfValues; i++){
				delete [] vcp->values[i].value.theString;
			}
		}
		else if (vct == MafOamSpiMoAttributeType_3_REFERENCE)
		{
			for (unsigned int i = 0; i < vcp->nrOfValues; i++){
				delete[] vcp->values[i].value.moRef;
			}
		}
		delete [] vcp->values;
		delete *ValCListIter; // delete vcp
	}
	if(myImmHandle != 0)
	{
		// OK, return memory to the IMM database that has been initialized in the constructor
		saImmOmAccessorFinalize(myAccessorHandle);
		DEBUG_OAMSA_TRANSLATIONS("OamSACache::~OamSACache before Finalizing myImmHandle");
		saImmOmFinalize(myImmHandle);
	}

	myAttValCList.clear();

	std::list<BridgeImmIterator*>::iterator myBridgeImmIterListIter = myBridgeImmIterList.begin();
	for (; myBridgeImmIterListIter != myBridgeImmIterList.end(); myBridgeImmIterListIter++)
	{
		delete *myBridgeImmIterListIter;
	}
	myBridgeImmIterList.clear();

	DEBUG_OAMSA_TRANSLATIONS("OamSACache::~OamSACache before Finalizing the MAP");
	std::map<BridgeImmIterator*, std::list<char*> >::iterator mit;
	for (mit = myBridgeImmIterMap.begin(); mit != myBridgeImmIterMap.end(); mit++)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSACache::~OamSACache finalizing iter %lu", (unsigned long) (*mit).first);
		std::list<char*>::iterator cit = (*mit).second.begin();
		for (; cit != (*mit).second.end(); cit++)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSACache::~OamSACache before deleting *cit");
			delete [] *cit;
		}
		(*mit).second.clear();
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACache::ObjectState OamSACache::InsertCreatedObject(const std::string& theObjectName,
															const std::string& theObjectClass,
															const std::string& theParentName,
															const std::string& theImmName,
															const char*		theAttributeName,
															const char*		theAttributeValue )
{
	ENTER_OAMSA_TRANSLATIONS();
	DNList	theDNList;
	SplitDN(theObjectName, theDNList);
	LEAVE_OAMSA_TRANSLATIONS();
	return InsertCreatedObject(theDNList, theObjectClass, theParentName, theImmName, theAttributeName, theAttributeValue);
}

OamSACache::ObjectState OamSACache::InsertCreatedObject(DNList& theDNList,
															const std::string& theObjectClass,
															const std::string& theParentName,
															const std::string& theImmName,
															const char*		theAttributeName,
															const char*		theAttributeValue )
{
	ENTER_OAMSA_TRANSLATIONS();

	MafMoAttributeValueContainer_3T*	theValueContainer_p = new MafMoAttributeValueContainer_3T;

	if (theValueContainer_p != NULL)
	{
		theValueContainer_p->values 				= new MafMoAttributeValue_3;
		if (theValueContainer_p->values != NULL)
		{
			//theValueContainer_p->values[0].value.theString 	= new char[strlen(theAttributeValue)+1];
			theValueContainer_p->values[0].value.theString = theTranslator.BuildAttributeValue(theAttributeName, theAttributeValue);
			theValueContainer_p->nrOfValues = 1;
			theValueContainer_p->type 		= MafOamSpiMoAttributeType_3_STRING;

			if (NULL != myModifiedObjectsList_p)
			{
				myModifiedObjectsList_p->InsertNewObject(theDNList,
														theImmName,
														theObjectClass,
														theParentName,
														theAttributeName,
														theValueContainer_p);
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("OamSACache::InsertCreatedObject myModifiedObjectsList_p is NULL");
			}

			delete [] theValueContainer_p->values[0].value.theString;
		}
		delete theValueContainer_p->values;
	}
	delete theValueContainer_p;
	LEAVE_OAMSA_TRANSLATIONS();
	return eObjectSuccess;
}

void OamSACache::DumpModifiedList()
{
	if (NULL != myModifiedObjectsList_p)
	{
		myModifiedObjectsList_p->Dump();
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::DumpModifiedList myModifiedObjectsList_p is NULL");
	}
}

bool OamSACache::RemoveCreatedObject(DNList& theDNList)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool ret = false;
	if (NULL != myModifiedObjectsList_p)
	{
		ret = myModifiedObjectsList_p->RemoveObject(theDNList);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::RemoveCreatedObject myModifiedObjectsList_p is NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return ret;
}

bool OamSACache::RemoveCreatedObject(const std::string& ObjectDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	DNList	theDNList;

	SplitDN(ObjectDN, theDNList);
	RetVal = RemoveCreatedObject(theDNList);
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

OamSACache::ObjectState OamSACache::InsertDeletedObject(const std::string& theObjectName, const std::string& theImmName)
{
	ENTER_OAMSA_TRANSLATIONS();
	DNList	theDNList;
	SplitDN(theObjectName, theDNList);
	LEAVE_OAMSA_TRANSLATIONS();
	return InsertDeletedObject(theDNList, theImmName);
}

OamSACache::ObjectState OamSACache::InsertDeletedObject(DNList& theList, const std::string& theImmName)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (NULL != myDeletedObjectsList_p)
	{
		myDeletedObjectsList_p->Insert(theList, theImmName);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::InsertDeletedObject myDeletedObjectsList_p is NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return eObjectSuccess;
}

void OamSACache::RemoveDeletedObject(std::list<std::string>&	theDisassembledDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (NULL != myDeletedObjectsList_p)
	{
		myDeletedObjectsList_p->Remove(theDisassembledDN);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::RemoveDeletedObject myDeletedObjectsList_p is NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSACache::RemoveDeletedObject(const std::string& ObjectDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	DNList	theDNList;
	SplitDN(ObjectDN, theDNList);
	RemoveDeletedObject(theDNList);
	LEAVE_OAMSA_TRANSLATIONS();
}

bool	OamSACache::IsDeleted(DNList& theDisassembledDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	if (NULL != myDeletedObjectsList_p)
	{
		RetVal = myDeletedObjectsList_p->IsDeleted(theDisassembledDN);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::IsDeleted myDeletedObjectsList_p is NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

bool	OamSACache::IsDeleted(const std::string& ObjectDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal =  false;
	DNList	theDNList;
	SplitDN(ObjectDN, theDNList);
	RetVal = IsDeleted(theDNList);
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

bool OamSACache::IsInCache(DNList& theDisassembledDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	if (NULL != myModifiedObjectsList_p)
	{
		RetVal = myModifiedObjectsList_p->Find(theDisassembledDN);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::IsInCache myModifiedObjectsList_p is NULL");
	}
	return RetVal;
}

OamSACache::ObjectState OamSACache::UpdateObjectAttribute(const std::string& theObjectName,
															const std::string& theImmName,
															const std::string& theAttributeName,
															const MafMoAttributeValueContainer_3T * attributeValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	DNList theList;
	SplitDN(theObjectName, theList);
	LEAVE_OAMSA_TRANSLATIONS();
	return UpdateObjectAttribute(theList, theImmName, theAttributeName, attributeValue);
}

OamSACache::ObjectState OamSACache::UpdateObjectAttribute(DNList& theObjectNameList,
															const std::string& theImmName,
															const std::string& theAttributeName,
															const MafMoAttributeValueContainer_3T * attributeValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	ObjectState myState = eObjectSuccess;
	if((NULL != myDeletedObjectsList_p) && (myDeletedObjectsList_p->IsDeleted(theObjectNameList)))
	{
		myState = eObjectDeleted;
	}
	else
	{
		if (NULL != myModifiedObjectsList_p)
		{
			myModifiedObjectsList_p->Insert(theObjectNameList, theImmName,  theAttributeName, attributeValue);
		}
		else
		{
			ERR_OAMSA_TRANSLATIONS("OamSACache::UpdateObjectAttribute myModifiedObjectsList_p is NULL");
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return myState;
}

OamSACache::ObjectState OamSACache::GetAttribute(const std::string& theObjectName,
													const std::string& theAttributeName,
													MafMoAttributeValueContainer_3T ** attributeValue_pp)
{
	ENTER_OAMSA_TRANSLATIONS();
	DNList theList;
	SplitDN(theObjectName, theList);
	LEAVE_OAMSA_TRANSLATIONS();
	return GetAttribute(theList, theAttributeName,attributeValue_pp);
}

OamSACache::ObjectState OamSACache::GetAttribute(DNList& theList,
													const std::string& theAttributeName,
													MafMoAttributeValueContainer_3T ** attributeValue_pp)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool FindVal = false;
	if (NULL != myModifiedObjectsList_p)
	{
		FindVal = myModifiedObjectsList_p->Find(theList, theAttributeName, attributeValue_pp);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::GetAttribute myModifiedObjectsList_p is NULL");
	}
	if (FindVal)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return eObjectSuccess;
	}
	else
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return eObjectNotFound;
	}
}

std::string OamSACache::GetNextChildObject(DNList& theObjectList, const std::string& theObjectClass, const std::string& thePreviousSibling)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string theSiblingName;
	if (NULL != myModifiedObjectsList_p)
	{
		theSiblingName = myModifiedObjectsList_p->GetNextChildObject(theObjectList, theObjectClass, thePreviousSibling);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::GetNextChildObject myModifiedObjectsList_p is NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return theSiblingName;
}

OamSACacheCreatedObjectsList*	OamSACache::GetListOfCreatedObjects()
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSACacheCreatedObjectsList* col = new OamSACacheCreatedObjectsList();
	this->GetCreatedData(*col);
	LEAVE_OAMSA_TRANSLATIONS();
	return col;
}

OamSACacheDeletedObjectsList* OamSACache::GetListOfDeletedObjects()
{
	ENTER_OAMSA_TRANSLATIONS();
	// Assume max 25 deleted objects initially after that, space in the vector will be added as needed
	// possibly resulting in a reallocation

	OamSACacheDeletedObjectsList* col = new OamSACacheDeletedObjectsList();
	this->GetDeletedData(*col);
	LEAVE_OAMSA_TRANSLATIONS();
	return col;
}

OamSACacheModifiedObjectsList OamSACache::GetListOfModifiedObjects()
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSACacheModifiedObjectsList* mol = new OamSACacheModifiedObjectsList();
	this->GetModifiedData(*mol);
	setFinalModifiedObjectsList(*mol);
	delete mol;
	LEAVE_OAMSA_TRANSLATIONS();
	return getFinalModifiedObjectsList();
}

void OamSACache::prepareListOfModifiedObjects(OamSACacheCreatedObjectsList& creObjList)
{
	ENTER_OAMSA_TRANSLATIONS();
	unsigned int noOfCreObj = creObjList.NoOfObjects();
	DEBUG_OAMSA_TRANSLATIONS("OamSACache::prepareListOfModifiedObjects Number of created objects = %u", noOfCreObj);

	OamSACacheModifiedObjectsList *modObjList = new OamSACacheModifiedObjectsList();
	std::vector<OamSACacheCreatedObjectData>::iterator it = creObjList.getMyObjectsIter();
	for(unsigned int creObjIndex = 0; creObjIndex < noOfCreObj; ++creObjIndex, ++it)
	{
		SaImmAttrModificationT_2** m_attrValues_pp;
		bool deletedAttrInList = false; //Main purpose of this implementation.

		SaImmAttrValuesT_2** c_attrValues_pp = it->AttrValues();
		unsigned int noOfEntries = 0;
		while (c_attrValues_pp[noOfEntries] != NULL) { ++noOfEntries; }
		DEBUG_OAMSA_TRANSLATIONS("OamSACache::prepareModifiedObjectList No of entries in %u = %u", creObjIndex, noOfEntries);

		// We create the ModifiedObjectsData entries with one entry less size because
		// one of the entries in CreatedObjectsData will be keyAttr.
		m_attrValues_pp = new SaImmAttrModificationT_2*[noOfEntries];

		std::string parentDn(it->getParent3GPPDN());
		std::string className(it->ClassName());
		std::string immRdn(it->ImmName());
		DEBUG_OAMSA_TRANSLATIONS("OamSACache::prepareListOfModifiedObjects parentDn :%s", parentDn.c_str());
		DEBUG_OAMSA_TRANSLATIONS("OamSACache::prepareListOfModifiedObjects className:%s", className.c_str());
		DEBUG_OAMSA_TRANSLATIONS("OamSACache::prepareListOfModifiedObjects immRdn:%s", immRdn.c_str());

		//Now we try to extract the keyAttribute from the Rdn.
		//for eg., if immRdn is 'fileGroupPolicyId=3,fileMId=1'
		//then the keyAttribute for the MOC FileGroupPolicy would be 'fileGroupPolicyId'
		std::string keyAttributeName = immRdn;
		try {
			// Remove everything after	=
			if (keyAttributeName.find('=') != std::string::npos)
			{
				keyAttributeName = keyAttributeName.substr(0, keyAttributeName.find('='));
			}
		} catch (...) { // Only to not core dump
			DEBUG_OAMSA_TRANSLATIONS("OamSACache::prepareListOfModifiedObjects failed to extract keyAttr from imm dn.");
		}
		DEBUG_OAMSA_TRANSLATIONS("OamSACache::prepareListOfModifiedObjects keyAttributeName:%s", keyAttributeName.c_str());

		bool keyFound = false; //will be true if we're able to retrieve keyAttr from the attributes container.
		unsigned int modIndex = 0;
		for (unsigned int index = 0; index < noOfEntries ; ++index)
		{
			std::string attrName(c_attrValues_pp[index]->attrName);
			DEBUG_OAMSA_TRANSLATIONS("AttrName :%s", attrName.c_str());

			//A filter step for key. supposed to handle struct attributes.
			//if you're attribute is struct and yet didn't hit this, then probably you need to
			//refer to section 3.1.3 of "Core MW SA Managed Object Modeling in Core MW Environment"
			/*FIX for TR HY67369*/
			if (keyAttributeName == "id")
			{
				std::string keyAttrValue;
				std::string structAttrName;
				if (keyAttributeName == attrName && !keyFound) {
					keyFound = fetchAttrValue(*c_attrValues_pp[index], keyAttrValue);
					if (keyFound) {
						DEBUG_OAMSA_TRANSLATIONS("struct keyAttrValue   :%s", keyAttrValue.c_str());
						if (keyAttrValue.find("_") != std::string::npos) {
							structAttrName = keyAttrValue.substr(0, keyAttrValue.find("_"));
						} else {
							keyFound = false;
							DEBUG_OAMSA_TRANSLATIONS("Unable to get struct attrib name from keyAttrValue('_' not found):%s", keyAttrValue.c_str());
						} //added new lines
					}
				}
				//OK, we got the struct attr name from the value.
				//now, we need to verify if it is a struct attribute or not.
				if (keyFound) {
					DEBUG_OAMSA_TRANSLATIONS("structAttrName   :%s", structAttrName.c_str());
					DNList theDNList;
					SplitDN(parentDn,theDNList);
					keyFound = theTranslator.isStructAttribute(theDNList, structAttrName);
					if (keyFound && theTranslator.isExclusiveStruct(parentDn, structAttrName.c_str())) {
						keyFound = false;
						break;
					}
				}
			}

			//struct type cases might not be caught here!
			else if ((!keyFound) && (keyAttributeName == attrName)) {
					keyFound = true;
			}

			//Rest of the attributes in the container
			else
			{
				unsigned int nrOfValues = c_attrValues_pp[index]->attrValuesNumber;
				if (nrOfValues > 0) {
					continue; //MAIN REASON BEHIND THIS IMPLEMENTATION!!!
				}
				bool status = false;
				OamSACacheCreatedObjectData::defaultvaluesMapIterator mapitr = it->getdefaultvaluesMap().find(c_attrValues_pp[index]->attrName);
				if (mapitr != it->getdefaultvaluesMap().end())
				{
					status = mapitr->second;
				}
				if(status == false){
					continue;
				}
				deletedAttrInList = true;
				DEBUG_OAMSA_TRANSLATIONS("Attribute copying to ModObj...%s", c_attrValues_pp[index]->attrName);
				//PrintAttributeValues(*c_attrValues_pp[index]); //For debugging purposes.
				m_attrValues_pp[modIndex] = new SaImmAttrModificationT_2;
				m_attrValues_pp[modIndex]->modType = SA_IMM_ATTR_VALUES_REPLACE;
				m_attrValues_pp[modIndex]->modAttr.attrName = new char[strlen(c_attrValues_pp[index]->attrName)+1];
				strcpy(m_attrValues_pp[modIndex]->modAttr.attrName, c_attrValues_pp[index]->attrName);
				m_attrValues_pp[modIndex]->modAttr.attrValueType = c_attrValues_pp[index]->attrValueType;
				m_attrValues_pp[modIndex]->modAttr.attrValuesNumber = c_attrValues_pp[index]->attrValuesNumber;
				m_attrValues_pp[modIndex]->modAttr.attrValues = CopyAllocAttributeValueArray((const void**)c_attrValues_pp[index]->attrValues,
						c_attrValues_pp[index]->attrValueType,
						c_attrValues_pp[index]->attrValuesNumber);
				++modIndex;
			}
		}
		while (modIndex < noOfEntries) {
			m_attrValues_pp[modIndex] = NULL;
			++modIndex;
		}
		if (!keyFound || !deletedAttrInList) {
			DEBUG_OAMSA_TRANSLATIONS(">>>ATTRIBUTE SKIPPED FOR ADDING TO modObjList<<<");
			//Ok, so either we were not able to extract the keyAttribute from the container
			//or, we there were no attributes that were configured to delete.
			//So, in either case, we have to empty our m_attrValues_pp container and skip 'add to modify list' step.
			for (unsigned int i = 0; i < noOfEntries ; ++i)
			{
				if (m_attrValues_pp[i] == NULL) {
					continue;
				}
				for (unsigned int j = 0; j < m_attrValues_pp[i]->modAttr.attrValuesNumber; j++)
				{
					DeleteVoidPointerValue(m_attrValues_pp[i]->modAttr.attrValues[j], m_attrValues_pp[i]->modAttr.attrValueType);
				}
				delete [] m_attrValues_pp[i]->modAttr.attrValues;
				m_attrValues_pp[i]->modAttr.attrValues = NULL;
				DEBUG_OAMSA_TRANSLATIONS(">>>skipping ... %s", m_attrValues_pp[i]->modAttr.attrName);
				delete [] m_attrValues_pp[i]->modAttr.attrName;
				m_attrValues_pp[i]->modAttr.attrName = NULL;
				delete m_attrValues_pp[i];
				m_attrValues_pp[i] = NULL;
			}
			delete [] m_attrValues_pp;
			m_attrValues_pp = NULL;
			continue;
		}
		SaNameT *immDn = makeSaNameT(immRdn.c_str());
		DEBUG_OAMSA_TRANSLATIONS("immName    :%s", immRdn.c_str());
		OamSACacheModifiedObjectData modObj(*immDn, m_attrValues_pp);
		modObjList->Insert(modObj);
		saNameDelete(immDn, true);
	}
	unsigned int noOfModObj = modObjList->NoOfObjects();
	DEBUG_OAMSA_TRANSLATIONS("OamSACache::prepareModifiedObjectList modified objects end = %u", noOfModObj);
	if (noOfModObj > 0) {
		setFinalModifiedObjectsList(*modObjList);
	}
	delete modObjList;
	LEAVE_OAMSA_TRANSLATIONS();
}

bool OamSACache::fetchAttrValue(SaImmAttrValuesT_2& AttrValues, std::string& keyAttrValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	try {
		std::string attrValue(*(char**)AttrValues.attrValues[0]);
		if (attrValue.find('=') != std::string::npos) {
			keyAttrValue = attrValue.substr(attrValue.find('=')+1); //extracting only the key-id from keyAttrValue.
		}
	} catch (...) {
		DEBUG_OAMSA_TRANSLATIONS("OamSACache::fetchAttrValue Unable to retrieve the Key Attribute value. Probably attrType is not String.");
		return false;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}

void OamSACache::clearFinalModifiedObjectsList()
{
	ENTER_OAMSA_TRANSLATIONS();
	finalModifiedObjectsList_p.clearMyObjects();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheModifiedObjectsList OamSACache::getFinalModifiedObjectsList()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return finalModifiedObjectsList_p;
}

void OamSACache::setFinalModifiedObjectsList(OamSACacheModifiedObjectsList& modObjList)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (0 == finalModifiedObjectsList_p.NoOfObjects()) {
		finalModifiedObjectsList_p = modObjList;
	}
	else {
		unsigned int noOfModObj = modObjList.NoOfObjects();
		std::vector<OamSACacheModifiedObjectData>::iterator it = modObjList.getMyObjectsIter();
		for (unsigned int modObjIndex = 0; modObjIndex < noOfModObj; ++modObjIndex, ++it) {
			finalModifiedObjectsList_p.Insert(*it);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * SplitDN. Divied a DN/RDN into its sections. Example "CM=blurpo,OM=bloppo,DM=koppo" is turned into
 *
 * theSplitDN[0] = "CM=blurpo"
 * theSplitDN[1] = "OM=bloppo"
 * theSplitDN[2] = "DM=koppo"
 */
void OamSACache::SplitDN(const std::string& theDN, DNList& theSplitDN)
{
	GlobalSplitDN(theDN,theSplitDN);
}

void OamSACache::GetCreatedData(OamSACacheCreatedObjectsList& col)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (NULL != myModifiedObjectsList_p)
	{
		myModifiedObjectsList_p->GetCreatedData(col);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::GetCreatedData myModifiedObjectsList_p is NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSACache::GetModifiedData(OamSACacheModifiedObjectsList& mol)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (NULL != myModifiedObjectsList_p)
	{
		myModifiedObjectsList_p->GetModifiedData(mol);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::GetModifiedData myModifiedObjectsList_p is NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSACache::GetDeletedData(OamSACacheDeletedObjectsList& dol)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (NULL != myDeletedObjectsList_p)
	{
		myDeletedObjectsList_p->GetDeletedData(dol);
	}
	else
	{
		ERR_OAMSA_TRANSLATIONS("OamSACache::GetDeletedData myDeletedObjectsList_p is NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


bool OamSACache::ReleaseAttributeValueContainer(MafMoAttributeValueContainer_3T* container)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool Retval = false;
	std::list<MafMoAttributeValueContainer_3T*>::iterator ValCListIter;
	DEBUG_OAMSA_TRANSLATIONS("OamSACache::ReleaseAttributeValueContainer before deleting container");

	for (ValCListIter = myAttValCList.begin() ;( Retval == false) && (ValCListIter != myAttValCList.end()) ;)
	{
		MafMoAttributeValueContainer_3T* vcp = *ValCListIter;
		if(vcp == container)
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSACache::ReleaseAttributeValueContainer located container, de-allocating and removing from list");
			// Remove this container from the list
			MafOamSpiMoAttributeType_3 vct = vcp->type;
			if (vct == MafOamSpiMoAttributeType_3_STRUCT)
			{
				for (unsigned int i = 0; i < vcp->nrOfValues; i++){
					MafMoAttributeValueStructMember_3T *member = vcp->values[i].value.structMember;
					while(member != NULL){
						delete[] (char*)member->memberName;
						Retval = ReleaseAttributeValueContainer(member->memberValue);
						MafMoAttributeValueStructMember_3T* tmp = member;
						member = member->next;
						delete tmp;
					}
				}
			}
			else if (vct == MafOamSpiMoAttributeType_3_STRING)
			{
				for (unsigned int i = 0; i < vcp->nrOfValues; i++){
					delete[] vcp->values[i].value.theString;
				}
			}
			else if (vct == MafOamSpiMoAttributeType_3_REFERENCE)
			{
				for (unsigned int i = 0; i < vcp->nrOfValues; i++){
					delete[] vcp->values[i].value.moRef;
				}
			}
			delete [] vcp->values;
			delete *ValCListIter; // delete vcp

			myAttValCList.erase(ValCListIter);
			Retval = true;
		}
		else
		{
			// Can only increment if it has not been deleted yet, otherwise we are accessing free'd memory.
			ValCListIter++;
		}
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return Retval;
}


bool OamSACache::GetImmAttributeData(char* className,
										char* attributeName,
										SaImmClassCategoryT& category,
										SaImmValueTypeT&	valueType,
										SaImmAttrFlagsT&	flags)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;

	DEBUG_OAMSA_TRANSLATIONS("OamSACache::GetImmAttributeData.  className %s , attribute name %s immHandle %u",className, attributeName,(unsigned int) myImmHandle);

	if (myImmHandle != 0)
	{
		SaImmAttrDefinitionT_2** theDefinitions = NULL;
		SaAisErrorT err = saImmOmClassDescriptionGet_2(myImmHandle, className, &category, &theDefinitions);
		if (SA_AIS_OK == err)
		{
			if(theDefinitions != NULL)
			{
			for (int i = 0; theDefinitions[i] != NULL; i++)
			{
				if (!strcmp(theDefinitions[i]->attrName,attributeName))
				{
					flags 		= theDefinitions[i]->attrFlags;
					valueType 	= theDefinitions[i]->attrValueType;
					DEBUG_OAMSA_TRANSLATIONS("HIT!! at loop %d,value type %d, flags %u ",i, valueType, (unsigned int)flags) ;
					RetVal 		= true;
					break;
				}
			}
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSACache::GetImmAttributeData(): saImmOmClassDescriptionGet_2() returned with attrDefinitions = NULL");
			}
			// OK, return memory to the IMM database
			saImmOmClassDescriptionMemoryFree_2(myImmHandle,theDefinitions);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}


bool OamSACache::GetClassNameFromImm(const std::string& objectName, std::string& className)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool RetVal = false;
	if (myImmHandle != 0 && myAccessorHandle != 0)
	{
		RetVal = theTranslator.GetClassNameFromImm(objectName, className, myImmHandle, myAccessorHandle);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

void OamSACache::RegisterCString(BridgeImmIterator* biip, char* namep)
{
	ENTER_OAMSA_TRANSLATIONS();
	// if the iterator is not registerd in the map then create a new list
	// otherwise add the string to the existing list

	if (myBridgeImmIterMap.find(biip) == myBridgeImmIterMap.end())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamCache::RegisterCString strings list not found for iterator %lu", (unsigned long) biip);
		myBridgeImmIterMap[biip].push_back(namep);
#ifdef _TRACE_FLAG
		std::list<char*>& testList = myBridgeImmIterMap[biip];
#endif
		DEBUG_OAMSA_TRANSLATIONS("OamCache::RegisterCString The strings list  size for iterator %lu is: %u", (unsigned long) biip, (unsigned int) testList.size());
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("OamCache::RegisterCString The iterator is found in the map");
		std::list<char*>& theCStrList = myBridgeImmIterMap[biip];
		theCStrList.push_back(namep);
		DEBUG_OAMSA_TRANSLATIONS("OamCache::RegisterCString The strings list size for iterator %lu is: %u", (unsigned long) biip, (unsigned int) theCStrList.size());
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


void OamSACache::RegisterAttributeValueContainer(MafMoAttributeValueContainer_3T* valuec)
{
	ENTER_OAMSA_TRANSLATIONS();
	myAttValCList.push_back(valuec);
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSACache::RegisterBridgeImmIter(BridgeImmIterator* biip)
{
	ENTER_OAMSA_TRANSLATIONS();
	myBridgeImmIterList.push_back(biip);
	LEAVE_OAMSA_TRANSLATIONS();
}

SaImmHandleT OamSACache::GetImmHandle()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myImmHandle;
}

std::list<MafMoAttributeValueContainer_3T*>& OamSACache::GetAttValCList()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myAttValCList;
}

std::list<BridgeImmIterator*>& OamSACache::GetBridgeImmIter()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myBridgeImmIterList;
}

std::map<BridgeImmIterator*, std::list<char*> >& OamSACache::GetTheBridgeImmIterMap()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myBridgeImmIterMap;
}

OamSACacheModifiedObjectData::OamSACacheModifiedObjectData()
{
	ENTER_OAMSA_TRANSLATIONS();
	saNameSet("", &objectName);
	attrValues_pp = NULL;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheModifiedObjectData::~OamSACacheModifiedObjectData()
{
	ENTER_OAMSA_TRANSLATIONS();
	if (attrValues_pp !=  NULL)
	{
		for (int i = 0; attrValues_pp[i] != NULL; i++)
		{
			for (unsigned int j = 0; j < attrValues_pp[i]->modAttr.attrValuesNumber; j++)
			{
				DeleteVoidPointerValue(attrValues_pp[i]->modAttr.attrValues[j], attrValues_pp[i]->modAttr.attrValueType);
			}
			delete [] attrValues_pp[i]->modAttr.attrValues;
			delete [] attrValues_pp[i]->modAttr.attrName;
			delete attrValues_pp[i];
		}
		delete [] attrValues_pp;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


OamSACacheModifiedObjectData& OamSACacheModifiedObjectData::operator=(const OamSACacheModifiedObjectData& od)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (attrValues_pp !=  NULL)
	{
		for (int i = 0; attrValues_pp[i] != NULL; i++)
		{
			for (unsigned int j = 0; j < attrValues_pp[i]->modAttr.attrValuesNumber; j++)
			{
				DeleteVoidPointerValue(attrValues_pp[i]->modAttr.attrValues[j], attrValues_pp[i]->modAttr.attrValueType);
			}
			delete [] attrValues_pp[i]->modAttr.attrValues;
			delete [] attrValues_pp[i]->modAttr.attrName;
			delete attrValues_pp[i];
		}
		delete [] attrValues_pp;
	}

	int i;
	// copy the object name

	memcpy(&objectName, &od.objectName, sizeof(objectName));

	// Get the number of entries in the source list
	for (i = 0; od.attrValues_pp[i] != NULL; i++);

	attrValues_pp = new SaImmAttrModificationT_2*[i+1];
	for (int j = 0; j < i; j++)
	{
		attrValues_pp[j] 							= new SaImmAttrModificationT_2;
		attrValues_pp[j]->modType 					= od.attrValues_pp[j]->modType;
		attrValues_pp[j]->modAttr.attrName				= new char[strlen(od.attrValues_pp[j]->modAttr.attrName)+1];

		strcpy(attrValues_pp[j]->modAttr.attrName, od.attrValues_pp[j]->modAttr.attrName);

		attrValues_pp[j]->modAttr.attrValueType 	= od.attrValues_pp[j]->modAttr.attrValueType;
		attrValues_pp[j]->modAttr.attrValuesNumber 	= od.attrValues_pp[j]->modAttr.attrValuesNumber;
		attrValues_pp[j]->modAttr.attrValues = CopyAllocAttributeValueArray((const void**)od.attrValues_pp[j]->modAttr.attrValues,
																			od.attrValues_pp[j]->modAttr.attrValueType,
																			od.attrValues_pp[j]->modAttr.attrValuesNumber);
	}
	attrValues_pp[i] = NULL;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

OamSACacheModifiedObjectData::OamSACacheModifiedObjectData(const OamSACacheModifiedObjectData& od)
{
	ENTER_OAMSA_TRANSLATIONS();
	int i;
	// copy the object name
	memcpy(&objectName, &od.objectName, sizeof(objectName));

	// Get the number of entries in the source list
	for (i = 0; od.attrValues_pp[i] != NULL; i++);

	attrValues_pp = new SaImmAttrModificationT_2*[i+1];
	for (int j = 0; j < i; j++)
	{
		attrValues_pp[j] 							= new SaImmAttrModificationT_2;
		attrValues_pp[j]->modType 					= od.attrValues_pp[j]->modType;
		attrValues_pp[j]->modAttr.attrName				= new char[strlen(od.attrValues_pp[j]->modAttr.attrName)+1];

		strcpy(attrValues_pp[j]->modAttr.attrName, od.attrValues_pp[j]->modAttr.attrName);

		attrValues_pp[j]->modAttr.attrValueType 	= od.attrValues_pp[j]->modAttr.attrValueType;
		attrValues_pp[j]->modAttr.attrValuesNumber 	= od.attrValues_pp[j]->modAttr.attrValuesNumber;
		attrValues_pp[j]->modAttr.attrValues = CopyAllocAttributeValueArray((const void**)od.attrValues_pp[j]->modAttr.attrValues,
																			od.attrValues_pp[j]->modAttr.attrValueType,
																			od.attrValues_pp[j]->modAttr.attrValuesNumber);
	}
	attrValues_pp[i] = NULL;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheModifiedObjectData::OamSACacheModifiedObjectData(const std::string& ObjName, OamSAModifiedEntry& mod)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (ObjName.size() <= saNameMaxLen())
	{
		saNameSet(ObjName.data(), &objectName);
	}
	OamSAModifiedEntry::OamSAAttributeMapIterator	theIter = mod.GetAttributeMap().begin();
	unsigned int NoOfEntries = mod.NoOfAttributes();
	attrValues_pp = new SaImmAttrModificationT_2*[NoOfEntries+1];

	for (unsigned int j = 0; j < NoOfEntries; j++, theIter++)
	{
		OamSADataContainer::DataPointerListIterator	theDataIter;
		attrValues_pp[j] 							= new SaImmAttrModificationT_2;
		attrValues_pp[j]->modType					= SA_IMM_ATTR_VALUES_REPLACE;
		attrValues_pp[j]->modAttr.attrName			= new char[theIter->first.length()+1];

		strcpy(attrValues_pp[j]->modAttr.attrName, theIter->first.c_str());

		// NB! This call below has side-effects in that it also updates attrValueType and attrValuesNumber
		// not all that transparent, but there are reasons.
		attrValues_pp[j]->modAttr.attrValues = CreateAttributeValueArray(theIter->second,
																				attrValues_pp[j]->modAttr.attrValueType,
																				attrValues_pp[j]->modAttr.attrValuesNumber);
	}
	attrValues_pp[NoOfEntries] = NULL;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheModifiedObjectData::OamSACacheModifiedObjectData(const SaNameT& ObjName, SaImmAttrModificationT_2** t_attrValues_pp)
{
	ENTER_OAMSA_TRANSLATIONS();
	objectName = ObjName;
	attrValues_pp = t_attrValues_pp;
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  Created object data
 */
OamSACacheCreatedObjectData::OamSACacheCreatedObjectData()
{
	ENTER_OAMSA_TRANSLATIONS();
	attrValues_pp = NULL;
	className =  NULL;
	memset(&parentName,0,sizeof(parentName));
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheCreatedObjectData& OamSACacheCreatedObjectData::operator=(const OamSACacheCreatedObjectData& od)
{
	ENTER_OAMSA_TRANSLATIONS();

	// Remove any old allocations in the target object. Needed for the assignment operator but not
	// for the copy constructor.
	delete [] className;

	if (attrValues_pp !=  NULL)
	{
		for (int i = 0; attrValues_pp[i] != NULL; i++)
		{
			for (unsigned int j = 0; j < attrValues_pp[i]->attrValuesNumber; j++)
			{
				DeleteVoidPointerValue(attrValues_pp[i]->attrValues[j], attrValues_pp[i]->attrValueType);
			}
			delete [] attrValues_pp[i]->attrValues;
			delete [] attrValues_pp[i]->attrName;
			delete attrValues_pp[i];
		}
		delete [] attrValues_pp;
	}

	int i;

	// copy class name
	className = new char[strlen(od.className)+1];
	strcpy(className,od.className);

	// copy the parent name
	memcpy(&parentName, &od.parentName, sizeof(parentName));

	immName = od.immName;
	parent3GPPDN = od.parent3GPPDN;
	defaultvaluesMapInfo = od.defaultvaluesMapInfo;

	// Get the number of entries in the source list
	for (i = 0; od.attrValues_pp[i] != NULL; i++);

	attrValues_pp = new SaImmAttrValuesT_2*[i+1];
	for (int j = 0; j < i; j++)
	{
		attrValues_pp[j] 							= new SaImmAttrValuesT_2;
		attrValues_pp[j]->attrName					= new char[strlen(od.attrValues_pp[j]->attrName)+1];

		strcpy(attrValues_pp[j]->attrName, od.attrValues_pp[j]->attrName);

		attrValues_pp[j]->attrValueType 			= od.attrValues_pp[j]->attrValueType;
		attrValues_pp[j]->attrValuesNumber 			= od.attrValues_pp[j]->attrValuesNumber;
		attrValues_pp[j]->attrValues 				= CopyAllocAttributeValueArray((const void**)od.attrValues_pp[j]->attrValues,
																						od.attrValues_pp[j]->attrValueType,
																						od.attrValues_pp[j]->attrValuesNumber);
	}
	attrValues_pp[i] = NULL;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

OamSACacheCreatedObjectData::OamSACacheCreatedObjectData(const OamSACacheCreatedObjectData& od)
{
	ENTER_OAMSA_TRANSLATIONS();
	int i;
	// copy class name

	DEBUG_OAMSA_TRANSLATIONS("OamSACacheCreatedObjectData::OamSACacheCreatedObjectData(const OamSACacheCreatedObjectData& od) before copying classname");
	if (od.className != NULL)
	{
		className = new char[strlen(od.className)+1];
		strcpy(className,od.className);
		DEBUG_OAMSA_TRANSLATIONS("After Copying class name, class name = %s",className);
	}
	else
	{
		className = NULL;
		DEBUG_OAMSA_TRANSLATIONS("od,className was NULL!");
	}

	// copy the parent name
	saNameSet(saNameGet(&od.parentName), &parentName);
	immName=od.immName;
	parent3GPPDN = od.parent3GPPDN;
	defaultvaluesMapInfo = od.defaultvaluesMapInfo;

	if (od.attrValues_pp != NULL)
	{
		// Get the number of entries in the source list
		for (i = 0; od.attrValues_pp[i] != NULL; i++);
		{
			attrValues_pp = new SaImmAttrValuesT_2*[i+1];
			for (int j = 0; j < i; j++)
			{
				attrValues_pp[j] 							= new SaImmAttrValuesT_2;
				attrValues_pp[j]->attrName						= new char[strlen(od.attrValues_pp[j]->attrName)+1];

				strcpy(attrValues_pp[j]->attrName, od.attrValues_pp[j]->attrName);

				DEBUG_OAMSA_TRANSLATIONS("J = %d attribute name %s",j,od.attrValues_pp[j]->attrName);
				attrValues_pp[j]->attrValueType 	= od.attrValues_pp[j]->attrValueType;
				attrValues_pp[j]->attrValuesNumber 	= od.attrValues_pp[j]->attrValuesNumber;
				attrValues_pp[j]->attrValues 		= CopyAllocAttributeValueArray((const void**)od.attrValues_pp[j]->attrValues,
																					od.attrValues_pp[j]->attrValueType,
																					od.attrValues_pp[j]->attrValuesNumber);

			}
			attrValues_pp[i] = NULL;
		}
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("Source attrValues_pp NULL");
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * OamSACacheCreatedObjectData()
 */
OamSACacheCreatedObjectData::OamSACacheCreatedObjectData(const std::string& ObjName, OamSAModifiedEntry& mod, OamSACache& theCache)
{
	ENTER_OAMSA_TRANSLATIONS();

	// Copy the class name
	className = new char[mod.GetClassName().length()+1];
	strcpy(className, mod.GetClassName().c_str());
	immName = mod.GetImmName();
	parent3GPPDN = mod.GetParentName();

	std::list<std::string> theList;
	std::string theImmParentName;
	theCache.SplitDN(parent3GPPDN, theList);

	// MR26712 changes: now need to pass IsImmRoot() the undecorated class name and parent dn to determine unique class names in IMM.
	OamSACache::DNList theSplitDN;
	GlobalSplitDN(ObjName,theSplitDN);

	OamSACache::DNListReverseIterator theDNIter;
	std::string TheUndecoratedClassName = "";

	theDNIter = theSplitDN.rbegin();
	TheUndecoratedClassName = *theDNIter;
	DEBUG_OAMSA_TRANSLATIONS("OamSACacheCreatedObjectData: className [%s] undecorated [%s] parentname [%s]",className, TheUndecoratedClassName.c_str(), parent3GPPDN.c_str());
	TheUndecoratedClassName = (TheUndecoratedClassName.substr(0, TheUndecoratedClassName.find('.'))).c_str();

	if (theTranslator.IsImmRoot(mod.GetClassName(), TheUndecoratedClassName, parent3GPPDN))	{
		// The created object is root in it self
		theImmParentName ="";
	}else {
		theTranslator.MO2Imm_DN(theList, theImmParentName);
	}
	DEBUG_OAMSA_TRANSLATIONS("OamSACacheCreatedObjectData: theImmParentName...%s", theImmParentName.c_str());
	saNameSet(theImmParentName.data(), &parentName);

	OamSAModifiedEntry::OamSAAttributeMapIterator	theIter = mod.GetAttributeMap().begin();
	unsigned int NoOfEntries = mod.NoOfAttributes();
	attrValues_pp = new SaImmAttrValuesT_2*[NoOfEntries+1];

	for (unsigned int j = 0; j < NoOfEntries; j++, theIter++) {
		OamSADataContainer::DataPointerListIterator	theDataIter;
		attrValues_pp[j] 							= new SaImmAttrValuesT_2;
		attrValues_pp[j]->attrName					= new char[theIter->first.length()+1];

		strcpy(attrValues_pp[j]->attrName, theIter->first.c_str());
		// NB! This call below has side-effects in that it also updates attrValueType and attrValuesNumber
		// not all that transparent, but there are reasons.
		attrValues_pp[j]->attrValues = CreateAttributeValueArray(theIter->second,
																attrValues_pp[j]->attrValueType,
																attrValues_pp[j]->attrValuesNumber);
		defaultvaluesMapInfo.insert(std::pair<std::string, bool>(attrValues_pp[j]->attrName,theIter->second.Gethasdefaultvalue()));
	}
	attrValues_pp[NoOfEntries] = NULL;
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  Destructor
 */
OamSACacheCreatedObjectData::~OamSACacheCreatedObjectData()
{
	ENTER_OAMSA_TRANSLATIONS();
	saNameDelete(&parentName, false);
	delete [] className;
	defaultvaluesMapInfo.clear();
	if (attrValues_pp !=  NULL)
	{
		for (int i = 0; attrValues_pp[i] != NULL; i++)
		{
			for (unsigned int j = 0; j < attrValues_pp[i]->attrValuesNumber; j++)
			{
				DeleteVoidPointerValue(attrValues_pp[i]->attrValues[j], attrValues_pp[i]->attrValueType);
			}
			delete [] attrValues_pp[i]->attrValues;
			delete[] attrValues_pp[i]->attrName;
			delete attrValues_pp[i];
		}
		delete [] attrValues_pp;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSACacheCreatedObjectData::Dump()
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSACacheCreatedObjectData::Dump()");
	if (className != NULL)
		DEBUG_OAMSA_TRANSLATIONS("Class name %s   ", className);
	else
		DEBUG_OAMSA_TRANSLATIONS("Class name NULL ");
	std::string ParentName;
	ParentName.append(saNameGet(&parentName), saNameLen(&parentName));
	DEBUG_OAMSA_TRANSLATIONS("Parent name %s",ParentName.c_str());

	if (attrValues_pp != NULL)
	{
		for (int i = 0; attrValues_pp[i] != NULL; i++)
		{
			PrintAttributeValues(*attrValues_pp[i]);
		}
	}
	else
		DEBUG_OAMSA_TRANSLATIONS("attrValues_pp == NULL!!!");
	LEAVE_OAMSA_TRANSLATIONS();
}

SaImmClassNameT OamSACacheCreatedObjectData::ClassName()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return className;
}

SaNameT* OamSACacheCreatedObjectData::ParentName()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return &parentName;
}

std::string OamSACacheCreatedObjectData::ImmName()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return immName;
}

std::string OamSACacheCreatedObjectData::getParent3GPPDN()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return parent3GPPDN;
}


SaImmAttrValuesT_2** OamSACacheCreatedObjectData::AttrValues()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return attrValues_pp;
}

/**
 *  OamSACacheCreatedObjectsList
 */
OamSACacheCreatedObjectsList::OamSACacheCreatedObjectsList()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheCreatedObjectsList::OamSACacheCreatedObjectsList(unsigned int InitialVectorSize) : myObjects(InitialVectorSize)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheCreatedObjectsList::~OamSACacheCreatedObjectsList()
{
	ENTER_OAMSA_TRANSLATIONS();
	myObjects.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSACacheCreatedObjectsList::Insert(OamSACacheCreatedObjectData& oc)
{
	ENTER_OAMSA_TRANSLATIONS();
	myObjects.push_back(oc);
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheCreatedObjectData& OamSACacheCreatedObjectsList::operator[](unsigned int idx)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (idx < myObjects.size())
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return myObjects[idx];
	}
	else
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return myObjects[myObjects.size()];
	}
}

void OamSACacheCreatedObjectsList::Dump()
{
	ENTER_OAMSA_TRANSLATIONS();
	for (unsigned int i = 0; i < myObjects.size(); i++)
	{
		myObjects[i].Dump();
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSACacheCreatedObjectsList::NoOfObjects()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myObjects.size();
}

/**
 *  OamSACacheDeletedObjectsList
 *  This is the list supplied to the prepare phase to enable deletion from IMM
 */
OamSACacheDeletedObjectsList::OamSACacheDeletedObjectsList()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheDeletedObjectsList::OamSACacheDeletedObjectsList(unsigned int InitialVectorSize) : myObjects(InitialVectorSize)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheDeletedObjectsList::~OamSACacheDeletedObjectsList()
{
	ENTER_OAMSA_TRANSLATIONS();
	myObjects.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheDeletedObjectData::OamSACacheDeletedObjectData()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheDeletedObjectData::~OamSACacheDeletedObjectData()
{
	ENTER_OAMSA_TRANSLATIONS();
	saNameDelete(&objectName, false);
	LEAVE_OAMSA_TRANSLATIONS();
}

SaNameT* OamSACacheDeletedObjectData::ObjectName()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return &objectName;
}

OamSACacheDeletedObjectData& OamSACacheDeletedObjectsList::operator[](unsigned int idx)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (idx < myObjects.size())
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return myObjects[idx];
	}
	else
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return myObjects[myObjects.size()];
	}
}

/**
 * OamSACacheDeletedObjectData::OamSACacheDeletedObjectData(const std::string& theString)
 */
OamSACacheDeletedObjectData::OamSACacheDeletedObjectData(const std::string& theString)
{
	ENTER_OAMSA_TRANSLATIONS();
	saNameSet(theString.c_str(), &objectName);
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 * OamSACacheDeletedObjectData::OamSACacheDeletedObjectData(const char* str)
 */
OamSACacheDeletedObjectData::OamSACacheDeletedObjectData(const char* str)
{
	ENTER_OAMSA_TRANSLATIONS();
	saNameSet(str, &objectName);
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  OamSACacheDeletedObjectData::OamSACacheDeletedObjectData(const OamSACacheDeletedObjectData& oc)
 */
OamSACacheDeletedObjectData::OamSACacheDeletedObjectData(const OamSACacheDeletedObjectData& oc)
{
	ENTER_OAMSA_TRANSLATIONS();
	saNameSet(saNameGet(&oc.objectName), &objectName);
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *  OamSACacheDeletedObjectData& OamSACacheDeletedObjectData::operator=(const OamSACacheDeletedObjectData& oc)
 */
OamSACacheDeletedObjectData& OamSACacheDeletedObjectData::operator=(const OamSACacheDeletedObjectData& oc)
{
	ENTER_OAMSA_TRANSLATIONS();
	saNameSet(saNameGet(&oc.objectName), &objectName);
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

SaNameT* OamSACacheModifiedObjectData::ObjectName()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return &objectName;
}

SaImmAttrModificationT_2** OamSACacheModifiedObjectData::AttrValues()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return attrValues_pp;
}

void OamSACacheDeletedObjectsList::Insert(OamSACacheDeletedObjectData& oc)
{
	ENTER_OAMSA_TRANSLATIONS();
	myObjects.push_back(oc);
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *   OamSACacheModifiedObjectsList
 */
OamSACacheModifiedObjectsList::OamSACacheModifiedObjectsList()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheModifiedObjectsList::~OamSACacheModifiedObjectsList()
{
	ENTER_OAMSA_TRANSLATIONS();
	myObjects.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheModifiedObjectsList::OamSACacheModifiedObjectsList(unsigned int InitialVectorSize) : myObjects(InitialVectorSize)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSACacheModifiedObjectsList::Insert(OamSACacheModifiedObjectData& oc)
{
	ENTER_OAMSA_TRANSLATIONS();
	myObjects.push_back(oc);
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSACacheModifiedObjectData&  OamSACacheModifiedObjectsList::operator[](unsigned int idx)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (idx < myObjects.size())
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return myObjects[idx];
	}
	else
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return myObjects[myObjects.size()];
	}
}

unsigned int OamSACacheModifiedObjectsList::NoOfObjects()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myObjects.size();
}

std::vector<OamSACacheCreatedObjectData>::iterator OamSACacheCreatedObjectsList::getMyObjectsIter()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return this->myObjects.begin();
}

void OamSACacheCreatedObjectData::setAttrValues(SaImmAttrValuesT_2** ret_attrValues_pp)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	if (ret_attrValues_pp !=  NULL) { //just a precaution.
		attrValues_pp = ret_attrValues_pp;
	}
}

void OamSACacheModifiedObjectsList::clearMyObjects()
{
	ENTER_OAMSA_TRANSLATIONS();
	this->myObjects.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}

std::vector<OamSACacheModifiedObjectData>::iterator OamSACacheModifiedObjectsList::getMyObjectsIter()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return this->myObjects.begin();
}

unsigned int OamSACacheDeletedObjectsList::NoOfObjects()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myObjects.size();
}

MafReturnT OamSACache::checkObjectExistInImm(const std::string& objectName)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT RetVal = MafFailure;
	if (myImmHandle != 0 && myAccessorHandle !=0) {
		RetVal = theTranslator.checkObjectExistInImm(objectName, myImmHandle, myAccessorHandle);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return RetVal;
}

int removeMOMPrefix(const char *immDn, unsigned* size, char *prefixImmDN)
{
	enum{
		DN_ERROR = -1,
		DN_LMID = 0,
		DN_IMM = 2
	};

	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("removeMOMPrefix() ENTER");
	if (NULL == immDn)
	{
		return DN_ERROR;  // if input dn is NULL then no conversion
	}
	DEBUG_OAMSA_TRANSLATIONS("removeMOMPrefix() immDn=%s", immDn);
	std::string theString(immDn);
	std::string lmClassStr = ",lmId=1,systemFunctionsId=1";
	std::string licenseIdStr = "licenseId=";

	size_t pos = theString.find(licenseIdStr);
	if(pos != std::string::npos)
	{
		theString = theString.substr(pos);
		DEBUG_OAMSA_TRANSLATIONS("removeMOMPrefix() Found 'licenseId'. theString=%s", theString.c_str());
		pos = theString.find("=");
		if(pos != std::string::npos)
		{
			theString = theString.substr(pos+1);
			DEBUG_OAMSA_TRANSLATIONS("removeMOMPrefix() Value of licenseId. theString=%s", theString.c_str());
			if(std::string::npos == theString.find("="))
			{
				theString = licenseIdStr + theString + lmClassStr;
				*size = strlen(theString.c_str());
				DEBUG_OAMSA_TRANSLATIONS("removeMOMPrefix() return %s, length %d", theString.c_str(), *size);
				LEAVE_OAMSA_TRANSLATIONS();
				strncpy(prefixImmDN, theString.c_str(), *size);
				prefixImmDN[*size + 1] = '\0';
				DEBUG_OAMSA_TRANSLATIONS("returning immDN = %s \n", prefixImmDN);
				return DN_LMID; // when License ID is found, then saNameSetLen
			}
		}
	}
	DEBUG_OAMSA_TRANSLATIONS("removeMOMPrefix() return NULL; ");
	LEAVE_OAMSA_TRANSLATIONS();
	return DN_IMM; // for alarms where license id is not found in DN, convertToImm
}
