#ifndef MOMROOTREPOSITORY_H_
#define MOMROOTREPOSITORY_H_
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
 *	 File:	 MOMRootRepository.h
 *
 *	 Author: xnikvap
 *
 *	 Date:	 2014-03-06
 *
 *	 This class provides data storage for the MOM information data
 *	 Separated from OamSATranslator.h
 *
 *	 Reviewed:
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 */
#include <stdlib.h>
#include <map>
#include <string>
#include <algorithm>
#include <cctype>
#include <list>
#include <stack>

#include "ComSA.h"
#include "OamSAKeyAttributeRepository.h"
#include "DxEtModelConstants.h"

#include "MafOamSpiManagedObject_3.h"
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiModelRepository_4.h"


/*
 * class MOMRootDataEntry
 * Contains data for every class which is a root in the IMM namespace.
 * These classes are either defined as root , or as the child of an EcimContribution by the ModelRepository SPI.
 * The class includes the list of parents up to the global  root "ManagedElement=1"
 */

class MOMRootDataEntry
{
public:
	typedef std::list <std::string> MOMRootParentList;
	typedef std::list <std::string>::iterator MOMRootParentListIterator;
	typedef std::list <std::string>::reverse_iterator MOMRootParentListReverseIterator;
	MOMRootDataEntry() {ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS();}
	MOMRootDataEntry(const MOMRootDataEntry& theEntry) { ENTER_OAMSA_TRANSLATIONS(); theRootList = theEntry.theRootList; LEAVE_OAMSA_TRANSLATIONS(); }
	~MOMRootDataEntry() {ENTER_OAMSA_TRANSLATIONS(); theRootList.clear(); LEAVE_OAMSA_TRANSLATIONS(); }

	// USed by Imm2MO_DN()
	MOMRootParentListReverseIterator	GetLastRoot() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return theRootList.rbegin(); }
	// Used by Imm2MO_DN()
	MOMRootParentListReverseIterator	GetReverseEnd() { ENTER_OAMSA_TRANSLATIONS(); LEAVE_OAMSA_TRANSLATIONS(); return theRootList.rend(); }
	std::string							GetParentName() { ENTER_OAMSA_TRANSLATIONS(); if (theRootList.empty()) { LEAVE_OAMSA_TRANSLATIONS(); return "";} else { LEAVE_OAMSA_TRANSLATIONS(); return *theRootList.begin(); } }

	void Insert(const std::string& theString) { ENTER_OAMSA_TRANSLATIONS(); theRootList.push_back(theString); LEAVE_OAMSA_TRANSLATIONS(); }

private:
	MOMRootParentList	theRootList;
};

/*
 * class MOMRootRepository
 * Used to store all necessary information that is read from the Model Repository SPI during Populate that is required by Translator.
 * Provides support for both COM MR SPI v2 and MAF MR SPI v3.
*  Contains the map of all class names that are defined as root, or the child of an EcimContribution, from the ModelRepository SPI.
 * Contains library functions that are used by Translator methods.
 * Contains all MOM decorations and containment extensions including splitImmDn.
 */
class MOMRootRepository
{
public:
	MOMRootRepository();
	typedef std::map <std::string, MOMRootDataEntry> MOMRootDataEntryMap;
	typedef std::map <std::string, MOMRootDataEntry>::iterator MOMRootDataEntryMapIterator;
	typedef std::pair < MOMRootDataEntryMapIterator, bool> MOMRootDataEntryMapRetVal;
	// store mocpath and mochandle in map
	typedef std::map<std::string, MafOamSpiMrMocHandle_4T > MOCPathHandler;
	// store secret values and mocpaths into map
	typedef std::map<std::string, std::string> MOCPathAttributeHandler;
	// This one searches only for the class as such, which is possible in some cases.
	// i.e. Im2MO_DN( ) since we then have the decorated name already, the class we search for is already unique.
	MOMRootDataEntryMapRetVal	Find(const std::string& theString);
	// This find method searches myMOMRootMap for Root classes
	// search key the is the concatenation of parent+root,
	// This to handle root classes that no longer are unique, i.e. MO2Imm_DN( )
	// since then we need to separate the class names by using the parent as part of the search key
	// returns true if the root is found
	bool	IsRoot(const std::string& theString,
				std::string& theParent);
	// MR26712: This find method searches myMOMRootMap for singletons only.
	// i.e Root Class where the parent containment must have splitImmDn = true,
	// Search key the is the concatenation of parent+root,
	// returns true if the root is found
	bool	IsSingletonRoot(const std::string& theString,
					std::string& theParent);
	void	GetRootSiblingList(const std::string& theParentName,
							std::list<std::string> &theSiblingList);
	// Get the key attribute for a decorated MOM
	void	FindDecoratedKeyAttribute(const std::string& keyAttribute,
									std::string& newKeyAttribute);
	// Get the decoration for a decorated MOM using a instantiation
	void	FindDecorationForMOMDN(const std::string& rootDN, const std::string& parent_rootDN,
								std::string& rootClassNameDecoration);
	// Get the original rootclass IMMDN for a decorated IMM rootclass DN
	void	FindUnDecoratedImmDN(const std::string& decoratedImmDN,
								std::string& unDecoratedImmDN);
	// Returns true if a root class for a MOM is decorated in IMM (Has the value MOM_VALUE set in the MOM)
	bool  FindIfRootMOMIsDecorated(std::string& rootClassName,
									std::string& rootParent);
	// Returns the decorated (3GPP) Root class name if found in parameter DecoratedRootClassName
	std::string&	FindDecoratedClassName(const std::string& immRootName);

	// This method stores the extension details for a containment.
	void	storeExtension(const std::string contName, const std::string extName, const std::string extValue);

	// Returns the splitImmDn flag
	bool	isSplitImmDn(const std::string parentClass_rootClass);

	// Return if the isSplitImmDn flag extension is valid
	bool	isSplitImmDnValid(const std::string parentClass_rootClass);
	bool IsMocChildofEcimContribution_V1(MafOamSpiMocT *mocHandle);

	void writeSecretsToFile();

#ifdef UNIT_TEST
	void ResetRootRepository();
#endif // UNIT_TEST



#ifdef UNIT_TEST
public:
	MOMRootDataEntryMap myMOMRootMap;
	MOCPathHandler mapMocHandler;
	MOCPathAttributeHandler mapMocpathToAttributes;
#else
private:
	MOMRootDataEntryMap myMOMRootMap;
	MOCPathHandler mapMocHandler;
	MOCPathAttributeHandler mapMocpathToAttributes;
#endif

private:

	typedef std::pair<std::string, std::string> DecoratedKeyAttributePair;
	typedef std::map<std::string, std::string>::iterator DecoratedKeyAttributeIterator;

	// Map with the imm root key attribute as search key, something like CmwTestMomcmwTestId
	// and the root class name as second attribute, something like CmwTestMomCmwTest (Notice the small and big c/C that is the difference here)
	// This map ONLY contains the imm root key attributes for the decorated classes.
	std::map<std::string, std::string> registeredDecoratedIMMKeyAttributes;
	// Map with key attributes that are decorated
	std::map<std::string, std::string> registeredDecoratedKeyAttributes;
	// Map with the root MOMs that are decorated and their decoration value
	std::map<std::string, std::string> registeredDecorationPrefix;
	// Map with the Decorated root classes as entry and the undecorated class name as second value
	std::map<std::string, std::string> registeredDecoratedImmRootClassesName;
	// A map with all Parent + _ + Rootnames that has value MOM_VALUE set, the second value is a bool only set to true
	// easy and fast to search
	std::map<std::string, bool> registeredParentMOCWithMOMVALUESet;
	// Empty string returned if no key attribute exist (to make testing easy)
	std::string emptyString;

	typedef struct {
		//value of splitImmDn for the containment.
		bool value;
		//if containment is valid or not.
		bool valid;
		//MOM name of the child MOC in the containment.
		//will be filled only if immNamespace=MOM_NAME in ECIM domain extension properties.
		std::string childMocMomName;
	} SplitImmDnAttributes;
	// A map with all containments that has value splitImmDn set
	std::map<std::string, SplitImmDnAttributes> containmentSplitImmDnAttributes;

	// These two constants control what extensions the MOMRootDataEntry::Populate method will use to enable prefixing of a MOM.
	const char* immNameDecorationExtension;
	const char* decorationEnabledForMOM;

	// known domain names. Agrred with DX ET tool and COM to use known and hardcoded nemes.
	const char* domainEcim;
	const char* domainImm;
	const char* domainCoreMW;

	// Agreed with DX ET tool to use this containment separator between class names
	const std::string contSeparator;


	// using Model Repository Ver.2
//	void Populate_V2();

	// Helper method that search for a extension in the MOM that tells that we shall decorate the IMM class with the MOM name
	bool getDecoratedRootClasses(MafOamSpiMrGeneralPropertiesHandle_4T &momHandleGeneralProperties,
								 MOMRootDataEntry &den,
								 char* &momMOCname,
								 char* &momName,
								 std::string& parentRootName,
								 char* &keyAttributeName);
	bool loopContainmentClass(MafOamSpiMrContainmentHandle_4T &mocParentContainment,
							  MafOamSpiMrContainmentHandle_4T &mocNextContainment,
							  MOMRootDataEntry	&den);
	// Get the key attribute name for the MOM
	// Iterate over the root moc's attributes
	bool getRootClassKeyAttribute(char* &keyAttributeName,
									MafOamSpiMrMocHandle_4T mocHandle);

	// Get the attributes from the class and check if they are notifiable
	// Add the attributes to the attribute storage in momKeyRepo with flag "isNotifiable"
	bool getNotifiableAttributes(MafOamSpiMrMocHandle_4T mocHandle, char* className,
								 char *keyAttributeName, OamSAKeyMOMRepo* momKeyRepo);

	// Get the key attribute name for all classes in the MOM
	// Then put them in the theKeyHolder
	void populateKeyHolderWithMOM(MafOamSpiMrMomHandle_4T momHandle, MafOamSpiMrMocHandle_4T mocHandle);
	/* METHOD TO GET ClassName and key attribute for a mocHandle */
	bool getMocNameAndKey(MafOamSpiMrMocHandle_4T mocHandle,
								std::string& className,
								std::string& keyAttribute,
								OamSAKeyMOMRepo* momKeyRepo = NULL,
								const char* momName = NULL);

	/* METHOD TO GET full list of Parents for a mocHandle */
	 bool getMocParents(MafOamSpiMrMocHandle_4T mocHandle,
						std::list<std::string>& theMocParentsList);

	// Check if it is the same mom name
	// return true if moc is in the same mom or if momName = NULL
	// else return false
	bool sameMom(MafOamSpiMrMocHandle_4T mocHandle, const char* momName);
	// Recursive method to get all classes and their key attributes in a mom
	// and add them to the OamSAKeyMOMRepo instance
	void RetriveMomClassesAndKeyAttributes(OamSAKeyMOMRepo* momKeyRepo,
										   MafOamSpiMrMocHandle_4T mocHandle,
											const char* previousClassName,
											const char* momName);

	// This method checks if the model is valid.
	void checkModelValid();
	// The method extracts the parent and child names from a containment
	bool extractContainment(const std::string containment, std::string &parent, std::string &child);

	   // *************************
	   // NEW SUPPORT FUNCTIONS TO USE MAF MR SPI V3 DURING POPULATE
	   // *************************
		void Populate_V4();
		void Populate_loopRootClasses_V4(MafOamSpiMrMocHandle_4T mocHandle, std::string mocPath = "");

	// Helper method that search for a extension in the MOM that tells that we shall decorate the IMM class with the MOM name
	bool getDecoratedRootClasses_V4(MafOamSpiMrGeneralPropertiesHandle_4T &momHandleGeneralProperties,
								MOMRootDataEntry &den,
								char* &momMOCname,
								char* &momName,
								std::string& parentRootName,
								char* &keyAttributeName);
	bool loopContainmentClass_V4(MafOamSpiMrContainmentHandle_4T &mocParentContainment,
								MafOamSpiMrContainmentHandle_4T &mocNextContainment,
								MOMRootDataEntry	&den);
	// Get the key attribute name for the MOM
	// Iterate over the root moc's attributes
	bool getRootClassKeyAttribute_V4(char* &keyAttributeName,
									MafOamSpiMrMocHandle_4T mocHandle);

	// Get the attributes from the class and check if they are notifiable
	// Add the attributes to the attribute storage in momKeyRepo with flag "isNotifiable"
	bool getNotifiableAttributes_V4(MafOamSpiMrMocHandle_4T mocHandle, char* className,
								 char *keyAttributeName, OamSAKeyMOMRepo* momKeyRepo);

	// Get the key attribute name for all classes in the MOM
	// Then put them in the theKeyHolder
	void populateKeyHolderWithMOM_V4(MafOamSpiMrMomHandle_4T momHandle, MafOamSpiMrMocHandle_4T mocHandle);
	/* METHOD TO GET ClassName and key attribute for a mocHandle */
	bool getMocNameAndKey_V4(MafOamSpiMrMocHandle_4T mocHandle,
								std::string& className,
								std::string& keyAttribute,
								OamSAKeyMOMRepo* momKeyRepo = NULL,
								const char* momName = NULL);

	/* METHOD TO GET full list of Parents for a mocHandle */
	 bool getMocParents_V4(MafOamSpiMrMocHandle_4T mocHandle,
						std::list<std::string>& theMocParentsList);

	// Check if it is the same mom name
	// return true if moc is in the same mom or if momName = NULL
	// else return false
	bool sameMom_V4(MafOamSpiMrMocHandle_4T mocHandle, const char* momName);

	// Recursive method to get all classes and their key attributes in a mom
	// and add them to the OamSAKeyMOMRepo instance
	void RetriveMomClassesAndKeyAttributes_V4(OamSAKeyMOMRepo* momKeyRepo,
											MafOamSpiMrMocHandle_4T mocHandle,
											const char* previousClassName,
											const char* momName);
#ifdef UNIT_TEST
public:
#endif
	bool IsMocChildofEcimContribution_V4(MafOamSpiMrMocHandle_4T mocHandle);

        void checkContainmentValid_V4(char * contName, MafOamSpiMrContainmentHandle_4T &mocParentContainment);
	// collect and store information about all classes from model repository
	void Populate();

	std::string getImmPath(std::string mocPath);
	std::string getStructAndMocPathName(std::string& mocPath);
	std::string getMockImmDn(std::string mocPath);
	std::string convertToImmPath(std::string mocPath, std::string& structPrefix);
	std::string renderImmClassName(std::string immFragment, OamSAKeyMOMRepo** keyRepo, std::string& immClassName);
	bool getSecretAttributesFromModelRepo();
	MafReturnT getPassphraseAttrInfo(MafOamSpiMrAttributeHandle_4T attrHandle, MafOamSpiMrTypeContainerHandle_4T attrTypeContHandle, std::string &passPhraseAttr);
	MafReturnT getEcimPasswordAttrInfo(MafOamSpiMrTypeContainerHandle_4T attrTypeContHandle, std::string &ecimPasswdAttr);

};


#endif // MOMROOTREPOSITORY_H_
