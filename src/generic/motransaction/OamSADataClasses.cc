/**
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
 *   File:   OamSADataClasses.h
 *
 *   Author: egorped
 *
 *   Date:   2010-05-21
 *
 *   This file implement Data classes for the cache.
 *
 *   Reviewed: efaiami 2010-06-22
 *
 *   Modify: efaiami 2011-02-22 for log and trace function
 *   Modify: xnikvap 2012-08-30 support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modify: xjonbuc 2012-09-06 Implement SDP1694 - support MAF SPI
 *   Modify: uabjoy  2014-03-24 Adding support for new Trace CC.
 *   Modify: xadaleg 2014-08-02 MR35347 - increase DN length
 *   Modify: xnikvap 2015-08-02 MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 ******************************************************************************/

#include "ComSA.h"
#include "OamSADataClasses.h"
#include "trace.h"

/**
 *	class OamSaDataClass
 */

OamSADataClass::OamSADataClass(): myFlags(0) , myImmValueType((SaImmValueTypeT) 0 ) {}

OamSADataClass::OamSADataClass(SaImmAttrFlagsT theFlags) : myFlags(theFlags), myImmValueType((SaImmValueTypeT) 0 )
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass::OamSADataClass(SaImmAttrFlagsT theFlags, SaImmValueTypeT theValueType) : myFlags(theFlags), myImmValueType(theValueType)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass::OamSADataClass(const OamSADataClass& dc)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dc.myFlags;
	myImmValueType = dc.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass::OamSADataClass(SaImmValueTypeT theValueType) : myFlags(0), myImmValueType(theValueType)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass::~OamSADataClass() {}

void OamSADataClass::SetFlags(SaImmAttrFlagsT theFlags)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = theFlags;
	LEAVE_OAMSA_TRANSLATIONS();
}

SaImmAttrFlagsT OamSADataClass::GetFlags()
{
	ENTER_OAMSA_TRANSLATIONS();
	return myFlags;
	LEAVE_OAMSA_TRANSLATIONS();
}

SaImmValueTypeT OamSADataClass::GetSaImmValueType()
{
	ENTER_OAMSA_TRANSLATIONS();
	return myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSADataClass::SetSaImmValueType(SaImmValueTypeT ImmType)
{
	ENTER_OAMSA_TRANSLATIONS();
	myImmValueType = ImmType;
	LEAVE_OAMSA_TRANSLATIONS();
}

/**
 *	class OamSAUInt8Data
 */

OamSAUInt8Data::~OamSAUInt8Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAUInt8Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(uint8_t);
}

OamSAUInt8Data::OamSAUInt8Data() : OamSADataClass(SA_IMM_ATTR_SAUINT32T)
{
}

OamSAUInt8Data::OamSAUInt8Data(const OamSAUInt8Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt8Data::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(uint8_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt8Data::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(uint8_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

MafOamSpiMoAttributeType_3T OamSAUInt8Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_UINT8;
}

void OamSAUInt8Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaUint32T*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt8Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaUint32T	SaVal = *(SaUint32T*) value_ptr;
	if (SaVal < 256)
		myValue = SaVal;
	else
		myValue = 255;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSAUInt8Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAUInt8Data(*this);
}

OamSAUInt8Data& OamSAUInt8Data::operator=(const OamSAUInt8Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSAUInt16Data
 */

OamSAUInt16Data::OamSAUInt16Data() : OamSADataClass(SA_IMM_ATTR_SAUINT32T)
{
ENTER_OAMSA_TRANSLATIONS();
LEAVE_OAMSA_TRANSLATIONS();
}

OamSAUInt16Data::OamSAUInt16Data(const OamSAUInt16Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAUInt16Data::~OamSAUInt16Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAUInt16Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	return sizeof(uint16_t);
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt16Data::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(uint16_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt16Data::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(uint16_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

MafOamSpiMoAttributeType_3T OamSAUInt16Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_UINT16;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt16Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaUint32T*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt16Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaUint32T	SaVal = *(SaUint32T*) value_ptr;
	if (SaVal < 65536)
		myValue = SaVal;
	else
		myValue = 65535;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSAUInt16Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAUInt16Data(*this);
}

OamSAUInt16Data& OamSAUInt16Data::operator=(const OamSAUInt16Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSAUInt32Data
 */

OamSAUInt32Data::OamSAUInt32Data() : OamSADataClass(SA_IMM_ATTR_SAUINT32T)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAUInt32Data::OamSAUInt32Data(const OamSAUInt32Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAUInt32Data::~OamSAUInt32Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAUInt32Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(uint32_t);
}

void OamSAUInt32Data::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(uint32_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt32Data::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(uint32_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

MafOamSpiMoAttributeType_3T OamSAUInt32Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_UINT32;
}

void OamSAUInt32Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaUint32T*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt32Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaUint32T	SaVal = *(SaUint32T*) value_ptr;
	myValue = SaVal;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSAUInt32Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAUInt32Data(*this);
}

OamSAUInt32Data& OamSAUInt32Data::operator=(const OamSAUInt32Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSAUInt64Data
 */

OamSAUInt64Data::OamSAUInt64Data() : OamSADataClass(SA_IMM_ATTR_SAUINT64T)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAUInt64Data::OamSAUInt64Data(const OamSAUInt64Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAUInt64Data::~OamSAUInt64Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAUInt64Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(uint64_t);
}

void OamSAUInt64Data::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(uint64_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt64Data::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(uint64_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

 MafOamSpiMoAttributeType_3T OamSAUInt64Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_UINT64;
}

 void OamSAUInt64Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaUint64T*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAUInt64Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaUint64T	SaVal = *(SaUint64T*) value_ptr;
	myValue = SaVal;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSAUInt64Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAUInt64Data(*this);
}

OamSAUInt64Data& OamSAUInt64Data::operator=(const OamSAUInt64Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSAInt8Data
 */

OamSAInt8Data::OamSAInt8Data() : OamSADataClass(SA_IMM_ATTR_SAINT32T)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

 OamSAInt8Data::OamSAInt8Data(const OamSAInt8Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAInt8Data::~OamSAInt8Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAInt8Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(int8_t);
}

 void OamSAInt8Data::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(int8_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAInt8Data::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(int8_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

 MafOamSpiMoAttributeType_3T OamSAInt8Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_INT8;
}

void OamSAInt8Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaInt32T*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAInt8Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaInt32T	SaVal = *(SaInt32T*) value_ptr;
	if (SaVal > 128)
		myValue = 128;
	else if (SaVal < -127)
		myValue = -127;
	else
		myValue = SaVal;
	LEAVE_OAMSA_TRANSLATIONS();
}


OamSADataClass* OamSAInt8Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAInt8Data(*this);
}

OamSAInt8Data& OamSAInt8Data::operator=(const OamSAInt8Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSAInt16Data
 */

OamSAInt16Data::OamSAInt16Data() : OamSADataClass(SA_IMM_ATTR_SAINT32T)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAInt16Data::OamSAInt16Data(const OamSAInt16Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAInt16Data::~OamSAInt16Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAInt16Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(int16_t);
}

void OamSAInt16Data::GetComOamValue(void *const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(int16_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAInt16Data::SetComOamValue(const void *const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(int16_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

 MafOamSpiMoAttributeType_3T OamSAInt16Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_INT16;
}

void OamSAInt16Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	*(SaInt32T*)value_ptr = myValue;
}

void OamSAInt16Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaInt32T	SaVal = *(SaInt32T*) value_ptr;
	if (SaVal > 32768)
		myValue = 32768;
	else if (SaVal < -32767)
		myValue = -32767;
	else
		myValue = SaVal;
	LEAVE_OAMSA_TRANSLATIONS();
}


OamSADataClass *OamSAInt16Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAInt16Data(*this);
}

OamSAInt16Data& OamSAInt16Data::operator=(const OamSAInt16Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSAInt32Data
 */

OamSAInt32Data::OamSAInt32Data() : OamSADataClass(SA_IMM_ATTR_SAINT32T)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAInt32Data::OamSAInt32Data(const OamSAInt32Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAInt32Data::~OamSAInt32Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAInt32Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(int32_t);
}

void OamSAInt32Data::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(int32_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAInt32Data::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(int32_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

 MafOamSpiMoAttributeType_3T OamSAInt32Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_INT32;
}

void OamSAInt32Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaInt32T*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAInt32Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaInt32T	SaVal = *(SaInt32T*) value_ptr;
	myValue = SaVal;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSAInt32Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAInt32Data(*this);
}

OamSAInt32Data& OamSAInt32Data::operator=(const OamSAInt32Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSAInt64Data
 */

OamSAInt64Data::OamSAInt64Data() : OamSADataClass(SA_IMM_ATTR_SAINT64T)
{
}

OamSAInt64Data::OamSAInt64Data(const OamSAInt64Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAInt64Data::~OamSAInt64Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAInt64Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(int64_t);
}

void OamSAInt64Data::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(int64_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAInt64Data::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(int64_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

 MafOamSpiMoAttributeType_3T OamSAInt64Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_INT64;
}

void OamSAInt64Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaInt64T*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAInt64Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaInt64T	SaVal = *(SaInt64T*) value_ptr;

	myValue = SaVal;
	LEAVE_OAMSA_TRANSLATIONS();
}


OamSADataClass* OamSAInt64Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAInt64Data(*this);
}

OamSAInt64Data& OamSAInt64Data::operator=(const OamSAInt64Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSADecimal64Data
 */

OamSADecimal64Data::OamSADecimal64Data() : OamSADataClass(SA_IMM_ATTR_SADOUBLET)
{
}

OamSADecimal64Data::OamSADecimal64Data(const OamSADecimal64Data& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADecimal64Data::~OamSADecimal64Data()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSADecimal64Data::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(int64_t);
}

void OamSADecimal64Data::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(double*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSADecimal64Data::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(double*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

 MafOamSpiMoAttributeType_3T OamSADecimal64Data::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_DECIMAL64;
}

void OamSADecimal64Data::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if(this->myImmValueType == SA_IMM_ATTR_SADOUBLET){
		*(SaDoubleT*)value_ptr = myValue;
	}
	else{
		*(SaFloatT*)value_ptr = myValue;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSADecimal64Data::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaDoubleT	SaVal = 0;
	if(this->myImmValueType == SA_IMM_ATTR_SADOUBLET){
		SaVal = *(SaDoubleT*) value_ptr;
	}
	else{
		SaVal = *(SaFloatT*) value_ptr;
	}

	myValue = SaVal;
	LEAVE_OAMSA_TRANSLATIONS();
}


OamSADataClass* OamSADecimal64Data::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSADecimal64Data(*this);
}

OamSADecimal64Data& OamSADecimal64Data::operator=(const OamSADecimal64Data& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}


/**
 *	class OamStringData
 */
OamSAStringData::OamSAStringData(const OamSAStringData& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (myValue_p != NULL)
	{
		delete myValue_p;
	}
	if (dt.myValue_p)
	{
		myValue_p = new char[strlen(dt.myValue_p)+1];
		if (myValue_p != NULL)
			strcpy(myValue_p, dt.myValue_p);
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAStringData::~OamSAStringData()
{
	ENTER_OAMSA_TRANSLATIONS();
	delete [] (char*)myValue_p;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAStringData::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (value_ptr != NULL)
	{
		if (myValue_p != NULL)
		{
			delete myValue_p;
		}
		myValue_p = new char[strlen((char*)value_ptr)+1];
		if (myValue_p != NULL)
		{
			strcpy(myValue_p, (char*)value_ptr);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAStringData::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (value_ptr != NULL)
	{
		if (myValue_p != NULL)
		{
			delete myValue_p;
		}
		myValue_p = new char[strlen((char*)value_ptr)+1];
		if (myValue_p != NULL)
		{
			strcpy(myValue_p, (char*)value_ptr);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAStringData& OamSAStringData::operator=(const OamSAStringData& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myImmValueType = dt.myImmValueType;
	if (myValue_p != NULL)
	{
		delete myValue_p;
	}
	if (dt.myValue_p != NULL)
	{
		myValue_p = new char[strlen(dt.myValue_p)+1];
		if (myValue_p != NULL)
		{
			strcpy(myValue_p, dt.myValue_p);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

OamSAStringData::OamSAStringData() : OamSADataClass(SA_IMM_ATTR_SASTRINGT), myValue_p(NULL)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAStringData::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if ((myValue_p != NULL) && (value_ptr != NULL))
		strcpy((char*)value_ptr, myValue_p);
	LEAVE_OAMSA_TRANSLATIONS();
}

MafOamSpiMoAttributeType_3T OamSAStringData::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_STRING;
}

void OamSAStringData::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if ((myValue_p != NULL) && (value_ptr != NULL))
	{

		if (SA_IMM_ATTR_SASTRINGT == myImmValueType)
		{
			strcpy((SaStringT)value_ptr, myValue_p);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSAStringData::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAStringData(*this);
}

unsigned int OamSAStringData::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	if (myValue_p != NULL) {
		LEAVE_OAMSA_TRANSLATIONS();
		return strlen(myValue_p);
	}
	else {
		LEAVE_OAMSA_TRANSLATIONS();
		return 0;
	}
}

/**
 *	class OamSAEnumData
 */
OamSAEnumData::OamSAEnumData() : OamSADataClass(SA_IMM_ATTR_SAINT32T)
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAEnumData::OamSAEnumData(const OamSAEnumData& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAEnumData::~OamSAEnumData()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSAEnumData::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(int64_t);
}

void OamSAEnumData::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(int64_t*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAEnumData::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(int64_t*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

 MafOamSpiMoAttributeType_3T OamSAEnumData::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_ENUM;
}

void OamSAEnumData::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaInt32T*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAEnumData::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaInt32T	SaVal = *(SaInt32T*) value_ptr;
	if (SaVal > 32768)
		myValue = 32768;
	else if (SaVal < -32767)
		myValue = -32767;
	else
		myValue = SaVal;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSAEnumData::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAEnumData(*this);
}

OamSAEnumData& OamSAEnumData::operator=(const OamSAEnumData& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	myImmValueType = dt.myImmValueType;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

/**
 *	class OamSABoolData
 */

OamSABoolData::OamSABoolData() : OamSADataClass(SA_IMM_ATTR_SAUINT32T)
{
}

OamSABoolData::OamSABoolData(const OamSABoolData& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSABoolData::~OamSABoolData()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

unsigned int OamSABoolData::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return sizeof(uint8_t);
}

void OamSABoolData::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(bool*)value_ptr = myValue;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSABoolData::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	myValue = *(bool*)value_ptr;
	LEAVE_OAMSA_TRANSLATIONS();
}

 MafOamSpiMoAttributeType_3T OamSABoolData::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_BOOL;
}

void OamSABoolData::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	*(SaUint32T*)value_ptr = myValue ? 1 : 0;
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSABoolData::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	SaUint32T	SaVal = *(SaUint32T*) value_ptr;
	myValue = (SaVal != 0);
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSABoolData::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSABoolData(*this);
}

OamSABoolData& OamSABoolData::operator=(const OamSABoolData& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myValue = dt.myValue;
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}


 /**
 *	Class Reference
 */
OamSAReferenceData::OamSAReferenceData(const OamSAReferenceData& dt) : OamSADataClass(dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (myValue_p != NULL)
	{
		delete myValue_p;
	}
	if (dt.myValue_p)
	{
		myValue_p = new char[strlen(dt.myValue_p)+1];
		if (myValue_p != NULL)
		{
			strcpy(myValue_p, dt.myValue_p);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAReferenceData::~OamSAReferenceData()
{
	ENTER_OAMSA_TRANSLATIONS();
	delete [] (char*)myValue_p;
	LEAVE_OAMSA_TRANSLATIONS();
}


void OamSAReferenceData::SetComOamValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (value_ptr != NULL)
	{
		if (myValue_p != NULL)
		{
			delete myValue_p;
		}
		myValue_p = new char[strlen((char*)value_ptr)+1];
		if (myValue_p != NULL)
		{
			strcpy(myValue_p, (char*)value_ptr);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSAReferenceData::SetImmValue(const void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (value_ptr != NULL)
	{
		if (myValue_p != NULL)
		{
			delete myValue_p;
		}
		myValue_p = new char[strlen((char*)value_ptr)+1];
		if (myValue_p != NULL)
		{
			strcpy(myValue_p, (char*)value_ptr);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAReferenceData& OamSAReferenceData::operator=(const OamSAReferenceData& dt)
{
	ENTER_OAMSA_TRANSLATIONS();
	myFlags = dt.myFlags;
	myImmValueType = dt.myImmValueType;
	if (myValue_p != NULL)
	{
		delete myValue_p;
	}
	if (dt.myValue_p != NULL)
	{
		myValue_p = new char[strlen(dt.myValue_p)+1];
		if (myValue_p != NULL)
		{
			strcpy(myValue_p, dt.myValue_p);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

OamSAReferenceData::OamSAReferenceData() : OamSADataClass(SA_IMM_ATTR_SANAMET), myValue_p(NULL)
{
}

void OamSAReferenceData::GetComOamValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if ((myValue_p != NULL) && (value_ptr != NULL))
		strcpy((char*)value_ptr, myValue_p);
	LEAVE_OAMSA_TRANSLATIONS();
}

MafOamSpiMoAttributeType_3T OamSAReferenceData::GetMafOamSpiMoAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafOamSpiMoAttributeType_3_REFERENCE;
}


void OamSAReferenceData::GetImmValue(void* const value_ptr)
{
	ENTER_OAMSA_TRANSLATIONS();
	if ((myValue_p != NULL) && (value_ptr != NULL))
	{
		// Start with translating int imm format
		char *immstr;
		theTranslator.MO2Imm_DN(myValue_p, &immstr);

		if (SA_IMM_ATTR_SANAMET == myImmValueType)
		{
			SaNameT* nap = (SaNameT*) value_ptr;
			saNameSet(immstr, nap);
		}
		delete [] immstr;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataClass* OamSAReferenceData::Duplicate()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return new OamSAReferenceData(*this);
}

unsigned int OamSAReferenceData::Size()
{
	ENTER_OAMSA_TRANSLATIONS();
	if (myValue_p != NULL) {
		LEAVE_OAMSA_TRANSLATIONS();
		return strlen(myValue_p);
	}
	else {
		LEAVE_OAMSA_TRANSLATIONS();
		return 0;
	}
}

/**
 *	OamSADataContainer
 */
OamSADataContainer::OamSADataContainer(OamSADataClass*	theDP)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafAttributeType	= theDP->GetMafOamSpiMoAttributeType();
	ImmValueType		= theDP->GetSaImmValueType();
	myDataPointerList.push_front(theDP);
	hasdefaultvalue     = false;
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataContainer::OamSADataContainer(const OamSADataContainer& dc)
{
	ENTER_OAMSA_TRANSLATIONS();
	DataPointerListIterator	dpli;
	DataPointerList*		dpli_p;
	myDataPointerList.clear();

	ImmValueType		= dc.ImmValueType;
	hasdefaultvalue     = dc.hasdefaultvalue;

	/* This rather complicated construct because of funny things with the compiler and templates */
	dpli_p = (DataPointerList*)&dc.myDataPointerList;
	dpli = dpli_p->begin();
	for (; dpli != dc.myDataPointerList.end(); dpli++)
	{
		myDataPointerList.push_back((*dpli)->Duplicate());
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataContainer& OamSADataContainer::operator=(const OamSADataContainer& dc)
{
	ENTER_OAMSA_TRANSLATIONS();
	DataPointerListIterator	dpli;
	DataPointerList*		dpli_p;
	myDataPointerList.clear();

	ImmValueType		= dc.ImmValueType;
	hasdefaultvalue     = dc.hasdefaultvalue;

	/* This rather complicated construct because of funny things with the compiler and templates */

	dpli_p = (DataPointerList*)&dc.myDataPointerList;
	dpli = dpli_p->begin();

	for (; dpli != dc.myDataPointerList.end(); dpli++)
	{
		myDataPointerList.push_back((*dpli)->Duplicate());
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return *this;
}

bool OamSADataContainer::GetFirstData(DataPointerListIterator& di)
{
	ENTER_OAMSA_TRANSLATIONS();
	di = myDataPointerList.begin();
	LEAVE_OAMSA_TRANSLATIONS();
	return di != myDataPointerList.end();
}

bool OamSADataContainer::GetNextData(DataPointerListIterator& di) const
{
	ENTER_OAMSA_TRANSLATIONS();
	di++;
	LEAVE_OAMSA_TRANSLATIONS();
	return di != myDataPointerList.end();
}

/**
 *	Translates data from a com value to something to put in the cache
 */
OamSADataClass* OamSADataFactory::CreateOamSAData(const MafOamSpiMoAttributeType_3 type,
												  const MafMoAttributeValue_3T &inputvalue)

{
	ENTER_OAMSA_TRANSLATIONS();
	OamSADataClass*	dc_p = NULL;

	/* Depending on what we get in , we will create something to put in the structures */
	switch (type)
	{
	case MafOamSpiMoAttributeType_3_INT8:
		dc_p = new OamSAInt8Data;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.i8);
		break;

	case MafOamSpiMoAttributeType_3_INT16:
		dc_p = new OamSAInt16Data;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.i16);
		break;

	case MafOamSpiMoAttributeType_3_INT32:
		dc_p = new OamSAInt32Data;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.i32);
		break;

	case MafOamSpiMoAttributeType_3_INT64:
		dc_p = new OamSAInt64Data;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.i64);
		break;

	case MafOamSpiMoAttributeType_3_UINT8:
		dc_p = new OamSAUInt8Data;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.u8);
		break;

	case MafOamSpiMoAttributeType_3_UINT16:
		dc_p = new OamSAUInt16Data;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.u16);
		break;

	case MafOamSpiMoAttributeType_3_UINT32:
		dc_p = new OamSAUInt32Data;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.u32);
		break;

	case MafOamSpiMoAttributeType_3_UINT64:
		dc_p = new OamSAUInt64Data;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.u64);
		break;

	case MafOamSpiMoAttributeType_3_STRING:
		dc_p = new OamSAStringData;
		if (dc_p != NULL)
			dc_p->SetComOamValue(inputvalue.value.theString);
		break;
	case MafOamSpiMoAttributeType_3_DECIMAL64:
			dc_p = new OamSADecimal64Data;
			if (dc_p != NULL)
				dc_p->SetComOamValue(&inputvalue.value.decimal64);
			break;

	case MafOamSpiMoAttributeType_3_BOOL:
		dc_p = new OamSABoolData;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.theBool);
		break;

	case MafOamSpiMoAttributeType_3_REFERENCE:
		dc_p = new OamSAReferenceData;
		if (dc_p != NULL)
			dc_p->SetComOamValue(inputvalue.value.moRef);
		break;

	case MafOamSpiMoAttributeType_3_ENUM:
		dc_p = new OamSAEnumData;
		if (dc_p != NULL)
			dc_p->SetComOamValue(&inputvalue.value.theEnum);
		break;

	case MafOamSpiMoAttributeType_3_STRUCT:
		// TBD
		break;

	default:
	  ERR_OAMSA_TRANSLATIONS("ERROR -- unknown MafOamSpiMoAttributeType_3 passed to OamSADataContainer::operator=");
	  break;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return dc_p;
}

OamSADataClass* OamSADataFactory::CreateOamSAData(const MafOamSpiMoAttributeType_3 type,
												  const MafMoAttributeValue_3T &inputvalue,
												  SaImmValueTypeT ImmValueType)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSADataClass* dc = OamSADataFactory::CreateOamSAData(type, inputvalue);
	if (dc != NULL)
	{
		dc->SetSaImmValueType(ImmValueType);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return dc;
}

/**
 *	CreateMafMoAttrValCont - translates data from the cache into a MafMoAttributeValueContainer_3T
 */
MafMoAttributeValueContainer_3T* OamSADataFactory::CreateMafMoAttrValCont(OamSADataContainer& dac)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafMoAttributeValueContainer_3T* AttrValCont_p = new MafMoAttributeValueContainer_3T;
	if (AttrValCont_p != NULL)
	{
		AttrValCont_p->nrOfValues = dac.NoOfDataElements();
		if (AttrValCont_p->nrOfValues > 0)
		{
			AttrValCont_p->values = new MafMoAttributeValue_3[AttrValCont_p->nrOfValues];
			if (AttrValCont_p->values != NULL)
			{
				OamSADataContainer::DataPointerListIterator	dpi;
				int i = 0;
				for (bool NotDone = dac.GetFirstData(dpi) ; NotDone; NotDone = dac.GetNextData(dpi), i++)
				{
					OamSADataClass* DataClass_p = *dpi;
					AttrValCont_p->type = DataClass_p->GetMafOamSpiMoAttributeType();
					switch (AttrValCont_p->type)
					{
					case MafOamSpiMoAttributeType_3_INT8:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.i8);
						break;
					case MafOamSpiMoAttributeType_3_INT16:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.i16);
						break;
					case MafOamSpiMoAttributeType_3_INT32:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.i32);
						break;
					case MafOamSpiMoAttributeType_3_INT64:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.i64);
						break;
					case MafOamSpiMoAttributeType_3_UINT8:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.u8);
						break;
					case MafOamSpiMoAttributeType_3_UINT16:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.u16);
						break;
					case MafOamSpiMoAttributeType_3_UINT32:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.u32);
						break;
					case MafOamSpiMoAttributeType_3_UINT64:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.u64);
						break;
					case MafOamSpiMoAttributeType_3_DECIMAL64:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.decimal64);
						break;
					case MafOamSpiMoAttributeType_3_STRING:
						AttrValCont_p->values[i].value.theString = new char[DataClass_p->Size()+1];
						if (AttrValCont_p->values[i].value.theString != NULL)
							DataClass_p->GetComOamValue(const_cast<char *>(AttrValCont_p->values[i].value.theString));
						else
						{
							delete AttrValCont_p->values;
							delete AttrValCont_p;
							AttrValCont_p = NULL;
							goto end_exit;
						}
						break;

					case MafOamSpiMoAttributeType_3_BOOL:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.theBool);
						break;
					case MafOamSpiMoAttributeType_3_REFERENCE:
						AttrValCont_p->values[i].value.moRef = new char[DataClass_p->Size()+1];
						if (AttrValCont_p->values[i].value.moRef != NULL)
						  DataClass_p->GetComOamValue(const_cast<char *>(AttrValCont_p->values[i].value.moRef));
						else
						{
							delete AttrValCont_p->values;
							delete AttrValCont_p;
							AttrValCont_p = NULL;
							goto end_exit;
						}
						break;

					case MafOamSpiMoAttributeType_3_ENUM:
						DataClass_p->GetComOamValue(&AttrValCont_p->values[i].value.theEnum);
						break;

					case MafOamSpiMoAttributeType_3_STRUCT:
						break;

					default:
						ERR_OAMSA_TRANSLATIONS(" ERROR -- unknown MafOamSpiMoAttributeType_3 ");
						break;
					}
				}
			}
			else
			{
				delete AttrValCont_p;
				AttrValCont_p = NULL;
			}
		}
		else
		{
			AttrValCont_p->values = NULL;
			AttrValCont_p->type = dac.GetMafAttributeType();
		}
	}
end_exit:
	LEAVE_OAMSA_TRANSLATIONS();
	return AttrValCont_p;
}

/**
 *	OamSADataContainer
 */


OamSADataContainer::OamSADataContainer()
{
	MafAttributeType = (MafOamSpiMoAttributeType_3T)0;
	ImmValueType =(SaImmValueTypeT)0;
	hasdefaultvalue = false;
}

OamSADataContainer::~OamSADataContainer()
{
	ENTER_OAMSA_TRANSLATIONS();
	// FIX need to delete allocated object pointers
	for(std::list<OamSADataClass*>::iterator it=myDataPointerList.begin(); it != myDataPointerList.end(); it++){
		delete *it;
	}
	myDataPointerList.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSADataContainer::Add(OamSADataClass* dcp)
{
	ENTER_OAMSA_TRANSLATIONS();
	myDataPointerList.push_back(dcp);
	LEAVE_OAMSA_TRANSLATIONS();
}

void OamSADataContainer::Clear() {
	ENTER_OAMSA_TRANSLATIONS();
	// FIX need to delete allocated object pointers
	for(std::list<OamSADataClass*>::iterator it=myDataPointerList.begin(); it != myDataPointerList.end(); it++){
		delete *it;
	}
	myDataPointerList.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}
unsigned int OamSADataContainer::NoOfDataElements() const
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return myDataPointerList.size();
}

bool OamSADataContainer::Gethasdefaultvalue()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return hasdefaultvalue;
}

void OamSADataContainer::SetAttributeTypes(MafOamSpiMoAttributeType_3T type, SaImmValueTypeT theValueType,bool defaultvalue)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafAttributeType = type;
	ImmValueType = theValueType;
	hasdefaultvalue = defaultvalue;
	LEAVE_OAMSA_TRANSLATIONS();
}

MafOamSpiMoAttributeType_3T OamSADataContainer::GetMafAttributeType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return MafAttributeType;
}

SaImmValueTypeT OamSADataContainer::GetImmValueType()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return ImmValueType;
}

/**
 *	OamSADataFactory
 */

OamSADataFactory::OamSADataFactory()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSADataFactory::~OamSADataFactory()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

