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
 * File: PmConsumer.hxx
 *
 * Author: ekorleo 2011-09-01
 *
 * Reviewed: efaiami 2012-04-14
 * Modified: xnikvap 2013-12-15 Converted to use COM PM SPI Ver.2
 *
 ************************************************************************** */
#ifndef PMCONSUMER_HXX_
#define PMCONSUMER_HXX_

#include "ComOamSpiPm_2.h"
#include "ComOamSpiPm_2_1.h"
#include "PmConsumerInterface.hxx"

/**
 * @file PmConsumer.hxx
 *
 * @ingroup PmtSa
 *
 * @brief Defines interfaces for accessing PM-data from the Osaf PM-services.
 *
 * This file defines the two interfaces that is needed to access all data
 * from the OpenSAF Performance Management services on a SAF HA system.
 */

namespace PmtSa {

/**
 * @ingroup PmtSa
 *
 * The method will fetch all available information connected to the
 * supplied job and return a pointer to that data.
 *
 * @param[in] info	A pointer to the structure provided in the callback from
 * 					the PM-consumer API
 *
 * @return ComOamSpiPmGpData_1T* Pointer to a memory-area that contains information
 * 								 about a finished job-gp.  Notice that the memory
 * 								 that is provided must be free'd up with the
 * 								 freeData() method once done.
 */
ComOamSpiPmGpData_2T* getData(SaPmCCallbackInfoT* info);

/**
 * @ingroup PmtSa
 *
 * The method will fetch all available information connected to the
 * supplied job and return a pointer to that data.
 *
 * @param[in] info      A pointer to the structure provided in the callback from
 *                                      the PM-consumer API
 *
 * @param[in] gpNameIndex Index to choose the respective Gp structure inorder to handle parallel request
 *
 * @return ComOamSpiPmGpData_1T* Pointer to a memory-area that contains information
 *                                                               about a finished job-gp.  Notice that the memory
 *                                                               that is provided must be free'd up with the
 *                                                               freeData() method once done.
 */
ComOamSpiPmGpData_2T* getData2(SaPmCCallbackInfoT* info, int gpNameIndex);

/**
 * @ingroup PmtSa
 *
 * The method returns the memory given from the getData() method.
 *
 * @param[in] data	A pointer to the given memory-area that should be disposed.
 *
 */
void freeData(ComOamSpiPmGpData_2T* data);


/**
 * @ingroup PmtSa
 *
 * The method returns the memory given from the getData() method.
 *
 * @param[in] data      A pointer to the given memory-area that should be disposed.
 *
 * @param[in] gpNameIndex Index to choose the respective Gp structure inorder to handle parallel request
 *
 */
void freeData2(ComOamSpiPmGpData_2T* data, int gpNameIndex);

} //namespace PmtSa

#endif /* PMCONSUMER_HXX_ */
