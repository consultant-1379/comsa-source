/**
 *   Copyright (C) 2012 by Ericsson AB
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
 *   File:   OamSAKeyAttributeRepository.cc
 *
 *   Author: uabjoy
 *
 *   Date:   2010-05-21
 *
 *   This file implementing a repository for each class in the managed object repository.
 *
 *   Modify: eaparob 2013-01-28:   data structure and its functions added to store isNotifiable data
 *
 */

#include "OamSAKeyAttributeRepository.h"

OamSAKeyAttributeRepository::OamSAKeyAttributeRepository() {
	// TODO Auto-generated constructor stub
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
}

OamSAKeyAttributeRepository::~OamSAKeyAttributeRepository() {
	// TODO Auto-generated destructor stub
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAKeyAttributeRepository::~OamSAKeyAttributeRepository() called");
	if(!momOamSAKeyMOMRepoMAP.empty())
	{
		std::map<std::string, OamSAKeyMOMRepo*>::iterator momNamepointer = momOamSAKeyMOMRepoMAP.begin();
		while (momNamepointer != momOamSAKeyMOMRepoMAP.end())
		{
			DEBUG_OAMSA_TRANSLATIONS("OamSAKeyAttributeRepository::~OamSAKeyAttributeRepository map element delete call");
			if(momNamepointer->second != NULL)
			{
				delete momNamepointer->second;
				momNamepointer->second = NULL;
			}
			momNamepointer++;
		}
		momOamSAKeyMOMRepoMAP.clear();
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

// Method to retrieve the key repository for a mom
OamSAKeyMOMRepo* OamSAKeyAttributeRepository::getMOMRepo(const std::string parentClass_rootClass)
{
	if (momNameExist(parentClass_rootClass))
	{
		DEBUG_OAMSA_TRANSLATIONS("getMOMRepo found repo for %s", parentClass_rootClass.c_str());
		std::map<std::string, OamSAKeyMOMRepo*>::iterator tempReference = momOamSAKeyMOMRepoMAP.find(parentClass_rootClass);
		return (*tempReference).second;
	}
	DEBUG_OAMSA_TRANSLATIONS("getMOMRepo failed to find repo for %s", parentClass_rootClass.c_str());
	return &emptyMOM;
}

// Method to retrieve the key repository for a mom USING Imm key attribute
OamSAKeyMOMRepo* OamSAKeyAttributeRepository::getMOMRepoImmKey(const std::string immKeyAttribute)
{
	if (momNameExistImmKey(immKeyAttribute))
	{
		DEBUG_OAMSA_TRANSLATIONS("getMOMRepoImmKey found repo for %s", immKeyAttribute.c_str());
		std::map<std::string, std::string>::iterator tempImmKeyReference = momOamSAKeyMOMRepoMAP_immKey.find(immKeyAttribute);
		std::map<std::string, OamSAKeyMOMRepo*>::iterator tempReference = momOamSAKeyMOMRepoMAP.find((*tempImmKeyReference).second);
		return (*tempReference).second;
	}
	DEBUG_OAMSA_TRANSLATIONS("getMOMRepoImmKey failed to find repo for %s", immKeyAttribute.c_str());
	return &emptyMOM;
}


// This method creates an instance of OamSAKeyMOMRepo that is empty
// Then to fill this instance, you need to use the getMOMRepo() method
// and fill in the values required.
bool OamSAKeyAttributeRepository::setMOMRepo(const std::string parentClass_rootClass)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (!momNameExist(parentClass_rootClass))
	{
		DEBUG_OAMSA_TRANSLATIONS("setMOMRepo creating repo for %s", parentClass_rootClass.c_str());
		momOamSAKeyMOMRepoMAP[parentClass_rootClass] = new OamSAKeyMOMRepo();
		return true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}

// This method creates a search key built on the IMM Key Attribute for the MOM,
// this is out of the box, i.e. if it is decorated it is set like that directly here
bool OamSAKeyAttributeRepository::setMOMRepoKeyAttribute(const std::string immKeyAttribute,
														const std::string parentClass_rootClass)
{	ENTER_OAMSA_TRANSLATIONS();
	if (momNameExist(parentClass_rootClass))
	{
		DEBUG_OAMSA_TRANSLATIONS("setMOMRepoKeyAttribute adding key attribute %s for %s", immKeyAttribute.c_str(), parentClass_rootClass.c_str());
		momOamSAKeyMOMRepoMAP_immKey[immKeyAttribute] = parentClass_rootClass;
		return true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}

// Method to test for if a imm keyattribute key exists
bool OamSAKeyAttributeRepository::momNameExistImmKey(const std::string immKeyAttribute)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::map<std::string, std::string>::iterator momNamepointer = momOamSAKeyMOMRepoMAP_immKey.find(immKeyAttribute);
	if (momNamepointer != momOamSAKeyMOMRepoMAP_immKey.end())
		return true;
	// else do this
	LEAVE_OAMSA_TRANSLATIONS();
	return	false;
}


// Method to test for if a parent Root class key exists
bool OamSAKeyAttributeRepository::momNameExist(const std::string parentClass_rootClass)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::map<std::string, OamSAKeyMOMRepo*>::iterator momNamepointer = momOamSAKeyMOMRepoMAP.find(parentClass_rootClass);
	if (momNamepointer != momOamSAKeyMOMRepoMAP.end())
		return true;
	// else do this
	LEAVE_OAMSA_TRANSLATIONS();
	return	false;
}

// Implementation of class OamSAKeyMOMRepo

// Method to get the key attribute for a particular class in this MOM
std::string OamSAKeyMOMRepo::getKeyAttributeForClass(const std::string ClassName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::map<std::string, keyPair>::iterator keyAtributepointer = keyAttributeMap.find(ClassName);
	if (keyAtributepointer != keyAttributeMap.end())
	{
		keyPair theResult = (*keyAtributepointer).second;
		return theResult.first;
	}
	// else do this
	LEAVE_OAMSA_TRANSLATIONS();
	return	empty.first;
}

// Method to get the decorated key attribute for a particular class in this MOM
std::string OamSAKeyMOMRepo::getDecoratedKeyAttributeForClass(const std::string ClassName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::map<std::string, keyPair>::iterator keyAtributepointer = keyAttributeMap.find(ClassName);
	if (keyAtributepointer != keyAttributeMap.end())
	{
		keyPair theResult = (*keyAtributepointer).second;
		return theResult.second;
	}
	// else do this
	LEAVE_OAMSA_TRANSLATIONS();
	return	empty.second;
}

// Method to set a key attribute for a class
bool OamSAKeyMOMRepo::setKeyAttributeForClass(const std::string ClassName,
							 const std::string KeyAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (!classNameInMOM(ClassName))
	{
		DEBUG_OAMSA_TRANSLATIONS("setKeyAttributeForClass Adding %s = %s", ClassName.c_str(), KeyAttributeName.c_str());
		keyAttributeMap[ClassName] = keyPair(KeyAttributeName,"");
		classNameMap[KeyAttributeName] = ClassName;
		return true;
	}
	// else do this
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}

// Method to set a decorated key attribute for a class
bool OamSAKeyMOMRepo::setDecoratedKeyAttributeForClass(const std::string ClassName,
							 	 	  const std::string DecoratedKeyAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::map<std::string, keyPair>::iterator keyAtributepointer = keyAttributeMap.find(ClassName);
	if (keyAtributepointer != keyAttributeMap.end())
	{
		DEBUG_OAMSA_TRANSLATIONS("setDecoratedKeyAttributeForClass Adding %s = %s", ClassName.c_str(), DecoratedKeyAttributeName.c_str());
		keyPair theResult = (*keyAtributepointer).second;
		theResult.second = DecoratedKeyAttributeName;
		// Set the value again
		keyAttributeMap[ClassName] = theResult;
		// set the map for the class as well
		classNameMap[DecoratedKeyAttributeName] = ClassName;
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}
	// NO, no place to put this attribute
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}

// Method to test for if a key attribute exist
bool OamSAKeyMOMRepo::classNameInMOM(const std::string ClassName)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string temporaryKey = getKeyAttributeForClass(ClassName);
	if (temporaryKey == "")
	{
		return false;
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("classNameInMOM found %s", temporaryKey.c_str());
		return true;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	// To remove warning from compiler
	return false;
}

void OamSAKeyMOMRepo::deleteAttrNotifiableMaps(void)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::deleteAttrNotifiableMaps() called");
	if(!classAttributeNotifiableMap.empty())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::deleteAttrNotifiableMaps(): ENTER size %d",(int)(classAttributeNotifiableMap.size()));
		classAttributeNotifiableMapIteratorType it;
		for(it = classAttributeNotifiableMap.begin(); it != classAttributeNotifiableMap.end(); it++)
		{
			if(it->second != NULL)
			{
				delete it->second;
				it->second = NULL;
			}
		}
		classAttributeNotifiableMap.clear();
	}
	LEAVE_OAMSA_TRANSLATIONS();
}

// This function called by "populate" only in case when the MOM is decorated.
void OamSAKeyMOMRepo::setMomIsDecoratedFlag(void)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::setMomIsDecoratedFlag(): mark this MOM as decorated");
	momDecorated = true;
	LEAVE_OAMSA_TRANSLATIONS();
	return;
};

/*
 * check if the MOM is decorated
 * return true  : if MOM decorated
 * return false : if MOM not decorated
 */
bool OamSAKeyMOMRepo::isMomDecorated(void)
{
	ENTER_OAMSA_TRANSLATIONS();
	if(momDecorated)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}
	else
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
};

// add to the map: <className, attrMap>
// where attrMap is <attributeName, isNotifiable>
void OamSAKeyMOMRepo::addAttributeToIsNotifiableMap(const std::string className, const std::string attributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	// search in the map if className already exists
	classAttributeNotifiableMapIteratorType it = classAttributeNotifiableMap.find(className);
	attributeNotifiableMapType *attrNotMap;
	// if: if className exists then use its existing attribute map
	// else: class does not exists so allocate new map for its attributes
	if(it != classAttributeNotifiableMap.end())
	{
		attrNotMap = it->second;
	}
	else
	{
		attrNotMap = new attributeNotifiableMapType;
	}
	// fill in attributes to the map
	// we don't check if the attribute already exists, because there must be only one attribute with the same name under a specific class
	(*attrNotMap)[attributeName] = true;
	classAttributeNotifiableMap[className] = attrNotMap;
	LEAVE_OAMSA_TRANSLATIONS();
	return;
}

// check if the input class is the root class of the MOM
// if yes : return true
// if no  : return false
bool OamSAKeyMOMRepo::isRootClass(std::string className)
{
	ENTER_OAMSA_TRANSLATIONS();
	if(className == rootClassName)
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

// this function is used only for debugging
void OamSAKeyMOMRepo::printClassAttributeNotifiableMap()
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::printClassAttributeNotifiableMap(): Print all notifiable attributes in this MOM: MOM name (%s)",momName.c_str());
	classAttributeNotifiableMapIteratorType cit;
	for(cit = classAttributeNotifiableMap.begin(); cit != classAttributeNotifiableMap.end(); cit++)
	{
		std::string className = cit->first;
		attributeNotifiableMapType *attrNotMap = cit->second;
		std::string rootClassText = "";
		if(isRootClass(className))
		{
			rootClassText = " root";
		}
		DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::printClassAttributeNotifiableMap():    Attributes of%s class (%s):",rootClassText.c_str(), className.c_str());

		attributeNotifiableMapIteratorType nit;
		for(nit = attrNotMap->begin(); nit != attrNotMap->end(); nit++)
		{
			//DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::printClassAttributeNotifiableMap():        class (%s) attribute (%s) isNotifiable (%d)",className.c_str(), nit->first.c_str(), nit->second);
			DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::printClassAttributeNotifiableMap():        class (%s) attribute (%s)",className.c_str(), nit->first.c_str());
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return;
}

// set the name of the MOM
bool OamSAKeyMOMRepo::setMomName(char *mom_Name)
{
	ENTER_OAMSA_TRANSLATIONS();
	if(mom_Name == NULL)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	else
	{
		momName = mom_Name;
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}
}

std::string OamSAKeyMOMRepo::getMomName()
{
	ENTER_OAMSA_TRANSLATIONS();
	LEAVE_OAMSA_TRANSLATIONS();
	return momName;
}

/* If attribute name not given, then check if the class is notifiable
 * else check if the attribute is notifiable
 * return true  if:     notifiable
 * return false if: not notifiable
 */
bool OamSAKeyMOMRepo::isNotifiable(const char *className, const char *attrName)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool returnValue;
	// class name is a must to have for this function. If no class name given, then return false
	if(className == NULL)
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::isNotifiable(): bad input parameter: className is NULL");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	if(attrName == NULL)
	{
		returnValue = isClassNotifiable(className);
		LEAVE_OAMSA_TRANSLATIONS();
		return returnValue;
	}
	else
	{
		returnValue = isAttributeNotifiable(className, attrName);
		LEAVE_OAMSA_TRANSLATIONS();
		return returnValue;
	}
}

/*
 * This function will provide the iterator pointing to the found class map, when called with an iterator pointer(1.).
 *
 * possible calls:
 *    1. isClassNotifiable(className, &classIter);
 *    2. isClassNotifiable(className);
 *
 * returns:
 *
 *    true  if: class name found
 *    false if: class name not found
 *
 */
bool OamSAKeyMOMRepo::isClassNotifiable(const char *className, classAttributeNotifiableMapIteratorType *it)
{
	ENTER_OAMSA_TRANSLATIONS();
	classAttributeNotifiableMapIteratorType iterInstance;
	// if function called with NULL pointer iterator, then use local iterator instance
	// else use the provided iterator pointer, because the caller function will need it
	if(it == NULL)
	{
		it = &iterInstance;
	}
	*it = classAttributeNotifiableMap.find(className);
	if(*it != classAttributeNotifiableMap.end())
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::isClassNotifiable(): (%s)? : yes",className);
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::isClassNotifiable(): (%s)? : no",className);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
}

bool OamSAKeyMOMRepo::isAttributeNotifiable(const char *className, const char *attrName)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool returnValue;
	classAttributeNotifiableMapIteratorType classIter;
	returnValue = isClassNotifiable(className, &classIter);
	if(!returnValue)
	{
		// class not found, return false
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	// class found, so check if the input attribute is notifiable
	else
	{
		attributeNotifiableMapType *attrMap = classIter->second;
		attributeNotifiableMapIteratorType attrIter = attrMap->find(attrName);
		if(attrIter == attrMap->end())
		{
			// attribute not found (so it is not notifiable), return false
			DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::isAttributeNotifiable(): (%s)? : no",attrName);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		else
		{
			// attribute found (so it is notifiable), return true
			DEBUG_OAMSA_TRANSLATIONS("OamSAKeyMOMRepo::isAttributeNotifiable(): (%s)? : yes",attrName);
			LEAVE_OAMSA_TRANSLATIONS();
			return true;
		}
	}
}

