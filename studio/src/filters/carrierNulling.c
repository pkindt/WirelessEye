/**
 * carrierNulling.c - simple carrier nulling filter
 *
 *  July 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
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

#define THISFILTER_DEFAULT_ACTIVE "1"
#define THISFILTER_DEFAULT_PRIORITY "10"

static uint32_t nGuardCarriers = 0;
static uint32_t guardCarriers[256];
void filter_getName(char* str){
  snprintf(str, CSI_FILTER_NAME_STLEN, "Subcarrier Nulling");
}

void filter_getDescription(char* str){
  snprintf(str, CSI_FILTER_NAME_DESCRIPTION_STLEN, "Sets amplitude + phase of certain subcarriers to zero. Used to cancel guard carriers");
}

void filter_setParameter(char* parameter, char* value){
  printf("[addRSSI Filter] set parameter %s to %s\n",parameter,value);
  if(strcmp(parameter,"guardCarriers")==0){
      uint32_t cnt = 0;
      char* token;
      token = strtok(value,",");
      for(cnt = 0; cnt < 100; cnt++){
        if(token == NULL){
          break;
        }
        guardCarriers[cnt] = atoi(token);
        printf("guardCarriers[%u] = %d\n",cnt,guardCarriers[cnt]);
        nGuardCarriers = cnt+1;
        token = strtok(NULL,",");
      }
  }

}

void filter_getParameter(char* parameter, char* value){
  char buf[20];
    //tell the GUI that this filter should be active by default
    if(strcmp(parameter,"defaultActive")==0){
      sprintf(value,THISFILTER_DEFAULT_ACTIVE);
    }
    //tell the GUI the default priority of this filter
    if(strcmp(parameter,"defaultPriority")==0){
      sprintf(value,THISFILTER_DEFAULT_PRIORITY);
    }
    if(strcmp(parameter,"guardCarriers")==0){
      strcpy(value,"");
      for(uint32_t cnt = 0; cnt < nGuardCarriers; cnt++){
        sprintf(buf,"%d,",guardCarriers[cnt]);
        strcat(value,buf);
      }
      value[strlen(value)-1]= '\0';           //remove last comma
    }
}

void filter_getParameterList(char* list){
  snprintf(list,CSI_FILTER_NAME_PARMETER_LIST_STLEN,"guardCarriers,Subcarriers to set to zero,string,0,0,0,0");

}

void filter_init(){
  guardCarriers[0] = 0;
  guardCarriers[1] = 1;
  guardCarriers[2] = 2;
  guardCarriers[3] = 3;
  guardCarriers[4] = 31;
  guardCarriers[5] = 61;
  guardCarriers[6] = 62;
  guardCarriers[7] = 63;
  guardCarriers[8] = 64;
  nGuardCarriers = 9;
  printf("Subcarrier Nulling Filter activated.\n");
}

void filter_finalize(){
  printf("Subcarrier Nulling Filter finalized.\n");
}

void filter_reset(){
  guardCarriers[0] = 0;
  guardCarriers[1] = 1;
  guardCarriers[2] = 2;
  guardCarriers[3] = 3;
  guardCarriers[4] = 31;
  guardCarriers[5] = 61;
  guardCarriers[6] = 62;
  guardCarriers[7] = 63;
  guardCarriers[8] = 64;
  nGuardCarriers = 9;
}
void filter_run(struct CSIData* data){
  uint8_t found;
  for(uint32_t i = 0; i < data->nSubCarriers;i++){
    found = 0;
    for(uint32_t cnt = 0; cnt < nGuardCarriers; cnt++){
      if(guardCarriers[cnt] == i){
        found = 1;
      }
    }
    if(found){
	    data->amplitude[i] = 0;
    	data->phase[i] = 0;
	}
  }
}


#ifdef __cplusplus
  }
#endif
