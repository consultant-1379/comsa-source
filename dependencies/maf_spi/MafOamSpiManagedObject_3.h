#ifndef MafOamSpiManagedObject_3_h
#define MafOamSpiManagedObject_3_h
#include <MafOamSpiTransaction_1.h>
#include <stdint.h>

#ifndef __cpluplus
#include <stdbool.h>
#endif

/**
 * Managed object interface.
 *
 * @file MafOamSpiManagedObject_3.h
 * @ingroup MafOamSpi
 *
 * Contains functions and data definitions for accessing the MOs.
 *
 *
 */


/**
 * Defines a handle to an MO iterator.
 */
typedef uint64_t MafOamSpiMoIteratorHandle_3T;

/**
 * Forward declaration.
 */
struct MafMoAttributeValueStructMember_3;
struct MafMoAttributeValueContainer_3;

/**
* Defines the list of the valid data types.
*/
typedef enum MafOamSpiMoAttributeType_3 {
    /**
     * An 8-bit integer.
     */
    MafOamSpiMoAttributeType_3_INT8 = 1,
    /**
     * A 16-bit integer.
     */
    MafOamSpiMoAttributeType_3_INT16 = 2,
    /**
     * A 32-bit integer.
     */
    MafOamSpiMoAttributeType_3_INT32 = 3,
    /**
     * A 64-bit integer.
     */
    MafOamSpiMoAttributeType_3_INT64 = 4,
    /**
     * An 8-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_3_UINT8 = 5,
    /**
     * A 16-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_3_UINT16 = 6,
    /**
     * A 32-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_3_UINT32 = 7,
    /**
     * A 64-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_3_UINT64 = 8,
    /**
     * A string value.
     */
    MafOamSpiMoAttributeType_3_STRING = 9,
    /**
     * A boolean.
     */
    MafOamSpiMoAttributeType_3_BOOL = 10,
    /**
     * A reference to another Managed Object (MO) class.
     */
    MafOamSpiMoAttributeType_3_REFERENCE = 11,
    /**
     * An enumeration.
     */
    MafOamSpiMoAttributeType_3_ENUM = 12,
    /**
     * A struct or aggregated data type.
     */
    MafOamSpiMoAttributeType_3_STRUCT = 14,


    /**
     * A 64 bits floating point value
     */
    MafOamSpiMoAttributeType_3_DECIMAL64 = 16

} MafOamSpiMoAttributeType_3T;



/**
 * A container for one attribute value.
 */
typedef struct MafMoAttributeValue_3 {

    union {
        /**
         * An 8-bit integer.
         */
        int8_t   i8;
        /**
         * A 16-bit integer.
         */
        int16_t  i16;
        /**
         * A 32-bit integer.
         */
        int32_t  i32;
        /**
         * A 64-bit integer.
         */
        int64_t  i64;
        /**
         * An 8-bit unsigned integer.
         */
        uint8_t  u8;
        /**
         * A 16-bit unsigned integer.
         */
        uint16_t u16;
        /**
         * A 32-bit unsigned integer.
         */
        uint32_t u32;
        /**
         * A 64-bit unsigned integer.
         */
        uint64_t u64;
        /**
         * A string value.
         */
        const char *   theString;
        /**
         * An enumeration.
         */
        int64_t theEnum;
        /**
         * A boolean.
         */
        bool     theBool;
        /**
         * A distinguished name (dn).
         */
        const char *   moRef;

        /**
         * A pointer to the first member of the struct.
         */
        struct MafMoAttributeValueStructMember_3 * structMember;

        /**
         * A 64 bits floating point number
         */
        double decimal64;

    } value;
} MafMoAttributeValue_3T;

/**
 * Used for the struct members. A struct contains one or more
 * members.
 */
typedef struct MafMoAttributeValueStructMember_3 {

    /**
     * Name of the member.
     */
    char * memberName;

    /**
     * Value of the member.
     */
    struct MafMoAttributeValueContainer_3 * memberValue;

    /**
     * Pointer to the next struct member, or null if
     * there are no more structs.
     */
    struct MafMoAttributeValueStructMember_3 * next;

} MafMoAttributeValueStructMember_3T;



/**
 * A container for an attribute including the type and the value.
 * It can contain a number of attribute values
 * having the same attribute type.
 */
typedef struct MafMoAttributeValueContainer_3 {

    /**
     * Type of the attributes in the container.
     */
    MafOamSpiMoAttributeType_3T type;

    /**
     * Number of values in the container.
     */
    unsigned int nrOfValues;

    /**
     * Pointer to the first element in the array.
     */
    MafMoAttributeValue_3T *values;

} MafMoAttributeValueContainer_3T;


/**
* A container for named attribute or action attribute values
*/
typedef struct MafMoNamedAttributeValueContainer_3 {
    /**
     * The name of the attribute
     */
    const char * name;
    /**
     * The value container
     */
    MafMoAttributeValueContainer_3T value;
} MafMoNamedAttributeValueContainer_3T;


/**
 * Retrieved data from getAttribute and from action.
 */
typedef struct MafMoAttributeValueResult_3 {

    /**
     * Function invoked by the SPI user to release allocated data
     * The pointer to the function is provided by the SPI implementor
     * @param container data to release
     */
    void (*release)(struct MafMoAttributeValueContainer_3* container);

    /**
     * returned attribute value. If no data is returned, this shall be set to NULL.
     */
    struct MafMoAttributeValueContainer_3* container;

} MafMoAttributeValueResult_3T;


/**
 * Retrieved data from getAttributes
 */
typedef struct MafMoAttributeValuesResult_3 {

    /**
       * Function invoked by the SPI user to release allocated data
       * The pointer to the function is provided by the SPI implementor
       * @param containers data to release.This is NULL terminated pointer array.
       */
    void (*release)(struct MafMoAttributeValueContainer_3** containers);

    /**
     * returned attribute values. This is NULL terminated pointer array.
     */
    struct MafMoAttributeValueContainer_3** containers;

} MafMoAttributeValuesResult_3T;


/**
 * Managed object interface.
 * This interface implements the model modification operations for
 * managed objects. The ManagedObject interface can be fetched from
 * MafMgmtSpiInterfacePortal using the identity in the
 * MafOamSpiService_1.h header file. The users of
 * MafOamSpiManagedObject may consider using MafOamSpiCmRouter
 * that automatically routes the calls to the component that
 * handles the requested part of the model. The identity of the
 * CM Router and the External CM router is specified in MafOamSpiService_1.h.
 * The External CM router is used by SW using DN:s as defined in the NBI.
 * The implementation of the MafOamSpiManagedObject_1 interface
 * works in close cooperation with the implementation of the
 * MafOamSpiTransactionalResource_1 interface.
 *
 * The implementation of this SPI must ensure the
 * transactional ACID properties, see MafOamSpiTransactionalResource_1.h.
 *
 * It is understood that some middlewares may not support transactions
 * in a way that allows the ACID properties to be always ensured. If
 * so, the implementation must do the best it can to reduce the risk
 * of failure. However, the implementation must at all times ensure
 * safe read, that is, handle the versioning of the data so that
 * the changes done in a transaction are visible in this and only this
 * transaction until it is committed. For example, if a value is set
 * to 5 in a transaction, then when reading the value in the same
 * transaction it must read 5. In a similar way, the objects created
 * must be visible and traversable in the transaction when they have
 * been created. The objects deleted in a transaction must not be
 * visible or traversable in the transaction when they have been deleted.
 *
 * @b Parameters: @n
 * All functions takes as one of the parameters a transaction handle
 * that represents the context in which the operation takes place.
 * The character strings are null-terminated.
 *
 * Memory for out values is in general allocated by the implementer and freed
 * by the implementer when the transaction finishes (that is, when
 * the finish function is called on the TransactionalResource_1
 * interface). The memory allocated by functions  getMoAttribute(),
 * getMoAttributes() and action() shall be released when the SPI user
 * calls a dedicated release function.
 *
 * @b Return: @n
 * A return value other than MafOk is considered a failure.
 * The user of the interface has to decide what strategy to apply for
 * different types of failures.
 *
 * Applications applying batch operations may benefit from
 * failing the complete batch on the first failure and abort the transaction.
 *
 * Applications operating on behalf of an operator issuing operations one at
 * the time, may benefit from taking a more forgiving attitude, allowing the
 * operator to try an fix the problem without aborting the transaction.
 *
 * At failure, a descriptive text is set in
 * the thread context, see MafMgmtSpiThreadContext.h.
 */
typedef struct MafOamSpiManagedObject_3 {
    /**
     * Common interface description. The "base class" for this
     * interface contains the component name, interface name, and
     * interface version.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Sets an attribute value.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format.
     *
     * @param[in] attributeName Attribute name.
     *
     * @param[in] attributeValue Value to set. Any existing value is
     *       completely replaced by the new value. For example, if
     *       the value is a multi valued attribute then all values
     *       in the existing set is removed and replaced by the new
     *       set of values. Furthermore, if the number of values in
     *       the attributeValue container is 0 then it means that
     *       the attribute is unset. The underlying basic
     *       type of derived types should be used when calling this function.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*setMoAttribute)(MafOamSpiTransactionHandleT txHandle,
                                 const char * dn,
                                 const char * attributeName,
                                 const MafMoAttributeValueContainer_3T * attributeValue);

    /**
     * Retrieves the value for an attribute.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format for the MO
     * to get an attribute from.
     *
     * @param[in] attributeName Attribute name..
     *
     * @param[out] result At success, the elemets in the result is set by the SPI implementor.
     *
     * The caller of the function shall invoke the release function provided by the SPI
     * implementation in the result to release allocated memory.
     * The caller shall invoke the release function regardless if returned MafOk
     * The implementor may set the function pointer to NULL to indicate that
     * there is nothing to release.
     *
     * If the attribute exists but contains no value, then the
     * MafMoAttributeValueResult_3T must return a
     * MafMoAttributeValueContainer_3T with nrOfValues set to 0.
     *
     * If the call fails, the implementor shall set the release function and the container
     * in the result to NULL.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     *
     * @n @b Example:
     * @code
     * ...
     * MafOamSpiManagedObject_3T * mib;
     * portal->getInterface(MafCmRouter_1Id,(MafMgmtSpiInterface_1**)&mib);
     * MafOamSpiTransactionMaster_1T * master;
     * portal->getInterface(MafOamSpiTransactionMaster_1Id,master);
     * MafOamSpiTransactionHandleT tx;
     * master->newTransaction(MafLockReadWrite,10, &tx);
     * MafMoAttributeValueResult_3T result;
     * mib->getMoAttribute(tx, "ManagedElement=1", "userLabel", &result);
     * // Use the result
     * if (result.release && result.container) {
     *    result.release(result.container);
     *}
     *
     * master->commit(tx);
     * ...
     * @endcode
     *
     */
    MafReturnT (*getMoAttribute)(MafOamSpiTransactionHandleT txHandle,
                                 const char * dn,
                                 const char * attributeName,
                                 MafMoAttributeValueResult_3T * result);

    /**
     * Retrieves the values for a list of attributes.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format for the MO
     * to get an attribute from.
     *
     * @param[in] attributeNames A NULL terminated array of names to the attributes to get.
     *
     * @param[out] result At success, the elements in the result is set by the SPI implementor.
     * The values are put in a NULL terminated array of pointers to MafMoAttributeValueContainer_3
     * (field containers in the result) and the order is the same as the requested attributeNames.
     *
     * The caller of the function shall invoke the release function provided by the SPI
     * implementation in the result to release allocated memory.
     * The caller shall invoke the release function if the function returned MafOk
     * The implementor may set the function pointer to NULL to indicate that
     * there is nothing to release.
     *
     * If an attribute exists but contains no value, then the
     * MafMoAttributeValuesResult_3T must include a
     * MafMoAttributeValueResult_3T for that attribute containing a
     * MafMoAttributeValueContainer_3T with nrOfValues set to 0.
     *
     * If the call fails, the implementor shall set the release function and the container
     * in the result to NULL.
     *
     * @return MafOk if all attributes could be retrieved, or one of the other MafReturnT return codes.
     *
     * @n @b Example:
     * @code
     * ...
     *  MafOamSpiManagedObject_3T * mib;
     * portal->getInterface(MafCmRouter_1Id,(MafMgmtSpiInterface_1**)&mib);
     * MafOamSpiTransactionMaster_1T * master;
     * portal->getInterface(MafOamSpiTransactionMaster_1Id,master);
     * MafOamSpiTransactionHandleT tx;
     * master->newTransaction(MafLockReadWrite,10, &tx);
     * MafMoAttributesValuesResult_3T result;
     * const char* attributes[3] = {"userLabel", "attr2", NULL};
     *
     * mib->getMoAttributes(tx, "ManagedElement=1", attributes, &result);
     *  // Use the result
     * if (result.release && result.containers) {
     *    result.release(result.containers);
     * }
     *
     * master->commit(tx);
     *
     * @endcode
     *
     */
    MafReturnT (*getMoAttributes)(MafOamSpiTransactionHandleT txHandle,
                                  const char * dn,
                                  const char ** attributeNames,
                                  MafMoAttributeValuesResult_3T * result);

    /**
     * Creates an iterator for iterating over the direct children of the
     * MO specified by the dn. The scope of the iterator is limited by the
     * className parameter. The purpose of this iterator is mainly for
     * supporting those with knowledge of model meta information. For
     * example, a netconf component may use it when scanning the model
     * instance for its structure prior to fetching its contents.
     *
     * Note that its currently the responsibility of the implementation
     * to know which instances exists of the class under the dn, even
     * if the class is a root and the parent is located in another MIB.
     *
     * When using MafOamSpiCmRouter the call will be routed to the
     * implementation that is responsible for the className.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn Parent distinguished name of the children to
     * iterate over. The dn is in 3GPP format.
     * An empty string means that the iterator will iterate over
     * the top level MOs in the MO tree that this implementation
     * of MafOamSpiManagedObject handles. That is, iterate over the
     * roots of the specified className.
     *
     * @param[in] className Class to limit the iteration to.
     *
     * @param[out] result At success, an iterator handle is stored
     * at the location referenced by the result. NOTE! The value of
     * the iterator handle must not be reused until the
     * MafOamSpiTransactionalResource finish function has been
     * called.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*newMoIterator)(MafOamSpiTransactionHandleT txHandle,
                                const char * dn,
                                const char * className,
                                MafOamSpiMoIteratorHandle_3T * result);

    /**
     * Retrieves the next MO.
     *
     * Memory allocated by the implementer of this operation must be
     * deallocated by the implementer when the transaction is finished.
     *
     * Note that its currently the responsibility of the implementation
     * to know which instances exists of the class under the dn, even
     * if the class is a root and the parent is located in another MIB.
     *
     * When using MafOamSpiCmRouter the call will be routed to the
     * implementation that is responsible for the className
     * specified in the newMoIterator call.
     *
     * @param[in] itHandle MO iterator handle.
     *
     * @param[out] result At success, a pointer to the value of
     * the naming attribute is stored at the location referenced
     * by the result, or null if the iterator has no more values.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     *
     * @n @b Example:
     * @code
     * ...
     * MafOamSpiManagedObject_3T * mib;
     * portal->getInterface(MafCmRouter_1Id,(MafMgmtSpiInterface_1**)&mib);
     * MafOamSpiTransactionMaster_1T * master;
     * portal->getInterface(MafOamSpiTransactionMaster_1Id,master);
     * MafOamSpiTransactionHandleT tx;
     * master->newTransaction(MafLockReadWrite,tx);
     * MafMoIteratorHandleT it;
     * mib->newMoIterator(tx, "ManagedElement=1,Fm=1","FmAlarmType",&it);
     * char * key = 0;
     * MafReturnT ret = MafOk;
     * while(ret == MafOk && key == 0){
     *     ret = mib->nextMo(it, &key);
     *     if(ret == MafOk){
     *         ... do something with key ...
     *     }
     *     else{
     *         ... handle error code ...
     *     }
     * };
     * ...
     * @endcode
     */
    MafReturnT (*nextMo)(MafOamSpiMoIteratorHandle_3T itHandle,
                         char **result);

    /**
     * Creates an instance of the className identified by the dn.
     * The implementer must be able to handle the situation that
     * the mandatory and restricted attributes are set later in the same transaction.
     * Each MO class must have a unique key attribute whose name and value
     * must be specified at creation time.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format of the
     * parent MO.
     *
     * @param[in] className Class to create an instance of.
     *
     * @param[in] keyAttributeName Name of the key attribute
     *
     * @param[in] keyAttributeValue Value of the key attribute
     *
     * @param[in] initialAttributes Initial attribute values for the MO. This is a NULL terminated array.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*createMo)(MafOamSpiTransactionHandleT txHandle,
                           const char * parentDn,
                           const char * className,
                           const char * keyAttributeName,
                           const char * keyAttributeValue,
                           MafMoNamedAttributeValueContainer_3T ** initialAttributes);

    /**
     * Recursively deletes the subtree in which the dn is the root.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*deleteMo)(MafOamSpiTransactionHandleT txHandle,
                           const char * dn);

    /**
     * Executes an action. The action may be executed immediately or when
     * the transaction commits. In either case, COM expects the action to return
     * without delay.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn The MO distinguished name in 3GPP format.
     *
     * @param[in] name The name of the action.
     *
     * @param[in] parameters The first parameter in a
     *       null-terminated array of action parameters. The
     *       implementation must interprete the values in the same
     *       order as they appear in the definition of the action
     *       parameters in the Model Repository.
     *
     * @param[out] result At success, a pointer to a MafMoAttributeValueContainerT
     *       instance is stored at the location referenced by result. If the action
     *       has a return value, the container and the release function in the result
     *       are assigned, otherwise these shall be set to NULL. The release function
     *       shall be invoked by the caller when the returned value is processed.
     *
     * @return MafOk, or one of the other MafReturnT return codes. A failure will abort
     * the transaction.
     */
    MafReturnT (*action)(MafOamSpiTransactionHandleT txHandle,
                         const char * dn,
                         const char * name,
                         MafMoNamedAttributeValueContainer_3T **parameters,
                         MafMoAttributeValueResult_3T * result);

    /**
     * Finalize an MO iterator. This makes it possible for the SPI implementor to release
     * memory allocated during iterating.
     *
     * @param[in] itHandle MO iterator handle.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     *
     * @n @b Example:
     * @code
     * ...
     * MafOamSpiManagedObject_3T * mib;
     * portal->getInterface(MafOamSpiExtCmRouterService_3Id ,(MafMgmtSpiInterface_1**)&mib);
     * MafOamSpiTransactionMaster_1T * master;
     * portal->getInterface(MafOamSpiTransactionMaster_2Id,master);
     * MafOamSpiTransactionHandleT tx;
     * master->newTransaction(MafLockReadWrite,tx);
     * MafOamSpiMoIteratorHandle_3T it;
     * mib->newMoIterator(tx, "ManagedElement=1,Fm=1","FmAlarmType",&it);
     * char * key = 0;
     * MafReturnT ret = MafOk;
     * while(ret == MafOk && key == 0){
     *     ret = mib->nextMo(it, &key);
     *     if(ret == MafOk){
     *         ... do something with key ...
     *     }
     *     else{
     *         ... handle error code ...
     *     }
     * };
     * mib->finalizeMoIterator(it);
     * ...
     * @endcode
     */
    MafReturnT (*finalizeMoIterator)(MafOamSpiMoIteratorHandle_3T itHandle);

    /**
      * Checks if an MO instance exists.
      *
      * @param[in] txHandle Transaction handle.
      *
      * @param[in] dn The MO distinguished name in 3GPP format for the MO to check
      * if it exists.
      *
      * @param[out] result At success, this is set to true or false depending on if
      * the MO exists or not.
      *
      * @return MafOk, or one of the other MafReturnT return codes.
      *
      * @n @b Example:
      * @code
      * ...
      * MafOamSpiManagedObject_3T * mib;
      * portal->getInterface(MafOamSpiExtCmRouterService_3Id ,(MafMgmtSpiInterface_1**)&mib);
      * MafOamSpiTransactionMaster_1T * master;
      * portal->getInterface(MafOamSpiTransactionMaster_2Id,master);
      * MafOamSpiTransactionHandleT tx;
      * master->newTransaction(MafLockReadWrite,tx);
      * bool mo_exists = false;
      * MafReturnT ret = MafOk;
      *
      * ret = mib->existsMo(tx, "ManagedElement=1,Fm=1", &mo_exists);
      *
      * if( MafOk == ret && mo_exists ) {
      *     ...
      * } else {
      *     ...
      * }
      * ...
      * @endcode
      */
    MafReturnT (*existsMo)(MafOamSpiTransactionHandleT txHandle,
                           const char * dn,
                           bool * result);


    /**
     * Returns the number of MO instances of a given MO class (MOC) directly below
     * the specified parent.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn The parent distinguished name of the children to
     * iterate over. The dn is in 3GPP format.
     * An empty string means that the iterator will iterate over
     * the top level MOs in the MO tree that this implementation
     * of MafOamSpiManagedObject handles. That is, iterate over the
     * roots of the specified className.
     *
     * @param[in] className Class to limit the count to.
     *
     * @param[out] result At success, this is set to the number of found MO instances.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     *
     * @n @b Example:
     * @code
     * ...
     * MafOamSpiManagedObject_3T * mib;
     * portal->getInterface(MafOamSpiExtCmRouterService_3Id ,(MafMgmtSpiInterface_1**)&mib);
     * MafOamSpiTransactionMaster_1T * master;
     * portal->getInterface(MafOamSpiTransactionMaster_2Id,master);
     * MafOamSpiTransactionHandleT tx;
     * master->newTransaction(MafLockReadWrite,tx);
     * uint64_t numMOs = 0;
     * MafReturnT ret = MafOk;
     *
     * ret = mib->countMoChildren(tx, "ManagedElement=1,Fm=1", "FmAlarmType", &numMOs);
     *
     * if( MafOk == ret && mo_exists ) {
     *     printf( "%d instances of FmAlarmType.\n", numMOs );
     * }
     * ...
     * @endcode
     */
    MafReturnT (*countMoChildren)(MafOamSpiTransactionHandleT txHandle,
                                  const char * dn,
                                  const char * className,
                                  uint64_t * result);

} MafOamSpiManagedObject_3T;

#endif
