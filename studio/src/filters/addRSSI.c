/*
 * addRSSI.c - simple RSSI addition filter
 *
 *  Jun. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WiFiEye.
 *
 *  WiFiEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WiFiEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WiFiEye. If not, see <https://www.gnu.org/licenses/>.
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

#define THISFILTER_DEFAULT_ACTIVE "0"
#define THISFILTER_DEFAULT_PRIORITY "1"

static int32_t scaleFactor = 1;
void filter_getName(char* str){
  snprintf(str, CSI_FILTER_NAME_STLEN, "Add RSSI to CSI Amplitude");
}

void filter_getDescription(char* str){
  snprintf(str, CSI_FILTER_NAME_DESCRIPTION_STLEN, "Adds the value of the RSSI signal to the Amplitude. ");
}

void filter_setParameter(char* parameter, char* value){
  printf("[addRSSI Filter] set parameter %s to %s\n",parameter,value);
  if(strcmp(parameter,"Scale Factor")==0){
    scaleFactor = atoi(value);
  }

}

void filter_getParameter(char* parameter, char* value){
  printf("[addRSSI Filter] read parameter %s\n",parameter);

  //tell the GUI that this filter should be active by default
  if(strcmp(parameter,"defaultActive")==0){
    sprintf(value,THISFILTER_DEFAULT_ACTIVE);
  }
  //tell the GUI the default priority of this filter
  if(strcmp(parameter,"defaultPriority")==0){
    sprintf(value,THISFILTER_DEFAULT_PRIORITY);
  }
  if(strcmp(parameter,"Scale Factor")==0){
    sprintf(value,"%d",scaleFactor);
  }

}

void filter_getParameterList(char* list){
  snprintf(list,CSI_FILTER_NAME_PARMETER_LIST_STLEN,"Scale Factor,integer,0,100,0,0");

}

void filter_init(){
  printf("CSI RSSI addition filter initialized.\n");
}

void filter_finalize(){
  printf("CSI RSSI addition filter finalized.\n");
}

void filter_reset(){

}
void filter_run(struct CSIData* data){
  for(uint32_t i = 0; i < data->nSubCarriers;i++){
      data->amplitude[i] = data->amplitude[i] + scaleFactor * data->RSSI;
  }
}


#ifdef __cplusplus
  }
#endif
