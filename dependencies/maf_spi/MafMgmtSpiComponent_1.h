#ifndef MafMgmtSpiComponent_1_h
#define MafMgmtSpiComponent_1_h
#include <MafMgmtSpiInterface_1.h>
/**
 *
 * @file MafMgmtSpiComponent_1.h
 * @ingroup MafMgmtSpi
 *
 * Component interface.
 * MAF uses this interface to retrieve information about
 * which interfaces this MAF component provides and their dependencies.
 * MAF also uses this interface to start and stop the MAF component's services.
 */

/**
 * Reason for calling the component start or stop functions.
 * This allows the component to handle resources in different ways
 * depending on the stop or start reason. The component may choose to ignore this information.
 */
typedef enum MafStateChangeReason {
    /**
     * <p>MAF changes to active state due to a normal start.
     * This reason is provided only to the start function.</p>
     */
    MafActivating = 1,
    /**
     * <p>MAF deactivates due to a normal terminate or deactivation.
     * This reason is provided only to the stop function.</p>
     */
    MafDeactivating = 2,
    /**
     * <p>MAF changes either to active or unactive state due to a failover.
     * This reason is given only if MAF is running in a high availability configuration.
     * It is provided to both the component start and stop functions.
     * However, it is unlikely that the stop function will be called during a failover.</p>
     */
    MafFailingOver = 3,
    /**
     * <p>MAF changes either to active or unactive state due to a switchover.
     * This reason is given only if MAF is running in a high availability configuration.
     * It is provided to both the component start and stop functions.</p>
     */
    MafSwitchingOver = 4
} MafStateChangeReasonT;

/**
 * Interface that must be implemented for each component.
 *
 * Information and callbacks used by MAF when a component providing a service is started or stopped.
 */
typedef struct MafMgmtSpiComponent_1 {
    /**
     * The base "class" information for this MafMgmtSpiComponent interface.
     *
     * @see MafMgmtSpiInterface_1.h for example
     */
    MafMgmtSpiInterface_1T base;
    /**
     * A null terminated array of interfaces that this component supplies.
     * It is null if no interfaces exist.
     */
    MafMgmtSpiInterface_1T ** interfaceArray;
    /**
     * A null terminated array of interface identities which this component uses,
     * that is, an array of the MafMgmtSpiInterface_1 struct with the values set so that each
     * instance identifies an interface that this component potentially can get from the MAF
     * Interface Portal.
     * It is null if no dependencies exist.
     * A special case is when the component does not specify a component name (setting the componentName to 0),
     * which will cause the component to be dependent on all the loaded implementations of the specified interface.
     */
    MafMgmtSpiInterface_1T ** dependencyArray;
    /**
     * This callback function is called when the component is ordered to start providing service.
     * If the component must use interfaces provided by the other components
     * they are not be retrieved from the MAF Interface Portal
     * before entering this function.
     *
     * @return Return code of type MafReturnT
     */
    MafReturnT (*start)(MafStateChangeReasonT reason);
    /**
     * This callback function is called when the component is ordered to stop providing service.
     * It must forget the references to the
     * other component interfaces that it has used, except for MafMgmtSpiInterfacePortal
     * and MafMgmtSpiInterfacePortalAccessor, which both can be kept.
     *
     * Example for possible reasons for calling stop:@n
     * &nbsp; 1) The component will be terminated.@n
     * &nbsp; 2) A component that this component is dependent on will be upgraded.@n
     * &nbsp; 3) The MAF process will be terminated, the next call would then terminate on each library.@n
     * &nbsp; 4) The MAF process will enter standby mode.@n
     *
     * @return Return code of type MafReturnT
     */
    MafReturnT (*stop)(MafStateChangeReasonT reason);
} MafMgmtSpiComponent_1T;

#endif
