/*
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
 *   File:   ImmCmd.h
 *
 *   Author: efaiami & egorped (actually most of it nicked from Karl-Magnus who inherited it from.......)
 *
 *   Date:   2010-05-21
 *
 *   This file implements command patterns towards Imm.
 *
 *   Reviewed: efaiami 2010-06-10
 *
 *   Reviewed: efaiami 2011-01-26  Com_SA Action
 *  Modify: efaiami 2011-02-22 for log and trace function
 *  Modify: xnikvap 2013-01-22 SDP875 support
 *  Modify: xjonbuc 2014-06-23  Implement MR-20275 support for explicit ccb validate() & abort()
 *
 ************************************************************************************************/

#ifndef __OAMSA_IMMCMD_H
#define __OAMSA_IMMCMD_H
#include <vector>
#include <string>
#include "saAis.h"
#ifndef SaStringT
typedef char * SaStringT;
#endif
#define IMM_VER 11
#include "saImm.h"
#include "saImmOm.h"
#include "TxContext.h"
#include "trace.h"

class BridgeImmIterator;

namespace CM {

// base class for imm "cmd's"
class ImmCmd {

public:
	std::vector<SaNameT*> mTmpDns;  // temp holder for SaNameTs
	TxContext * mTxContextIn;
	std::string mCmd;
	static SaVersionT mVersion;
	int mRetries;
	bool isCCNameLM;
	// stores returned values in mTmpDns
	SaNameT *toSaNameT(std::string &dnIn);
	virtual SaAisErrorT execute();
	virtual SaAisErrorT doExecute() = 0;
	ImmCmd(TxContext * txContextIn, std::string cmd, int retries = 59);
	virtual ~ImmCmd();
	bool isReqVersion(const SaVersionT currentVersion, SaUint8T reqReleaseCode, SaUint8T reqMajorVersion, SaUint8T reqMinorVersion);
};

class ImmCmdOmInit : public ImmCmd{
	//saImmOmInitialize
public:
	SaAisErrorT doExecute();
	ImmCmdOmInit(TxContext * txContext);
	virtual ~ImmCmdOmInit(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmAccessorInit : public ImmCmd{
	SaImmAccessorHandleT *mAccessorHandleP;
	//saImmOmAccessorInitialize
public:
	SaAisErrorT doExecute();
	//this one should be removed when the bridge gets threded
	//ImmCmdOmAccessorInit(BridgeImmClient *bridgeImmClient);
	ImmCmdOmAccessorInit(TxContext * txContext);
	virtual ~ImmCmdOmAccessorInit(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmAdminOwnerInit : public ImmCmd{
	//saImmOmAdminOwnerInitialize
	std::string mImmOwnerNameIn;
	SaBoolT mReleaseOnFinalizeIn;
public:
	SaAisErrorT doExecute();
	ImmCmdOmAdminOwnerInit(TxContext * txContext,
						   std::string immOwnerNameIn,
						   SaBoolT releaseOnFinalizeIn);
	virtual ~ImmCmdOmAdminOwnerInit(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmAdminOwnerSet : public ImmCmd{
	std::vector <std::string> * mObjectDns;
	SaImmScopeT mScope;
public:
	SaAisErrorT doExecute();
	ImmCmdOmAdminOwnerSet(TxContext * txContextIn,
						  std::vector <std::string> *objectNames,
						  SaImmScopeT scope);
	virtual ~ImmCmdOmAdminOwnerSet(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmAdminOwnerRelease : public ImmCmd{
	std::vector <std::string> * mObjectDns;
	SaImmScopeT mScope;
public:
	SaAisErrorT doExecute();
	ImmCmdOmAdminOwnerRelease(TxContext * txContextIn,
						  std::vector <std::string> *objectNames,
						  SaImmScopeT scope);
	virtual ~ImmCmdOmAdminOwnerRelease(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmAdminOwnerClear : public ImmCmd{
	std::string mDnIn;
	SaImmScopeT mScope;
public:
	SaAisErrorT doExecute();
	ImmCmdOmAdminOwnerClear(TxContext * txContextIn,
							std::string dnIn,
							SaImmScopeT scope);
	virtual ~ImmCmdOmAdminOwnerClear(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmClassDescriptionGet : public ImmCmd{
	SaImmAttrDefinitionT_2 ***mAttrDef;
	SaImmClassCategoryT    *mClassCategory;
	SaImmClassNameT         mClassName;
public:
	SaAisErrorT doExecute();
	ImmCmdOmClassDescriptionGet(TxContext * txContextIn,
								SaImmClassNameT className,
								SaImmClassCategoryT *classCategory,
								SaImmAttrDefinitionT_2 *** attrDef);
	virtual ~ImmCmdOmClassDescriptionGet(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmKeySearchInit : public ImmCmd{
	//saImmOmSearchInitialize, for iterating over keys below dn
	std::string mDnIn;
	std::string mClassNameIn;
	BridgeImmIterator* myIter_p;
public:
	SaAisErrorT doExecute();
	ImmCmdOmKeySearchInit(TxContext *txContextIn,
						  std::string dnIn,
						  std::string classNameIn,
						  BridgeImmIterator* iter_p);
	virtual ~ImmCmdOmKeySearchInit(){ENTER_IMM_OM(); LEAVE_IMM_OM(); };
};

class ImmCmdOmSearchNext : public ImmCmd{
	//saImmOmSearchNext
	std::string  mDnIn;
	std::string* mDnOut;
	SaImmAttrValuesT_2*** mAttrValsOut; //out
	BridgeImmIterator* myIter_p;

public:
	SaAisErrorT doExecute();
	ImmCmdOmSearchNext(TxContext *txContextIn,
						std::string dnIn,
						BridgeImmIterator* Iter_p,
						std::string *dnOut,
						SaImmAttrValuesT_2*** attrValsOut);
	virtual ~ImmCmdOmSearchNext(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmAccessorGet : public ImmCmd {
	// get an attr
	std::string  mDnIn;
	std::string  mAttrNameIn;
	SaImmAttrValuesT_2*** mAttrValsOut; //out

public:
	SaAisErrorT doExecute();
	ImmCmdOmAccessorGet(TxContext *txContextIn,
						std::string  dnIn,
						std::string attrNameIn,
						SaImmAttrValuesT_2*** attrValsOut);
	virtual ~ImmCmdOmAccessorGet(){ENTER_IMM_OM(); LEAVE_IMM_OM();};

};

class ImmCmdOmAdminOperationInvoke : public ImmCmd {
	// Invoke admin operation
	std::string  mDnIn;
	SaImmAdminOperationIdT mOperationId;
	SaImmAdminOperationParamsT_2 **mParams;
	SaAisErrorT *mOperationReturnValue; /*out*/
	SaImmAdminOperationParamsT_2 ***mReturnParams; /*out*/
public:
	SaAisErrorT doExecute();
	ImmCmdOmAdminOperationInvoke (TxContext *txContextIn,
						std::string  dnIn,
						SaImmAdminOperationIdT operationId,
						SaImmAdminOperationParamsT_2 **params,
						SaAisErrorT *operationReturnValue /*out*/,
						SaImmAdminOperationParamsT_2 ***returnParams /*out*/);
	virtual ~ImmCmdOmAdminOperationInvoke(){};

};

class ImmCmdOmAdminOperationMemoryFree : public ImmCmd {
	// Free memory for return parameters received from saImmOmAdminOperationInvoke_o2
	SaImmAdminOperationParamsT_2 **mReturnParams;
public:
	SaAisErrorT doExecute();
	ImmCmdOmAdminOperationMemoryFree (TxContext *txContextIn,
				  SaImmAdminOperationParamsT_2 **returnParams);
	virtual ~ImmCmdOmAdminOperationMemoryFree(){};

};

class ImmCmdOmCcbInit : public ImmCmd {
public:
	SaAisErrorT doExecute();
	ImmCmdOmCcbInit(TxContext *txContextIn);
	virtual ~ImmCmdOmCcbInit(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmCcbFinalize : public ImmCmd {
public:
	SaAisErrorT doExecute();
	ImmCmdOmCcbFinalize(TxContext *txContextIn);
	virtual ~ImmCmdOmCcbFinalize(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmCcbObjectModify : public ImmCmd {
	SaNameT*  mDnIn;
	SaImmAttrModificationT_2 **mAttrModsIn;
public:
	SaAisErrorT doExecute();
	ImmCmdOmCcbObjectModify(TxContext *txContextIn,
							SaNameT*  dnIn,
							SaImmAttrModificationT_2 **attrModsIn);
	virtual ~ImmCmdOmCcbObjectModify(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};



class ImmCmdOmCcbValidate: public ImmCmd {
public:
	SaAisErrorT doExecute();
	ImmCmdOmCcbValidate(TxContext *txContextIn);
	virtual ~ImmCmdOmCcbValidate(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};


class ImmCmdOmCcbAbort: public ImmCmd {
public:
	SaAisErrorT doExecute();
	ImmCmdOmCcbAbort(TxContext *txContextIn);
	virtual ~ImmCmdOmCcbAbort(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};


class ImmCmdOmCcbApply : public ImmCmd {
public:
	SaAisErrorT doExecute();
	ImmCmdOmCcbApply(TxContext *txContextIn);
	virtual ~ImmCmdOmCcbApply(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmCcbObjectCreate: public ImmCmd {

	SaNameT* mParentIn;
	SaImmClassNameT mClassNameIn;
	SaImmAttrValuesT_2** mAttrValsIn;
public:
	SaAisErrorT doExecute();
	ImmCmdOmCcbObjectCreate(TxContext* txContextIn,
							SaNameT *parentIn,
							SaImmClassNameT classNameIn,
							SaImmAttrValuesT_2** attrValsIn);
	virtual ~ImmCmdOmCcbObjectCreate(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};

class ImmCmdOmCcbObjectDelete: public ImmCmd {
	SaNameT*  mDnIn;
public:
	SaAisErrorT doExecute();
	ImmCmdOmCcbObjectDelete(TxContext* txContextIn,
							SaNameT*  dnIn);
   virtual ~ImmCmdOmCcbObjectDelete(){ENTER_IMM_OM(); LEAVE_IMM_OM();};
};
}// namespace
#endif
