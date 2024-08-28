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
 *   File:
 *
 *   Author: uabjoy
 *
 *   Date: 2011-09-19
 *
 *   This file implements the Object Implementer inside ComSA which shall forward
 *   IMM callbacks to their mapped counterparts in the COM OIs
 *
 *   Modify: emilzor  2012-02-14
 *
 *   efaiami 2012-04-21
 *
 *   uabjoy	 2012-11-20 Changed so the method imm2MO_DN is used.
 *
 *   eaparob, eniatan 2013-12-17: correction added for "unregisterComplexTypeAttrClasses" method
 *
 *   Modified: xadaleg 2014-08-02  MR35347 - increase DN length
 *   Modified: xnikvap 2015-08-02  MR42277 - Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *   Modified: xthabui 2015-08-05  MR36067 - Improved OI/SPI
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <list>
#include "MafMgmtSpiCommon.h"
#include "MafMgmtSpiInterface_1.h"
#include "MafOamSpiModelRepository_1.h"
#include "MafOamSpiRegisterObjectImplementer_2.h"
#include "MafOamSpiRegisterObjectImplementer_3.h"
#include "MafOamSpiServiceIdentities_1.h"
#include "ComSA.h"
#include "OamSACache.h"
#include "OamSARegisterObjectImplementer.h"
#include "OamSATranslator.h"
#include "imm_utils.h"
#include "trace.h"
#include "OamSARegisterObjectUtils.h"
#include "OamSAOIProxy.h"


extern SaImmHandleT immOmHandle;
extern SaImmOiHandleT immOiHandle;
extern SaImmOiHandleT immOiApplierHandle;
// Handle to the object access API
extern SaImmAccessorHandleT accessorHandleOI;

extern MafOamSpiModelRepository_1T* theModelRepo_v1_p;

extern OamSATranslator theTranslator;

extern MafMgmtSpiInterfacePortal_3_1T* portal_3_1;

MafReturnT registerClass_2(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                           MafMgmtSpiInterface_1T transactionalResourceId,
                           const char *mocPath);

MafReturnT registerDn_2(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                        MafMgmtSpiInterface_1T transactionalResourceId,
                        const char *dn);

MafReturnT unregisterClass_2(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                             MafMgmtSpiInterface_1T transactionalResourceId,
                             const char *mocPath);

MafReturnT unregisterDn_2(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                          MafMgmtSpiInterface_1T transactionalResourceId,
                          const char *dn);

MafReturnT registerClass(const char * componentName,
									const char *mocPath);

MafReturnT registerDn(const char * componentName,
								const char *dn);

MafReturnT unregisterClass(const char * componentName,
										const char *mocPath);

MafReturnT unregisterDn(const char * componentName,
									const char *dn);

//initialize the struct's members
static MafOamSpiRegisterObjectImplementer_2T InterfaceStruct_2 = {
	MafOamSpiRegisterObjectImplementer_2Id,	//base
	registerClass_2,
	registerDn_2,
	unregisterClass_2,
	unregisterDn_2
};

//initialize the struct's members
static MafOamSpiRegisterObjectImplementer_3T InterfaceStruct = {
	MafOamSpiRegisterObjectImplementer_3Id,	//base
	registerClass,
	registerDn,
	unregisterClass,
	unregisterDn
};

//This function is used for releasing memory
static void releaseResource(MafMgmtSpiInterface_1T ***result_MO, MafMgmtSpiInterface_1T ***result_TxR);

/**
 *  Returns a pointer to a struct describing the interface and containing the
 *  function pointer to use in calling the services
 *
 *  @return pointer to a struct of type MafOamSpiRegisterObjectImplementer_2T
 */
MafOamSpiRegisterObjectImplementer_2T* ExportOamSARegisterObjectImplementerInterface_2(void)
{
	ENTER_OIPROXY();
	LEAVE_OIPROXY();
	return &InterfaceStruct_2;
}

/**
 *  Returns a pointer to a struct describing the interface and containing the
 *  function pointer to use in calling the services
 *
 *  @return pointer to a struct of type MafOamSpiRegisterObjectImplementer_3T
 */
MafOamSpiRegisterObjectImplementer_3T* ExportOamSARegisterObjectImplementerInterface(void)
{
	ENTER_OIPROXY();
	LEAVE_OIPROXY();
	return &InterfaceStruct;
}

/**
 *  Returns true  if the IMM class type is runtime
 *  Returns false if the IMM class type is config
 *
 */
bool isRuntimeClass(const char *className)
{
	ENTER_OIPROXY();
	DEBUG_OIPROXY("isRuntimeClass(): enter with className(%s)",className);
	bool runtimeClass = false;
	SaAisErrorT immRet = SA_AIS_OK;
	SaImmClassCategoryT classCat = SA_IMM_CLASS_CONFIG;
	SaImmAttrDefinitionT_2 **attrDef = NULL;

	// Get class description from IMM
	immRet = autoRetry_saImmOmClassDescriptionGet_2(immOmHandle, (const SaImmClassNameT)className, &classCat, &attrDef);

	if (SA_AIS_ERR_BAD_HANDLE == immRet)
	{
		LOG_OIPROXY("saImmOmClassDescriptionGet_2() returned BAD_HANDLE, reinitializing OI handlers before another attempt");
		ObjImp_finalize_imm(true,false);
		ObjImp_init_imm(true);

		immRet = autoRetry_saImmOmClassDescriptionGet_2(immOmHandle, (const SaImmClassNameT)className, &classCat, &attrDef);
	}

	if(immRet != SA_AIS_OK)
	{
		DEBUG_OIPROXY("isRuntimeClass(): Cannot get class (%s) description retVal = (%d), returning false", className, immRet);
		LEAVE_OIPROXY();
		return false;
	}

	if(!attrDef)
	{
		DEBUG_OIPROXY("isRuntimeClass(): attrDef is NULL, returning false");
		LEAVE_OIPROXY();
		return false;
	}
	if(classCat == SA_IMM_CLASS_RUNTIME)
	{
		runtimeClass = true;
	}

	// Class description memory free
	immRet = saImmOmClassDescriptionMemoryFree_2(immOmHandle, attrDef);
	if (immRet != SA_AIS_OK)
	{
		DEBUG_OIPROXY("isRuntimeClass(): saImmOmClassDescriptionMemoryFree_2() failed retVal = (%d)",(int)immRet);
		ERR_OIPROXY("saImmOmClassDescriptionMemoryFree_2() failed retVal = %d",(int)immRet);
	}
	DEBUG_OIPROXY("isRuntimeClass(): return with (%d)",runtimeClass);
	LEAVE_OIPROXY();
	return runtimeClass;
}

/**
 * Register complex type attributes.
 * For each registration of DN or class, listeners for create and modification
 * will be set for their complex type attributes to be able to follow
 * complex type attributes changes.
 *
 * @param	managedObjectInterface	the ManagedObject interface id
 * @param	transactionalResource	the TransactionalResource interface id.
 * @param	mocDnPath				the complete path from the root and down to
 * 			the MOC the OI will represent. Example: /Me/ApplX/SomeMoc
 * 			will be handled by this OI
 * @param	attrList				list of MOC complex type attributes
 * 			which will be monitored for create/modification
 * @return	MafOk if the operation succeeded, otherwise one of the other
 * 			MafReturnT error codes
 */
MafReturnT registerComplexTypeAttrClasses(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId,
											const char *mocDnPath, std::vector<MafOamSpiMoAttributeT *> &attrList) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("registerComplexTypeAttrClasses(): enter with mocDnPath(%s)", mocDnPath);
	MafReturnT ret = MafOk;
	const char *immClassName = NULL;
	std::string path;
	//	For each attribute...
	for(size_t i=0; i<attrList.size(); i++)
	{
		//	Get IMM class name from MOC attribute struct
		immClassName = getComplexTypeImmClass(attrList[i]);
		// Handle runtime classes here
		if(isRuntimeClass(immClassName))
		{
			DEBUG_OIPROXY("registerComplexTypeAttrClasses():    %d. immClassName(%s) class type in IMM: runtime",(int)i, immClassName);
		}
		// Handle config classes here
		else
		{
			DEBUG_OIPROXY("registerComplexTypeAttrClasses():    %d. immClassName(%s) class type in IMM: config",(int)i, immClassName);
			path = mocDnPath;
			path.append(":").append(attrList[i]->generalProperties.name);
			DEBUG_OIPROXY("registerComplexTypeAttrClasses():    %d. immClassName(%s) path(%s)",(int)i, immClassName, path.c_str());
			//	Check if the attribute has already been registered
			if(existRegisteredCTClass(immClassName))
			{
				if(existRegisteredCTClassAttr(immClassName, path.c_str()))
				{
					//	Unexpected error occurred, don't register anymore
					ret = MafAlreadyExist;
					break;
				}
				else
				{
					//	Class is already registered for this complex type, but for some other attribute
					//	So, the new complex type class will be added to the list, but not be registered in the IMM
					addNewRegisteredCTClass(managedObjectInterfaceId, transactionalResourceId, (char *)immClassName, path.c_str());
				}
			}
			else
			{
				// Complex type class is not registered, so do it now.
				DEBUG_OIPROXY("registerComplexTypeAttrClasses(): call autoRetry_saImmOiClassImplementerSet(%s)",immClassName);
				SaAisErrorT retCode = autoRetry_saImmOiClassImplementerSet(immOiApplierHandle, (char *)immClassName);
				DEBUG_OIPROXY("registerComplexTypeAttrClasses(): autoRetry_saImmOiClassImplementerSet returned %d",retCode);
				if(retCode != SA_AIS_OK) {
					//	Unexpected error occurred, don't register anymore
					ret = MafNotExist;
					break;
				}

				//	Check if the complex type attribute is already registered,
				//	and add it to the map if it isn't
				if(!existRegisteredCTClassAttr(immClassName, path.c_str()))
				{
					addNewRegisteredCTClass(managedObjectInterfaceId, transactionalResourceId, (char *)immClassName, path.c_str());
				}
			}
		}
	}
	LEAVE_OIPROXY();
	return ret;
}

/**
 * Unregister complex type attributes that have been registered before for
 * registerDn or registerClass.
 *
 * @param	mocDnPath				the complete path from the root and down to
 * 			the MOC the OI will represent. Example: /Me/ApplX/SomeMoc
 * 			will be handled by this OI
 * @param	attrList				list of complex type attributes
 * 			which will be monitored for create/modification
 * @return	MafOk if the operation succeeded, otherwise one of the other
 * 			MafReturnT error codes
 */
MafReturnT unregisterComplexTypeAttrClasses(const char *mocDnpath, std::vector<MafOamSpiMoAttributeT *> &attrList) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("unregisterComplexTypeAttrClasses(): ENTER with mocDnpath(%s)", mocDnpath);
	MafReturnT ret = MafOk;
	const char *immClassName;
	std::string path;
	//	For each attribute...
	for(size_t i=0; i<attrList.size(); i++)
	{
		//	Get IMM class name from MOC attribute struct
		immClassName = getComplexTypeImmClass(attrList[i]);
		DEBUG_OIPROXY("unregisterComplexTypeAttrClasses():    immClassName(%s)",immClassName);

		// If the class is a runtime class then do nothing, because runtime classes are not registered.
		if(isRuntimeClass(immClassName))
		{
			DEBUG_OIPROXY("unregisterComplexTypeAttrClasses():    immClassName(%s) class type in IMM: runtime", immClassName);
		}
		// if the class is a config class then remove it from the map and try to unregister
		else
		{
			path = mocDnpath;
			path.append(":").append(attrList[i]->generalProperties.name);
			DEBUG_OIPROXY("unregisterComplexTypeAttrClasses():    path(%s)",path.c_str());

			//	Remove the complex type attribute class from the map
			removeRegisteredCTClass(immClassName, path.c_str());

			//	If there is no any other registered complex type class, then unregister class
			if(!existRegisteredCTClass(immClassName))
			{

				if(autoRetry_saImmOiClassImplementerRelease(immOiApplierHandle, (char *)immClassName) != SA_AIS_OK)
				{
					DEBUG_OIPROXY("unregisterComplexTypeAttrClasses(): Cannot release complex type class implementer: setting ret to ComFailure");
					ERR_OIPROXY("Cannot release complex type class implementer (%s -> %s (%s))", (char *)mocDnpath, attrList[i]->generalProperties.name, (char *)immClassName);
					ret = MafFailure;
				}
			}
		}
	}
	DEBUG_OIPROXY("unregisterComplexTypeAttrClasses(): RETURN %d",ret);
	LEAVE_OIPROXY();
	return ret;
}

MafReturnT registerClass_2(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                           MafMgmtSpiInterface_1T transactionalResourceId,
                           const char *mocPath)
{

	//	Check input parameters
	if(0 == strcmp(mocPath, "\0") || !mocPath)
	{
		ERR_OIPROXY("ERROR: mocPath is not valid");
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	//	Get model repository
	DEBUG_OIPROXY("registerClass: calling getModelRepository()");
	MafOamSpiModelRepository_1T *modelRepository = getModelRepository();
	if(!modelRepository) {
		ERR_OIPROXY("ERROR: Cannot find model repository");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Get MOC root element
	MafOamSpiMocT *mocRoot = NULL;
	if(modelRepository->getTreeRoot(&mocRoot) != MafOk) {
		ERR_OIPROXY("ERROR: Cannot get MO tree root");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Split MOC path into fragments and store them into the list
	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	//	Find class MOC element
	MafOamSpiMocT *classMoc = findComMoc(mocRoot, mocPathList);
	if(!classMoc) {
		ERR_OIPROXY("ERROR: Cannot find com moc");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Create class name from MOC path fragments
	//	That's the last element in the list :)
	std::string className(*(mocPathList.rbegin()));
	#ifdef UNIT_TEST
		std::string immClassName = className;
	#else
		std::string immClassName = theTranslator.TransformImmClassName(mocPathList, (*mocPathList.rbegin()));
	#endif

	//	Check if the class has already been registered
	if(existRegisteredClass(className.c_str())) {
		ERR_OIPROXY("ERROR: Class %s has already been registered", className.c_str());
		LEAVE_OIPROXY();
		return MafAlreadyExist;
	}

	//	Check for the exclusion for DN <-> Class registration
	std::list<std::string>::iterator it;
	std::list<std::string> immDnList;
	OamSACache::DNList dnList;
	std::string fullMoDn;

	//	Take all existing objects that are the same class as the one we want to register
	getAllClassDNs(immOmHandle, className.c_str(), immDnList);
	for(it = immDnList.begin(); it != immDnList.end(); it++) {
		//	Clear all lists
		dnList.clear();
		fullMoDn.clear();

		//	Get full MO dn
		GlobalSplitDN(*it, dnList);
		theTranslator.Imm2MO_DN(0, dnList, fullMoDn);

		//	Check if the the object is already registered
		if(existRegisteredDN(fullMoDn.c_str())) {
			ERR_OIPROXY("ERROR: Exclusion between registerDn and registerClass. An object with the same class has already been registered with registerDN (%s)" , fullMoDn.c_str());
			LEAVE_OIPROXY();
			return MafFailure;
		}
	}

	// Now register the class and its complex type attributes
	MafReturnT ret = MafOk;
	SaAisErrorT retCode = autoRetry_saImmOiClassImplementerSet(immOiHandle, (char*)immClassName.c_str());
	if(retCode != SA_AIS_OK) {
		ERR_OIPROXY("ERROR: Cannot set class implementer for '%s'", className.c_str());
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	//	Find all complex type attributes
	std::vector<MafOamSpiMoAttributeT *> attrList;
	getMocComplexAttributes(classMoc, attrList);

	//	Register all complex type attributes to be able to get callbacks for them
	ret = registerComplexTypeAttrClasses(managedObjectInterfaceId, transactionalResourceId, mocPath, attrList);

	//	If something went wrong... clean what was done
	if(ret != MafOk) {
		if(autoRetry_saImmOiClassImplementerRelease(immOiHandle, (char *)immClassName.c_str()) != SA_AIS_OK)
			ERR_OIPROXY("Cannot release class implementer (%s)", (char *)className.c_str());

		ret = unregisterComplexTypeAttrClasses(mocPath, attrList);
	} else	//	At the end register class (add class name to the map
		addNewRegisteredClass(managedObjectInterfaceId, transactionalResourceId, className.c_str(), mocPath);

	DEBUG_OIPROXY("registerClass(%s) exists with code error: %d", mocPath, ret);
	LEAVE_OIPROXY();

	return ret;
}

MafReturnT registerDn_2(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                        MafMgmtSpiInterface_1T transactionalResourceId,
                        const char *dn)
{

	//	Check input parameters
	if(0 == strcmp(dn , "\0") || !dn) {
		ERR_OIPROXY("ERROR: dn is not valid");
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	//	Check if MO dn has already been registered
	if(existRegisteredDN(dn)) {
		ERR_OIPROXY("ERROR: '%s' has already been registered", dn);
		LEAVE_OIPROXY();
		return MafAlreadyExist;
	}

	//	Split MO dn into DN fragments, and store them into the list
	OamSACache::DNList moName;
	GlobalSplitDN(dn, moName);

	//	Get MOC element of the MO element
	MafOamSpiMocT *moc = theTranslator.GetComMoc(moName);
	if(!moc) {
		ERR_OIPROXY("ERROR: Cannot find moc for '%s'", dn);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Translate MO dn into IMM dn
	std::string immDn;
	theTranslator.MO2Imm_DN(moName, immDn);
	if(immDn.length() > saNameMaxLen()) {
		ERR_OIPROXY("ERROR: IMM DN is longer than the limitation (%u chars) length('%s') = %u", saNameMaxLen(), immDn.c_str(), (unsigned int)immDn.length());
		LEAVE_OIPROXY();
		return MafFailure;
	}

	SaNameT objectDn;
	saNameSet(immDn.c_str(), &objectDn);

	//	Set object implmenter to IMM dn object
	SaAisErrorT errCode;
	if((errCode = autoRetry_saImmOiObjectImplementerSet(immOiHandle, &objectDn, SA_IMM_ONE)) != SA_AIS_OK) {
		if(errCode == SA_AIS_ERR_NOT_EXIST) {
			saNameDelete(&objectDn, false);
			ERR_OIPROXY("ERROR: Failed to set object implementer to '%s'. The object does not exist", immDn.c_str());
			LEAVE_OIPROXY();
			return MafNotExist;
		} else {
			saNameDelete(&objectDn, false);
			ERR_OIPROXY("ERROR: Failed to set object implementer to '%s' (%d)", immDn.c_str(), (int)errCode);
			LEAVE_OIPROXY();
			return MafFailure;
		}
	}

	//	Find all complex type attributes
	std::vector<MafOamSpiMoAttributeT *> attrList;
	getMocComplexAttributes(moc, attrList);

	//	Register all complex type attributes to be able to get callbacks for them
	MafReturnT ret = registerComplexTypeAttrClasses(managedObjectInterfaceId, transactionalResourceId, dn, attrList);

	//	If something went wrong... clean what was done
	if(ret != MafOk) {
		if(autoRetry_saImmOiObjectImplementerRelease(immOiHandle, &objectDn, SA_IMM_ONE) != SA_AIS_OK)
			ERR_OIPROXY("Cannot release object implementer (%s -> %s)", dn, (char *)immDn.c_str());

		//	Unregister all complex type attributes that are registered
		unregisterComplexTypeAttrClasses(dn, attrList);
	} else	// At the end register object DN
		addNewRegisteredDN(managedObjectInterfaceId, transactionalResourceId, dn, moc);

	saNameDelete(&objectDn, false);
	return ret;
}

MafReturnT unregisterClass_2(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                             MafMgmtSpiInterface_1T transactionalResourceId,
                             const char *mocPath)
{

	//	Check input parameters
	if(0 == strcmp(mocPath, "\0") || !mocPath)
	{
		ERR_OIPROXY("ERROR: mocPath is invalid");
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	//	Get model repository
	DEBUG_OIPROXY("unregisterClass: calling getModelRepository()");
	MafOamSpiModelRepository_1T *modelRepository = getModelRepository();
	if(!modelRepository) {
		ERR_OIPROXY("ERROR: Cannot find model repository");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Get MOC root element
	MafOamSpiMocT *mocRoot = NULL;
	if(modelRepository->getTreeRoot(&mocRoot) != MafOk) {
		ERR_OIPROXY("ERROR: Cannot get MO tree root");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Split MOC path into fragments and store them into a list
	OamSACache::MocPathList mocPathList;
	GlobalSplitMocPath(mocPath, mocPathList);

	//	Find MOC element of the MOC path
	MafOamSpiMocT *classMoc = findComMoc(mocRoot, mocPathList);
	if(!classMoc) {
		ERR_OIPROXY("ERROR: Cannot find moc for %s", mocPath);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Create class name (last element in MOC path fragments)
	std::string className(*(mocPathList.rbegin()));
	#ifdef UNIT_TEST
		std::string immClassName = className;
	#else
		std::string immClassName = theTranslator.TransformImmClassName(mocPathList, (*mocPathList.rbegin()));
	#endif

	//	Class has already been registered
	if(!existRegisteredClass(className.c_str())) {
		ERR_OIPROXY("ERROR: '%s' has not been registered", className.c_str());
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	//	Find registered class data
	RegisteredClassStructT *regClass = getRegisteredClass(className.c_str());
	if(!regClass) {
		ERR_OIPROXY("ERROR: Failed to getRegisteredClass()");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Compare if managed object interface match with registered managed object interface
	if(strcmp(managedObjectInterfaceId.componentName, regClass->managedObjectInterfaceId.componentName) ||
			strcmp(managedObjectInterfaceId.interfaceName, regClass->managedObjectInterfaceId.interfaceName) ||
			strcmp(managedObjectInterfaceId.interfaceVersion, regClass->managedObjectInterfaceId.interfaceVersion)) {
		ERR_OIPROXY("ERROR: Other component cannot unregister the class");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Find all complex type attributes
	std::vector<MafOamSpiMoAttributeT *> attrList;
	getMocComplexAttributes(classMoc, attrList);

	//	Release class implementer from IMM class
	if(autoRetry_saImmOiClassImplementerRelease(immOiHandle, (char *)immClassName.c_str()) != SA_AIS_OK) {
		ERR_OIPROXY("ERROR: Cannot release class implementer (%s)", (char *)className.c_str());
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Remove the class from the map
	removeRegisteredClass(className.c_str());

	//	Remove all class implementers from complex type attributes
	MafReturnT ret = unregisterComplexTypeAttrClasses(mocPath, attrList);

	return ret;
}

MafReturnT unregisterDn_2(MafMgmtSpiInterface_1T managedObjectInterfaceId,
                          MafMgmtSpiInterface_1T transactionalResourceId,
                          const char *dn)
{

	//	Check input parameters
	if(0 == strcmp(dn, "\0") || !dn) {
		ERR_OIPROXY("ERROR: dn is invalid");
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	DEBUG_OIPROXY("unregisterDn(): Check if the MO dn is registered");
	//	Check if the MO dn is registered
	if(!existRegisteredDN(dn)) {
		ERR_OIPROXY("ERROR: '%s' has not been registered", dn);
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	DEBUG_OIPROXY("unregisterDn(): Get registered DN data");
	//	Get registered DN data
	RegisteredDNStructT *regDn = getRegisteredDN(dn);
	if(!regDn) {
		ERR_OIPROXY("ERROR: Failed to getRegisteredDN()");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	DEBUG_OIPROXY("unregisterDn(): Compare if managed object interface match with registered managed object interface");
	//	Compare if managed object interface match with registered managed object interface
	if(strcmp(managedObjectInterfaceId.componentName, regDn->managedObjectInterfaceId.componentName) ||
			strcmp(managedObjectInterfaceId.interfaceName, regDn->managedObjectInterfaceId.interfaceName) ||
			strcmp(managedObjectInterfaceId.interfaceVersion, regDn->managedObjectInterfaceId.interfaceVersion)) {
		ERR_OIPROXY("ERROR: Other component cannot unregister the DN");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	DEBUG_OIPROXY("unregisterDn(): Split MO name into fragments and store them into a list");
	//	Split MO name into fragments and store them into a list
	OamSACache::DNList moName;
	GlobalSplitDN(dn, moName);

	DEBUG_OIPROXY("unregisterDn(): Find IMM dn from MO dn");
	//	Find IMM dn from MO dn
	std::string immDn;
	theTranslator.MO2Imm_DN(moName, immDn);
	if(immDn.length() > saNameMaxLen()) {
		ERR_OIPROXY("ERROR: IMM DN is too long(%u is allowed). length('%s') = %u", saNameMaxLen(), immDn.c_str(), (unsigned int)immDn.length());
		LEAVE_OIPROXY();
		return MafFailure;
	}

	DEBUG_OIPROXY("unregisterDn(): Get complex type attributes");
	//	Get complex type attributes
	std::vector<MafOamSpiMoAttributeT *> attrList;
	getMocComplexAttributes(regDn->dnMoc, attrList);

	SaNameT objectDn;
	saNameSet(immDn.c_str(), &objectDn);

	DEBUG_OIPROXY("unregisterDn(): call autoRetry_saImmOiObjectImplementerRelease(%s)",immDn.c_str());
	//	If the object is deleted, then the implementer is removed, and SA_AIS_ERR_NOT_EXIST will be returned
	SaAisErrorT errCode = autoRetry_saImmOiObjectImplementerRelease(immOiHandle, &objectDn, SA_IMM_ONE);
	DEBUG_OIPROXY("unregisterDn(): autoRetry_saImmOiObjectImplementerRelease returned %d", errCode);
	if(errCode != SA_AIS_OK && errCode != SA_AIS_ERR_NOT_EXIST) {
		ERR_OIPROXY("ERROR: Cannot release object implementer (%s -> %s)", dn, immDn.c_str());
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Remove MO dn from the map
	removeRegisteredDN(dn);

	DEBUG_OIPROXY("unregisterDn(): Unregister complex type attributes");
	//	Unregister complex type attributes
	MafReturnT ret = unregisterComplexTypeAttrClasses(dn, attrList);


	return ret;
}

/**
 * Register as object implementer for a specific MO class.
 *
 * @param	componentName	the Component Name of the Object Implementer
 * 			that the SA shall use to fetch the ManagedObject and
 * 			TransactionalResource interfaces from the Portal.
 * @param	mocPath		the complete path from the root and down to
 *			the MOC the OI will represent. Example: /Me/ApplX/SomeMoc
 * 			The path is needed for the OI to resolve ambiguous MOC names.
 * @return	MafOk if the  the operation succeeded and the OamSa is
 * 			responsible for this mocPath, MafNotExist if the OamSa doesn't
 * 			handle this mocPath, MafAlreadyExist if the object is already
 * 			registered, otherwise one of the other
 * 			MafReturnT error codes
 */
MafReturnT registerClass(const char * componentName, const char *mocPath) {
	ENTER_OIPROXY();
	ERR_OIPROXY("registerClass(): enter with componentName(%s) mocPath(%s)", componentName, mocPath);

	/* Validate the componentName */
	if(0 == strcmp(componentName, "\0") || !componentName)
	{
		ERR_OIPROXY("ERROR: componenName is invalid");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Check input parameters
	if(0 == strcmp(mocPath, "\0") || !mocPath)
	{
		ERR_OIPROXY("ERROR: mocPath is not valid");
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	/*
	 * MR-36067
	 */
	/* This variable is used for storing the interface array of register components */
	MafMgmtSpiInterface_1T ***result_MO = (MafMgmtSpiInterface_1T ***) malloc(sizeof(MafMgmtSpiInterface_1T **));
	MafMgmtSpiInterface_1T ***result_TxR = (MafMgmtSpiInterface_1T ***) malloc(sizeof(MafMgmtSpiInterface_1T **));

	if(!result_MO || !result_TxR)
	{
		ERR_OIPROXY("ERROR: Cannot allocate memory for results");
		releaseResource(result_MO, result_TxR);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	MafReturnT retval_MO = portal_3_1->getComponentInterfaceArray(componentName, "MafOamSpiManagedObject", result_MO);
	MafReturnT retval_TxR = portal_3_1->getComponentInterfaceArray(componentName, "MafOamSpiTransactionalResource", result_TxR);
	if(MafOk != retval_MO || MafOk != retval_TxR)
	{
		releaseResource(result_MO, result_TxR);
		if(MafOk != retval_MO)
		{
			ERR_OIPROXY("ERROR: getComponentInterfaceArray returns: %d", (int)retval_MO);
			LEAVE_OIPROXY();
			return retval_MO;
		}
		else
		{
			ERR_OIPROXY("ERROR: getComponentInterfaceArray returns: %d", (int)retval_TxR);
			LEAVE_OIPROXY();
			return retval_TxR;
		}
	}

	if((!*result_MO) || (!*result_TxR))
	{
		ERR_OIPROXY("ERROR: Cannot get Component Interface");
		releaseResource(result_MO, result_TxR);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//Get the interfaceName and interfaceVersion of managedObjectInterfaceId
	MafMgmtSpiInterface_1T **array = *result_MO;
	MafMgmtSpiInterface_1T managedObjectInterfaceId = {componentName, "MafOamSpiManagedObject", array[0]->interfaceVersion};

	//Get the interfaceName and interfaceVersion of transactionalResourceId
	array = *result_TxR;
	MafMgmtSpiInterface_1T transactionalResourceId = {componentName, "MafOamSpiTransactionalResource", array[0]->interfaceVersion};

	MafReturnT ret = registerClass_2(managedObjectInterfaceId, transactionalResourceId, mocPath);

	array = NULL;
	releaseResource(result_MO, result_TxR);

	return ret;
}

/**
 * Register for a particular Mo instance
 * that the Sa will forward calls to from the middleware.
 *
 * @param	componentName	the Component Name of the Object Implementer
 * 			that the SA shall use to fetch the ManagedObject and
 * 			TransactionalResource interfaces from the Portal.
 * @param	dn 						the dn in 3GPP format
 * @return	MafOk if the operation succeeded and the OamSa is
 * 			responsible for this dn, ComNotExist if the OamSa doesn't
 * 			handle this dn, otherwise one of the other
 * 			MafReturnT error codes
 */
MafReturnT registerDn(const char * componentName, const char *dn) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("registerDn(): enter with componentName(%s) dn(%s)", componentName, dn);

	/* Validate the componentName */
	if(0 == strcmp(componentName, "\0") || !componentName)
	{
		ERR_OIPROXY("ERROR: componenName is invalid");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Check input parameters
	if(0 == strcmp(dn , "\0") || !dn) {
		ERR_OIPROXY("ERROR: dn is not valid");
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	//	Check if MO dn has already been registered
	if(existRegisteredDN(dn)) {
		ERR_OIPROXY("ERROR: '%s' has already been registered", dn);
		LEAVE_OIPROXY();
		return MafAlreadyExist;
	}

	/*
	 *	MR-36067
	 */

	/* This variable is used for storing the interface array of register components */
	MafMgmtSpiInterface_1T ***result_MO = (MafMgmtSpiInterface_1T ***) malloc(sizeof(MafMgmtSpiInterface_1T **));
	MafMgmtSpiInterface_1T ***result_TxR = (MafMgmtSpiInterface_1T ***) malloc(sizeof(MafMgmtSpiInterface_1T **));

	if(!result_MO || !result_TxR)
	{
		ERR_OIPROXY("ERROR: Cannot allocate memory for results");
		releaseResource(result_MO, result_TxR);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	MafReturnT retval_MO = portal_3_1->getComponentInterfaceArray(componentName, "MafOamSpiManagedObject", result_MO);
	MafReturnT retval_TxR = portal_3_1->getComponentInterfaceArray(componentName, "MafOamSpiTransactionalResource", result_TxR);
	if(MafOk != retval_MO || MafOk != retval_TxR)
	{
		releaseResource(result_MO, result_TxR);
		if(MafOk != retval_MO)
		{
			ERR_OIPROXY("ERROR: getComponentInterfaceArray returns: %d", (int)retval_MO);
			LEAVE_OIPROXY();
			return retval_MO;
		}
		else
		{
			ERR_OIPROXY("ERROR: getComponentInterfaceArray returns: %d", (int)retval_TxR);
			LEAVE_OIPROXY();
			return retval_TxR;
		}
	}

	if((!*result_MO) || (!*result_TxR))
	{
		ERR_OIPROXY("ERROR: Cannot get Component Interface");
		releaseResource(result_MO, result_TxR);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//Get the interfaceName and interfaceVersion of managedObjectInterfaceId
	MafMgmtSpiInterface_1T **array = *result_MO;
	MafMgmtSpiInterface_1T managedObjectInterfaceId = {componentName, "MafOamSpiManagedObject", array[0]->interfaceVersion};

	//Get the interfaceName and interfaceVersion of transactionalResourceId
	array = *result_TxR;
	MafMgmtSpiInterface_1T transactionalResourceId = {componentName, "MafOamSpiTransactionalResource", array[0]->interfaceVersion};

	MafReturnT ret = registerDn_2(managedObjectInterfaceId, transactionalResourceId, dn);

	//	Deallocate memory
	array = NULL;
	releaseResource(result_MO, result_TxR);

	DEBUG_OIPROXY("registerDn(%s) exits with code error: %d", dn, ret);
	LEAVE_OIPROXY();
	return ret;
}

/**
 * Unregister as object implementer for a specific MO class.
 *
 * @param	componentName	the Component Name of the Object Implementer
 * 			that the SA shall use to fetch the ManagedObject and
 * 			TransactionalResource interfaces from the Portal.
 * @param	mocPath					the complete path from the root and down to
 * 			the MOC the OI will represent. Example: /Me/ApplX/SomeMoc
 * 			will be handled by this OI
 * @return	MafOk if the operation succeeded, ComNotExist if the class is not
 * 			registered, otherwise one of the other
 * 			MafReturnT error codes
 */
MafReturnT unregisterClass(const char * componentName, const char *mocPath) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("unregisterClass(): enter with componentName(%s) dn(%s)", componentName, mocPath);

	/* Validate the componentName */
	if(0 == strcmp(componentName, "\0") || !componentName)
	{
		ERR_OIPROXY("ERROR: componenName is invalid");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//	Check input parameters
	if(0 == strcmp(mocPath, "\0") || !mocPath)
	{
		ERR_OIPROXY("ERROR: mocPath is invalid");
		LEAVE_OIPROXY();
		return MafNotExist;
	}

	/*
	 * MR-36067
	 */
	/* This variable is used for storing the interface array of register components */
	MafMgmtSpiInterface_1T ***result_MO = (MafMgmtSpiInterface_1T ***) malloc(sizeof(MafMgmtSpiInterface_1T **));

	if(!result_MO)
	{
		ERR_OIPROXY("ERROR: Cannot allocate memory for results");
		releaseResource(result_MO, NULL);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	MafReturnT retval_MO = portal_3_1->getComponentInterfaceArray(componentName, "MafOamSpiManagedObject", result_MO);
	if(MafOk != retval_MO)
	{
		releaseResource(result_MO, NULL);
		ERR_OIPROXY("ERROR: getComponentInterfaceArray returns: %d", (int)retval_MO);
		LEAVE_OIPROXY();
		return retval_MO;
	}

	if(!*result_MO)
	{
		ERR_OIPROXY("ERROR: Cannot get Component Interface");
		releaseResource(result_MO, NULL);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//Get the interfaceName and interfaceVersion of managedObjectInterfaceId
	MafMgmtSpiInterface_1T **array = *result_MO;
	MafMgmtSpiInterface_1T managedObjectInterfaceId = {componentName, "MafOamSpiManagedObject", array[0]->interfaceVersion};

	MafReturnT ret = unregisterClass_2(managedObjectInterfaceId, managedObjectInterfaceId, mocPath);

	//	Deallocate memory
	array = NULL;
	releaseResource(result_MO, NULL);
	DEBUG_OIPROXY("unregisterClass(%s) exists with code error: %d", mocPath, ret);

	LEAVE_OIPROXY();
	return ret;
}

/**
 * Unregister for a particular Mo instance
 *
 * @param	componentName	the Component Name of the Object Implementer
 * 			that the SA shall use to fetch the ManagedObject and
 * 			TransactionalResource interfaces from the Portal.
 * @param	dn 						the dn in 3GPP format
 * @return	MafOk if the operation succeeded, otherwise one of the other
 * 			MafReturnT error codes
 */
MafReturnT unregisterDn(const char * componentName, const char *dn) {
	ENTER_OIPROXY();
	DEBUG_OIPROXY("unregisterDn(): enter with componentName(%s) dn(%s)", componentName, dn);

	/* Validate the componentName */
	if(0 == strcmp(componentName, "\0") || !componentName)
	{
		ERR_OIPROXY("ERROR: componenName is invalid");
		LEAVE_OIPROXY();
		return MafFailure;
	}

	/*
	 *	MR-36067
	 */
	/* This variable is used for storing the interface array of register components */
	MafMgmtSpiInterface_1T ***result_MO = (MafMgmtSpiInterface_1T ***) malloc(sizeof(MafMgmtSpiInterface_1T **));
	if(!result_MO)
	{
		ERR_OIPROXY("ERROR: Cannot allocate memory for results");
		releaseResource(result_MO, NULL);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	MafReturnT retval_MO = portal_3_1->getComponentInterfaceArray(componentName, "MafOamSpiManagedObject", result_MO);
	if(MafOk != retval_MO)
	{
		releaseResource(result_MO, NULL);
		ERR_OIPROXY("ERROR: getComponentInterfaceArray returns: %d", (int)retval_MO);
		LEAVE_OIPROXY();
		return retval_MO;
	}

	if(!*result_MO)
	{
		ERR_OIPROXY("ERROR: Cannot get Component Interface");
		releaseResource(result_MO, NULL);
		LEAVE_OIPROXY();
		return MafFailure;
	}

	//Get the interfaceName and interfaceVersion of managedObjectInterfaceId
	MafMgmtSpiInterface_1T **array = *result_MO;
	MafMgmtSpiInterface_1T managedObjectInterfaceId = {componentName, "MafOamSpiManagedObject", array[0]->interfaceVersion};

	MafReturnT ret = unregisterDn_2(managedObjectInterfaceId, managedObjectInterfaceId, dn);
	//	Deallocate memory
	array = NULL;
	releaseResource(result_MO, NULL);

	DEBUG_OIPROXY("unregisterDn(%s) RETURN %d", dn, ret);
	LEAVE_OIPROXY();
	return ret;
}

static void releaseResource(MafMgmtSpiInterface_1T ***result_MO, MafMgmtSpiInterface_1T ***result_TxR)
{
	if(result_MO)
	{
		if(*result_MO)
		{
			free(*result_MO);
			*result_MO = NULL;
		}
		free(result_MO);
		result_MO = NULL;
	}

	if(result_TxR)
	{
		if(*result_TxR)
		{
			free(*result_TxR);
			*result_TxR = NULL;
		}
		free(result_TxR);
		result_TxR = NULL;
	}
}
