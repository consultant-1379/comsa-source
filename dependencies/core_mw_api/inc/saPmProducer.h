/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

/**
 * @cond OSAF_PM_API
 * @mainpage Performance Management Producer API
 * @endcond
 *
 * @section public_sec Public API's
 *
 * @subsection pm_producer The PM Producer API
 * The PM Producer API is the main API for producing PM measurements.
 * The API is documented in
 * @ref saPmProducer.h
 *
 */


/**
 * @file saPmProducer.h
 * @ingroup PmProducer
 * @brief This file contains the PM Producer API
 * @example pmsv_producer_example.c
 * This example shows how to use the PM Producer API
 */

#ifndef SAPMPRODUCER_H
#define	SAPMPRODUCER_H

#include "saAis.h"

#ifdef	__cplusplus
extern "C" {
#endif

	/**
	 * A handle used when accessing PM Producer API.
	 */
	typedef void *SaPmHandleT;
	/**
	 * A handler for a Cumulative Counter measurement type
	 */
	typedef SaUint32T SaPmCcT;
	/**
	 * A handle for a Status Inspection measurement type
	 */
	typedef SaUint32T SaPmSiT;
	/**
	 * A handle for a Gauge measurement type
	 */
	typedef SaUint32T SaPmGaugeT;
	/**
	 * A handle for a Discrete Event Register measurement type
	 */
	typedef SaUint32T SaPmDerT;
	/**
	 * A handle that could be any measurement type
	 */
	typedef SaUint32T SaPmAnyT;

	typedef enum {
		PMA_MT_IS_NOT_MEASURED = 0,
		PMA_MT_IS_MEASURED = 1
	} SaPmfMTIsMeasuredState;

	/**
	 * This is sent as argument to callback.
	 *
	 * Note that the values of this structure are only valid until the
	 * callback returns
	 */
	typedef struct {
		/** The handler return by the #saPmPInitialize_3
		 */
		SaPmHandleT clientHdlr;

		/**
		 * A handle for measurement type
		 */
		SaPmAnyT measRef;

		SaPmfMTIsMeasuredState isMeasuredState;

	} SaPmPCallbackInfoT;

	/**
	 * Typedef for the callback function
	 */
	typedef SaAisErrorT(*SaPmPCallbackT) (SaPmPCallbackInfoT * info);

	/**
	 * This contains information about the callback used when subscribing
	 */
	typedef struct {
		/**The supplied callback that should be called once for each MT*/
		SaPmPCallbackT cbPtr;
	} SaPmPSubscribeInfoT;


	/****************************************************************************/
	/********************** PM API function declarations ************************/
	/****************************************************************************/
	/**
         * @cond OSAF_PM_API
	 * @ingroup PmProducer
         * @deprecated Since A.01.02
	 * @brief Initializes the PM Producer Agent
	 * @param pmHandle : [out] handle for PM Agent
	 * @param version : [in:out] version information
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_VERSION Unsupported version
	 * @retval SA_AIS_ERR_BAD_OPERATION PM Producer Agent already initialized
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 *
	 * This function initializes the PM Producer for the invoking process.
	 * This function must be invoked prior to the invocation of any other
	 * PM Producer functions. The handle pointed to by pmHandle is returned
	 * by the PM Agent as the reference to this association between the
	 * process and the PM Producer. The process uses this handle in
	 * subsequent communication with the PM Producer.
	 *
	 * @b Example:
	 * @code
	 * version.releaseCode = 'A';
	 * version.majorVersion = 1;
	 * version.minorVersion = 1;
	 *
	 * saPmPInitialize(&pmHandle, &version);
	 * @endcode
	 */
	extern SaAisErrorT saPmPInitialize(SaPmHandleT *pmHandle, SaVersionT *version);
	/* @endcond */

	/**
         * @cond OSAF_PM_API
	 * @ingroup PmProducer
         * @deprecated Since A.03.01
	 * @brief Initializes the PM Producer Agent
	 * @param pmHandle : [out] handle for PM Agent
	 * @param version : [in:out] version information
	 * @param subscribeInfo : [in] struct initialized  with the subscribeInfo
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_VERSION Unsupported version
	 * @retval SA_AIS_ERR_INVALID_PARAM parameters are invalid
	 * @retval SA_AIS_ERR_BAD_OPERATION PM Producer Agent already initialized
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 *
	 * This function initializes the PM Producer for the invoking process.
	 * This function must be invoked prior to the invocation of any other
	 * PM Producer functions. The handle pointed to by pmHandle is returned
	 * by the PM Agent as the reference to this association between the
	 * process and the PM Producer. The process uses this handle in
	 * subsequent communication with the PM Producer.
	 *
	 * The important members of the @b subscribeInfo are
	 * @li cbPtr, the callback that should be called when there are some state changed event
	 * with any measurement types of current Producer.
	 *
	 * @b Example:
	 * @code
	 * version.releaseCode = 'A';
	 * version.majorVersion = 1;
	 * version.minorVersion = 2;
	 *
	 * SaPmPSubscribeInfoT subinfo;
	 * subinfo.cbPtr = producerCb;
	 *
	 * saPmPInitialize_2(&pmHandle, &version, &subinfo);
	 * @endcode
	 */
	extern SaAisErrorT saPmPInitialize_2(SaPmHandleT *pmHandle, SaVersionT *version, SaPmPSubscribeInfoT *subscribeInfo);
	/* @endcond */

	/**
	 * @ingroup PmProducer
	 * @brief Initializes the PM Producer Agent
	 * @param pmHandle : [out] handle for PM Agent
	 * @param version : [in:out] version information
	 * @param subscribeInfo : [in] struct initialized  with the subscribeInfo
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_VERSION Unsupported version
	 * @retval SA_AIS_ERR_INVALID_PARAM parameters are invalid
	 * @retval SA_AIS_ERR_BAD_OPERATION PM Producer Agent already initialized
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 *
	 * This function initializes the PM Producer for the invoking process.
	 * This function must be invoked prior to the invocation of any other
	 * PM Producer functions. The handle pointed to by pmHandle is returned
	 * by the PM Agent as the reference to this association between the
	 * process and the PM Producer. The process uses this handle in
	 * subsequent communication with the PM Producer.
	 *
	 * The important members of the @b subscribeInfo are
	 * @li cbPtr, the callback that should be called when there are some state changed event
	 * with any measurement types of current Producer.
	 *
	 * The subscribeInfo parameter is allowed to be NULL if measurement type
	 * callback is not required.
	 *
	 * @b Example:
	 * @code
	 * version.releaseCode = 'A';
	 * version.majorVersion = 3;
	 * version.minorVersion = 1;
	 *
	 * SaPmPSubscribeInfoT subinfo;
	 * subinfo.cbPtr = producerCb;
	 *
	 * saPmPInitialize_3(&pmHandle, &version, &subinfo);
	 * @endcode
	 */
	extern SaAisErrorT saPmPInitialize_3(SaPmHandleT *pmHandle, SaVersionT *version, SaPmPSubscribeInfoT *subscribeInfo);

	/**
	 * @ingroup PmProducer
	 * @brief Finalizes the PM Producer Agent
	 * @param pmHandle : [in] the handle returned by #saPmPInitialize_3
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE Handle does not refer to an open session
	 * @retval SA_AIS_ERR_BAD_OPERATION PM Producer Agent already finalized
	 * or have not been initialized yet.
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * The #saPmPFinalize function closes the association represented by
	 * the pmHandle parameter between the invoking process and the PM Producer.
	 * The process must have invoked #saPmPInitialize_3 before it invokes this function.
	 *
	 * @b Example:
	 * @code
	 * saPmPFinalize(pmHandle);
	 * @endcode
	 */
	extern SaAisErrorT saPmPFinalize(SaPmHandleT pmHandle);

	/**
	 * @ingroup PmProducer
	 * @brief Release a handle for a measurement type.
	 * @param pmHandle : [in] the handle returned by #saPmPInitialize_3
	 * @param measRef : [in] a handle for the measurement type
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 *
	 * This function is used to release the handle for a measurement type.
	 *
	 */
	extern SaAisErrorT saPmPRefRelease(SaPmHandleT pmHandle, SaPmAnyT measRef);

	/**
	 * @ingroup PmProducer
	 * @brief Check if the MT is currently involved in a Job
	 * @param measRef : [in] handle to a measurement type
	 * @param isMeasuredState : [out] measurement type is measured state
	 * @return SaAisErrorT:
	 * @retval SA_AIS_OK
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_INVALID_PARAM A parameter is invalid
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 */
	extern SaAisErrorT saPmPMtIsMeasured(SaPmAnyT measRef, SaPmfMTIsMeasuredState *isMeasuredState);

	/**
	 * @ingroup PmProducer
	 * @brief Get reference to a Cumulative Counter measurement type.
	 * @param pmHandle : [in] the handle returned by #saPmPInitialize_3
	 * @param measRef : [out] a handle for the Cumulative Counter
	 * @param moClass : [in] Name of Managed Object Class
	 * @param measType : [in] Name of measurement type
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_NOT_EXIST The named moClass - measType does not
	 * exist
	 * @retval SA_AIS_ERR_BAD_HANDLE The named moClass - measType exist as
	 * another collection method
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_TRY_AGAIN The PM Agent is not currently available
	 *
	 * @b Example:
	 * @code
	 * saPmPCumulativeCounterRefGet(pmHandle, &ccHandle, "SS7", "RecvdMessages");
	 * @endcode
	 */
	extern SaAisErrorT saPmPCumulativeCounterRefGet(SaPmHandleT pmHandle, SaPmCcT *measRef, SaStringT moClass, SaStringT measType);

	/**
	 * @ingroup PmProducer
	 * @brief Add value to a Cumulative Counter measurement type.
	 * @param measRef : [in] the handle returned by #saPmPCumulativeCounterRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param value : [in] the value used in the operation
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE measRef references a different collection method
	 * or handle has been released
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function adds to the counter value associated with the instance name for
	 * the Cumulative Counter.
	 * The supplied value is added to counter.
	 *
	 * If the instance doesn't exist it will be created
	 * by the operation and the counter will be set to an initial value,
	 * usually zero, before the supplied value is added.
	 *
	 * @b Example:
	 * @code
	 * saPmPCumulativeCounterRefGet(pmHandle, &ccHandle, "SS7", "RecvdMessages");
	 * saPmPCumulativeCounterInc(ccHandle, "IAM", 1); // Create and increase no. received IAM msgs by 1
	 * saPmPCumulativeCounterInc(ccHandle, "IAM", 5); // Increase no. received IAM msgs by 5
	 * saPmPCumulativeCounterInc(ccHandle, "REL", 1); // Create and increase no. received REL msgs by 1
	 * saPmPCumulativeCounterInc(ccHandle, "REL", 2); // Increase no. received REL msgs by 2
	 * @endcode
	 */
	extern SaAisErrorT saPmPCumulativeCounterInc(SaPmCcT measRef, SaStringT measInst, SaUint32T value);

	/**
	 * @ingroup PmProducer
	 * @brief Add multiple values to a Cumulative Counter measurement type.
	 * @param measRef : [in] the handle returned by #saPmPCumulativeCounterRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param values[] : [in] the values used in the operation.
	 * @param multiplicity : [in] the number of values used in this operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE measRef references a different collection method
	 * or handle has been released
	 * or multiplicity is different from the measurementType multiplicity.
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function adds to the counter multiple values associated with the instance name for
	 * the Cumulative Counter.
	 * The supplied values are added to counter.
	 *
	 * If the instance doesn't exist it will be created
	 * by the operation and the counter will be set to an initial values,
	 * usually zero, before the supplied values are added.
	 *
	 * @b Example:
	 * @code
	 * SaUInt32T uint_values[4] = {1,6,41,2};
	 * saPmPCumulativeCounterRefGet(pmHandle, &ccHandle, "SS7", "MultiValues");
	 * saPmPCumulativeCounterMultiInc(ccHandle, "CC", uint_values, 4);
	 * //measurement instance has 4 values: instance[0] += 1, instance[1] += 6, instance[2] += 41, instance[3] += 2
	 * @endcode
	 */
	extern SaAisErrorT saPmPCumulativeCounterMultiInc(SaPmCcT measRef, SaStringT measInst, SaUint32T values[], SaInt16T multiplicity);

	/* SI */
	/**
	 * @ingroup PmProducer
	 * @brief Get reference to a Status Inspection measurement type.
	 * @param pmHandle : [in] the handle returned by #saPmPInitialize_3
	 * @param measRef : [out] a handle for the measurement type.
	 * @param moClass : [in] Name of Managed Object Class
	 * @param measType : [in] Name of measurement type.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_NOT_EXIST The named moClass - measType does not
	 * exist
	 * @retval SA_AIS_ERR_BAD_HANDLE Bad handle or the named moClass -
	 * measType exist as another collection method
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_TRY_AGAIN The PM Agent is not currently available
	 *
	 * @b Example:
	 * @code
	 * saPmPStatusInspectionRefGet(pmHandle, &siHandle, "SS7", "Internalrate");
	 * @endcode
	 */
	extern SaAisErrorT saPmPStatusInspectionRefGet(SaPmHandleT pmHandle, SaPmSiT *measRef, SaStringT moClass, SaStringT measType);

	/**
	 * @ingroup PmProducer
	 * @brief Sets a value to a real Status Inspection measurement type.
	 * @param measRef : [in] the handle returned by #saPmPStatusInspectionRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param value : [in] the value used in the operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event Registration measurement
	 * type
	 * or handle has been released
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function sets the float value associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be a real Status Inspection
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * saPmPStatusInspectionRefGet(pmHandle, &siHandle, "SS7", "InternalFloat");
	 * saPmPStatusInspectionRealSet(siHandle, "Meas1", 150.5);  // Create and set inst. to 150.5
	 * saPmPStatusInspectionRealSet(siHandle, "Value2", 201.1); // Create and set inst. to 201.1
	 * saPmPStatusInspectionRealSet(siHandle, "Meas1", 167.8);  // Set inst. to 167.8
	 * saPmPStatusInspectionRealSet(siHandle, "Value2", 101.7); // Set inst. to 101.7
	 * @endcode
	 */
	extern SaAisErrorT saPmPStatusInspectionRealSet(SaPmSiT measRef, SaStringT measInst, SaFloatT value);

	/**
	 * @ingroup PmProducer
	 * @brief Sets a value to an integer Status Inspection measurement type.
	 * @param measRef : [in] the handle returned by #saPmPStatusInspectionRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param value : [in] the value used in the operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event
	 * Registration measurement type
	 * or handle has been released
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function sets the integer value associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be an integer Status Inspection
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * saPmPStatusInspectionRefGet(pmHandle, &siHandle, "SS7", "Internalrate");
	 * saPmPStatusInspectionIntegerSet(siHandle, "BER", 1500); // Create and set inst. to 1500
	 * saPmPStatusInspectionIntegerSet(siHandle, "CRC", 750);  // Create and set inst. to 750
	 * saPmPStatusInspectionIntegerSet(siHandle, "BER", 900);  // Set BER SI to 900
	 * saPmPStatusInspectionIntegerSet(siHandle, "CRC", 500);  // Set CRC SI to 500
	 * @endcode
	 */
	extern SaAisErrorT saPmPStatusInspectionIntegerSet(SaPmSiT measRef, SaStringT measInst, SaInt32T value);

	/**
	 * @ingroup PmProducer
	 * @brief Sets multiple values to an integer Status Inspection measurement type.
	 * @param measRef : [in] the handle returned by #saPmPStatusInspectionRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param values[] : [in] the values used in the operation.
	 * @param multiplicity : [in] the number of values used in this operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event
	 * Registration measurement type
	 * or handle has been released
	 * or multiplicity is different from the measurementType multiplicity.
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function sets the multiple integer values associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be an integer Status Inspection
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * SaInt32T int_values[4] = {1,6,-41,2}
	 * saPmPStatusInspectionRefGet(pmHandle, &siHandle, "SS7", "MultiValues");
	 * saPmPStatusInspectionMultiIntegerSet(siHandle, "IntegerSI", int_values, 4);
	 * // measurement instance has 4 values: instance[0] = 1, instance[1] = 6, instance[2] = -41, instance[3] = 2
	 * @endcode
	 */
	extern SaAisErrorT saPmPStatusInspectionMultiIntegerSet(SaPmSiT measRef, SaStringT measInst, SaInt32T values[], SaInt16T multiplicity);

	/* Gauge */
	/**
	 * @ingroup PmProducer
	 * @brief Get reference to a Gauge measurement type.
	 * @param pmHandle : [in] the handle returned by #saPmPInitialize_3
	 * @param measRef : [out] a handle for the measurement type.
	 * @param moClass : [in] Name of Managed Object Class
	 * @param measType : [in] Name of measurement type.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_NOT_EXIST the named moClass - measType does not
	 * exist
	 * @retval SA_AIS_ERR_BAD_HANDLE Bad handle or the named moClass -
	 * measType exist as another collection method
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_TRY_AGAIN The PM agent is not currently available
	 *
	 * @b Example:
	 * @code
	 * saPmPGaugeRefGet(pmHandle, &gaugeHandle, "SS7", "RecvdMessages");
	 * @endcode
	 */
	extern SaAisErrorT saPmPGaugeRefGet(SaPmHandleT pmHandle, SaPmGaugeT *measRef, SaStringT moClass, SaStringT measType);

	/**
	 * @ingroup PmProducer
	 * @brief Sets a value to a real Gauge measurement type.
	 * @param measRef : [in] the handle returned by #saPmPGaugeRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param value : [in] the value used in the operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event Registration measurement
	 * type
	 * or handle has been released
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function sets the float value associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be a real Gauge
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * saPmPGaugeRefGet(pmHandle, &gaugeHandle, "SS7", "Processor");
	 * saPmPGaugeRealSet(gaugeHandle, "Temp1", 28.1);  // Create and set inst. to 28.1
	 * saPmPGaugeRealSet(gaugeHandle, "Temp2", 30.1);  // Create and set inst. to 30.1
	 * saPmPGaugeRealSet(gaugeHandle, "Temp1", 28.5);  // Set inst. to 28.5
	 * saPmPGaugeRealSet(gaugeHandle, "Temp2", 31.1);  // Set inst. to 31.1
	 * @endcode
	 */
	extern SaAisErrorT saPmPGaugeRealSet(SaPmGaugeT measRef, SaStringT measInst, SaFloatT value);

	/**
	 * @ingroup PmProducer
	 * @brief Sets a value to an integer Gauge measurement type.
	 * @param measRef : [in] the handle returned by #saPmPGaugeRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param value : [in] the value used in the operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event Registration measurement
	 * type
	 * or handle has been released
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function sets the integet value associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be an integer Gauge
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * saPmPGaugeRefGet(pmHandle, &gaugeHandle, "SS7", "RecvdMessages");
	 * saPmPGaugeIntegerSet(gaugeHandle, "IAM", 200); // Create and set inst. to 200
	 * saPmPGaugeIntegerSet(gaugeHandle, "ACM", 350); // Create and set inst. to 350
	 * saPmPGaugeIntegerSet(gaugeHandle, "IAM", 150); // Set IAM's gauge to 150
	 * saPmPGaugeIntegerSet(gaugeHandle, "ACM", 100); // Set ACM's gauge to 100
	 * @endcode
	 */
	extern SaAisErrorT saPmPGaugeIntegerSet(SaPmGaugeT measRef, SaStringT measInst, SaInt32T value);

	/**
	 * @ingroup PmProducer
	 * @brief Sets multiple values to an integer Gauge measurement type.
	 * @param measRef : [in] the handle returned by #saPmPGaugeRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param values[] : [in] the values used in the operation.
	 * @param multiplicity : [in] the number of values used in this operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event Registration measurement
	 * type
	 * or handle has been released
	 * or multiplicity is different from the measurementType multiplicity.
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function sets the multiple integer values associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be an integer Gauge
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * SaInt32T int_values[4] = {1,6,-41,2};
	 * saPmPGaugeRefGet(pmHandle, &gaugeHandle, "SS7", "MultiValues");
	 * saPmPGaugeMultiIntegerSet(gaugeHandle, "IntegerGauge", int_values, 4);
	 * // measurement instance has 4 values: instance[0] = 1, instance[1] = 6, instance[2] = -41, instance[3] = 2
	 * @endcode
	 */
	extern SaAisErrorT saPmPGaugeMultiIntegerSet(SaPmGaugeT measRef, SaStringT measInst, SaInt32T values[], SaInt16T multiplicity);

	/* DER */
	/**
	 * @ingroup PmProducer
	 * @brief Get reference to a Discrete Event Registration measurement type.
	 * @param pmHandle : [in] the handle returned by #saPmPInitialize_3
	 * @param measRef : [out] a handle for the measurement type.
	 * @param moClass : [in] Name of Managed Object Class
	 * @param measType : [in] Name of measurement type.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_NOT_EXIST the named moClass - measType does not
	 * exist
	 * @retval SA_AIS_ERR_BAD_HANDLE Bad handle or the named moClass -
	 * measType exist as another collection method
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_TRY_AGAIN The PM agent is not currently available
	 *
	 * @b Example:
	 * @code
	 * saPmPDiscreteEventRegistrationRefGet(pmHandle, &derHandle, "SS7", "Events");
	 * @endcode
	 */
	extern SaAisErrorT saPmPDiscreteEventRegistrationRefGet(SaPmHandleT pmHandle, SaPmDerT *measRef, SaStringT moClass, SaStringT measType);

	/**
	 * @ingroup PmProducer
	 * @brief Sets a value to a real Discrete Event Registration measurement type.
	 * @param measRef : [in] the handle returned by #saPmPDiscreteEventRegistrationRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param value : [in] the value used in the operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event Registration measurement
	 * type
	 * or handle has been released
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function sets the float value associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be a real Discrete Event Registration
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * saPmPDiscreteEventRegistrationRefGet(pmHandle, &derHandle, "SS7", " Events");
	 * saPmPDiscreteEventRegistrationRealSet(derHandle, "Var1", 50.5); // Create and set inst. to 50.5
	 * saPmPDiscreteEventRegistrationRealSet(derHandle, "Var2", 60.7); // Set inst. to 60.7
	 * @endcode
	 */
	extern SaAisErrorT saPmPDiscreteEventRegistrationRealSet(SaPmDerT measRef, SaStringT measInst, SaFloatT value);

	/**
	 * @ingroup PmProducer
	 * @brief Sets a value to an integer Discrete Event Registration measurement type.
	 * @param measRef : [in] the handle returned by #saPmPDiscreteEventRegistrationRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param value : [in] the value used in the operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event Registration measurement
	 * type
	 * or handle has been released
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function sets the integer value associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be an integer Discrete Event Registration
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * saPmPDiscreteEventRegistrationRefGet(pmHandle, &derHandle, "SS7", " Events");
	 * saPmPDiscreteEventRegistrationIntegerSet(derHandle, "MSU_Sent", 100); // Create and set inst. to 100
	 * saPmPDiscreteEventRegistrationIntegerSet(derHandle, "MSU_Sent", 750); // Set inst. to 750

	 * @endcode
	 */
	extern SaAisErrorT saPmPDiscreteEventRegistrationIntegerSet(SaPmDerT measRef, SaStringT measInst, SaInt32T value);

	/**
	 * @ingroup PmProducer
	 * @brief Sets multiple values to an integer Discrete Event Registration measurement type.
	 * @param measRef : [in] the handle returned by #saPmPDiscreteEventRegistrationRefGet
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @param values[] : [in] the values used in the operation.
	 * @param multiplicity : [in] the number of values used in this operation.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE The handler was referring to a measurement
	 * type with different collection method or a real Discrete Event Registration measurement
	 * type
	 * or handle has been released
	 * or multiplicity is different from the measurementType multiplicity.
	 * This function sets the multiple integer values associated with the instance name for the Status
	 * Inspection measurement type referred to by the argument measRef.
	 * If the instance doesn't exist, it will be created.
	 *
	 * The measurement type must be an integer Discrete Event Registration
	 * measurement type, otherwise the operation will fail.
	 *
	 * @b Example:
	 * @code
	 * SaInt32T int_values[4] = {1,6,-41,2}
	 * saPmPDiscreteEventRegistrationRefGet(pmHandle, &derHandle, "SS7", "MultiValues");
	 * saPmPDiscreteEventRegistrationMultiIntegerSet(derHandle, "IntegerDER", int_values, 4);
	 * // measurement instance has 4 values: instance[0] = 1, instance[1] = 6, instance[2] = -41, instance[3] = 2
	 * @endcode
	 */
	extern SaAisErrorT saPmPDiscreteEventRegistrationMultiIntegerSet(SaPmDerT measRef, SaStringT measInst, SaInt32T values[], SaInt16T  multiplicity);

	/**
	 * @ingroup PmProducer
	 * @brief Indicate that the measurement data may not be fully reliable due to an external event.
	 * @param measRef : [in] the handle returned by *RefGet() functions
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_BAD_HANDLE Handle has been released
	 * @retval SA_AIS_ERR_INVALID_PARAM measRef is NULL, an empty string or
	 * too long.
	 * @retval SA_AIS_ERR_UNAVAILABLE The measurement type measRef refers to
	 * has been deleted
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function can be used by the producer of measurement data when there is a suspicion that
	 * measurement data does not include the full number of increments/updates as it would during
	 * normal operation.
	 *
	 * @b Example:
	 * @code
	 * saPmPCumulativeCounterRefGet(pmHandle, &ccHandle, "SS7", "RecvdMessages");
	 * saPmPInstanceSuspectSet(ccHandle, "IAM"); // Set suspect flag of "IAM" instance
	 * @endcode
	 */
	extern SaAisErrorT saPmPInstanceSuspectSet(SaPmAnyT measRef, SaStringT measInst);

	/**
	 * @ingroup PmProducer
	 * @brief Indicate that the measurement data may not be fully reliable due to an external event.
	 * @param pmHandle : [in] the handle returned by #saPmPInitialize_3
	 * @param measInst : [in] the name of the instance.
	 * Max length allowed is 2047 characters and min length is one character.
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 * This function can be used by the producer of measurement data when there is a suspicion that
	 * measurement data does not include the full number of increments/updates as it would during
	 * normal operation.
	 *
	 * @b Example:
	 * @code
	 * saPmPAllInstanceSuspectSet(pmHandle, "IAM"); // Set suspect flag of "IAM" instance
	 * @endcode
	 */
	extern SaAisErrorT saPmPAllInstanceSuspectSet(SaPmHandleT pmHandle, SaStringT measInst);

	/**
	 * @ingroup PmProducer
	 * @brief Indicate that the measurement data may not be fully reliable due to an external event.
	 * @param pmHandle : [in] the handle returned by #saPmPInitialize_3
	 * @return SaAisErrorT
	 * @retval SA_AIS_OK Operation succeeded
	 * @retval SA_AIS_ERR_NO_MEMORY System or process out of memory
	 * @retval SA_AIS_ERR_LIBRARY An unexpected problem occurred in the library
	 * (such as corruption)
	 *
	 *
	 * This function can be used by the producer of measurement data when there is a suspicion that
	 * measurement data does not include the full number of increments/updates as it would during
	 * normal operation.
	 *
	 * The scope of the suspicion is all measurements being referenced by this node.
	 *
	 * @b Example:
	 * @code
	 * saPmPSuspectSet(pmHandle); // Set suspect flag for all measurement types of PM Producer Agent
	 * @endcode
	 */
	extern SaAisErrorT saPmPSuspectSet(SaPmHandleT pmHandle);

	/****************************************************************************/
	/****************** End of PM API function declarations *********************/
	/****************************************************************************/
#ifdef	__cplusplus
}
#endif

#endif	/* SAPMPRODUCER_H */

