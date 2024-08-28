#ifndef ComMgmtSpiComponent_1_h
#define ComMgmtSpiComponent_1_h
#include <ComMgmtSpiInterface_1.h>
/**
 *
 * @file ComMgmtSpiComponent_1.h
 * Component interface.
 * COM uses this interface to retrieve information about
 * which interfaces this COM component provides and their dependencies.
 * COM also uses this interface to start and stop the COM component's services.
 */

/**
 * Reason for calling the component start or stop functions.
 * This allows the component to handle resources in different ways
 * depending on the stop or start reason. The component may choose to ignore this information.
 */
typedef enum ComStateChangeReason {
    /**
     * <p>COM changes to active state due to a normal start.
     * This reason is provided only to the start function.</p>
     */
    ComActivating = 1,
    /**
     * <p>COM deactivates due to a normal terminate or deactivation.
     * This reason is provided only to the stop function.</p>
     */
    ComDeactivating = 2,
    /**
     * <p>COM changes either to active or inactive state due to a failover.
     * This reason is given only if COM is running in a high availability configuration.
     * It is provided to both the component start and stop functions.
     * However, it is unlikely that the stop function will be called during a failover.</p>
     */
    ComFailingOver = 3,
    /**
     * <p>COM changes either to active or inactive state due to a switchover.
     * This reason is given only if COM is running in a high availability configuration.
     * It is provided to both the component start and stop functions.</p>
     */
    ComSwitchingOver = 4
} ComStateChangeReasonT;

/**
 * Interface that must be implemented for each component.
 *
 * Information and callbacks used by COM when a component providing a service is started or stopped.
 */
typedef struct ComMgmtSpiComponent_1 {
    /**
     * The base "class" information for this ComMgmtSpiComponent interface.
     *
     * @see ComMgmtSpiInterface_1.h for example
     */
    ComMgmtSpiInterface_1T base;
    /**
     * A null terminated array of interfaces that this component supplies.
     * It is null if no interfaces exist.
     */
    ComMgmtSpiInterface_1T ** interfaceArray;
    /**
     * A null terminated array of interface identities which this component uses,
     * that is, an array of the ComMgmtSpiInterface_1 struct with the values set so that each
     * instance identifies an interface that this component potentially can get from the COM
     * Interface Portal.
     * It is null if no dependencies exist.
     * A special case is when the component does not specify a component name (setting the componentName to 0),
     * which will cause the component to be dependent on all the loaded implementations of the specified interface.
     */
    ComMgmtSpiInterface_1T ** dependencyArray;
    /**
     * This callback function is called when the component is ordered to start providing service.
     * If the component must use interfaces provided by the other components
     * they are not be retrieved from the COM Interface Portal
     * before entering this function.
     *
     * @return Return code of type ComReturnT
     */
    ComReturnT (*start)(ComStateChangeReasonT reason);
    /**
     * This callback function is called when the component is ordered to stop providing service.
     * It must forget the references to the
     * other component interfaces that it has used, except for ComMgmtSpiInterfacePortal
     * and ComMgmtSpiInterfacePortalAccessor, which both can be kept.
     *
     * Example for possible reasons for calling stop:@n
     * &nbsp; 1) The component will be terminated.@n
     * &nbsp; 2) A component that this component is dependent on will be upgraded.@n
     * &nbsp; 3) The COM process will be terminated, the next call would then terminate on each library.@n
     * &nbsp; 4) The COM process will enter standby mode.@n
     *
     * @return Return code of type ComReturnT
     */
    ComReturnT (*stop)(ComStateChangeReasonT reason);
} ComMgmtSpiComponent_1T;

#endif
