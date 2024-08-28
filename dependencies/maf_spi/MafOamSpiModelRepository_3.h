#ifndef _MafOamSpiMrModelRepository_3_h_
#define _MafOamSpiMrModelRepository_3_h_

#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * @file MafOamSpiModelRepository_3.h
 * @ingroup MafOamSpi
 *
 * Model Repository interface, version 3.
 *
 * This SPI provides access to MOM definitions.
 */

/**
 *
 * The status property is for COM internal use, when deciding on visibility of elements accessed via the NBI.
 * SA's shall not use this property.
 *
 *
 * Defines the different status properties (current, deprecated, preliminary and obsolete)
 * that the modeling elements can be assigned.
 */

/**
* The definition is current and valid.
*/
#define MafOamSpiStatus_CURRENT_3 "CURRENT"

/**
* The definition is deprecated, but it permits new or continued implementation
* in order to foster interoperability with older or existing implementations.
*/
#define MafOamSpiStatus_DEPRECATED_3 "DEPRECATED"

/**
* The definition is obsolete. It is not recommended to implement.
* It can be removed if previously implemented.
*/
#define MafOamSpiStatus_OBSOLETE_3 "OBSOLETE"

/**
 * The definition is preliminary. It means it is usable but the feature is not yet implemented.
 * Modification of an entity has no effect on the system.
 */
#define MafOamSpiStatus_PRELIMINARY_3 "PRELIMINARY"


/**
 * Defines available data types within Model Repository.
 */
typedef enum MafOamSpiMrType_3 {
    /**
     * 8-bit integer.
     */
    MafOamSpiMrTypeInt8_3 = 1,

    /**
     * 16-bit integer.
     */
    MafOamSpiMrTypeInt16_3 = 2,

    /**
     * 32-bit integer.
     */
    MafOamSpiMrTypeInt32_3 = 3,

    /**
     * 64-bit integer.
     */
    MafOamSpiMrTypeInt64_3 = 4,

    /**
     * 8-bit unsigned integer.
     */
    MafOamSpiMrTypeUint8_3 = 5,

    /**
     * 16-bit unsigned integer.
     */
    MafOamSpiMrTypeUint16_3 = 6,
    /**
     * 32-bit unsigned integer.
     */
    MafOamSpiMrTypeUint32_3 = 7,

    /**
     * 64-bit unsigned integer.
     */
    MafOamSpiMrTypeUint64_3 = 8,
    /**
     * String.
     */
    MafOamSpiMrTypeString_3 = 9,

    /**
     * Boolean.
     */
    MafOamSpiMrTypeBool_3 = 10,

    /**
     * Reference to another MOC.
     */
    MafOamSpiMrTypeReference_3 = 11,

    /**
     * Enumeration.
     */
    MafOamSpiMrTypeEnum_3 = 12,

    /**
     * Struct or aggregated data type.
     */
    MafOamSpiMrTypeStruct_3 = 14,

    /**
     * Void data type. Currently applicable only for return type of actions
     * indicating that there are no return value.
     */
    MafOamSpiMrTypeVoid_3 = 15,

    /**
     * A 64 bits floating point value
     */
    MafOamSpiMrTypeDouble_3 = 16
} MafOamSpiMrType_3T;

/**
 * General properties handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrGeneralPropertiesHandle_3T;

/**
 * MOM handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrMomHandle_3T;

/**
 * MOC handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrMocHandle_3T;

/**
 * Attribute handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrAttributeHandle_3T;

/**
 * Action handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrActionHandle_3T;

/**
 * Action parameter handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrParameterHandle_3T;

/**
 * Containment handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrContainmentHandle_3T;

/**
 * Value range handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrValueRangeHandle_3T;

/**
 * Enum literal handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrEnumLiteralHandle_3T;

/**
 * Type container handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrTypeContainerHandle_3T;

/**
 * Struct member handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrStructMemberHandle_3T;

/**
 * General String properties
 */
typedef enum {
    /**
     * the name of the artifact
     */
    MafOamSpiMrGeneralProperty_name_3 = 0,

    /**
     * hide group name can be used to hide information. Also called "filter"
     */
    MafOamSpiMrGeneralProperty_hideGroupName_3 = 1,

    /**
     * the documentation text for the artifact
     */
    MafOamSpiMrGeneralProperty_documentation_3 = 2,

    /**
     * comma separated list of references to standards
     */
    MafOamSpiMrGeneralProperty_specification_3 = 3,

    /**
     * the status of the artifact (CURRENT,OBSOLETE,DEPRECATED,PRELIMINARY)
     */
    MafOamSpiMrGeneralProperty_status_3 = 4
} MafOamSpiMrGeneralStringProperty_3T;

/**
 * General Integer properties (none so far)
 */
typedef enum {

    MafOamSpiMrGeneralIntProperty_3T_dummyNotUsed = 0

} MafOamSpiMrGeneralIntProperty_3T;

/**
 * General Boolean properties (none so far)
 */
typedef enum {

    MafOamSpiMrGeneralBoolProperty_3T_dummyNotUsed = 0

} MafOamSpiMrGeneralBoolProperty_3T;

/**
 * Interface to access model information about general properties.
 */
typedef struct MafOamSpiMrGeneralProperties_3 {

    /**
     * Gets the value of a general string property
     * @param[in] handle A general properties handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrGeneralPropertiesHandle_3T handle, MafOamSpiMrGeneralStringProperty_3T property, const char** value);

    /**
     * Gets the value of a general boolean property
     * @param[in] handle A general properties handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrGeneralPropertiesHandle_3T handle, MafOamSpiMrGeneralBoolProperty_3T property, bool* value);

    /**
     * Gets the value of an general integer property
     * @param[in] handle A general properties handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrGeneralPropertiesHandle_3T handle, MafOamSpiMrGeneralIntProperty_3T property, int64_t* value);

    /**
     * Gets the domain of the domain extension.
     * @param[in] handle A general properties handle.
     * @param[in] domainName the name of the domain
     * @param[in] extentionName the name of the extension (within the domain)
     * @param[out] the value of the extension.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getDomainExtension)(MafOamSpiMrGeneralPropertiesHandle_3T handle, const char* domainName, const char* extensionName, const char** value);

} MafOamSpiMrGeneralProperties_3T;


/**
 * MOM string properties
 */
typedef enum {
    /**
     * MOM version
     */
    MafOamSpiMrMomProperty_version_3 = 0,

    /**
     * MOM release
     */
    MafOamSpiMrMomProperty_release_3 = 1,

    /**
     * MOM name space
     */
    MafOamSpiMrMomProperty_namespace_3 = 2,

    /**
     * MOM name space prefix
     */
    MafOamSpiMrMomProperty_namespacePrefix_3 = 3,

    /**
     * MOM document number
     */
    MafOamSpiMrMomProperty_docNo_3 = 4,

    /**
     * MOM document revision
     */
    MafOamSpiMrMomProperty_revision_3 = 5,

    /**
     * MOM author
     */
    MafOamSpiMrMomProperty_author_3 = 6,

    /**
     * MOM organization
     */
    MafOamSpiMrMomProperty_organization_3 = 7,

    /**
     * MOM correction
     */
    MafOamSpiMrMomProperty_correction_3 = 8
} MafOamSpiMrMomStringProperty_3T;

/**
 * MOM boolean properties (none so far)
 */
typedef enum {

    MafOamSpiMrMomBoolProperty_3T_dummyNotUsed = 0

} MafOamSpiMrMomBoolProperty_3T;


/**
 * MOM integer properties (none so far)
 */
typedef enum {

    MafOamSpiMrMomIntProperty_3T_dummyNotUsed = 0

} MafOamSpiMrMomIntProperty_3T;

/**
 * Interface to access model information about MOMs.
 */
typedef struct MafOamSpiMrMom_3 {
    /**
     * Gets the general properties.
     * @param[in] handle A MOM handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrMomHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);

    /**
     * Gets the value of a MOM string property
     * @param[in] handle A MOM handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrMomHandle_3T handle, MafOamSpiMrMomStringProperty_3T property, const char** value);

    /**
     * Gets the value of a MOM boolean property
     * @param[in] handle A MOM handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrMomHandle_3T handle, MafOamSpiMrMomBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a MOM integer property
     * @param[in] handle A MOM handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrMomHandle_3T handle, MafOamSpiMrMomIntProperty_3T property, int64_t* value);

    /**
     * Gets the next MOM.
     * @param[in] handle A MOM handle.
     * @param[out] next A MOM handle that refers to the next MOM.
     * @return MafOk, MafNotExist if there are no more MOMs, or on of the other
     * other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiMrMomHandle_3T handle, MafOamSpiMrMomHandle_3T* next);

} MafOamSpiMrMom_3T;

/**
 * MOC string properties
 */
typedef enum {

    /**
     * MOC constraint. The constraints expressed in the MOM. The format is of the constraints is schematron
     */
    MafOamSpiMrMocStringProperty_constraint_3T = 0

} MafOamSpiMrMocStringProperty_3T;

/**
 * MOC boolean properties
 */
typedef enum {
    /**
     * MOC property indicating if this is a root MOC
     */
    MafOamSpiMrMocBoolProperty_isRoot_3 = 0,

    /**
     * MOC property indicating if this is system created (can not be created/deleted) over the NBI
     */
    MafOamSpiMrMocBoolProperty_isSystemCreated_3 = 1
} MafOamSpiMrMocBoolProperty_3T;


/**
 * MOC integer properties (none so far)
 */
typedef enum {

    MafOamSpiMrMocIntProperty_3T_dummyNotUsed = 0

} MafOamSpiMrMocIntProperty_3T;

/**
 * Interface to access model information about MOCs.
 */
typedef struct MafOamSpiMrMoc_3 {
    /**
     * Gets the general properties.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);

    /**
     * Gets the MOM that the MOC belongs to.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrMomHandle_3T* subHandle);

    /**
     * Gets the value of a MOC string property
     * @param[in] handle A MOC handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrMocStringProperty_3T property, const char** value);

    /**
     * Gets the value of a MOC boolean property
     * @param[in] handle A MOC handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrMocBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a MOC integer property
     * @param[in] handle A MOC handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrMocIntProperty_3T property, int64_t* value);

    /**
     * Gets the first attribute.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle An attribute handle that refers to the first attribute.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getAttribute)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrAttributeHandle_3T* subHandle);

    /**
     * Gets the first action.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle An action handle that refers to the first action.
     * @return MafOk, MafNotExist if there are no actions, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getAction)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrActionHandle_3T* subHandle);

    /**
     * Gets the first child containment in the list of child containments for this Moc.
     * @param[in] handle A MOC handle.
     * @param[out] child A containment handle that refers to the first
     * child containment.
     * @return MafOk, MafNotExist if the MOC is a leaf, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getChildContainment)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrContainmentHandle_3T* child);

    /**
     * Gets the first parent containment in the list if parent containments for this Moc.
     * @param[in] handle A MOC handle.
     * @param[out] parent A containment handle that refers to the first
     * parent containment.
     * @return MafOk, MafNotExist if the MOC is root of the top MOM, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getParentContainment)(MafOamSpiMrMocHandle_3T handle, MafOamSpiMrContainmentHandle_3T* parent);

} MafOamSpiMrMoc_3T;


/**
 * Containment string properties (none so far)
 */
typedef enum {

    MafOamSpiMrContainmentStringProperty_3T_dummyNotUsed = 0

} MafOamSpiMrContainmentStringProperty_3T;

/**
 * Containment boolean properties (none so far)
 */
typedef enum {

    MafOamSpiMrContainmentBoolProperty_3T_dummyNotUsed = 0

} MafOamSpiMrContainmentBoolProperty_3T;


/**
 * Containment integer properties
 */
typedef enum {
    /**
     * Containment max number of children
     */
    MafOamSpiMrContainmentIntProperty_cardinalityMin_3 = 0,

    /**
     * Containment min number of children
     */
    MafOamSpiMrContainmentIntProperty_cardinalityMax_3 = 1
} MafOamSpiMrContainmentIntProperty_3T;


/**
 * Interface to access model information about containment associations.
 */
typedef struct MafOamSpiMrContainment_3 {
    /**
     * Gets the general properties.
     * @param[in] handle A containment handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);

    /**
     * Gets the MOM that the containment belongs to.
     * @param[in] handle A containment handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrMomHandle_3T* subHandle);

    /**
     * Gets the child MOC.
     * @param[in] handle A containment handle.
     * @param[out] child A MOC handle that refers to the child MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getChildMoc)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrMocHandle_3T* child);

    /**
     * Gets the parent MOC.
     * @param[in] handle A containment handle.
     * @param[out] parent A MOC handle that refers to the parent MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParentMoc)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrMocHandle_3T* parent);

    /**
     * This function is used for iterating over the parent containments
     * of a certain Moc.To get the first containment
     * in the list the getParentContainment in MafOamSpiMrMoc_3T must be used.
     * Gets the next containment relation in the list that has the same
     * child MOC.
     * @param[in] handle A containment handle.
     * @param[out] next A containment handle that refers to the next containment.
     * @return MafOk, MafNotExist if there are no more relations, or one of the
     * other MafReturnT returns codes.
     */
    MafReturnT (*getNextContainmentSameChildMoc)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrContainmentHandle_3T* next);

    /**
     * This function is used for iterating over the child containments
     * of a certain  Moc.  To get the first containment
     * in the list the getChildContainment in MafOamSpiMrMoc_3T must be used.
     * Gets the next containment relation in the list that has the same
     * parent MOC.
     * @param[in] handle A containment handle.
     * @param[out] next A containment handle that refers to the next containment.
     * @return MafOk, MafNotExist if there are no more relations, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNextContainmentSameParentMoc)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrContainmentHandle_3T* next);

    /**
     * Gets the value of a containment string property
     * @param[in] handle A containment handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrContainmentStringProperty_3T property, const char** value);

    /**
     * Gets the value of a containment bool property
     * @param[in] handle A containment handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrContainmentBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a containment integer property
     * @param[in] handle A containment handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrContainmentHandle_3T handle, MafOamSpiMrContainmentIntProperty_3T property, int64_t* value);

} MafOamSpiMrContainment_3T;

/**
 * Type container string properties (none so far)
 */
typedef enum {

    MafOamSpiMrTypeContainerStringProperty_3T_dummyNotUsed = 0

} MafOamSpiMrTypeContainerStringProperty_3T;


/**
 * Type container boolean properties
 */
typedef enum {
    /**
     * indicates if the order of the values in a multi-valued attribute has relevance
     */
    MafOamSpiMrTypeContainerBoolProperty_isOrdered_3 = 0,

    /**
     * indicates if each value in a multi-valued must be unique
     */
    MafOamSpiMrTypeContainerBoolProperty_isUnique_3 = 1
} MafOamSpiMrTypeContainerBoolProperty_3T;

/**
 * Type container integer properties
 */
typedef enum {
    /**
     * the minimum number of values
     */
    MafOamSpiMrTypeContainerIntProperty_multiplicityMin_3 = 0,

    /**
     * the maximum number of values
     */
    MafOamSpiMrTypeContainerIntProperty_multiplicityMax_3 = 1
} MafOamSpiMrTypeContainerIntProperty_3T;


/**
 * Interface to access model information about data types.
 */
typedef struct MafOamSpiMrTypeContainer_3 {

    /**
     * Gets the general properties. If the type is not defined by a derived type, it will
     * have no general properties (and return MafNotExist)
     * @param[in] handle A type container handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);



    /**
     * Gets the data type.
     * @param[in] handle A type container handle.
     * @param[out] type The type.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getType)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrType_3T* type);

    /**
     * Gets a @a null terminated array of the default values. The values are set
     * automatically at the creation of an element.
     * @param[in] handle A type container handle.
     * @param[out] defaultValue The values.
     * @return MafOk, MafNotExist if there are no default values, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getDefaultValue)(MafOamSpiMrTypeContainerHandle_3T handle, const char*** defaultValue);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrTypeContainerStringProperty_3T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrTypeContainerBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrTypeContainerIntProperty_3T property, int64_t* value);

} MafOamSpiMrTypeContainer_3T;

/**
 * String type container string properties
 */
typedef enum {
    /**
     * a regular expression restricting the attribute value
     */
    MafOamSpiMrTypeContainerStringProperty_stringRestrictionPattern_3 = 0,
    /**
     * The exception text associated to the regular expression restriction.
     */
    MafOamSpiMrTypeContainerStringProperty_stringRestrictionExceptionText_3 = 1
} MafOamSpiMrStringStringProperty_3T;

/**
 * String type container string properties (none so far)
 */
typedef enum {

    MafOamSpiMrStringBoolProperty_3T_dummyNotUsed = 0

} MafOamSpiMrStringBoolProperty_3T;

/**
 * String type container integer properties
 */
typedef enum {
    /**
     * maximum length for a string
     */
    MafOamSpiMrTypeContainerStringProperty_maxLength_3 = 0,

    /**
     * minimum length for a string
     */
    MafOamSpiMrTypeContainerStringProperty_minLength_3 = 1
} MafOamSpiMrStringIntProperty_3T;

/**
 * Interface to access type information which is specific for string data types
 */
typedef struct MafOamSpiMrString_3 {

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrStringStringProperty_3T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrStringBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrStringIntProperty_3T property, int64_t* value);

} MafOamSpiMrString_3T;

/**
 * Integer type container string properties (none so far)
 */
typedef enum {

    MafOamSpiMrIntStringProperty_3T_dummyNotUsed = 0

} MafOamSpiMrIntStringProperty_3T;

/**
 * Integer type container boolean properties (none so far)
 */
typedef enum {

    MafOamSpiMrIntBoolProperty_3T_dummyNotUsed = 0

} MafOamSpiMrIntBoolProperty_3T;

/**
 * Integer type container integer properties (none so far)
 */
typedef enum {
    /**
     * the resolution of the value (0 means that any value is allowed, 10 means 0,10,20 etc. is allowed)
     */
    MafOamSpiMrIntIntProperty_resolution_3 = 0
} MafOamSpiMrIntIntProperty_3T;

/**
 * Interface to access type information which is specific for integer data types
 */
typedef struct MafOamSpiMrInt_3 {

    /**
     * Gets numeric restriction length. The property is optional and applicable
     * only if the type is numeric. Use length restriction for strings.
     * @param[in] handle A type handle.
     * @param[out] subHandle A value range handle that refers to the value range.
     * @return MafOk, MafNotExist if there is no value range, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNumericRestrictionRange)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrValueRangeHandle_3T* subHandle);

    /**
     * Gets min value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] min The min value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMin)(MafOamSpiMrValueRangeHandle_3T handle, int64_t* min);

    /**
     * Gets max value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] max The max value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMax)(MafOamSpiMrValueRangeHandle_3T handle, int64_t* max);

    /**
     * Gets the next value range.
     * @param[in] handle A value range handle.
     * @param[out] next A value range handle that refers to the next value range.
     * @return MafOk, MafNotExist if there are no more value ranges, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeNext)(MafOamSpiMrValueRangeHandle_3T handle, MafOamSpiMrValueRangeHandle_3T* next);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrIntStringProperty_3T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrIntBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrIntIntProperty_3T property, int64_t* value);

} MafOamSpiMrInt_3T;

/**
 * Float type container string properties (none so far)
 */
typedef enum {
    /**
      * The resolution of the value. For example in the range of
      * [-0.5 .. 5.5] and resolution 0.3 indicates that the values allowed are
      * -0.5, -0.2, 0.1 ... 5.5
     */
    MafOamSpiMrFloatStringProperty_resolution_3 = 0

} MafOamSpiMrFloatStringProperty_3T;

/**
 * Float type container boolean properties (none so far)
 */
typedef enum {

    MafOamSpiMrFloatBoolProperty_3T_dummyNotUsed = 0

} MafOamSpiMrFloatBoolProperty_3T;

/**
 * Float type container float properties
 */
typedef enum {
    /**
      * The resolution of the value. For example in the range of
      * [-0.5 .. 5.5] and resolution 0.3 indicates that the values allowed are
      * -0.5, -0.2, 0.1 ... 5.5
     */
    MafOamSpiMrFloatFloatProperty_resolution_3 = 0

} MafOamSpiMrFloatFloatProperty_3T;


/**
 * Interface to access type information which is specific for float data types
 */
typedef struct MafOamSpiMrFloat_3 {

    /**
     * Gets numeric restriction length. The property is optional and applicable
     * only if the type is numeric. Use length restriction for strings.
     * @param[in] handle A type handle.
     * @param[out] subHandle A value range handle that refers to the value range.
     * @return MafOk, MafNotExist if there is no value range, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNumericRestrictionRange)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrValueRangeHandle_3T* subHandle);

    /**
     * Gets min value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] min The min value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMin)(MafOamSpiMrValueRangeHandle_3T handle, double* min);

    /**
     * Gets max value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] max The max value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMax)(MafOamSpiMrValueRangeHandle_3T handle, double* max);

    /**
     * Gets the next value range.
     * @param[in] handle A value range handle.
     * @param[out] next A value range handle that refers to the next value range.
     * @return MafOk, MafNotExist if there are no more value ranges, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeNext)(MafOamSpiMrValueRangeHandle_3T handle, MafOamSpiMrValueRangeHandle_3T* next);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrFloatStringProperty_3T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrFloatBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a type container float property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getFloatProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrFloatFloatProperty_3T property, double* value);

} MafOamSpiMrFloat_3T;

/**
 * MoRef type container string properties (none so far)
 */
typedef enum {

    MafOamSpiMrMoRefStringProperty_3T_dummyNotUsed = 0

} MafOamSpiMrMoRefStringProperty_3T;

/**
 * MoRef type container boolean properties (none so far)
 */
typedef enum {

    MafOamSpiMrMoRefBoolProperty_3T_dummyNotUsed = 0

} MafOamSpiMrMoRefBoolProperty_3T;

/**
 * MoRef type container integer properties (none so far)
 */
typedef enum {

    MafOamSpiMrMoRefIntProperty_3T_dummyNotUsed = 0

} MafOamSpiMrMoRefIntProperty_3T;

typedef struct MafOamSpiMrMoRef_3 {

    /**
     * Gets the MOC that is referenced.
     * @param[in] handle A type container handle.
     * @param[out] subHandle A MOC handle that refers to the referenced MOC.
     * @return MafOk, MafNotExist if the type is not reference, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getReferencedMoc)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrMocHandle_3T* subHandle);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrMoRefStringProperty_3T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrIntBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrMoRefIntProperty_3T property, int64_t* value);

} MafOamSpiMrMoRef_3T;


/**
 * Interface to access type information which is specific for enum data types
 */
typedef struct MafOamSpiMrEnum_3 {
    /**
     * Gets the general properties.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);

    /**
     * Gets the first literal.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A literal handle that refers to the first literal.
     * @return MafOk, MafNotExist it there are no literals, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getLiteral)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrEnumLiteralHandle_3T* subHandle);
    /**
     * Gets the general properties of a literal.
     * @param[in] handle A literal handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getLiteralGeneralProperties)(MafOamSpiMrEnumLiteralHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);
    /**
     * Gets the value of a literal.
     * @param[in] handle A literal handle.
     * @param[out] value The value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getLiteralValue)(MafOamSpiMrEnumLiteralHandle_3T handle, int64_t* value);

    /**
     * Gets the next literal.
     * @param[in] handle A literal handle.
     * @param[out] next A literal handle that refers to the next literal.
     * @return MafOk, MafNotExist of there are no more literals, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getLiteralNext)(MafOamSpiMrEnumLiteralHandle_3T handle, MafOamSpiMrEnumLiteralHandle_3T* next);
} MafOamSpiMrEnum_3T;

/**
 * Struct type container string properties (none so far)
 */
typedef enum {

    MafOamSpiMrStructStringProperty_3T_dummyNotUsed = 0

} MafOamSpiMrStructStringProperty_3T;

/**
 * Struct type container boolean properties
 */
typedef enum {
    /**
     * an exclusive struct is acting like a union, where only one element at a time
     * can have a value
     */
    MafOamSpiMrStructBoolProperty_isExclusive_3 = 0
} MafOamSpiMrStructBoolProperty_3T;

/**
 * Struct type container integer properties (none so far)
 */
typedef enum {

    MafOamSpiMrStructIntProperty_3T_dummyNotUsed = 0

} MafOamSpiMrStructIntProperty_3T;

/**
 * Struct member string properties (none so far)
 */
typedef enum {

    MafOamSpiMrStructMemberStringProperty_3T_dummyNotUsed = 0

} MafOamSpiMrStructMemberStringProperty_3T;

/**
 * Struct member integer properties (none so far)
 */
typedef enum {

    MafOamSpiMrStructMemberIntProperty_3T_dummyNotUsed = 0

} MafOamSpiMrStructMemberIntProperty_3T;

/**
 * Struct member boolean properties
 */
typedef enum {
    MafOamSpiMrStructMemberBoolProperty_isKey_3 = 0
} MafOamSpiMrStructMemberBoolProperty_3T;

/**
 * Interface to access type information which is specific for struct data types
 */
typedef struct MafOamSpiMrStruct_3 {

    /**
     * Gets the first member of a struct.
     * @param[in] handle A struct handle.
     * @param[out] subHandle A member handle that refers to the member.
     * @return MafOk, MafNotExist if there are no members, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getMember)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrStructMemberHandle_3T* subHandle);

    /**
     * Gets the general properties of a member.
     * @param[in] handle A member handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMemberGeneralProperties)(MafOamSpiMrStructMemberHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);

    /**
     * Gets the type container of a member.
     * @param[in] handle A member handle.
     * @param[out] subHandle A type container that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMemberTypeContainer)(MafOamSpiMrStructMemberHandle_3T handle, MafOamSpiMrTypeContainerHandle_3T* subHandle);

    /**
     * Gets the next member.
     * @param[in] handle A member handle.
     * @param[out] next A member handle that refers to the next member.
     * @return MafOk, MafNotExist if there are no more members, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getMemberNext)(MafOamSpiMrStructMemberHandle_3T handle, MafOamSpiMrStructMemberHandle_3T* next);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrStructStringProperty_3T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrStructBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_3T handle, MafOamSpiMrStructIntProperty_3T property, int64_t* value);

    /**
     * Gets the value of a struct member string property
     * @param[in] handle A struct member handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getMemberStringProperty)(MafOamSpiMrStructMemberHandle_3T handle, MafOamSpiMrStructMemberStringProperty_3T property, const char** value);

    /**
     * Gets the value of a struct member boolean property
     * @param[in] handle A struct member handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getMemberBoolProperty)(MafOamSpiMrStructMemberHandle_3T handle, MafOamSpiMrStructMemberBoolProperty_3T property, bool* value);

    /**
     * Gets the value of a struct member integer property
     * @param[in] handle A struct member handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getMemberIntProperty)(MafOamSpiMrStructMemberHandle_3T handle, MafOamSpiMrStructMemberIntProperty_3T property, int64_t* value);

} MafOamSpiMrStruct_3T;

/**
 * Attribute string properties
 */
typedef enum {
    /**
     * the unit of the attribute
     */
    MafOamSpiMrAttributeStringProperty_unit_3 = 0
} MafOamSpiMrAttributeStringProperty_3T;

/**
 * Attribute boolean properties
 */
typedef enum {
    /**
     * true if the attribute is the key attribute for the MO class
     */
    MafOamSpiMrAttributeBoolProperty_isKey_3 = 0,

    /**
     * Mandatory means that there must be at least one value
     * this property can be derived from multiplicity and default value
     */
    MafOamSpiMrAttributeBoolProperty_isMandatory_3 = 1,

    /**
     * Defines if the attribute can be written over the NBI (isReadOnly == false)
     */
    MafOamSpiMrAttributeBoolProperty_isReadOnly_3 = 2,

    /**
     * defines if the attribute can only be set at create
     */
    MafOamSpiMrAttributeBoolProperty_isRestricted_3 = 3,

    /**
     * defines if the attribute should emit Attribute Value Change notifications
     */
    MafOamSpiMrAttributeBoolProperty_isNotifiable_3 = 4
} MafOamSpiMrAttributeBoolProperty_3T;

/**
 * Attribute integer properties
 */
typedef enum {

    MafOamSpiMrAttributeIntProperty_3T_dummyNotUsed = 0

} MafOamSpiMrAttributeIntProperty_3T;

/**
 * Interface to access model information about attributes.
 */
typedef struct MafOamSpiMrAttribute_3 {
    /**
     * Gets the general properties.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrAttributeHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);

    /**
     * Gets the MOC an attribute belongs to.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A MOC handle that refers to the MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoc)(MafOamSpiMrAttributeHandle_3T handle, MafOamSpiMrMocHandle_3T* subHandle);

    /**
     * Gets the type container.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A type container handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getTypeContainer)(MafOamSpiMrAttributeHandle_3T handle, MafOamSpiMrTypeContainerHandle_3T* subHandle);

    /**
     * Gets the next attribute.
     * @param[in] handle An attribute handle.
     * @param[out] next An attribute handle that refers to the next attribute.
     * @return MafOk, MafNotExist if there are no more attributes, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiMrAttributeHandle_3T handle, MafOamSpiMrAttributeHandle_3T* next);

    /**
     * Gets the value of an attribute string property
     * @param[in] handle An attribute handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrAttributeHandle_3T handle, MafOamSpiMrAttributeStringProperty_3T property, const char** value);

    /**
     * Gets the value of an attribute boolean property
     * @param[in] handle An attribute handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrAttributeHandle_3T handle, MafOamSpiMrAttributeBoolProperty_3T property, bool* value);

    /**
     * Gets the value of an attribute integer property
     * @param[in] handle An attribute handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrAttributeHandle_3T handle, MafOamSpiMrAttributeIntProperty_3T property, int64_t* value);

} MafOamSpiMrAttribute_3T;

/**
 * Action string properties
 */
typedef enum {

    MafOamSpiMrActionStringProperty_3T_dummyNotUsed = 0

} MafOamSpiMrActionStringProperty_3T;

/**
 * Action boolean properties
 */
typedef enum {

    MafOamSpiMrActionBoolProperty_3T_dummyNotUsed = 0

} MafOamSpiMrActionBoolProperty_3T;

/**
 * Action integer properties
 */
typedef enum {

    MafOamSpiMrActionIntProperty_3T_dummyNotUsed = 0

} MafOamSpiMrActionIntProperty_3T;

/**
 * Interface to access model information about actions.
 */
typedef struct MafOamSpiMrAction_3 {
    /**
     * Gets the general properties.
     * @param[in] handle An action handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrActionHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);

    /**
     * Gets the MOC that an action belongs to.
     * @param[in] handle An action handle.
     * @param[out] subHandle A MOC handle that refers to the MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoc)(MafOamSpiMrActionHandle_3T handle, MafOamSpiMrMocHandle_3T* subHandle);

    /**
     * Gets the type container  of the action result value.
     * @param[in] handle An action handle.
     * @param[out] subHandle A type container handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getTypeContainer)(MafOamSpiMrActionHandle_3T handle, MafOamSpiMrTypeContainerHandle_3T* subHandle);

    /**
     * Gets the next action.
     * @param[in] handle An action handle.
     * @param[out] next An action handle that refers to the next action.
     * @return MafOk, MafNotExist if there are no more actions, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiMrActionHandle_3T handle, MafOamSpiMrActionHandle_3T* next);

    /**
     * Gets the first parameter of an action.
     * @param[in] handle An action handle.
     * @param[out] subHandle A parameter handle that refers to the first parameter.
     * @return MafOk, MafNotExist if there is no parameters, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getParameter)(MafOamSpiMrActionHandle_3T handle, MafOamSpiMrParameterHandle_3T* subHandle);

    /**
     * Gets the general properties of a parameter.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterGeneralProperties)(MafOamSpiMrParameterHandle_3T handle, MafOamSpiMrGeneralPropertiesHandle_3T* subHandle);

    /**
     * Gets the action that a parameter belongs to.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle An action handle that refers to the action.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterAction)(MafOamSpiMrParameterHandle_3T handle, MafOamSpiMrActionHandle_3T* subHandle);

    /**
     * Gets the type container of a parameter.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle A type handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterTypeContainer)(MafOamSpiMrParameterHandle_3T handle, MafOamSpiMrTypeContainerHandle_3T* subHandle);

    /**
     * Gets the next parameter.
     * @param[in] handle A parameter handle.
     * @param[out] next A parameter handle that refers to the next parameter.
     * @return MafOk, MafNotExist if there are no more parameters, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getParameterNext)(MafOamSpiMrParameterHandle_3T handle, MafOamSpiMrParameterHandle_3T* next);

    /**
     * Gets the value of an action string property
     * @param[in] handle An action handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrActionHandle_3T handle, MafOamSpiMrActionStringProperty_3T property, const char** value);

    /**
     * Gets the value of an action boolean property
     * @param[in] handle An action handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrActionHandle_3T handle, MafOamSpiMrActionBoolProperty_3T property, bool* value);

    /**
     * Gets the value of an action integer property
     * @param[in] handle An action handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrActionHandle_3T handle, MafOamSpiMrActionIntProperty_3T property, int64_t* value);

} MafOamSpiMrAction_3T;

/**
 * Interface to access first step modeling elements like MOMs and MOCs.
 */
typedef struct MafOamSpiMrEntry_3 {
    /**
     * Gets the first MOM that has been registered in the Model Repository.
     * @param[out] handle A MOM handle that refers to the first MOM.
     * @return MafOk if at least one MOM was found, MafNotExist if no MOMs are
     * available, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMrMomHandle_3T* handle);

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
    MafReturnT (*getMoc)(const char* momName, const char* momVersion, const char* mocName, MafOamSpiMrMocHandle_3T* handle);

    /**
     * Gets the MOC handle for the class of an MO instance
     * @param[in] dn The Distinguished Name of the MO instance
     * @param[in] checkFullMocPath if true, the full MOC path is always checked. If false,
     * the MOC path is only checked if there are more than one MOC with the same name.
     * Looking up the MOC handle may be faster if checkFullMocPath == false.
     * @param[out] handle A handle to the MOC (MO Class)
     * @return MafOk if the MOC was found, MafNotExist if it wasn't found, or
     * one of the other MafReturnT return codes.
     */
    MafReturnT (*getMocFromDn)(const char* dn, bool checkFullMocPath, MafOamSpiMrMocHandle_3T* handle);

    /**
     * Gets the root MOC of the top MOM.
     * @param[out] handle A MOC handle that refers to the root MOC.
     * @return MafOk if the root MOC was found, MafNotExist if it wasn't found,
     * or one of the other MafReturnT return codes.
     */
    MafReturnT (*getRoot)(MafOamSpiMrMocHandle_3T* handle);

} MafOamSpiMrEntry_3T;

/**
 * Second version of Model Repository (MR) interface. Contains initialized
 * instances of all the other MR structures forming an interface to dynamically
 * access model information from parsed MAF model files.
 *
 * @code
 * // Simple how-to use the interface examples
 *
 * // Get a handle to the first mom in the list of moms
 * MafOamSpiMrMomHandle_3T momHandle;
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
 * MafOamSpiMrMocHandle_3T mocHandle;
 * retVal = mr->entry->getMocFromDn("ManagedElement=1,SystemFunctions=1", false, &mocHandle);
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 *
 * // Iterate over the root moc's attributes
 * MafOamSpiMrAttributeHandle_3T attrHandle;
 * retVal = mr->moc->getAttribute(mocHandle, &attrHandle);
 * while (retVal == MafOk) {
 *     bool isKey = false;
 *     retVal = mr->attribute->getBoolProperty(attrHandle, MafOamSpiMrAttributeBoolProperty_isKey_3, &isKey);
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
 * MafOamSpiMrContainmentHandle_3T childContainmentHandle;
 * retVal = mr->moc->getChildContainment(mocHandle, &childContainmentHandle));
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 * MafOamSpiMrMocHandle_3T childMocHandle;
 * retVal = mr->containment->getChildMoc(childContainmentHandle, &childMocHandle));
 * MafOamSpiMrContainmentHandle_3T parentOfChildContainmentHandle;
 * retVal = mr->moc->getParentContainment(childMocHandle, &parentOfChildContainmentHandle));
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 *
 * @endcode
 */
typedef struct MafOamSpiModelRepository_3 {
    /**
     * Interface identification.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Interface for accessing entry definitions.
     */
    MafOamSpiMrEntry_3T* entry;

    /**
     * Interface for accessing general properties definitions.
     */
    MafOamSpiMrGeneralProperties_3T* generalProperties;

    /**
     * Interface for accessing MOM definitions.
     */
    MafOamSpiMrMom_3T* mom;

    /**
     * Interface for accessing MOC definitions.
     */
    MafOamSpiMrMoc_3T* moc;

    /**
     * Interface for accessing containment definitions.
     */
    MafOamSpiMrContainment_3T* containment;

    /**
     * Interface for accessing type container definitions.
     */
    MafOamSpiMrTypeContainer_3T* typeContainer;

    /**
     * Interface for accessing enum definitions.
     */
    MafOamSpiMrEnum_3T* enumType;

    /**
     * Interface for accessing struct definitions.
     */
    MafOamSpiMrStruct_3T* structType;

    /**
     * Interface for accessing attribute definitions.
     */
    MafOamSpiMrAttribute_3T* attribute;

    /**
     * Interface for accessing action definitions.
     */
    MafOamSpiMrAction_3T* action;

    /**
     * Interface for accessing MoRef definitions.
     */
    MafOamSpiMrMoRef_3T* moRef;

    /**
     * Interface for accessing integer definitions.
     */
    MafOamSpiMrInt_3T* intIf;

    /**
     * Interface for accessing string definitions.
     */
    MafOamSpiMrString_3T* stringIf;

    /**
     * Interface for accessing float definitions.
     */
    MafOamSpiMrFloat_3T* floatIf;

} MafOamSpiModelRepository_3T;

#endif // _MafOamSpiMrModelRepository_3_h_
