/*
 * subCarrierReordering.c - bring subcarriers into a linear order
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



#include "../CSIFilter.h"               //for constants on the max. supported string length, e.g. CSI_FILTER_NAME_STLEN
#include "../CSIData.h"                 //for struct CSIData
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

// Default values. They will be used in the filter_getParameter function
#define THISFILTER_DEFAULT_ACTIVE "1"                                   //Should the filter be activated by default? "0" => no, "1" => yes
#define THISFILTER_DEFAULT_PRIORITY "1"                               //The default priority of the filter. When multiple filters are activated, the ones with higher priority are executed before ones with lower priority

static double amplitudes[512];
static double phases[512];

//The name of the filter. The gui will provide a buffer of length CSI_FILTER_NAME_STLEN (which is defined in CSIFIlter.h").
//Write the name of the filter in this buffer. It will show up like this in the GUI.
void filter_getName(char* str){
  snprintf(str, CSI_FILTER_NAME_STLEN, "Subcarrier Reordering");
}

//Similarly to the name, every filter has some short description what it does.
void filter_getDescription(char* str){
  snprintf(str, CSI_FILTER_NAME_DESCRIPTION_STLEN, "Brings the subcarriers into the right order");
}

void filter_setParameter(char* parameter, char* value){

}

void filter_getParameter(char* parameter, char* value){

  //tell the GUI that this filter should be active by default. Note that a) this is optional and b) does not need to be contained in the parameter list (see filter_getParameterList), since it should not show up as an option in the GUI. If this parameter is not implemented, the filter is disactivated by default.
  if(strcmp(parameter,"defaultActive")==0){
    sprintf(value,THISFILTER_DEFAULT_ACTIVE);
  }
  //tell the GUI the default priority of this filter. Note that this does not need to be contained in the parameter list (see filter_getParameterList), since it should not show up as an option in the GUI. If this parameter is not implemented, the default priority is 1.
  if(strcmp(parameter,"defaultPriority")==0){
    sprintf(value,THISFILTER_DEFAULT_PRIORITY);
  }

}

/* This function is called by the GUI to obtain a list of parameters. The maximum length of the list is given by CSI_FILTER_NAME_PARMETER_LIST_STLEN, as defined in CSIFilter.h
 * Format:
 * 1) Per parameter one line, multiple lines are separated by '\n'
 * 2) Format of every line:
 *    Parameter filterIdent,Description,type,min,max,digits,RFU
 *
 * Here:
 *      - filterIdent: an unique identifyer of the parameter. Can be any string, as long as it's unique      
 *      - Description: Short description of the parameter to be shown in the GUI. No longer than very few words. Put a longer description in the filter description instead of the parameter description.
 *      - Type: Can be a) "integer": An integer value
 *                     b) "string":  A string parameter
 *                     c) "bool": A boolean parameter
 *                     d) "float": A floating point value
 *      - min, max: Minimum and maximum value. Only applies for integer parameters
 *      - digits: Number of digits (for floats)
 *      - RFU: reserved for future use. Set to 0 for now
 *
 * Example:
 * "Scale factor,integer,0,100\nAnother parameter,bool,0,0,0,0\n"
 *
 * Do not use whitespaces before/after the commas or newlines.
 * "list" is a buffer of length CSI_FILTER_NAME_PARMETER_LIST_STLEN provided by the GUI.
 */
void filter_getParameterList(char* list){
}

//Before filter_run() is called for the first time, filter_init is called once. This happens after the activation of the filter.
void filter_init(){
  printf("Subcarrier reordering filter initialized.\n");
}

//After filter_run() has been called for the last time, filter_init is called once. This happens when the filter is deactivated.
void filter_finalize(){
  printf("Subcarrier reordering filter finalized.\n");
}


//Reset the filter.
void filter_reset(){
}
void filter_run(struct CSIData* data){
  uint32_t i;
  memcpy(amplitudes,data->amplitude,data->nSubCarriers*sizeof(double));
  memcpy(phases,data->phase,data->nSubCarriers*sizeof(double));
  if((data->nSubCarriers == 64)&&(data->nSubCarriers_orig==64)){                   //20MHz Channel
  //for 20MHz, just swap upper and lower 32 channels
  for(i = 0; i < 32;i++){
      //simply scale the amplitude :)
      data->amplitude[i] = amplitudes[i+32];
      data->phase[i] = phases[i+32];
  }
  for(i = 32; i < 64;i++){
      //simply scale the amplitude :)
      data->amplitude[i] = amplitudes[i-32];
      data->phase[i] = phases[i-32];
  }
}

//2do: handle 40 and 80 MHz channels
}


#ifdef __cplusplus
  }
#endif
