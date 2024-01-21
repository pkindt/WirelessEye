/**
 * RSSISmoothing.c - Exponential smoothing filter to denoise the RSSI
 *
 *  Jun. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
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
#include <float.h>

#define THISFILTER_DEFAULT_ACTIVE "1"
#define THISFILTER_DEFAULT_PRIORITY "10"
#define THISFILTER_DEFAULT_MULTIMAC_ACTIVE 1
#define THISFILTER_SMOOTHING_ALPHA 0.02
#define THISFILTER_MAXMACSSUPPORTED 100

  static double smoothing_alpha = THISFILTER_SMOOTHING_ALPHA;
  static double rssi_filtered[THISFILTER_MAXMACSSUPPORTED];
  static uint8_t senderMacs[THISFILTER_MAXMACSSUPPORTED][7];
  static uint8_t multiMac =  THISFILTER_DEFAULT_MULTIMAC_ACTIVE;
 static uint32_t nMACs = 1;
void filter_getName(char* str){
  snprintf(str, CSI_FILTER_NAME_STLEN, "Exponential smoothing for the RSSI signal");
}

void filter_getDescription(char* str){
  snprintf(str, CSI_FILTER_NAME_DESCRIPTION_STLEN, "Exponential smoothing for de-noising the RSSI signal.\nCarries out the following equation: RSSI_smoothened[t] = alpha*RSSI[t] + (1-alpha)*RSSI_smoothened[t-1]");
}

void filter_setParameter(char* parameter, char* value){
  if(strcmp(parameter,"alpha")==0){
     smoothing_alpha = atof(value);
   }
  if(strcmp(parameter,"multiMACs")==0){
     multiMac = atoi(value);
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
  if(strcmp(parameter,"alpha")==0){
     sprintf(value,"%.3f",smoothing_alpha);
   }
  if(strcmp(parameter,"multiMACs")==0){
    sprintf(value,"%u", multiMac);
  }
 }

void filter_getParameterList(char* list){
  sprintf(list, "alpha,Smoothing Factor Alpha,float,0,1,3,0\nmultiMACs,Support multiple sender MACs,bool,0,0,0,0");
}

static void filter_init_internal(){
  nMACs = 0;
  rssi_filtered[0] = DBL_MIN;
}

static void filter_finalize_internal(){
    nMACs = 0;
}

void filter_init(){
  filter_init_internal();
  printf("RSSI smoothing filter initialized.\n");
 }

void filter_finalize(){
    
  filter_finalize_internal();
  printf("RSSI smoothing filter finalized.\n");


}

void filter_reset(){

    filter_init_internal();
}

//see Equation (9) in Zhihui Gao , Yunfan Gao , Sulei Wang , Dan Li , and Yuedong Xu: CRISLoc: Reconstructable CSI Fingerprinting for Indoor Smartphone Localization. IEEE INTERNET OF THINGS JOURNAL, VOL. 8, NO. 5, MARCH 1, 2021
void filter_run(struct CSIData* data){
       int8_t found = -1;
    if(multiMac){
       for(uint32_t i = 0; i < nMACs; i++){
         if((memcmp(senderMacs[i],data->senderMAC,7)==0)){
             found = i;
        }        
       }
        if(found < 0){
         if(nMACs < THISFILTER_MAXMACSSUPPORTED){
           nMACs++;
         }
         found = nMACs - 1;
         memcpy(senderMacs[found], data->senderMAC,7);
         rssi_filtered[found] = DBL_MIN;
        }
    }else{
        found = 0;
    }
  if(rssi_filtered[found] == DBL_MIN){
    //this is the first value => initialize
    rssi_filtered[found] = (double) data->RSSI;
  }else{
    rssi_filtered[found] = (double) data->RSSI * smoothing_alpha + (1.0-smoothing_alpha)*rssi_filtered[found];
  }
 // printf("[%u] Unfiltered: %f - Filtered: %f\n",found, data->RSSI,rssi_filtered[found]);
  data->RSSI = rssi_filtered[found];
}


#ifdef __cplusplus
  }
#endif
