/* CSIData.h - A data structure for RSSI
 *
 *      Nov. 2020, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CSIDATA_H_
#define CSIDATA_H_
#include <inttypes.h>
#include <time.h>
#ifdef __cplusplus
  extern "C" {
#endif

  /**
   * \brief A structure representing the CSI data belonging to one frame.
   *
   * A structure of type CSI data is exchanged between WirelessEyeStudio and each filter plugin.
   * The filter plugin may not only read, but also modify the data. It will be read back after executing the filter function.
   *
   */
struct CSIData{
  struct tm timeStamp;                          ///Time when the WiFi Frame was acquired
/**
 * MAC address of the sending device.
 * A filter might be called for displaying and for data export, which might imply different
 *  values To allow for a distinction, byte 7 in the MAC is 0 for display and 1 for export, while bytes 1-6 contain the actual MAC
 */
  uint8_t senderMAC[7];
                                                ///It is recommended to regard all 7 bytes as a MAC address.
  uint16_t seqNr;                               ///Sequence number
  uint16_t streamNr;                            ///Spatial stream number
  uint16_t chanSpec;                            ///Channel specification
  uint16_t chipVersion;                         ///Chip version
  double RSSI;                                  ///RSSI
  uint8_t frame_control;                        ///Frame control field
  uint32_t nSubCarriers;                        ///number of subcarriers
  uint32_t nSubCarriers_orig;                   ///number of subcarriers as received from Nexmon.
  double amplitude[512];                        ///CSI amplitudes. Only the indices 0...nSubcarriers-1 are actually used
  double phase[512];                            ///CSI phases. Only the indices 0...nSubcarriers-1 are actually used.

};


#ifdef __cplusplus
  }
#endif /* CSIDATA_H_ */

#endif
