/*
 * phaseUnwrapping.c - compensates for 2*pi-jumps in the phase anda also forces the phase offset between different frames to zero.
 *
 *
 *  Jul 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
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


/** Algorithm Idea:
1) Set first phase to zero
2) "Unwrap" phase jumps of 2*pi or more
3) Subtract unwrapped phase of this subcarrier from uwrapped phase of the same subcarrier in the previous frame
*/

#define THISFILTER_DEFAULT_ACTIVE "1"
#define THISFILTER_DEFAULT_PRIORITY "60"
#define THISFILTER_DEFAULT_MULTIMAC_ACTIVE 1
#define THISFILTER_MAXMACSSUPPORTED 100
#define THISFILTER_DEFAULT_PHASE_SUBTRACTION_ACTIVE 1
#define THISFILTER_DEFAULT_INITIAL_PHASE_NORMALIZATION_ACTIVE 1
#define THISFILTER_DEFAULT_EXCLUDE_GUARDS_ACTIVE 1

#define PI 3.1415927
#define JUMP_TOLERANCE  PI/1.0
static double phasePrev[THISFILTER_MAXMACSSUPPORTED][512];

static uint8_t senderMacs[THISFILTER_MAXMACSSUPPORTED][7];
static uint8_t multiMac =  THISFILTER_DEFAULT_MULTIMAC_ACTIVE;
static uint8_t phaseSubtraction =  THISFILTER_DEFAULT_PHASE_SUBTRACTION_ACTIVE;
static uint8_t initialPhaseNormalization =  THISFILTER_DEFAULT_INITIAL_PHASE_NORMALIZATION_ACTIVE;
static uint8_t excludeGuards =  THISFILTER_DEFAULT_EXCLUDE_GUARDS_ACTIVE;
static uint32_t guardCarriers[100];
static uint32_t nGuardCarriers = 9;
static uint32_t nMACs = 1;
static uint32_t firstNonGuardCarrier = 0;
static uint8_t firstFrame[THISFILTER_MAXMACSSUPPORTED];
void filter_getName(char* str){
  snprintf(str, CSI_FILTER_NAME_STLEN, "Phase Unwrapping");
}

void filter_getDescription(char* str){
  snprintf(str, CSI_FILTER_NAME_DESCRIPTION_STLEN, "Unwraps the CSI Phase");
}

void filter_setParameter(char* parameter, char* value){
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

	//identify first non-guard carrier
	firstNonGuardCarrier = 0;
	uint8_t found = 0;
	for(uint32_t i = 0; i < 512; i++){
		found = 0;		
		for(cnt = 0; cnt < nGuardCarriers; cnt++){
			if(guardCarriers[cnt] == i){
				found = 1;
			} 		
		}
		
		if(!found){
			firstNonGuardCarrier = i;
			break;
		}
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
  sprintf(list, "phaseSubtraction,Form Difference Between two Consecutive Frames,bool,0,1,0,0\nmultiMACs,Support multiple sender MACs,bool,0,1,0,0\nexcludeguards,Exclude invalid subcarriers (guard carriers and similar),bool,0,1,0,0\nguardCarriers,Subcarriers to Exclude (if excluding is activated),string,0,0,0,0");
}

static void filter_init_internal(){
  nMACs = 0;
  memset(firstFrame,1,THISFILTER_MAXMACSSUPPORTED);
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
  firstNonGuardCarrier = 4;
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
  uint8_t found = 0;
  uint8_t guardFound = 0;

  uint8_t lastWasGuard = 0;
  uint32_t i,j;
  double phaseOffset = 0;
  double phaseLastSubCarrier = 0;
/*  Determine which MAC this frame belongs to. if not found, add it to senderMacs[]-list */
  if(multiMac){
    //finding out to which MAC we belong...
    for(i = 0; i < nMACs; i++){
      if((memcmp(senderMacs[i],data->senderMAC,7)==0)){
        found = i;
      }
    }

    //the MAC address does not yet exist
    if(found == 0){
      if(nMACs < THISFILTER_MAXMACSSUPPORTED){
        nMACs++;
      }
      found = nMACs - 1;
      memcpy(senderMacs[found], data->senderMAC,7);
    }
  }else{
    //multiMAC off
    found = 0;
  }

	// set first phase to 0 by adjusting the phaseOffset;
	phaseOffset = data->phase[firstNonGuardCarrier];
	lastWasGuard = 1;
	phaseLastSubCarrier = 0;
	for(i = 0; i < data->nSubCarriers; i++){
		//check if this is a guard carrier. if so, set to zero
	        guardFound = 0;
		for(j = 0; j < nGuardCarriers; j++){
			if(guardCarriers[j] == i){
			  guardFound = 1;
			   break;
			}	
		}
		if((guardFound)&&(excludeGuards)){
			//guard carrier
			data->phase[i] = 0;
			lastWasGuard = 1;
	               // printf("[%u] %.2f\n",i,data->phase[i]);

			continue;
		}else{
			//no guard => apply offset + unwrap
			data->phase[i] = data->phase[i] - phaseOffset;

			//pos. phase warparound

			while((lastWasGuard == 0)&&(data->phase[i] - phaseLastSubCarrier > 2*PI - JUMP_TOLERANCE)){
				data->phase[i] = data->phase[i] - 2*PI;
				phaseOffset = phaseOffset + 2*PI; 			
			}

			//neg. phase warparound
			while((lastWasGuard == 0)&&(data->phase[i] - phaseLastSubCarrier < -(2*PI - JUMP_TOLERANCE))){
				data->phase[i] = data->phase[i] + 2*PI;
				phaseOffset = phaseOffset - 2*PI; 			
			}

			 //Needs to be stored here, before we form the difference with previous frame phase
                        phaseLastSubCarrier = data->phase[i];

			if((phaseSubtraction)&&(firstFrame[found] == 0)){
			    data->phase[i] = (data->phase[i] - phasePrev[found][i]);
			    phasePrev[found][i] = phaseLastSubCarrier;
			}

                        lastWasGuard = 0;
		}
		//printf("[%u] %.2f\n",i,data->phase[i]);
	}	
	//indicate that the next frame of this MAC is not the first frame
	firstFrame[found] = 0;
}


#ifdef __cplusplus
}
#endif
