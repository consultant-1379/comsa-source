
#ifndef _ComOamSpiModelRepository_2_h_
#define _ComOamSpiModelRepository_2_h_

#include <ComMgmtSpiCommon.h>
#include <ComMgmtSpiInterface_1.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * @file ComOamSpiModelRepository_2.h
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
 * @li ComOk if everything went well
 * @li ComNotExist if requested element is not specified in the model or in
 * case of iteration, there are no more elements to retrieve.
 * @li ComInvalidArgument if a handle is not valid or used uninitialized
 * or if a return value pointer is 0.
 *
 * The order of all the list elements (like attributes and actions parameters)
 * is enforced to follow the order specified in COM model files.
 *
 * All the memory allocated during parse of the models and returned by the
 * getter functions are shared between all the users and must not be modified
 * by any of them. The keyword @a const is used where possible to enforce it at
 * some level, still @a cast can be used to violate this restriction.
 *
 * Widely used abbreviations are explained in the document
 * @li COM Glossary of Terms and Acronyms, TERMINOLOGY, 1/0033-APR 901 0443/1.
 */

/**
 * Defines available data types within Model Repository.
 */
typedef enum ComOamSpiType_2 {
    /**
     * 8-bit integer.
     */
    ComOamSpiTypeInt8 = 1,
    /**
     * 16-bit integer.
     */
    ComOamSpiTypeInt16 = 2,
    /**
     * 32-bit integer.
     */
    ComOamSpiTypeInt32 = 3,
    /**
     * 64-bit integer.
     */
    ComOamSpiTypeInt64 = 4,
    /**
     * 8-bit unsigned integer.
     */
    ComOamSpiTypeUint8 = 5,
    /**
     * 16-bit unsigned integer.
     */
    ComOamSpiTypeUint16 = 6,
    /**
     * 32-bit unsigned integer.
     */
    ComOamSpiTypeUint32 = 7,
    /**
     * 64-bit unsigned integer.
     */
    ComOamSpiTypeUint64 = 8,
    /**
     * String.
     */
    ComOamSpiTypeString = 9,
    /**
     * Boolean.
     */
    ComOamSpiTypeBool = 10,
    /**
     * Reference to another MOC.
     */
    ComOamSpiTypeReference = 11,
    /**
     * Enumeration.
     */
    ComOamSpiTypeEnum = 12,
    /**
     * Derived data type.
     */
    ComOamSpiTypeDerived = 13,
    /**
     * Struct or aggregated data type.
     */
    ComOamSpiTypeStruct = 14,
    /**
     * Void data type. Currently applicable only for return type of actions
     * indicating that there is no return value.
     */
    ComOamSpiTypeVoid = 15
} ComOamSpiType_2T;

/**
 * Defines status properties.
 */
typedef enum ComOamSpiStatus_2 {
    /**
     * The definition is current and valid.
     */
    ComOamSpiStatusCurrent = 1,
    /**
     * The definition is deprecated, but it permits new or continued
     * implementation in order to foster interoperability with older
     * or existing implementations.
     */
    ComOamSpiStatusDeprecated = 2,

    /**
     * The definition is obsolete. It is not recommended to be used in
     * implementations.
     */
    ComOamSpiStatusObsolete = 3
} ComOamSpiStatus_2T;

/**
 * Extension handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiExtensionHandleT;

/**
 * General properties handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiGeneralPropertiesHandleT;

/**
 * MOM handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiMomHandleT;

/**
 * MOC handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiMocHandleT;

/**
 * Attribute handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiAttributeHandleT;

/**
 * Action handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiActionHandleT;

/**
 * Parameter handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiParameterHandleT;

/**
 * Containment handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiContainmentHandleT;

/**
 * Derived data type handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiDerivedHandleT;

/**
 * Value range handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiValueRangeHandleT;

/**
 * Enum handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiEnumHandleT;

/**
 * Enum literal handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiEnumLiteralHandleT;

/**
 * Type container handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiTypeContainerHandleT;

/**
 * Struct handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiStructHandleT;

/**
 * Struct member handle.
 */
typedef struct {
    /**
     * Identification of handle.
     */
    uint64_t handle;
} ComOamSpiStructMemberHandleT;

/**
 * Interface to access model information about general properties.
 */
typedef struct ComOamSpiGeneralProperties_2 {
    /**
     * Gets the name.
     * @param[in] handle A general properties handle.
     * @param[out] name The name.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getName)(ComOamSpiGeneralPropertiesHandleT handle,
                          const char** name);
    /**
     * Gets the description. Contains meaningful description.
     * @param[in] handle A general properties handle.
     * @param[out] description The description.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getDescription)(ComOamSpiGeneralPropertiesHandleT handle,
                                 const char** description);
    /**
     * Gets the specification. Reference to a standard.
     * @param[in] handle A general properties handle.
     * @param[out] specification The specification.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getSpecification)(ComOamSpiGeneralPropertiesHandleT handle,
                                   const char** specification);
    /**
     * Gets the status. Sets ComOamSpiStatusCurrent by default.
     * @param[in] handle A general properties handle.
     * @param[out] status The status.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getStatus)(ComOamSpiGeneralPropertiesHandleT handle,
                            ComOamSpiStatus_2T* status);
    /**
     * Gets the hidden property. It can be used to hide information from the
     * northbound interfaces and the CPI. All modeling elements with
     * the same value for this <em>hidden</em> attribute are considered
     * to form a group and are treated the same with respect to
     * being visible or not in a northbound interface.
     * @param[in] handle a general properties handle.
     * @param[out] hidden The hidden property. If the hidden property is not
     *                    set, NULL is returned.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getHidden)(ComOamSpiGeneralPropertiesHandleT handle,
                            const char** hidden);
    /**
     * Gets the domain of the domain extension.
     * @param[in] handle A general properties handle.
     * @param[out] domain The domain.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getDomain)(ComOamSpiGeneralPropertiesHandleT handle,
                            const char** domain);
    /**
     * Gets the first extension of the domain extension.
     * @param[in] handle A general properties handle.
     * @param[out] subHandle An extension handle that refers to the first extension.
     * @return ComOk, ComNotExist if there are no extensions, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getExtension)(ComOamSpiGeneralPropertiesHandleT handle,
                               ComOamSpiExtensionHandleT* subHandle);
    /**
     * Gets the name of an extension.
     * @param[in] handle An extension handle.
     * @param[out] name The name.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getExtensionName)(ComOamSpiExtensionHandleT handle,
                                   const char** name);
    /**
     * Gets the value of an extension.
     * @param[in] handle An extension handle.
     * @param[out] value The value.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getExtensionValue)(ComOamSpiExtensionHandleT handle,
                                    const char** value);
    /**
     * Gets the next extension.
     * @param[in] handle An extension handle.
     * @param[out] next An extension handle that refers to the next extension.
     * @return ComOk, ComNotExist if there are no more extensions, or one of
     * the other ComReturnT return codes.
     */
    ComReturnT (*getExtensionNext)(ComOamSpiExtensionHandleT handle,
                                   ComOamSpiExtensionHandleT* next);
} ComOamSpiGeneralProperties_2T;

/**
 * Interface to access model information about MOMs.
 */
typedef struct ComOamSpiMom_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A MOM handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getGeneralProperties)(ComOamSpiMomHandleT handle,
                                       ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets MOM's one and only root MOC.
     * @param[in] handle A MOM handle.
     * @param[out] subHandle A MOC handle that refers to the root MOC.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getRootMoc)(ComOamSpiMomHandleT handle,
                             ComOamSpiMocHandleT* subHandle);
    /**
     * Gets the version of the MOM. If @a null, then only the name is used to
     * identify the MOM.
     * @param[in] handle A MOM handle.
     * @param[out] version The version.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getVersion)(ComOamSpiMomHandleT handle, const char** version);
    /**
     * Gets the release of the MOM. Within one MOM version there can be
     * several backward-compatible releases.
     * @param[in] handle A MOM handle.
     * @param[out] release The release.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getRelease)(ComOamSpiMomHandleT handle, const char** release);
    /**
     * Gets the URI of the model's own namespace. It is proposed to be in URN
     * format, as defined in RFC 2396.
     * @param[in] handle A MOM handle.
     * @param[out] namespaceUri The URI of the model's namespace.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getNamespaceUri)(ComOamSpiMomHandleT handle,
                                  const char** namespaceUri);
    /**
     * Gets a globally unique identifier of the namespace. The namespace prefix
     * is normally derived directly from the name of the EcimMom converted to
     * lower case characters.
     * @param[in] handle A MOM handle.
     * @param[out] namespacePrefix A globally unique identifier of the namespace.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getNamespacePrefix)(ComOamSpiMomHandleT handle,
                                     const char** namespacePrefix);
    /**
     * Gets the MOM document number.
     * @param[in] handle A MOM handle.
     * @param[out] docNo The number.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getDocNo)(ComOamSpiMomHandleT handle, const char** docNo);
    /**
     * Gets the MOM document revision.
     * @param[in] handle A MOM handle.
     * @param[out] revision The revision.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getRevision)(ComOamSpiMomHandleT handle, const char** revision);
    /**
     * Gets the author of the MOM.
     * @param[in] handle A MOM handle.
     * @param[out] author The author.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getAuthor)(ComOamSpiMomHandleT handle, const char** author);
    /**
     * Gets the author's organization.
     * @param[in] handle A MOM handle.
     * @param[out] organization The organization.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getOrganization)(ComOamSpiMomHandleT handle,
                                  const char** organization);
    /**
     * Gets the next MOM.
     * @param[in] handle A MOM handle.
     * @param[out] next A MOM handle that refers to the next MOM.
     * @return ComOk, ComNotExist if there are no more MOMs, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getNext)(ComOamSpiMomHandleT handle, ComOamSpiMomHandleT* next);
} ComOamSpiMom_2T;

/**
 * Interface to access model information about MOCs.
 */
typedef struct ComOamSpiMoc_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getGeneralProperties)(ComOamSpiMocHandleT handle,
                                       ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM that the MOC belongs to.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMom)(ComOamSpiMocHandleT handle,
                         ComOamSpiMomHandleT* subHandle);
    /**
     * Gets the read-only mark. Read-only classes can not be created or deleted
     * from the NBI. A read-only class can contain only read-only attributes.
     * @param[in] handle A MOC handle.
     * @param[out] isReadOnly The mark.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsReadOnly)(ComOamSpiMocHandleT handle, bool* isReadOnly);
    /**
     * Gets the root mark. Indicates if this is the root MOC of the MOM.
     * @param[in] handle A MOC handle.
     * @param[out] isRoot The mark.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsRoot)(ComOamSpiMocHandleT handle, bool* isRoot);
    /**
     * Gets the constraint expressed in the Object Constraint Language (OCL).
     * Note: This is currently no supported since it is not yet known how to
     * validate an OCL expression in COM.
     * @param[in] handle A MOC handle.
     * @param[out] constraint The constraint.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getConstraint)(ComOamSpiMocHandleT handle,
                                const char** constraint);
    /**
     * Gets the first attribute.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle An attribute handle that refers to the first attribute.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getAttribute)(ComOamSpiMocHandleT handle,
                               ComOamSpiAttributeHandleT* subHandle);
    /**
     * Gets the first action.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle An action handle that refers to the first action.
     * @return ComOk, ComNotExist if there are no actions, or one of the other
     * ComReturnT return codes.
     */
    ComReturnT (*getAction)(ComOamSpiMocHandleT handle,
                            ComOamSpiActionHandleT* subHandle);
    /**
     * Gets the first child containment in the list of child containments for this Moc.
     * @param[in] handle A MOC handle.
     * @param[out] child A containment handle that refers to the first
     * child containment.
     * @return ComOk, ComNotExist if the MOC is a leaf, or one of the other
     * ComReturnT return codes.
     */
    ComReturnT (*getChildContainment)(ComOamSpiMocHandleT handle,
                                      ComOamSpiContainmentHandleT* child);
    /**
     * Gets the first parent containment in the list if parent containments for this Moc.
     * @param[in] handle A MOC handle.
     * @param[out] parent A containment handle that refers to the first
     * parent containment.
     * @return ComOk, ComNotExist if the MOC is root of the top MOM, or one of
     * the other ComReturnT return codes.
     */
    ComReturnT (*getParentContainment)(ComOamSpiMocHandleT handle,
                                       ComOamSpiContainmentHandleT* parent);
    /**
     * Gets the isSystemCreated flag
     * @param[in] handle A MOC handle.
     * @param[out] a boolean isSystemCreated
     */
    ComReturnT (*getIsSystemCreated)(ComOamSpiMocHandleT handle, bool* isSystemCreated);

} ComOamSpiMoc_2T;

/**
 * Interface to access model information about containments.
 */
typedef struct ComOamSpiContainment_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A containment handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getGeneralProperties)(ComOamSpiContainmentHandleT handle,
                                       ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM that the containment belongs to.
     * @param[in] handle A containment handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMom)(ComOamSpiContainmentHandleT handle,
                         ComOamSpiMomHandleT* subHandle);
    /**
     * Gets the child MOC.
     * @param[in] handle A containment handle.
     * @param[out] child A MOC handle that refers to the child MOC.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getChildMoc)(ComOamSpiContainmentHandleT handle,
                              ComOamSpiMocHandleT* child);
    /**
     * Gets the parent MOC.
     * @param[in] handle A containment handle.
     * @param[out] parent A MOC handle that refers to the parent MOC.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getParentMoc)(ComOamSpiContainmentHandleT handle,
                               ComOamSpiMocHandleT* parent);
    /**
     * This function is used for iterating over the parent containments
     * of a certain Moc.To get the first containment
     * in the list the getParentContainment in ComOamSpiMoc_2T must be used.
     * Gets the next containment relation in the list that has the same
     * child MOC.
     * @param[in] handle A containment handle.
     * @param[out] next A containment handle that refers to the next containment.
     * @return ComOk, ComNotExist if there are no more relations, or one of the
     * other ComReturnT returns codes.
     */
    ComReturnT (*getNextContainmentSameChildMoc)(ComOamSpiContainmentHandleT handle,
            ComOamSpiContainmentHandleT* next);
    /**
     * This function is used for iterating over the child containments
     * of a certain  Moc.  To get the first containment
     * in the list the getChildContainment in ComOamSpiMoc_2T must be used.
     * Gets the next containment relation in the list that has the same
     * parent MOC.
     * @param[in] handle A containment handle.
     * @param[out] next A containment handle that refers to the next containment.
     * @return ComOk, ComNotExist if there are no more relations, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getNextContainmentSameParentMoc)(ComOamSpiContainmentHandleT handle,
            ComOamSpiContainmentHandleT* next);
    /**
     * Get the is system created mark. Indicates if this containment relationship
     * and the corresponding child MOC is automatically created by the NE.
     * Note: This property is currently not supported.
     * @param[in] handle A containment handle.
     * @param[out] isSystemCreated The mark.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsSystemCreated)(ComOamSpiContainmentHandleT handle,
                                     bool* isSystemCreated);
    /**
     * Gets the cardinality min value. If a value is not defined in the model a
     * default value will be provided.
     * The lower bound of child MOs that can be created under the parent MO.
     * @param[in] handle A containment handle.
     * @param[out] min The min value.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getCardinalityMin)(ComOamSpiContainmentHandleT handle,
                                    uint64_t* min);
    /**
     * Gets the cardinality max value. If a value is not defined in the model a
     * default value will be provided.
     * The upper bound of child MOs that can be created under the parent MO.
     * @param[in] handle A containment handle.
     * @param[out] max The max value.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getCardinalityMax)(ComOamSpiContainmentHandleT handle,
                                    uint64_t* max);
} ComOamSpiContainment_2T;

/**
 * Interface to access model information about data types. Container is
 * reused by attributes, structs, actions and action parameters.
 */
typedef struct ComOamSpiTypeContainer_2 {
    /**
     * Gets the data type.
     * @param[in] handle A type container handle.
     * @param[out] type The type.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getType)(ComOamSpiTypeContainerHandleT handle,
                          ComOamSpiType_2T* type);
    /**
     * Gets the @a derived data type.
     * @param[in] handle A type container handle.
     * @param[out] subHandle A derived handle that refers to the derived.
     * @return ComOk, ComNotExist if the type is not derived, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getDerived)(ComOamSpiTypeContainerHandleT handle,
                             ComOamSpiDerivedHandleT* subHandle);
    /**
     * Gets the @a enum data type.
     * @param[in] handle A type container handle.
     * @param[out] subHandle An enum handle that refers to the enum.
     * @return ComOk, ComNotExist if the type is not enum, or one of the other
     * ComReturnT return codes.
     */
    ComReturnT (*getEnum)(ComOamSpiTypeContainerHandleT handle,
                          ComOamSpiEnumHandleT* subHandle);
    /**
     * Gets the @a struct data type.
     * @param[in] handle A type container handle.
     * @param[out] subHandle A struct handle that refers to the struct.
     * @return ComOk, ComNotExist if the type is not struct, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getStruct)(ComOamSpiTypeContainerHandleT handle,
                            ComOamSpiStructHandleT* subHandle);
    /**
     * Gets the MOC that is referenced.
     * @param[in] handle A type container handle.
     * @param[out] subHandle A MOC handle that refers to the referenced MOC.
     * @return ComOk, ComNotExist if the type is not reference, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getReferencedMoc)(ComOamSpiTypeContainerHandleT handle,
                                   ComOamSpiMocHandleT* subHandle);
    /**
     * Gets a @a null terminated array of the default values. The values are set
     * automatically at the creation of an element.
     * @param[in] handle A type container handle.
     * @param[out] defaultValue The values.
     * @return ComOk, ComNotExist if there are no default values, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getDefaultValue)(ComOamSpiTypeContainerHandleT handle,
                                  const char*** defaultValue);
    /**
     * Indicates that the order of the values in the set is significant.
     * A set consisting of the same values, but ordered differently,
     * is considered to be a separate entity.
     * Note: This property is not supported since it is currently not part of
     * the mp.dtd definition.
     * @param[in] handle A type container handle.
     * @param[out] isOrdered There is ordered indication.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsOrdered)(ComOamSpiTypeContainerHandleT handle,
                               bool* isOrdered);

    /**
     * Gets the unique mark. Indicates that the values are unique within the
     * value container (for example within values of multi-value attribute)
     * @param[in] handle A type container handle.
     * @param[out] isUnique The mark.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsUnique)(ComOamSpiTypeContainerHandleT handle,
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
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMultiplicityMin)(ComOamSpiTypeContainerHandleT handle,
                                     uint64_t* min);
    /**
     * Gets the multiplicity max value. If a value is not defined in the model
     * a default value will be provided.
     * The upper bound of the values that the element contains.
     * @param[in] handle A type container handle.
     * @param[out] max The max value.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMultiplicityMax)(ComOamSpiTypeContainerHandleT handle,
                                     uint64_t* max);
} ComOamSpiTypeContainer_2T;

/**
 * Interface to access model information about derived data types.
 */
typedef struct ComOamSpiDerived_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getGeneralProperties)(ComOamSpiDerivedHandleT handle,
                                       ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM a derived belongs to.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMom)(ComOamSpiDerivedHandleT handle,
                         ComOamSpiMomHandleT* subHandle);
    /**
     * Gets the data type.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A type handle that refers to the type.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getType)(ComOamSpiDerivedHandleT handle,
                          ComOamSpiType_2T* type);
    /**
     * Gets the POSIX Basic Regular Expression compatible pattern. The
     * property is optional and applicable only if the type is string.
     * @param[in] handle A derived handle.
     * @param[out] pattern The pattern.
     * @return ComOk, ComNotExist if there is no pattern, or one of the other
     * ComReturnT return codes.
     */
    ComReturnT (*getStringRestrictionPattern)(ComOamSpiDerivedHandleT handle,
            const char** pattern);
    /**
     * Gets the string restriction length. The property is optional and
     * applicable only if the type is string. Use numeric range for numbers.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A range handle that refers to the range.
     * @return ComOk, ComNotExist if there is no length restriction, or one of
     * the other ComReturnT return codes.
     */
    ComReturnT (*getStringRestrictionLength)(ComOamSpiDerivedHandleT handle,
            ComOamSpiValueRangeHandleT* subHandle);
    /**
     * Gets numeric restriction length. The property is optional and applicable
     * only if the type is numeric. Use length restriction for strings.
     * @param[in] handle A derived handle.
     * @param[out] subHandle A value range handle that refers to the value range.
     * @return ComOk, ComNotExist if there is no value range, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getNumericRestrictionRange)(ComOamSpiDerivedHandleT handle,
            ComOamSpiValueRangeHandleT* subHandle);
    /**
     * Gets min value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] min The min value.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getValueRangeMin)(ComOamSpiValueRangeHandleT handle, int64_t* min);
    /**
     * Gets max value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] max The max value.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getValueRangeMax)(ComOamSpiValueRangeHandleT handle, int64_t* max);
    /**
     * Gets the next value range.
     * @param[in] handle A value range handle.
     * @param[out] next A value range handle that refers to the next value range.
     * @return ComOk, ComNotExist if there are no more value ranges, or one of
     * the other ComReturnT return codes.
     */
    ComReturnT (*getValueRangeNext)(ComOamSpiValueRangeHandleT handle,
                                    ComOamSpiValueRangeHandleT* next);
} ComOamSpiDerived_2T;

/**
 * Interface to access model information about enums.
 */
typedef struct ComOamSpiEnum_2 {
    /**
     * Gets the general properties.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getGeneralProperties)(ComOamSpiEnumHandleT handle,
                                       ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM an enum belongs to.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMom)(ComOamSpiEnumHandleT handle, ComOamSpiMomHandleT* subHandle);
    /**
     * Gets the first literal.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A literal handle that refers to the first literal.
     * @return ComOk, ComNotExist it there are no literals, or one of the other
     * ComReturnT return codes.
     */
    ComReturnT (*getLiteral)(ComOamSpiEnumHandleT handle,
                             ComOamSpiEnumLiteralHandleT* subHandle);
    /**
     * Gets the general properties of a literal.
     * @param[in] handle A literal handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getLiteralGeneralProperties)(ComOamSpiEnumLiteralHandleT handle,
            ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the value of a literal.
     * @param[in] handle A literal handle.
     * @param[out] value The value.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getLiteralValue)(ComOamSpiEnumLiteralHandleT handle, int16_t* value);
    /**
     * Gets the next literal.
     * @param[in] handle A literal handle.
     * @param[out] next A literal handle that refers to the next literal.
     * @return ComOk, ComNotExist of there are no more literals, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getLiteralNext)(ComOamSpiEnumLiteralHandleT handle,
                                 ComOamSpiEnumLiteralHandleT* next);
} ComOamSpiEnum_2T;

/**
 * Interface to access model information about structs.
 */
typedef struct ComOamSpiStruct_2 {
    /**
     * Gets the general properties.
     * @param[in] handle A struct handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getGeneralProperties)(ComOamSpiStructHandleT handle,
                                       ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOM a struct belongs to.
     * @param[in] handle A struct handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMom)(ComOamSpiStructHandleT handle,
                         ComOamSpiMomHandleT* subHandle);
    /**
     * Gets the first member of a struct.
     * @param[in] handle A struct handle.
     * @param[out] subHandle A member handle that refers to the member.
     * @return ComOk, ComNotExist if there are no members, or one of the other
     * ComReturnT return codes.
     */
    ComReturnT (*getMember)(ComOamSpiStructHandleT handle,
                            ComOamSpiStructMemberHandleT* subHandle);
    /**
     * Gets the general properties of a member.
     * @param[in] handle A member handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMemberGeneralProperties)(ComOamSpiStructMemberHandleT handle,
            ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the type container of a member.
     * @param[in] handle A member handle.
     * @param[out] subHandle A type container that refers to the type container.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMemberTypeContainer)(ComOamSpiStructMemberHandleT handle,
                                         ComOamSpiTypeContainerHandleT* subHandle);
    /**
     * Gets the next member.
     * @param[in] handle A member handle.
     * @param[out] next A member handle that refers to the next member.
     * @return ComOk, ComNotExist if there are no more members, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getMemberNext)(ComOamSpiStructMemberHandleT handle,
                                ComOamSpiStructMemberHandleT* next);
} ComOamSpiStruct_2T;

/**
 * Interface to access model information about attributes.
 */
typedef struct ComOamSpiAttribute_2 {
    /**
     * Gets the general properties.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getGeneralProperties)(ComOamSpiAttributeHandleT handle,
                                       ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOC an attribute belongs to.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A MOC handle that refers to the MOC.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMoc)(ComOamSpiAttributeHandleT handle,
                         ComOamSpiMocHandleT* subHandle);
    /**
     * Gets the type container.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A type container handle that refers to the type container.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getTypeContainer)(ComOamSpiAttributeHandleT handle,
                                   ComOamSpiTypeContainerHandleT* subHandle);
    /**
     * Gets an indication if this is a key (naming) attribute of the MOC it
     * belongs to. There must be one and only one key attribute per every MOC.
     * @param[in] handle An attribute handle.
     * @param[out] isKey The key indication.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsKey)(ComOamSpiAttributeHandleT handle, bool* isKey);
    /**
     * Gets an indication if an attribute is mandatory. An attribute must be
     * provided at create of a MO. If mandatory, then default must not be
     * specified. If not mandatory, then the attribute value does not have to
     * be supplied at creation of a MO.
     * @param[in] handle An attribute handle.
     * @param[out] isMandatory The mandatory indication.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsMandatory)(ComOamSpiAttributeHandleT handle, bool* isMandatory);
    /**
     * Gets an indication if an attribute is persistent. Indicates that the
     * attribute value survives a restart (that is, a power cycle) of the
     * network element. This is only applicable to read-only attributes
     * since all configuration attributes are assumed to be persistent.
     * @param[in] handle An attributes handle.
     * @param[out] isPersistent The persistent indication.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsPersistent)(ComOamSpiAttributeHandleT handle, bool* isPersistent);
    /**
     * Gets an indication if an attributes is read-only. If isReadOnly equals true,
     * then the attribute is only for read; the system provides updates internally.
     *
     * @param[in] handle An attributes handle.
     * @param[out] isReadOnly The read only indication.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsReadOnly)(ComOamSpiAttributeHandleT handle, bool* isReadOnly);
    /**
     * Gets the string with the unit used for the attribute type. The unit is
     * a standard measure of a quantity.
     * @param[in] handle An attribute handle.
     * @param[out] unit The unit.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getUnit)(ComOamSpiAttributeHandleT handle, const char** unit);
    /**
     * Gets the next attribute.
     * @param[in] handle An attribute handle.
     * @param[out] next An attribute handle that refers to the next attribute.
     * @return ComOk, ComNotExist if there are no more attributes, or one of
     * the other ComReturnT return codes.
     */
    ComReturnT (*getNext)(ComOamSpiAttributeHandleT handle,
                          ComOamSpiAttributeHandleT* next);

    /**
     * Gets an indication if the attribute has restricted property set.
     * @param[in] handle An attribute handle.
     * @param[out] isRestricted The indication.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsRestricted)(ComOamSpiAttributeHandleT handle, bool* isRestricted);

    /**
     * Gets an indication if the attribute generates value change notification.
     * @param[in] handle An attribute handle.
     * @param[out] isNotifiable The indication.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getIsNotifiable)(ComOamSpiAttributeHandleT handle, bool* isNotifiable);

} ComOamSpiAttribute_2T;

/**
 * Interface to access model information about actions.
 */
typedef struct ComOamSpiAction_2 {
    /**
     * Gets the general properties.
     * @param[in] handle An action handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getGeneralProperties)(ComOamSpiActionHandleT handle,
                                       ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the MOC that an action belongs to.
     * @param[in] handle An action handle.
     * @param[out] subHandle A MOC handle that refers to the MOC.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMoc)(ComOamSpiActionHandleT handle,
                         ComOamSpiMocHandleT* subHandle);
    /**
     * Gets the type container.
     * @param[in] handle An action handle.
     * @param[out] subHandle A type container handle that refers to the type container.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getTypeContainer)(ComOamSpiActionHandleT handle,
                                   ComOamSpiTypeContainerHandleT* subHandle);

    /**
     * Gets the next action.
     * @param[in] handle An action handle.
     * @param[out] next An action handle that refers to the next action.
     * @return ComOk, ComNotExist if there are no more actions, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getNext)(ComOamSpiActionHandleT handle,
                          ComOamSpiActionHandleT* next);
    /**
     * Gets the first parameter of an action.
     * @param[in] handle An action handle.
     * @param[out] subHandle A parameter handle that refers to the first parameter.
     * @return ComOk, ComNotExist if there are no parameters, or one of the
     * other ComReturnT return codes.
     */
    ComReturnT (*getParameter)(ComOamSpiActionHandleT handle,
                               ComOamSpiParameterHandleT* subHandle);
    /**
     * Gets the general properties of a parameter.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getParameterGeneralProperties)(ComOamSpiParameterHandleT handle,
            ComOamSpiGeneralPropertiesHandleT* subHandle);
    /**
     * Gets the action that a parameter belongs to.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle An action handle that refers to the action.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getParameterAction)(ComOamSpiParameterHandleT handle,
                                     ComOamSpiActionHandleT* subHandle);
    /**
     * Gets the type container of a parameter.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle A type handle that refers to the type container.
     * @return ComOk, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getParameterTypeContainer)(ComOamSpiParameterHandleT handle,
                                            ComOamSpiTypeContainerHandleT* subHandle);
    /**
     * Gets the next parameter.
     * @param[in] handle A parameter handle.
     * @param[out] next A parameter handle that refers to the next parameter.
     * @return ComOk, ComNotExist if there are no more parameters, or one of
     * the other ComReturnT return codes.
     */
    ComReturnT (*getParameterNext)(ComOamSpiParameterHandleT handle,
                                   ComOamSpiParameterHandleT* next);
} ComOamSpiAction_2T;

/**
 * Interface to access first step modeling elements like MOMs and MOCs.
 */
typedef struct ComOamSpiEntry_2 {
    /**
     * Gets the first MOM that has been registered in the Model Repository.
     * @param[out] handle A MOM handle that refers to the first MOM.
     * @return ComOk if at least one MOM was found, ComNotExist if no MOMs are
     * available, or one of the other ComReturnT return codes.
     */
    ComReturnT (*getMom)(ComOamSpiMomHandleT* handle);
    /**
     * Gets the requested MOC.
     * @param[in] momName The name of the MOM.
     * @param[in] momVersion The version of the MOM. The argument is optional,
     * if it's @a null, then the latest version is used.
     * @param[in] mocName The name of the MOC.
     * @param[out] handle A MOC handle that refers to the requested MOC.
     * @return ComOk if the MOC was found, ComNotExist if it wasn't found, or
     * one of the other ComReturnT return codes.
     */
    ComReturnT (*getMoc)(const char* momName, const char* momVersion,
                         const char* mocName, ComOamSpiMocHandleT* handle);
    /**
     * Gets the root MOC of the top MOM.
     * @param[out] handle A MOC handle that refers to the root MOC.
     * @return ComOk if the root MOC was found, ComNotExist if it wasn't found,
     * or one of the other ComReturnT return codes.
     */
    ComReturnT (*getRoot)(ComOamSpiMocHandleT* handle);
} ComOamSpiEntry_2T;

/**
 * Second version of Model Repository (MR) interface. Contains initialized
 * instances of all the other MR structures forming an interface to dynamically
 * access model information from parsed COM model files.
 *
 * @code
 * // Simple how-to use the interface examples
 *
 * // Get a handle to the first mom in the list of moms
 * ComOamSpiMomHandleT momHandle;
 * ComReturnT retVal = mr->entry->getMom(&momHandle);
 * if (retVal != ComOk) {
 *     // Handle error code
 * }
 *
 * // Use the handle to access mim attributes
 * const char* version = 0;
 * retVal = mr->mom->getVersion(momHandle, &version);
 * if (retVal != ComOk) {
 *     // Handle error code
 * }
 *
 * // Get a handle to the root moc
 * ComOamSpiMocHandleT mocHandle;
 * retVal = mr->mom->getRootMoc(momHandle, &mocHandle);
 * if (retVal != ComOk) {
 *     // Handle error code
 * }
 *
 * // Iterate over the root moc's attributes
 * ComOamSpiAttributeHandleT attrHandle;
 * retVal = mr->moc->getAttribute(mocHandle, &attrHandle);
 * while (retVal == ComOk) {
 *     bool isKey = false;
 *     retVal = mr->attribute->getIsKey(attrHandle, &isKey);
 *     if (retVal != ComOk) {
 *         // Handle error code
 *     }
 *     if (isKey) {
 *         // This is key attribute
 *     }
 *     retVal = mr->attribute->getNext(attrHandle, &attrHandle);
 * }
 *
 * // Traverse the tree of mocs
 * ComOamSpiContainmentHandleT childContainmentHandle;
 * retVal = mr->moc->getChildContainment(mocHandle, &childContainmentHandle));
 * if (retVal != ComOk) {
 *     // Handle error code
 * }
 * ComOamSpiMocHandleT childMocHandle;
 * retVal = mr->containment->getChildMoc(childContainmentHandle, &childMocHandle));
 * ComOamSpiContainmentHandleT parentOfChildContainmentHandle;
 * retVal = mr->moc->getParentContainment(childMocHandle, &parentOfChildContainmentHandle));
 * if (retVal != ComOk) {
 *     // Handle error code
 * }
 *
 * @endcode
 */
typedef struct ComOamSpiModelRepository_2 {
    /**
     * Interface identification.
     */
    ComMgmtSpiInterface_1T base;
    /**
     * Interface for accessing entry definitions.
     */
    ComOamSpiEntry_2T* entry;
    /**
     * Interface for accessing general properties definitions.
     */
    ComOamSpiGeneralProperties_2T* generalProperties;
    /**
     * Interface for accessing MOM definitions.
     */
    ComOamSpiMom_2T* mom;
    /**
     * Interface for accessing MOC definitions.
     */
    ComOamSpiMoc_2T* moc;
    /**
     * Interface for accessing containment definitions.
     */
    ComOamSpiContainment_2T* containment;
    /**
     * Interface for accessing type container definitions.
     */
    ComOamSpiTypeContainer_2T* typeContainer;
    /**
     * Interface for accessing derived data type definitions.
     */
    ComOamSpiDerived_2T* derivedType;
    /**
     * Interface for accessing enum definitions.
     */
    ComOamSpiEnum_2T* enumType;
    /**
     * Interface for accessing struct definitions.
     */
    ComOamSpiStruct_2T* structType;
    /**
     * Interface for accessing attribute definitions.
     */
    ComOamSpiAttribute_2T* attribute;
    /**
     * Interface for accessing action definitions.
     */
    ComOamSpiAction_2T* action;
} ComOamSpiModelRepository_2T;

#endif // _ComOamSpiModelRepository_2_h_

