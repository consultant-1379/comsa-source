#ifndef OAMSATRANSLATOR_H_
#define OAMSATRANSLATOR_H_
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
 *	 File:	 OamSATranslator.h
 *
 *	 Author: egorped
 *
 *	 Date:	 2010-05-21
 *
 *	 This file declares translations between Imm and MO.
 *
 *	 Reviewed: efaiami 2010-07-05
 *
 *	 Reviewed: efaiami 2011-01-26  Com_SA Action
 *
 *	 Reviewed: efaiami 2012-03-26  sanityCheckAdminParameters()
 *
 *	 Modified: uabjoy  2012-04-10  Added support methods to handle decorated IMM class names.
 *	 Modified: uabjoy  2012-05-30  Added method Imm2MO_DNRoot to support OIProxy code.
 *	 Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *	 Modified: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *	 Modified: uabjoy  2012-09-21  Correction of TR HQ23564
 *	 Modified: eaparob 2013-01-28  Added function "getNotifiableAttributes" to class "MOMRootRepository"
 *	 Modified: eaparob 2013-01-31  Added function "isNotified" to class OamSATranslator,
 *	 Modified: xduncao 2013-05-16  Implemented support for any return type from action()
 *	 Modified: xadaleg 2013-07-16  MR26712 - Support option to not split IMM DN at EcimContribution
 *	 Modifies: uabjoy  2013-08-26  Correcting trouble report HR51069
 *	 Modified: xngangu 2014-02-19  Fix HS30458 conflict immhandle/sub_immhandle between alarm notify and register OI class thread while node lock/unlock
 *	 Modified: xjonbuc 2014-03-01  Implement MR29333 No root classes (including use of MAF MR SPI V3)
 *	 Modified: xnikvap 2014-03-06  classes classMOMRootDataEntry and MOMRootRepository taken out in separate file
 */

#include <map>
#include <string>
#include <algorithm>
#include <cctype>
#include <list>
#include <stack>

#include "saImm.h"
#include "imm_utils.h"

#include "OamSAKeyAttributeRepository.h"
#include "OamSACache.h"
#include "MOMRootRepository.h"

#include "OamSATransactionRepository.h"
#include "ImmCmd.h"
#include "TxContext.h"
#include "OamSAImmBridge.h"

#include "ComSA.h"

extern SaImmAccessorHandleT accessorHandleOI;


class OamSATranslator
{
public:
	OamSATranslator();
	~OamSATranslator();
	// imm2MO_AttrValue - converts an imm data value to a MO value
	bool Imm2MO_AttrValue(MafOamSpiTransactionHandleT txHandle,
						  const SaImmAttrValuesT_2* attr,
						  MafOamSpiMoAttributeType_3 attrType,
						  MafMoAttributeValueContainer_3T** result);
	bool MO2Imm_DN(OamSACache::DNList& mo_name, std::string& imm_name);
	bool MO2Imm_DN(const char* mo_name, char** imm_name);
	bool Imm2MO_DN(MafOamSpiTransactionHandleT txHandle,
				   OamSACache::DNList& imm_name,
				   std::string& n3gpp_name);
	bool Imm2MO_DN(MafOamSpiTransactionHandleT txHandle,
				   const char* imm_name,
				   char** n3gpp_name);

	// This method builds the DN for a provided immRDN
	std::string ImmRdn2MOTop(const std::string immRdn);

	void BuildObjectName(const std::string& ParentName,
						 const std::string& ClassNameMOM,
						 const std::string& KeyAttributeName,
						 const std::string& KeyAttributeValue,
						 std::string&		ObjectName);

	std::string RemoveClassPart(const std::string& theDn);

	char* BuildAttributeValue(const char* theAttributeName, const char* theAttributeValue);

	MafOamSpiMoAttributeType_3 GetComAttributeType(OamSACache::DNList& mo_name, const std::string& AttrName);

	void GetRootLevelChildren(const std::string& ParentName, std::list<std::string>& theRootChildrenList);

	bool IsImmRoot(const std::string& className, const std::string& undecoratedclassName, const std::string &parentDn);


	std::string Convert3GPPNameFragmentToImmNameFragment(const std::string& the3GPPString);
	// This method handles the correct translation of 3GPP by looking up in the internal database.
	std::string Convert3GPPNameFragmentToImmNameFragment(const std::string& the3GPPString,
														 OamSAKeyMOMRepo* momRepository);

	std::string ConvertImmNameFragmentTo3GPPNameFragment(const std::string& theImmString);
	std::string ConvertImmNameFragmentTo3GPPNameFragment(MafOamSpiTransactionHandleT txHandle,
														 const std::string& theImmString,
														 const std::string objectName);
	std::string ConvertImmNameFragmentTo3GPPNameFragmentWoMomPre(MafOamSpiTransactionHandleT txHandle,
																 const std::string& theImmString,
																 const std::string objectName);

	bool isExclusiveStruct (const std::string& dn, const char* structName);
	bool isStructAttribute(OamSACache::DNList& dn, const std::string& attributeName);
	std::string findAttributeTypeName(OamSACache::DNList& dn, const std::string& attributeName);
	std::list<std::string> getStructMembers(OamSACache::DNList& dn, const std::string& attributeName);
	bool getStructMemberComDatatype(OamSACache::DNList& dn, const std::string& structAttributeName, const std::string& structMemberName, MafOamSpiMoAttributeType_3T* res);
	MafOamSpiMoAttributeT* GetComAttributeList(OamSACache::DNList& mo_name);

	SaImmValueTypeT ConvertComAttributeTypeToImmType(MafOamSpiMoAttributeType_3T type);

	MafOamSpiMocT* GetComMoc(OamSACache::DNList& mo_name);
	MafOamSpiMomT* GetComMom(OamSACache::DNList& mo_name);
	MafOamSpiMoActionT* GetComActionList(OamSACache::DNList& mo_name);
	MafOamSpiMoActionT* GetComAction(OamSACache::DNList& mo_name, const std::string &actionName);

	MafReturnT	ConvertToAdminOpParameters(MafOamSpiMoActionT				*theAction_p,
										   MafMoAttributeValueContainer_3T **theComParameterList_pp,
										   SaImmAdminOperationParamsT_2	***theAdminOpParams_ppp);

	// SDP872: Convert OIProxy moIf->action() return parameters from COM to IMM format
	MafReturnT	ConvertActionReturnToImmParameters(MafOamSpiMoActionT				*theAction_p,
												   MafMoAttributeValueContainer_3T **theComParameterList_pp,
												   SaImmAdminOperationParamsT_2	***theAdminOpParams_ppp,
												   bool isMoSpiVersion2 = false);

	// SDP872 convert admin operation single or multivalue result to COM MO Attribute value container
	MafReturnT	ConvertSimpleToMoAttribute(MafOamSpiTransactionHandleT txHandle,
										   MafOamSpiDatatypeContainerT		*returnType_p,
										   SaImmAdminOperationParamsT_2		**theAdminOpParams_pp,
										   MafMoAttributeValueContainer_3T	**theResult_pp,
										   char *errorText);

	// SDP872 convert admin operation struct result to COM MO Attribute value container
	MafReturnT	HandleStructToMoAttribute(MafOamSpiTransactionHandleT	   txHandle,
										  MafOamSpiDatatypeContainerT	   *returnType_p,
										  SaImmAdminOperationParamsT_2	   **theAdminOpParams_pp,
										  MafMoAttributeValueContainer_3T  **theResult_pp,
										  char *errorText);

#define ERR_STR_BUF_SIZE 255 // max size for the error strins we create here
	std::string TransformImmClassName(OamSACache::DNList& ParentName,
									  const std::string& ClassName);

	// Method used by the MDF code to check if a specific Class exists in the MORepository
	bool IsClassNamePresent(const std::string& ClassName);
	/**
	 *	getImmKeyAttribute -
	 *	return the key attribute name for the class. This can be prefixed or not.
	 **/
	std::string getImmKeyAttribute(const char *parentClass,
								   const char *className,
								   const char *keyAttributeName);
	/**
	 *	isImmKeyAttribute -
	 *	return the key attribute name for the class. This can be prefixed or not.
	 **/
	bool isImmKeyAttribute(const char *parentClass,
						   const char *className,
						   const char *AttributeName);

	// Method to retrieve the AttributeType of a attribute in the MoRepository of a particular
	// object (in 3gpp DN format).
	// return true if type found else false
	bool RetrieveComAttributeType(const char *the3gppDN, const char *attributeName,
								 MafOamSpiMoAttributeType_3 &attrType);
#ifdef UNIT_TEST
	void ResetMOMRoot();
#endif

	// method to check if the class or the attribute under the class is notified
	bool isNotified(const char *immRdn, const char *className, const char *attrName = NULL);
	std::string nextDn(std::string& immRdnHead);
	// Get Class name for a immRdn, using the key repo
	// Used in the CMEvent handling
	bool GetClassName(const char* objectName,
					  std::string& className, std::string& comclassName);//, std::string& comkeyAttributeName);
	// Read the value of a struct object in IMM and convert it to a MafValueContainer
	bool  fillStructMembers(MafMoAttributeValueContainer_3T **structAttributePointer,
							char *immRdn, char *saAttributeName);
	// Get the Name of a struct attribute
	std::string getStructName(char *immRdn, char *saAttributeName);
	// Check if the DN list is valid
	bool IsDnListValid(OamSACache::DNList& imm_name, const char *className);

	// Get the class name for a IMM RDN
	bool GetClassNameFromImm(const std::string&	 objectName, std::string& className, SaImmHandleT immHandle, SaImmAccessorHandleT accessorHandle = accessorHandleOI);
	MafReturnT checkObjectExistInImm(const std::string& objectName, SaImmHandleT immHandle, SaImmAccessorHandleT accessorHandle);

	// for bad handlers
	SaAisErrorT reInitializeBadHandler(SaImmHandleT immOmHandle, SaImmAccessorHandleT immOmAccessorHandle);

private:
	// Helper method to handle error case for imm name fragment translation in method
	// ConvertImmNameFragmentTo3GPPNameFragment
	std::string ConvertImmNameFragmentTo3GPPNameFragmentErrorCase(const std::string& theImmString);
	// Check if the Class is Struct or not
	bool isStructClassNotifiable(const char *immRdn, const char *attrName, OamSAKeyMOMRepo* tempRepo);
	// QUICK FIX OF STRING PROBLEM FOUND 4 DAYS BEFORE PRA COMSA3.2
	// FIXME : REWRITE THIS FROM SCRATCH AS SOON AS POSSIBLE
	std::string addTwoString(std::string stringOne, std::string stringTwo);
	// Helper to Convert MafMoAttributeValue_3T values to MafMoAttributeValue_3T
	void convertToMafValue(MafOamSpiMoAttributeType_3T type,
						   MafMoAttributeValue_3T *valuesM,
						   MafMoAttributeValue_3T *valuesC);

	bool initializeImmHandle();
	void finalizeImmHandle();
	bool initImmHandleForTxhandleZero(SaImmHandleT *immOmHandleTxZero, SaImmAccessorHandleT *OamSAaccessorHandleTxZero);
	void finalizeImmHandleForTxhandleZero(SaImmHandleT immOmHandleTxZero, SaImmAccessorHandleT OamSAaccessorHandleTxZero);
	// Retrive the matching struct attribute for this immRdn
	std::string getStructRefName(std::string& stringimmRdn, std::string& parentClassName,
								 std::string& comParenClassName, const char *attributeName = NULL);
	// Break this out to a separate function
	SaImmHandleT immOmHandleLocal;
	// Handle to the object access API
	SaImmAccessorHandleT OamSAaccessorHandleT;

	bool getImmAttributeSimpleValue(const char *immRdn, const char *attributeName,
									MafOamSpiMoAttributeType_3T attrType,
									MafMoNamedAttributeValueContainer_3T **structAttributePointer);

	std::string ImmBuilder(const std::string immRdn, OamSAKeyMOMRepo* tempRepo);
	// Helper method that retrieves everything before the "="
	std::string retriveKey(const std::string nextpiece);
	// Helper method that retrieves everything efter the "="
	std::string retiveValue(const std::string nextpiece);

	// HElper method retrieving the last piece of a immRdn and removes it.
	std::string nextFragment(std::string& immRdnHead);

	//Creates the DN from CompTop down to the root class.
	std::string retriveMoTop(MOMRootRepository::MOMRootDataEntryMapRetVal theMOMRetVal);

	// Convert all letters to lower case
	std::string ConvertToLowerCase(std::string word)
	{
		std::transform(word.begin(), word.end(), word.begin(), ::tolower); // scope resolution operator ::
		return word;
	}


	// This method search the MOM repository to find the root class and the MOM data
	// for the mom that list mo_name points to.
	void GetComMocAndMom(OamSACache::DNList mo_name,
						 MafOamSpiMocT** theMoc_pp,
						 MafOamSpiMomT** theMom_pp);

	void GetComMocAndMom_loopRootClasses(MafOamSpiMocT* parentMoc,
										 OamSACache::DNList mo_name,
										 const std::string parentName,
										 const std::string className,
										 MafOamSpiMocT** theMoc_pp,
										 MafOamSpiMomT** theMom_pp);

	typedef std::list<SaImmAdminOperationParamsT_2*> OamSATransAdmOpParList;
	typedef OamSATransAdmOpParList::iterator		OamSATransAdmOpParListIter;

	MafReturnT HandleStructToAdminOpPar(const std::string& theStructName,
										MafOamSpiStructMemberT* member_p,
										OamSATransAdmOpParList& theAdmOpParList,
										MafMoAttributeValue_3T* theComParameterValue_p,
										bool isMoSpiVersion2 = false);

	SaImmAdminOperationParamsT_2* ConvertASingleAdmOpParam(const std::string& theParamName,
														   MafOamSpiDatatypeContainerT* parameterType_p,
														   MafMoAttributeValueContainer_3T* AttrValC_p,
														   int i,
														   bool isMoSpiVersion2 = false);

	typedef std::map < std::string, SaImmAdminOperationParamsT_2**> StructMemberMap;
	typedef std::list<StructMemberMap> StructMemberMapList;
	typedef std::map < std::string, SaImmAdminOperationParamsT_2**>::iterator StructMemberMapIterator;
	typedef std::list<StructMemberMap>::iterator StructMemberMapListIterator;

	void insert_nameList(std::list<std::string>& memberNameList, std::string& paramNameStr);
	MafMoAttributeValueStructMember_3T* create_structMember(MafOamSpiTransactionHandleT txHandle, MafOamSpiStructMemberT* member_p, StructMemberMap& structMemberMap, MafReturnT& RetVal, char* errorText);
	SaImmAdminOperationParamsT_2** create_tempParam(SaImmAdminOperationParamsT_2 **adminOpParams_pp, std::string& name);
	// Method that check that the storage type for the provided parameter list are correct.
	bool sanityCheckAdminParameters(MafOamSpiParameterT* par_p,
									MafOamSpiDatatypeT type);

	bool sanityCheckReturnComParameters(MafOamSpiDatatypeContainerT returnParameters_p,
										MafOamSpiDatatypeT type,
										bool isMoSpiVersion2 = false);

	// method to check if this root class needs to be decorated to access the class in the IMM
	bool isDecorated(std::string theRootName, std::string theParent);

	// method that converts a decorated imm dn to the name format known by the MOM
	bool unDecorateRootDn(std::string& immRootDn, std::string& originalImmRootDn);

	// Method that removes the equals sign and the . in a MOM DN name so it becomes a class name.
	std::string& convertDnToClass(std::string& dnName);

	// Retrive the name of the parent for a root class in the MOM
	std::list<std::string> getParentName(MafOamSpiMocT* rootClass);

	// Helper function that fix a 3GPP DN so it do not contain any '.' and '='signs
	void removeEqualSignAndDotPartOfDN(std::string &theString);

	// Method that builds a correct imm object path of a serie of object fragments
	std::string buildImmObjectPath(std::string objectPath, std::string objectFragment);
};


#endif

