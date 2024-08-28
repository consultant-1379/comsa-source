#ifndef ComOamSpiModelRepository_1_h
#define ComOamSpiModelRepository_1_h

#include <ComMgmtSpiInterface_1.h>
#include <ComMgmtSpiInterfacePortal_1.h>
#include <ComMgmtSpiInterfacePortalAccessor.h>

#include <stdint.h>
#include <stdbool.h>

/**
 * Model Repository interface.
 *
 * @file ComOamSpiModelRepository_1.h
 *
 * This interface defines the Model Repository SPI. It follows
 * the Ericsson Common Information Model (ECIM) Domain Specific
 * Language (DSL).
 *
 * The Model Repository service identity is found in file
 * ComOamSpiServices.h.
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
typedef const struct ComOamSpiMom ComOamSpiMomT;
typedef const struct ComOamSpiMoc ComOamSpiMocT;
typedef const struct ComOamSpiMoAttribute ComOamSpiMoAttributeT;
typedef const struct ComOamSpiMoAction ComOamSpiMoActionT;
typedef const struct ComOamSpiParameter ComOamSpiParameterT;
typedef const struct ComOamSpiContainment ComOamSpiContainmentT;
typedef const struct ComOamSpiStruct ComOamSpiStructT;
typedef struct ComOamSpiValueRange ComOamSpiValueRangeT;
typedef struct ComOamSpiStructMember ComOamSpiStructMemberT;
typedef struct ComOamSpiEnumLiteral ComOamSpiEnumLiteralT;

/**
 * Defines the list of the valid data types.
 *
 * @deprecated replaced by ComOamSpiDatatype
 */
typedef enum ComOamSpiMoAttributeType {
    /**
     * An 8-bit integer.
     */
    ComOamSpiMoAttributeType_INT8 = 1,
    /**
     * A 16-bit integer.
     */
    ComOamSpiMoAttributeType_INT16 = 2,
    /**
     * A 32-bit integer.
     */
    ComOamSpiMoAttributeType_INT32 = 3,
    /**
     * A 64-bit integer.
     */
    ComOamSpiMoAttributeType_INT64 = 4,
    /**
     * An 8-bit unsigned integer.
     */
    ComOamSpiMoAttributeType_UINT8 = 5,
    /**
     * A 16-bit unsigned integer.
     */
    ComOamSpiMoAttributeType_UINT16 = 6,
    /**
     * A 32-bit unsigned integer.
     */
    ComOamSpiMoAttributeType_UINT32 = 7,
    /**
     * A 64-bit unsigned integer.
     */
    ComOamSpiMoAttributeType_UINT64 = 8,
    /**
     * A string value.
     */
    ComOamSpiMoAttributeType_STRING = 9,
    /**
     * A boolean.
     */
    ComOamSpiMoAttributeType_BOOL = 10,
    /**
     * A reference to another Managed Object (MO) class.
     */
    ComOamSpiMoAttributeType_REFERENCE = 11,
    /**
     * An enumeration.
     */
    ComOamSpiMoAttributeType_ENUM = 12,
    /**
     * A derived data type.
     */
    ComOamSpiMoAttributeType_DERIVED = 13,
    /**
     * A struct or aggregated data type.
     */
    ComOamSpiMoAttributeType_STRUCT = 14,
    /**
     * A void data type.
     */
    ComOamSpiMoAttributeType_VOID = 15
} ComOamSpiMoAttributeTypeT;

/**
 * Defines the list of the valid data types.
 */
typedef enum ComOamSpiDatatype {
    /**
     * An 8-bit integer.
     */
    ComOamSpiDatatype_INT8 = 1,
    /**
     * A 16-bit integer.
     */
    ComOamSpiDatatype_INT16 = 2,
    /**
     * A 32-bit integer.
     */
    ComOamSpiDatatype_INT32 = 3,
    /**
     * A 64-bit integer.
     */
    ComOamSpiDatatype_INT64 = 4,
    /**
     * An 8-bit unsigned integer.
     */
    ComOamSpiDatatype_UINT8 = 5,
    /**
     * A 16-bit unsigned integer.
     */
    ComOamSpiDatatype_UINT16 = 6,
    /**
     * A 32-bit unsigned integer.
     */
    ComOamSpiDatatype_UINT32 = 7,
    /**
     * A 64-bit unsigned integer.
     */
    ComOamSpiDatatype_UINT64 = 8,
    /**
     * A string value.
     */
    ComOamSpiDatatype_STRING = 9,
    /**
     * A boolean.
     */
    ComOamSpiDatatype_BOOL = 10,
    /**
     * A reference to another Managed Object class.
     */
    ComOamSpiDatatype_REFERENCE = 11,
    /**
     * An enumeration.
     */
    ComOamSpiDatatype_ENUM = 12,
    /**
     * A derived data type.
     */
    ComOamSpiDatatype_DERIVED = 13,
    /**
     * A struct or aggregated data type.
     */
    ComOamSpiDatatype_STRUCT = 14,
    /**
     * A void data type.
     */
    ComOamSpiDatatype_VOID = 15
} ComOamSpiDatatypeT;

/**
 * Describes the different status properties (current, deprecated,
 * and obsolete) that the modeling elements can be assigned.
 */
typedef enum ComOamSpiStatus {
    /**
     * The definition is current and valid.
     */
    ComOamSpiStatus_CURRENT = 1,
    /**
     * The definition is deprecated, but it permits new or continued
     * implementation in order to foster interoperability with older
     * or existing implementations.
     */
    ComOamSpiStatus_DEPRECATED = 2,
    /**
     * The definition is obsolete. It is not recommended to implement.
     * It can be removed if previously implemented.
     */
    ComOamSpiStatus_OBSOLETE = 3
} ComOamSpiStatusT;

/**
 * Type declaration for the extension container.
 */
typedef struct ComOamSpiExtension ComOamSpiExtensionT;

/**
 * The container for the extension.
 */
struct ComOamSpiExtension {
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
    ComOamSpiExtensionT* next;
};

/**
 * The container for the domain extension.
 */
typedef struct ComOamSpiDomainExtension {
    /**
     * The identifier of the domain extension.
     */
    const char* domain;
    /**
     * A pointer to the first extension, or null if no extensions.
     */
    ComOamSpiExtensionT* extensions;
} ComOamSpiDomainExtensionT;

/**
 * A container for general properties (for example, name and description).
 */
typedef struct ComOamSpiGeneralProperties {
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
    ComOamSpiStatusT status;
    /**
     * The hidden property can be used to hide information from the
     * northbound interfaces and the CPI. All modeling elements with
     * the same value for this <em>hidden</em> attribute are considered
     * to form a group and are treated the same with respect to
     * being visible or not in a northbound interface.
     */
    char * hidden;
} ComOamSpiGeneralPropertiesT;

/**
 * Describes the Managed Object Model.
 */
struct ComOamSpiMom {

    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

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
    ComOamSpiMocT * rootMoc;
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
    ComOamSpiMomT *next;
};

/**
 * Contains a range of values used by attribute multiplicities and
 * containment cardinalities.
 */
typedef struct ComOamSpiMultiplicityRange {
    /**
     * Minimum or lower bound value.
     */
    int16_t min;
    /**
     * Maximum or upper bound value.
     */
    int16_t max;
} ComOamSpiMultiplicityRangeT;


/**
 * Contains a range of values used by numeric restricted data types.
 */
struct ComOamSpiValueRange {
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
    ComOamSpiValueRangeT * next;
};


/**
 * Model information about an MO class.
 */
struct ComOamSpiMoc {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    ComOamSpiMomT * mom;

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
     * how to validate an OCL expression in COM.
     */
    char * constraint;

    /**
     * Reference to the first MO attribute that belongs to this
     * MO class.
     */
    ComOamSpiMoAttributeT * moAttribute;

    /**
     * Reference to the first MO action that belongs to this
     * MO class.
     */
    ComOamSpiMoActionT * moAction;

    /**
     * Reference to the first child containment relationship,
     * or null if this MO class is a leaf.
     */
    ComOamSpiContainmentT * childContainment;

    /**
     * Reference to the first parent containment relationship,
     * or null if this is the root MO class of the top MOM.
     */
    ComOamSpiContainmentT * parentContainment;

};

/**
 * Model information about a containment relationship, that is
 * a parent-child relationship between two MO classes.
 */
struct ComOamSpiContainment {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    ComOamSpiMomT * mom;

    /**
     * Parent MO class of this containment relationship.
     */
    ComOamSpiMocT * parentMoc;

    /**
     * Child MO class of this containment relationship.
     */
    ComOamSpiMocT * childMoc;

    /**
     * Range of child MOs that can be contributed under the parent MO.
     */
    ComOamSpiMultiplicityRangeT cardinality;

    /**
     * Indicates if this containment relationship and the corresponding
     * child MO are automatically created by the NE.
     * Note: This property is currently not supported.
     */
    bool isSystemCreated;

    /**
     * Reference to the next containment relationship in the list that
     * has the same parent MO class, null if no more relation exists.
     */
    ComOamSpiContainmentT * nextContainmentSameParentMoc;

    /**
     * Reference to the next containment relationship in the list that
     * has the same child MO class, null if no more relation exists.
     */
    ComOamSpiContainmentT * nextContainmentSameChildMoc;
};

/**
 * Describes a derived data type, that is a string or a numeric data type
 * that has been named and restricted.
 */
typedef const struct ComOamSpiDerivedDatatype {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    ComOamSpiMomT * mom;

    /**
     * Restricted data type.
     */
    ComOamSpiMoAttributeTypeT type;

    /**
     * If type equals ComOamSpiMoAttributeType_STRING this property
     * may be set with a pattern according to POSIX BASIC Regular Expression.
     */
    char * stringRestrictionPattern;

    /**
     * If type equals ComOamSpiMoAttributeType_STRING this property
     * may be set with a length range for the string value.
     */
    ComOamSpiValueRangeT stringRestrictionLength;

    /**
     * If type is numeric this property defines the allowed ranges.
     */
    ComOamSpiValueRangeT * numericRestrictionRange;
} ComOamSpiDerivedDatatypeT;

/**
 * Describes an enumeration literal.
 */
struct ComOamSpiEnumLiteral {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Integer value of the enumeration literal.
     */
    int16_t value;

    /**
     * Reference to the next literal of this enumeration,
     * or null if there are no more literals.
     */
    ComOamSpiEnumLiteralT * next;

};

/**
 * Model information about an enumeration.
 */
typedef const struct ComOamSpiEnum {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    ComOamSpiMomT * mom;

    /**
     * The enumeration literals.
     */
    ComOamSpiEnumLiteralT *literals;
} ComOamSpiEnumT;

/**
 * Container for the datatype and its related parameters.
 */
typedef struct ComOamSpiDatatypeContainer {
    /**
     * Data type of this element which can be an attribute,
     * an action parameter, an action result or a struct member.
     */
    ComOamSpiDatatypeT type;

    /**
     * Derived data type.
     * If the element is ComOamSpiDatatype_DERIVED then
     * this field contains a pointer to the derived data type.
     */
    ComOamSpiDerivedDatatypeT * derivedDatatype;

    /**
     * Enumeration data type.
     * If the element is ComOamSpiDatatype_ENUM then
     * this field contains a pointer to the enumeration data type.
     */
    ComOamSpiEnumT * enumDatatype;

    /**
     * Struct data type.
     * If the element is a ComOamSpiDatatype_STRUCT then
     * this field contains a pointer to the struct data type.
     */
    ComOamSpiStructT * structDatatype;

    /**
     * The MO class that is referenced.
     * If the element is ComOamSpiDatatype_REFERENCE then
     * this field contains a pointer to the referenced MO class.
     */
    ComOamSpiMocT * referencedMoc;

    /**
     * Value that is set automatically at the creation of the element.
     * If the value is a multivalue then space is the separator
     * between the subvalues, If a subvalue contains space then each
     * space must be preceded by a '\'. If a subvalue contains '\'
     * then each '\' must be preceded by a '\'.
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
    ComOamSpiMultiplicityRangeT multiplicity;

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
} ComOamSpiDatatypeContainerT;

/**
 * Describes a struct member for the struct modeling element.
 */
struct ComOamSpiStructMember {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Member type.
     */
    ComOamSpiDatatypeContainerT memberType;

    /**
     * Reference to the next struct member of this struct modeling element,
     * or null if there are no more members.
     */
    ComOamSpiStructMemberT * next;
};

/**
 * Model information about a struct.
 */
struct ComOamSpiStruct {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MOM that this modeling element belongs to.
     */
    ComOamSpiMomT * mom;

    /**
     * Reference to the first struct member modeling element
     */
    ComOamSpiStructMemberT * members;
};


/**
 * Model information about an MO attribute.
 */
struct ComOamSpiMoAttribute {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MO class of this attribute.
     */
    ComOamSpiMocT * moc;

    /**
     * Data type of this attribute.
     */
    ComOamSpiMoAttributeTypeT type;

    /**
     * Derived data type.
     * If the attribute's type is ComOamSpiMoAttributeType_DERIVED then
     * this field contains a pointer to the derived data type.
     */
    ComOamSpiDerivedDatatypeT * derivedDatatype;

    /**
     * Enumeration data type.
     * If the attribute's type is ComOamSpiMoAttributeType_ENUM then
     * this field contains a pointer to the enumeration data type.
     */
    ComOamSpiEnumT * enumDatatype;

    /**
     * Struct data type.
     * If the attribute's type is ComOamSpiMoAttributeType_STRUCT then
     * this field contains a pointer to the struct data type.
     */
    ComOamSpiStructT * structDatatype;

    /**
     * The MO class that is referenced.
     * If the attribute's type is ComOamSpiMoAttributeType_REFERENCE then
     * this field contains a pointer to the referenced MO class.
     */
    ComOamSpiMocT * referencedMoc;

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
    ComOamSpiMultiplicityRangeT multiplicity;

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
    ComOamSpiMoAttributeT * next;
};

/**
 * Model information about an MO action.
 */
struct ComOamSpiMoAction {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the MO class of this attribute.
     */
    ComOamSpiMocT * moc;

    /**
     * Return type.
     */
    ComOamSpiDatatypeContainerT returnType;

    /**
     * Reference to the first parameter that belongs to this MO action.
     */
    ComOamSpiParameterT * parameters;

    /**
     * Reference to the next action of this MO, or null if no more exists.
     */
    ComOamSpiMoActionT * next;

    /**
     * The domain extension, or null if not exist.
     */
    ComOamSpiDomainExtensionT* domainExtension;
};

/**
 * Model information about an action parameter. All parameters
 * have direction in, that is they cannot be used for output
 * values.
 */
struct ComOamSpiParameter {
    /**
     * General properties for this modeling element.
     */
    ComOamSpiGeneralPropertiesT generalProperties;

    /**
     * Reference to the action of this parameter.
     */
    ComOamSpiMoActionT * moAction;

    /**
     * Parameter type.
     */
    ComOamSpiDatatypeContainerT parameterType;

    /**
     * Reference to the next action of this MO, or null if no more exists.
     */
    ComOamSpiParameterT * next;
};

/**
 * Defines the Model Repository bootstrap functions. Once
 * an MOM or an MO class has been looked up, other modeling
 * elements are referenced within.
 */
typedef struct ComOamSpiModelRepository_1 {
    /**
     * The base interface.
     */
    ComMgmtSpiInterface_1T base;

    /**
     * Returns all MOMs that have been registered with the model repository.
     *
     * @param[out] result The first in a list of MOMs.
     *
     * @return ComOk if at least one MOM was found, otherwise ComNotExist.
     */
    ComReturnT (*getMoms)(const ComOamSpiMomT ** result);

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
     * @return ComOk if an MO class was found, otherwise ComNotExist.
     */
    ComReturnT (*getMoc)(const char * momName, const char * momVersion,
                         const char * mocName, ComOamSpiMocT ** result);

    /**
     * Retrieves the root MO class of the top MOM.
     *
     * @param[out] result The reference to the root MO class or
     * NULL if the model contains errors.
     *
     * @return ComOk if a MO class was found, or @n
     * ComNotActive if Model Repository service is not started, or @n
     * ComInvalidArgument if result is NULL.
     */
    ComReturnT (*getTreeRoot)(const ComOamSpiMocT **result);

} ComOamSpiModelRepository_1T;

#endif

