#ifndef MafOamSpiModelRepository_1_h
#define MafOamSpiModelRepository_1_h

#include <MafMgmtSpiInterface_1.h>
#include <MafMgmtSpiInterfacePortal_1.h>
#include <MafMgmtSpiInterfacePortalAccessor.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * Model Repository interface.
 *
 * @file MafOamSpiModelRepository_1.h
 * @ingroup MafOamSpi
 *
 * This interface defines the Model Repository SPI. It follows
 * the Ericsson Common Information Model (ECIM) Domain Specific
 * Language (DSL).
 *
 * The Model Repository service identity is found in file
 * MafOamSpiServices.h.
 */

/**
 * Maximum allowed number for a 64-bit (signed) integer. Used to
 * denote a maximum upper limit in ranges.
 */
#define MAX_INT64 9223372036854775807LL;

/**
 * Minimum allowed number for a 64-bit (signed) integer. Used to
 * denote a minimum lower limit in ranges.
 */
#define MIN_INT64 -9223372036854775807LL;

// forward declaration
typedef const struct MafOamSpiMom MafOamSpiMomT;
typedef const struct MafOamSpiMoc MafOamSpiMocT;
typedef const struct MafOamSpiMoAttribute MafOamSpiMoAttributeT;
typedef const struct MafOamSpiMoAction MafOamSpiMoActionT;
typedef const struct MafOamSpiParameter MafOamSpiParameterT;
typedef const struct MafOamSpiContainment MafOamSpiContainmentT;
typedef const struct MafOamSpiStruct MafOamSpiStructT;
typedef struct MafOamSpiValueRange MafOamSpiValueRangeT;
typedef struct MafOamSpiStructMember MafOamSpiStructMemberT;
typedef struct MafOamSpiEnumLiteral MafOamSpiEnumLiteralT;

/**
 * Defines the list of the valid data types.
 *
 * @deprecated replaced by MafOamSpiDatatype
 */
typedef enum MafOamSpiMoAttributeType {
    /**
     * An 8-bit integer.
     */
    MafOamSpiMoAttributeType_INT8 = 1,
    /**
     * A 16-bit integer.
     */
    MafOamSpiMoAttributeType_INT16 = 2,
    /**
     * A 32-bit integer.
     */
    MafOamSpiMoAttributeType_INT32 = 3,
    /**
     * A 64-bit integer.
     */
    MafOamSpiMoAttributeType_INT64 = 4,
    /**
     * An 8-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_UINT8 = 5,
    /**
     * A 16-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_UINT16 = 6,
    /**
     * A 32-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_UINT32 = 7,
    /**
     * A 64-bit unsigned integer.
     */
    MafOamSpiMoAttributeType_UINT64 = 8,
    /**
     * A string value.
     */
    MafOamSpiMoAttributeType_STRING = 9,
    /**
     * A boolean.
     */
    MafOamSpiMoAttributeType_BOOL = 10,
    /**
     * A reference to another Managed Object (MO) class.
     */
    MafOamSpiMoAttributeType_REFERENCE = 11,
    /**
     * An enumeration.
     */
    MafOamSpiMoAttributeType_ENUM = 12,
    /**
     * A derived data type.
     */
    MafOamSpiMoAttributeType_DERIVED = 13,
    /**
     * A struct or aggregated data type.
     */
    MafOamSpiMoAttributeType_STRUCT = 14,
    /**
     * A void data type.
     */
    MafOamSpiMoAttributeType_VOID = 15
} MafOamSpiMoAttributeTypeT;

/**
 * Defines the list of the valid data types.
 */
typedef enum MafOamSpiDatatype {
    /**
     * An 8-bit integer.
     */
    MafOamSpiDatatype_INT8 = 1,
    /**
     * A 16-bit integer.
     */
    MafOamSpiDatatype_INT16 = 2,
    /**
     * A 32-bit integer.
     */
    MafOamSpiDatatype_INT32 = 3,
    /**
     * A 64-bit integer.
     */
    MafOamSpiDatatype_INT64 = 4,
    /**
     * An 8-bit unsigned integer.
     */
    MafOamSpiDatatype_UINT8 = 5,
    /**
     * A 16-bit unsigned integer.
     */
    MafOamSpiDatatype_UINT16 = 6,
    /**
     * A 32-bit unsigned integer.
     */
    MafOamSpiDatatype_UINT32 = 7,
    /**
     * A 64-bit unsigned integer.
     */
    MafOamSpiDatatype_UINT64 = 8,
    /**
     * A string value.
     */
    MafOamSpiDatatype_STRING = 9,
    /**
     * A boolean.
     */
    MafOamSpiDatatype_BOOL = 10,
    /**
     * A reference to another Managed Object class.
     */
    MafOamSpiDatatype_REFERENCE = 11,
    /**
     * An enumeration.
     */
    MafOamSpiDatatype_ENUM = 12,
    /**
     * A derived data type.
     */
    MafOamSpiDatatype_DERIVED = 13,
    /**
     * A struct or aggregated data type.
     */
    MafOamSpiDatatype_STRUCT = 14,
    /**
     * A void data type.
     */
    MafOamSpiDatatype_VOID = 15
} MafOamSpiDatatypeT;

/**
 * Describes the different status properties (current, deprecated,
 * and obsolete) that the modeling elements can be assigned.
 */
typedef enum MafOamSpiStatus {
    /**
     * The definition is current and valid.
     */
    MafOamSpiStatus_CURRENT = 1,
    /**
     * The definition is deprecated, but it permits new or continued
     * implementation in order to foster interoperability with older
     * or existing implementations.
     */
    MafOamSpiStatus_DEPRECATED = 2,
    /**
     * The definition is obsolete. It is not recommended to implement.
     * It can be removed if previously implemented.
     */
    MafOamSpiStatus_OBSOLETE = 3
} MafOamSpiStatusT;

/**
 * Type declaration for the extension container.
 */
typedef struct MafOamSpiExtension MafOamSpiExtensionT;

/**
 * The container for the extension.
 */
struct MafOamSpiExtension {
    /**
     * The name of the extension.
     */
    const char* name;
    /**
     * The value of the extension.
     */
    const char* value;
    /**
     * The pointer to the next extension, or null if there are no more
     * extensions.
     */
    MafOamSpiExtensionT* next;
};

/**
 * The container for the domain extension.
 */
typedef struct MafOamSpiDomainExtension {
    /**
     * The identifier of the domain extension.
     */
    const char* domain;
    /**
     * A pointer to the first extension, or null if no extensions.
     */
    MafOamSpiExtensionT* extensions;
} MafOamSpiDomainExtensionT;

/**
 * A container for general properties (for example, name and description).
 */
typedef struct MafOamSpiGeneralProperties {
    /**
     * The name of this modeling element.
     */
    char * name;
    /**
     * Contains a meaningful description of this model element.
     */
    char * description;
    /**
     * Reference to a standard.
     */
    char * specification;
    /**
     * Shows if the modeling element is current, deprecated, or obsolete.
     */
    MafOamSpiStatusT status;
    /**
     * The hidden property can be used to hide information from the
     * northbound interfaces and the CPI. All modeling elements with
     * the same value for this <em>hidden</em> attribute are considered
     * to form a group and are treated the same with respect to
     * being visible or not in a northbound interface.
     */
    char * hidden;
} MafOamSpiGeneralPropertiesT;

/**
 * Describes the Managed Object Model.
 */
struct MafOamSpiMom {

    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * The version of the MOM. If null, then only the name is used
     * to identify the MOM.
     */
    char * version;
    /**
     * MOM release identity. Within one MOM version there can be several
     * backward-compatible releases.
     */
    char * release;
    /**
     * URI of the model's own name space. It is proposed to be in URN
     * format, as defined by RFC 2396.
     */
    char * namespaceURI;
    /**
     * A globally unique identifier of the namespace. The namespace
     * prefix is normally derived directly from the name of the EcimMom,
     * converted to lower case characters.
     *
     */
    char * namespacePrefix;
    /**
     * Reference to the MOM's one and only root MO class.
     */
    MafOamSpiMocT * rootMoc;
    /**
     * MOM document number.
     */
    char * docNo;
    /**
     * MOM document revision.
     */
    char * revision;
    /**
     * Author of the MOM.
     */
    char * author;
    /**
     * Author's organization.
     */
    char * organization;
    /**
     * Reference to the next MOM, or null if
     * this MOM is the last in the list.
     */
    MafOamSpiMomT *next;
};

/**
 * Contains a range of values used by attribute multiplicities and
 * containment cardinalities.
 */
typedef struct MafOamSpiMultiplicityRange {
    /**
     * Minimum or lower bound value.
     */
    int16_t min;
    /**
     * Maximum or upper bound value.
     */
    int16_t max;
} MafOamSpiMultiplicityRangeT;


/**
 * Contains a range of values used by numeric restricted data types.
 */
struct MafOamSpiValueRange {
    /**
     * Minimum or lower bound value.
     */
    int64_t min;

    /**
     * Maximum or upper bound value.
     */
    int64_t max;

    /**
     * Reference to the next range, if such exist.
     */
    MafOamSpiValueRangeT * next;
};


/**
 * Model information about an MO class.
 */
struct MafOamSpiMoc {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    MafOamSpiMomT * mom;

    /**
     * Read-only classes can not be created or deleted from the NBI.
     * A read-only class can only contain read-only attributes.
     */
    bool isReadOnly;

    /**
     * Indicates if this is the MOM's root MO class.
     */
    bool isRoot;

    /**
     * A constraint expressed in the Object Constraint Language (OCL).
     * Note: This is currently not supported since it is not yet known
     * how to validate an OCL expression in MAF.
     */
    char * constraint;

    /**
     * Reference to the first MO attribute that belongs to this
     * MO class.
     */
    MafOamSpiMoAttributeT * moAttribute;

    /**
     * Reference to the first MO action that belongs to this
     * MO class.
     */
    MafOamSpiMoActionT * moAction;

    /**
     * Reference to the first child containment relationship,
     * or null if this MO class is a leaf.
     */
    MafOamSpiContainmentT * childContainment;

    /**
     * Reference to the first parent containment relationship,
     * or null if this is the root MO class of the top MOM.
     */
    MafOamSpiContainmentT * parentContainment;

};

/**
 * Model information about a containment relationship, that is
 * a parent-child relationship between two MO classes.
 */
struct MafOamSpiContainment {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    MafOamSpiMomT * mom;

    /**
     * Parent MO class of this containment relationship.
     */
    MafOamSpiMocT * parentMoc;

    /**
     * Child MO class of this containment relationship.
     */
    MafOamSpiMocT * childMoc;

    /**
     * Range of child MOs that can be contributed under the parent MO.
     */
    MafOamSpiMultiplicityRangeT cardinality;

    /**
     * Indicates if this containment relationship and the corresponding
     * child MO is automatically created by the NE.
     * Note: This property is currently not supported.
     */
    bool isSystemCreated;

    /**
     * Reference to the next containment relationship in the list that
     * has the same parent MO classi, null if no more relation exist.
     */
    MafOamSpiContainmentT * nextContainmentSameParentMoc;

    /**
     * Reference to the next containment relationship in the list that
     * has the same child MO class, null if no more relation exist.
     */
    MafOamSpiContainmentT * nextContainmentSameChildMoc;
};

/**
 * Describes a derived data type, that is a string or a numeric data type
 * that has been named and restricted.
 */
typedef const struct MafOamSpiDerivedDatatype {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    MafOamSpiMomT * mom;

    /**
     * Restricted data type.
     */
    MafOamSpiMoAttributeTypeT type;

    /**
     * If type equals MafOamSpiMoAttributeType_STRING this property
     * may be set with a pattern according to POSIX BASIC Regular Expression.
     */
    char * stringRestrictionPattern;

    /**
     * If type equals MafOamSpiMoAttributeType_STRING this property
     * may be set with a length range for the string value.
     */
    MafOamSpiValueRangeT stringRestrictionLength;

    /**
     * If type is numeric this property defines the allowed ranges.
     */
    MafOamSpiValueRangeT * numericRestrictionRange;
} MafOamSpiDerivedDatatypeT;

/**
 * Describes an enumeration literal.
 */
struct MafOamSpiEnumLiteral {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Integer value of the enumeration literal.
     */
    int16_t value;

    /**
     * Reference to the next literal of this enumeration,
     * or null if there are no more literals.
     */
    MafOamSpiEnumLiteralT * next;

};

/**
 * Model information about an enumeration.
 */
typedef const struct MafOamSpiEnum {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    MafOamSpiMomT * mom;

    /**
     * The enumeration literals.
     */
    MafOamSpiEnumLiteralT *literals;
} MafOamSpiEnumT;

/**
 * Container for the datatype and its related parameters.
 */
typedef struct MafOamSpiDatatypeContainer {
    /**
     * Data type of this element which can be an attribute,
     * an action parameter, an action result or a struct member.
     */
    MafOamSpiDatatypeT type;

    /**
     * Derived data type.
     * If the element is MafOamSpiDatatype_DERIVED then
     * this field contains a pointer to the derived data type.
     */
    MafOamSpiDerivedDatatypeT * derivedDatatype;

    /**
     * Enumeration data type.
     * If the element is MafOamSpiDatatype_ENUM then
     * this field contains a pointer to the enumeration data type.
     */
    MafOamSpiEnumT * enumDatatype;

    /**
     * Struct data type.
     * If the element is a MafOamSpiDatatype_STRUCT then
     * this field contains a pointer to the struct data type.
     */
    MafOamSpiStructT * structDatatype;

    /**
     * The MO class that is referenced.
     * If the element is MafOamSpiDatatype_REFERENCE then
     * this field contains a pointer to the referenced MO class.
     */
    MafOamSpiMocT * referencedMoc;

    /**
     * Value that is set automatically at the creation of the element.
     * If the value is a multivalue then space is the separator
     * between the subvalues, If a subvalue contains space then each
     * space must be preceded by a '\'. If a subvalue contains '\'
     * then each '\' must be preceeded by a '\'.
     */
    char * defaultValue;

    /**
     * Indicates that the element contains a list of values.
     * The property conveys the exact range. If the lower bound is 0,
     * a value does not have to be supplied at create.
     * However, deleting a member of attribute/enum/struct or action
     * once created is not supported. Note: If the multiplicity
     * lower bound is 1 (or higher), then the mandatory property
     * equals true. If the multiplicity lower bound is 0, then the
     * mandatory property equals false.
     */
    MafOamSpiMultiplicityRangeT multiplicity;

    /**
     * Indicates that the values of the members are in a specific
     * order, that is, the same sets of values are different
     * if the order of the values is not the same.
     * Note: This property is not supported since it is currently
     * not part of the mp.dtd definition.
     */
    bool isOrdered;

    /**
     * Indicates that the values are unique (within all MOs of
     * the same EcimMoClass).
     */
    bool isUnique;
} MafOamSpiDatatypeContainerT;

/**
 * Describes a struct member for the struct modeling element.
 */
struct MafOamSpiStructMember {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Member type.
     */
    MafOamSpiDatatypeContainerT memberType;

    /**
     * Reference to the next struct member of this struct modeling element,
     * or null if there are no more members.
     */
    MafOamSpiStructMemberT * next;
};

/**
 * Model information about a struct.
 */
struct MafOamSpiStruct {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    MafOamSpiMomT * mom;

    /**
     * Reference to the first struct member modeling element
     */
    MafOamSpiStructMemberT * members;
};


/**
 * Model information about an MO attribute.
 */
struct MafOamSpiMoAttribute {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MO class of this attribute.
     */
    MafOamSpiMocT * moc;

    /**
     * Data type of this attribute.
     */
    MafOamSpiMoAttributeTypeT type;

    /**
     * Derived data type.
     * If the attribute's type is MafOamSpiMoAttributeType_DERIVED then
     * this field contains a pointer to the derived data type.
     */
    MafOamSpiDerivedDatatypeT * derivedDatatype;

    /**
     * Enumeration data type.
     * If the attribute's type is MafOamSpiMoAttributeType_ENUM then
     * this field contains a pointer to the enumeration data type.
     */
    MafOamSpiEnumT * enumDatatype;

    /**
     * Struct data type.
     * If the attribute's type is MafOamSpiMoAttributeType_STRUCT then
     * this field contains a pointer to the struct data type.
     */
    MafOamSpiStructT * structDatatype;

    /**
     * The MO class that is referenced.
     * If the attribute's type is MafOamSpiMoAttributeType_REFERENCE then
     * this field contains a pointer to the referenced MO class.
     */
    MafOamSpiMocT * referencedMoc;

    /**
     * Marks if this attribute is a key (naming) attribute.
     */
    bool isKey;
    /**
     * An attribute must be provided at create of the MO. If mandatory,
     * then default must not be specified. If not mandatory,
     * then the attribute value does not have to be supplied at
     * creation of MO.
     */
    bool isMandatory;
    /**
     * Indicates that the attribute value survives a restart (that is,
     * a power cycle) of the network element. This is only applicable to
     * read-only attributes since all configuration attributes are
     * assumed to be persistent.
     */
    bool isPersistent;
    /**
     * If isReadOnly equals true, then the attribute is only for
     * read; the system provides updates internally.
     * A NETCONF get request will return read-only attributes.
     * If isReadOnly equals false, then the attribute is read-write
     * using the NBI. A NETCONF get request will not return
     * read-only attributes.
     */
    bool isReadOnly;
    /**
     * Indicates that the attribute contains a list of values. The
     * property conveys the exact range. If the lower bound is 0, a
     * value does not have to be supplied at create. However, deleting
     * an attribute once created is not supported.
     * Note: If the multiplicity lower bound is 1 (or higher), then
     * the mandatory property equals true. If the multiplicity lower
     * bound is 0, then the mandatory property equals false.
     */
    MafOamSpiMultiplicityRangeT multiplicity;

    /**
     * Indicates that the values of the attribute are in a specific order,
     * that is, same sets of values are different
     * if the order of the values is not the same.
     *
     * Note: This property is not supported since it is currently
     * not part of the mp.dtd definition.
     */
    bool isOrdered;

    /**
     * Indicates that the values are unique (within all MOs of
     * the same EcimMoClass).
     */
    bool isUnique;

    /**
     * Value for this attribute that is set automatically at
     * creation of the MO.
     * If the value is a multivalue then space is the separator
     * between the subvalues, If a subvalue contains space then each
     * space must be preceded by a '\'. If a subvalue contains '\'
     * then each '\' must be preceeded by a '\'.
     *  */
    char * defaultValue;

    /**
     * String with the unit used for the attribute type. The
     * unit is a standard measure of a quantity.
     */
    char * unit;

    /**
     * Reference to the next attribute of this MO, or null
     * if no more exists.
     */
    MafOamSpiMoAttributeT * next;
};

/**
 * Model information about an MO action.
 */
struct MafOamSpiMoAction {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MO class of this attribute.
     */
    MafOamSpiMocT * moc;

    /**
     * Return type.
     */
    MafOamSpiDatatypeContainerT returnType;

    /**
     * Reference to the first parameter that belongs to this MO action.
     */
    MafOamSpiParameterT * parameters;

    /**
     * Reference to the next action of this MO, or null if no more exists.
     */
    MafOamSpiMoActionT * next;

    /**
     * The domain extension, or null if not exist.
     */
    MafOamSpiDomainExtensionT* domainExtension;
};

/**
 * Model information about an action parameter. All parameters
 * have direction in, that is they cannot be used for output
 * values.
 */
struct MafOamSpiParameter {
    /**
     * General properties for this modeling element.
     */
    MafOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the action of this parameter.
     */
    MafOamSpiMoActionT * moAction;

    /**
     * Parameter type.
     */
    MafOamSpiDatatypeContainerT parameterType;

    /**
     * Reference to the next action of this MO, or null if no more exists.
     */
    MafOamSpiParameterT * next;
};

/**
 * Defines the Model Repository bootstrap functions. Once
 * an MOM or an MO class has been looked up, other modeling
 * elements are referenced within.
 */
typedef struct MafOamSpiModelRepository_1 {
    /**
     * The base interface.
     */
    MafMgmtSpiInterface_1T base;

    /**
     * Returns all MOMs that have been registered with the model repository.
     *
     * @param[out] result The first in a list of MOMs.
     *
     * @return MafOk if at least one MOM was found, otherwise MafNotExist.
     */
    MafReturnT (*getMoms)(const MafOamSpiMomT ** result);

    /**
     * Looks up an MO class.
     *
     * @param[in] momName Name of the MOM.
     *
     * @param[in] momVersion Version of the MOM.
     * If it is NULL, then the latest version is considered.
     *
     * @param[in] mocName MO class name.
     *
     * @param[out] result Returned MO class.
     *
     * @return MafOk if an MO class was found, otherwise MafNotExist.
     */
    MafReturnT (*getMoc)(const char * momName, const char * momVersion,
                         const char * mocName, MafOamSpiMocT ** result);

    /**
     * Retrieves the root MO class of the top MOM.
     *
     * @param[out] result The reference to the root MO class or
     * NULL if the model contains errors.
     *
     * @return MafOk if a MO class was found, or @n
     * MafNotActive if Model Repository service is not started, or @n
     * MafInvalidArgument if result is NULL.
     */
    MafReturnT (*getTreeRoot)(const MafOamSpiMocT **result);

} MafOamSpiModelRepository_1T;

#endif

