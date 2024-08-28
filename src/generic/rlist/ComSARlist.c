/******************************************************************************
 * Copyright (C) 2010 by Ericsson AB
 * S - 125 26  STOCKHOLM
 * SWEDEN, tel int + 46 10 719 0000
 *
 * The copyright to the computer program herein is the property of
 * Ericsson AB. The program may be used and/or copied only with the
 * written permission from Ericsson AB, or in accordance with the terms
 * and conditions stipulated in the agreement/contract under which the
 * program has been supplied.
 *
 * All rights reserved.
 *
 *	 Author: uablrek
 *
 *	 File:	ComSARlist.c
 *	 The ReplicatedList service in COM_SA.
 *
 *****************************************************************************/
/*
 * Reviewed: uaberin 2010-06-18
 *
 * Modify: efaiami	 2011-03-09 for log and trace function
 *
 * Modify: efaiami	 2011-11-17 Removed get_accessmgm_interface
 *
 * Modify: xjonbuc	 2012-09-06	Implement SDP1694 - support MAF SPI
 *
 * Modify: eaparob	 2012-10-23 Correction made on replicated list max size configuration
 *
 * Mofify: eaparob	 2013-10-09	fix for HR80670 added (only assert is removed, the negative case is handled already in the code)
 *
 * Modify: uabjoy	 2014-03-18	Adding support for Trace CC
 *
 * Modify: ejnolsz	 2014-04-15 use new method getMillisecondsSinceEpochUnixTime() for marking COM SA start and stop procedure limits
 *
 * Modify: xdonngu	 2014-05-12 kill process whenever return code is try again after timeout when open check point.
 *
 * Modify: xadaleg	 2014-08-02 MR35347 - increase DN length
 *
 * Modify: xadaleg	 2015-02-17 MR37637 - Adapt IMM for replicated list service instead of checkpoint
 *
 * Modify: xnikvap   2015-04-22  Corection for TR TR HT65567: IMM Accessor handle leak and SPI return codes
 *
 *****************************************************************************/

#include <ComSARlist.h>
#include <ComSARlist_imm.h>

unsigned long long rlist_maxsize = 2 * 1024 * 1024;
bool clearAlarmsOnClusterReboot = true;
bool replicateInIMM = false;
/* Forward declarations; */
#ifndef UNIT_TEST
static void set_rlist_maxsize(xmlNode* cfg);
static void set_clearAlarmsOnClusterReboot(xmlNode* cfg);
static void set_replicateInIMM();
#else
void set_rlist_maxsize(xmlNode* cfg);
void set_clearAlarmsOnClusterReboot(xmlNode* cfg);
void set_replicateInIMM();
#endif

/* ----------------------------------------------------------------------
 * Interface Functions using MAF SPI;
 */

/**
* Creates a new list instance associated with a cluster wide unique name.
*
* @param[in] listInstanceName Pointer to a parameter which associate the created instance of
* the link list with a cluster wide unique name.
*
* @param[in] dataBufferSize The size in bytes for each item in the created link list
* instance.
*
* @return MafOk, or @n
* MafAlreadyExist if the linked list instance name already exists, or @n
* MafNoResources if no memory is available and there is a reasonable
* chance that COM can continue execution.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listCreate(
	const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize)
#else
MafReturnT maf_comsa_listCreate(const MafMwSpiListNameT* listInstanceName, uint32_t dataBufferSize)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;

	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listCreate called.");

	rc = maf_comsa_imm_listCreate( listInstanceName, dataBufferSize);
	if (MafOk != rc)
	{
		if (MafAlreadyExist == rc) {
			DEBUG_MWSA_REPLIST("maf_comsa_imm_listCreate: listInstanceName %s , return code MafAlreadyExist", (char*)listInstanceName->value);
		}
		else {
			ERR_MWSA_REPLIST("maf_comsa_imm_listCreate Error: listInstanceName %s , return code %d", (char*)listInstanceName->value, rc);
		}
		LEAVE_MWSA_REPLIST();
		return rc;
	}

	LEAVE_MWSA_REPLIST();
	DEBUG_MWSA_REPLIST("maf_comsa_imm_listCreate returning com_rc = %d  ...", rc);
	return rc;
}


/**
* Removes all linked list items, deletes the complete link list instance and free all used memory.
* The listInstanceName associated with the link list instance is also removed.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @return MafOk, or MafNotExist if the linked list instance name does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listDelete(const MafMwSpiListNameT* listInstanceName)
#else
MafReturnT maf_comsa_listDelete(const MafMwSpiListNameT* listInstanceName)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listDelete called.");
	rc = maf_comsa_imm_listDelete(listInstanceName);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Removes all items in the linked list instance.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @return MafOk, or MafNotExist if the linked list instance name does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listClear(const MafMwSpiListNameT* listInstanceName)
#else
MafReturnT maf_comsa_listClear(const MafMwSpiListNameT* listInstanceName)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listClear called.");
	rc = maf_comsa_imm_listClear(listInstanceName);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Returns the number of items in the linked list instance.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @param[out] listSize Pointer to the size data, that is the number of items in the link list instance.
*
* @return MafOk, or MafNotExist if the linked list instance name does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize)
#else
MafReturnT maf_comsa_listGetSize(const MafMwSpiListNameT* listInstanceName, uint32_t* listSize)
#endif
{
	ENTER_MWSA_REPLIST();
	DEBUG_MWSA_REPLIST("maf_comsa_listGetSize called with listInstanceName: %s", (char*) listInstanceName->value);
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listGetSize called.");
	rc = maf_comsa_imm_listGetSize(listInstanceName, listSize);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Verifies if the linked list instance contains items.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @param[out] listEmpty Pointer to a boolean parameter which is true if the link list instance contains no items.
*
* @return MafOk, or MafNotExist if the linked list instance name does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty)
#else
MafReturnT maf_comsa_listIsEmpty(const MafMwSpiListNameT* listInstanceName, bool* listEmpty)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listIsEmpty called.");
	rc = maf_comsa_imm_listIsEmpty(listInstanceName, listEmpty);
	LEAVE_MWSA_REPLIST();
	return rc;
}


 // List Data Access methods

/**
* Adds a new item to the end of the link list instance.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @param[out] listItemName Pointer to a local linked list instance wide unique item name.
*
* @param[in] newItemDataBuffer Pointer to a data buffer containing the new data item.
*
* @return MafOk, or @n
* MafNotExist if the linked list instance name does not exist, or @n
* MafNoResources if no memory is available and there is a reasonable
* chance that COM can continue execution.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listPushBack(const MafMwSpiListNameT* listInstanceName,
										 MafMwSpiListItemNameT* listItemName, /* OUT parameter */
										 void* newItemDataBuffer)
#else
MafReturnT maf_comsa_listPushBack(const MafMwSpiListNameT* listInstanceName,
								  MafMwSpiListItemNameT* listItemName, /* OUT parameter */
								  void* newItemDataBuffer)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listPushBack called.");
	rc = maf_comsa_imm_listPushBack(listInstanceName, listItemName, newItemDataBuffer);

	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Removes the last item in the linked list instance.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @return MafOk, or MafNotExist if the linked list instance name does not exists.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listPopBack(const MafMwSpiListNameT* listInstanceName)
#else
MafReturnT maf_comsa_listPopBack(const MafMwSpiListNameT* listInstanceName)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listPopBack called.");
	rc = maf_comsa_imm_listPopBack(listInstanceName);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Removes the item in the linked list instance pointed out by listItemName.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @param[in] listItemName Pointer to a local link list instance wide unique item name.
*
* @return MafOk, or MafNotExist if the linked list instance name or the item name does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listEraseItem(
		const MafMwSpiListNameT* listInstanceName,
		const MafMwSpiListItemNameT* listItemName)
#else
MafReturnT maf_comsa_listEraseItem(
	const MafMwSpiListNameT* listInstanceName,
	const MafMwSpiListItemNameT* listItemName)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	if (listInstanceName == NULL || listItemName == NULL) {
		return MafInvalidArgument;
	}

	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listEraseItem called.");
	rc = maf_comsa_imm_listEraseItem(listInstanceName, listItemName);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Returns a reference to the first item in the link list instance.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @param[out] listInstanceFrontRef Pointer to the first item in the link list instance. If
* no item exist, null is returned.
*
* @return MafOk, or MafNotExist if the linked list instance name does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef)
#else
MafReturnT maf_comsa_listGetFrontRef(const MafMwSpiListNameT* listInstanceName, MafMwSpiListItemRefT* listInstanceFrontRef)
#endif
{
	ENTER_MWSA_REPLIST();
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listGetFrontRef called.");
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("maf_comsa_listGetFrontRef(): called with listInstanceFrontRef:  %p", listInstanceFrontRef);
	rc = maf_comsa_imm_listGetFrontRef(listInstanceName, listInstanceFrontRef);
	DEBUG_MWSA_REPLIST("maf_comsa_listGetFrontRef returning com_rc = %d  ...", rc);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Removes the memory associated with currentItemRef. This function should
* be called after the last call to listGetNextItemFront if the end of the
* list was not reached, that is if the currentItemRef is not NULL.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
* @param[in] currentItemRef Reference to an item in the linked list instance.
*
* @return MafOk, or MafNotExists if the linked list instance name or currentItemRef does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listGetFinalize(
		const MafMwSpiListNameT* listInstanceName,
		const MafMwSpiListItemRefT currentItemRef)
#else
MafReturnT maf_comsa_listGetFinalize(
	const MafMwSpiListNameT* listInstanceName,
	const MafMwSpiListItemRefT currentItemRef)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listGetFinalize called.");
	rc = maf_comsa_imm_listGetFinalize(listInstanceName, currentItemRef);
	LEAVE_MWSA_REPLIST();
	DEBUG_MWSA_REPLIST("maf_comsa_imm_listGetFinalize returning com_rc = %d  ...", rc);
	return rc;
}


/**
* Retrieves a copy a linked list instance data item pointed out by currentItemRef.
* The output value of currentItemRef points to the next item in the linked list instance.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @param[in,out] currentItemRef Reference to an item in the linked list instance.
*
* @li In value is pointing to the current item in the linked list instance.
* @li Out value is pointing to the next item in the linked list instance. If the value is null, the end of the list is reached.
* @li The initial value of this parameter must be set by the function @em mwSpiListGetFrontRef().
*
* @param[out] listItemName Pointer to the name of the copied data item. The name
* is unique within the linked list.
*
* @param[out] copyOfItemData Pointer to a data buffer containing a copy of the
* linked list instance data item pointed out by the input value of currentItemRef.
* The buffer is provided by the interface client.
*
* @return MafOk, or MafNotExist if the linked list instance name does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listGetNextItemFront(const MafMwSpiListNameT* listInstanceName,
		MafMwSpiListItemRefT* currentItemRef, MafMwSpiListItemNameT* listItemName, void* copyOfItemData)
#else
MafReturnT maf_comsa_listGetNextItemFront(const MafMwSpiListNameT* listInstanceName,
		MafMwSpiListItemRefT* currentItemRef, MafMwSpiListItemNameT* listItemName, void* copyOfItemData)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listGetNextItemFront called.");
	rc = maf_comsa_imm_listGetNextItemFront(listInstanceName, currentItemRef, listItemName, copyOfItemData);
	LEAVE_MWSA_REPLIST();
	DEBUG_MWSA_REPLIST("maf_comsa_listGetNextItemFront returning com_rc = %d  ...", rc);
	return rc;
}


/**
* Traverses the list and retrieves a copy of a linked list instance data item pointed out by listItemName.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @param[in] listItemName Pointer to the local link list instance wide unique item
* name for the searched data item.
*
* @param[out] copyOfItemData Pointer to a data buffer containing a copy of the
* linked list instance data item pointed out by listItemName.
* The buffer is provided by the interface client.
*
* @return MafOk, or MafNotExist if the linked list instance name or listItemName does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listFindItem(
		const MafMwSpiListNameT* listInstanceName,
		const MafMwSpiListItemNameT* listItemName,
		void* copyOfItemData)
#else
MafReturnT maf_comsa_listFindItem(
	const MafMwSpiListNameT* listInstanceName,
	const MafMwSpiListItemNameT* listItemName,
	void* copyOfItemData)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listFindItem called.");
	rc = maf_comsa_imm_listFindItem(listInstanceName, listItemName, copyOfItemData);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Traverses the list and replaces a linked list instance data item pointed out by listItemName.
* The old item is no longer available.
*
* @param[in] listInstanceName Pointer to a unique name for the linked list instance.
*
* @param[in] listItemName Pointer to the local link list instance wide unique item
* name for the searched data item.
*
* @param[in] replaceItemData Pointer to a data buffer containing the data item used
* to replace the existing data item. The buffer is provided by the interface client.
*
* @return MafOk, or MafNotExist if the linked list instance name or listItemName does not exist.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listReplaceItem(const MafMwSpiListNameT* listInstanceName,
		const MafMwSpiListItemNameT* listItemName, const void* replaceItemData)
#else
MafReturnT maf_comsa_listReplaceItem(const MafMwSpiListNameT* listInstanceName,
		const MafMwSpiListItemNameT* listItemName, const void* replaceItemData)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listReplaceItem called.");
	rc = maf_comsa_imm_listReplaceItem(listInstanceName, listItemName, replaceItemData);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Returns the current number of link lists.
*
* @param[out] numberOfLinkListInstances Pointer to the parameter containing the current
* number of linked list instances.
*
* @return In principle always MafOk.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listNumberOfListInstances(uint32_t* numberOfLinkListInstances)
#else
MafReturnT maf_comsa_listNumberOfListInstances(uint32_t* numberOfLinkListInstances)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;
	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listNumberOfListInstances called.");
	rc = maf_comsa_imm_listNumberOfListInstances(numberOfLinkListInstances);
	LEAVE_MWSA_REPLIST();
	return rc;
}


/**
* Returns the memoryUsed (in per cent) and the totalMemoryAvailable (in bytes).
*
* @param[out] memoryUsed Pointer to the parameter containing the current memory
* usage (in per cent).
*
* @param[out] totalMemoryAvailable Pointer to the parameter containing the total memory
* available (in bytes) for the Replica List provider.
*
* @return MafOk if memory is available, or MafOutOfMemory if no memory is available.
*/
#ifndef UNIT_TEST
static MafReturnT maf_comsa_listMemoryUsage(
	uint32_t* memoryUsed, uint32_t* totalMemoryAvailable)
#else
MafReturnT maf_comsa_listMemoryUsage(uint32_t* memoryUsed, uint32_t* totalMemoryAvailable)
#endif
{
	ENTER_MWSA_REPLIST();
	// we will have a flag to determine which function will be called.
	MafReturnT rc = MafOk;

	DEBUG_MWSA_REPLIST("RL_SPI: maf_comsa_listMemoryUsage called.");
	rc = maf_comsa_imm_listMemoryUsage(memoryUsed, totalMemoryAvailable);

	LEAVE_MWSA_REPLIST();
	return rc;
}


/* ----------------------------------------------------------------------
 * Startup;
 */

/* Forward declarations; */

/* Data; */
#define MW_COMPONENT_NAME "MW"
static char const* const oam_component_name = MW_COMPONENT_NAME;

/* SDP 1694 - support MAF API */

#ifndef UNIT_TEST
static MafReturnT maf_comSAMwComponentStart(MafStateChangeReasonT reason);
static MafReturnT maf_comSAMwComponentStop(MafStateChangeReasonT reason);
#else
MafReturnT maf_comSAMwComponentStart(MafStateChangeReasonT reason);
MafReturnT maf_comSAMwComponentStop(MafStateChangeReasonT reason);
#endif

/* Data; */
static MafMwSpiReplicatedList_1T maf_rlistService = {
	.base = MafMwSpiReplicatedList_1Id,
	.listCreate = maf_comsa_listCreate,
	.listDelete = maf_comsa_listDelete,
	.listClear = maf_comsa_listClear,
	.listGetSize = maf_comsa_listGetSize,
	.listIsEmpty = maf_comsa_listIsEmpty,
	.listGetFrontRef = maf_comsa_listGetFrontRef,
	.listPushBack = maf_comsa_listPushBack,
	.listPopBack = maf_comsa_listPopBack,
	.listEraseItem = maf_comsa_listEraseItem,
	.listGetNextItemFront = maf_comsa_listGetNextItemFront,
	.listGetFinalize = maf_comsa_listGetFinalize,
	.listFindItem = maf_comsa_listFindItem,
	.listReplaceItem = maf_comsa_listReplaceItem,
	.listNumberOfListInstances = maf_comsa_listNumberOfListInstances,
	.listMemoryUsage = maf_comsa_listMemoryUsage
};
static MafMgmtSpiInterface_1T* maf_ifarray[3] = {
	(MafMgmtSpiInterface_1T*)&maf_rlistService,
	NULL
};
static MafMgmtSpiInterface_1T* maf_deparray[] =	 { NULL };
static MafMgmtSpiInterface_1T* maf_optarray[] =  { NULL };
static MafMgmtSpiComponent_2T maf_mw = {
	.base = {MW_COMPONENT_NAME, "MafMgmtSpiComponent", "2"},
	.interfaceArray = maf_ifarray,
	.dependencyArray = maf_deparray,
	.start = maf_comSAMwComponentStart,
	.stop = maf_comSAMwComponentStop,
	.optionalDependencyArray = maf_optarray
};


#ifndef UNIT_TEST
static MafReturnT maf_comSAMwComponentStart(MafStateChangeReasonT reason)
#else
MafReturnT maf_comSAMwComponentStart(MafStateChangeReasonT reason)
#endif
{
	ENTER_MWSA();
	LOG_MWSA ("maf_comSAMwComponentStart (): MW SA component start procedure begins: %llu",getMillisecondsSinceEpochUnixTime());
	DEBUG_MWSA("maf_comSAMwComponentStart called...");
	MafReturnT maf_rc = maf_comsa_imm_comSAMwComponentStart();
	if (MafOk != maf_rc)
	{
		LOG_MWSA("maf_comsa_imm_comSAMwComponentStart failed, return code %d",maf_rc);
		LEAVE_MWSA();
		return maf_rc;
	}
	LOG_MWSA("maf_comSAMwComponentStart(): MW SA component start procedure completed: %llu", getMillisecondsSinceEpochUnixTime());
	LEAVE_MWSA();
	return maf_rc;
}


#ifndef UNIT_TEST
static MafReturnT maf_comSAMwComponentStop(MafStateChangeReasonT reason)
#else
MafReturnT maf_comSAMwComponentStop(MafStateChangeReasonT reason)
#endif
{
	ENTER_MWSA();
	LOG_MWSA("maf_comSAMwComponentStop(): MW SA component stop procedure begins: %llu", getMillisecondsSinceEpochUnixTime());
	DEBUG_MWSA("maf_comSAMwComponentStop called...");
	MafReturnT maf_rc = maf_comsa_imm_comSAMwComponentStop();
	if (MafOk != maf_rc)
	{
		LOG_MWSA("maf_comsa_imm_comSAMwComponentStop() failed, return code %d",maf_rc);
		LEAVE_MWSA();
		return maf_rc;
	}
	LOG_MWSA("maf_comSAMwComponentStop(): MW SA component stop procedure completed: %llu", getMillisecondsSinceEpochUnixTime());
	LEAVE_MWSA();
	return maf_rc;
}


MafReturnT maf_comSAMwComponentInitialize(
	MafMgmtSpiInterfacePortal_3T* portal,
	xmlDocPtr config)
{
	ENTER_MWSA();
	xmlNode* cfg;

	cfg = coremw_find_config_item(oam_component_name, "replicatedlist");
	// if config value available then execute if statement
	// else do nothing, because the value of variable "rlist_maxsize" is already set by default
	if (cfg != NULL)
	{
		set_rlist_maxsize(cfg);
		set_clearAlarmsOnClusterReboot(cfg);
	}

	set_replicateInIMM();

	LOG_MWSA("ReplicatedList, maxsize=%llu, clearAlarmsOnClusterReboot=%d", rlist_maxsize, clearAlarmsOnClusterReboot);

	maf_ifarray[0] = (MafMgmtSpiInterface_1T*)&maf_rlistService;
	maf_ifarray[0]->componentName = oam_component_name;
	maf_ifarray[1] = NULL;
	maf_ifarray[2] = NULL;
	MafReturnT maf_rc = portal->registerComponent(&maf_mw);

	LEAVE_MWSA();
	return maf_rc;
}


void maf_comSAMwComponentFinalize(
	MafMgmtSpiInterfacePortal_3T* portal)
{
	ENTER_MWSA();
	portal->unregisterComponent(&maf_mw);
	LEAVE_MWSA();
}


/*
 * Return cases:
 *
 *	  positive: - return the value which was read out from system file, if this value is greater then ULLONG_MAX then, return ULLONG_MAX
 *
 *	  negative:
 *
 *				- return 0 - file is not in place
 *				- return 0 - value not readable as unsigned long long
 */
#ifndef UNIT_TEST
static unsigned long long readShmmaxValue()
#else
unsigned long long readShmmaxValue()
#endif
{
	ENTER_MWSA_REPLIST();
	// set the default return value to 0
	unsigned long long ret = 0;
	FILE* pConfigFile = NULL;

	if ((pConfigFile = fopen(shmmaxFile,"r")) != NULL)
	{
		DEBUG_MWSA_REPLIST("file opened: (%s)",shmmaxFile);
		char buf[SA_MAX_UNEXTENDED_NAME_LENGTH];
		if (fgets(buf,sizeof(buf),pConfigFile) != NULL)
		{
			// convert the value. If value is greater then ULLONG_MAX then ULLONG_MAX will be the return value
			ret = strtoull(buf, NULL, 0);
			DEBUG_MWSA_REPLIST("config value: (%llu)",ret);
		}
		fclose(pConfigFile);
		DEBUG_MWSA_REPLIST("file closed: (%s)",shmmaxFile);
	}
	else
	{
		DEBUG_MWSA_REPLIST("no such file: (%s)",shmmaxFile);
	}

	DEBUG_MWSA_REPLIST("readShmmaxValue leave with: (%llu)",ret);
	LEAVE_MWSA_REPLIST();
	return ret;
}


/*
 * If config value available then try to read system value and set it to max value.
 *		   If there is no system value then set the default max value to max value.
 *		   Compare config value to max value, if config value is less then the max value then set config value to max value,
 *		   otherwise max value will remain as max value
 *
 * else if config value not available then do nothing, because the value of variable "rlist_maxsize" is already set by default
 *
 * function results:
 *
 *				 -update the value of global variable "rlist_maxsize" with the new value or
 *				 -do not update rlist_maxsize
 */
#ifndef UNIT_TEST
static void set_rlist_maxsize(xmlNode* cfg)
#else
void set_rlist_maxsize(xmlNode* cfg)
#endif
{
	ENTER_MWSA_REPLIST();
	char const* maxsize = coremw_xmlnode_get_attribute(cfg, "maxsize");
	// if config value available then execute if statement
	// else do nothing, because the value of variable "rlist_maxsize" is already set by default
	if (maxsize != NULL)
	{
		unsigned long long rlist_confsize;
		// read config value
		rlist_confsize = strtoull(maxsize, NULL, 0);
		DEBUG_MWSA_REPLIST("rlist_confsize: %llu",rlist_confsize);
		// if rlist_confsize is not zero, then proceed with the value
		// else if rlist_confsize is zero, then the conversion was not successful, so do nothing
		if(rlist_confsize != 0)
		{
			unsigned long long systemMaxSize;
			// save the original rlist_maxsize value
			unsigned long long default_rlist_maxsize = rlist_maxsize;
			/*
			 *	read systemMaxSize value:
			 *	  execute  if  case: if value of readShmmaxValue() is not 0. It is also valid when the value reached the ULLONG_MAX where we also set the rlist_maxsize to the value read out from readShmmaxValue().
			 *	  execute else case: if value of readShmmaxValue() is 0
			 */
			systemMaxSize = readShmmaxValue();
			if(systemMaxSize)
			{
				rlist_maxsize = systemMaxSize;
				DEBUG_MWSA_REPLIST("rlist_maxsize = systemMaxSize = %llu",rlist_maxsize);
			}
			else
			{
				rlist_maxsize = default_rlist_maxsize;
				DEBUG_MWSA_REPLIST("Set rlist_maxsize to default: %llu",rlist_maxsize);
			}

			// compare
			if (rlist_confsize < rlist_maxsize)
			{
				DEBUG_MWSA_REPLIST("rlist_confsize < rlist_maxsize");
				rlist_maxsize = rlist_confsize;
			}
		}
	}
	LEAVE_MWSA_REPLIST();
}

/*
 * If config value available
 *         then read the value and set it to clearAlarmsOnClusterReboot.
 *
 * else if config value not available
 *         then do nothing, because the value of variable "clearAlarmsOnClusterReboot" is already set by default
 *
 */
#ifndef UNIT_TEST
static void set_clearAlarmsOnClusterReboot(xmlNode* cfg)
#else
void set_clearAlarmsOnClusterReboot(xmlNode* cfg)
#endif
{
	ENTER_MWSA_REPLIST();
	char const* value = coremw_xmlnode_get_attribute(cfg, "clearAlarmsOnClusterReboot");

	if (value != NULL)
	{
		DEBUG_MWSA_REPLIST("clearAlarmsOnClusterReboot: %s",value);

		if(strcasecmp(value, "false") == 0) {
			clearAlarmsOnClusterReboot = false;
		}
	}
	LEAVE_MWSA_REPLIST();
}

#ifndef UNIT_TEST
static void set_replicateInIMM()
#else
void set_replicateInIMM()
#endif
{
	ENTER_MWSA_REPLIST();

	char* isLdaDeployment = getenv("LDA_DEPLOYMENT");
	if( isLdaDeployment != NULL ){
		if (0 == strcmp(isLdaDeployment, "1")) {
			DEBUG_MWSA_REPLIST("set_replicateInIMM() LDA deployment. RP List will be stored in IMM");
			replicateInIMM = true;
		}
	}

	LEAVE_MWSA_REPLIST();
}
