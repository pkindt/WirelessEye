/**
 * sample filter .c - simple example on how to write a filter for the CSIGUI
 *
 *  July 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WiFiEye.
 *
 *  WiFiEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WiFiEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WiFiEye. If not, see <https://www.gnu.org/licenses/>.
 *
 *    GENERIC INFOS:
 *
 * 1) Filtering
 *     The actual filtering works by filter_run().
 *      Before calling filter_run() for the first time, the GUI calls filter_init(). Before the filter is deactivated, it will call filter_finalize().
 *      For every wifi-frame, filter_run() is called once. It obtains a pointer to the CSI data. The filter is allowed to modify these data.
 *      The execution order of different filters is determined by their priorities, which is an integer between 0 and 100 that can be chosen by the user in the GUI.
 *      A filter with higher priority is executed before a lower-priority one. Multiple filters are daisy-chained (i.e., the input of the next filter is the output of the previous one).
 *
 * 2) Parameters
 *      Every filter have an arbitrary number of parameter values that can be controlled by the GUI.
 *      A parameter is read by the GUI by filter_getParameter() and written by filter_setParameter. The GUI obtains a list of available parameters using filter_getParameterList().
 *      Parameters are exchanged as key-value pairs, where "name" is the key and "value" the value. "key" and "value" are strings. "value" typically represents a numeric value (e.g., using atoi()).
 *
 * 3) Getting displayed in the GUI
 *     Filters are compiled into a shared object (see below) and get the ending of ".cfi" (CSI filter). The gui will search for all ".cft" files in a certain folder (by default, in the "filters" subdirectory.
 *     Every filter has a name (see filter_getName()) and a description (see filter_getDescription()). Both are shown in the GUI, along with all parameters listed by filter_getParameterList().
 * 4) Compiling filters
 *     In a terminal and in the filters folder, type "./compile sample_filter" (e.g., the filename of the .c-file without the .c-extension"). Filters can/must be compiled independently from the GUI.
 * 5) Priorities:
 *    Filters with lower priority number are executed before those with higher priority number (i.e., execution order from low to high)
 *
 * Note: If you would like to create additional functions in a filter, which are not called by the GUI but which you call internally from within the filter c-code, you need to declare them as static. Otherwise,
 * compilation will fail.
 */

#ifdef __cplusplus                      //this is needed because our plugin is written in C, whreas WiFiEye is a C++ program
  extern "C" {
#endif



#include "../CSIFilter.h"               //for constants on the max. supported string length, e.g. CSI_FILTER_NAME_STLEN
#include "../CSIData.h"                 //for struct CSIData
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

/// Default values. They will be used in the filter_getParameter function
#define THISFILTER_DEFAULT_ACTIVE "0"                                   //Should the filter be activated by default? "0" => no, "1" => yes
#define THISFILTER_DEFAULT_PRIORITY "1"                                 //The default priority of the filter. When multiple filters are activated, the ones with lower priority number are executed before ones with higher priority


///Some variable. Be sure to declare your variables static. Their values will be retained between different calls of the filter functions
static int32_t scaleFactor = 1;

/**
* The name of the filter. The gui will provide a buffer of length CSI_FILTER_NAME_STLEN (which is defined in CSIFIlter.h").
* Write the name of the filter in this buffer. It will show up like this in the GUI.
*/
void filter_getName(char* str){
  snprintf(str, CSI_FILTER_NAME_STLEN, "A simple sample filter");
}

/*
 * Similarly to the name, every filter has some short description what it does.
 */
void filter_getDescription(char* str){
  snprintf(str, CSI_FILTER_NAME_DESCRIPTION_STLEN, "Scales the CSI amplitude by an integer factor. Not really useful, but it will illustrate how to write a filter.");
}

/**
 * The GUI and the filter can exchange parameter values. All parameters are exchanged via strings.
 * Every parameter has a name (string) and a value (string), which form a key-value pair.
 * The parameter is identified via its name. The value in the string can either be numeric (e.g., "12345") or
 * a string in the broader sense (e.g., "CSI is pretty cool").
 *
 * There are 3 steps that contribute to the parameter exchange mechanism:
 * - filter_setParameter() is used to set a parameter value. The GUI will call it when it wants to change the parameter value.
 * - filter_getParameter() is used by the GUI to read a parameter from the filter
 * - filter_getParameterList() is called by the GUI to get an overview on which parameters are available at the filter, and how the data contained in the value string needs to be interpreted.
 *
 * The filter_setParameter() - function sets the parameter with title "parameter" to the value "value". It is called by the GUI to communicate a parameter value to the filter.
 * If the GUI tries to set the value of a parameter that does not exist, "value" should remain unchanged.
 * Both for "parameter" and "value", the GUI provides a memory buffer of length CSI_FILTER_NAME_PARMETER_STLEN bytes, as defined in CSIFIlter.h.
 */
void filter_setParameter(char* parameter, char* value){
  printf("[addRSSI Filter] set parameter %s to %s\n",parameter,value);
  if(strcmp(parameter,"Scale Factor")==0){
    scaleFactor = atoi(value);
  }

}

/**
* The filter_getParameter() is called by the GUI to read the parameter with name "parameter". The value needs to be written into the "value" parameter as a string.
* If the GUI tries to read the value of a parameter that does not exist, "value" should remain unchanged.
* Both for parameter and value, the GUI provides a memory buffer of length CSI_FILTER_NAME_PARMETER_STLEN bytes, as defined in CSIFIlter.h.
*/
void filter_getParameter(char* parameter, char* value){
  printf("[addRSSI Filter] read parameter %s\n",parameter);

 // Tell the GUI that this filter should be active by default. Note that a) this is optional and b) does not need to be contained in the parameter list (see filter_getParameterList), since it should not show up as an option in the GUI. If
 // this parameter is not implemented, the filter is disactivated by default.
  if(strcmp(parameter,"defaultActive")==0){
    sprintf(value,THISFILTER_DEFAULT_ACTIVE);
  }

  //Tell the GUI the default priority of this filter. Note that this does not need to be contained in the parameter list (see filter_getParameterList), since it should not show up as an option in the GUI. If this parameter is not 
  // implemented, the default priority is 1.
  if(strcmp(parameter,"defaultPriority")==0){
    sprintf(value,THISFILTER_DEFAULT_PRIORITY);
  }

  
 //The scale factor by which we scale the CSI amplitude in this filter
  if(strcmp(parameter,"Scale Factor")==0){
    sprintf(value,"%d",scaleFactor);
  }

}

/**
 * This function is called by the GUI to obtain a list of parameters. The maximum length of the list is given by CSI_FILTER_NAME_PARMETER_LIST_STLEN, as defined in CSIFilter.h
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
 * "Scale factor,integer,0,100,0,0\nAnother parameter,bool,0,0,0,0\n"
 * Separate multiple lines by newlines ("\n").
 * Do not use whitespaces before/after the commas or newlines.
 * "list" is a buffer of length CSI_FILTER_NAME_PARMETER_LIST_STLEN provided by the GUI.
 */
void filter_getParameterList(char* list){
  snprintf(list,CSI_FILTER_NAME_PARMETER_LIST_STLEN,"scaleFactor,Scale Factor,integer,0,10000,0,0");

}


/**
 *    Before filter_run() is called for the first time, filter_init is called once. This happens after the activation of the filter.
 */
void filter_init(){
  printf("CSI sample filter initialized.\n");
}

/**
 *After filter_run() has been called for the last time, filter_init is called once. This happens when the filter is deactivated.
 */
void filter_finalize(){
  printf("CSI sample filter finalized.\n");
}


/**
* Reset the filter.
*/
void filter_reset(){
  scaleFactor = 1;             //reset to default
}

/**
 * Run the actual filter. This will be called once per WiFi frame.
 * "data" is a pointer to all data related to the WiFi frame. The filter may modify the "amplitude" and "phase" information. All
 * other parameters should be left unchanged. The data structure is defined in CSIData.h
 *
 * - data->SubCarriers contains the number of subcarriers.
 * - data->amplitudes contains nSubCarriers CSI amplitudes. They may be read or modified (or both)
 * - data->phases contains nSubCarriers CSI phases. They may be read or modified (or both)
 */
void filter_run(struct CSIData* data){
  for(uint32_t i = 0; i < data->nSubCarriers;i++){
      //simply scale the amplitude :)
      data->amplitude[i] = data->amplitude[i] * (double) scaleFactor;
  }
}


#ifdef __cplusplus
  }
#endif
