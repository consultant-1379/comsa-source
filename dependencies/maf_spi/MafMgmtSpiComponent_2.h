#ifndef MafMgmtSpiComponent_2_h
#define MafMgmtSpiComponent_2_h
#include <MafMgmtSpiInterface_1.h>

#include <MafMgmtSpiComponent_1.h>

#ifndef __cpluplus
#include <stdbool.h>
#endif

/**
 *
 * @file MafMgmtSpiComponent_2.h
 * @ingroup MafMgmtSpi
 *
 * Component interface.
 * MAF uses this interface to retrieve information about
 * which interfaces this MAF component provides and their dependencies.
 * MAF also uses this interface to start and stop the MAF component's services.
 */

/**
 * Interface that must be implemented for each component.
 *
 * Information and callbacks used by MAF when a component providing a service is started or stopped.
 */
typedef struct MafMgmtSpiComponent_2 {
    /**
     * The base "class" information for this MafMgmtSpiComponent interface.
     *
     * @see MafMgmtSpiInterface_1.h for example
     */
    MafMgmtSpiInterface_1T base;
    /**
     * A null terminated array of interfaces that this component supplies.
     * If no interfaces exist, the array pointer pointed to by interfaceArray should be NULL (i.e. *interfaceArray == 0).
     */
    MafMgmtSpiInterface_1T ** interfaceArray;
    /**
     * A null terminated array of interface identities which this component uses,
     * that is, an array of the MafMgmtSpiInterface_1 struct with the values set so that each
     * instance identifies an interface that this component potentially can get from the MAF
     * Interface Portal.
     * If no dependencies exist, the array pointer pointed to by dependencyArray should be NULL (i.e. *interfaceArray == 0).
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
    /**
     * OptionalDependencyArray has the same semantics as dependencyArray,
     * except that if the dependency is not found in the system, Maf will be allowed to start.
     * The component with the optional interfaces will also be started. It has to check which optional
     * interfaces where actually started, this can be done using MafMgmtSpiInterfacePortal interface.
     * The component needs of course be able to provide services when its optional interfaces are
     * missing in the system.
     */
    MafMgmtSpiInterface_1T ** optionalDependencyArray;
    /**
     * Indicates if this component is passive, i.e. it does not use services it depends on spontaneously.
     * It only use services when serving requests from other services.
     * This indication is needed when Maf calculates the start order of the components.
     */
    bool passive;
} MafMgmtSpiComponent_2T;

#endif
