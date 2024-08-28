/*
* MockTransactionMasterIF2.cpp
*
*  Created on: Sep 30, 2011
*      Author: uabjoy
*/

#include "MockTransactionMasterIF2.h"
#include <string>
#include <stdio.h>

extern MafOamSpiModelRepository_1T* theModelRepo_v1_p;
/*
* MafMgmtSpiThreadContext_2
*/

MafReturnT addMessage(ThreadContextMsgCategory_2T category, const char* message) {
	return MafOk;
}

MafReturnT newMessageIterator(ThreadContextMsgCategory_2T category, ThreadContextHandle_2T* handle) {
	return MafOk;
}


MafReturnT nextMessage(ThreadContextHandle_2T handle, const char** message) {
	return MafOk;
}


MafReturnT clearMessages(ThreadContextMsgCategory_2T category) {
	return MafOk;
}

MafReturnT messageCount(ThreadContextMsgCategory_2T category, uint32_t* count) {
	return MafOk;
}


MafReturnT getMessages(ThreadContextMsgCategory_2T category, ThreadContextHandle_2T* msgHandle, const char*** messages)
{

	typedef char* myStringType;

	myStringType* eStrings = new myStringType[128];

	static char sEStringOne[] = "ErrorStringOne";
	eStrings[0] = sEStringOne;

	static char sEStringTwo[] = "ErrorStringTwo";
	eStrings[1] = sEStringTwo;

	eStrings[2] = NULL;
	*messages = const_cast<const char **>(eStrings);
	return MafOk;
}


MafReturnT releaseHandle(ThreadContextHandle_2T handle) {
	return MafOk;
}


MafMgmtSpiThreadContext_2 stubThreadContext_2 = { { "", "", "" }, addMessage, newMessageIterator,
	nextMessage, clearMessages, messageCount, getMessages, releaseHandle};

/*
* MafOamSpiManagedObject Ver.3
*/

std::string receivedData_setMoAttribute;
std::string receivedDN_setMoAttribute;


static MafReturnT setMoAttribute(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * attributeName, const MafMoAttributeValueContainer_3T * attributeValue) {

	receivedData_setMoAttribute=attributeValue->values->value.theString;
	receivedDN_setMoAttribute=dn;
	return MafOk;
}


MafMoAttributeValueResult_3T  * getMoAttributeResult;


static MafReturnT getMoAttribute(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * attributeName, MafMoAttributeValueResult_3T  * result) {
	//Return prepared data structure
	//	*result=getMoAttributeresult;
	result = getMoAttributeResult;
	return MafOk;
}


MafMoAttributeValuesResult_3T  * getMoAttributesResult;


static MafReturnT getMoAttributes(MafOamSpiTransactionHandleT txHandle, const char * dn, const char ** attributeNames, MafMoAttributeValuesResult_3T * result)
{
	result = getMoAttributesResult;
	return MafOk;
}


static MafReturnT newMoIterator(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * className, MafOamSpiMoIteratorHandle_3T *result) {
	return MafOk;
}


static MafReturnT nextMo(MafOamSpiMoIteratorHandle_3T itHandle, char **result) {
	return MafOk;
}


std::string parentDnReceived, classNameReceived, keyAttributeNameReceived, keyAttributevalueReceived;


static MafReturnT createMo(MafOamSpiTransactionHandleT txHandle, const char * parentDn, const char * className, const char * keyAttributeName,  const char * keyAttributevalue, MafMoNamedAttributeValueContainer_3T ** initialAttributes) {
	parentDnReceived.clear();
	classNameReceived.clear();
	keyAttributeNameReceived.clear();
	keyAttributevalueReceived.clear();
	parentDnReceived.clear();

	parentDnReceived = parentDn;
	classNameReceived = className;
	keyAttributeNameReceived = keyAttributeName;
	keyAttributevalueReceived = keyAttributevalue;

	return MafOk;
}

std::string dnReceived;
MafReturnT ReturndeleteMo = MafOk;

static MafReturnT deleteMo(MafOamSpiTransactionHandleT txHandle, const char * dn) {
	dnReceived.clear();
	dnReceived = dn;
	return ReturndeleteMo;
}


MafMoAttributeValueContainer_3T parameters_received[10];

static MafReturnT action(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * name, MafMoNamedAttributeValueContainer_3T **parameters, MafMoAttributeValueResult_3T *result) {
	return MafOk;
}


/**
*  finalizeMoIterator
*/
static MafReturnT finalizeMoIterator(MafOamSpiMoIteratorHandle_3T itHandle)
{
	return MafOk;
}


static MafReturnT existsMo(MafOamSpiTransactionHandleT txHandle, const char * dn, bool * result)
{
	return MafOk;
}


static MafReturnT countMoChildren(MafOamSpiTransactionHandleT txHandle, const char * dn, const char * className, uint64_t * result)
{
	return MafOk;
}


MafOamSpiManagedObject_3T stubPortalManaged = {
	{ "", "", "" },
	setMoAttribute,
	getMoAttribute,
	getMoAttributes,
	newMoIterator,
	nextMo,
	createMo,
	deleteMo,
	action,
	finalizeMoIterator,
	existsMo,
	countMoChildren
};

/*
* TransactionMaster_2
*/

bool newTransactionCalled = false;
MafOamSpiTransactionHandleT newTransactionResult = 6303;
MafReturnT newTransaction(MafLockPolicyT policy, unsigned int timeout, MafOamSpiTransactionHandleT *result) {
	newTransactionCalled = true;
	*result = newTransactionResult;
	return MafOk;
}

bool commitReceived = false;
MafReturnT commit(MafOamSpiTransactionHandleT txHandle) {
	commitReceived = true;
	return MafOk;
}

bool explicitPrepareReceived = false;
MafReturnT explicitPrepare(MafOamSpiTransactionHandleT txHandle) {
	explicitPrepareReceived = true;
	return MafOk;
}

MafReturnT abort(MafOamSpiTransactionHandleT txHandle) {
	return MafOk;
}

bool isRegisteredReceived = false;
MafReturnT isRegistered( MafOamSpiTransactionHandleT txHandle, MafOamSpiTransactionalResource_2T * participant, bool *result) {
	isRegisteredReceived = true;
	return MafOk;
}

MafReturnT validate(MafOamSpiTransactionHandleT txHandle, bool *result) {
	*result = true;
	return MafOk;
}

MafOamSpiTransactionMaster_2T txMasterMockInstance = { { "", "", "" }, newTransaction, commit, abort,
	explicitPrepare, isRegistered, validate};

/*
* End Of TransactionMaster_2
*/

//
//MafReturnT getInterfaceArray(const char *interface, const char * version, MafMgmtSpiInterface_1T ***result) {
//	return MafOk;
//}
//
//MafReturnT registerComponent(MafMgmtSpiComponent_2T *component) {
//	return MafOk;
//}
//
//MafReturnT unregisterComponent(MafMgmtSpiComponent_2T *component) {
//	return MafOk;
//}
//// Is the code below needed by the test environment ??
//
//MafReturnT registerParticipant(MafOamSpiTransactionHandleT txHaSndle,
//		MafOamSpiTransactionalResource_1T * resp) {
//	return MafOk;
//}
//
//MafReturnT setContext(MafOamSpiTransactionHandleT txHandle,
//		MafOamSpiTransactionalResource_1T *resource, void *context) {
//	return MafOk;
//}
//
//MafReturnT getContext(MafOamSpiTransactionHandleT txHandle,
//		MafOamSpiTransactionalResource_1T *resource, void **context) {
//	return MafOk;
//}
//
//MafReturnT getLockPolicy(MafOamSpiTransactionHandleT txHandle,
//		MafLockPolicyT *result) {
//	return MafOk;
//}


#ifdef  __cplusplus
extern "C" {
#endif

	bool joinCalled = false;
	MafReturnT join(MafOamSpiTransactionHandleT txHandle) {
		joinCalled = true;
		return MafOk;
	}
	MafReturnT prepare(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	/* Already defined use that one.
MafReturnT commit(MafOamSpiTransactionHandleT txHandle) {
	return MafOk;
}
*/
	MafReturnT myabort(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}
	MafReturnT finish(MafOamSpiTransactionHandleT txHandle) {
		return MafOk;
	}

	MafOamSpiTransactionalResource_1T myTxRes = { { "", "", "" }, join, prepare,
		commit, myabort, finish };

#ifdef  __cplusplus
}
#endif

//MafOamSpiTransaction_1T type = { { "", "", "" }, registerParticipant,
//		setContext, getContext, getLockPolicy };

//MafOamSpiTransaction_1* MafOamSpiTransactionStruct_p = &type;

//////////////////////////////////////////////////////
// SPI Interface portal exports
//////////////////////////////////////////////////////
bool MafOamSpiTransactionMaster_1_available=true;
bool MafOamSpiTransactionMaster_2_available=true;
bool MafOamSpiTransactionMaster_1_selected;
bool MafOamSpiTransactionMaster_2_selected;


MafReturnT getInterface( MafMgmtSpiInterface_1T interfaceId, MafMgmtSpiInterface_1T **result) {
	/*
	* Need to add portal for Transaction Master 2 Interface here.
	*/

	std::string  interfaceNameFound(interfaceId.interfaceName);
	if (interfaceNameFound == "MafOamSpiTransactionMaster") {
		/*
		* Here we have this coming in : (MafMgmtSpiInterface_1T**)&txMaster
		* And txMaster defined like this : MafOamSpiTransactionMaster_2T * txMaster;
		* So now we assign this interface to the pointer.
		*
		* NOTA BENE :-) Now two versions of this interface is available, must check version number!
		*/
		MafOamSpiTransactionMaster_1_selected=false;
		MafOamSpiTransactionMaster_2_selected=false;
		if ( std::string(interfaceId.interfaceVersion) == "1" ) {
			*result = (MafMgmtSpiInterface_1T *)&myTxRes;
			if (MafOamSpiTransactionMaster_1_available) {
				MafOamSpiTransactionMaster_1_selected=true;
				return MafOk;
			}
			else return MafFailure;
		}else {
			// Version two here!
			*result = (MafMgmtSpiInterface_1T *)&txMasterMockInstance;
			if (MafOamSpiTransactionMaster_2_available){
				MafOamSpiTransactionMaster_2_selected=true;
				return MafOk;
			} else return MafFailure;
		}
		return MafOk;
	}

	if (interfaceNameFound == "OamTransactionalInterface") {
		/*
		* Here we have this coming in : (MafMgmtSpiInterface_1T**)&txMaster
		* And txMaster defined like this : MafOamSpiTransactionMaster_2T * txMaster;
		* So now we assign this interface to the pointer.
		*/
		*result = (MafMgmtSpiInterface_1T *)&myTxRes;
		return MafOk;
	}

	if (interfaceNameFound == "MafOamSpiManagedObject") {
		/*
		* Here we have this coming in : (MafMgmtSpiInterface_1T**)&txMaster
		* And txMaster defined like this : MafOamSpiTransactionMaster_2T * txMaster;
		* So now we assign this interface to the pointer.
		*/
		*result = (MafMgmtSpiInterface_1T *)&stubPortalManaged;
		return MafOk;
	}

	// This is a fix to be able to reuse the RegisterObjectImplementer_unittest.cc
	// for test of setMoAttribute( .. )
	if (interfaceNameFound == "OamSARegisterObjectInterface") {
		*result = (MafMgmtSpiInterface_1T *)&stubPortalManaged;
		return MafOk;
	}

	if (interfaceNameFound == "MafMgmtSpiThreadContext") {
		*result = (MafMgmtSpiInterface_1T *)&stubThreadContext_2;
		return MafOk;
	}

	if (interfaceNameFound == "MafOamSpiModelRepository") {
		*result = (MafMgmtSpiInterface_1T *)&theModelRepo_v1_p->base;
		return MafOk;
	}

	return MafOk;
}


//MafMgmtSpiInterfacePortal_3T stubPortal = { { "", "", "" }, getInterface, getInterfaceArray,
//											registerComponent, unregisterComponent};

//MafMgmtSpiInterface_1T* portal= (MafMgmtSpiInterface_1T *)&stubPortal;



MockTransactionMasterIF_2::MockTransactionMasterIF_2() {
	// TODO Auto-generated constructor stub

}

MockTransactionMasterIF_2::~MockTransactionMasterIF_2() {
	// TODO Auto-generated destructor stub
}

