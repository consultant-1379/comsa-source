/* ***************************************************************************
* Copyright (C) 2011 by Ericsson AB
* S - 125 26  STOCKHOLM
* SWEDEN, tel int + 46 10 719 0000
*
* The copyright to the computer program herein is the property of
* Ericsson AB. The program may be used and/or copied only with the
* written permission from Ericsson AB, or in accordance with the terms
* and conditions stipulated in the agreement/contract under which the
* program has been supplied.
*
* All rights reserved.
*
* File: PmConsumer.cxx
*
* Author: ekorleo 2011-09-01
*
* Reviewed: efaiami 2012-04-14
* Modified: xtronle 2013-12-25 Support PDF counter
* Modified: xjonbuc 2015-04-16  MR40135 Always generate PM results.
* Modified: xnikvap 2015-08-02  MR42277 Remove old interfaces exposed to COM & CoreMW that are no longer supported for SLES12
*
************************************************************************** */
#include <stdlib.h>
#include <vector>
#include <map>
#include <algorithm>
#include <cassert>
#include <cstdio>

#include "PmConsumer.hxx"
#include "PmtSaTrace.hxx"
#include "PmRunnable.hxx"

/**
* @file PmConsumer.cxx
*
* @ingroup PmtSa
*
* @brief Provides interfaces towards OpenSAF PM-data
*
* Enter a suitable description of the file here, preferably on
* more than one line!
*/

using namespace std;

namespace PmtSa
{

	/**
	* @ingroup PmtSa
	*
	* This function reads a single value and populates the out variable result. If a value was successfully read, true is returned.
	*
	* @param[out] result If the functions returns <em>true</em>, the out parameter result will be set, else the value will be undefined.
	* @param[in]  iteratorHandle The iterator handle that will be used to read new values.
	* @param[out] suspect This out parameter will be set to <em>true</em> if an unforeseen error was encountered while using the iterator (that is, any error).
	*
	* @returns true iff a value was successfully read.
	* @returns false if a value could not be read.
	*/

	inline bool readValuePdf(ComOamSpiPmAggregatedValue_2T& result, SaPmCIteratorHandleT_2& iteratorHandle2, bool& suspect)
        {
                ENTER_PMTSA();
                PMTSA_DEBUG("readValuePdf() called");
                SaPmCIteratorInfoT_2 iteratorInfo_2;
                SaPmCAggregatedValuesT dataValues;
                SaAisErrorT code = PmConsumerInterface::instance()->iteratorNext_2(iteratorHandle2, &iteratorInfo_2, &dataValues);
                if (code != SA_AIS_OK)
                {
                        // this will set a suspect flag on the whole job. This is an abnormal case
                        PMTSA_WARN("Unexpected return-code from iteratorNext_2 interface on value = %d", code);
                        suspect = true;
                        LEAVE_PMTSA();
                        return false;
                }
                else if (iteratorInfo_2.objectType != PMF_JOB_ITERATOR_OBJECT_VALUE)
                {
                        // this is the normal end condition for reading values, we have probably read a 'closing' PMSV_JOB_ITERATOR_OBJECT_LDN
                        LEAVE_PMTSA();
                        return false;
                }

                /////get multi-value
                result.valueSize = iteratorInfo_2.valueInfo.multiplicity;    //number element of array

                SaPmAggregatedValueT *iteratorValues = dataValues;

                if (result.valueSize == 0)
                {
                        result.valueType      = ComOamSpiPmValueType_2_NIL;
                }
                else
                {
                        if (iteratorInfo_2.valueInfo.isFloat)
                        {
                                result.valueType      = ComOamSpiPmValueType_2_FLOATARR;
                                result.value.floatArr = new double[result.valueSize];      // this memory is freed in freeData()
                        }
                        else
			{
				result.valueType    = ComOamSpiPmValueType_2_INTARR;
                                result.value.intArr = new int64_t[result.valueSize];      // this memory is freed in freeData()
                        }
                }


                for (unsigned int i = 0; i < result.valueSize ; i++)
                {
                        if (iteratorInfo_2.valueInfo.isFloat)
                        {
                                result.value.floatArr[i] = iteratorValues[i].floatVal;
                        }
                        else
                        {
                                result.value.intArr[i] = iteratorValues[i].intVal;
                        }
                }
                result.measType = iteratorInfo_2.nodeName;
                result.isSuspect = iteratorInfo_2.valueInfo.isSuspect;

                LEAVE_PMTSA();
                return true;
        }


	inline bool readValuePdf2(ComOamSpiPmAggregatedValue_2T& result, SaPmCIteratorHandleT_2& iteratorHandle2, bool& suspect)
	{
		ENTER_PMTSA();
		PMTSA_DEBUG("readValuePdf2() called");
		SaPmCIteratorInfoT_2 iteratorInfo_2;
		SaPmCAggregatedValuesT dataValues;
		SaAisErrorT code = PmConsumerInterface::instance()->iteratorNext_2(iteratorHandle2, &iteratorInfo_2, &dataValues);
		if (code != SA_AIS_OK)
		{
			// this will set a suspect flag on the whole job. This is an abnormal case
			PMTSA_WARN("Unexpected return-code from iteratorNext_2 interface on value = %d", code);
			suspect = true;
			LEAVE_PMTSA();
			return false;
		}
		else if (iteratorInfo_2.objectType != PMF_JOB_ITERATOR_OBJECT_VALUE)
		{
			// this is the normal end condition for reading values, we have probably read a 'closing' PMSV_JOB_ITERATOR_OBJECT_LDN
			LEAVE_PMTSA();
			return false;
		}

		/////get multi-value
		result.valueSize = iteratorInfo_2.valueInfo.multiplicity;    //number element of array

		SaPmAggregatedValueT *iteratorValues = dataValues;

		if (result.valueSize == 0)
		{
			result.valueType      = ComOamSpiPmValueType_2_NIL;
		}
		else
		{
			if (iteratorInfo_2.valueInfo.isFloat)
			{
				result.valueType      = ComOamSpiPmValueType_2_FLOATARR;
				result.value.floatArr = new double[result.valueSize];      // this memory is freed in freeData()
			}
			else
			{
				result.valueType    = ComOamSpiPmValueType_2_INTARR;
				result.value.intArr = new int64_t[result.valueSize];      // this memory is freed in freeData()
			}
		}


		for (unsigned int i = 0; i < result.valueSize ; i++)
		{
			if (iteratorInfo_2.valueInfo.isFloat)
			{
				result.value.floatArr[i] = iteratorValues[i].floatVal;
			}
			else
			{
				result.value.intArr[i] = iteratorValues[i].intVal;
			}
		}
		result.measType = iteratorInfo_2.nodeName;
		result.isSuspect = iteratorInfo_2.valueInfo.isSuspect;

		LEAVE_PMTSA();
		return true;
	}


	inline bool readInstancePdf(ComOamSpiPmInstance_2T& result, SaPmCIteratorHandleT_2& iteratorHandle2, bool& suspect)
        {
                ENTER_PMTSA();
                SaPmCIteratorInfoT_2 iteratorInfo_2;
                SaPmCAggregatedValuesT dataValues;
                PMTSA_DEBUG("readInstancePdf() called");

                SaAisErrorT code = PmConsumerInterface::instance()->iteratorNext_2(iteratorHandle2, &iteratorInfo_2, &dataValues);
                if (code != SA_AIS_OK)
                {
                        // this will set a suspect flag on the whole job. This is an abnormal case
                        PMTSA_WARN("Unexpected return-code from iteratorNext_2 interface on instance, code = %d", code);
                        suspect = true;
                        LEAVE_PMTSA();
                        return false;
                }

                if(iteratorInfo_2.objectType != PMF_JOB_ITERATOR_OBJECT_LDN)
                {
                        LEAVE_PMTSA();
                        return false;
                }

                result.measObjLDN = iteratorInfo_2.nodeName;

                // The tmpList is static, so that the same buffer can be used as long
                // as it is big enough. a small performance hack
                static vector<ComOamSpiPmAggregatedValue_2T> tmpList;
                tmpList.clear(); // neded because tmpList is static

                for (ComOamSpiPmAggregatedValue_2T val; readValuePdf(val, iteratorHandle2, suspect);)
                {
                        tmpList.push_back(val);
                }

                result.values = new ComOamSpiPmAggregatedValue_2T[tmpList.size()]; // this memory is freed in freeData()
                copy (tmpList.begin(), tmpList.end(), result.values);
                result.size = tmpList.size();

		LEAVE_PMTSA();
                return true;
        }

	inline bool readInstancePdf2(ComOamSpiPmInstance_2T& result, SaPmCIteratorHandleT_2& iteratorHandle2, bool& suspect, int gpIndex)
	{
		ENTER_PMTSA();
		SaPmCIteratorInfoT_2 iteratorInfo_2;
		SaPmCAggregatedValuesT dataValues;
		PMTSA_DEBUG("readInstancePdf2() called");

		SaAisErrorT code = PmConsumerInterface::instance()->iteratorNext_2(iteratorHandle2, &iteratorInfo_2, &dataValues);
		if (code != SA_AIS_OK)
		{
			// this will set a suspect flag on the whole job. This is an abnormal case
			PMTSA_WARN("Unexpected return-code from iteratorNext_2 interface on instance, code = %d", code);
			suspect = true;
			LEAVE_PMTSA();
			return false;
		}

		if(iteratorInfo_2.objectType != PMF_JOB_ITERATOR_OBJECT_LDN)
		{
			LEAVE_PMTSA();
			return false;
		}

		result.measObjLDN = iteratorInfo_2.nodeName;

		// The tmpList is static, so that the same buffer can be used as long
		// as it is big enough. a small performance hack
		// Handled multiple parallel gp request by creating individual structure for each Gp
		static vector<ComOamSpiPmAggregatedValue_2T> tmpList[7];
		tmpList[gpIndex].clear(); // neded because tmpList is static

		for (ComOamSpiPmAggregatedValue_2T val; readValuePdf2(val, iteratorHandle2, suspect);)
		{
			tmpList[gpIndex].push_back(val);
		}

		result.values = new ComOamSpiPmAggregatedValue_2T[tmpList[gpIndex].size()]; // this memory is freed in freeData()
		copy (tmpList[gpIndex].begin(), tmpList[gpIndex].end(), result.values);
		result.size = tmpList[gpIndex].size();

		LEAVE_PMTSA();
		return true;
	}


	/**
	* @ingroup PmtSa
	*
	* -Replace-this-with-an-accurate-description-
	*
	* @param[in,out] 	result The result
	* @param[in,out]   iteratorHandle  The iterator handle
	* @param[in,out]   suspect If the result is valid or not
	*
	* @return 	bool    true on success, false otherwise
	*
	*/

	inline bool readMoClassPdf(ComOamSpiPmMeasObjClass_2T& result, SaPmCIteratorHandleT_2& iteratorHandle2, bool& suspect)
        {
                ENTER_PMTSA();

                SaPmCIteratorInfoT_2 iteratorInfo_2;
                SaPmCAggregatedValuesT dataValues;

                PMTSA_DEBUG("readMoClassPdf() called");
                SaAisErrorT code = PmConsumerInterface::instance()->iteratorNext_2(iteratorHandle2, &iteratorInfo_2, &dataValues);
                if (code != SA_AIS_OK)
                {
                        // SA_AIS_ERR_NOT_EXIST is expected when the last MoClass is read. Thus
                        // set no suspect flag if that error condition is reported on MoClass "level"
                        if (code != SA_AIS_ERR_NOT_EXIST)
                        {
                                PMTSA_LOG("Unexpected return-code from iteratorNext_2 interface on moClass");
                                suspect = true; // this will set a suspect flag on the whole job. This is an abnormal case
                        }
                        LEAVE_PMTSA();
                        return false;
                }

                assert(iteratorInfo_2.objectType == PMF_JOB_ITERATOR_OBJECT_MOCLASS);
                result.measObjClass = iteratorInfo_2.nodeName;

                // The tmpList is static, so that the same buffer can be used as long
                // as it is big enough. a small performance hack
                static vector<ComOamSpiPmInstance_2T> tmpList;
                tmpList.clear(); // Needed because tmpList is static
                for (ComOamSpiPmInstance_2T val; readInstancePdf(val, iteratorHandle2, suspect);)
                {
                        tmpList.push_back(val);
                }
                result.instances = new ComOamSpiPmInstance_2T[tmpList.size()]; // this memory is freed in freeData()
                copy (tmpList.begin(), tmpList.end(), result.instances);
                result.size = tmpList.size();
                LEAVE_PMTSA();
		return true;
        }

	inline bool readMoClassPdf2(ComOamSpiPmMeasObjClass_2T& result, SaPmCIteratorHandleT_2& iteratorHandle2, bool& suspect, int gpIndex)
	{
		ENTER_PMTSA();

		SaPmCIteratorInfoT_2 iteratorInfo_2;
		SaPmCAggregatedValuesT dataValues;

		PMTSA_DEBUG("readMoClassPdf2() called");
		SaAisErrorT code = PmConsumerInterface::instance()->iteratorNext_2(iteratorHandle2, &iteratorInfo_2, &dataValues);
		if (code != SA_AIS_OK)
		{
			// SA_AIS_ERR_NOT_EXIST is expected when the last MoClass is read. Thus
			// set no suspect flag if that error condition is reported on MoClass "level"
			if (code != SA_AIS_ERR_NOT_EXIST)
			{
				PMTSA_LOG("Unexpected return-code from iteratorNext_2 interface on moClass");
				suspect = true; // this will set a suspect flag on the whole job. This is an abnormal case
			}
			LEAVE_PMTSA();
			return false;
		}

		assert(iteratorInfo_2.objectType == PMF_JOB_ITERATOR_OBJECT_MOCLASS);
		result.measObjClass = iteratorInfo_2.nodeName;

		// The tmpList is static, so that the same buffer can be used as long
		// as it is big enough. a small performance hack
		// Handled multiple parallel gp request by creating individual structure for each Gp
		static vector<ComOamSpiPmInstance_2T> tmpList[7];
		tmpList[gpIndex].clear(); // Needed because tmpList is static
		for (ComOamSpiPmInstance_2T val; readInstancePdf2(val, iteratorHandle2, suspect, gpIndex);)
		{
			tmpList[gpIndex].push_back(val);
		}
		result.instances = new ComOamSpiPmInstance_2T[tmpList[gpIndex].size()]; // this memory is freed in freeData()
		copy (tmpList[gpIndex].begin(), tmpList[gpIndex].end(), result.instances);
		result.size = tmpList[gpIndex].size();
		LEAVE_PMTSA();
		return true;
	}


	// This map will be used to see which handle was used to copy data into a omOamSpiPmGpData_1T.
	// The map is used so that the iterator can be finalized when the data is to be freed.
	map<ComOamSpiPmGpData_2T*, SaPmCIteratorHandleT> iteratorMap;
	map<ComOamSpiPmGpData_2T*, SaPmCIteratorHandleT> iteratorMap2[7];


	/**
	* @ingroup PmtSa
	*
	* Given a callback, this function will allocate and populate a ComOamSpiPmGpData_2T. The
	* returned data is freed with the function <code>freeData</code>.
	*
	* @param[in] info The callback information given by Core MW.
	*
	* @returns All data from a GP (Granularity Period). Each variable might be <em>suspect</em>
	*          marked, even the whole GP might be suspect marked. When the whole GP is suspect marked,
	*          that means that we of unknown reasons failed to iterate through all the data. However,
	*          although some of the data might be missing, the data that is delivered should be reliable
	*          (if not individually suspect marked).
	*/
	ComOamSpiPmGpData_2T* getData(SaPmCCallbackInfoT* info)
        {
                ENTER_PMTSA();
                PMTSA_DEBUG("getData() called");

                ComOamSpiPmGpData_2T* result = new ComOamSpiPmGpData_2T(); // this memory is freed in freeData()

                assert(info != NULL);
                //result->jobId = info->jobname;
                //result->gpStartTimestampInNanoSeconds = info->gpStartTimestamp;
                //result->gpEndTimestampInNanoSeconds = info->gpEndTimestamp;
                result->measObjClasses = NULL;
                result->size = 0;
                result->isSuspect = false;

                SaPmCIteratorHandleT_2 iteratorHandle2; // for PDF support

                if (PmConsumerInterface::instance()->iteratorInitialize_2(info->jobHandler, &iteratorHandle2) != SA_AIS_OK)
                {
                        PMTSA_WARN("Failed initializing iterator (handle2) for job-id %llu\n", info->jobHandler);
                        result->isSuspect = true;
                        LEAVE_PMTSA();
                        return result;
                }

                // The tmpList is static, so that the same buffer can be used as long
                // as it is big enough. a small performance hack
                static vector<ComOamSpiPmMeasObjClass_2T> tmpList;
                tmpList.clear(); // neded because tmpList is static

		iteratorMap.insert(make_pair(result, iteratorHandle2));
                for (ComOamSpiPmMeasObjClass_2T res; readMoClassPdf(res, iteratorHandle2, result->isSuspect);)
                {
                        tmpList.push_back(res);
                }

                result->measObjClasses = new ComOamSpiPmMeasObjClass_2T[tmpList.size()]; // this memory is freed in freeData()
                copy (tmpList.begin(), tmpList.end(), result->measObjClasses);
                result->size = tmpList.size();

                LEAVE_PMTSA();
                return result;
        }


	/**
        * @ingroup PmtSa
        *
        * Given a callback, this function will allocate and populate a ComOamSpiPmGpData_2T. The
        * returned data is freed with the function <code>freeData</code>.
        *
        * @param[in] info The callback information given by Core MW.
	* @param[in] gpNameIndex Index to choose the respective Gp structure inorder to handle parallel request
        *
        * @returns All data from a GP (Granularity Period). Each variable might be <em>suspect</em>
        *          marked, even the whole GP might be suspect marked. When the whole GP is suspect marked,
        *          that means that we of unknown reasons failed to iterate through all the data. However,
        *          although some of the data might be missing, the data that is delivered should be reliable
        *          (if not individually suspect marked).
        */
	ComOamSpiPmGpData_2T* getData2(SaPmCCallbackInfoT* info, int gpNameIndex)
	{
		ENTER_PMTSA();
		PMTSA_DEBUG("getData2() called");

		ComOamSpiPmGpData_2T* result = new ComOamSpiPmGpData_2T(); // this memory is freed in freeData()

		assert(info != NULL);
		//result->jobId = info->jobname;
		//result->gpStartTimestampInNanoSeconds = info->gpStartTimestamp;
		//result->gpEndTimestampInNanoSeconds = info->gpEndTimestamp;
		result->measObjClasses = NULL;
		result->size = 0;
		result->isSuspect = false;

		SaPmCIteratorHandleT_2 iteratorHandle2;	// for PDF support

		if (PmConsumerInterface::instance()->iteratorInitialize_2(info->jobHandler, &iteratorHandle2) != SA_AIS_OK)
		{
			PMTSA_WARN("Failed initializing iterator (handle2) for job-id %llu", info->jobHandler);
			result->isSuspect = true;
			LEAVE_PMTSA();
			return result;
		}

		// The tmpList is static, so that the same buffer can be used as long
		// as it is big enough. a small performance hack
		// Handled multiple parallel gp request by creating individual structure for each Gp
		static vector<ComOamSpiPmMeasObjClass_2T> tmpList[7];
		tmpList[gpNameIndex].clear(); // neded because tmpList is static

		iteratorMap2[gpNameIndex].insert(make_pair(result, iteratorHandle2));
		for (ComOamSpiPmMeasObjClass_2T res; readMoClassPdf2(res, iteratorHandle2, result->isSuspect, gpNameIndex);)
		{
			tmpList[gpNameIndex].push_back(res);
		}

		result->measObjClasses = new ComOamSpiPmMeasObjClass_2T[tmpList[gpNameIndex].size()]; // this memory is freed in freeData()
		copy (tmpList[gpNameIndex].begin(), tmpList[gpNameIndex].end(), result->measObjClasses);
		result->size = tmpList[gpNameIndex].size();

		LEAVE_PMTSA();
		return result;
	}


	/**
	* @ingroup PmtSa
	*
	* The function freeData will recursively free all the data that is hold by the
	* <em>data</em> parameter. The iterator is finalized in this function and not in
	* <code>getData</code>, this is because the strings in shared memory are never
	* copied, only the pointer to them. If we would have finalized the iterator handle
	* in get data, the strings would have been reclaimed.
	*
	* @param[in]   data    Pointer to data that is to be freed.
	*/
	void freeData(ComOamSpiPmGpData_2T* data)
        {
                ENTER_PMTSA();
                PMTSA_DEBUG("freeData() called");
                // find the iterator handle that was used to copy data to the ComOamSpiPmGpData_2T structure and finalize it if it exists.
                map<ComOamSpiPmGpData_2*, SaPmCIteratorHandleT>::iterator i = iteratorMap.find(data);

                if (i != iteratorMap.end()) // if iteratorInitialize failed, no iterator has been added to the map.
                {
                        PmConsumerInterface::instance()->iteratorFinalize_2(i->second);
                        iteratorMap.erase(i);
                }

                for (unsigned mo=0; mo<data->size; ++mo)
                {
                        for (unsigned instance=0; instance<data->measObjClasses[mo].size; ++instance)
                        {
                                for (unsigned val=0; val<data->measObjClasses[mo].instances[instance].size; ++val)
                                {
                                        if (data->measObjClasses[mo].instances[instance].values[val].valueType == ComOamSpiPmValueType_2_FLOATARR)
                                        {
                                                delete [] data->measObjClasses[mo].instances[instance].values[val].value.floatArr;
                                        }
                                        if (data->measObjClasses[mo].instances[instance].values[val].valueType == ComOamSpiPmValueType_2_INTARR)
                                        {
                                                delete [] data->measObjClasses[mo].instances[instance].values[val].value.intArr;
                                        }
                                }
                                delete [] data->measObjClasses[mo].instances[instance].values;
                        }
                        delete [] data->measObjClasses[mo].instances;
                }

                delete [] data->measObjClasses;
                delete data;
                LEAVE_PMTSA();
	}

	/**
        * @ingroup PmtSa
        *
        * The function freeData will recursively free all the data that is hold by the
        * <em>data</em> parameter. The iterator is finalized in this function and not in
        * <code>getData</code>, this is because the strings in shared memory are never
        * copied, only the pointer to them. If we would have finalized the iterator handle
        * in get data, the strings would have been reclaimed.
        *
        * @param[in]   data    Pointer to data that is to be freed.
	* @param[in] gpNameIndex Index to choose the respective Gp structure inorder to handle parallel request
        */
	void freeData2(ComOamSpiPmGpData_2T* data,int gpNameIndex)
	{
		ENTER_PMTSA();
		PMTSA_DEBUG("freeData2() called");
		// find the iterator handle that was used to copy data to the ComOamSpiPmGpData_2T structure and finalize it if it exists.
		map<ComOamSpiPmGpData_2*, SaPmCIteratorHandleT>::iterator i = iteratorMap2[gpNameIndex].find(data);

		if (i != iteratorMap2[gpNameIndex].end()) // if iteratorInitialize failed, no iterator has been added to the map.
		{
			PmConsumerInterface::instance()->iteratorFinalize_2(i->second);
			iteratorMap2[gpNameIndex].erase(i);
		}

		for (unsigned mo=0; mo<data->size; ++mo)
		{
			for (unsigned instance=0; instance<data->measObjClasses[mo].size; ++instance)
			{
				for (unsigned val=0; val<data->measObjClasses[mo].instances[instance].size; ++val)
				{
					if (data->measObjClasses[mo].instances[instance].values[val].valueType == ComOamSpiPmValueType_2_FLOATARR)
					{
						delete [] data->measObjClasses[mo].instances[instance].values[val].value.floatArr;
					}
					if (data->measObjClasses[mo].instances[instance].values[val].valueType == ComOamSpiPmValueType_2_INTARR)
					{
						delete [] data->measObjClasses[mo].instances[instance].values[val].value.intArr;
					}
				}
				delete [] data->measObjClasses[mo].instances[instance].values;
			}
			delete [] data->measObjClasses[mo].instances;
		}

		delete [] data->measObjClasses;
		delete data;
		LEAVE_PMTSA();
	}
}

