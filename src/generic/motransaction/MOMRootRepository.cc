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
 *	 File:	 MOMRootRepository.cc
 *
 *	 Author: xnikvap
 *
 *	 Date:	 2014-03-06
 *
 *	 This class provides data storage for the MOM information data
 *	 Separated from OamSATranslator.cc
 *
 *	 Reviewed:
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 */

#include "MOMRootRepository.h"
#include "OamSATranslator.h"

#include <fstream>

extern MafOamSpiModelRepository_4T* theModelRepo_v4_p;

extern int theModelRepoVersion;

// Repository for the existing key attributes in the model repository
// Only visible inside OamSATranslator
extern OamSAKeyAttributeRepository theKeyHolder;

/**
 *	Default constructor
 */
MOMRootRepository::MOMRootRepository():
	immNameDecorationExtension(DOMAIN_EXT_NAME_IMM_NAMESPACE),
	decorationEnabledForMOM(DN_VALUE_MOM_NAME),
	domainEcim(DOMAIN_NAME_ECIM),
	domainImm(DOMAIN_NAME_IMM),
	domainCoreMW(DOMAIN_NAME_COREMW),
	contSeparator(CONTAINMENT_SEPARATOR)
{
	ENTER_OAMSA_TRANSLATIONS();
	mapMocHandler.clear();
	mapMocpathToAttributes.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}


#ifdef UNIT_TEST
/*
 * MOMRootRepository::ResetRootRepository
 *
 * Clear the myMOMRootMap data structure, to make unit test easy.
 * */
void MOMRootRepository::ResetRootRepository()
{
	// Ok, remove the content of the MOMRootDataEntryMap
	myMOMRootMap.clear();
	containmentSplitImmDnAttributes.clear();
}
#endif // UNIT_TEST


// Returns the decorated (3GPP) Root class name if found in parameter DecoratedRootClassName
std::string& MOMRootRepository::FindDecoratedClassName(const std::string& immRootName)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Trim off = and anything beyond
	std::string theSearchStr = immRootName.substr(0, immRootName.find('='));
	if ( theSearchStr.find('.') != std::string::npos)
	{
		theSearchStr = theSearchStr.erase(theSearchStr.find_first_of('.'));
	}
	std::map<std::string, std::string>::iterator it_Search = registeredDecoratedIMMKeyAttributes.find(theSearchStr);
	if (it_Search != registeredDecoratedIMMKeyAttributes.end())
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return it_Search->second;
	}
	else
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return emptyString;
	}
}


// Returns true if a root class for a MOM is decorated in IMM (Has the value MOM_VALUE set in the MOM)
bool  MOMRootRepository::FindIfRootMOMIsDecorated(std::string& rootClassName,
												  std::string& rootParent)
{
	ENTER_OAMSA_TRANSLATIONS();
	// OK, search the map and find if an entry exist in there, if so return true otherwise return false
	// First we need to clean the names from possible key attribute names like:
	// CmwTest.Tricky ---becomes----> CmwTest
	std::size_t postionOfDotInRootParent = rootParent.find_first_of(".");
	std::size_t positionOfDotInRootClassName = rootClassName.find_first_of(".");
	if (postionOfDotInRootParent!=std::string::npos)
	{
		rootParent = rootParent.substr(0, postionOfDotInRootParent);
	}
	if (positionOfDotInRootClassName!=std::string::npos)
	{
		rootClassName = rootClassName.substr(0, positionOfDotInRootClassName);
	}
	std::string searchKey = rootParent + "_" + rootClassName;
	std::map<std::string, bool>::iterator it_Search = registeredParentMOCWithMOMVALUESet.find(searchKey);
	if (it_Search != registeredParentMOCWithMOMVALUESet.end())
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


// Get the original rootclass IMMDN for a decorated IMM rootclass DN
// Expect input like : CmwPmpmId=1 and should return pmId=1
void  MOMRootRepository::FindUnDecoratedImmDN(const std::string& decoratedImmDN,
											  std::string& unDecoratedImmDN)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Search the list of decorated imm DN that we now of in COM-SA
	// Check so we do not receive the empty string here
	// Convert to Class name, only to remove everything after =
	std::string className = decoratedImmDN;
	std::string equalSignAndTheRest = decoratedImmDN;
	// Correction of TR HP81651, which generated a core dump in this position.
	bool trimOfClassName = false;
	size_t pos = decoratedImmDN.find_first_of('=');
	if (pos != std::string::npos)
	{
		className.erase(pos, std::string::npos);
		equalSignAndTheRest.erase(0, pos);
		trimOfClassName = true;
	}
	// TEST CODE HERE
	// END OF TEST CODE
	std::string tempResult;
	std::map<std::string, std::string>::iterator it_Search = registeredDecoratedImmRootClassesName.find(className);
	if (it_Search != registeredDecoratedImmRootClassesName.end())
	{
		if (trimOfClassName)
		{
			tempResult = it_Search->second;
			unDecoratedImmDN = tempResult + equalSignAndTheRest;
		}
		else
		{
			unDecoratedImmDN = it_Search->second;
		}
	}
	else
	{
		unDecoratedImmDN = emptyString;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


// Get the decoration for a decorated MOM
void  MOMRootRepository::FindDecorationForMOMDN(const std::string& rootDN, const std::string& parent_rootDN,
												std::string& rootClassNameDecoration)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Search the list of decorated classes that we now of in COM-SA
	rootClassNameDecoration.clear();
	// Convert to Class name, only to remove everything after = or everything before '.'
	// Example
	// ManagedElement.slurk=1 --> ManagedElement
	// ManagedElement=1		  --> ManagedElement
        std::string parent_className = parent_rootDN;
	std::string className = rootDN;
	size_t pos = className.find_first_of('.');
	if (pos != std::string::npos)
	{
		className.erase(pos, std::string::npos);
	}
	else
	{
		pos = className.find_first_of('=');
		if (pos != std::string::npos)
		{
			className.erase(pos, std::string::npos);
		}
	}
        pos = parent_className.find_first_of('.');
        if (pos != std::string::npos)
	{
		parent_className.erase(pos, std::string::npos);
	}
	else
	{
		pos = parent_className.find_first_of('=');
		if (pos != std::string::npos)
		{
			parent_className.erase(pos, std::string::npos);
		}
	}

        std::string searchKey = parent_className + "_" + className;
        OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
        std::string decoratedkeyAttributeName = momKeyRepo->getDecoratedKeyAttributeForClass(className);
        std::string TheDecoratedRootName = FindDecoratedClassName(decoratedkeyAttributeName);
        rootClassNameDecoration = TheDecoratedRootName.substr(0, TheDecoratedRootName.rfind(className));
        DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::FindDecorationForMOMDN -- rootClassNameDecoration = %s",rootClassNameDecoration.c_str());
        LEAVE_OAMSA_TRANSLATIONS();
}


// Search for decorated key attribute found when populating internal data structure.
void  MOMRootRepository::FindDecoratedKeyAttribute(const std::string &keyAttribute,
													std::string& newKeyAttribute)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Search the list of decorated key attributes that we now of in COM-SA
	newKeyAttribute.clear();
	std::map<std::string, std::string>::iterator it_Search = registeredDecoratedKeyAttributes.find(keyAttribute);
	if (it_Search != registeredDecoratedKeyAttributes.end())
	{
		newKeyAttribute = it_Search->second;
	}
	else
	{
		newKeyAttribute = emptyString;
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


// This method stores the extension details for a containment.
void MOMRootRepository::storeExtension(const std::string contName, const std::string extName, const std::string extValue)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::storeExtension Containment Name %s, Extension Name: %s, Value: %s", contName.c_str(), extName.c_str(), extValue.c_str());
	if (extName.compare(DOMAIN_EXT_NAME_SPLIT_IMM_DN) == 0)
	{
		if (extValue.compare(DOMAIN_EXT_VALUE_TRUE) == 0)
		{
			containmentSplitImmDnAttributes[contName].value = true;
		}
		else if (extValue.compare(DOMAIN_EXT_VALUE_FALSE) == 0)
		{
			containmentSplitImmDnAttributes[contName].value = false;
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::storeExtension splitImmDn invalid value");
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


// The method extracts the parent and child names from a containment name
bool MOMRootRepository::extractContainment(const std::string containment, std::string &parent, std::string &child)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool retVal = false;
	std::size_t found;
	found = containment.find(contSeparator);
	if (found != std::string::npos)
	{
		parent = containment.substr(0, found);
		child = containment.substr(found+contSeparator.length());
		retVal = true;
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}

// This method checks if the model is valid.
// - if a child has two parents, only on parent containment can have splitImmDn = true
void MOMRootRepository::checkModelValid()
{
	ENTER_OAMSA_TRANSLATIONS();
	std::map<std::string, SplitImmDnAttributes>::iterator it_parent1, it_parent2;
	std::string parent1, parent2, child1, child2;
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::checkModelValid containmentSplitImmDnAttributes count=%d", static_cast<int>(containmentSplitImmDnAttributes.size()));
	for (it_parent1 = containmentSplitImmDnAttributes.begin(); it_parent1 != containmentSplitImmDnAttributes.end(); ++it_parent1)
	{
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::checkModelValid containment -%s", (it_parent1->first).c_str());
		if (extractContainment(it_parent1->first, parent1, child1))
		{
			if (it_parent1->second.value)
			{
				for(it_parent2 = it_parent1; it_parent2 != containmentSplitImmDnAttributes.end(); ++it_parent2)
				{
					if (extractContainment(it_parent2->first, parent2, child2))
					{
						if ((child1.compare(child2) == 0) && (parent1.compare(parent2) != 0) && it_parent2->second.value)
						{
							//check if the child MOCs are in the same MOM.
							if (it_parent1->second.childMocMomName == it_parent2->second.childMocMomName)
							{
								ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkModelValid invalid model: Class %s has more than one parent: %s, %s with splitImmDn set to true", child1.c_str(), parent1.c_str(), parent2.c_str());
								it_parent1->second.valid = false;
								it_parent2->second.valid = false;
							}
						}
					}
				}
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


/*
 * MOMRootRepository::Populate
 *
 * Populates the mom repository with the root nodes found
 *
 * Added extension using MafOamSpiModelRepository_4T to support decoration of MOM MOC classes
 * in the IMM database to avoid name clash between classes in MOM's with the same name.
 */
void MOMRootRepository::Populate()
{
	ENTER_OAMSA_TRANSLATIONS();

	Populate_V4();
	LEAVE_OAMSA_TRANSLATIONS();
}


/*
 * MOMRootRepository::IsRoot
 *
 * Searches the repository to see if the concatenation, i.e. theParent + theString exist in the repository
 * if it is we have a root.
 * Return true if the root is found
 * Returns false otherwise
 */
bool	MOMRootRepository::IsRoot(const std::string& theString, std::string& theParent)
{
	ENTER_OAMSA_TRANSLATIONS();
	//DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::IsRoot : enter theString=%s / theParent=%s", theString.c_str(), theParent.c_str());
	MOMRootDataEntryMapIterator theRetVal;
	if (myMOMRootMap.empty())
	{
		Populate();
	}
	// Check if this is a sruct class that we check for being a root
	if (theString[0] == '=')
	{
		// This is a struct class, which never is root
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	// Trim off = and anything beyond
	std::string theSearchStr = theString.substr(0, theString.find('='));
	std::string theSearchStrParent = theParent.substr(0, theParent.find('='));
	if ( theSearchStr.find('.') != std::string::npos)
	{
		theSearchStr = theSearchStr.erase(theSearchStr.find_first_of('.'));
	}
	// Remove everything after .
	if ( theSearchStrParent.find('.') != std::string::npos)
	{
		theSearchStrParent = theSearchStrParent.erase(theSearchStrParent.find_first_of('.'));
	}
	// NEw search key is parent + root name
	theSearchStr = theSearchStrParent + theSearchStr;
	//DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository IsRoot searching for %s in cached repository", theSearchStr.c_str());
	theRetVal = myMOMRootMap.find(theSearchStr);
	LEAVE_OAMSA_TRANSLATIONS();
	//DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository IsRoot returning theRetVal.second = %d", (int)theRetVal.second);
	return theRetVal != myMOMRootMap.end();
}


/*
 * MOMRootRepository::IsSingletonRoot
 * MR26712.
 * Searches the repository to see if the concatenation, i.e. theParent + theString exist in the repository and is singleton class
 * if it is we have a root.
 * Return true if the root is found
 * Returns false otherwise
 */
bool	MOMRootRepository::IsSingletonRoot(const std::string& theString, std::string& theParent)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::IsSingletonRoot : enter theString=%s / theParent=%s", theString.c_str(), theParent.c_str());
	MOMRootDataEntryMapIterator theRetVal;
	if (myMOMRootMap.empty())
	{
		Populate();
	}
	// Check if this is a sruct class that we check for being a root
	if (theString[0] == '=')
	{
		// This is a struct class, which never is root
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	if (theParent.empty())
	{
		// Must be at top - "ManagedElement=1"
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}

	// Trim off = and anything beyond
	std::string theSearchStr = theString.substr(0, theString.find('='));
	std::string theSearchStrParent = theParent.substr(0, theParent.find('='));
	std::string theContainmentSearchStr = theSearchStrParent + contSeparator + theSearchStr;

	if (!isSplitImmDn(theContainmentSearchStr))
	{
		// NOT a singleton class
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	if ( theSearchStr.find('.') != std::string::npos)
	{
		theSearchStr = theSearchStr.erase(theSearchStr.find_first_of('.'));
	}
	// Remove everything after .
	if ( theSearchStrParent.find('.') != std::string::npos)
	{
		theSearchStrParent = theSearchStrParent.erase(theSearchStrParent.find_first_of('.'));
	}
	// New search key is parent + root name
	theSearchStr = theSearchStrParent + theSearchStr;
	//DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository IsSingletonRoot searching for %s in cached repository", theSearchStr.c_str());
	theRetVal = myMOMRootMap.find(theSearchStr);
	LEAVE_OAMSA_TRANSLATIONS();
	//DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository IsSingletonRoot returning theRetVal.second = %d", (int)(theRetVal != myMOMRootMap.end()));
	return theRetVal != myMOMRootMap.end();
}


/*
 * MOMRootRepository::Find
 *
 * Searches the repository to see if the specified name is in there,
 * if it is we have a root.
 *
 */
MOMRootRepository::MOMRootDataEntryMapRetVal	MOMRootRepository::Find(const std::string& theString)
{
	ENTER_OAMSA_TRANSLATIONS();
	MOMRootDataEntryMapRetVal theRetVal;
	if (myMOMRootMap.empty())
	{
		Populate();
	}

	// Trim off = and anything beyond
	std::string theSearchStr = theString.substr(0, theString.find('='));
	if ( theSearchStr.find('.') != std::string::npos)
	{
		theSearchStr = theSearchStr.erase(theSearchStr.find_first_of('.'));
	}
	theRetVal.first = myMOMRootMap.find(theSearchStr);
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository Find searching for %s in cached repository", theSearchStr.c_str());
	theRetVal.second= theRetVal.first != myMOMRootMap.end();
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository Find returning theRetVal.second = %d", theRetVal.second);
	LEAVE_OAMSA_TRANSLATIONS();
	return theRetVal;
}


/*
 * MOMRootRepository::GetRootSiblingList
 *
 * Looks for children (also MOM roots) of the specified node
 * in order to be able to put together a complete (3gpp)tree
 *
 */
void MOMRootRepository::GetRootSiblingList(const std::string& theParentName,
											std::list<std::string> &theSiblingList)
{
	ENTER_OAMSA_TRANSLATIONS();
	// first, check so that it isn't empty the same as for Find
	if (myMOMRootMap.empty())
	{
		Populate();
	}

	DEBUG_OAMSA_TRANSLATIONS("GetRootSiblingList looking for root children to \"%s\"",theParentName.c_str());

	// Just to be sure, don't think it's really neccessary

	//std::string theSearchStr = theParentName.substr(0, theParentName.find('=')); // Unused variable

	// And this shouldn't be neccesary either

	theSiblingList.clear();

	// Now walk across the map and find the root entries which have the specified father

	for (MOMRootDataEntryMapIterator iter = myMOMRootMap.begin(); iter != myMOMRootMap.end(); iter++)
	{
		std::string aParentName = iter->second.GetParentName();
		DEBUG_OAMSA_TRANSLATIONS("GetRootSiblingList for  \"%s\" parent is \"%s\" ", iter->first.c_str(), aParentName.c_str());
		if (aParentName == theParentName)
		{
			theSiblingList.push_back(iter->first);
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
}


/* MOMRootRepository::isSplitImmDn method
 *
 * This method return the isSplitImmDn flag extension
 *
 * return true : if the flag is not provided or split IMM DN flag is true
 * return false: if split IMM DN flag is false
 */
bool MOMRootRepository::isSplitImmDn(const std::string parentClass_rootClass)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool retVal = true;
	std::string parentClass_to_rootClass;
	if ( parentClass_rootClass.find('.') != std::string::npos)
	{
		std::string theSearchStrParent = parentClass_rootClass.substr(0, parentClass_rootClass.find(contSeparator));
		std::string theSearchStr = parentClass_rootClass.substr(parentClass_rootClass.find(contSeparator)+contSeparator.size());
		if ( theSearchStr.find('.') != std::string::npos)
		{
			theSearchStr = theSearchStr.erase(theSearchStr.find_first_of('.'));
		}
		// Remove everything after .
		if ( theSearchStrParent.find('.') != std::string::npos)
		{
			theSearchStrParent = theSearchStrParent.erase(theSearchStrParent.find_first_of('.'));
		}
		parentClass_to_rootClass = theSearchStrParent + contSeparator + theSearchStr;
	}
	else
	{
		parentClass_to_rootClass = parentClass_rootClass;
	}

	std::map<std::string, SplitImmDnAttributes>::iterator it = containmentSplitImmDnAttributes.find(parentClass_to_rootClass);
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::isSplitImmDn %s", parentClass_to_rootClass.c_str());

	if (myMOMRootMap.empty())
	{
		Populate();
	}

	if (it != containmentSplitImmDnAttributes.end())
	{
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::isSplitImmDn containment found");
		retVal = it->second.value;
	}

	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::isSplitImmDn: parentClass_rootClass=%s retVal=%d", parentClass_to_rootClass.c_str(), retVal);
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}


/* MOMRootRepository::isSplitImmDnValid method
 *
 * This method return if the isSplitImmDn flag extension is valid
 *
 * return true : if the model is valid for the split IMM DN flag (only one parent has split IMM DN = true)
 * return false: if the model is invalid
 */
bool MOMRootRepository::isSplitImmDnValid(const std::string parentClass_rootClass)
{
	ENTER_OAMSA_TRANSLATIONS();
	bool retVal = true;
	std::string parentClass_to_rootClass;
	if ( parentClass_rootClass.find('.') != std::string::npos)
	{
		std::string theSearchStrParent = parentClass_rootClass.substr(0, parentClass_rootClass.find(contSeparator));
		std::string theSearchStr = parentClass_rootClass.substr(parentClass_rootClass.find(contSeparator)+contSeparator.size());
		if ( theSearchStr.find('.') != std::string::npos)
		{
			theSearchStr = theSearchStr.erase(theSearchStr.find_first_of('.'));
		}
		// Remove everything after .
		if ( theSearchStrParent.find('.') != std::string::npos)
		{
			theSearchStrParent = theSearchStrParent.erase(theSearchStrParent.find_first_of('.'));
		}
		parentClass_to_rootClass = theSearchStrParent + contSeparator + theSearchStr;
	}
	else
	{
		parentClass_to_rootClass = parentClass_rootClass;
	}

	std::map<std::string, SplitImmDnAttributes>::iterator it = containmentSplitImmDnAttributes.find(parentClass_to_rootClass);
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::isSplitImmDnValid %s", parentClass_to_rootClass.c_str());

	if (myMOMRootMap.empty())
	{
		Populate();
	}

	if (it != containmentSplitImmDnAttributes.end())
	{
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::isSplitImmDnValid containment found");
		retVal = it->second.valid;
	}

	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::isSplitImmDnValid: parentClass_rootClass=%s retVal=%d", parentClass_to_rootClass.c_str(), retVal);
	LEAVE_OAMSA_TRANSLATIONS();
	return retVal;
}


/*	========================================================================
 *	 New functions for MR-29333 No Root Classes using MR SPI Ver. 3
 */


// This method checks if the containment is valid.
// Inside the whole model containment tree, there must not be any splitting
//contribution below a class with more than one parent.
//This rule is recursive and should trace back through all ancestors in the
//model tree up to the root element "ManagedElement=1" and checks for any
//classes in the tree with more than one parent, to make sure IMM namespace is unique
void MOMRootRepository::checkContainmentValid_V4(char * contName, MafOamSpiMrContainmentHandle_4T &mocParentContainment)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafOamSpiMrMocHandle_4T subHandleMOCParentContainmentMOC1, subHandleMOCParentContainmentMOC2;
	MafOamSpiMrMocHandle_4T subHandleMOCChildContainmentMOC;
	MafOamSpiMrContainmentHandle_4T nextContainment;
	MafOamSpiMrGeneralPropertiesHandle_4T subHandleGeneralProperties;
	char *parent1;
	char *parent2;
	char *child;
	bool ModelInvalid = false;
	bool firstIteration = true;

	DEBUG_OAMSA_TRANSLATIONS("Enter checkContainmentValid_V4 with containment [%s]", contName);
	// ---------------
	// * This containment is a splitting containment with splitImmDn=true
	// * iterate up through parent containments.
	// * if any class has multiple parents, this is an invalid containment.
	// ---------------
	do
	{
		// ---------------
		//Get Next parent containment with same child
		// ---------------
		MafReturnT retVal = theModelRepo_v4_p->containment->getNextContainmentSameChildMoc(mocParentContainment, &nextContainment);
		if ((nextContainment.handle != mocParentContainment.handle) && (retVal == MafOk) && (false == firstIteration))
		{
			// ---------------
			// We have found a class with more than one parent. Log an error
			// ---------------
			containmentSplitImmDnAttributes[contName].valid=false;

			ModelInvalid=true;
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkContainmentValid_V4 invalid model: A class exists above the splitting contribution %s with more than one parent ", contName);
		}

		firstIteration = false;

		// ---------------
		//Get name for parent1
		// ---------------
		retVal = theModelRepo_v4_p->containment->getParentMoc(mocParentContainment, &subHandleMOCParentContainmentMOC1);
		if (retVal != MafOk)
		{
			// Handle error code
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository checkContainmentValid_V4 failed to get containment MOC, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}

		retVal = theModelRepo_v4_p->moc->getGeneralProperties(subHandleMOCParentContainmentMOC1, &subHandleGeneralProperties);
		if (retVal != MafOk)
		{
			// Handle error code
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkContainmentValid_V4 failed to get GeneralProperties for grandParent generalProperty, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}

		retVal = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleGeneralProperties,
																	MafOamSpiMrGeneralProperty_name_4,
																	const_cast<const char**>(&parent1));

		if (retVal != MafOk)
		{
			// Handle error code
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkContainmentValid_V4 failed to getStringProperty for parent1, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}



		if (ModelInvalid)
		{
			// ---------------
			//Get name for child
			// ---------------
			retVal = theModelRepo_v4_p->containment->getChildMoc(mocParentContainment, &subHandleMOCChildContainmentMOC);
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository checkContainmentValid_V4 failed to get containment MOC, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return;
			}

			retVal = theModelRepo_v4_p->moc->getGeneralProperties(subHandleMOCChildContainmentMOC, &subHandleGeneralProperties);
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkContainmentValid_V4 failed to get GeneralProperties for grandParent generalProperty, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return;
			}

			retVal = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleGeneralProperties,
																		MafOamSpiMrGeneralProperty_name_4,
																		const_cast<const char**>(&child));

			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkContainmentValid_V4 failed to getStringProperty for child, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return;
			}


			// ---------------
			//Get name for parent2
			// ---------------
			retVal = theModelRepo_v4_p->containment->getParentMoc(nextContainment, &subHandleMOCParentContainmentMOC2);
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository checkContainmentValid_V4 failed to get nextContainment MOC, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return;
			}

			retVal = theModelRepo_v4_p->moc->getGeneralProperties(subHandleMOCParentContainmentMOC2, &subHandleGeneralProperties);
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkContainmentValid_V4 failed to get GeneralProperties for grandParent generalProperty, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return;
			}

			retVal = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleGeneralProperties,
																		MafOamSpiMrGeneralProperty_name_4,
																		const_cast<const char**>(&parent2));

			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkContainmentValid_V4 failed to getStringProperty for parent2, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return;
			}

			// ---------------
			// Log an error and return
			// ---------------
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::checkContainmentValid_V4 invalid model: Class %s exists above the splitting contribution, and has two parents %s and %s", child, parent1, parent2);
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}

		// Otherwise, continue searching through parents recursively to the global model root "ManagedElement=1"

	} while (theModelRepo_v4_p->moc->getParentContainment(subHandleMOCParentContainmentMOC1, &mocParentContainment) == MafOk);

	LEAVE_OAMSA_TRANSLATIONS();
}



// This method loops over the containments for the class up to the root.
// The output is stored in the variable den.
bool MOMRootRepository::loopContainmentClass_V4(MafOamSpiMrContainmentHandle_4T &mocParentContainment,
												MafOamSpiMrContainmentHandle_4T &mocNextContainment,
												MOMRootDataEntry	&den)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Used in the do{ }while() condition
	MafOamSpiMrMocHandle_4T subHandleMOCParentContainmentMOC;
	MafOamSpiMrMocHandle_4T subHandleMOCChildContainmentMOC;
	const char* extNameSplitImm = DOMAIN_EXT_NAME_SPLIT_IMM_DN;
	bool firstLoopContainment = true;
	char* extensionValue;
	char* immNamespaceValue;

	do
	{
		if (!firstLoopContainment)
		{
			mocParentContainment = mocNextContainment;
		}
		else
		{
			firstLoopContainment = false;
		}
		MafReturnT retVal = theModelRepo_v4_p->containment->getParentMoc(mocParentContainment, &subHandleMOCParentContainmentMOC);
		if (retVal != MafOk)
		{
			// Handle error code
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository loopContainmentClass_V4 failed to get containment MOC, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		MafOamSpiMrGeneralPropertiesHandle_4T subHandleGrandParentGeneralProperties;
		retVal = theModelRepo_v4_p->moc->getGeneralProperties(subHandleMOCParentContainmentMOC, &subHandleGrandParentGeneralProperties);
		if (retVal != MafOk)
		{
			// Handle error code
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4 failed to get GeneralProperties for grandParent generalProperty, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}

		MafOamSpiMrGeneralPropertiesHandle_4T contProperties;
		// use this containment handle to obtain the containment properties
		retVal = theModelRepo_v4_p->containment->getGeneralProperties(mocParentContainment, &contProperties);
		if (MafOk != retVal)
		{
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4 failed to get GeneralProperties for parent containment, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4 Parent Containment Properties:");
			char* contName = NULL;
			char* momName = NULL;
			MafOamSpiMrGeneralPropertiesHandle_4T momHandleGeneralProperties;
			MafOamSpiMrMomHandle_4T momHandle;

			// ----------------------
			// Need to see if this parent_child containment is an EcimContribution
			// Compare parent class with momName of child using sameMom_V4()
			// -----------------------
			retVal = theModelRepo_v4_p->containment->getChildMoc(mocParentContainment, &subHandleMOCChildContainmentMOC);
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository loopContainmentClass_V4 failed to getChildMoc, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}


			retVal = theModelRepo_v4_p->moc->getMom(subHandleMOCChildContainmentMOC, &momHandle);
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository loopContainmentClass_V4 failed to getMom, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}

			retVal = theModelRepo_v4_p->mom->getGeneralProperties(momHandle, &momHandleGeneralProperties);
			//maf_retval = theModelRepo_v4_p->mom->getRootMoc(momHandle, &mocHandle);
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4 failed to get handle to MOMGeneralProperties , return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}

			retVal = theModelRepo_v4_p->generalProperties->getStringProperty(momHandleGeneralProperties,
			                                                                  MafOamSpiMrGeneralProperty_name_4,
			                                                                  const_cast<const char**>(&momName));
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4 failed to getStringProperty to MOMGeneralProperties , return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}

			retVal = theModelRepo_v4_p->generalProperties->getStringProperty(contProperties,
			                                                                  MafOamSpiMrGeneralProperty_name_4,
			                                                                  const_cast<const char**>(&contName));
			if (retVal != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository loopContainmentClass_V4 failed to getStringProperty, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}

			if (NULL != contName)
			{
				DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4  Parent Containment Name: %s", contName);
				containmentSplitImmDnAttributes[contName].valid = true;

				if (sameMom_V4(subHandleMOCParentContainmentMOC, const_cast<const char*>(momName)))
				{
					// All containments that are within the same MOM have default split=false
					containmentSplitImmDnAttributes[contName].value = false;
				}
				else
				{
					// All EcimContributions between different MOMs have default split=true
					containmentSplitImmDnAttributes[contName].value = true;
				}

				retVal = theModelRepo_v4_p->generalProperties->getDomainExtension(momHandleGeneralProperties,
				                                                                   const_cast<const char*>(domainEcim),
				                                                                   const_cast<const char*>(immNameDecorationExtension),
				                                                                   const_cast<const char**>(&immNamespaceValue));
				if ((MafOk == retVal) && (!strcmp(decorationEnabledForMOM, immNamespaceValue))){
					containmentSplitImmDnAttributes[contName].childMocMomName = static_cast<std::string>(momName);
				}
				else {
					DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4 failed to retrieve namespacePrefix. retVal:%d", int(retVal));
					containmentSplitImmDnAttributes[contName].childMocMomName = "";
				}
			}
			else
			{
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4 failed to get containment name, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}

			/*
			 ** Try retrieving extensions from the containment properties. For MAF MR SPI v3, the domain must be hard coded.
			 ** First try domain "CoreMW"
			 */
			retVal = theModelRepo_v4_p->generalProperties->getDomainExtension(contProperties,
			                                                                   const_cast<const char*>(domainCoreMW),
			                                                                   const_cast<const char*>(extNameSplitImm),
			                                                                   const_cast<const char**>(&extensionValue));

			if (retVal != MafOk)
			{
				/*
				 ** Next try with domain "IMM"
				 */
				retVal = theModelRepo_v4_p->generalProperties->getDomainExtension(contProperties,
				                                                                   const_cast<const char*>(domainImm),
				                                                                   const_cast<const char*>(extNameSplitImm),
				                                                                   const_cast<const char**>(&extensionValue));
			}

			if (retVal == MafOk)
			{
				DEBUG_OAMSA_TRANSLATIONS("loopContainmentClass_V4()  getDomainExtension [%s] for containment %s returns %s",
						extNameSplitImm, contName, extensionValue);

				if ((extNameSplitImm != NULL) && (extensionValue != NULL))
				{
					storeExtension(contName, extNameSplitImm, extensionValue);
				}
				else
				{
					ERR_OAMSA_TRANSLATIONS("MOMRootRepository::loopContainmentClass_V4: Containment Name %s, Invalid Extension Name: %s, Value:%s",
							contName, extNameSplitImm, extensionValue);
					LEAVE_OAMSA_TRANSLATIONS();
					return false;
				}
			}

			/*
			** Implement model checking for any child class below a containment with splitImmDn=true
			** Recursively search through all parents to global root "ManagedElement=1"
			** Must not be any class with more than one parent present.
			*/
			if (isSplitImmDn(contName))
			{
				checkContainmentValid_V4(contName, mocParentContainment);
			}

		}

		char* name = NULL;
		retVal = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleGrandParentGeneralProperties,
																		 MafOamSpiMrGeneralProperty_name_4,
																		 const_cast<const char**>(&name));
		if (retVal != MafOk)
		{
			// Handle error code
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository Populate failed to get MOC grandparent Name, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}

		std::string grandParentName = name;
		size_t pos = grandParentName.find('.');
		if (pos != std::string::npos)
		{
			// The name could be in the format Class.namingattribute=blopp
			grandParentName.erase(0, pos + 1);
		}
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository MOM Moc parent name %s", grandParentName.c_str());
		den.Insert(grandParentName);
		// OK, now get the containment for this parent
	} while (theModelRepo_v4_p->moc->getParentContainment(subHandleMOCParentContainmentMOC, &mocNextContainment) == MafOk);
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}


// Helper method that search for a extension in the MOM that tells that we shall decorate the IMM class with the MOM name
// This method returns true if decoration is enabled for the MOM
// This method returns false if decoration is disabled for the MOM
bool MOMRootRepository::getDecoratedRootClasses_V4(MafOamSpiMrGeneralPropertiesHandle_4T &momHandleGeneralProperties,
													MOMRootDataEntry &den,
													char* &momMOCname,
													char* &momName,
													std::string& parentRootName,
													char* &keyAttributeName)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4() : Enter");

	bool returnValue = false; // No decoration found
	char *extensionValue;
	MafReturnT maf_retval;

	maf_retval = theModelRepo_v4_p->generalProperties->getDomainExtension(momHandleGeneralProperties,
																		  const_cast<const char*>(domainEcim),
																		  const_cast<const char*>(immNameDecorationExtension),
																		  const_cast<const char**>(&extensionValue));
	if (maf_retval != MafOk)
	{
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4() : failed to get domain extension for domain: %s extension: %s",
			domainEcim, immNameDecorationExtension);
		LEAVE_OAMSA_TRANSLATIONS();
		return returnValue;
	}
	DEBUG_OAMSA_TRANSLATIONS("getDecoratedRootClasses_V4()	 getDomainExtension %s/%s for MOM  %s returns %s",
		  domainEcim, immNameDecorationExtension, momName, extensionValue);

	if (std::string(extensionValue) == std::string(decorationEnabledForMOM))
	{
		// Yes, this MOM is to be decorated
		// Get the name of the MOM (ParentName) to decorate with
		returnValue = true;
		std::string newClassName = std::string(momName) + std::string(momMOCname);
		std::string parent_rootName = parentRootName + "_" + momMOCname;
		std::string ContainmentName= parentRootName + contSeparator + momMOCname;

		//MR26712: Only add decorated Moc to map for normal scenario where splitImmDn=true.
		// This will cause Find() and IsImmRoot() to return false when writing to IMM when splitImmDn=false.
		// so the parent name will not be truncated.
		if (isSplitImmDn(ContainmentName))
		{
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 split IMM at DN, inserting prefixed root %s", newClassName.c_str());
			myMOMRootMap.insert(std::pair<std::string, MOMRootDataEntry> (newClassName, den));
		}
		else
		{
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 no split IMM at DN, not inserting root");
		}

		// Insert a entry in the map for decorated key attributes
		std::string keyAttributeNameString(keyAttributeName);
		std::string newKeyAtribute = std::string(momName) + keyAttributeNameString;
		// insert the new entry for the key attribute
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 inserting prefixed key attribute %s = %s", keyAttributeName, newKeyAtribute.c_str());
		registeredDecoratedKeyAttributes[keyAttributeNameString] = newKeyAtribute;
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 inserting prefix for root class  %s, decoration = %s", momMOCname, momName);
		registeredDecorationPrefix[std::string(momMOCname)] = std::string(momName);
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 inserting prefixImmrootName  %s, UndecoratedImmName = %s", newKeyAtribute.c_str(), keyAttributeNameString.c_str());
		registeredDecoratedImmRootClassesName[newKeyAtribute] = keyAttributeNameString;
		// Set the combination of Parent _ RootName in the map registeredParentMOCWithMOMVALUESet
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 inserting parent_rootName = %s",parent_rootName.c_str());
		OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(parent_rootName);
		momKeyRepo->setMomIsDecoratedFlag();
		if (momKeyRepo->setDecoratedKeyAttributeForClass(momMOCname,newKeyAtribute))
		{
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 add decorated key for MOM(%s) %s", parent_rootName.c_str(), newKeyAtribute.c_str());
		}
		else
		{
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 Failed to add decorated key for MOM %s", parent_rootName.c_str());
		}
		// Replace the Imm KeyAttribute search key for this MOM to the decorated one.
		if (!theKeyHolder.setMOMRepoKeyAttribute(newKeyAtribute,	parent_rootName))
		{
			ERR_OAMSA_TRANSLATIONS("MOMRootRepositor::getDecoratedRootClasses_V4y Failed to add decorated key for MOM %s with search key %s", parent_rootName.c_str(),
				  newKeyAtribute.c_str());
		}

		registeredParentMOCWithMOMVALUESet[parent_rootName] = true;
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getDecoratedRootClasses_V4 inserting newKeyAtribute = %s, with newClassName = %s",newKeyAtribute.c_str(), newClassName.c_str());
		registeredDecoratedIMMKeyAttributes[newKeyAtribute] = newClassName;
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return returnValue;
}


// Populate COM SA classes with COM models retrieved using MAF MR SPI v3
void MOMRootRepository::Populate_V4(void)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::Populate_V4 called");
	if (NULL == theModelRepo_v4_p)
	{
		ERR_OAMSA_TRANSLATIONS("MOMRootRepository::Populate_V4 called with no MR Ver.3 repository handle");
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}

	MafReturnT maf_retval ;
	MafOamSpiMrMocHandle_4T mocHandle;
	MafOamSpiMrGeneralPropertiesHandle_4T mocGpHandle;
	char* momName = NULL;
	char *mocName;

	// Start with Root Class, then iterate down
	maf_retval = theModelRepo_v4_p->entry->getRoot(&mocHandle);
	if (maf_retval != MafOk)
	{
		ERR_OAMSA_TRANSLATIONS("Populate_V4 failed to get the root MOC handle, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return ;
	}

	maf_retval = theModelRepo_v4_p->moc->getGeneralProperties(mocHandle, &mocGpHandle);
	if (maf_retval != MafOk)
	{
		ERR_OAMSA_TRANSLATIONS("Populate_V4 failed to get GeneralProperties for child class, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return ;
	}

	maf_retval = theModelRepo_v4_p->generalProperties->getStringProperty(mocGpHandle,
																		 MafOamSpiMrGeneralProperty_name_4,
																		 const_cast<const char**>(&mocName));
	if (maf_retval != MafOk)
	{
		ERR_OAMSA_TRANSLATIONS("MOMRootRepository::Populate_V4 failed to get getStringProperty for child class, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return ;
	}

	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::Populate_V4 MOM name: '%s' Top Root MOC name: '%s'", momName, mocName);

	Populate_loopRootClasses_V4(mocHandle);

	checkModelValid();

	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository : leaving Populate_V4()");
	// Dump the content of the map registeredDecoratedKeyAttributes
	MOMRootDataEntryMapIterator myIterator;
	DEBUG_OAMSA_TRANSLATIONS("Populate_V4 contains");
	for (myIterator = myMOMRootMap.begin(); myIterator != myMOMRootMap.end(); ++myIterator)
	{
		DEBUG_OAMSA_TRANSLATIONS("%s", (myIterator->first).c_str());
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return ;
}


// This recursive functon handles all root classes and for new models which have no root class
// it will instead handle all children of EcimContributions	 (i.e parent and child class are in different MOMs)
// but it will only store them internally as roots and loop over the parent containments for the parent
// containments that point to another MOM (i.e an EcimContribution).
void MOMRootRepository::Populate_loopRootClasses_V4(MafOamSpiMrMocHandle_4T mocHandle, std::string mocPath )
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 called");
	if (NULL == theModelRepo_v4_p)
	{
		ERR_OAMSA_TRANSLATIONS("MOMRootRepository::Populate_loopRootClasses_V4 called with no MR Ver.3 handle");
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}
	MafReturnT maf_retval;
	MafOamSpiMrMomHandle_4T momHandle;
	MafOamSpiMrMocHandle_4T nextMocHandle, parentMocHandle;
	MafOamSpiMrContainmentHandle_4T childContainment, nextChildContainment;
	MafOamSpiMrGeneralPropertiesHandle_4T subHandleMOCGeneralProperties;

	char* momMOCname;

	// Get the name of the	class being processed.
	maf_retval = theModelRepo_v4_p->moc->getGeneralProperties(mocHandle, &subHandleMOCGeneralProperties);
	if (maf_retval != MafOk)
	{
		// Handle error code
		ERR_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 failed to get mom root moc generalProperties, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}

	maf_retval = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleMOCGeneralProperties,
			MafOamSpiMrGeneralProperty_name_4,
			const_cast<const char**>(&momMOCname));
	if (maf_retval != MafOk)
	{
		// Handle error code
		ERR_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 failed to get mom root moc name, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return ;
	}

	DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 momMOCname [%lu] [%s]", mocHandle.handle, momMOCname);
	if(mocPath.empty()){
		mocPath = std::string(momMOCname);
	}else{
		mocPath = mocPath +  "," + std::string(momMOCname);
	}

	mapMocHandler.insert(std::pair< std::string, MafOamSpiMrMocHandle_4T>( mocPath, mocHandle));

	// Check if this is a root class to be processed
	if (IsMocChildofEcimContribution_V4(mocHandle))
	{
		// container to store the parents found for this MOM
		MOMRootDataEntry den;
		bool foundKeyAttribute = false;
		char* keyAttributeName = NULL;

		maf_retval = theModelRepo_v4_p->moc->getMom(mocHandle, &momHandle);
		if (maf_retval == MafOk)
		{
			// Get the key attribute name for the MOM
			// Iterate over the root moc's attributes
			foundKeyAttribute = getRootClassKeyAttribute_V4(keyAttributeName, mocHandle);

			// Populate the theKeyHolder with all classes and keys for the repository
			populateKeyHolderWithMOM_V4(momHandle, mocHandle);
		}
		else
		{
			// Handle error code
			ERR_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 failed to get MOM handle from MOC handle, return code %d", maf_retval);
			LEAVE_OAMSA_TRANSLATIONS();
			return ;
		}

		if (maf_retval == MafOk && foundKeyAttribute)
		{
			DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 : succeed to get handle for MOC");

			// get the name of the mom
			MafOamSpiMrGeneralPropertiesHandle_4T momHandleGeneralProperties;
			maf_retval = theModelRepo_v4_p->mom->getGeneralProperties(momHandle, &momHandleGeneralProperties);
			//maf_retval = theModelRepo_v4_p->mom->getRootMoc(momHandle, &mocHandle);
			if (maf_retval != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::Populate_loopRootClasses_V4 failed to get handle to MOMGeneralProperties , return code %d", maf_retval);
				LEAVE_OAMSA_TRANSLATIONS();
				return ;
			}
			char* momName = NULL;
			maf_retval = theModelRepo_v4_p->generalProperties->getStringProperty(momHandleGeneralProperties,
					MafOamSpiMrGeneralProperty_name_4,
					const_cast<const char**>(&momName));
			if (maf_retval != MafOk)
			{
				// Handle error code
				ERR_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 failed to get mom name, return code %d", maf_retval);
				LEAVE_OAMSA_TRANSLATIONS();
				return ;
			}
			DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4: Mom name %s", momName);

			// Loop over all containments for this MOC Up to the Global root
			MafOamSpiMrContainmentHandle_4T mocParentContainment;
			MafOamSpiMrContainmentHandle_4T mocNextContainment;

			maf_retval = theModelRepo_v4_p->moc->getParentContainment(mocHandle, &mocParentContainment);
			if (maf_retval != MafOk)
			{
				// This happens when we start with the ComTop, because it do not have any parent :-)
				DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 failed to getParentContainment for root moc, return code %d", maf_retval);
				// Insert the ManagedElement as a root without any grandparents
				DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 inserting root %s", momMOCname);
				myMOMRootMap.insert(std::pair<std::string, MOMRootDataEntry> (std::string(momMOCname), den));
			}
			else
			{
				MafOamSpiMrContainmentHandle_4T mocSiblingContainmentHandle = mocParentContainment;
				MafOamSpiMrGeneralPropertiesHandle_4T SiblingContProperties;
				char* SiblingContName = NULL;
				bool StoreECIMContributionInRootMap = false;

				while(maf_retval == MafOk)
				{
					maf_retval = theModelRepo_v4_p->containment->getParentMoc(mocParentContainment, &parentMocHandle);
					if ((maf_retval == MafOk)  && (sameMom_V4(parentMocHandle, const_cast<const char*>(momName))) )
					{
						DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4: Avoid processing this class together with parent containment pointing to a parent class in the same MOM");
					}
					else
					{
						// This is an EcimContribution with a parent in another MOM. Process this as a root class
						// The combination Parent + Root name is only added if the parent is from another MOM
						// or no parent because at the top of the model tree.
						StoreECIMContributionInRootMap = true;
					}

					// container to store the parents found for this MOM
					MOMRootDataEntry	entry;
					// Loop over the parents to the root class and add them here
					if (!loopContainmentClass_V4(mocParentContainment, mocNextContainment, entry))
					{
						DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 failed in loopContainmentClass_V4()");
					}
					// OK, add extended moc name if ECIM prefix is available in the model.
					// Get the extensions in the MOM.
					std::string parentName = entry.GetParentName();
					if (! getDecoratedRootClasses_V4(momHandleGeneralProperties, entry, momMOCname,
							momName, parentName, keyAttributeName) )
					{
						// If not prefixed, then add the expected un prefixed name here.
						DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 inserting root %s", momMOCname);
						myMOMRootMap.insert(std::pair<std::string, MOMRootDataEntry> (std::string(momMOCname), entry));
					}

					if(StoreECIMContributionInRootMap)
					{
						StoreECIMContributionInRootMap = false;
						DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4 inserting parent/root %s%s", (entry.GetParentName()).c_str(), momMOCname);
						myMOMRootMap.insert(std::pair<std::string, MOMRootDataEntry> (std::string(entry.GetParentName()+momMOCname), entry));
					}

					// Find next class by searching containments.
					maf_retval	= theModelRepo_v4_p->containment->getNextContainmentSameChildMoc(mocSiblingContainmentHandle,
							&mocSiblingContainmentHandle);
					if (maf_retval != MafOk)
					{
						DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4: Failed to getNextContainmentSameChildMoc() on child containment ");
					}
					else
					{
						// Save handle for next iteration
						mocParentContainment =	mocSiblingContainmentHandle;

						DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4: getNextContainmentSameChildMoc() on child containment OK");

						maf_retval = theModelRepo_v4_p->containment->getGeneralProperties(mocSiblingContainmentHandle,
								&SiblingContProperties);
						if (maf_retval != MafOk)
						{
							ERR_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4: Failed to get Next Containment with same Child Properties");
							LEAVE_OAMSA_TRANSLATIONS();
							return;
						}

						maf_retval = theModelRepo_v4_p->generalProperties->getStringProperty(SiblingContProperties,
								MafOamSpiMrGeneralProperty_name_4,
								const_cast<const char**>(&SiblingContName));
						if (MafOk == maf_retval)
						{
							DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4: Got Next Containment with same Child containment name [%s]", SiblingContName);
						}
					}
				}
			}
		}
	}

	// Recursively call Populate_loopRootClasses_V4() with next class.
	// First look for a child containment from this class
	// else look for the next child containment with same parent

	maf_retval = theModelRepo_v4_p->moc->getChildContainment(mocHandle, &childContainment);

	if (maf_retval != MafOk)
	{
		DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4() could not get Child containment for class %s", momMOCname);
	}
	else
	{
		// Loop over all neighboring siblings with the same parent
		while (maf_retval == MafOk)
		{
			maf_retval = theModelRepo_v4_p->containment->getChildMoc(childContainment, &nextMocHandle);

			if (maf_retval == MafOk)
			{
				//populate loop root classes  on childmoc only if parent and child moc handle are not same
				if (mocHandle.handle != nextMocHandle.handle)
				{
					Populate_loopRootClasses_V4(nextMocHandle, mocPath);
				}
				DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4()  Looking for next child containment with same parent [%s]", momMOCname);
				maf_retval = theModelRepo_v4_p->containment->getNextContainmentSameParentMoc(childContainment, &nextChildContainment);
				if (maf_retval != MafOk)
				{
					DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4() could not get next Child containment for parent class [%s]. Finished processing this class.", momMOCname);
				}
				else {
					childContainment.handle=nextChildContainment.handle;
				}
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("Populate_loopRootClasses_V4() failed to getChildMoc for next child containment with same parent");
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return ;
}


// Returns true if the mocHandle is the child of an EcimContribution and parent is from another MOM
bool MOMRootRepository::IsMocChildofEcimContribution_V4(MafOamSpiMrMocHandle_4T mocHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafReturnT maf_retval;
	MafOamSpiMrGeneralPropertiesHandle_4T subHandleMOCGeneralProperties;
	MafOamSpiMrGeneralPropertiesHandle_4T subHandleMOMGeneralProperties;
	MafOamSpiMrContainmentHandle_4T parentContainment, nextParentContainment;
	MafOamSpiMrMocHandle_4T parentMocHandle;
	MafOamSpiMrMomHandle_4T momHandle;
	char *mocName;
	char *momName;
	bool isRoot=false;

	if (0 == mocHandle.handle)
	{
		ERR_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  NULL mocHandle");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	else
	{
		DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  mocHandle not NULL");
	}

	maf_retval = theModelRepo_v4_p->moc->getGeneralProperties(mocHandle, &subHandleMOCGeneralProperties);
	if (maf_retval != MafOk)
	{
		// Handle error code
		ERR_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  failed to get GeneralProperties for child class, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	maf_retval = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleMOCGeneralProperties,
																		 MafOamSpiMrGeneralProperty_name_4,
																		 const_cast<const char**>(&mocName));
	if (maf_retval != MafOk)
	{
		// Handle error code
		ERR_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  failed to get getStringProperty for child class, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	maf_retval = theModelRepo_v4_p->moc->getBoolProperty(mocHandle, MafOamSpiMrMocBoolProperty_isRoot_4, (bool *)&isRoot);
	if (maf_retval != MafOk)
	{
		// Handle error code
		ERR_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  failed to get getBoolProperty for child class, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	maf_retval = theModelRepo_v4_p->moc->getMom(mocHandle, &momHandle);
	if (maf_retval != MafOk)
	{
		// Handle error code
		DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  failed to getMom for child class, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	maf_retval = theModelRepo_v4_p->mom->getGeneralProperties(momHandle, &subHandleMOMGeneralProperties);
	if (maf_retval != MafOk)
	{
		ERR_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  failed to getGeneralProperties for MOM, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	maf_retval = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleMOMGeneralProperties,
																		 MafOamSpiMrGeneralProperty_name_4,
																		 const_cast<const char**>(&momName));
	if (maf_retval != MafOk)
	{
		ERR_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  failed to getStringProperty for MOM, return code %d", maf_retval);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4()  MOC Name [%s] MOM Name [%s] isRoot %d", mocName, momName, isRoot);
	if (isRoot)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}

	// Check if the class is a child of an EcimContribution and has a parent in a different MOM
	maf_retval = theModelRepo_v4_p->moc->getParentContainment(mocHandle, &parentContainment);

	if (maf_retval != MafOk)
	{
		DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4() could not get Parent Containment for class %s", mocName);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	else
	{
		nextParentContainment=parentContainment;

		// Check all parents of this class by iterating across parent containments.
		while (maf_retval == MafOk)
		{
			DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4() retrieving parent Class from containment for class %s", mocName);
			// Get MOM name, compare with parent class
			maf_retval = theModelRepo_v4_p->containment->getParentMoc(parentContainment, &parentMocHandle);

			if (maf_retval == MafOk)
			{
				if(sameMom_V4(parentMocHandle, const_cast<const char*>(momName)))
				{
					//get next parent containment
					maf_retval = theModelRepo_v4_p->containment->getNextContainmentSameChildMoc(parentContainment, &nextParentContainment);
					if ((nextParentContainment.handle == parentContainment.handle) || (maf_retval != MafOk))
					{
						// Finished searching all parents, none found from another MOM.
						DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4() got the same parent containment handle or no next parent containment, returning false");
						LEAVE_OAMSA_TRANSLATIONS();
						return false;
					}
					parentContainment = nextParentContainment;
				}
				else
				{
					// we have a parent from a different MOM, this class should be stored by COM SA as a 'root'
					DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4() found parent for class %s from a different MOM - return true", mocName);
					LEAVE_OAMSA_TRANSLATIONS();
					return true;
				}
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V4() could not retrieve parent Class from containment for class %s", mocName);
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}


//#ifdef KEEP_MR_SPI_V1_AND_V2
// Returns true if the mocHandle is the child of an EcimContribution and parent is from another MOM
bool MOMRootRepository::IsMocChildofEcimContribution_V1(MafOamSpiMocT *mocHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafOamSpiContainmentT *parentContainment;
	std::string momName="";
	std::string mocName="";
	MafOamSpiContainmentT *nextParentContainment;
	MafOamSpiMomT *momHandle;

	if (NULL == mocHandle)
	{
		ERR_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V1()  NULL mocHandle");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}

	if (mocHandle->isRoot)
	{
		//DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V1()  mocHandle->isRoot true, leaving");
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}

	mocName = mocHandle->generalProperties.name;
	momHandle = mocHandle->mom;

	if (NULL == momHandle)
	{
		DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V1()  NULL momHandle");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	else
	{
		momName = momHandle->generalProperties.name;
		//DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V1() class [%s] momName [%s]", mocName.c_str(), momName.c_str());
	}


	// Check if the class is a child of an EcimContribution and has a parent in a different MOM
	parentContainment = mocHandle->parentContainment;

	if (NULL == parentContainment)
	{
		DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V1()  NULL parentContainment");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	else
	{
		nextParentContainment=parentContainment;

		// Check all parents of this class by iterating across parent containments.
		while (nextParentContainment != NULL)
		{
			MafOamSpiMomT *parentmomHandle = parentContainment->parentMoc->mom;
			if (NULL == parentmomHandle )
			{
				DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V1()  NULL parentmomHandle");
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}
			else
			{
				// Check if parent is from sameMom as momHandle
				std::string parentMomName=parentmomHandle->generalProperties.name;
				if (parentMomName != momName)
				{
					DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V1()  found parent from different MOM [%s], returning true", parentMomName.c_str());
					LEAVE_OAMSA_TRANSLATIONS();
					return true;
				}
				else
				{
					nextParentContainment = parentContainment->nextContainmentSameChildMoc;
					if ((nextParentContainment== parentContainment) || (NULL == nextParentContainment))
					{
						// Finished searching all parents, none found from another MOM.
						//DEBUG_OAMSA_TRANSLATIONS("IsMocChildofEcimContribution_V1() got the same parent containment or no next parent containment, returning false");
						LEAVE_OAMSA_TRANSLATIONS();
						return false;
					}
					parentContainment = nextParentContainment;

				}
			}
		}
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}
//#endif // KEEP_MR_SPI_V1_AND_V2



// Check if it is the same mom name
// return true if moc is in the same mom or if momName = NULL
// else return false
bool MOMRootRepository::sameMom_V4(MafOamSpiMrMocHandle_4T mocHandle, const char* momName)
{
	ENTER_OAMSA_TRANSLATIONS();
	if (momName == NULL)
	{
		DEBUG_OAMSA_TRANSLATIONS("sameMom_V4 called with no MOM name");
		LEAVE_OAMSA_TRANSLATIONS();
		return true;
	}
	else
	{
		// Check if mom is the same
		MafOamSpiMrMomHandle_4T subHandlemom;
		MafReturnT returnCode = theModelRepo_v4_p->moc->getMom(mocHandle, &subHandlemom);
		if (returnCode != MafOk)
		{
			ERR_OAMSA_TRANSLATIONS("sameMom_V4: can not get a MOM handle from MOC handle, retun code = %d", returnCode);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		MafOamSpiMrGeneralPropertiesHandle_4T subHandleGenProp;
		returnCode = theModelRepo_v4_p->mom->getGeneralProperties(subHandlemom, &subHandleGenProp);
		if (returnCode != MafOk)
		{
			ERR_OAMSA_TRANSLATIONS("sameMom_V4: can not get a MOM general properties handle, retun code = %d", returnCode);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		const char* momNameMOC = NULL;
		returnCode = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleGenProp,
																			MafOamSpiMrGeneralProperty_name_4,
																			const_cast<const char**>(&momNameMOC));
		if (returnCode != MafOk)
		{
			ERR_OAMSA_TRANSLATIONS("sameMom_V4: can not get a MOM name from general properties handle, retun code = %d", returnCode);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}

		std::string momNameString = momName;
		std::string momNameMOCString ="";
		if (momNameMOC != NULL)
		{
			momNameMOCString = momNameMOC;
			DEBUG_OAMSA_TRANSLATIONS("sameMom_V4: momName [%s] / momName from Parent MOC [%s]", momName, momNameMOC);
		}
		if (momNameMOCString == momNameString)
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
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}


bool MOMRootRepository::getMocNameAndKey_V4(MafOamSpiMrMocHandle_4T mocHandle,
											std::string& className,
											std::string& keyAttribute,
											OamSAKeyMOMRepo* momKeyRepo,
											const char* momName)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMocNameAndKey_V4 started");
	// Get the class name
	MafOamSpiMrGeneralPropertiesHandle_4T generalPropertiesMOC;
	MafReturnT returnCode = theModelRepo_v4_p->moc->getGeneralProperties(mocHandle, &generalPropertiesMOC);
	if (returnCode != MafOk)
	{
		ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getMocNameAndKey_V4 failed to get MOC general properties handle, return code: %d", returnCode);
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	if (sameMom_V4(mocHandle, momName))
	{
		char* rootClassName;
		returnCode = theModelRepo_v4_p->generalProperties->getStringProperty(generalPropertiesMOC,
																			 MafOamSpiMrGeneralProperty_name_4,
																			 const_cast<const char**>(&rootClassName));
		if (returnCode != MafOk)
		{
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getMocNameAndKey_V4 failed to get MOC string properties, return code: %d", returnCode);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		else
		{
			// Check if this belong to the MOM provided.
			className = rootClassName;
		}

		// Get the key attribute name
		char* keyAttributeName = NULL;
		if (!getRootClassKeyAttribute_V4(keyAttributeName, mocHandle))
		{
			// Hup no key attribute for class, return false
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMocNameAndKey_V4(): No key attribute for class, return false");
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		keyAttribute = keyAttributeName;

		if (!getNotifiableAttributes_V4(mocHandle, rootClassName, keyAttributeName, momKeyRepo))
		{
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMocNameAndKey_V4(): failed to get isNotifiable setting, return false");
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
	}
	else
	{
		// OK, this is not in this mom. skip this and continue
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMocNameAndKey_V4: NOT THE SAME MOM, continue...");
		LEAVE_OAMSA_TRANSLATIONS();
		return false;
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}


// MR26712: Support multiple parents when not splitting DN at EcimContribution
/* METHOD TO GET MOC parent names for a mocHandle */
bool MOMRootRepository::getMocParents_V4(MafOamSpiMrMocHandle_4T mocHandle,
										 std::list<std::string>& theMocParentsList)
{
	ENTER_OAMSA_TRANSLATIONS();
	// Get the class name
	MafOamSpiMrGeneralPropertiesHandle_4T generalPropertiesMOC;
	MafOamSpiMrContainmentHandle_4T parent;
	MafOamSpiMrContainmentHandle_4T nextParent;

	MafReturnT returnCode = theModelRepo_v4_p->moc->getParentContainment(mocHandle, &parent);
	if (returnCode != MafOk)
	{
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMocParents_V4: Found ComTop here, continue");
		theMocParentsList.push_back("");
	}
	else
	{
		while (returnCode == MafOk)
		{
			MafOamSpiMrMocHandle_4T parentMoc;
			returnCode = theModelRepo_v4_p->containment->getParentMoc(parent, &parentMoc);
			if (returnCode == MafNotExist)
			{
				//DEBUG_OAMSA_TRANSLATIONS("getMocParents : ComTop here");
				//OK ComTop here
				theMocParentsList.push_back("");
				LEAVE_OAMSA_TRANSLATIONS();
				return true;
			}
			else if (returnCode != MafOk)
			{
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getMocParents_V4: can not get parent MOC handle, return code: %d", returnCode);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}

			returnCode = theModelRepo_v4_p->moc->getGeneralProperties(parentMoc, &generalPropertiesMOC);
			if (returnCode != MafOk)
			{
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getMocParents_V4: can not get general properties handle for parent MOC, return code: %d", returnCode);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}
			char* rootParentClassName = NULL;
			returnCode = theModelRepo_v4_p->generalProperties->getStringProperty(generalPropertiesMOC,
																				 MafOamSpiMrGeneralProperty_name_4,
																				 const_cast<const char**>(&rootParentClassName));
			if (returnCode != MafOk)
			{
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getMocParents_V4: can not get name string for parent MOC, return code: %d", returnCode);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMocParents_V4: parent name: %s", rootParentClassName);
				theMocParentsList.push_back(rootParentClassName);
			}
			returnCode = theModelRepo_v4_p->containment->getNextContainmentSameChildMoc(parent, &nextParent);
			if ((returnCode == MafOk) && (nextParent.handle != parent.handle))
			{
				// New unique parent containment found, continue
				DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMocParents_V4: New unique parent containment found, continue");
				parent = nextParent;
			}
			else
			{
				// Finished searching all parents, none found
				DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMocParents_V4: Finished searching all parents, none found");
				returnCode = MafAborted;
			}
		}
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}


// Get the key attribute name for the MOM
// Iterate over the root moc's attributes
bool MOMRootRepository::getRootClassKeyAttribute_V4(char* &keyAttributeName,
													MafOamSpiMrMocHandle_4T mocHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	MafOamSpiMrAttributeHandle_4T attrHandle;
	MafOamSpiMrGeneralPropertiesHandle_4T subHandleKeyAttribute;

	// Iterate over all of the classes attributes
	MafReturnT maf_retVal = theModelRepo_v4_p->moc->getAttribute(mocHandle, &attrHandle);
	if (MafOk != maf_retVal)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getRootClassKeyAttribute_V4: can not get attribute handle for MOC, return code %d", maf_retVal);
		return false;
	}

	while (maf_retVal == MafOk)
	{
		bool isKey = false;
		maf_retVal = theModelRepo_v4_p->attribute->getBoolProperty(attrHandle,
																	MafOamSpiMrAttributeBoolProperty_isKey_4,
																	&isKey);
		if (maf_retVal != MafOk)
		{
			// OK, no key attribute found
			DEBUG_OAMSA_TRANSLATIONS("getRootClassKeyAttribute_V4 failed to get mom root moc key attribute, return code %d", maf_retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}

		if (isKey)
		{
			// This is the key attribute, get it and store the name
			maf_retVal = theModelRepo_v4_p->attribute->getGeneralProperties(attrHandle,
																			&subHandleKeyAttribute);
			if (maf_retVal == MafOk)
			{
				// Get the actual name and store it.
				maf_retVal = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleKeyAttribute,
																					 MafOamSpiMrGeneralProperty_name_4,
																					 const_cast<const char**>(&keyAttributeName));
				if (maf_retVal == MafOk)
				{
					DEBUG_OAMSA_TRANSLATIONS("getRootClassKeyAttribute_V4 key attribute name = %s", keyAttributeName);
					LEAVE_OAMSA_TRANSLATIONS();
					return true;
				}
				else
				{
					ERR_OAMSA_TRANSLATIONS("getRootClassKeyAttribute_V4 failed to get moc key attribute name, return code %d", maf_retVal);
					LEAVE_OAMSA_TRANSLATIONS();
					return false;
				}
			}
			else
			{
				// OK, no name available, make note and continue
				DEBUG_OAMSA_TRANSLATIONS("getRootClassKeyAttribute_V4 failed to get moc key attribute properties, return code %d", maf_retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}
		}
		// continue search for key attribute
		maf_retVal = theModelRepo_v4_p->attribute->getNext(attrHandle, &attrHandle);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return false;
}


// Get the attributes from the class and check if they are notifiable
// Add the attributes to the attribute storage in momKeyRepo with flag "isNotifiable"
bool MOMRootRepository::getNotifiableAttributes_V4(MafOamSpiMrMocHandle_4T mocHandle, char* className, char *keyAttributeName, OamSAKeyMOMRepo* momKeyRepo)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getNotifiableAttributes_V4 ENTER");
	MafOamSpiMrAttributeHandle_4T attrHandle;
	MafOamSpiMrGeneralPropertiesHandle_4T subHandleKeyAttribute;
	// OK, all classes shall have a create and Delete notification even if they do not have a attribute set
	// So the key attribute is per definition always notifyable independent what is said in the repository so we add it.
	if(momKeyRepo != NULL)
	{
		// only add the key attribute name attributes to the map
		momKeyRepo->addAttributeToIsNotifiableMap(className, keyAttributeName);
	}

	MafReturnT retVal = theModelRepo_v4_p->moc->getAttribute(mocHandle, &attrHandle);
	if (MafOk != retVal)
	{
		LEAVE_OAMSA_TRANSLATIONS();
		ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getNotifiableAttributes_V4: can not get attribute handle for MOC, return code %d", retVal);
		return false;
	}

	while (retVal == MafOk)
	{
		bool isNotifiable = false;
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getNotifiableAttributes_V4 getIsNotifiable");
		retVal = theModelRepo_v4_p->attribute->getBoolProperty(attrHandle,
																MafOamSpiMrAttributeBoolProperty_isNotifiable_4,
																&isNotifiable);
		if (retVal != MafOk)
		{
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::getNotifiableAttributes failed to get isNotifiable setting, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}

		retVal = theModelRepo_v4_p->attribute->getGeneralProperties(attrHandle, &subHandleKeyAttribute);
		if (retVal == MafOk)
		{
			// Get the actual attribute name and store it.
			char* attributeName = NULL;
			retVal = theModelRepo_v4_p->generalProperties->getStringProperty(subHandleKeyAttribute,
																			 MafOamSpiMrGeneralProperty_name_4,
																			 const_cast<const char**>(&attributeName));
			if (retVal == MafOk)
			{
				DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getNotifiableAttributes momKeyRepo (%p) className (%s) attributeName (%s) isNotifiable (%d)",
					  momKeyRepo, className, attributeName, isNotifiable);
				if(momKeyRepo != NULL && isNotifiable)
				{
					// only add the notifiable attributes to the map
					momKeyRepo->addAttributeToIsNotifiableMap(className, attributeName);
				}
			}
			else
			{
				DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getNotifiableAttributes failed to get moc attribute, return code %d", retVal);
				LEAVE_OAMSA_TRANSLATIONS();
				return false;
			}
		}
		else
		{
			// OK, no name available, make note and continue
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getNotifiableAttributes failed to get moc attribute properties, return code %d", retVal);
			LEAVE_OAMSA_TRANSLATIONS();
			return false;
		}
		// continue search for key attribute
		retVal = theModelRepo_v4_p->attribute->getNext(attrHandle, &attrHandle);
	}
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getNotifiableAttributes RETURN true");
	LEAVE_OAMSA_TRANSLATIONS();
	return true;
}


// Get the key attribute name for all classes in the MOM
// Then put them in the theKeyHolder
void MOMRootRepository::populateKeyHolderWithMOM_V4(MafOamSpiMrMomHandle_4T momHandle, MafOamSpiMrMocHandle_4T mocHandle)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("Enter populateKeyHolderWithMOM_V4");
	// Class name for the root class
	std::string classNameRoot;
	std::string keyAttributeRoot;
	std::list<std::string> theMocParentsList;
	std::list<std::string>::iterator parent_iterator;

	DEBUG_OAMSA_TRANSLATIONS("getMocNameAndKey: isNotifiable search step 1");
	if (!getMocNameAndKey_V4(mocHandle, classNameRoot, keyAttributeRoot)  || !getMocParents_V4(mocHandle, theMocParentsList) )
	{
		// Hup no root class, only return!!
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::populateKeyHolderWithMOM_V4 : class '%s', keyAttr: '%s', No root found returning",
			  classNameRoot.c_str(), keyAttributeRoot.c_str());
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}

	for( parent_iterator = theMocParentsList.begin(); parent_iterator != theMocParentsList.end(); ++parent_iterator)
	{
		std::string searchKey = (*parent_iterator) + "_" + classNameRoot;

		DEBUG_OAMSA_TRANSLATIONS("populateKeyHolderWithMOM_V4 : parent iterator: %s", (*parent_iterator).c_str());
		// Get the OamSAKeyMOMRepo for this MOM, and also create the MOMRepo
		if (!theKeyHolder.setMOMRepo(searchKey))
		{
			// Hup, already exists, only return!!
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::populateKeyHolderWithMOM_V4 : entry exist already");
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}
		else
		{
			// Now add the new IMM key attribute for this MOM
			if (!theKeyHolder.setMOMRepoKeyAttribute(keyAttributeRoot, searchKey))
			{
				// Hup, not working!
				ERR_OAMSA_TRANSLATIONS("MOMRootRepository::populateKeyHolderWithMOM_V4 : Failed to set Imm Key attributefor MOM");
			}
		}
		OamSAKeyMOMRepo* momKeyRepo = theKeyHolder.getMOMRepo(searchKey);
		// OK, add the root class itself to the MOM repository :-)
		momKeyRepo->setKeyAttributeForClass(classNameRoot, keyAttributeRoot);
		momKeyRepo->setRootClass(classNameRoot);
		// Get the mom name so we can check that this class is a member of this MOM
		MafOamSpiMrGeneralPropertiesHandle_4T subHandleMOMGENERALPROP;
		MafReturnT returnCode  = theModelRepo_v4_p->mom->getGeneralProperties(momHandle, &subHandleMOMGENERALPROP);
		char* momName;
		if (returnCode != MafOk)
		{
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::populateKeyHolderWithMOM_V4: getGeneralproperties failed, continue");
			LEAVE_OAMSA_TRANSLATIONS();
			return ;
		}
		returnCode	= theModelRepo_v4_p->generalProperties->getStringProperty(subHandleMOMGENERALPROP,
																			  MafOamSpiMrGeneralProperty_name_4,
																			  const_cast<const char**>(&momName));
		if (returnCode != MafOk)
		{
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::populateKeyHolderWithMOM_V4: get string failed, continue");
			LEAVE_OAMSA_TRANSLATIONS();
			return ;
		}

		// Save the MOM name in the key MOM repository
		if(!momKeyRepo->setMomName(momName))
		{
#ifndef UNIT_TEST

			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::populateKeyHolderWithMOM_V4: setMomName failed, continue");
			LEAVE_OAMSA_TRANSLATIONS();
			return ;
#endif
		}

		// process the root class of the MOM
		// Now we got the momKeyRepo, so search for isNotifiable flags, and save them to momKeyRepo
		// It is needed because the tree-traversal ("RetriveMomClassesAndKeyAttributes") will find only the elements below the root class
		DEBUG_OAMSA_TRANSLATIONS("getMocNameAndKey_V4: isNotifiable search step 2");
		if (!getMocNameAndKey_V4(mocHandle, classNameRoot, keyAttributeRoot, momKeyRepo))
		{
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::populateKeyHolderWithMOM_V4 : return false");
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}

		// Fill the repository
		RetriveMomClassesAndKeyAttributes_V4(momKeyRepo, mocHandle, classNameRoot.c_str(), momName);
		// Done with population of this MOM

		// Debug the class attribute-isNotifiable maps for the MOM.
		momKeyRepo->printClassAttributeNotifiableMap();
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return;
}


// get all classes and Key attributes for the provided mocHandle and momHandle and add them to the repo
void MOMRootRepository::RetriveMomClassesAndKeyAttributes_V4(OamSAKeyMOMRepo* momKeyRepo,
															 MafOamSpiMrMocHandle_4T mocHandle,
															 const char* previousClassName,
															 const char* momName)
{
	ENTER_OAMSA_TRANSLATIONS();
	if(!momKeyRepo )
	{
		LEAVE_OAMSA_TRANSLATIONS();
		return ;
	}
	// Get the key and class names
	std::string className;
	std::string keyAttribute;
	//std::string classNameParent; // Unused variable
	MafOamSpiMrContainmentHandle_4T child;
	MafOamSpiMrMocHandle_4T moc;

	MafReturnT returnCode  = theModelRepo_v4_p->moc->getChildContainment(mocHandle, &child);
	if(returnCode != MafOk)
	{
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::RetriveMomClassesAndKeyAttributes_V4 can not getChildContainment, return code: %d", returnCode);
	}
	else
	{
		returnCode = theModelRepo_v4_p->containment->getChildMoc(child, &moc);
		if(returnCode != MafOk)
		{
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::RetriveMomClassesAndKeyAttributes_V4 can not get a MOC handle for a child, return code: %d", returnCode);
			LEAVE_OAMSA_TRANSLATIONS();
			return;
		}

		while(returnCode == MafOk)
		{
			DEBUG_OAMSA_TRANSLATIONS("getMocNameAndKey: isNotifiable search step 3");
			if (getMocNameAndKey_V4(moc, className, keyAttribute, momKeyRepo, momName))
			{
				DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::RetriveMomClassesAndKeyAttributes_V4 Match class = %s, Match keyAttribute = %s",
				  className.c_str(), keyAttribute.c_str());
				momKeyRepo->setKeyAttributeForClass(className, keyAttribute);
				// OK, investigate this class subclasses
				if (className != std::string(previousClassName))
				{
					//DEBUG_OAMSA_TRANSLATIONS("CALL RetriveMomClassesAndKeyAttributes with class=%s, mom=%s", className.c_str(), momName);
					RetriveMomClassesAndKeyAttributes_V4(momKeyRepo, moc, className.c_str(), momName);
				}
			}
			// Continue with next containment to find next class
			returnCode	= theModelRepo_v4_p->containment->getNextContainmentSameParentMoc(child, &child);
			if (returnCode == MafOk)
			{
				returnCode	= theModelRepo_v4_p->containment->getChildMoc(child, &moc);
			}
		}
	}
	// OK, at this point we are done and can return :-)
	LEAVE_OAMSA_TRANSLATIONS();
	return;
}

/* writeSecretsToFile
 *
 * Populates Secrets data to mapMocpathToAttributes, converts data to IMM format,
 * and writes this data to "/opt/com/run/reencryptor-model-data".
 * Calls Populate_V4() if necessary.
 *
 * This function is to support input file for com-reencryptor component.
 * Hence, only to be invoked when COM receives ACTIVE assignment.
 */
void MOMRootRepository::writeSecretsToFile()
{
	ENTER_OAMSA_TRANSLATIONS();

	if (myMOMRootMap.empty()) {
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::writeSecretsToFile: myMOMRootMap is empty. calling Populate_V4()");
		Populate_V4();
	}

	// This is applicable for COM only
	if (false == getSecretAttributesFromModelRepo()) {
		ERR_OAMSA_TRANSLATIONS("MOMRootRepository::writeSecretsToFile failed to get secret attributes info");
		mapMocpathToAttributes.clear();
		return;
	}
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::writeSecretsToFile: MocPath Attrs map size: %d", static_cast<int>(mapMocpathToAttributes.size()));

	std::list<std::string> dnList;
	std::map<std::string, std::string>::iterator itMap;

	remove(REENCRYPTOR_MODEL_DATA_FILE_PATH);
	ofstream flatFile;
	flatFile.open(REENCRYPTOR_MODEL_DATA_FILE_PATH);

	if (false == flatFile.good()) {
		ERR_OAMSA_TRANSLATIONS("MOMRootRepository::writeSecretsToFile(): Unable to open Flat file for writing.");
		flatFile.close();
		mapMocpathToAttributes.clear();
		LEAVE_OAMSA_TRANSLATIONS();
		return;
	}

	for (itMap = mapMocpathToAttributes.begin(); itMap != mapMocpathToAttributes.end(); itMap++) {

		std::string immPath = getImmPath(itMap->first);
		if (immPath.empty()) {
			WARN_OAMSA_TRANSLATIONS("MOMRootRepository::writeSecretsToFile(): Unable to prepare immPath. Flat file data inconsistent.");
		}
		else {
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::writeSecretsToFile(): immPath: %s", immPath.c_str());
			flatFile << immPath << ";" << itMap->second << std::endl;
		}
	}

	flatFile.close();
	mapMocpathToAttributes.clear();
	LEAVE_OAMSA_TRANSLATIONS();
}

/* getImmPath
 *
 * Converts mocPath in 3GPP format to IMM format with support to
 * StructRef Names (Struct attributes are classes in IMM).
 *
 * Example:
 * input(mocPath):ManagedElement,SystemFunctions,SysM,Snmp,SnmpTargetV3:EcimPassword
 * return(immPath):Snmp,SnmpTargetV3,EcimPassword
 */
std::string MOMRootRepository::getImmPath(std::string mocPath)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string structName = getStructAndMocPathName(mocPath);

	std::string structPrefix;

	std::string immPath = convertToImmPath(mocPath, structPrefix);

	if (false == immPath.empty()) {

		if (false == structName.empty()) {

			immPath += "," + structPrefix + structName;
		}
	}

	LEAVE_OAMSA_TRANSLATIONS();
	return immPath;
}

/* convertToImmPath
 *
 * Converts mocPath in 3GPP format to IMM format.
 *
 * Example:
 * input(mocPath):ManagedElement,SystemFunctions,SysM,Snmp,SnmpTargetV3
 * return(immPath):Snmp,SnmpTargetV3
 */
std::string MOMRootRepository::convertToImmPath(std::string mocPath, std::string& structPrefix)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string mockImmDn = getMockImmDn(mocPath);

	if (mockImmDn.empty()) {
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::convertToImmPath(): mockImmDn is empty");
		LEAVE_OAMSA_TRANSLATIONS();
		return mockImmDn; //return empty
	}

	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::convertToImmPath(): mockImmDn: %s", mockImmDn.c_str());

	size_t pos;
	OamSAKeyMOMRepo* keyRepo = NULL;

	std::string immPath;

	do {

		if (false == immPath.empty()) {
			immPath += ",";
		}

		pos = mockImmDn.find_last_of(',');
		std::string immFragment;

		if (pos != std::string::npos) {

			immFragment = mockImmDn.substr(pos + 1);
			mockImmDn = mockImmDn.substr(0, pos);
		}
		else {
			immFragment = mockImmDn;
			mockImmDn.clear();
		}
		pos = immFragment.find_first_of('=');
		if (pos == std::string::npos) {
			//something wrong, probably. We continue anyway to give it a chance.
		}
		else {
			immFragment = immFragment.substr(0, pos);
		}

		std::string immClass;
		structPrefix = renderImmClassName(immFragment, &keyRepo, immClass);

		if(immClass.empty()) {
			ERR_OAMSA_TRANSLATIONS("MOMRootRepository::renderImmClassName() unable to get immClass");
			LEAVE_OAMSA_TRANSLATIONS();
			return immClass; //return empty
		}

		DEBUG_OAMSA_TRANSLATIONS(">>> immFragment: %s, immClass: %s", immFragment.c_str(), immClass.c_str());

		immPath += immClass;

	} while(false == mockImmDn.empty());

	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::renderImmClassName() immPath: %s", immPath.c_str());

	LEAVE_OAMSA_TRANSLATIONS();

	return immPath;
}

std::string MOMRootRepository::renderImmClassName(std::string immFragment, OamSAKeyMOMRepo** keyRepo, std::string& immClass)
{
	ENTER_OAMSA_TRANSLATIONS();
	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::renderImmClassName() immFragment: %s", immFragment.c_str());

	std::string momPrefix;

	if (keyRepo && *keyRepo) {
		//Searching for class name in the present Key Repository.
		immClass = (*keyRepo)->getClassNameForKeyAttribute(immFragment);
	}

	if (immClass.empty()) {
		//If class name not found, this might be a new root fragment.
		//Try to re-acquire the Key Repository for this new fragment.
		(*keyRepo) = theKeyHolder.getMOMRepoImmKey(immFragment);
		if (NULL == *keyRepo) {
			//If not found, then something wrong :)
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::renderImmClassName() unable to find key repository");
			LEAVE_OAMSA_TRANSLATIONS();
			return momPrefix;
		}
		else {
			immClass = (*keyRepo)->getClassNameForKeyAttribute(immFragment);
		}

		if (immClass.empty()) {
			//If class name is empty, even after retry, then return empty :)
			DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::renderImmClassName() unable to find %s in key repository", immFragment.c_str());
			LEAVE_OAMSA_TRANSLATIONS();
			return momPrefix;
		}
	}

	//Get the MOM prefix, if enabled for the MOM.
	if ((*keyRepo)->isMomDecorated()) {
		momPrefix = (*keyRepo)->getMomName();
		immClass = momPrefix + immClass;
	}

	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::renderImmClassName() immClass: %s", immClass.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return momPrefix;
}

/* getMockImmDn
 *
 * Prepares a dummy Dn from the input mocPath and attempts to
 * translate this Dn to Imm Dn and returns this Mocked Imm DN.
 *
 * Example:
 * input(mocPath)=ManagedElement,SystemFunctions,SysM,Snmp,SnmpTargetV3
 * return(immDn)=snmpTargetV3Id=0,snmpId=0
 */
std::string MOMRootRepository::getMockImmDn(std::string mocPath)
{
	ENTER_OAMSA_TRANSLATIONS();
	OamSATranslator translate;
	std::list<std::string> mocDnList;
	size_t pos;
	do {

		pos = mocPath.find_first_of(',');
		if (pos == std::string::npos) {
			mocDnList.push_back(mocPath + "=0");
		}
		else {
			mocDnList.push_back(mocPath.substr(0, pos) + "=0");
			mocPath = mocPath.substr(pos + 1);
		}

	} while(pos != std::string::npos);

	std::string immDn;
	if (false == translate.MO2Imm_DN(mocDnList, immDn)) {
		DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMockImmDn() Unable to convert to IMM dn");
		LEAVE_OAMSA_TRANSLATIONS();
		return std::string(""); //return empty
	}

	DEBUG_OAMSA_TRANSLATIONS("MOMRootRepository::getMockImmDn(): immDn: %s", immDn.c_str());
	LEAVE_OAMSA_TRANSLATIONS();
	return immDn;
}

/* getStructAndMocPathName
 *
 * Extracts and returns the structRef name from the mocPath.
 * Also trims off this struct name from the mocPath.
 *
 * Example:
 * input(mocPath): ManagedElement,SystemFunctions,SysM,Snmp,SnmpTargetV3:EcimPassword
 * mocPath=ManagedElement,SystemFunctions,SysM,Snmp,SnmpTargetV3
 * return value will be EcimPassword
 */
std::string MOMRootRepository::getStructAndMocPathName(std::string& mocPath)
{
	ENTER_OAMSA_TRANSLATIONS();
	std::string structName;
	size_t pos = mocPath.find_first_of(':');
	if (pos != std::string::npos) {

		structName = mocPath.substr(pos + 1);
		mocPath = mocPath.substr(0, pos);
	}
	LEAVE_OAMSA_TRANSLATIONS();
	return structName;
}

// fetch all MOC paths and attribute names from modelrepo
bool MOMRootRepository::getSecretAttributesFromModelRepo() {

	MafReturnT mafRetVal = MafOk;
	// iterator for mapMocHandler map
	std::map<std::string, MafOamSpiMrMocHandle_4T >::iterator mocHandleriterator = mapMocHandler.begin();

	while(mocHandleriterator != mapMocHandler.end()) {
		MafOamSpiMrMocHandle_4T mocHandle = mocHandleriterator->second;
		MafOamSpiMrAttributeHandle_4T attrHandle;
		MafOamSpiMrTypeContainerHandle_4T attrTypeContHandle;
		MafOamSpiMrType_4T attrType;

		bool ecimPasswdFound = false;
		std::string passPhraseAttrs = ""; //separated by comma
		mafRetVal = theModelRepo_v4_p->moc->getAttribute(mocHandle, &attrHandle);
		if (MafOk != mafRetVal) {
			ERR_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() failed to get attributes, return code %d", mafRetVal);
			mapMocpathToAttributes.clear();
			break;
		}
		DEBUG_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() mocPath = %s", (mocHandleriterator->first).c_str());
		//fetch the attribute values using do_while
		do {
			mafRetVal = theModelRepo_v4_p->attribute->getTypeContainer(attrHandle, &attrTypeContHandle);
			if (MafOk != mafRetVal) {
				ERR_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() failed to get type container, return code %d", mafRetVal);
				mapMocpathToAttributes.clear();
				break;
			}

			mafRetVal = theModelRepo_v4_p->typeContainer->getType(attrTypeContHandle, &attrType);
			if (MafOk != mafRetVal) {
				ERR_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() failed to get container type, return code %d", mafRetVal);
				mapMocpathToAttributes.clear();
				break;
			}
			// filter for isPassphrase from modelrepo
			if (MafOamSpiMrTypeString_4 == attrType) {
				std::string passPhraseAttr="";
				mafRetVal = getPassphraseAttrInfo(attrHandle,attrTypeContHandle,passPhraseAttr);
				if (MafOk != mafRetVal) {
					ERR_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() failed to get Passphrase attributes info, return code %d", mafRetVal);
					mapMocpathToAttributes.clear();
					break;
				}
				if(!passPhraseAttr.empty()){
					if(passPhraseAttrs.empty()) {
						passPhraseAttrs = passPhraseAttr;
						DEBUG_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() passPhraseAttrs [%s]", passPhraseAttrs.c_str());
					} else {
						passPhraseAttrs = passPhraseAttrs + "," + passPhraseAttr;
						DEBUG_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() passPhraseAttrs [%s]", passPhraseAttrs.c_str());
					}
				}
			}
			// filter for EcimPassword from modelrepo
			if ((MafOamSpiMrTypeStruct_4 == attrType) && (!ecimPasswdFound)) {
				std::string ecimPasswdAttr="";
				mafRetVal = getEcimPasswordAttrInfo(attrTypeContHandle,ecimPasswdAttr);
				if (MafOk != mafRetVal) {
					ERR_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() failed to get Ecimpassword attributes info, return code %d", mafRetVal);
					mapMocpathToAttributes.clear();
					break;
				}
				if(!ecimPasswdAttr.empty()){
					std::string mocPath = mocHandleriterator->first + ":EcimPassword";
					mapMocpathToAttributes[mocPath] = "password";
					DEBUG_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() mocpath [%s]", mocPath.c_str());
					ecimPasswdFound = true;
				}
			}
		} while (MafOk == theModelRepo_v4_p->attribute->getNext(attrHandle, &attrHandle));//attributes do_while

		if (MafOk != mafRetVal) {
			ERR_OAMSA_TRANSLATIONS("getSecretAttributesFromModelRepo() failed to get next attribute, return code %d", mafRetVal);
			mapMocpathToAttributes.clear();
			break;
		}

		if(!passPhraseAttrs.empty()){
			mapMocpathToAttributes[mocHandleriterator->first] = passPhraseAttrs;
		}
		mocHandleriterator++;
	}//MOCs while
	return (mafRetVal == MafOk);
}

MafReturnT MOMRootRepository::getPassphraseAttrInfo(MafOamSpiMrAttributeHandle_4T attrHandle, MafOamSpiMrTypeContainerHandle_4T attrTypeContHandle, std::string &passPhraseAttr)
{
	bool isPassPhrase=false;
	MafReturnT mafRetVal = MafOk;
	MafOamSpiMrGeneralPropertiesHandle_4T attrGenPropHandle;

	mafRetVal = theModelRepo_v4_p->attribute->getGeneralProperties(attrHandle, &attrGenPropHandle);
	if (MafOk != mafRetVal) {
		ERR_OAMSA_TRANSLATIONS("getPassphraseAttrInfo() failed to get attribute generalProperties, return code %d", mafRetVal);
		return mafRetVal;
	}

	mafRetVal = theModelRepo_v4_p->stringIf->getBoolProperty(attrTypeContHandle, MafOamSpiMrTypeContainerBoolProperty_isPassphrase_4, &isPassPhrase);
	if (isPassPhrase == false) {
		if(mafRetVal == MafNotExist) {
			mafRetVal = MafOk;
		}
		return mafRetVal;
	}
	char* attrName = NULL;
	mafRetVal = theModelRepo_v4_p->generalProperties->getStringProperty(attrGenPropHandle,
			MafOamSpiMrGeneralProperty_name_4,
			const_cast<const char**>(&attrName));
	if (MafOk != mafRetVal) {
		ERR_OAMSA_TRANSLATIONS("getPassphraseAttrInfo() failed to get string properties, return code %d", mafRetVal);
		return mafRetVal;
	}
	passPhraseAttr = std::string(attrName);
	DEBUG_OAMSA_TRANSLATIONS("getPassphraseAttrInfo() passPhraseAttr is :%s", passPhraseAttr.c_str());
	return mafRetVal;
}

MafReturnT MOMRootRepository::getEcimPasswordAttrInfo(MafOamSpiMrTypeContainerHandle_4T attrTypeContHandle, std::string& ecimPasswdAttr)
{
	MafReturnT mafRetVal = MafOk;
	MafOamSpiMrGeneralPropertiesHandle_4T attrTypeContGenPropHandle;

	mafRetVal = theModelRepo_v4_p->typeContainer->getGeneralProperties(attrTypeContHandle, &attrTypeContGenPropHandle);
	if (MafOk != mafRetVal) {
		ERR_OAMSA_TRANSLATIONS("getEcimPasswordAttrInfo() failed get general properties for container, return code = %d", mafRetVal);
		return mafRetVal;
	}
	char* structRefName = NULL;
	mafRetVal = theModelRepo_v4_p->generalProperties->getStringProperty(attrTypeContGenPropHandle,
			MafOamSpiMrGeneralProperty_name_4,
			const_cast<const char**>(&structRefName));
	if (MafOk != mafRetVal) {
		ERR_OAMSA_TRANSLATIONS("getEcimPasswordAttrInfo() failed to get string property, return code %d", mafRetVal);
		return mafRetVal;
	}
	if (strcmp(structRefName, "EcimPassword") == 0) {
		ecimPasswdAttr = "password";
		DEBUG_OAMSA_TRANSLATIONS("getEcimPasswordAttrInfo() filter structRef [%s] attributeName [%s]", structRefName, ecimPasswdAttr.c_str());
	}
	return mafRetVal;
}
