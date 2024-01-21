/**
 *  Jun. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 * AGCCompensation.c - Compensates the AGC of the 802.11 SoC
 * Method:
 * see Equation (9) in
 * Zhihui Gao , Yunfan Gao , Sulei Wang , Dan Li , and Yuedong Xu:
 * CRISLoc: Reconstructable CSI Fingerprinting for Indoor Smartphone Localization.
 *  IEEE Internet of Things Journal, Vol. 8, No. 5, March 1, 2021
 *
 *
 *
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifdef __cplusplus
  extern "C" {
#endif


#include "../CSIFilter.h"
#include "../CSIData.h"
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define THISFILTER_DEFAULT_ACTIVE "1"
#define THISFILTER_DEFAULT_PRIORITY "50"
#define THISFILTER_DEFAULT_GAIN 1000
static uint32_t gain = THISFILTER_DEFAULT_GAIN;

void filter_getName(char* str){
  snprintf(str, CSI_FILTER_NAME_STLEN, "Corrects Gain Compensation using RSSI");
}

void filter_getDescription(char* str){
  snprintf(str, CSI_FILTER_NAME_DESCRIPTION_STLEN, "Corrects AGC of the WIFI SoC.\n\nApplies Equation (9) from\nZhihui Gao , Yunfan Gao , Sulei Wang , Dan Li , and Yuedong Xu:\nCRISLoc: Reconstructable CSI Fingerprinting for Indoor Smartphone Localization.\nIEEE Internet of Things Journal, Vol. 8, No. 5, March 1, 2021.");
}

void filter_setParameter(char* parameter, char* value){
  if(strcmp(parameter,"gain")==0){
    gain = atoi(value);
  }

}

void filter_getParameter(char* parameter, char* value){

  //tell the GUI that this filter should be active by default
  if(strcmp(parameter,"defaultActive")==0){
    sprintf(value,THISFILTER_DEFAULT_ACTIVE);
  }
  //tell the GUI the default priority of this filter
  if(strcmp(parameter,"defaultPriority")==0){
    sprintf(value,THISFILTER_DEFAULT_PRIORITY);
  }

  //tell the GUI the default priority of this filter
  if(strcmp(parameter,"gain")==0){
    sprintf(value,"%u",gain);
  }
}

void filter_getParameterList(char* list){
  sprintf(list,"gain,Filter gain (to amplify the signal),integer,1,100000,1,0");
}

void filter_init(){
  printf("AGC filter initialized.\n");
}

void filter_finalize(){
  printf("AGC filter finalized.\n");
}

void filter_reset(){

}

//see Equation (9) in Zhihui Gao , Yunfan Gao , Sulei Wang , Dan Li , and Yuedong Xu: CRISLoc: Reconstructable CSI Fingerprinting for Indoor Smartphone Localization. IEEE INTERNET OF THINGS JOURNAL, VOL. 8, NO. 5, MARCH 1, 2021
void filter_run(struct CSIData* data){
  static double s = 0;
  static double sq = 0;
	sq = 0;
  for(uint32_t i = 0; i < data->nSubCarriers;i++){
    sq += data->amplitude[i] * data->amplitude[i];
  }
   s = sqrt(pow(10.0,((double) data->RSSI)/10.0)/sq);
  for(uint32_t i = 0; i < data->nSubCarriers;i++){
    data->amplitude[i] = data->amplitude[i] * s * ((double) gain);
  }

}


#ifdef __cplusplus
  }
#endif
