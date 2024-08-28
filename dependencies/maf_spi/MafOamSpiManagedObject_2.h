#ifndef MafOamSpiManagedObject_2_h
#define MafOamSpiManagedObject_2_h

#include <MafOamSpiTransaction_1.h>
#include <stdint.h>

#ifndef __cpluplus
#include <stdbool.h>
#endif

/**
 * Managed object interface.
 *
 * @file MafOamSpiManagedObject_2.h
 * @ingroup MafOamSpi
 *
 * Contains functions and data definitions for accessing the MOs.
 *
 * @date 2011-06-15 etazkha,  new version with use of new Transaction Interface and
 * new action function and MafMoNamedAttributeValueContainerT.
 * MafOamSpiManagedObject_2_h is proposed by ehskaja.
 *
 */



/**
 * Defines a void pointer type for generic MO iterator.
 */
#ifndef MafOamSpiMoIteratorHandleT_typedef
#define MafOamSpiMoIteratorHandleT_typedef
typedef void * MafOamSpiMoIteratorHandleT;
#endif
/**
 * Forward declaration.
 */
struct MafMoAttributeValueStructMember_2;
struct MafMoAttributeValueContainer_2;

/**
 * Defines the list of the valid data types.
 */
typedef enum MafOamSpiMoAttributeType_2 {
    /**
     * An 8-bit integer.
     */
    MafOamSpiMoAttributeType_2_INT8 = 1,
    /**
     * A 16-bit integer.
     */
    MafOamSpiMoAttributeType_2_INT16 = 2,
    /**
     * A 32-bit integer.
     */
    MafOamSpiMoAttributeType_2_INT32 = 3,
    /**
     * A 64-bit integer.
     */
    MafOamSpiMoAttributeType_2_INT64 = 4,
    /**
     * An 8-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_2_UINT8 = 5,
    /**
     * A 16-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_2_UINT16 = 6,
    /**
     * A 32-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_2_UINT32 = 7,
    /**
     * A 64-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_2_UINT64 = 8,
    /**
     * A string value.
     */
    MafOamSpiMoAttributeType_2_STRING = 9,
    /**
     * A boolean.
     */
    MafOamSpiMoAttributeType_2_BOOL = 10,
    /**
     * A reference to another Managed Object (MO) class.
     */
    MafOamSpiMoAttributeType_2_REFERENCE = 11,
    /**
     * An enumeration.
     */
    MafOamSpiMoAttributeType_2_ENUM = 12,
    /**
     * A struct or aggregated data type.
     */
    MafOamSpiMoAttributeType_2_STRUCT = 14,
    /**
     * A void data type.
     */
    MafOamSpiMoAttributeType_2_VOID = 15
} MafOamSpiMoAttributeType_2T;

/**
 * A container for one attribute value.
 */
typedef struct MafMoAttributeValue_2 {

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
        char *   theString;
        /**
         * An enumeration.
         */
        int16_t theEnum;
        /**
         * A boolean.
         */
        bool     theBool;
        /**
         * A distinguished name (dn).
         */
        char *   moRef;

        /**
         * A pointer to the first member of the struct.
         */
        struct MafMoAttributeValueStructMember_2 * structMember;
    } value;
} MafMoAttributeValue_2T;

/**
 * Used for the struct members. A struct contains one or more
 * members.
 */
typedef struct MafMoAttributeValueStructMember_2 {

    /**
     * Name of the member.
     */
    char * memberName;

    /**
     * Value of the member.
     */
    struct MafMoAttributeValueContainer_2 * memberValue;

    /**
     * Pointer to the next struct member, or null if
     * there are no more structs.
     */
    struct MafMoAttributeValueStructMember_2 *next;

} MafMoAttributeValueStructMember_2T;

/**
 * A container for an attribute including the type and the value.
 * It can contain a number of attribute values
 * having the same attribute type.
 */
typedef struct MafMoAttributeValueContainer_2 {
    /**
     * Type of the attributes in the container.
     */
    MafOamSpiMoAttributeType_2T type;
    /**
     * Number of values in the container.
     */
    unsigned int nrOfValues;
    /**
     * Pointer to the first element in the array.
     */
    struct MafMoAttributeValue_2 *values;
} MafMoAttributeValueContainer_2T;

/**
 * A Container for named attribute or action attribute values
 *
 */

typedef struct MafMoNamedAttributeValueContainer {
    /**
     * The name of the attribute
     */
    char * name;
    /**
     * The value container
     */
    MafMoAttributeValueContainer_2T value;
} MafMoNamedAttributeValueContainerT;

/**
 * Managed object interface.
 * This interface implements the model modification operations for
 * managed objects. The ManagedObject interface can be fetched from
 * MafMgmtSpiInterfacePortal using the identity in the
 * MafOamSpiService_1.h header file. The users of
 * MafOamSpiManagedObject may consider using MafOamSpiCmRouter
 * that automatically routes the calls to the component that
 * handles the requested part of the model. The MafOamSpiCmRouter
 * is provided in MafOamSpiCmRouter_1.h, and its identity is specified in
 * MafOamSpiService_1.h.
 *
 * The implementation of the MafOamSpiManagedObject_2 interface
 * works in close cooperation with the implementation of the
 * MafOamSpiTransactionalResource_2 interface.
 *
 * The implementation of MafOamSpiManagedObject_2 must ensure the
 * transactional ACID properties, see MafOamSpiTransactionalResource_2.h.
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
 * Memory for out values is allocated by the implementer and freed
 * by the implementer when the transaction finishes (that is, when
 * the finish function is called on the TransactionalResource_1
 * interface).
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
typedef struct MafOamSpiManagedObject_2 {
    /**
     * Common interface description.
     * The "base class" for this interface contains the component
     * name, interface name, and interface version.
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
     *       the attribute is unset. The type of \p attributeValue
     *       must not be set to \c MafOamSpiMoAttributeType_DERIVED or
     *       \c MafOamSpiMoAttributeType_VOID. The underlying basic
     *       type of derived types should be used when calling this function.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*setMoAttribute)(MafOamSpiTransactionHandleT txHandle,
                                 const char * dn,
                                 const char * attributeName,
                                 const MafMoAttributeValueContainer_2T * attributeValue);
    /**
     * Retrieves an attribute value. The values, arrays, and structs
     * returned by pointer values must be deallocated by the implementer of
     * this operation when the transaction is finished.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn MO distinguished name in 3GPP format for the MO
     * to get an attribute from.
     *
     * @param[in] attributeName Name of the attribute to get.
     *
     * @param[out] result At success, a pointer to a MafAttributeValueT
     * instance is stored at the location referenced by the result. If
     * the attribute exists but contains no values, then
     * MafMoAttributeValueContainerT::nrOfValues in the result is set to 0.
     * The type of \p result must not be set to
     * \c MafOamSpiMoAttributeType_DERIVED or \c MafOamSpiMoAttributeType_VOID.
     * The underlying basic type of derived types should be used when calling
     * this function.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoAttribute)(MafOamSpiTransactionHandleT txHandle,
                                 const char * dn,
                                 const char * attributeName,
                                 MafMoAttributeValueContainer_2T  ** result);
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
                                MafOamSpiMoIteratorHandleT *result);
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
     * MafOamSpiManagedObject_2T * totalModel;
     * portal->getInterface(MafCmRouter_1Id,(MafMgmtSpiInterface_1**)&totalModel);
     * MafOamSpiTransactionMaster_1T * master;
     * portal->getInterface(MafOamSpiTransactionMaster_2Id,master);
     * MafOamSpiTransactionHandleT tx;
     * master->newTransaction(MafLockReadWrite,tx);
     * MafMoIteratorHandleT it;
     * totalModel->newMoIterator(tx, "ManagedElement=1,Fm=1","FmAlarmType",&it);
     * char * key = 0;
     * MafReturnT ret = MafOk;
     * while(ret == MafOk && key == 0){
     *     ret = totalModel->nextMo(it, &key);
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
    MafReturnT (*nextMo)(MafOamSpiMoIteratorHandleT itHandle,
                         char **result);
    /**
     * Creates an instance of the className identified by the dn.
     * The implementer must be able to handle the situation that
     * the mandatory attributes are set later in the same transaction.
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
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*createMo)(MafOamSpiTransactionHandleT txHandle,
                           const char * parentDn,
                           const char * className,
                           const char * keyAttributeName,
                           const char * keyAttributevalue);
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
     * the transaction commits. In either case, MAF expects the action to return
     * without delay.
     *
     * @param[in] txHandle Transaction handle.
     *
     * @param[in] dn The MO distinguished name in 3GPP format.
     *
     * @param[in] name The name of the action.
     *
     * @param[in] parameters The first parameter in a
     *       null-terminated array of named action parameters.
     *
     * @param[out] result At success, a pointer to a
     *                   MafMoAttributeValueContainerT
     *                   instance is stored at the location
     *                   referenced by result. The result instance
     *                   must exist during the transaction (until
     *                   commit or abort). The actual action may be
     *                   executed when the action call is made, or
     *                   later, e.g. pre or post, prepare or commit,
     *                   depending on the implementation. So the
     *                   actual execution result can only be
     *                   returned if the action is executed
     *                   immediately. A container must always be
     *                   returned in the result, even if it contains
     *                   no values, in which case the type member of
     *                   \p result must be set to MafOamSpiMoAttributeType_VOID.
     *
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*action)(MafOamSpiTransactionHandleT txHandle,
                         const char * dn,
                         const char * name,
                         MafMoNamedAttributeValueContainerT **parameters,
                         MafMoAttributeValueContainer_2T **result);

} MafOamSpiManagedObject_2T;
/**
 * Identity of the Managed Object SPI implemented by any service.
 */

#endif
