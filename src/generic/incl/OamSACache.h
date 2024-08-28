#ifndef __OAMSA_CACHE_H
#define __OAMSA_CACHE_H
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
 *
 *   File:   OamSACache.h
 *
 *   Author: egorped
 *
 *   Date:   2010-05-21
 *
 *   This file declares functions for Cache
 *
 *   Reviewed: efaiami 2010-06-29
 *
 *   Modify: efaiami 2011-02-22 for log and trace function
 *   Modify: xnikvap, xngangu, xjonbuc, xduncao 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *
 *   Modify: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modify: xadaleg 2014-08-02  MR35347 - increase DN length
 *   Modify: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *********************************************************************************/

#include <memory.h>
#include <string.h>
#include <map>
#include <list>
#include <vector>
#include <string>
#include "MafOamSpiManagedObject_2.h"
#include "MafOamSpiManagedObject_3.h"
#include "saAis.h"
#include "saImm.h"
#include "saImmOm.h"
#include "trace.h"
#include "InternalTrace.h"
#include "imm_utils.h"

/**
 *  Forward class declarations
 */
class OamSADeletedObjectsList;
class OamSAModifiedObjectsList;
class OamSAModifiedEntry;
class OamSADeletedEntry;
class BridgeImmIterator;
class OamSACache;


/**
 *  Classes used when fetching data to apply to IMM.
 *
 *  OamSACacheCreatedObjectData - contains data for a newly created object. OamSACacheCreatedObjectsList contains these
 * 								  This object is NOT used internally in the cache
 */
class OamSACacheCreatedObjectData
{
public:
	// Constructors
	OamSACacheCreatedObjectData();
	OamSACacheCreatedObjectData(const std::string& ObjName, OamSAModifiedEntry& mod, OamSACache& theCache);
	OamSACacheCreatedObjectData(const OamSACacheCreatedObjectData& oc);

	// Destructor
	~OamSACacheCreatedObjectData();

	// Operators
	OamSACacheCreatedObjectData &operator=(const OamSACacheCreatedObjectData& oc);

	// Public methods
	void Dump();
	SaImmClassNameT        ClassName();
	SaNameT*               ParentName();
	SaImmAttrValuesT_2**   AttrValues();
	std::string            ImmName();
	std::string            getParent3GPPDN();
	typedef std::map<std::string, bool> defaultvaluesMap;
	typedef std::map <std::string, bool>::iterator defaultvaluesMapIterator;
	defaultvaluesMap&      getdefaultvaluesMap(){return defaultvaluesMapInfo;}

	void setAttrValues(SaImmAttrValuesT_2** ret_attrValues_pp);

private:
	SaImmClassNameT        className;
	SaNameT                parentName;
	std::string            immName;
	std::string            parent3GPPDN;
	SaImmAttrValuesT_2**   attrValues_pp;
	defaultvaluesMap       defaultvaluesMapInfo;
};

/**
 *  OamSACacheCreatedObjectsList - contains data for all newly created objects.
 * 								   This kind of list is supplied to the caller when calling
 * 								   OamSACache::GetListOfCreatedObjects(). This list should be deallocated after use
 * 								   by calling delete on the listpointer.
 */
class OamSACacheCreatedObjectsList
{
public:
	// Constructors
	OamSACacheCreatedObjectsList();
	OamSACacheCreatedObjectsList(unsigned int InitialVectorSize);

	// Destructor
	~OamSACacheCreatedObjectsList();

	// Operators
	OamSACacheCreatedObjectData& operator[](unsigned int idx);

	// Public methods
	unsigned int NoOfObjects();
	void 		 Dump();
	void 		 Insert(OamSACacheCreatedObjectData& oc);
	std::vector<OamSACacheCreatedObjectData>::iterator getMyObjectsIter ();

private:
	std::vector<OamSACacheCreatedObjectData> myObjects;
};

/**
 * OamSACacheDeletedObjectData - data for an object to delete from IMM. OamSACacheDeletedObjectsList contains these
 * 								  This object is NOT used internally in the cache
 */
class OamSACacheDeletedObjectData
{
public:
	// Constructors
	OamSACacheDeletedObjectData();
	OamSACacheDeletedObjectData(const std::string& theString);
	OamSACacheDeletedObjectData(const char* str);
	OamSACacheDeletedObjectData(const OamSACacheDeletedObjectData& oc);

	// Destructor
	~OamSACacheDeletedObjectData();

	// Operators
	OamSACacheDeletedObjectData& operator=(const OamSACacheDeletedObjectData& oc);

	// Public Methods
	SaNameT*				ObjectName();

private:
	SaNameT					objectName;
};


/**
 *  OamSACacheDeletedObjectsList - contains data for all objects to be deleted from IMM
 *
 * 								  This kind of list is supplied to the caller when calling
 * 								  OamSACache::GetListOfDeletedObjects(). This list should be deallocated after use
 * 								  by calling delete on the listpointer.
 */
class OamSACacheDeletedObjectsList
{
public:
	// Constructors
	OamSACacheDeletedObjectsList();
	OamSACacheDeletedObjectsList(unsigned int InitialVectorSize);

	// Destructor
	~OamSACacheDeletedObjectsList();

	// Operators
	OamSACacheDeletedObjectData& operator[](unsigned int idx);

	// Public Methods
	void 			Insert(OamSACacheDeletedObjectData& oc);
	unsigned int 	NoOfObjects();

private:
	std::vector<OamSACacheDeletedObjectData> myObjects;
};

/**
 *  OamSACacheModifiedObjectData - contains data for a data modification for an object in IMM.
 * 								   OamSACacheModofiedObjectsList contains these
 * 								   This object is NOT used internally in the cache
 */
class OamSACacheModifiedObjectData
{
public:
	// Constructors
	OamSACacheModifiedObjectData();
	OamSACacheModifiedObjectData(const OamSACacheModifiedObjectData& od);
	OamSACacheModifiedObjectData(const std::string& ObjName, OamSAModifiedEntry& mod);
	OamSACacheModifiedObjectData(const SaNameT& ObjName, SaImmAttrModificationT_2** attrValues_pp);

	// Destructor
	~OamSACacheModifiedObjectData();

	// Operators
	OamSACacheModifiedObjectData& operator=(const OamSACacheModifiedObjectData& od);

	// Public Methods
	SaNameT*						ObjectName();
	SaImmAttrModificationT_2**		AttrValues();

private:
	SaNameT						objectName;
	SaImmAttrModificationT_2**	attrValues_pp;
};

/**
 * OamSACacheModifiedObjectsList - contains all modifications for all objects.
 *
 * 								  This kind of list is supplied to the caller when calling
 * 								  OamSACache::GetListOfModifiedObjects(). This list should be deallocated after use
 * 								  by calling delete on the listpointer.
 */
class OamSACacheModifiedObjectsList
{
public:
	// Constructors
	OamSACacheModifiedObjectsList();
	OamSACacheModifiedObjectsList(unsigned int InitialVectorSize);

	// Destructor
	~OamSACacheModifiedObjectsList();

	// Operators
	OamSACacheModifiedObjectData& operator[](unsigned int idx);

	// Public Methods
	void 			Insert(OamSACacheModifiedObjectData& oc);
	unsigned int 	NoOfObjects();
	std::vector<OamSACacheModifiedObjectData>::iterator getMyObjectsIter ();
	void clearMyObjects();

private:
	std::vector<OamSACacheModifiedObjectData> myObjects;
};

/**
 *  OamSACache
 *
 * 	This class contains all data for a transaction until prepare when data is fetched and written to the CCB. Reads will
 *  also pass here since modifications in a transaction must be reflected in these for reads in that context, but not in
 *  other transactions until that is successfully written to IMM.
 *  There is one cache class instance for every transaction
 *
 */
class OamSACache
{
public:
	// Type definitions
 	typedef enum {eObjectSuccess  = 0,
				  eObjectDeleted  = 1,
				  eObjectNotFound = 2,
				  eObjectAlreadyExists = 3} ObjectState;
	typedef std::list<std::string>						DNList;				// These types are used for the object names when it's split into
	typedef std::list<std::string>						MocPathList;		// These types are used for the MO class path when it's split into
	typedef std::list<std::string>::iterator 			DNListIterator;		// sub parts
	typedef std::list<std::string>::reverse_iterator 	DNListReverseIterator;

	// Constructors
	OamSACache();

	// Destructors
	~OamSACache();

	// Public methods
	// InsertCreatedObject. Inserts a new object in the Modified objects list (this is used for both new objects and modifications
	// to existing objects.
	ObjectState InsertCreatedObject(const std::string& theObjectName,
									const std::string& theObjectClass,
									const std::string& theParentName,
									const std::string& theImmName,
									const char*		   theAttributeName,
									const char*		   theAttriburteValue );

	ObjectState InsertCreatedObject(DNList& theObjectList,
									const std::string& theObjectClass,
									const std::string& theParentName,
									const std::string& theImmName,
									const char*		   theAttributeName,
									const char*		   theAttriburteValue );

	// RemoveCreatedObject. Removes a new object from the modified objects list. This could happen if the same object is
	// specified as created and then deleted in the same transaction. A bit useless but not forbidden.
	bool 	RemoveCreatedObject(DNList& theDNList);
	bool	RemoveCreatedObject(const std::string& ObjectDN);

	// InsertDeletedObject. Inserts a deleted object in the list of deleted objects.
	ObjectState InsertDeletedObject(const std::string& theObjectName,const std::string&  theImmName);
	ObjectState InsertDeletedObject(DNList& theObjectList,const std::string&  theImmName);

	// RemoveDeletedObject. Removes an object from the deleted list. This might happen if someone first deletes an objects
	// and then creates it again in the same transaction. As useless as first creating and then deleting, but as valid.
	void	RemoveDeletedObject(std::list<std::string>&	theDisassembledDN);
	void	RemoveDeletedObject(const std::string& ObjectDN);

	// IsDeleted. Checks if the object is present in the deleted objects list
	bool		IsDeleted(DNList& theDisassembledDN);
	bool		IsDeleted(const std::string& ObjectDN);

	// IsInCache Reports if an object exists in cache

	bool 		IsInCache(DNList& theDisassembledDN);

	// UpdateObjectAttribute. Adds an object modification to the modified objects list.
	ObjectState UpdateObjectAttribute(const std::string& theObjectName,
									  const std::string& theImmName,
									  const std::string& theAttributeName,
									  const MafMoAttributeValueContainer_3T * attributeValue);

	ObjectState UpdateObjectAttribute(DNList& theObjectList,
									  const std::string& theImmName,
									  const std::string& theAttributeName,
									  const MafMoAttributeValueContainer_3T * attributeValue);

	// GetAttribute. Reads an attribute from the modified objects list. Return not found if not in the cache, that is a valid condition
	// it might be in the IMM anyway.
	ObjectState GetAttribute(const std::string& theObjectName,  const std::string& theAttributeName, MafMoAttributeValueContainer_3T** attributeValue);
	ObjectState GetAttribute(DNList& theObjectList,  const std::string& theAttributeName, MafMoAttributeValueContainer_3T** attributeValue);

	// GetNextChildObject. Returns the next sibling if it's exists in the cache. Used when transversing the object tree using
	// nextMo
	std::string GetNextChildObject(DNList& theObjectList, const std::string& theObjectClass, const std::string& thePreviousSibling);

	// These methods below supplies data to calls to the corresponding calls to IMM
	OamSACacheCreatedObjectsList*		GetListOfCreatedObjects();
	OamSACacheDeletedObjectsList*		GetListOfDeletedObjects();
	OamSACacheModifiedObjectsList		GetListOfModifiedObjects();

	//This method extracts the instances of "deleting of default-valued attributes during the MOC creation"
	//from Created Objects List and appends them to Modified Objects List. (TR HU83697)
	void prepareListOfModifiedObjects(OamSACacheCreatedObjectsList& creObj);

	//A helper method to retrieve the Attribute value(trimming everything before '=')
	bool fetchAttrValue(SaImmAttrValuesT_2& AttrValues, std::string& keyAttrValue);

	//returns the finalModifiedObjectsList_p
	OamSACacheModifiedObjectsList getFinalModifiedObjectsList();

	//Sets/modifies the finalModifiedObjectsList_p
	void setFinalModifiedObjectsList(OamSACacheModifiedObjectsList& modObjList);

	//Clears the finalModifiedObjectsList_p
	void clearFinalModifiedObjectsList();

	// Spilts a name of the form "safBlolp=murkelluns,soffBump=blurlplapp,snorkApp=boppebopp" into
	// a list with three elements "safBlolp=murkelluns" "soffBump=blurlplapp" and "snorkApp=boppebopp"
	void		SplitDN(const std::string& theDn, DNList& theSplitDN);

	// These routines register the corresponding datastructures in the corresponding lists in the cache context
	// so they might be deleted when the chache is destroyed
	void RegisterAttributeValueContainer(MafMoAttributeValueContainer_3T* valuec);
	void RegisterBridgeImmIter(BridgeImmIterator* biip);
	// for each iterator there could be many strings. So using map, not list
	void RegisterCString(BridgeImmIterator* biip, char* namep);

	SaImmHandleT GetImmHandle();
	bool ReleaseAttributeValueContainer(MafMoAttributeValueContainer_3T* container);
	std::list<MafMoAttributeValueContainer_3T*>& GetAttValCList();
	std::list<BridgeImmIterator*>& GetBridgeImmIter();
	std::map<BridgeImmIterator*, std::list<char*> >& GetTheBridgeImmIterMap();

	// Retrieves attribute information for a specified attribute for a specified class from IMM
	bool	GetImmAttributeData(char* className,
								char* attributeName,
								SaImmClassCategoryT& category,
								SaImmValueTypeT&	valueType,
								SaImmAttrFlagsT&	flags);

	// Retrives the class name from an object instance.
	bool 	GetClassNameFromImm(const std::string& objectName,
								std::string& className);
	MafReturnT checkObjectExistInImm(const std::string& objectName);
	void DumpModifiedList();
private:
	// Helper methods to GetListOfxxxxxObjects
	void	GetDeletedData(OamSACacheDeletedObjectsList& dol);
	void	GetModifiedData(OamSACacheModifiedObjectsList& dol);
	void	GetCreatedData(OamSACacheCreatedObjectsList& dol);

	// Private members
	OamSADeletedObjectsList*		myDeletedObjectsList_p;
	OamSAModifiedObjectsList*		myModifiedObjectsList_p;
	std::list<MafMoAttributeValueContainer_3T*> myAttValCList;
	std::list<BridgeImmIterator*> myBridgeImmIterList;
	std::map<BridgeImmIterator*, std::list<char*> > myBridgeImmIterMap;
	SaImmHandleT myImmHandle;
	SaImmAccessorHandleT myAccessorHandle;
	OamSACacheModifiedObjectsList finalModifiedObjectsList_p;
};

// Non member global functions
extern void GlobalSplitDN(const std::string& theDN, OamSACache::DNList& theSplitDN);
extern void GlobalSplitStrDN(const char *theDN, OamSACache::DNList& theSplitDN);
extern void GlobalSplitMocPath(const std::string& mocPath, OamSACache::MocPathList& theSplitPath);

#endif
