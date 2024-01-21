/*
 * CSIFilter.h - shared data for filters
 *
 *  Jun. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemniz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CSIFILTER_H_
#define CSIFILTER_H_


/**
 * Limits of string lengths:
 * The CSIGUI and filters exchange data (such as parameters) via strings.
 * The values below form the maximum lengths of each string. Every filter has to obey these limits to avoid memory
 * issues.
 * */

#define CSI_FILTER_NAME_STLEN 100                       ///Maximum string length for the name of the filter
#define CSI_FILTER_NAME_DESCRIPTION_STLEN 500           ///Maximum string length for the description of the filter
#define CSI_FILTER_NAME_PARMETER_STLEN 100              ///Maximum string length for exchanging parameter names and values
#define CSI_FILTER_NAME_PARMETER_LIST_STLEN 500         ///Maximum string length for the list of parameter values




#endif /* CSIFILTER_H_ */
