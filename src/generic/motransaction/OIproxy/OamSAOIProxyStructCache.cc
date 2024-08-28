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
 *   File:   OamSAOIProxyStructCache.cc
 *
 *   Author: emilzor
 *
 *   Date:   2012-02-28
 *
 *   This file implements handling arrays of complex type and
 *   sending data to COM
 *
 *   Reviewed: efaiami 2012-04-21
 *   Modify:   xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *
 *   Modify: xdonngu 2013-12-12  run cppcheck 1.62 and fix errors and warnings
 *   Modify: xdonngu 2014-04-18: Fix some potential memory leaks/lacking support for moRef type.
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <saImm.h>
#include <saImmOi.h>
#include "ComSA.h"

#include "MafMgmtSpiCommon.h"
#include "OamSAOIProxyStructCache.h"

using namespace std;

#define OIPROXY_CACHE_EMPTY			0
#define OIPROXY_CACHE_INUSE			1
#define OIPROXY_CMP_OK				0
#define OIPROXY_CMP_SAME			1
#define OIPROXY_CMP_NEXT			2	//	next in an array
#define OIPROXY_CMP_PARENT			3
#define OIPROXY_CMP_CHILD			4
#define OIPROXY_CMP_DIFFERENT		5
#define OIPROXY_CMP_NOT_EXIST		6
#define OIPROXY_CMP_EMPTY			7


typedef struct OIProxyAttrHelperMem {
	int status;		//	OIPROXY_CACHE_EMPTY or OIPROXY_CACHE_INUSE
	MafOamSpiManagedObject_3T *moIf;        //      MO interface used for the struct. Needs for ccbApply.
	char *the3gppDn;
	char *attrName;
	MafMoAttributeValueContainer_3T container;
} OIProxyAttrHelperMemT;

static map<SaImmOiCcbIdT, OIProxyAttrHelperMemT *> attrStructCache;


/**
 * Compare two DNs if they are of the same type
 * They are the same type if DNs are identical or they differ in cardinality of the same DN, e.g. arrays
 */
int compareDns(const char *dn1, const char *dn2) {
	ENTER_OIPROXY();

	if(!dn1 || !dn2) {
		LEAVE_OIPROXY();
		return OIPROXY_CMP_DIFFERENT;
	}

	if(!strcmp(dn1, dn2)) {
		LEAVE_OIPROXY();
		return OIPROXY_CMP_SAME;
	}
	LEAVE_OIPROXY();
	return OIPROXY_CMP_DIFFERENT;
}

/**
 * Check if a complex type object exists in the memory and if it's the same type as other data saved in memory
 */
int isSameStructType(SaImmOiCcbIdT ccbId, const char *dn, const char *attrName) {
	ENTER_OIPROXY();
	int cmp;
	map<SaImmOiCcbIdT, OIProxyAttrHelperMemT *>::iterator it = attrStructCache.find(ccbId);
	if(it == attrStructCache.end()) {
		LEAVE_OIPROXY();
		return OIPROXY_CMP_EMPTY;
	}

	OIProxyAttrHelperMemT *mem = (*it).second;
	if(!mem) {
		LEAVE_OIPROXY();
		return OIPROXY_CMP_NOT_EXIST;
	}

	if(mem->status == OIPROXY_CACHE_EMPTY) {
		LEAVE_OIPROXY();
		return OIPROXY_CMP_EMPTY;
	}

	if(strcmp(mem->attrName, attrName)) {
		LEAVE_OIPROXY();
		return OIPROXY_CMP_DIFFERENT;
	}

	cmp = compareDns(mem->the3gppDn, dn);
	if(cmp == OIPROXY_CMP_SAME || cmp == OIPROXY_CMP_NEXT) {
		LEAVE_OIPROXY();
		return OIPROXY_CMP_OK;
	}
	LEAVE_OIPROXY();
	return OIPROXY_CMP_DIFFERENT;
}

MafReturnT deleteAttrValueStruct(MafMoAttributeValueStructMember_3T *str) {
	ENTER_OIPROXY();
	if(!str) {
		LEAVE_OIPROXY();
		return MafFailure;
	}
	while(str) {
		if(str->memberName)
			free(str->memberName);

		if(str->memberValue) {
			MafMoAttributeValueContainer_3T *memberValue = str->memberValue;
			if(memberValue->type == MafOamSpiMoAttributeType_3_STRING)
			{
				for(unsigned int i=0; i<memberValue->nrOfValues; i++)
				{
					free(const_cast<char*>(memberValue->values[i].value.theString));
				}
			}
			else if(memberValue->type == MafOamSpiMoAttributeType_3_REFERENCE)
			{
				for(unsigned int i=0; i<memberValue->nrOfValues; i++)
				{
					free(const_cast<char*>(memberValue->values[i].value.moRef));
				}
			}
			free(memberValue->values);
			free(str->memberValue);
		}

		MafMoAttributeValueStructMember_3T *t = str;
		str = str->next;
		free(t);
	}
	LEAVE_OIPROXY();
	return MafOk;
}

MafReturnT emptyAttrValueContainer(MafMoAttributeValueContainer_3T *container) {
	ENTER_OIPROXY();
	if(!container) {
		LEAVE_OIPROXY();
		return MafFailure;
	}
	if(container->type != MafOamSpiMoAttributeType_3_STRUCT) {
		LEAVE_OIPROXY();
		return MafInvalidArgument;
	}
	for(unsigned int i=0; i<container->nrOfValues; i++)
		deleteAttrValueStruct(container->values[i].value.structMember);

	if(container->values) {
		free(container->values);
		container->values = NULL;
	}
	container->nrOfValues = 0;
	LEAVE_OIPROXY();
	return MafOk;
}

MafReturnT emptyHelperMemory(OIProxyAttrHelperMemT *mem) {
	ENTER_OIPROXY();
	if(!mem) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	if(mem->status == OIPROXY_CACHE_EMPTY) {
		LEAVE_OIPROXY();
		return MafOk;
	}

	if(mem->the3gppDn) {
		free(mem->the3gppDn);
		mem->the3gppDn = NULL;
	}

	if(mem->attrName) {
		free(mem->attrName);
		mem->attrName = NULL;
	}

	emptyAttrValueContainer(&mem->container);
	mem->status = OIPROXY_CACHE_EMPTY;
	LEAVE_OIPROXY();
	return MafOk;
}

/**
 * Duplicate attributes of a struct
 */
MafMoAttributeValueStructMember_3T *dupAttrValueStruct(MafMoAttributeValueStructMember_3T *src) {
	ENTER_OIPROXY();
	if(!src) {
		LEAVE_OIPROXY();
		return NULL;
	}

	MafMoAttributeValueStructMember_3T *dst = NULL;
	MafMoAttributeValueStructMember_3T *t = NULL;

	while(src) {
		if(!t) {
			dst = t = (MafMoAttributeValueStructMember_3T *)malloc(sizeof(MafMoAttributeValueStructMember_3T));
		} else {
			t->next = (MafMoAttributeValueStructMember_3T *)malloc(sizeof(MafMoAttributeValueStructMember_3T));
			t = t->next;
		}
		t->next = NULL;
		t->memberName = (char *)malloc(strlen(src->memberName) + 1);
		strcpy(t->memberName, src->memberName);

		if(src->memberValue) {
			t->memberValue = (MafMoAttributeValueContainer_3T *)malloc(sizeof(MafMoAttributeValueContainer_3T));
			t->memberValue->type = src->memberValue->type;
			t->memberValue->nrOfValues = src->memberValue->nrOfValues;
			t->memberValue->values = (MafMoAttributeValue_3T *)malloc(sizeof(MafMoAttributeValue_3T) * src->memberValue->nrOfValues);

			if(src->memberValue->type == MafOamSpiMoAttributeType_3_STRING)
			{
				for(unsigned int i=0; i<src->memberValue->nrOfValues; i++)
				{
					t->memberValue->values[i].value.theString = (char *)malloc(strlen(src->memberValue->values[i].value.theString) + 1);
					strcpy(const_cast<char*>(t->memberValue->values[i].value.theString), src->memberValue->values[i].value.theString);
				}
			}
			else if(src->memberValue->type == MafOamSpiMoAttributeType_3_REFERENCE)
			{
				for(unsigned int i=0; i<src->memberValue->nrOfValues; i++)
				{
					t->memberValue->values[i].value.moRef = (char *)malloc(strlen(src->memberValue->values[i].value.moRef) + 1);
					strcpy(const_cast<char*>(t->memberValue->values[i].value.moRef), src->memberValue->values[i].value.moRef);
				}
			}
			else
			{
				memcpy(t->memberValue->values, src->memberValue->values, sizeof(MafMoAttributeValue_3T) * src->memberValue->nrOfValues);
			}
		}
		else
			t->memberValue = NULL;
		src = src->next;
	}
	LEAVE_OIPROXY();
	return dst;
}

/**
 * Add an attribute container to the complex type
 * New added attributes will be added to the end of the list
 */
MafReturnT addAttrValueContainer(MafOamSpiManagedObject_3T *moIf, SaImmOiCcbIdT ccbId, const char *the3gppDn, const char *attrName, const MafMoAttributeValueContainer_3T *container) {
	ENTER_OIPROXY();
	if(!container) {
		LEAVE_OIPROXY();
		return MafFailure;
	}
	if(container->type != MafOamSpiMoAttributeType_3_STRUCT) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	OIProxyAttrHelperMem *mem = attrStructCache[ccbId];
	if(!mem) {	//	check if we already have ccbId in the memory, and if we don't, we shall create a new one
		mem = static_cast<OIProxyAttrHelperMem *>(malloc(sizeof(OIProxyAttrHelperMem)));
		mem->status = OIPROXY_CACHE_EMPTY;
		// FIXME: Can we save moIf like this ????
		mem->moIf = moIf;
		mem->the3gppDn = NULL;
		mem->attrName = NULL;
		memset(&mem->container, 0, sizeof(MafMoAttributeValueContainer_3T));
		mem->container.type = MafOamSpiMoAttributeType_3_STRUCT;
		attrStructCache[ccbId] = mem;
	}
	else
	{	// check if new attributes are the same type as attributes in memory
		int cmp = isSameStructType(ccbId, the3gppDn, attrName);
		if(cmp != OIPROXY_CMP_OK && cmp != OIPROXY_CMP_EMPTY)
		{
			LEAVE_OIPROXY();
			return MafInvalidArgument;
		}
	}

	//	Increasing memory space for new attributes
	MafMoAttributeValue_3T *newval = (MafMoAttributeValue_3T *)realloc(mem->container.values, sizeof(MafMoAttributeValue_3T) * (mem->container.nrOfValues + container->nrOfValues));
	if(!newval)
	{
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	TODO: Is it possible that dupAttrValueStruct return some weird results, and how should we handle it ???
	//	For now, I assume that input data are correct, and in that case dupAttrValueStruct cannot return NULL.
	for(unsigned int i=0; i<container->nrOfValues; i++)
		newval[mem->container.nrOfValues + i].value.structMember = dupAttrValueStruct(container->values[i].value.structMember);

	if(mem->status == OIPROXY_CACHE_EMPTY)
	{
		mem->the3gppDn = (char *)malloc(strlen(the3gppDn) + 1);
		strcpy(mem->the3gppDn, the3gppDn);
		mem->attrName = (char *)malloc(strlen(attrName) + 1);
		strcpy(mem->attrName, attrName);
	}

	mem->container.values = newval;
	mem->container.nrOfValues = mem->container.nrOfValues + container->nrOfValues;
	mem->status = OIPROXY_CACHE_INUSE;

	LEAVE_OIPROXY();
	return MafOk;
}

/**
 * Add a single attribute
 * If index == -1, that the attribute will be added to the last one in the list
 * It's possible to add a new attribute only into the existing list, or create another struct member and insert it there.
 * index cannot point to the struct array that is 2 or more in a head of current number of the array
 */
MafReturnT addAttrValue(MafOamSpiManagedObject_3T *moIf, SaImmOiCcbIdT ccbId, const char *the3gppDn, const char *attrName, int index, const char *memberName, MafOamSpiMoAttributeType_3T type, const MafMoAttributeValue_3T *values) {
	ENTER_OIPROXY();
	if(!the3gppDn || index < -1 || !memberName || !values)
	{
		LEAVE_OIPROXY();
		return MafFailure;
	}

	// We can insert only simple type attributes
	if(type == MafOamSpiMoAttributeType_3_STRUCT || type == MafOamSpiMoAttributeType_3_REFERENCE)
	{
		LEAVE_OIPROXY();
		return MafFailure;
	}

	OIProxyAttrHelperMem *mem = attrStructCache[ccbId];
	if(!mem)
	{	//	check if we already have ccbId in the memory, and if we don't, we shall create a new one
		mem = static_cast<OIProxyAttrHelperMem *>(malloc(sizeof(OIProxyAttrHelperMem)));
		mem->status = OIPROXY_CACHE_EMPTY;
		mem->moIf = moIf;
		mem->the3gppDn = NULL;
		mem->attrName = NULL;
		memset(&mem->container, 0, sizeof(MafMoAttributeValueContainer_3T));
		mem->container.type = MafOamSpiMoAttributeType_3_STRUCT;
		attrStructCache[ccbId] = mem;
	}
	else
	{	// check if new attributes are the same type as attributes in memory
		int cmp = isSameStructType(ccbId, the3gppDn, attrName);
		if(cmp != OIPROXY_CMP_OK && cmp != OIPROXY_CMP_EMPTY) {
			LEAVE_OIPROXY();
			return MafInvalidArgument;
		}
	}

	//	We don't need to clean upper created ccbId memory structure, it will be done later in the code when cbbId is released
	if(index > (int)mem->container.nrOfValues)
	{
		LEAVE_OIPROXY();
		return MafFailure;
	}

	MafMoAttributeValue_3T *val;
	MafMoAttributeValueStructMember_3T *str;
	if(index == (int)mem->container.nrOfValues) {	// a new struct is created
		// Increase the member array for a new struct
		val = (MafMoAttributeValue_3T *)realloc(mem->container.values, sizeof(MafMoAttributeValue_3T) * (mem->container.nrOfValues + 1));
		if(!val) {
			LEAVE_OIPROXY();
			return MafFailure;
		}

		if(mem->status == OIPROXY_CACHE_EMPTY) {
			mem->the3gppDn = (char *)malloc(strlen(the3gppDn) + 1);
			strcpy(mem->the3gppDn, the3gppDn);
			mem->attrName = (char *)malloc(strlen(attrName) + 1);
			strcpy(mem->attrName, attrName);
		}

		mem->container.values = val;
		mem->container.nrOfValues++;
		str = mem->container.values[index].value.structMember = (MafMoAttributeValueStructMember_3T *)malloc(sizeof(MafMoAttributeValueStructMember_3T));
		str->memberName = (char *)malloc(strlen(memberName) + 1);
		strcpy(str->memberName, memberName);
		str->next = NULL;
		str->memberValue = (MafMoAttributeValueContainer_3T *)malloc(sizeof(MafMoAttributeValueContainer_3T));
		str->memberValue->nrOfValues = 1;
		str->memberValue->type = type;
		str->memberValue->values = (MafMoAttributeValue_3T *)malloc(sizeof(MafMoAttributeValue_3T));
		if(type == MafOamSpiMoAttributeType_3_STRING) {
			str->memberValue->values[0].value.theString = (char *)malloc(strlen(values->value.theString) + 1);
			strcpy(const_cast<char*>(str->memberValue->values[0].value.theString), values->value.theString);
		} else
			str->memberValue->values[0].value = values->value;
		mem->status = OIPROXY_CACHE_INUSE;
		LEAVE_OIPROXY();
		return MafOk;
	} else if(index == -1)	//	Add the new attr value to the end of list
		str = mem->container.values[mem->container.nrOfValues - 1].value.structMember;
	else if(index >= 0)		//	Add the new value to the [index] struct member in the list
		str = mem->container.values[index].value.structMember;
	else {
		LEAVE_OIPROXY();
		return MafFailure;
	}
	if(!str) {
		LEAVE_OIPROXY();
		return MafFailure;
	}
	if(mem->status == OIPROXY_CACHE_EMPTY) {
		mem->the3gppDn = (char *)malloc(strlen(the3gppDn) + 1);
		strcpy(mem->the3gppDn, the3gppDn);
		mem->attrName = (char *)malloc(strlen(attrName) + 1);
		strcpy(mem->attrName, attrName);
	}

	// First we need to check if the attribute already exists in the attribute list
	MafMoAttributeValueStructMember_3T *t = str;
	while(t) {
		t = str->next;
		if(!strcmp(str->memberName, memberName))
			break;

		if(t)
			str = str->next;
	}

	int ind;
	MafMoAttributeValueContainer_3T *memberValue;
	if(t) {		//	The new attr is in the list. The attr value list will be increased by one.
		memberValue = str->memberValue;
		memberValue->values = (MafMoAttributeValue_3T *)realloc(memberValue->values, sizeof(MafMoAttributeValue_3T) * (memberValue->nrOfValues + 1));
		ind = memberValue->nrOfValues;
		memberValue->nrOfValues++;
	} else {	//	The new attr is not in the list. We need to add it
		memberValue = (MafMoAttributeValueContainer_3T *)malloc(sizeof(MafMoAttributeValueContainer_3T));
		memberValue->type = type;
		memberValue->nrOfValues = 1;
		memberValue->values = (MafMoAttributeValue_3T *)malloc(sizeof(MafMoAttributeValue_3T));

		t = (MafMoAttributeValueStructMember_3T *)malloc(sizeof(MafMoAttributeValueStructMember_3T));
		t->memberName = (char *)malloc(strlen(memberName) + 1);
		strcpy(t->memberName, memberName);
		t->next = NULL;
		t->memberValue = memberValue;
		str->next = t;
		ind = 0;
	}

	if(type == MafOamSpiMoAttributeType_3_STRING) {
		memberValue->values[ind].value.theString = (char *)malloc(strlen(values->value.theString) + 1);
		strcpy(const_cast<char*>(memberValue->values[ind].value.theString), values->value.theString);
	} else
		memberValue->values[ind].value = values->value;
	mem->status = OIPROXY_CACHE_INUSE;

	LEAVE_OIPROXY();
	return MafOk;
}

/**
 * Clean data for next complex type within the same ccbId
 * ccbId still remains in the memory
 */
MafReturnT cleanCcbId(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	map<SaImmOiCcbIdT, OIProxyAttrHelperMemT *>::iterator it = attrStructCache.find(ccbId);
	if(it == attrStructCache.end()) {
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	OIProxyAttrHelperMemT *mem = (*it).second;
	if(!mem) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	if(mem->status == OIPROXY_CACHE_EMPTY) {
		LEAVE_OIPROXY();
		return MafOk;
	}

	emptyHelperMemory(mem);
	mem->status = OIPROXY_CACHE_EMPTY;
	LEAVE_OIPROXY();
	return MafOk;
}

/**
 * Call this function when finalize of ccbId is called
 * This function clean everything from memory that is connected to ccbId
 */
MafReturnT closeCcbId(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	MafReturnT ret = cleanCcbId(ccbId);

	map<SaImmOiCcbIdT, OIProxyAttrHelperMemT *>::iterator it = attrStructCache.find(ccbId);
	if(it != attrStructCache.end())
		free((*it).second);

	if(ret == MafOk)
		attrStructCache.erase(ccbId);
	LEAVE_OIPROXY();
	return ret;
}


MafReturnT OIProxy_cache_existCacheData(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();

	map<SaImmOiCcbIdT, OIProxyAttrHelperMemT *>::iterator it = attrStructCache.find(ccbId);
	if(it == attrStructCache.end()) {
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	OIProxyAttrHelperMemT *mem = (*it).second;
	if(!mem) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	if(mem->status == OIPROXY_CACHE_EMPTY) {
		LEAVE_OIPROXY();
		return MafNotExist;
	}
	LEAVE_OIPROXY();
	return MafOk;
}

/**
 * Retrieve data for a structure
 * NEVER release return value. It will be released when ccbId is closed
 */
MafMoAttributeValueContainer_3T *OIProxy_cache_getCacheData(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	map<SaImmOiCcbIdT, OIProxyAttrHelperMemT *>::iterator it = attrStructCache.find(ccbId);
	if(it == attrStructCache.end()) {
		LEAVE_OIPROXY();
		return NULL;
	}

	OIProxyAttrHelperMemT *mem = (*it).second;
	if(!mem) {
		LEAVE_OIPROXY();
		return NULL;
	}

	if(mem->status == OIPROXY_CACHE_EMPTY) {
		LEAVE_OIPROXY();
		return NULL;
	}

	LEAVE_OIPROXY();
	return &mem->container;
}

/**
 * Retrieve MO interface for a structure
 * NEVER release return value. It will be released when ccbId is closed
 */
MafOamSpiManagedObject_3T *OIProxy_cache_getCacheMOIf(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();

	map<SaImmOiCcbIdT, OIProxyAttrHelperMemT *>::iterator it = attrStructCache.find(ccbId);
	if(it == attrStructCache.end()) {
		LEAVE_OIPROXY();
		return NULL;
	}

	OIProxyAttrHelperMemT *mem = (*it).second;
	if(!mem) {
		LEAVE_OIPROXY();
		return NULL;
	}

	if(mem->status == OIPROXY_CACHE_EMPTY) {
		LEAVE_OIPROXY();
		return NULL;
	}

	LEAVE_OIPROXY();
	return mem->moIf;
}

/**
 * Retrieve all memory data at once
 * NEVER release memory from retrieved parameters
 */
MafReturnT OIProxy_cache_getAllCacheData(SaImmOiCcbIdT ccbId, char **the3gppDn, char **attrName, MafMoAttributeValueContainer_3T **container, MafOamSpiManagedObject_3T **moIf) {
	ENTER_OIPROXY();
	if(!the3gppDn || !attrName || !container) {
		LEAVE_OIPROXY();
		return MafInvalidArgument;
	}

	map<SaImmOiCcbIdT, OIProxyAttrHelperMemT *>::iterator it = attrStructCache.find(ccbId);
	if(it == attrStructCache.end()) {
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	OIProxyAttrHelperMemT *mem = (*it).second;
	if(!mem) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	if(mem->status == OIPROXY_CACHE_EMPTY) {
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	*moIf = mem->moIf;
	*the3gppDn = mem->the3gppDn;
	*attrName = mem->attrName;
	*container = &mem->container;

	if(!moIf) {
		DEBUG_OIPROXY("OIProxy_cache_getAllCacheData(): Value of moIf is NULL");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	LEAVE_OIPROXY();
	return MafOk;
}

MafReturnT OIProxy_cache_sendMoData(MafOamSpiTransactionHandleT txHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	char *the3gpp;
	char *attrName;
	MafMoAttributeValueContainer_3T *container;
	MafOamSpiManagedObject_3T *moIf;
	//	Take all relevant data at once ....
	MafReturnT ret = OIProxy_cache_getAllCacheData(ccbId, &the3gpp, &attrName, &container,&moIf);
	if(ret != MafOk)
	{
		ERR_OIPROXY("OIProxy_cache_sendMoData(): ERROR OIProxy_cache_getAllCacheData() Failed with error code (%d)", ret);
		LEAVE_OIPROXY();
		return ret;
	}

	//FIXME: Change this to take version dynamically. Not by component!
	bool isMoSpiVersion2 = (0 == strcmp(_CC_NAME, "lm"));

	if(isMoSpiVersion2)
	{
		DEBUG_OIPROXY("OIProxy_cache_sendMoData(): _2");
		MafOamSpiManagedObject_2T* moIf_2 = reinterpret_cast<MafOamSpiManagedObject_2T*>(moIf);
		ret = moIf_2->setMoAttribute(txHandle, the3gpp, attrName, (MafMoAttributeValueContainer_2T*)container);
	}
	else
	{
		DEBUG_OIPROXY("OIProxy_cache_sendMoData(): _3");
		ret = moIf->setMoAttribute(txHandle, the3gpp, attrName, container);
	}

	if (MafOk != ret)
	{
		ERR_OIPROXY("OIProxy_cache_sendMoData(): ERROR moIf->setMoAttribute() Failed with error code (%d)", ret);
	}
	else
	{
		DEBUG_OIPROXY("OIProxy_cache_sendMoData(): after moIf->setMoAttribute() RETURN with (%d)",ret);
	}
	LEAVE_OIPROXY();
	return ret;
}

MafReturnT OIProxy_cache_sendCache(MafOamSpiManagedObject_3T *moIf, MafOamSpiTransactionHandleT txHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();
	MafReturnT ret = OIProxy_cache_sendMoData(txHandle, ccbId);
	if(ret != MafOk) {
		LEAVE_OIPROXY();
		return ret;
	}

	cleanCcbId(ccbId);
	LEAVE_OIPROXY();
	return ret;
}

MafReturnT OIProxy_cache_setMoAttribute(MafOamSpiManagedObject_3T*        moIf,
                                        MafOamSpiTransactionHandleT       txHandle,
                                        SaImmOiCcbIdT                     ccbId,
                                        const char*                       the3gpp,
                                        const char*                       attrName,
                                        MafOamSpiMoAttributeType_3T       attrType,
                                        MafMoAttributeValueContainer_3T*  attrValue,
                                        bool                              isMoSpiVersion2)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): ENTER");
	if(!moIf || !the3gpp || !attrName || !attrValue) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	MafReturnT ret;
	if(attrType != MafOamSpiMoAttributeType_3_STRUCT) {
		//	If the attribute is not a struct type, then first send struct type attribute, and then send incoming attribute
		if(OIProxy_cache_existCacheData(ccbId) == MafOk)
		{
			ret = OIProxy_cache_sendMoData(txHandle, ccbId);
			if(ret != MafOk)
			{
				DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): after 1. call to OIProxy_cache_sendMoData() RETURN with (%d)",ret);
				LEAVE_OIPROXY();
				return ret;
			}
			cleanCcbId(ccbId);
		}
		DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): calling moIf->setMoAttribute() with txHandle(%lu) the3gpp(%s) attrName(%s)",txHandle, the3gpp, attrName);
		MafReturnT retValSetMo = MafOk;
		if(isMoSpiVersion2)
		{
			DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): _2");
			MafOamSpiManagedObject_2T* moIf_2 = reinterpret_cast<MafOamSpiManagedObject_2T*>(moIf);
			retValSetMo = moIf_2->setMoAttribute(txHandle, the3gpp, attrName, (MafMoAttributeValueContainer_2T*)attrValue);
		}
		else
		{
			DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): _3");
			retValSetMo = moIf->setMoAttribute(txHandle, the3gpp, attrName, attrValue);
		}

		DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): after moIf->setMoAttribute() RETURN with (%d)", retValSetMo);
		LEAVE_OIPROXY();
		return retValSetMo;
	}

	int cmp = isSameStructType(ccbId, the3gpp, attrName);
	if(cmp == OIPROXY_CMP_DIFFERENT)
	{
		//	If there is already different attribute type, send it first
		ret = OIProxy_cache_sendMoData(txHandle, ccbId);
		if(ret != MafOk)
		{
			DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): after 2. call to OIProxy_cache_sendMoData() RETURN with (%d)",ret);
			LEAVE_OIPROXY();
			return ret;
		}
		cleanCcbId(ccbId);
	}
	else if(cmp != OIPROXY_CMP_OK && cmp != OIPROXY_CMP_EMPTY)
	{
		DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): RETURN MafFailure");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Add a new struct attribute
	ret = addAttrValueContainer(moIf, ccbId, the3gpp, attrName, attrValue);
	DEBUG_OIPROXY("OIProxy_cache_setMoAttribute(): RETURN with (%d)",ret);
	LEAVE_OIPROXY();
	return ret;
}

MafReturnT OIProxy_cache_deleteMo(MafOamSpiManagedObject_3T*   moIf,
                                  MafOamSpiTransactionHandleT  txHandle,
                                  SaImmOiCcbIdT                ccbId,
                                  const char*                  the3gpp,
                                  bool                         isMoSpiVersion2)
{
	ENTER_OIPROXY();
	if(!moIf || !the3gpp) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	MafReturnT ret;
	//	First check if any complex struct exists in memory and send it to the interface ....
	if(OIProxy_cache_existCacheData(ccbId) == MafOk) {
		ret = OIProxy_cache_sendMoData(txHandle, ccbId);
		if(ret != MafOk) {
			LEAVE_OIPROXY();
			return ret;
		}
		cleanCcbId(ccbId);
	}

	//	... then delete MO object
	if(isMoSpiVersion2)
	{
		DEBUG_OIPROXY("OIProxy_cache_deleteMo(): deleteMo() _2");
		MafOamSpiManagedObject_2T* moIf_2 = reinterpret_cast<MafOamSpiManagedObject_2T*>(moIf);
		ret = moIf_2->deleteMo(txHandle, the3gpp);
	}
	else
	{
		DEBUG_OIPROXY("OIProxy_cache_deleteMo(): deleteMo() _3");
		ret = moIf->deleteMo(txHandle, the3gpp);
	}

	if (MafOk != ret)
	{
		ERR_OIPROXY("OIProxy_cache_deleteMo(): deleteMo() Failed with %d", ret);
	}
	LEAVE_OIPROXY();
	return ret;
}

MafReturnT OIProxy_cache_createMo(MafOamSpiManagedObject_3T*  moIf,
                                  MafOamSpiTransactionHandleT txHandle,
                                  SaImmOiCcbIdT               ccbId,
                                  const char*                 the3gpp,
                                  const char*                 className,
                                  const char*                 keyAttrName,
                                  const char*                 keyAttrValue,
                                  bool                        isMoSpiVersion2)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("OIProxy_cache_createMo(): ENTER");
	if(!moIf || !the3gpp || !className || !keyAttrName || !keyAttrValue)
	{
		DEBUG_OIPROXY("OIProxy_cache_createMo(): RETURN MafFailure");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Check if there is any complex struct, and send it to interface ....
	if(OIProxy_cache_existCacheData(ccbId) == MafOk)
	{
		DEBUG_OIPROXY("OIProxy_cache_createMo(): OIProxy_cache_existCacheData(ccbId=%llu) == MafOk",ccbId);
		MafReturnT ret = OIProxy_cache_sendMoData(txHandle, ccbId);
		if(ret != MafOk)
		{
			DEBUG_OIPROXY("OIProxy_cache_createMo(): RETURN with (%d)",ret);
			LEAVE_OIPROXY();
			return ret;
		}
		cleanCcbId(ccbId);
	}
	//	... then send create MO object to the interface

	// initialAttributes - new argument in SPI Ver.3
	MafMoNamedAttributeValueContainer_3T ** initialAttributes = NULL;
	// it is safe to set initialAttributes to NULL until we decide how to use it.
	DEBUG_OIPROXY("OIProxy_cache_createMo(): calling moIf->createMo() with txHandle(%lu) the3gpp(%s) className(%s) keyAttrName(%s) keyAttrValue(%s)",txHandle, the3gpp, className, keyAttrName, keyAttrValue);
	MafReturnT retValue = MafOk;
	if(isMoSpiVersion2)
	{
		DEBUG_OIPROXY("OIProxy_cache_createMo(): _2");
		MafOamSpiManagedObject_2T* moIf_2 = reinterpret_cast<MafOamSpiManagedObject_2T*>(moIf);
		retValue = moIf_2->createMo(txHandle, the3gpp, className, keyAttrName, keyAttrValue);
	}
	else
	{
		DEBUG_OIPROXY("OIProxy_cache_createMo(): _3");
		retValue = moIf->createMo(txHandle, the3gpp, className, keyAttrName, keyAttrValue, initialAttributes);
	}

	if (MafOk != retValue)
	{
		ERR_OIPROXY("OIProxy_cache_createMo(): RETURN with (%d)", retValue);
	}
	LEAVE_OIPROXY();
	return retValue;
}


MafReturnT OIProxy_cache_ccbApply(MafOamSpiManagedObject_3T *moIf, MafOamSpiTransactionHandleT txHandle, SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();

	if(!moIf) {
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	If any complex struct has left in the memory, then send it to the interface and empty the memory
	if(OIProxy_cache_existCacheData(ccbId) == MafOk) {
		MafReturnT ret = OIProxy_cache_sendMoData(txHandle, ccbId);
		if(ret != MafOk) {
			LEAVE_OIPROXY();
			return ret;
		}
		cleanCcbId(ccbId);
	}

	LEAVE_OIPROXY();
	return MafOk;
}


MafReturnT OIProxy_cache_ccbAbort(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();

	//	If any complex struct has left in the memory, then empty the memory. there is no need to send it to an interface
	if(OIProxy_cache_existCacheData(ccbId) == MafOk)
		cleanCcbId(ccbId);

	LEAVE_OIPROXY();
	return MafOk;
}


MafReturnT OIProxy_cache_ccbFinalize(SaImmOiCcbIdT ccbId) {
	ENTER_OIPROXY();

	// Remove all with ccbId
	closeCcbId(ccbId);

	LEAVE_OIPROXY();
	return MafOk;
}
