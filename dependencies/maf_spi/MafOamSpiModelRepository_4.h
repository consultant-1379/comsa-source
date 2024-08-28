#ifndef _MafOamSpiMrModelRepository_4_h_
#define _MafOamSpiMrModelRepository_4_h_

#include <MafMgmtSpiCommon.h>
#include <MafMgmtSpiInterface_1.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * @file MafOamSpiModelRepository_4.h
 * @ingroup MafOamSpi
 *
 * Model Repository interface, version 4.
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
#define MafOamSpiStatus_CURRENT_4 "CURRENT"

/**
* The definition is deprecated, but it permits new or continued implementation
* in order to foster interoperability with older or existing implementations.
*/
#define MafOamSpiStatus_DEPRECATED_4 "DEPRECATED"

/**
* The definition is obsolete. It is not recommended to implement.
* It can be removed if previously implemented.
*/
#define MafOamSpiStatus_OBSOLETE_4 "OBSOLETE"

/**
 * The definition is preliminary. It means it is usable but the feature is not yet implemented.
 * Modification of an entity has no effect on the system.
 */
#define MafOamSpiStatus_PRELIMINARY_4 "PRELIMINARY"

/**
 * Defines available data types within Model Repository.
 */
typedef enum MafOamSpiMrType_4 {
    /**
     * 8-bit integer.
     */
    MafOamSpiMrTypeInt8_4 = 1,

    /**
     * 16-bit integer.
     */
    MafOamSpiMrTypeInt16_4 = 2,

    /**
     * 32-bit integer.
     */
    MafOamSpiMrTypeInt32_4 = 3,

    /**
     * 64-bit integer.
     */
    MafOamSpiMrTypeInt64_4 = 4,

    /**
     * 8-bit unsigned integer.
     */
    MafOamSpiMrTypeUint8_4 = 5,

    /**
     * 16-bit unsigned integer.
     */
    MafOamSpiMrTypeUint16_4 = 6,
    /**
     * 32-bit unsigned integer.
     */
    MafOamSpiMrTypeUint32_4 = 7,

    /**
     * 64-bit unsigned integer.
     */
    MafOamSpiMrTypeUint64_4 = 8,
    /**
     * String.
     */
    MafOamSpiMrTypeString_4 = 9,

    /**
     * Boolean.
     */
    MafOamSpiMrTypeBool_4 = 10,

    /**
     * Reference to another MOC.
     */
    MafOamSpiMrTypeReference_4 = 11,

    /**
     * Enumeration.
     */
    MafOamSpiMrTypeEnum_4 = 12,

    /**
     * Struct or aggregated data type.
     */
    MafOamSpiMrTypeStruct_4 = 14,

    /**
     * Void data type. Currently applicable only for return type of actions
     * indicating that there are no return value.
     */
    MafOamSpiMrTypeVoid_4 = 15,

    /**
     * A 64 bits floating point value
     */
    MafOamSpiMrTypeDouble_4 = 16
} MafOamSpiMrType_4T;

/**
 * General properties handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrGeneralPropertiesHandle_4T;

/**
 * MOM handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrMomHandle_4T;

/**
 * MOC handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrMocHandle_4T;

/**
 * Attribute handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrAttributeHandle_4T;

/**
 * Action handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrActionHandle_4T;

/**
 * Action parameter handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrParameterHandle_4T;

/**
 * Containment handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrContainmentHandle_4T;

/**
 * Value range handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrValueRangeHandle_4T;

/**
 * Enum literal handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrEnumLiteralHandle_4T;

/**
 * Type container handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrTypeContainerHandle_4T;

/**
 * Struct member handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrStructMemberHandle_4T;

/**
 * End point association handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrAssociationEndHandle_4T;

/**
 * Bidirectional association handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrBiDirAssociationHandle_4T;

/**
 * Unidirectional association handle. The contents of the handle is for internal use.
 */
typedef struct {
    uint64_t handle;
} MafOamSpiMrUniDirAssociationHandle_4T;


/**
 * General String properties
 */
typedef enum {
    /**
     * the name of the artifact
     */
    MafOamSpiMrGeneralProperty_name_4 = 0,

    /**
     * hide group name can be used to hide information. Also called "filter"
     */
    MafOamSpiMrGeneralProperty_hideGroupName_4 = 1,

    /**
     * the documentation text for the artifact
     */
    MafOamSpiMrGeneralProperty_documentation_4 = 2,

    /**
     * comma separated list of references to standards
     */
    MafOamSpiMrGeneralProperty_specification_4 = 3,

    /**
     * the status of the artifact (CURRENT,OBSOLETE,DEPRECATED)
     */
    MafOamSpiMrGeneralProperty_status_4 = 4
} MafOamSpiMrGeneralStringProperty_4T;

/**
 * General Integer properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrGeneralIntProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrGeneralIntProperty_4T;

/**
 * General Boolean properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrGeneralBoolProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrGeneralBoolProperty_4T;

/**
 * Interface to access model information about general properties.
 */
typedef struct MafOamSpiMrGeneralProperties_4 {

    /**
     * Gets the value of a general string property
     * @param[in] handle A general properties handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrGeneralPropertiesHandle_4T handle, MafOamSpiMrGeneralStringProperty_4T property, const char** value);

    /**
     * Gets the value of a general boolean property
     * @param[in] handle A general properties handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrGeneralPropertiesHandle_4T handle, MafOamSpiMrGeneralBoolProperty_4T property, bool* value);

    /**
     * Gets the value of an general integer property
     * @param[in] handle A general properties handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrGeneralPropertiesHandle_4T handle, MafOamSpiMrGeneralIntProperty_4T property, int64_t* value);

    /**
     * Gets the domain of the domain extension.
     * @param[in] handle A general properties handle.
     * @param[in] domainName the name of the domain
     * @param[in] extentionName the name of the extension (within the domain)
     * @param[out] the value of the extension.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getDomainExtension)(MafOamSpiMrGeneralPropertiesHandle_4T handle, const char* domainName, const char* extensionName, const char** value);

} MafOamSpiMrGeneralProperties_4T;


/**
 * MOM string properties
 */
typedef enum {
    /**
     * MOM version
     */
    MafOamSpiMrMomProperty_version_4 = 0,

    /**
     * MOM release
     */
    MafOamSpiMrMomProperty_release_4 = 1,

    /**
     * MOM name space
     */
    MafOamSpiMrMomProperty_namespace_4 = 2,

    /**
     * MOM name space prefix
     */
    MafOamSpiMrMomProperty_namespacePrefix_4 = 3,

    /**
     * MOM document number
     */
    MafOamSpiMrMomProperty_docNo_4 = 4,

    /**
     * MOM document revision
     */
    MafOamSpiMrMomProperty_revision_4 = 5,

    /**
     * MOM author
     */
    MafOamSpiMrMomProperty_author_4 = 6,

    /**
     * MOM organization
     */
    MafOamSpiMrMomProperty_organization_4 = 7,

    /**
     * MOM correction
     */
    MafOamSpiMrMomProperty_correction_4 = 8

} MafOamSpiMrMomStringProperty_4T;

/**
 * MOM boolean properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrMomBoolProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrMomBoolProperty_4T;


/**
 * MOM integer properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrMomIntProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrMomIntProperty_4T;

/**
 * Interface to access model information about MOMs.
 */
typedef struct MafOamSpiMrMom_4 {
    /**
     * Gets the general properties.
     * @param[in] handle A MOM handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrMomHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the value of a MOM string property
     * @param[in] handle A MOM handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrMomHandle_4T handle, MafOamSpiMrMomStringProperty_4T property, const char** value);

    /**
     * Gets the value of a MOM boolean property
     * @param[in] handle A MOM handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrMomHandle_4T handle, MafOamSpiMrMomBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a MOM integer property
     * @param[in] handle A MOM handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrMomHandle_4T handle, MafOamSpiMrMomIntProperty_4T property, int64_t* value);

    /**
     * Gets the model features of the MOM.
     * @param[in] handle A MOM handle.
     * @param[out] value the value of all model features
     * @return MafOk if the model features are retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getModelFeatures)(MafOamSpiMrMomHandle_4T handle, const char*** value);

    /**
     * Gets the next MOM.
     * @param[in] handle A MOM handle.
     * @param[out] next A MOM handle that refers to the next MOM.
     * @return MafOk, MafNotExist if there are no more MOMs, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiMrMomHandle_4T handle, MafOamSpiMrMomHandle_4T* next);

} MafOamSpiMrMom_4T;

/**
 * MOC string properties
 */
typedef enum {

    /**
     * MOC constraint. The constraints expressed in the MOM. The format of the constraints is schematron
     */
    MafOamSpiMrMocStringProperty_constraint_4T = 0

} MafOamSpiMrMocStringProperty_4T;

/**
 * MOC boolean properties
 */
typedef enum {
    /**
     * MOC property indicating if this is a root MOC
     */
    MafOamSpiMrMocBoolProperty_isRoot_4 = 0,

    /**
     * MOC property indicating if this is system created (can not be created/deleted) over the NBI
     */
    MafOamSpiMrMocBoolProperty_isSystemCreated_4 = 1

} MafOamSpiMrMocBoolProperty_4T;


/**
 * MOC integer properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrMocIntProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrMocIntProperty_4T;

/**
 * Interface to access model information about MOCs.
 */
typedef struct MafOamSpiMrMoc_4 {
    /**
     * Gets the general properties.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the MOM that the MOC belongs to.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrMomHandle_4T* subHandle);

    /**
     * Gets the value of a MOC string property
     * @param[in] handle A MOC handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrMocStringProperty_4T property, const char** value);

    /**
     * Gets the value of a MOC boolean property
     * @param[in] handle A MOC handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrMocBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a MOC integer property
     * @param[in] handle A MOC handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrMocIntProperty_4T property, int64_t* value);

    /**
     * Gets the first attribute.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle An attribute handle that refers to the first attribute.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getAttribute)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrAttributeHandle_4T* subHandle);

    /**
     * Gets the first action.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle An action handle that refers to the first action.
     * @return MafOk, MafNotExist if there are no actions, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getAction)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrActionHandle_4T* subHandle);

    /**
     * Gets the first child containment in the list of child containments for this Moc.
     * @param[in] handle A MOC handle.
     * @param[out] child A containment handle that refers to the first
     * child containment.
     * @return MafOk, MafNotExist if the MOC is a leaf, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getChildContainment)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrContainmentHandle_4T* child);

    /**
     * Gets the first parent containment in the list if parent containments for this Moc.
     * @param[in] handle A MOC handle.
     * @param[out] parent A containment handle that refers to the first
     * parent containment.
     * @return MafOk, MafNotExist if the MOC is root of the top MOM, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getParentContainment)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrContainmentHandle_4T* parent);

    /**
     * Gets the first bidirectional association for this MOC.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A handle to a bidirectional association
     * involving this MOC.
     * @return MafOk, MafNotExist if the MOC isn't part of any bidirectional association, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getFirstBidirectionalAssociation)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrBiDirAssociationHandle_4T* subHandle);

    /**
     * Gets the first unidirectional association for this MOC.
     * @param[in] handle A MOC handle.
     * @param[out] subHandle A handle to a unidirectional association
     * involving this MOC.
     * @return MafOk, MafNotExist if the MOC isn't part of any unidirectional association, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getFirstUnidirectionalAssociation)(MafOamSpiMrMocHandle_4T handle, MafOamSpiMrUniDirAssociationHandle_4T* subHandle);


} MafOamSpiMrMoc_4T;


/**
 * Containment string properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrContainmentStringProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrContainmentStringProperty_4T;

/**
 * Containment boolean properties (
 */
typedef enum {
    /**
     * Containment property indicating if the associated MOC can be created over the NBI
     */
    MafOamSpiMrContainmentBoolProperty_canCreate_4 = 1,

    /**
     * Containment property indicating if the associated MOC can be deleted over the NBI
     */
    MafOamSpiMrContainmentBoolProperty_canDelete_4 = 2
} MafOamSpiMrContainmentBoolProperty_4T;


/**
 * Containment integer properties
 */
typedef enum {
    /**
     * Containment max number of children
     */
    MafOamSpiMrContainmentIntProperty_cardinalityMin_4 = 0,

    /**
     * Containment min number of children
     */
    MafOamSpiMrContainmentIntProperty_cardinalityMax_4 = 1
} MafOamSpiMrContainmentIntProperty_4T;


/**
 * Interface to access model information about containment associations.
 */
typedef struct MafOamSpiMrContainment_4 {
    /**
     * Gets the general properties.
     * @param[in] handle A containment handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the MOM that the containment belongs to.
     * @param[in] handle A containment handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrMomHandle_4T* subHandle);

    /**
     * Gets the child MOC.
     * @param[in] handle A containment handle.
     * @param[out] child A MOC handle that refers to the child MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getChildMoc)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrMocHandle_4T* child);

    /**
     * Gets the parent MOC.
     * @param[in] handle A containment handle.
     * @param[out] parent A MOC handle that refers to the parent MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParentMoc)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrMocHandle_4T* parent);

    /**
     * This function is used for iterating over the parent containments
     * of a certain Moc.To get the first containment
     * in the list the getParentContainment in MafOamSpiMrMoc_4T must be used.
     * Gets the next containment relation in the list that has the same
     * child MOC.
     * @param[in] handle A containment handle.
     * @param[out] next A containment handle that refers to the next containment.
     * @return MafOk, MafNotExist if there are no more relations, or one of the
     * other MafReturnT returns codes.
     */
    MafReturnT (*getNextContainmentSameChildMoc)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrContainmentHandle_4T* next);

    /**
     * This function is used for iterating over the child containments
     * of a certain  Moc.  To get the first containment
     * in the list the getChildContainment in MafOamSpiMrMoc_4T must be used.
     * Gets the next containment relation in the list that has the same
     * parent MOC.
     * @param[in] handle A containment handle.
     * @param[out] next A containment handle that refers to the next containment.
     * @return MafOk, MafNotExist if there are no more relations, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNextContainmentSameParentMoc)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrContainmentHandle_4T* next);

    /**
     * Gets the value of a containment string property
     * @param[in] handle A containment handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrContainmentStringProperty_4T property, const char** value);

    /**
     * Gets the value of a containment bool property
     * @param[in] handle A containment handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrContainmentBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a containment integer property
     * @param[in] handle A containment handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrContainmentHandle_4T handle, MafOamSpiMrContainmentIntProperty_4T property, int64_t* value);

} MafOamSpiMrContainment_4T;


/**
 * Association end string properties
 */
typedef enum {
    /**
     * name of the association end holding the reference to the target
     */
    MafOamSpiMrAssociationEndStringProperty_name_4 = 0,

    /**
     * target classifier for the association end
     */
    MafOamSpiMrAssociationEndStringProperty_classifier_4 = 1
} MafOamSpiMrAssociationEndStringProperty_4T;


/**
 * Association end boolean properties
 */
typedef enum {

    /**
     * indicate that the owning MO instance cannot be deleted
     * as long as the end exists
     */
    MafOamSpiMrAssociationEndBoolProperty_isReserving_4 = 0,

    /**
     * defines if the association end can only be set at create
     */
    MafOamSpiMrAssociationEndBoolProperty_isRestricted_4 = 1

} MafOamSpiMrAssociationEndBoolProperty_4T;


/**
 * Association end integer properties
 */
typedef enum {
    /**
     * Association cardinality min number of targets
     */
    MafOamSpiMrAssociationEndIntProperty_lower_4 = 0,

    /**
     * Association cardinality max number of targets
     */
    MafOamSpiMrAssociationEndIntProperty_upper_4 = 1
} MafOamSpiMrAssociationEndIntProperty_4T;



/**
 * Interface to access model information about MOC associations.
 */
typedef struct MafOamSpiMrAssociationEnd_4 {
    /**
     * Gets the association end MOC.
     * @param[in] handle An association handle.
     * @param[out] subHandle The MOC handle.
     * @return MafOk if the MOC was found, MafNotExist if it wasn't found, or
     * one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoc)(MafOamSpiMrAssociationEndHandle_4T handle, MafOamSpiMrMocHandle_4T* subHandle);

    /**
     * Gets the association end attribute.
     * @param[in] handle An association handle.
     * @param[out] subHandle An attribute handle.
     * @return MafOk if the attribute was found, MafNotExist if it wasn't found, or
     * one of the other MafReturnT return codes.
     */
    MafReturnT (*getAttribute)(MafOamSpiMrAssociationEndHandle_4T handle, MafOamSpiMrAttributeHandle_4T* subHandle);

    /**
     * Gets the general properties.
     * @param[in] handle An association handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrAssociationEndHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the value of an association end string property
     * @param[in] handle An association handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrAssociationEndHandle_4T handle, MafOamSpiMrAssociationEndStringProperty_4T property, const char** value);

    /**
     * Gets the value of an association end boolean property
     * @param[in] handle An association handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrAssociationEndHandle_4T handle, MafOamSpiMrAssociationEndBoolProperty_4T property, bool* value);

    /**
     * Gets the value of an association end integer property
     * @param[in] handle An association handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrAssociationEndHandle_4T handle, MafOamSpiMrAssociationEndIntProperty_4T property, int64_t* value);

} MafOamSpiMrAssociationEnd_4T;


/**
 * Type container string properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrTypeContainerStringProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrTypeContainerStringProperty_4T;


/**
 * Type container boolean properties
 */
typedef enum {
    /**
     * indicates if the order of the values in a multi-valued attribute has relevance
     */
    MafOamSpiMrTypeContainerBoolProperty_isOrdered_4 = 0,

    /**
     * indicates if each value in a multi-valued must be unique
     */
    MafOamSpiMrTypeContainerBoolProperty_isUnique_4 = 1
} MafOamSpiMrTypeContainerBoolProperty_4T;

/**
 * Type container integer properties
 */
typedef enum {
    /**
     * the minimum number of values
     */
    MafOamSpiMrTypeContainerIntProperty_multiplicityMin_4 = 0,

    /**
     * the maximum number of values
     */
    MafOamSpiMrTypeContainerIntProperty_multiplicityMax_4 = 1
} MafOamSpiMrTypeContainerIntProperty_4T;


/**
 * Interface to access model information about data types.
 */
typedef struct MafOamSpiMrTypeContainer_4 {

    /**
     * Gets the general properties. If the type is not defined by a derived type, it will
     * have no general properties (and return MafNotExist)
     * @param[in] handle A type container handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the data type.
     * @param[in] handle A type container handle.
     * @param[out] type The type.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getType)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrType_4T* type);

    /**
     * Gets a @a null terminated array of the default values. The values are set
     * automatically at the creation of an element.
     * @param[in] handle A type container handle.
     * @param[out] defaultValue The values.
     * @return MafOk, MafNotExist if there are no default values, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getDefaultValue)(MafOamSpiMrTypeContainerHandle_4T handle, const char*** defaultValue);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrTypeContainerStringProperty_4T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrTypeContainerBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrTypeContainerIntProperty_4T property, int64_t* value);

} MafOamSpiMrTypeContainer_4T;

/**
 * String type container string properties
 */
typedef enum {
    /**
     * a regular expression restricting the attribute value
     */
    MafOamSpiMrTypeContainerStringProperty_stringRestrictionPattern_4 = 0,
    /**
     * The exception text associated to the regular expression restriction.
     */
    MafOamSpiMrTypeContainerStringProperty_stringRestrictionExceptionText_4 = 1
} MafOamSpiMrStringStringProperty_4T;

/**
 * String type container string properties
 */
typedef enum {

    /**
     * Defines if the attribute represents a passphrase. Only relevant for string attributes.
     */
    MafOamSpiMrTypeContainerBoolProperty_isPassphrase_4 = 0
} MafOamSpiMrStringBoolProperty_4T;

/**
 * String type container integer properties
 */
typedef enum {
    /**
     * maximum length for a string
     */
    MafOamSpiMrTypeContainerStringProperty_maxLength_4 = 0,

    /**
     * minimum length for a string
     */
    MafOamSpiMrTypeContainerStringProperty_minLength_4 = 1
} MafOamSpiMrStringIntProperty_4T;

/**
 * Interface to access type information which is specific for string data types
 */
typedef struct MafOamSpiMrString_4 {

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrStringStringProperty_4T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrStringBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrStringIntProperty_4T property, int64_t* value);

} MafOamSpiMrString_4T;

/**
 * Integer type container string properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrIntStringProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrIntStringProperty_4T;

/**
 * Integer type container boolean properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrIntBoolProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrIntBoolProperty_4T;

/**
 * Integer type container integer properties (none so far)
 */
typedef enum {
    /**
     * the resolution of the value (0 means that any value is allowed, 10 means 0,10,20 etc. is allowed)
     */
    MafOamSpiMrIntIntProperty_resolution_4 = 0
} MafOamSpiMrIntIntProperty_4T;

/**
 * Interface to access type information which is specific for integer data types
 */
typedef struct MafOamSpiMrInt_4 {

    /**
     * Gets numeric restriction length. The property is optional and applicable
     * only if the type is numeric. Use length restriction for strings.
     * @param[in] handle A type handle.
     * @param[out] subHandle A value range handle that refers to the value range.
     * @return MafOk, MafNotExist if there is no value range, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNumericRestrictionRange)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrValueRangeHandle_4T* subHandle);

    /**
     * Gets min value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] min The min value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMin)(MafOamSpiMrValueRangeHandle_4T handle, int64_t* min);

    /**
     * Gets max value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] max The max value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMax)(MafOamSpiMrValueRangeHandle_4T handle, int64_t* max);

    /**
     * Gets the next value range.
     * @param[in] handle A value range handle.
     * @param[out] next A value range handle that refers to the next value range.
     * @return MafOk, MafNotExist if there are no more value ranges, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeNext)(MafOamSpiMrValueRangeHandle_4T handle, MafOamSpiMrValueRangeHandle_4T* next);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrIntStringProperty_4T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrIntBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrIntIntProperty_4T property, int64_t* value);

} MafOamSpiMrInt_4T;

/**
 * Float type container string properties (none so far)
 */
typedef enum {
    /**
      * The resolution of the value. For example in the range of
      * [-0.5 .. 5.5] and resolution 0.3 indicates that the values allowed are
      * -0.5, -0.2, 0.1 ... 5.5
      */
    MafOamSpiMrFloatStringProperty_resolution_4 = 0
} MafOamSpiMrFloatStringProperty_4T;

/**
 * Float type container boolean properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrFloatBoolProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrFloatBoolProperty_4T;

/**
 * Float type container float properties
 */
typedef enum {
    /**
      * The resolution of the value. For example in the range of
      * [-0.5 .. 5.5] and resolution 0.3 indicates that the values allowed are
      * -0.5, -0.2, 0.1 ... 5.5
      */
    MafOamSpiMrFloatFloatProperty_resolution_4 = 0
} MafOamSpiMrFloatFloatProperty_4T;


/**
 * Interface to access type information which is specific for float data types
 */
typedef struct MafOamSpiMrFloat_4 {

    /**
     * Gets numeric restriction length. The property is optional and applicable
     * only if the type is numeric. Use length restriction for strings.
     * @param[in] handle A type handle.
     * @param[out] subHandle A value range handle that refers to the value range.
     * @return MafOk, MafNotExist if there is no value range, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNumericRestrictionRange)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrValueRangeHandle_4T* subHandle);

    /**
     * Gets min value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] min The min value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMin)(MafOamSpiMrValueRangeHandle_4T handle, double* min);

    /**
     * Gets max value of the value range.
     * @param[in] handle A value range handle.
     * @param[out] max The max value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeMax)(MafOamSpiMrValueRangeHandle_4T handle, double* max);

    /**
     * Gets the next value range.
     * @param[in] handle A value range handle.
     * @param[out] next A value range handle that refers to the next value range.
     * @return MafOk, MafNotExist if there are no more value ranges, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getValueRangeNext)(MafOamSpiMrValueRangeHandle_4T handle, MafOamSpiMrValueRangeHandle_4T* next);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrFloatStringProperty_4T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrFloatBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a type container float property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getFloatProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrFloatFloatProperty_4T property, double* value);

} MafOamSpiMrFloat_4T;

/**
 * MoRef type container string properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrMoRefStringProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrMoRefStringProperty_4T;

/**
 * MoRef type container boolean properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrMoRefBoolProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrMoRefBoolProperty_4T;

/**
 * MoRef type container integer properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrMoRefIntProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrMoRefIntProperty_4T;

typedef struct MafOamSpiMrMoRef_4 {

    /**
     * Gets the MOC that is referenced.
     * @param[in] handle A type container handle.
     * @param[out] subHandle A MOC handle that refers to the referenced MOC.
     * @return MafOk, MafNotExist if the type is not reference, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getReferencedMoc)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrMocHandle_4T* subHandle);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrMoRefStringProperty_4T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrIntBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrMoRefIntProperty_4T property, int64_t* value);

} MafOamSpiMrMoRef_4T;


/**
 * Interface to access type information which is specific for enum data types
 */
typedef struct MafOamSpiMrEnum_4 {
    /**
     * Gets the general properties.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the first literal.
     * @param[in] handle An enum handle.
     * @param[out] subHandle A literal handle that refers to the first literal.
     * @return MafOk, MafNotExist it there are no literals, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getLiteral)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrEnumLiteralHandle_4T* subHandle);
    /**
     * Gets the general properties of a literal.
     * @param[in] handle A literal handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getLiteralGeneralProperties)(MafOamSpiMrEnumLiteralHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);
    /**
     * Gets the value of a literal.
     * @param[in] handle A literal handle.
     * @param[out] value The value.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getLiteralValue)(MafOamSpiMrEnumLiteralHandle_4T handle, int64_t* value);

    /**
     * Gets the next literal.
     * @param[in] handle A literal handle.
     * @param[out] next A literal handle that refers to the next literal.
     * @return MafOk, MafNotExist of there are no more literals, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getLiteralNext)(MafOamSpiMrEnumLiteralHandle_4T handle, MafOamSpiMrEnumLiteralHandle_4T* next);
} MafOamSpiMrEnum_4T;

/**
 * Struct type container string properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrStructStringProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrStructStringProperty_4T;

/**
 * Struct type container boolean properties
 */
typedef enum {
    /**
     * an exclusive struct is acting like a union, where only one element at a time
     * can have a value
     */
    MafOamSpiMrStructBoolProperty_isExclusive_4 = 0
} MafOamSpiMrStructBoolProperty_4T;

/**
 * Struct type container integer properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrStructIntProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrStructIntProperty_4T;

/**
 * Struct member string properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrStructMemberStringProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrStructMemberStringProperty_4T;

/**
 * Struct member integer properties (none so far)
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrStructMemberIntProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrStructMemberIntProperty_4T;

/**
 * Struct member boolean properties
 */
typedef enum {
    MafOamSpiMrStructMemberBoolProperty_isKey_4 = 0
} MafOamSpiMrStructMemberBoolProperty_4T;

/**
 * Interface to access type information which is specific for struct data types
 */
typedef struct MafOamSpiMrStruct_4 {

    /**
     * Gets the first member of a struct.
     * @param[in] handle A struct handle.
     * @param[out] subHandle A member handle that refers to the member.
     * @return MafOk, MafNotExist if there are no members, or one of the other
     * MafReturnT return codes.
     */
    MafReturnT (*getMember)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrStructMemberHandle_4T* subHandle);

    /**
     * Gets the general properties of a member.
     * @param[in] handle A member handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMemberGeneralProperties)(MafOamSpiMrStructMemberHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the type container of a member.
     * @param[in] handle A member handle.
     * @param[out] subHandle A type container that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMemberTypeContainer)(MafOamSpiMrStructMemberHandle_4T handle, MafOamSpiMrTypeContainerHandle_4T* subHandle);

    /**
     * Gets the next member.
     * @param[in] handle A member handle.
     * @param[out] next A member handle that refers to the next member.
     * @return MafOk, MafNotExist if there are no more members, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getMemberNext)(MafOamSpiMrStructMemberHandle_4T handle, MafOamSpiMrStructMemberHandle_4T* next);

    /**
     * Gets the value of a type container string property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrStructStringProperty_4T property, const char** value);

    /**
     * Gets the value of a type container boolean property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrStructBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a type container integer property
     * @param[in] handle A type container handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrTypeContainerHandle_4T handle, MafOamSpiMrStructIntProperty_4T property, int64_t* value);

    /**
     * Gets the value of a struct member string property
     * @param[in] handle A struct member handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getMemberStringProperty)(MafOamSpiMrStructMemberHandle_4T handle, MafOamSpiMrStructMemberStringProperty_4T property, const char** value);

    /**
     * Gets the value of a struct member boolean property
     * @param[in] handle A struct member handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getMemberBoolProperty)(MafOamSpiMrStructMemberHandle_4T handle, MafOamSpiMrStructMemberBoolProperty_4T property, bool* value);

    /**
     * Gets the value of a struct member integer property
     * @param[in] handle A struct member handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getMemberIntProperty)(MafOamSpiMrStructMemberHandle_4T handle, MafOamSpiMrStructMemberIntProperty_4T property, int64_t* value);

} MafOamSpiMrStruct_4T;

/**
 * Attribute string properties
 */
typedef enum {
    /**
     * the unit of the attribute
     */
    MafOamSpiMrAttributeStringProperty_unit_4 = 0
} MafOamSpiMrAttributeStringProperty_4T;

/**
 * Attribute boolean properties
 */
typedef enum {
    /**
     * true if the attribute is the key attribute for the MO class
     */
    MafOamSpiMrAttributeBoolProperty_isKey_4 = 0,

    /**
     * Mandatory means that there must be at least one value
     * this property can be derived from multiplicity and default value
     */
    MafOamSpiMrAttributeBoolProperty_isMandatory_4 = 1,

    /**
     * Defines if the attribute can be written over the NBI (isReadOnly == false)
     */
    MafOamSpiMrAttributeBoolProperty_isReadOnly_4 = 2,

    /**
     * Defines if the attribute can only be set at create
     */
    MafOamSpiMrAttributeBoolProperty_isRestricted_4 = 3,

    /**
     * Defines if the attribute should emit Attribute Value Change notifications
     */
    MafOamSpiMrAttributeBoolProperty_isNotifiable_4 = 4

} MafOamSpiMrAttributeBoolProperty_4T;

/**
 * Attribute integer properties
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrAttributeIntProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrAttributeIntProperty_4T;

/**
 * Interface to access model information about attributes.
 */
typedef struct MafOamSpiMrAttribute_4 {
    /**
     * Gets the general properties.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the MOC an attribute belongs to.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A MOC handle that refers to the MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoc)(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrMocHandle_4T* subHandle);

    /**
     * Gets the type container.
     * @param[in] handle An attribute handle.
     * @param[out] subHandle A type container handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getTypeContainer)(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrTypeContainerHandle_4T* subHandle);

    /**
     * Gets the next attribute.
     * @param[in] handle An attribute handle.
     * @param[out] next An attribute handle that refers to the next attribute.
     * @return MafOk, MafNotExist if there are no more attributes, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrAttributeHandle_4T* next);

    /**
     * Gets the value of an attribute string property
     * @param[in] handle An attribute handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrAttributeStringProperty_4T property, const char** value);

    /**
     * Gets the value of an attribute boolean property
     * @param[in] handle An attribute handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrAttributeBoolProperty_4T property, bool* value);

    /**
     * Gets the value of an attribute integer property
     * @param[in] handle An attribute handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrAttributeHandle_4T handle, MafOamSpiMrAttributeIntProperty_4T property, int64_t* value);

} MafOamSpiMrAttribute_4T;

/**
 * Action string properties
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrActionStringProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrActionStringProperty_4T;

/**
 * Action boolean properties
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrActionBoolProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrActionBoolProperty_4T;

/**
 * Action integer properties
 */
typedef enum {
#ifndef __cplusplus
    MafOamSpiMrActionIntProperty_4T_dummyNotUsed = 0
#endif
} MafOamSpiMrActionIntProperty_4T;

/**
 * Interface to access model information about actions.
 */
typedef struct MafOamSpiMrAction_4 {
    /**
     * Gets the general properties.
     * @param[in] handle An action handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrActionHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the MOC that an action belongs to.
     * @param[in] handle An action handle.
     * @param[out] subHandle A MOC handle that refers to the MOC.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMoc)(MafOamSpiMrActionHandle_4T handle, MafOamSpiMrMocHandle_4T* subHandle);

    /**
     * Gets the type container  of the action result value.
     * @param[in] handle An action handle.
     * @param[out] subHandle A type container handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getTypeContainer)(MafOamSpiMrActionHandle_4T handle, MafOamSpiMrTypeContainerHandle_4T* subHandle);

    /**
     * Gets the next action.
     * @param[in] handle An action handle.
     * @param[out] next An action handle that refers to the next action.
     * @return MafOk, MafNotExist if there are no more actions, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getNext)(MafOamSpiMrActionHandle_4T handle, MafOamSpiMrActionHandle_4T* next);

    /**
     * Gets the first parameter of an action.
     * @param[in] handle An action handle.
     * @param[out] subHandle A parameter handle that refers to the first parameter.
     * @return MafOk, MafNotExist if there is no parameters, or one of the
     * other MafReturnT return codes.
     */
    MafReturnT (*getParameter)(MafOamSpiMrActionHandle_4T handle, MafOamSpiMrParameterHandle_4T* subHandle);

    /**
     * Gets the general properties of a parameter.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterGeneralProperties)(MafOamSpiMrParameterHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the action that a parameter belongs to.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle An action handle that refers to the action.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterAction)(MafOamSpiMrParameterHandle_4T handle, MafOamSpiMrActionHandle_4T* subHandle);

    /**
     * Gets the type container of a parameter.
     * @param[in] handle A parameter handle.
     * @param[out] subHandle A type handle that refers to the type container.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getParameterTypeContainer)(MafOamSpiMrParameterHandle_4T handle, MafOamSpiMrTypeContainerHandle_4T* subHandle);

    /**
     * Gets the next parameter.
     * @param[in] handle A parameter handle.
     * @param[out] next A parameter handle that refers to the next parameter.
     * @return MafOk, MafNotExist if there are no more parameters, or one of
     * the other MafReturnT return codes.
     */
    MafReturnT (*getParameterNext)(MafOamSpiMrParameterHandle_4T handle, MafOamSpiMrParameterHandle_4T* next);

    /**
     * Gets the value of an action string property
     * @param[in] handle An action handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getStringProperty)(MafOamSpiMrActionHandle_4T handle, MafOamSpiMrActionStringProperty_4T property, const char** value);

    /**
     * Gets the value of an action boolean property
     * @param[in] handle An action handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getBoolProperty)(MafOamSpiMrActionHandle_4T handle, MafOamSpiMrActionBoolProperty_4T property, bool* value);

    /**
     * Gets the value of an action integer property
     * @param[in] handle An action handle.
     * @param[in] property identifies which property to retrieve
     * @param[out] value the value of the property
     * @return MafOk if the property is retrieved properly or other MafReturnT return codes.
     * MafNotExist indicates that the property does not exist.
     */
    MafReturnT (*getIntProperty)(MafOamSpiMrActionHandle_4T handle, MafOamSpiMrActionIntProperty_4T property, int64_t* value);

} MafOamSpiMrAction_4T;


/**
 * Interface to access model information about bidirectional associations.
 */
typedef struct MafOamSpiMrBiDirAssociation_4 {

    /**
     * Gets the general properties.
     * @param[in] handle A bidirectional association handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrBiDirAssociationHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the MOM that the bidirectional association belongs to.
     * @param[in] handle A bidirectional association handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMrBiDirAssociationHandle_4T handle, MafOamSpiMrMomHandle_4T* subHandle);

    /**
     * Gets the first associated end for the bidirectional association.
     * @param[in] handle A bidirectional association handle.
     * @param[out] subHandle A handle to the first association end.
     * @return MafOk or one of the other MafReturnT return codes.
     */
    MafReturnT (*getFirstAssociationEnd)(MafOamSpiMrBiDirAssociationHandle_4T handle, MafOamSpiMrAssociationEndHandle_4T* subHandle);

    /**
     * Gets the second associated end for the bidirectional association.
     * @param[in] handle A bidirectional association handle.
     * @param[out] subHandle A handle to the second association end.
     * @return MafOk or one of the other MafReturnT return codes.
     */
    MafReturnT (*getSecondAssociationEnd)(MafOamSpiMrBiDirAssociationHandle_4T handle, MafOamSpiMrAssociationEndHandle_4T* subHandle);

    /**
     * This function is used for iterating over the bidirectional associations.
     * To get the first association in the list
     * the getFirstBidirectionalAssociation in MafOamSpiMrMoc_4T must be used.
     * Gets the next association in the list for the referenced MOC.
     * @param[in] handle A handle to a bidirectional association.
     * @param[in] mocHandle A handle to the referenced moc.
     * @param[out] subHandle A handle to the next bidirectional association
     * involving this MOC.
     * @return MafOk, MafNotExist if there are no more bidirectional associations for the referenced MOC, or one of
     * the other MafReturnT return code
    */
    MafReturnT (*getNextBidirectionalAssociation)(MafOamSpiMrBiDirAssociationHandle_4T handle, MafOamSpiMrMocHandle_4T mocHandle, MafOamSpiMrBiDirAssociationHandle_4T* next);

} MafOamSpiMrBiDirAssociation_4T;

/**
 * Interface to access model information about unidirectional associations.
 */
typedef struct MafOamSpiMrUniDirAssociation_4 {

    /**
     * Gets the general properties.
     * @param[in] handle A unidirectional association handle.
     * @param[out] subHandle A general properties handle that refers to the
     * general properties.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getGeneralProperties)(MafOamSpiMrUniDirAssociationHandle_4T handle, MafOamSpiMrGeneralPropertiesHandle_4T* subHandle);

    /**
     * Gets the MOM that the containment belongs to.
     * @param[in] handle A unidirectional association handle.
     * @param[out] subHandle A MOM handle that refers to the MOM.
     * @return MafOk, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMrUniDirAssociationHandle_4T handle, MafOamSpiMrMomHandle_4T* subHandle);

    /**
     * Gets the associated end for the unidirectional association.
     * @param[in] handle A unidirectional association handle.
     * @param[out] subHandle A handle to the association end.
     * @return MafOk or one of the other MafReturnT return codes.
     */
    MafReturnT (*getAssociationEnd)(MafOamSpiMrUniDirAssociationHandle_4T handle, MafOamSpiMrAssociationEndHandle_4T* subHandle);

    /**
     * This function is used for iterating over the unidirectional associations.
     * To get the first association in the list
     * the getFirstUnidirectionalAssociation in MafOamSpiMrMoc_4T must be used.
     * Gets the next association in the list that has the same MOC.
     * @param[in] handle A handle to a unidirectional association.
     * @param[out] subHandle A handle to the next unidirectional association
     * involving this MOC.
     * @return MafOk, MafNotExist if there are no more unidirectional associations for the referenced MOC, or one of
     * the other MafReturnT return code
    */
    MafReturnT (*getNextUnidirectionalAssociationSameMoc)(MafOamSpiMrUniDirAssociationHandle_4T handle, MafOamSpiMrUniDirAssociationHandle_4T* next);

} MafOamSpiMrUniDirAssociation_4T;


/**
 * Interface to access first step modeling elements like MOMs and MOCs.
 */
typedef struct MafOamSpiMrEntry_4 {
    /**
     * Gets the first MOM that has been registered in the Model Repository.
     * @param[out] handle A MOM handle that refers to the first MOM.
     * @return MafOk if at least one MOM was found, MafNotExist if no MOMs are
     * available, or one of the other MafReturnT return codes.
     */
    MafReturnT (*getMom)(MafOamSpiMrMomHandle_4T* handle);

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
    MafReturnT (*getMoc)(const char* momName, const char* momVersion, const char* mocName, MafOamSpiMrMocHandle_4T* handle);

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
    MafReturnT (*getMocFromDn)(const char* dn, bool checkFullMocPath, MafOamSpiMrMocHandle_4T* handle);

    /**
     * Gets the root MOC of the top MOM.
     * @param[out] handle A MOC handle that refers to the root MOC.
     * @return MafOk if the root MOC was found, MafNotExist if it wasn't found,
     * or one of the other MafReturnT return codes.
     */
    MafReturnT (*getRoot)(MafOamSpiMrMocHandle_4T* handle);

} MafOamSpiMrEntry_4T;

/**
 * Second version of Model Repository (MR) interface. Contains initialized
 * instances of all the other MR structures forming an interface to dynamically
 * access model information from parsed MAF model files.
 *
 * @code
 * // Simple how-to use the interface examples
 *
 * // Get a handle to the first mom in the list of moms
 * MafOamSpiMrMomHandle_4T momHandle;
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
 * MafOamSpiMrMocHandle_4T mocHandle;
 * retVal = mr->entry->getMocFromDn("ManagedElement=1,SystemFunctions=1", false, &mocHandle);
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 *
 * // Iterate over the root moc's attributes
 * MafOamSpiMrAttributeHandle_4T attrHandle;
 * retVal = mr->moc->getAttribute(mocHandle, &attrHandle);
 * while (retVal == MafOk) {
 *     bool isKey = false;
 *     retVal = mr->attribute->getBoolProperty(attrHandle, MafOamSpiMrAttributeBoolProperty_isKey_4, &isKey);
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
 * MafOamSpiMrContainmentHandle_4T childContainmentHandle;
 * retVal = mr->moc->getChildContainment(mocHandle, &childContainmentHandle));
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 * MafOamSpiMrMocHandle_4T childMocHandle;
 * retVal = mr->containment->getChildMoc(childContainmentHandle, &childMocHandle));
 * MafOamSpiMrContainmentHandle_4T parentOfChildContainmentHandle;
 * retVal = mr->moc->getParentContainment(childMocHandle, &parentOfChildContainmentHandle));
 * if (retVal != MafOk) {
 *     // Handle error code
 * }
 *
 * @endcode
 */
typedef struct MafOamSpiModelRepository_4 {
    /**
     * Interface identification.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Interface for accessing entry definitions.
     */
    MafOamSpiMrEntry_4T* entry;

    /**
     * Interface for accessing general properties definitions.
     */
    MafOamSpiMrGeneralProperties_4T* generalProperties;

    /**
     * Interface for accessing MOM definitions.
     */
    MafOamSpiMrMom_4T* mom;

    /**
     * Interface for accessing MOC definitions.
     */
    MafOamSpiMrMoc_4T* moc;

    /**
     * Interface for accessing containment definitions.
     */
    MafOamSpiMrContainment_4T* containment;

    /**
     * Interface for accessing association end definitions.
     */
    MafOamSpiMrAssociationEnd_4T* associationEnd;

    /**
     * Interface for accessing bidirectional association end definitions.
     */
    MafOamSpiMrBiDirAssociation_4T* bidirectionalAssociation;

    /**
     * Interface for accessing unidirectional association end definitions.
     */
    MafOamSpiMrUniDirAssociation_4T* unidirectionalAssociation;

    /**
     * Interface for accessing type container definitions.
     */
    MafOamSpiMrTypeContainer_4T* typeContainer;

    /**
     * Interface for accessing enum definitions.
     */
    MafOamSpiMrEnum_4T* enumType;

    /**
     * Interface for accessing struct definitions.
     */
    MafOamSpiMrStruct_4T* structType;

    /**
     * Interface for accessing attribute definitions.
     */
    MafOamSpiMrAttribute_4T* attribute;

    /**
     * Interface for accessing action definitions.
     */
    MafOamSpiMrAction_4T* action;

    /**
     * Interface for accessing MoRef definitions.
     */
    MafOamSpiMrMoRef_4T* moRef;

    /**
     * Interface for accessing integer definitions.
     */
    MafOamSpiMrInt_4T* intIf;

    /**
     * Interface for accessing string definitions.
     */
    MafOamSpiMrString_4T* stringIf;

    /**
     * Interface for accessing float definitions.
     */
    MafOamSpiMrFloat_4T* floatIf;

} MafOamSpiModelRepository_4T;

#endif // _MafOamSpiMrModelRepository_4_h_
