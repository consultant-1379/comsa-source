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

#ifndef SAPM_H
#define	SAPM_H

#ifdef	__cplusplus
extern "C" {
#endif

	/**
	 * Enum for collection methods.
	 *
	 * The enum SaPmfCollectionMethodT defines all allowed collection methods,
	 * both integer and real.
	 */
	typedef enum {
		/** Collection method not specified or unknown */
		SA_PMF_COLLECTION_METHOD_NONE = 0,
		/** Collection method Cumulative Counter, only integer by definition */
		SA_PMF_COLLECTION_METHOD_CC						= 1,
		/** Integer gauge */
		SA_PMF_COLLECTION_METHOD_GAUGE				= 2,
		/** Integer DER */
		SA_PMF_COLLECTION_METHOD_DER					= 3,
		/** Integer SI */
		SA_PMF_COLLECTION_METHOD_SI						= 4,
		/** Float SI. */
		SA_PMF_COLLECTION_METHOD_SI_FLOAT			= 5,
		/** Float DER */
		SA_PMF_COLLECTION_METHOD_DER_FLOAT		= 6,
		/** Float gauge */
		SA_PMF_COLLECTION_METHOD_GAUGE_FLOAT	= 7
	} SaPmfCollectionMethodT;

	typedef enum {
		SA_PMF_AGGREGATION_NONE					= 0,
		SA_PMF_AGGREGATION_SUM					= 1,
		SA_PMF_AGGREGATION_AVG					= 2,
		SA_PMF_AGGREGATION_MIN					= 3,
		SA_PMF_AGGREGATION_MAX					= 4,
		SA_PMF_AGGREGATION_LAST_UPDATE	= 5
	} SaPmfAggregationT;

	typedef enum {
		/** Measurement job */
		SA_PMF_JOB_TYPE_MEASUREMENT	    = 1,
		/** Threshold job */
		SA_PMF_JOB_TYPE_THRESHOLD		= 2,
		/** Resource monitor Job  */
		SA_PMF_JOB_TYPE_RESOURCEMONITOR = 3,
	} SaPmfJobTypeT;

	typedef enum pmsv_job_priority {
		SA_PMF_JOB_PRIO_LOW			= 1,
		SA_PMF_JOB_PRIO_MEDIUM	= 2,
		SA_PMF_JOB_PRIO_HIGH		= 3
	} SaPmfJobPriorityT;

	typedef enum pmsv_job_state {
		SA_PMF_JOB_STATE_ACTIVE		= 1,
		SA_PMF_JOB_STATE_STOPPED	= 2,
		SA_PMF_JOB_STATE_FAILED		= 3
	} SaPmfJobStateT;

	typedef enum pmsv_threshold_direction {
		SA_PMF_THRESHOLD_DIRECTION_NONE = 0,
		SA_PMF_THRESHOLD_DIRECTION_INCREASING = 1,
		SA_PMF_THRESHOLD_DIRECTION_DECREASING = 2
	} SaPmfThresholdDirectionT;

    typedef enum pmsv_thresholdRateOfVariation {
		SA_PMF_THRESHOLD_RATE_VARIATION_PER_SECOND = 1,
		SA_PMF_THRESHOLD_RATE_VARIATION_PER_GP = 2
    } SaPmfThresholdRateOfVariationT;

	typedef enum {
		SA_PMF_ADMIN_START_JOB	= 1,
		SA_PMF_ADMIN_STOP_JOB		= 2
	} SaPmfAdminOperationIdT;

	typedef enum {
		SA_PMF_JOB_TIME_PERIOD_10_SEC		= 1,
		SA_PMF_JOB_TIME_PERIOD_30_SEC		= 2,
		SA_PMF_JOB_TIME_PERIOD_1_MIN		= 3,
		SA_PMF_JOB_TIME_PERIOD_5_MIN		= 4,
		SA_PMF_JOB_TIME_PERIOD_15_MIN		= 5,
		SA_PMF_JOB_TIME_PERIOD_30_MIN		= 6,
		SA_PMF_JOB_TIME_PERIOD_1_HOUR		= 7,
		SA_PMF_JOB_TIME_PERIOD_12_HOUR	= 8,
		SA_PMF_JOB_TIME_PERIOD_24_HOUR	= 9
	}SaPmfJobTimePeriodT;

	typedef enum {
		SA_PMF_REPORT_CHANGED_ONLY	= 1,
		SA_PMF_REPORT_MISSING_MOIDS	= 2
	} SaPmfReportContentT;

#ifdef	__cplusplus
}
#endif

#endif	/* SAPM_H */

