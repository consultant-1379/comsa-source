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
 *   Modify: eaparob 2013-01-28:   data structure added to store isNotifiable data
 *
 */

#ifndef OAMSAKEYATTRIBUTEREPOSITORY_H_
#define OAMSAKEYATTRIBUTEREPOSITORY_H_

#include <stdlib.h>
#include <map>
#include <string>
#include <algorithm>
#include <cctype>
#include <list>
#include "trace.h"
#include "ComSA.h"


class OamSAKeyMOMRepo {
	// This class keep the key attributes and its class names for a specific MOM
	// The MOM is identified in the class OamSAKeyAttributeRepository that acts an
	// container for a number of OamSAKeyMOMRepo's
public:
	OamSAKeyMOMRepo(): empty("",""){ ENTER_OAMSA_TRANSLATIONS(); momDecorated = false; momName = ""; LEAVE_OAMSA_TRANSLATIONS(); };
	~OamSAKeyMOMRepo() { ENTER_OAMSA_TRANSLATIONS();  deleteAttrNotifiableMaps(); LEAVE_OAMSA_TRANSLATIONS(); };

	OamSAKeyMOMRepo* operator=(OamSAKeyMOMRepo* dt)
	{
		// make a reference when this operator is called
		return (dt);
	}

	// Method to set the root class name for this MOM
	void setRootClass(const std::string RootClassName)
	{
		rootClassName = RootClassName;
	}

	// Method to get the root class name for this MOM
	std::string getRootClass()
	{
	    ENTER_OAMSA_TRANSLATIONS();
		DEBUG_OAMSA_TRANSLATIONS("getRootClass returns %s", rootClassName.c_str());
		LEAVE_OAMSA_TRANSLATIONS();
		return rootClassName;
	}

	// Method to get the key attribute for a particular class in this MOM
	std::string getKeyAttributeForClass(const std::string ClassName);

	// Method to get the decorated key attribute for a particular class in this MOM
	std::string getDecoratedKeyAttributeForClass(const std::string ClassName);


	// Method to set a key attribute for a class
	bool setKeyAttributeForClass(const std::string ClassName,
								 const std::string KeyAttributeName);

	// Method to set a decorated key attribute for a class
	bool setDecoratedKeyAttributeForClass(const std::string ClassName,
								 	 	  const std::string DecoratedKeyAttributeName);

	// Method to test for if a key attribute exist
	bool classNameInMOM(const std::string ClassName);

	// Method returning the class corresponding to a key attribute
	std::string getClassNameForKeyAttribute(const::std::string keyAttribute)
	{
		ENTER_OAMSA_TRANSLATIONS();
		std::map<std::string, std::string>::iterator keyAtributepointer = classNameMap.find(keyAttribute);
		if (keyAtributepointer != classNameMap.end())
		{
			LEAVE_OAMSA_TRANSLATIONS();
			return (*keyAtributepointer).second;
		}
		// else do this return nothing
		LEAVE_OAMSA_TRANSLATIONS();
		return	"";
	}
	// This function called by "populate" only in case when the MOM is decorated.
	void setMomIsDecoratedFlag(void);
	// Get if the MOM is decorated or not
	bool isMomDecorated(void);
	void addAttributeToIsNotifiableMap(const std::string className, const std::string attributeName);
	void printClassAttributeNotifiableMap();
	bool isNotifiable(const char *className, const char *attrName = NULL);
	bool setMomName(char *mom_Name);
	std::string getMomName(void);
	bool isRootClass(std::string className);

private:
	// data structure to keep key attributes
	// key = class name
	// value = keyPair --> key attribute, prefixed key attribute
	typedef std::pair<std::string,std::string> keyPair;
	std::map<std::string, std::pair<std::string,std::string> > keyAttributeMap;
	std::pair<std::string,std::string> empty;
	// The root class name
	std::string rootClassName;
	// The map with all key attributes and their corresponding undecorated class names
	std::map<std::string, std::string> classNameMap;
	// variable for storing that the MOM is decorated or not. Populate function sets its value.
	// true if decorated
	// false if not decorated
	bool momDecorated;

	// define map type for attributes where:
	// key: name of the notifiable attribute
	// value: empty
	typedef std::map<std::string,bool> attributeNotifiableMapType;
	typedef attributeNotifiableMapType::iterator attributeNotifiableMapIteratorType;
	// a map to store the key attributes in classes
	// key: class name
	// value: map (key: notifiable attribute name, value: empty)
	typedef std::map<std::string, attributeNotifiableMapType*> classAttributeNotifiableMapType;
	typedef classAttributeNotifiableMapType::iterator classAttributeNotifiableMapIteratorType;
	classAttributeNotifiableMapType classAttributeNotifiableMap;
	bool isClassNotifiable(const char *className, classAttributeNotifiableMapIteratorType *it = NULL);
	bool isAttributeNotifiable(const char *className, const char *attrName);
	// the name of the MOM
	std::string momName;
	void deleteAttrNotifiableMaps(void);
};

class OamSAKeyAttributeRepository {
public:
	OamSAKeyAttributeRepository();
	~OamSAKeyAttributeRepository();

	// Method to retrieve the key repository for a mom
	OamSAKeyMOMRepo* getMOMRepo(const std::string parentClass_rootClass);

	// Method to retrieve the key repository for a mom USING Imm key attribute
	OamSAKeyMOMRepo* getMOMRepoImmKey(const std::string immKeyAttribute);

	// This method creates an instance of OamSAKeyMOMRepo that is empty
	// Then to fill this instance, you need to use the getMOMRepo() method
	// and fill in the values required.
	bool setMOMRepo(const std::string parentClass_rootClass);

	// This method creates a search key built on the IMM Key Attribute for the MOM,
	// this is out of the box, i.e. if it is decorated it is set like that directly here
	bool setMOMRepoKeyAttribute(const std::string immKeyAttribute,
			        			const std::string parentClass_rootClass);

	// Method to test for if a parent Root class key exists
	bool momNameExist(const std::string parentClass_rootClass);

	// Method to test for if a imm keyattribute key exists
	bool momNameExistImmKey(const std::string immKeyAttribute);

#ifdef UNIT_TEST
	void ResetRepo() {
		if(!momOamSAKeyMOMRepoMAP.empty())
		{
			std::map<std::string, OamSAKeyMOMRepo*>::iterator momNamepointer = momOamSAKeyMOMRepoMAP.begin();
			while (momNamepointer != momOamSAKeyMOMRepoMAP.end())
			{
				DEBUG_OAMSA_TRANSLATIONS("OamSAKeyAttributeRepository::ResetRepo map element delete call");
				if(momNamepointer->second != NULL)
				{
					delete momNamepointer->second;
					momNamepointer->second = NULL;
				}
				momNamepointer++;
			}
			momOamSAKeyMOMRepoMAP.clear();
		}
	}
#endif

private:
	// map data structure to keep key MOM data sorted like this :
	// key = parentclassName_rootclassName
	// value = instance of class OamSAKeyMOMRepo
	std::map<std::string, OamSAKeyMOMRepo*> momOamSAKeyMOMRepoMAP;
	// map data structure to keep the relation between the imm Key Attribute for the root and the MOM search key :
	// key = immKeyAttributeName
	// value = parentclassName_rootclassName (For the corresponding MOM for this IMM Key)
	std::map<std::string, std::string> momOamSAKeyMOMRepoMAP_immKey;
	OamSAKeyMOMRepo emptyMOM;
};

#endif /* OAMSAKEYATTRIBUTEREPOSITORY_H_ */
