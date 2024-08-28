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
 *   File:	OamSARegisterObjectUtils.h
 *
 *   Author: uabjoy
 *
 *   Date:   2011-09-19
 *
 *   This file contains basic helper functions needed for OIProxy implementation
 *
 *   Modify: emilzor  2012-04-14
 *
 *   Reviewed: efaiami 2012-04-21
 *
 *   Modified: xnikvap 2012-08-30  support for COM MO SPI Ver.3 (Ver.1 is not supported any more)
 *   Modified: xjonbuc 2012-09-06  Implement SDP1694 - support MAF SPI
 *   Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
 *
 *****************************************************************************/


#ifndef OAMSAREGISTEROBJECTUTILS_H_
#define OAMSAREGISTEROBJECTUTILS_H_


/**
 * Structures used in OIProxy
 */

typedef struct RegisteredCommonStruct {
	MafMgmtSpiInterface_1T managedObjectInterfaceId;
	MafMgmtSpiInterface_1T transactionalResourceId;
} RegisteredCommonStructT;

typedef struct RegisteredClassStruct {
	MafMgmtSpiInterface_1T managedObjectInterfaceId;
	MafMgmtSpiInterface_1T transactionalResourceId;
	char *mocPath;
} RegisteredClassStructT;

typedef struct RegisteredCTClassStruct {
	MafMgmtSpiInterface_1T managedObjectInterfaceId;
	MafMgmtSpiInterface_1T transactionalResourceId;
	char *path;
} RegisteredCTClassStructT;

typedef struct RegisteredDNStruct {
	MafMgmtSpiInterface_1T managedObjectInterfaceId;
	MafMgmtSpiInterface_1T transactionalResourceId;
	MafOamSpiMocT *dnMoc;
} RegisteredDNStructT;

typedef enum RegisteredMoInterface {
	MafOamSpiManagedObject2 = 2,
	MafOamSpiManagedObject3
} RegisteredMoInterfaceT;

/**
 *	Functions for handling registered classes
 */
MafReturnT addNewRegisteredClass(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId,
								const char *className, const char *mocPath);
bool removeRegisteredClass(const char *className);
RegisteredClassStructT *getRegisteredClass(const char *className);
const char *getRegisteredClassMocPath(const char *className);
bool existRegisteredClass(const char *className);


/**
 *	Functions for handling registered DN
 */
MafReturnT addNewRegisteredDN(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId,
								const char *dn, MafOamSpiMocT *moc);
MafReturnT removeRegisteredDN(const char *dn);
RegisteredDNStructT *getRegisteredDN(const char *dn);
bool existRegisteredDN(const char *dn);


/**
 *	Functions for handling registered struct type classes
 *
 *	path can be one of next cases:
 *	path = <mocPath> ':' <attributeName>
 *	path = <dn> ':' <attributeName>
 *
 */
MafReturnT addNewRegisteredCTClass(MafMgmtSpiInterface_1T managedObjectInterfaceId, MafMgmtSpiInterface_1T transactionalResourceId,
									const char *immClassName, const char *path);
MafReturnT removeRegisteredCTClass(const char *immClassName, const char *path);
RegisteredCTClassStructT *getRegisteredCTClass(const char *immClassName, const char *path);
bool existRegisteredCTClass(const char *immClassName);
bool existRegisteredCTClassAttr(const char *immClassName, const char *path);


/**
 *	Functions for handling transactions (ccbId)
 */
MafReturnT addNewCcbTransaction(SaImmOiCcbIdT ccbId, MafOamSpiTransactionHandleT tx);
MafReturnT removeCcbTransaction(SaImmOiCcbIdT ccbId);
bool getCcbTransaction(SaImmOiCcbIdT ccbId, MafOamSpiTransactionHandleT *txHandle);
bool existCcbTransaction(SaImmOiCcbIdT ccbId);

/**
 *	Functions for taking care of multiply applier callback return values
 */
void setCcbApplierError(SaImmOiCcbIdT ccbId, SaAisErrorT err);
SaAisErrorT getCcbApplierError(SaImmOiCcbIdT ccbId);
void removeCcbApplierError(SaImmOiCcbIdT ccbId);


/**
 * Misc functions
 */
void dumpAttribute(char *attrName, MafMoAttributeValueContainer_3T *cnt);
void dumpStructAttr(char *attrName, MafMoAttributeValueContainer_3T *cnt);

SaAisErrorT getAllClassDNs(SaImmHandleT immHandle, const char *className, std::list<std::string> &immDnList);

MafOamSpiModelRepository_1T *getModelRepository();

bool parseImmDN(const char *immDn, std::string &moDn, std::string &ctAttr, int *index);
bool parsePartialImmDN(const char *immDn, std::string &moDn);
bool getCtAttrFromImmDN(const char *immDn, std::string &ctAttr, int *index);
bool createMocPath(const char *moDn, std::string &mocPath);
MafOamSpiMocT *findComMoc(MafOamSpiMocT *moc, const OamSACache::MocPathList &mocPathList);
MafOamSpiMoAttributeT *getMocAttribute(const MafOamSpiMocT *moc, const char *attrName);
void getMocComplexAttributes(MafOamSpiMocT *moc, std::vector<MafOamSpiMoAttributeT *> &attrList);
bool getFullMoDN(const char *moDn, std::string &fullMoDn);

bool getFragmentKey(std::string &fragment, std::string &key);

const char *getComplexTypeImmClass(MafOamSpiMoAttributeT *attr);


#endif /* OAMSAREGISTEROBJECTUTILS_H_ */
