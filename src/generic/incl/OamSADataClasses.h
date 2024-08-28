#ifndef __OAMSA_DATACLASSES_H
#define __OAMSA_DATACLASSES_H
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
 *   This file declares Data classes for the cache.
 *
 *   Reviewed: efaiami 2010-06-22
 *
 *   Modify: efaiami 2011-02-22  for log and trace function
 *   Modify: xnikvap, xngangu 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify: xanhdao 2013-10-15 Implement MR24146 for supporting floating point
 *   Modify: uabjoy  2014-03-24 Adding support for Trace CC.
 *   Modify: xadaleg 2014-08-02 MR35347 - increase DN length
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *************************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <list>
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiManagedObject_3.h"
#include "saImm.h"
#include "trace.h"
#include "OamSATranslator.h"

extern OamSATranslator	theTranslator;
/**
 *  Base class OamSADataClass
 */
class OamSADataClass
{
public:
	OamSADataClass();
	OamSADataClass(SaImmAttrFlagsT theFlags);
	OamSADataClass(SaImmAttrFlagsT theFlags, SaImmValueTypeT theValueType);
	OamSADataClass(const OamSADataClass& dc);
	OamSADataClass(SaImmValueTypeT theValueType);

	virtual void GetComOamValue(void* const value_ptr) = 0;
	virtual void SetComOamValue(const void* const value_ptr) = 0;
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType()= 0;
	virtual void GetImmValue(void* const value_ptr) = 0;
	virtual void SetImmValue(const void* const value_ptr) = 0;
	virtual OamSADataClass *Duplicate() = 0;
	virtual ~OamSADataClass();
	virtual unsigned int Size() = 0;

	void SetFlags(SaImmAttrFlagsT theFlags);
	SaImmAttrFlagsT GetFlags();
	SaImmValueTypeT GetSaImmValueType();
	void SetSaImmValueType(SaImmValueTypeT ImmType);

protected:
	SaImmAttrFlagsT myFlags;
	SaImmValueTypeT myImmValueType;

};

/**
 * class OamSAUInt8Data
 */
class OamSAUInt8Data : public  OamSADataClass
{
public:
	OamSAUInt8Data();
	OamSAUInt8Data(const OamSAUInt8Data& dt);

 	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAUInt8Data& operator=(const OamSAUInt8Data& dt);
	virtual ~OamSAUInt8Data();
	virtual OamSADataClass *Duplicate();
	virtual unsigned int Size();

private:
	uint8_t	myValue;
};


/**
 *  class OamSAUInt16Data
 */
class OamSAUInt16Data : public  OamSADataClass
{
public:
	OamSAUInt16Data();
	OamSAUInt16Data(const OamSAUInt16Data& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAUInt16Data& operator=(const OamSAUInt16Data& dt);
	virtual ~OamSAUInt16Data();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	uint16_t	myValue;
};

/**
 *  class OamSAUInt32Data
 */
class OamSAUInt32Data : public  OamSADataClass
{
public:
	OamSAUInt32Data();
	OamSAUInt32Data(const OamSAUInt32Data& dt);

 	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAUInt32Data& operator=(const OamSAUInt32Data& dt);
	virtual ~OamSAUInt32Data();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	uint32_t	myValue;
};

/**
 *  class OamSAUInt64Data
 */
class OamSAUInt64Data : public  OamSADataClass
{
public:
	OamSAUInt64Data();
	OamSAUInt64Data(const OamSAUInt64Data& dt);
 	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAUInt64Data& operator=(const OamSAUInt64Data& dt);
	virtual ~OamSAUInt64Data();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	uint64_t	myValue;
};

/**
 *  class OamSAInt8Data
 */
class OamSAInt8Data : public  OamSADataClass
{
public:
	OamSAInt8Data();
	OamSAInt8Data(const OamSAInt8Data& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAInt8Data& operator=(const OamSAInt8Data& dt);
	virtual ~OamSAInt8Data();
	virtual OamSADataClass*	Duplicate();
	virtual unsigned int Size();

private:
	int8_t	myValue;
};

/**
 *  class OamSAInt16Data
 */
class OamSAInt16Data : public  OamSADataClass
{
public:
	OamSAInt16Data();
	OamSAInt16Data(const OamSAInt16Data& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAInt16Data& operator=(const OamSAInt16Data& dt);
	virtual ~OamSAInt16Data();
	virtual OamSADataClass*	Duplicate();
	virtual unsigned int Size();

private:
	int16_t	myValue;
};

/**
 *  class OamSAInt32Data
 */
class OamSAInt32Data : public  OamSADataClass
{
public:
	OamSAInt32Data();
	OamSAInt32Data(const OamSAInt32Data& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAInt32Data& operator=(const OamSAInt32Data& dt);
	virtual ~OamSAInt32Data();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	int32_t	myValue;
};


/**
 *  class OamSAInt64Data
 */
class OamSAInt64Data : public  OamSADataClass
{
public:
	OamSAInt64Data();
	OamSAInt64Data(const OamSAInt64Data& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAInt64Data& operator=(const OamSAInt64Data& dt);
	virtual ~OamSAInt64Data();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	int64_t	myValue;
};

/**
 *  class OamSADecimal64Data
 */
class OamSADecimal64Data : public  OamSADataClass
{
public:
	OamSADecimal64Data();
	OamSADecimal64Data(const OamSADecimal64Data& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSADecimal64Data& operator=(const OamSADecimal64Data& dt);
	virtual ~OamSADecimal64Data();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	double	myValue;
};



/**
 *  class OamSAStringData
 */
class OamSAStringData : public  OamSADataClass
{
public:
	OamSAStringData();
	OamSAStringData(const OamSAStringData& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAStringData& operator=(const OamSAStringData& dt);
	virtual ~OamSAStringData();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	char*	myValue_p;
};


/**
 *  class OamSAReferenceData
 */
class OamSAReferenceData : public  OamSADataClass
{
public:
	OamSAReferenceData();
	OamSAReferenceData(const OamSAReferenceData& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAReferenceData& operator=(const OamSAReferenceData& dt);
	virtual ~OamSAReferenceData();
	virtual OamSADataClass*	Duplicate();
	virtual unsigned int Size();

private:
	char*	myValue_p;
};


/**
 *  class OamSAEnumData
 */
class OamSAEnumData : public  OamSADataClass
{
public:
	OamSAEnumData();
	OamSAEnumData(const OamSAEnumData& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSAEnumData& operator=(const OamSAEnumData& dt);
	virtual ~OamSAEnumData();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	int64_t	myValue;
};


/**
 *  class OamSABoolData
 */
class OamSABoolData : public  OamSADataClass
{
public:
	OamSABoolData();
	OamSABoolData(const OamSABoolData& dt);
	virtual void GetComOamValue(void* const value_ptr);
	virtual void SetComOamValue(const void* const value_ptr);
	virtual MafOamSpiMoAttributeType_3T GetMafOamSpiMoAttributeType();
	virtual void GetImmValue(void* const value_ptr);
	virtual void SetImmValue(const void* const value_ptr);
	OamSABoolData& operator=(const OamSABoolData& dt);
	virtual ~OamSABoolData();
	virtual OamSADataClass* Duplicate();
	virtual unsigned int Size();

private:
	bool	myValue;
};


/**
 *  class OamSADataCointainer
 */

class OamSADataContainer
{
public:
	typedef std::list<OamSADataClass*>::iterator	DataPointerListIterator;
	OamSADataContainer();
	OamSADataContainer(OamSADataClass*	theDP);
	OamSADataContainer(const OamSADataContainer& dc);
	~OamSADataContainer();
	OamSADataContainer& operator=(const OamSADataContainer& oc);
	void Add(OamSADataClass* dcp);
	void Clear();
	unsigned int NoOfDataElements() const;
	bool	GetFirstData(DataPointerListIterator& di);
	bool	GetNextData(DataPointerListIterator& di) const;
	void 	SetAttributeTypes(MafOamSpiMoAttributeType_3T type,SaImmValueTypeT theValueType,bool defaultvalue);
	MafOamSpiMoAttributeType_3T GetMafAttributeType();
	SaImmValueTypeT GetImmValueType();
	bool Gethasdefaultvalue();

private:
	MafOamSpiMoAttributeType_3T				MafAttributeType;
	SaImmValueTypeT 						ImmValueType;
	typedef std::list<OamSADataClass*>		DataPointerList;
	DataPointerList	myDataPointerList;
	bool hasdefaultvalue;
};

/**
 * 	Class OamSADataFactory - a place to go to when you want to make some data into something else
 */
class OamSADataFactory
{
public:
	OamSADataFactory();
	~OamSADataFactory();
	OamSADataClass* CreateOamSAData(const MafOamSpiMoAttributeType_3 type,
									const MafMoAttributeValue_3T& inputvalue);

	OamSADataClass* CreateOamSAData(const MafOamSpiMoAttributeType_3 type,
									const MafMoAttributeValue_3T& inputvalue,
									SaImmValueTypeT ImmValueType);

	MafMoAttributeValueContainer_3T*  CreateMafMoAttrValCont(OamSADataContainer&);

};
#endif
