#ifndef _MafMwSpiReplicatedList_1_h_
#define _MafMwSpiReplicatedList_1_h_

#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>

#include <stdint.h>
#include <stdbool.h>


/**
 * Constant for name length
 */
#define MW_SPI_MAX_NAME_LENGTH	256

/**
 * Replicated list service interface.
 *
 * @file MafMwSpiReplicatedList_1.h
 * @ingroup MafMwSpi
 *
 * The provider of the replicated list shall secure that the content of all defined
 * replicated lists survive all kinds of disturbances except a full cluster restart.
 * This means that the replicated list does not necessarily need to be stored on
 * persistent media.
 */


/**
 * A cluster wide unique name identifier for a list instance.
 */
typedef struct MafMwSpiListName {
    /**
     * The length of the name.
     */
    uint16_t length;
    /**
     * The name containing the characters in the string without the
     * terminating null character, and the length member of this struct
     * the number of these characters.
     */
    uint8_t value[MW_SPI_MAX_NAME_LENGTH];
} MafMwSpiListNameT;


/**
 * A list instance wide unique name identifier for an item in the list instance.
 */
typedef MafMwSpiListNameT MafMwSpiListItemNameT;


/**
 * A reference to an item in a list, used to get items and traverse the list.
 */
typedef void* MafMwSpiListItemRefT;


/**
 * Replicated List service interface.
 */
typedef struct MafMwSpiReplicatedList_1 {

    /**
     * Common interface description.
     * The "base class" for this interface contains the component
     * name, interface name, and interface version.
     */
    MafMgmtSpiInterface_1T base;


    // List Management methods

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
    MafReturnT (*listCreate) (const MafMwSpiListNameT* listInstanceName,
                              uint32_t dataBufferSize);


    /**
     * Removes all linked list items, deletes the complete link list instance and free all used memory.
     * The listInstanceName associated with the link list instance is also removed.
     *
     * @param[in] listInstanceName Pointer to a unique name for the linked list instance.
     *
     * @return MafOk, or MafNotExist if the linked list instance name does not exist.
     */
    MafReturnT (*listDelete) (const MafMwSpiListNameT* listInstanceName);


    /**
     * Removes all items in the linked list instance.
     *
     * @param[in] listInstanceName Pointer to a unique name for the linked list instance.
     *
     * @return MafOk, or MafNotExist if the linked list instance name does not exist.
     */
    MafReturnT (*listClear) (const MafMwSpiListNameT* listInstanceName);


    /**
     * Returns the number of items in the linked list instance.
     *
     * @param[in] listInstanceName Pointer to a unique name for the linked list instance.
     *
     * @param[out] listSize Pointer to the size data, that is the number of items in the link list instance.
     *
     * @return MafOk, or MafNotExist if the linked list instance name does not exist.
    */
    MafReturnT (*listGetSize) (const MafMwSpiListNameT* listInstanceName,
                               uint32_t* listSize);


    /**
     * Verifies if the linked list instance contains items.
     *
     * @param[in] listInstanceName Pointer to a unique name for the linked list instance.
     *
     * @param[out] listEmpty Pointer to a boolean parameter which is true if the link list instance contains no items.
     *
     * @return MafOk, or MafNotExist if the linked list instance name does not exist.
     */
    MafReturnT (*listIsEmpty) (const MafMwSpiListNameT* listInstanceName,
                               bool* listEmpty);


    // List Data Access methods

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
    MafReturnT (*listGetFrontRef) (const MafMwSpiListNameT* listInstanceName,
                                   MafMwSpiListItemRefT* listInstanceFrontRef);


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
    MafReturnT (*listPushBack) (const MafMwSpiListNameT* listInstanceName,
                                MafMwSpiListItemNameT* listItemName,
                                void* newItemDataBuffer);


    /**
     * Removes the last item in the linked list instance.
     *
     * @param[in] listInstanceName Pointer to a unique name for the linked list instance.
     *
     * @return MafOk, or MafNotExist if the linked list instance name does not exists.
     */
    MafReturnT (*listPopBack) (const MafMwSpiListNameT* listInstanceName);


    /**
     * Removes the item in the linked list instance pointed out by listItemName.
     *
     * @param[in] listInstanceName Pointer to a unique name for the linked list instance.
     *
     * @param[in] listItemName Pointer to a local link list instance wide unique item name.
     *
     * @return MafOk, or MafNotExist if the linked list instance name or the item name does not exist.
     */
    MafReturnT (*listEraseItem) (const MafMwSpiListNameT* listInstanceName,
                                 const MafMwSpiListItemNameT* listItemName);


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
    MafReturnT (*listGetNextItemFront) (const MafMwSpiListNameT* listInstanceName,
                                        MafMwSpiListItemRefT* currentItemRef,
                                        MafMwSpiListItemNameT* listItemName,
                                        void* copyOfItemData);

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
    MafReturnT (*listGetFinalize) (const MafMwSpiListNameT* listInstanceName,
                                   const MafMwSpiListItemRefT currentItemRef);


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
    MafReturnT (*listFindItem) (const MafMwSpiListNameT* listInstanceName,
                                const MafMwSpiListItemNameT* listItemName,
                                void* copyOfItemData);


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
    MafReturnT (*listReplaceItem) (const MafMwSpiListNameT* listInstanceName,
                                   const MafMwSpiListItemNameT* listItemName,
                                   const void* replaceItemData);

    // List Utility methods

    /**
     * Returns the current number of link lists.
     *
     * @param[out] numberOfLinkListInstances Pointer to the parameter containing the current
     * number of linked list instances.
     *
     * @return In principle always MafOk.
     */
    MafReturnT (*listNumberOfListInstances) (uint32_t* numberOfLinkListInstances);


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
    MafReturnT (*listMemoryUsage) (uint32_t* memoryUsed,
                                   uint32_t* totalMemoryAvailable);
} MafMwSpiReplicatedList_1T;

#endif
