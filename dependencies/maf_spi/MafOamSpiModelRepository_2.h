
#ifndef _MafOamSpiModelRepository_2_h_
#define _MafOamSpiModelRepository_2_h_

#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * @file MafOamSpiModelRepository_2.h
 * @ingroup MafOamSpi
 *
 * Model Repository interface, version 2.
 *
 * This version of the Model Repository interface is designed to support
 * binary level backwards compatibility between this and future versions
 * of the interface.
 *
 * Major changes against first version of Model Repository interface
 * @li Handles and getter functions are used to navigate in the model and
 * for accessing model information.
 * @li A data type container is used to access model information about data types.
 * @li General properties are extended to support domain extension.
 *
 * All return values of the getter functions are according the following pattern
 * @li MafOk if everything went well
 * @li MafNotExist if requested element is not specified in the model or in
 * case of iteration, there are no more elements to retreive.
 * @li MafInvalidArgument if a handle is not valid or used uninitialized
 * or if a return value pointer is 0.
 *
 * The order of all the list elements (like attributes and actions parameters)
 * are enforced to follow the order specified in MAF model files.
 *
 * All the memory allocated during parse of the models and returned by the
 * getter functions are shared between all the users and must not be modified
 * by any of them. The keyword @a const is used where possible to enforce it at
 * some level, still @a cast can be used to violate this restriction.
 *
 * Widely used abbreviations are explained in the document
 * @li MAF Glossary of Terms and Acronyms, TERMINOLOGY, 1/0033-APR 901 0443/1.
 */

/**
 * Defines available data types within Model Repository.
 */
typedef enum MafOamSpiType_2 {
    /**
     * 8-bit integer.
     */
    MafOamSpiTypeInt8 = 1,
    /**
     * 16-bit integer.
     */
    MafOamSpiTypeInt16 = 2,
    /**
     * 32-bit integer.
     */
    MafOamSpiTypeInt32 = 3,
    /**
     * 64-bit integer.
     */
    MafOamSpiTypeInt64 = 4,
    /**
     * 8-bit unsigned integer.
     */
    MafOamSpiTypeUint8 = 5,
    /**
     * 16-bit unsigned integer.
     */
    MafOamSpiTypeUint16 = 6,
    /**
     * 32-bit unsigned integer.
     */
    MafOamSpiTypeUint32 = 7,
    /**
     * 64-bit unsigned integer.
     */
    MafOamSpiTypeUint64 = 8,
    /**
     * String.
     */
    MafOamSpiTypeString = 9,
    /**
     * Boolean.
     */
    MafOamSpiTypeBool = 10,
    /**
     * Reference to another MOC.
     */
    MafOamSpiTypeReference = 11,
    /**
     * Enumeration.
     */
    MafOamSpiTypeEnum = 12,
    /**
     * Derived data type.
     */
    MafOamSpiTypeDerived = 13,
    /**
     * Struct or aggregated data type.
     */
    MafOamSpiTypeStruct = 14,
    /**
     * Void data type. Currently applicable only for return type of actions
     * indicating that there are no return value.
     */
    MafOamSpiTypeVoid = 15
} MafOamSpiType_2T;

/**
 * Defines status properties.
 */
typedef enum MafOamSpiStatus_2 {
    /**
     * The definition is current and valid.
     */
    MafOamSpiStatusCurrent = 1,
    /**
     * The definition is deprecated, but it permits new or continued
     * implementation in order to foster interoperability with older
     * or existing implementations.
     */
    MafOamSpiStatusDeprecated = 2,

    /**
     * The definition is obsolete. It is not recommended to be used in
     * implementations.
     */
    MafOamSpiStatusObsolete = 3
} MafOamSpiStatus_2T;

/**
 * Extension handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiExtensionHandleT;

/**
 * General properties handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiGeneralPropertiesHandleT;

/**
 * MOM handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiMomHandleT;

/**
 * MOC handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiMocHandleT;

/**
 * Attribute handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiAttributeHandleT;

/**
 * Action handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiActionHandleT;

/**
 * Parameter handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiParameterHandleT;

/**
 * Containment handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiContainmentHandleT;

/**
 * Derived data type handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiDerivedHandleT;

/**
 * Value range handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiValueRangeHandleT;

/**
 * Enum handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiEnumHandleT;

/**
 * Enum literal handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiEnumLiteralHandleT;

/**
 * Type container handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiTypeContainerHandleT;

/**
 * Struct handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiStructHandleT;

/**
 * Struct member handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} MafOamSpiStructMemberHandleT;

/**
 * Interface to access model information about general properties.
 */
typedef struct MafOamSpiGeneralProperties_2 {
    /**
     * Gets the name.
     * @param[in] handle A general properties handle.
     * @param[out] name The name.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getName)(MafOamSpiGeneralPropertiesHandleT handle,
                          const char** name);
    /**
     * Gets the description. Contains meaningful description.
     * @param[in] handle A general properties handle.
     * @param[out] description The description.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getDescription)(MafOamSpiGeneralPropertiesHandleT handle,
                                 const char** description);
    /**
     * Gets the specification. Reference to a standard.
     * @param[in] handle A general properties handle.
     * @param[out] specification The specification.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getSpecification)(MafOamSpiGeneralPropertiesHandleT handle,
                                   const char** specification);
    /**
     * Gets the status. Sets MafOamSpiStatusCurrent by default.
     * @param[in] handle A general properties handle.
     * @param[out] status The status.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getStatus)(MafOamSpiGeneralPropertiesHandleT handle,
                            MafOamSpiStatus_2T* status);
    /**
     * Gets the hidden property. It can be used to hide information from the
     * northbound interfaces and the CPI. All modeling elements with
     * the same value for this <em>hidden</em> attribute are considered
     * to form a group and are treated the same with respect to
     * being visible or not in a northbound interface.
     * @param[in] handle a general properties handle.
     * @param[out] hidden The hidden property. If the hidden property is not
     *                    set, NULL is returned.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getHidden)(MafOamSpiGeneralPropertiesHandleT handle,
                            const char** hidden);
    /**
     * Gets the domain of the domain extension.
     * @param[in] handle A general properties handle.
     * @param[out] domain The domain.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getDomain)(MafOamSpiGeneralPropertiesHandleT handle,
                            const char** domain);
    /**
     * Gets the first extenstion of the domain extension.
     * @param[in] handle A general properties handle.
     * @param[out] subHandle An extension handle that refers to the first extension.
     * @return MafOk, MafNotExist if there are no extensions, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getExtension)(MafOamSpiGeneralPropertiesHandleT handle,
                               MafOamSpiExtensionHandleT* subHandle);
    /**
     * Gets the name of an extension.
     * @param[in] handle An extension handle.
     * @param[out] name The name.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getExtensionName)(MafOamSpiExtensionHandleT handle,
                                   const char** name);
    /**
     * Gets the value of an extension.
     * @param[in] handle An extension handle.
     * @param[out] value The value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getExtensionValue)(MafOamSpiExtensionHandleT handle,
                                    const char** value);
    /**
     * Gets the next extension.
     * @param[in] handle An extension handle.
     * @param[out] next An extension handle that refers to the next extension.
     * @return MafOk, MafNotExist if there are no more extensions, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getExtensionNext)(MafOamSpiExtensionHandleT handle,
                                   MafOamSpiExtensionHandleT* next);
} MafOamSpiGeneralProperties_2T;

/**
 * Interface to access model information about MOMs.
 */
typedef struct MafOamSpiMom_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A MOM handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMomHandleT handle,
                                       MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets MOM's one and only root MOC.
     * @param[in] handle A MOM handle.
     * @param[out] subHandle A MOC handle that refers to the root MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getRootMoc)(MafOamSpiMomHandleT handle,
                             MafOamSpiMocHandleT* subHandle);
    /**
     * Gets the version of the MOM. If @a null, then only the name is used to
     * identify the MOM.
     * @param[in] handle A MOM handle.
     * @param[out] version The version.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getVersion)(MafOamSpiMomHandleT handle, const char** version);
    /**
     * Gets the release of the MOM. Within one MOM version there can be
     * several backward-compatible releases.
     * @param[in] handle A MOM handle.
     * @param[out] release The release.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getRelease)(MafOamSpiMomHandleT handle, const char** release);
    /**
     * Gets the URI of the model's own namespace. It is proposed to be in URN
     * format, as defined in RFC 2396.
     * @param[in] handle A MOM handle.
     * @param[out] namespaceUri The URI pf the model's namespace.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getNamespaceUri)(MafOamSpiMomHandleT handle,
                                  const char** namespaceUri);
    /**
     * Gets a globally unique identifier of the namespace. The namespace prefix
     * is normally derived directly from the name of the EcimMom converted to
     * lower case characters.
     * @param[in] handle A MOM handle.
     * @param[out] namespacePrefix A globally unique identifier of the namespace.
     * @return MafOk, or one og the other MafReturnT return codes.
     */
    MafReturnT (*getNamespacePrefix)(MafOamSpiMomHandleT handle,
                                     const char** namespacePrefix);
    /**
     * Gets the MOM document number.
     * @param[in] handle A MOM handle.
     * @param[out] docNo The number.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getDocNo)(MafOamSpiMomHandleT handle, const char** docNo);
    /**
     * Gets the MOM document revision.
     * @param[in] handle A MOM handle.
     * @param[out] revision The revision.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getRevision)(MafOamSpiMomHandleT handle, const char** revision);
    /**
     * Gets the author of the MOM.
     * @param[in] handle A MOM handle.
     * @param[out] author The author.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getAuthor)(MafOamSpiMomHandleT handle, const char** author);
    /**
     * Gets the author's organization.
     * @param[in] handle A MOM handle.
     * @param[out] organization The organization.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getOrganization)(MafOamSpiMomHandleT handle,
                                  const char** organization);
    /**
     * Gets the next MOM.
     * @param[in] handle A MOM handle.
     * @param[out] next A MOM handle that refers to the next MOM.
     * @return MafOk, MafNotExist if there are no more MOMs, or on of the other
     * other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiMomHandleT handle, MafOamSpiMomHandleT* next);
} MafOamSpiMom_2T;

/**
 * Interface to access model information about MOCs.
 */
typedef struct MafOamSpiMoc_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMocHandleT handle,
                                       MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM that the MOC belongs to.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMocHandleT handle,
                         MafOamSpiMomHandleT* subHandle);
    /**
     * Gets the read-only mark. Read-only classes can not be created or deleted
     * from the NBI. A read-only class can contain only read-only attributes.
     * @param[in] handle A MOC handle.
     * @param[out] isReadOnly The mark.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsReadOnly)(MafOamSpiMocHandleT handle, bool* isReadOnly);
    /**
     * Gets the root mark. Indicates if this is the root MOC of the MOM.
     * @param[in] handle A MOC handle.
     * @param[out] isRoot The mark.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsRoot)(MafOamSpiMocHandleT handle, bool* isRoot);
    /**
     * Gets the constraint expressed in the Object Constraint Language (OCL).
     * Note: This is currently no supported since it is not yet known how to
     * validate an OCL expression in MAF.
     * @param[in] handle A MOC handle.
     * @param[out] constraint The constraint.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getConstraint)(MafOamSpiMocHandleT handle,
                                const char** constraint);
    /**
     * Gets the first attribute.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle An attribute handle that refers to the first attribute.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getAttribute)(MafOamSpiMocHandleT handle,
                               MafOamSpiAttributeHandleT* subHandle);
    /**
     * Gets the first action.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle An action handle that refers to the first action.
     * @return MafOk, MafNotExist if there are no actions, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getAction)(MafOamSpiMocHandleT handle,
                            MafOamSpiActionHandleT* subHandle);
    /**
     * Gets the first child containment in the list of child containments for this Moc.
     * @param[in] handle A MOC handle.
     * @param[out] child A containment handle that refers to the first
     * child containment.
     * @return MafOk, MafNotExist if the MOC is a leaf, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getChildContainment)(MafOamSpiMocHandleT handle,
                                      MafOamSpiContainmentHandleT* child);
    /**
     * Gets the first parent containment in the list if parent containments for this Moc.
     * @param[in] handle A MOC handle.
     * @param[out] parent A containment handle that refers to the first
     * parent containment.
     * @return MafOk, MafNotExist if the MOC is root of the top MOM, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getParentContainment)(MafOamSpiMocHandleT handle,
                                       MafOamSpiContainmentHandleT* parent);
    /**
     * Gets the isSystemCreated flag
     * @param[in] handle A MOC handle.
     * @param[out] a boolean isSystemCreated
     */
    MafReturnT (*getIsSystemCreated)(MafOamSpiMocHandleT handle, bool* isSystemCreated);

} MafOamSpiMoc_2T;

/**
 * Interface to access model information about containments.
 */
typedef struct MafOamSpiContainment_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A containment handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiContainmentHandleT handle,
                                       MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM that the containment belongs to.
     * @param[in] handle A containment handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiContainmentHandleT handle,
                         MafOamSpiMomHandleT* subHandle);
    /**
     * Gets the child MOC.
     * @param[in] handle A containment handle.
     * @param[out] child A MOC handle that refers to the child MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getChildMoc)(MafOamSpiContainmentHandleT handle,
                              MafOamSpiMocHandleT* child);
    /**
     * Gets the parent MOC.
     * @param[in] handle A containment handle.
     * @param[out] parent A MOC handle that refers to the parent MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParentMoc)(MafOamSpiContainmentHandleT handle,
                               MafOamSpiMocHandleT* parent);
    /**
     * This function is used for iterating over the parent containments
     * of a certain Moc.To get the first containment
     * in the list the getParentContainment in MafOamSpiMoc_2T must be used.
     * Gets the next containment relation in the list that has the same
     * child MOC.
     * @param[in] handle A containment handle.
     * @param[out] next A containment handle that refers to the next containment.
     * @return MafOk, MafNotExist if there are no more relations, or one of the
     * other MafReturnT returns codes.
     */
    MafReturnT (*getNextContainmentSameChildMoc)(MafOamSpiContainmentHandleT handle,
            MafOamSpiContainmentHandleT* next);
    /**
     * This function is used for iterating over the child containments
     * of a certain  Moc.  To get the first containment
     * in the list the getChildContainment in MafOamSpiMoc_2T must be used.
     * Gets the next containment relation in the list that has the same
     * parent MOC.
     * @param[in] handle A containment handle.
     * @param[out] next A containment handle that refers to the next containment.
     * @return MafOk, MafNotExist if there are no more relations, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNextContainmentSameParentMoc)(MafOamSpiContainmentHandleT handle,
            MafOamSpiContainmentHandleT* next);
    /**
     * Get the is system created mark. Indicates if this containment relationship
     * and the corresponding child MOC is automatically created by the NE.
     * Note: This property is currently not supported.
     * @param[in] handle A containment handle.
     * @param[out] isSystemCreated The mark.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsSystemCreated)(MafOamSpiContainmentHandleT handle,
                                     bool* isSystemCreated);
    /**
     * Gets the cardinality min value. If a value is not defined in the model a
     * default value will be provided.
     * The lower bound of child MOs that can be created under the parent MO.
     * @param[in] handle A containment handle.
     * @param[out] min The min value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getCardinalityMin)(MafOamSpiContainmentHandleT handle,
                                    uint64_t* min);
    /**
     * Gets the cardinality max value. If a value is not defined in the model a
     * default value will be provided.
     * The upper bound of child MOs that can be created under the parent MO.
     * @param[in] handle A containment handle.
     * @param[out] max The max value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getCardinalityMax)(MafOamSpiContainmentHandleT handle,
                                    uint64_t* max);
} MafOamSpiContainment_2T;

/**
 * Interface to access model information about data types. Container is
 * reused by attributes, structs, actions and action parameters.
 */
typedef struct MafOamSpiTypeContainer_2 {
    /**
     * Gets the data type.
     * @param[in] handle A type container handle.
     * @param[out] type The type.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getType)(MafOamSpiTypeContainerHandleT handle,
                          MafOamSpiType_2T* type);
    /**
     * Gets the @a derived data type.
     * @param[in] handle A type container handle.
     * @param[out] subHandle A derived handle that refers to the derived.
     * @return MafOk, MafNotExist if the type is not derived, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getDerived)(MafOamSpiTypeContainerHandleT handle,
                             MafOamSpiDerivedHandleT* subHandle);
    /**
     * Gets the @a enum data type.
     * @param[in] handle A type container handle.
     * @param[out] subHandle An enum handle that refers to the enum.
     * @return MafOk, MafNotExist if the type is not enum, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getEnum)(MafOamSpiTypeContainerHandleT handle,
                          MafOamSpiEnumHandleT* subHandle);
    /**
     * Gets the @a struct data type.
     * @param[in] handle A type container handle.
     * @param[out] subHandle A struct handle that refers to the struct.
     * @return MafOk, MafNotExist if the type is not struct, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getStruct)(MafOamSpiTypeContainerHandleT handle,
                            MafOamSpiStructHandleT* subHandle);
    /**
     * Gets the MOC that is referenced.
     * @param[in] handle A type container handle.
     * @param[out] subHandle A MOC handle that refers to the referenced MOC.
     * @return MafOk, MafNotExist if the type is not reference, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getReferencedMoc)(MafOamSpiTypeContainerHandleT handle,
                                   MafOamSpiMocHandleT* subHandle);
    /**
     * Gets a @a null terminated array of the default values. The values are set
     * automatically at the creation of an element.
     * @param[in] handle A type container handle.
     * @param[out] defaultValue The values.
     * @return MafOk, MafNotExist if there are no default values, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getDefaultValue)(MafOamSpiTypeContainerHandleT handle,
                                  const char*** defaultValue);
    /**
     * Indicates that the order of the values in the set is significant.
     * A set consisting of the same values, but ordered differently,
     * is considered to be a separate entity.
     * Note: This property is not supported since it is currently not part of
     * the mp.dtd definition.
     * @param[in] handle A type container handle.
     * @param[out] isOrdered The is ordered indication.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsOrdered)(MafOamSpiTypeContainerHandleT handle,
                               bool* isOrdered);

    /**
     * Gets the unique mark. Indicates that the values are unique within the
     * value container (for example within values of multi-value attribute)
     * @param[in] handle A type container handle.
     * @param[out] isUnique The mark.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsUnique)(MafOamSpiTypeContainerHandleT handle,
                              bool* isUnique);
    /**
     * Gets the multiplicity min value. If a value is not defined in the model
     * a default value will be provided.
     * The lower bound of the values that the element contains. If the lower
     * bound is 0, a value does not have to be supplied at creation.
     * Note: If the multiplicity lower bound is 1 (or higher), then the
     * mandatory property equals true. If the multiplicity lower bound is 0,
     * then the mandatory property equals false.
     * @param[in] handle A type container handle.
     * @param[out] min The min value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMultiplicityMin)(MafOamSpiTypeContainerHandleT handle,
                                     uint64_t* min);
    /**
     * Gets the multiplicity max value. If a value is not defined in the model
     * a default value will be provided.
     * The upper bound of the values that the element contains.
     * @param[in] handle A type container handle.
     * @param[out] max The max value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMultiplicityMax)(MafOamSpiTypeContainerHandleT handle,
                                     uint64_t* max);
} MafOamSpiTypeContainer_2T;

/**
 * Interface to access model information about derived data types.
 */
typedef struct MafOamSpiDerived_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiDerivedHandleT handle,
                                       MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM a derived belongs to.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiDerivedHandleT handle,
                         MafOamSpiMomHandleT* subHandle);
    /**
     * Gets the data type.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A type handle that refers to the type.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getType)(MafOamSpiDerivedHandleT handle,
                          MafOamSpiType_2T* type);
    /**
     * Gets the POSIX Basic Regular Expression compatible pattern. The
     * property is optional and applicable only if the type is string.
     * @param[in] handle A derived handle.
     * @param[out] pattern The pattern.
     * @return MafOk, MafNotExist if there is no pattern, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getStringRestrictionPattern)(MafOamSpiDerivedHandleT handle,
            const char** pattern);
    /**
     * Gets the string restriction length. The property is optional and
     * applicable only if the type is string. Use numeric range for numbers.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A range handle that refers to the range.
     * @return MafOk, MafNotExist if there is no length restriction, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getStringRestrictionLength)(MafOamSpiDerivedHandleT handle,
            MafOamSpiValueRangeHandleT* subHandle);
    /**
     * Gets numeric restriction length. The property is optional and applicable
     * only if the type is numeric. Use length restriction for strings.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A value range handle that refers to the value range.
     * @return MafOk, MafNotExist if there is no value range, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNumericRestrictionRange)(MafOamSpiDerivedHandleT handle,
            MafOamSpiValueRangeHandleT* subHandle);
    /**
     * Gets min value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] min The min value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMin)(MafOamSpiValueRangeHandleT handle, int64_t* min);
    /**
     * Gets max value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] max The max value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMax)(MafOamSpiValueRangeHandleT handle, int64_t* max);
    /**
     * Gets the next value range.
     * @param[in] handle A value range handle.
     * @param[out] next A value range handle that refers to the next value range.
     * @return MafOk, MafNotExist if there are no more value ranges, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeNext)(MafOamSpiValueRangeHandleT handle,
                                    MafOamSpiValueRangeHandleT* next);
} MafOamSpiDerived_2T;

/**
 * Interface to access model information about enums.
 */
typedef struct MafOamSpiEnum_2 {
    /**
     * Gets the general properties.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiEnumHandleT handle,
                                       MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM an enum belongs to.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiEnumHandleT handle, MafOamSpiMomHandleT* subHandle);
    /**
     * Gets the first literal.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A literal handle that refers to the first literal.
     * @return MafOk, MafNotExist it there are no literals, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getLiteral)(MafOamSpiEnumHandleT handle,
                             MafOamSpiEnumLiteralHandleT* subHandle);
    /**
     * Gets the general properties of a literal.
     * @param[in] handle A literal handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getLiteralGeneralProperties)(MafOamSpiEnumLiteralHandleT handle,
            MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the value of a literal.
     * @param[in] handle A literal handle.
     * @param[out] value The value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getLiteralValue)(MafOamSpiEnumLiteralHandleT handle, int16_t* value);
    /**
     * Gets the next literal.
     * @param[in] handle A literal handle.
     * @param[out] next A literal handle that refers to the next literal.
     * @return MafOk, MafNotExist of there are no more literals, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getLiteralNext)(MafOamSpiEnumLiteralHandleT handle,
                                 MafOamSpiEnumLiteralHandleT* next);
} MafOamSpiEnum_2T;

/**
 * Interface to access model information about structs.
 */
typedef struct MafOamSpiStruct_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A struct handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiStructHandleT handle,
                                       MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM a struct belongs to.
     * @param[in] handle A struct handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiStructHandleT handle,
                         MafOamSpiMomHandleT* subHandle);
    /**
     * Gets the first member of a struct.
     * @param[in] handle A struct handle.
     * @param[out] subHandle A member handle that refers to the member.
     * @return MafOk, MafNotExist if there are no members, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getMember)(MafOamSpiStructHandleT handle,
                            MafOamSpiStructMemberHandleT* subHandle);
    /**
     * Gets the general properties of a member.
     * @param[in] handle A member handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMemberGeneralProperties)(MafOamSpiStructMemberHandleT handle,
            MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the type container of a member.
     * @param[in] handle A member handle.
     * @param[out] subHandle A type container that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMemberTypeContainer)(MafOamSpiStructMemberHandleT handle,
                                         MafOamSpiTypeContainerHandleT* subHandle);
    /**
     * Gets the next member.
     * @param[in] handle A member handle.
     * @param[out] next A member handle that refers to the next member.
     * @return MafOk, MafNotExist if there are no more members, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getMemberNext)(MafOamSpiStructMemberHandleT handle,
                                MafOamSpiStructMemberHandleT* next);
} MafOamSpiStruct_2T;

/**
 * Interface to access model information about attributes.
 */
typedef struct MafOamSpiAttribute_2 {
    /**
     * Gets the general properties.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiAttributeHandleT handle,
                                       MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOC an attribute belongs to.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A MOC handle that refers to the MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoc)(MafOamSpiAttributeHandleT handle,
                         MafOamSpiMocHandleT* subHandle);
    /**
     * Gets the type container.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A type container handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getTypeContainer)(MafOamSpiAttributeHandleT handle,
                                   MafOamSpiTypeContainerHandleT* subHandle);
    /**
     * Gets an indication if this is a key (naming) attribute of the MOC it
     * belongs to. There must be one and only one key attribute per every MOC.
     * @param[in] handle An attribute handle.
     * @param[out] isKey The is key indication.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsKey)(MafOamSpiAttributeHandleT handle, bool* isKey);
    /**
     * Gets an indication if an attribute is mandatory. An attribute must be
     * provided at create of a MO. If mandatory, then default must not be
     * specified. If not mandatory, then the attribute value does not have to
     * be supplied at creation of a MO.
     * @param[in] handle An attribute handle.
     * @param[out] isMandatory The is mandatory indication.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsMandatory)(MafOamSpiAttributeHandleT handle, bool* isMandatory);
    /**
     * Gets an indication if an attribute is persistent. Indicates that the
     * attribute value survives a restart (that is, a power cycle) of the
     * network element. This is only applicable to read-only attributes
     * since all configuration attributes are assumed to be persistent.
     * @param[in] handle An attributes handle.
     * @param[out] isPersistent The is persistent indication.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsPersistent)(MafOamSpiAttributeHandleT handle, bool* isPersistent);
    /**
     * Gets an indication if an attributes is read-only. If isReadOnly equals true,
     * then the attribute is only for read; the system provides updates internally.
     *
     * @param[in] handle An attributes handle.
     * @param[out] isReadOnly The is readonly indication.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsReadOnly)(MafOamSpiAttributeHandleT handle, bool* isReadOnly);
    /**
     * Gets the string with the unit used for the attribute type. The unit is
     * a standard measure of a quantity.
     * @param[in] handle An attribute handle.
     * @param[out] unit The unit.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getUnit)(MafOamSpiAttributeHandleT handle, const char** unit);
    /**
     * Gets the next attribute.
     * @param[in] handle An attribute handle.
     * @param[out] next An attribute handle that refers to the next attribute.
     * @return MafOk, MafNotExist if there are no more attributes, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiAttributeHandleT handle,
                          MafOamSpiAttributeHandleT* next);

    /**
     * Gets an indication if the attribute has restricted property set.
     * @param[in] handle An attribute handle.
     * @param[out] isRestricted The indication.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsRestricted)(MafOamSpiAttributeHandleT handle, bool* isRestricted);

    /**
     * Gets an indication if the attribute generates value change notification.
     * @param[in] handle An attribute handle.
     * @param[out] isNotifiable The indication.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getIsNotifiable)(MafOamSpiAttributeHandleT handle, bool* isNotifiable);

} MafOamSpiAttribute_2T;

/**
 * Interface to access model information about actions.
 */
typedef struct MafOamSpiAction_2 {
    /**
     * Gets the general properties.
     * @param[in] handle An action handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiActionHandleT handle,
                                       MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOC that an action belongs to.
     * @param[in] handle An action handle.
     * @param[out] subHandle A MOC handle that refers to the MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoc)(MafOamSpiActionHandleT handle,
                         MafOamSpiMocHandleT* subHandle);
    /**
     * Gets the type container of the action result value.
     * @param[in] handle An action handle.
     * @param[out] subHandle A type container handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getTypeContainer)(MafOamSpiActionHandleT handle,
                                   MafOamSpiTypeContainerHandleT* subHandle);

    /**
     * Gets the next action.
     * @param[in] handle An action handle.
     * @param[out] next An action handle that refers to the next action.
     * @return MafOk, MafNotExist if there are no more actions, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiActionHandleT handle,
                          MafOamSpiActionHandleT* next);
    /**
     * Gets the first parameter of an action.
     * @param[in] handle An action handle.
     * @param[out] subHandle A parameter handle that refers to the first parameter.
     * @return MafOk, MafNotExist if there is no parameters, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getParameter)(MafOamSpiActionHandleT handle,
                               MafOamSpiParameterHandleT* subHandle);
    /**
     * Gets the general properties of a parameter.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterGeneralProperties)(MafOamSpiParameterHandleT handle,
            MafOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the action that a parameter belongs to.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle An action handle that refers to the action.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterAction)(MafOamSpiParameterHandleT handle,
                                     MafOamSpiActionHandleT* subHandle);
    /**
     * Gets the type container of a parameter.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle A type handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterTypeContainer)(MafOamSpiParameterHandleT handle,
                                            MafOamSpiTypeContainerHandleT* subHandle);
    /**
     * Gets the next parameter.
     * @param[in] handle A parameter handle.
     * @param[out] next A parameter handle that refers to the next parameter.
     * @return MafOk, MafNotExist if there are no more parameters, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getParameterNext)(MafOamSpiParameterHandleT handle,
                                   MafOamSpiParameterHandleT* next);
} MafOamSpiAction_2T;

/**
 * Interface to access first step modeling elements like MOMs and MOCs.
 */
typedef struct MafOamSpiEntry_2 {
    /**
     * Gets the first MOM that has been registered in the Model Repository.
     * @param[out] handle A MOM handle that refers to the first MOM.
     * @return MafOk if at least one MOM was found, MafNotExist if no MOMs are
     * available, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMomHandleT* handle);
    /**
     * Gets the requested MOC.
     * @param[in] momName The name of the MOM.
     * @param[in] momVersion The version of the MOM. The argument is optional,
     * if it's @a null, then the latest version is used.
     * @param[in] mocName The name of the MOC.
     * @param[out] handle A MOC handle that refers to the requested MOC.
     * @return MafOk if the MOC was found, MafNotExist if it wasn't found, or
     * one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoc)(const char* momName, const char* momVersion,
                         const char* mocName, MafOamSpiMocHandleT* handle);
    /**
     * Gets the root MOC of the top MOM.
     * @param[out] handle A MOC handle that refers to the root MOC.
     * @return MafOk if the root MOC was found, MafNotExist if it wasn't found,
     * or one of the other MafReturnT return codes.
     */
    MafReturnT (*getRoot)(MafOamSpiMocHandleT* handle);
} MafOamSpiEntry_2T;

/**
 * Second version of Model Repository (MR) interface. Contains initialized
 * instances of all the other MR structures forming an interface to dynamically
 * access model information from parsed MAF model files.
 *
 * @code
 * // Simple how-to use the interface examples
 *
 * // Get a handle to the first mom in the list of moms
 * MafOamSpiMomHandleT momHandle;
 * MafReturnT retVal = mr->entry->getMom(&momHandle);
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 *
 * // Use the handle to access mim attributes
 * const char* version = 0;
 * retVal = mr->mom->getVersion(momHandle, &version);
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 *
 * // Get a handle to the root moc
 * MafOamSpiMocHandleT mocHandle;
 * retVal = mr->mom->getRootMoc(momHandle, &mocHandle);
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 *
 * // Iterate over the root moc's attributes
 * MafOamSpiAttributeHandleT attrHandle;
 * retVal = mr->moc->getAttribute(mocHandle, &attrHandle);
 * while (retVal == MafOk) {
 *     bool isKey = false;
 *     retVal = mr->attribute->getIsKey(attrHandle, &isKey);
 *     if (retVal != MafOk) {
 *         // Handle error code
 *     }
 *     if (isKey) {
 *         // This is key attribute
 *     }
 *     retVal = mr->attribute->getNext(attrHandle, &attrHandle);
 * }
 *
 * // Traverse the tree of mocs
 * MafOamSpiContainmentHandleT childContainmentHandle;
 * retVal = mr->moc->getChildContainment(mocHandle, &childContainmentHandle));
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 * MafOamSpiMocHandleT childMocHandle;
 * retVal = mr->containment->getChildMoc(childContainmentHandle, &childMocHandle));
 * MafOamSpiContainmentHandleT parentOfChildContainmentHandle;
 * retVal = mr->moc->getParentContainment(childMocHandle, &parentOfChildContainmentHandle));
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 *
 * @endcode
 */
typedef struct MafOamSpiModelRepository_2 {
    /**
     * Interface identification.
     */
    MafMgmtSpiInterface_1T base;
    /**
     * Interface for accessing entry definitions.
     */
    MafOamSpiEntry_2T* entry;
    /**
     * Interface for accessing general properties definitions.
     */
    MafOamSpiGeneralProperties_2T* generalProperties;
    /**
     * Interface for accessing MOM definitions.
     */
    MafOamSpiMom_2T* mom;
    /**
     * Interface for accessing MOC definitions.
     */
    MafOamSpiMoc_2T* moc;
    /**
     * Interface for accessing containment definitions.
     */
    MafOamSpiContainment_2T* containment;
    /**
     * Interface for accessing type container definitions.
     */
    MafOamSpiTypeContainer_2T* typeContainer;
    /**
     * Interface for accessing derived data type definitions.
     */
    MafOamSpiDerived_2T* derivedType;
    /**
     * Interface for accessing enum definitions.
     */
    MafOamSpiEnum_2T* enumType;
    /**
     * Interface for accessing struct definitions.
     */
    MafOamSpiStruct_2T* structType;
    /**
     * Interface for accessing attribute definitions.
     */
    MafOamSpiAttribute_2T* attribute;
    /**
     * Interface for accessing action definitions.
     */
    MafOamSpiAction_2T* action;
} MafOamSpiModelRepository_2T;

#endif // _MafOamSpiModelRepository_2_h_

