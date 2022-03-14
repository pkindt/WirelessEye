/*
 * phaseUnwrapping_old.c - compensates for 2*pi-jumps in the phase anda also forces the phase offset between different frames to zero.
 * This filter is outdated. use phaseUnwrapping.c instead
 *
 *  July 2021,  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
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
#include <math.h>
#include <float.h>


#define THISFILTER_DEFAULT_ACTIVE "0"
#define THISFILTER_DEFAULT_PRIORITY "60"
#define THISFILTER_DEFAULT_MULTIMAC_ACTIVE 1
#define THISFILTER_MAXMACSSUPPORTED 100
#define THISFILTER_DEFAULT_INTERFRAME_COMPENSATION_ACTIVE 0
#define THISFILTER_DEFAULT_PHASE_SUBTRACTION_ACTIVE 1
#define THISFILTER_DEFAULT_INITIAL_PHASE_NORMALIZATION_ACTIVE 1
#define THISFILTER_DEFAULT_EXCLUDE_GUARDS_ACTIVE 1

#define PI 3.1415927
#define JUMP_TOLERANCE  PI/2.0
static double phasePrev[THISFILTER_MAXMACSSUPPORTED];
static double phaseOffsets[THISFILTER_MAXMACSSUPPORTED];

static uint8_t senderMacs[THISFILTER_MAXMACSSUPPORTED][7];
static uint8_t multiMac =  THISFILTER_DEFAULT_MULTIMAC_ACTIVE;
static uint8_t interFrameCompensation =  THISFILTER_DEFAULT_INTERFRAME_COMPENSATION_ACTIVE;
static uint8_t phaseSubtraction =  THISFILTER_DEFAULT_PHASE_SUBTRACTION_ACTIVE;
static uint8_t initialPhaseNormalization =  THISFILTER_DEFAULT_INITIAL_PHASE_NORMALIZATION_ACTIVE;
static uint8_t excludeGuards =  THISFILTER_DEFAULT_EXCLUDE_GUARDS_ACTIVE;
static uint32_t guardCarriers[100];
static uint32_t nGuardCarriers = 9;
static uint32_t nMACs = 1;
void filter_getName(char* str){
  snprintf(str, CSI_FILTER_NAME_STLEN, "[Deprecaded] Phase Unwrapping");
}

void filter_getDescription(char* str){
  snprintf(str, CSI_FILTER_NAME_DESCRIPTION_STLEN, "Unwraps the CSI Phase. Old algorithm.");
}

void filter_setParameter(char* parameter, char* value){
  if(strcmp(parameter,"interFrameCompensation")==0){
    interFrameCompensation = atoi(value);
  }
  if(strcmp(parameter,"phaseSubtraction")==0){
    phaseSubtraction = atoi(value);
  }
  if(strcmp(parameter,"multiMACs")==0){
    multiMac = atoi(value);
    if(!multiMac){
      nMACs = 1;
    }
  }
  if(strcmp(parameter,"excludeguards")==0){
    excludeGuards = atoi(value);
  }
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
  if(strcmp(parameter,"interFrameCompensation")==0){
    sprintf(value,"%u",interFrameCompensation);
  }
  if(strcmp(parameter,"phaseSubtraction")==0){
    sprintf(value,"%u",phaseSubtraction);
  }
  if(strcmp(parameter,"multiMACs")==0){
    sprintf(value,"%u", multiMac);
  }
  if(strcmp(parameter,"excludeguards")==0){
    sprintf(value,"%u", excludeGuards);
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
  sprintf(list, "interFrameCompensation, Compensate Phase Difference Between Consecutive Frames,bool,0,1,0,0\nphaseSubtraction,Form Difference Between two Consecutive Subcarriers,bool,0,1,0,0\nmultiMACs,Support multiple sender MACs,bool,0,1,0,0\nexcludeguards,Exclude invalid subcarriers (guard carriers and similar),bool,0,1,0,0\nguardCarriers,Subcarriers to Exclude (if excluding is activated),string,0,0,0,0");
}

static void filter_init_internal(){
  nMACs = 0;
  phasePrev[0] = DBL_MIN;
  phaseOffsets[0] = 0;
}

static void filter_finalize_internal(){
  nMACs = 0;
}

void filter_init(){
  filter_init_internal();
  guardCarriers[0] = 0;
  guardCarriers[1] = 1;
  guardCarriers[2] = 2;
  guardCarriers[3] = 3;
  guardCarriers[4] = 31;
  guardCarriers[5] = 61;
  guardCarriers[6] = 62;
  guardCarriers[7] = 63;
  guardCarriers[8] = 64;
  printf("Phase unwrapping filter initialized.\n");
}

void filter_finalize(){

  filter_finalize_internal();
  printf("Phase unwrapping filter finalized.\n");


}

void filter_reset(){

  filter_init_internal();
}

void filter_run(struct CSIData* data){
  int8_t found = -1;
  uint32_t i;
  double tmp;

/**  Determine which MAC this frame belongs to. if not found, add it to senderMacs[]-list */ 
  if(multiMac){
    //finding out to which MAC we belong...
    for(i = 0; i < nMACs; i++){
      if((memcmp(senderMacs[i],data->senderMAC,7)==0)){
        found = i;
      }
    }

    //the MAC address does not yet exist
    if(found < 0){
      if(nMACs < THISFILTER_MAXMACSSUPPORTED){
        nMACs++;
      }
      found = nMACs - 1;
      memcpy(senderMacs[found], data->senderMAC,7);
      phasePrev[found] = DBL_MIN;
      phaseOffsets[found] = 0;
    }
  }else{
    //multiMAC off
    found = 0;
  }



  //there is no prev yet....
  if(phasePrev[found] == DBL_MIN){
    phasePrev[found] = data->phase[0];
  }


  if(interFrameCompensation){
    //make this phase smooth with the last one of the previous packet
    phaseOffsets[found] = phasePrev[found] - data->phase[0];
  }else{
    if(initialPhaseNormalization){
      //set phase of first subcarrier to zero
      phaseOffsets[found] = -data->phase[0];
    }else{
      //do not influence phase of first packet
      phaseOffsets[found] = 0;
    }
  }
  phasePrev[found] = phaseOffsets[found];
  uint32_t j;
  uint8_t skip = 0;
  int32_t iFirstNonSkipped = -1;
  for(uint32_t i = 0; i < data->nSubCarriers; i++){
    skip = 0;
    if(excludeGuards){
      for(j = 0; j < nGuardCarriers; j++){
        if(i == guardCarriers[j]){
          skip = 1;
        }
      }
      if(skip){
        if((phaseSubtraction)){
          data->phase[i] = phasePrev[found];
        }else{
          data->phase[i] = 0;//phasePrev[found];
        }

        continue;
      }
    }
    if(iFirstNonSkipped == -1){
      iFirstNonSkipped = i;
    }

    //compensate for 2PI-jumps
    if(data->phase[i] + phaseOffsets[found] - phasePrev[found] < -(2*PI - JUMP_TOLERANCE)){
      if((interFrameCompensation)||(i > iFirstNonSkipped)){
        phaseOffsets[found] =  phaseOffsets[found] + 2*PI;
      }
    }
    if(data->phase[i] + phaseOffsets[found] - phasePrev[found] > (2*PI - JUMP_TOLERANCE)){
      if((interFrameCompensation)||(i > iFirstNonSkipped)){
        phaseOffsets[found] =  phaseOffsets[found] - 2*PI;
      }
    }
    data->phase[i] = data->phase[i] + phaseOffsets[found];



    tmp = phasePrev[found];
    phasePrev[found] = data->phase[i];
    //subtract any two consecutive phases
    if(phaseSubtraction){
      data->phase[i] = data->phase[i] - tmp;
    }
  }

}


#ifdef __cplusplus
}
#endif
