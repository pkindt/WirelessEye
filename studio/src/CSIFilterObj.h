/*
 * CSIFilterObj.h
 *
 *  Jul. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CSIFILTEROBJ_H_
#define CSIFILTEROBJ_H_

#include "CSIFilter.h"
#include "CSIData.h"
#include <QString>

/**
 * \brief Interface to a filter plugin
 *
 * This class represents one single filter plugin in an abstract form. It is used to interface the actual filter plugin.
 * The collection of CSIFilter objects is maintained in the CSIFilterManager class.
 *
 * The best way to learn the actual filter interface is looking at sample_filter.c in the filters/ - directory.
 *
 * A filter plugin is a shared object, from which multiple functions are imported using dlsym().
 * The main purpose of this class is storing pointers to these functions, and to provide a convenient interface to them.
 */
class CSIFilterObj{
private:
    char fileName[CSI_FILTER_NAME_STLEN];                       ///File name of the filter
    char filterName[CSI_FILTER_NAME_STLEN];                     ///Name of the filter
    char filterDescription[CSI_FILTER_NAME_DESCRIPTION_STLEN];  ///Description of the filter
   char filterParameterList[CSI_FILTER_NAME_PARMETER_LIST_STLEN];///Formatted string describing all paramters of the filter
    void* do_handle;                                             ///Handle for the dynamic object belonging to this filter
    bool prepared;                                               ///true => the filter has been initialized

    /* Function pointers to call functions of the filter*/
    void (*fptr_getName)(char*);                                ///Read the name of the filter
    void (*fptr_getDesc)(char*);                                ///Read the desctription of the filter
    void (*fptr_execute)(CSIData*);                             ///Execute the actual filter function
    void (*fptr_getParameter)(char*, char*);                    ///Get the value of a certain parameter
    void (*fptr_setParameter)(char*, char*);                    ///Set the value of a certain parameter
    void (*fptr_getParameterList)(char*);                       ///Obtain a list of parameters available for this filter plugin
    void (*fptr_init)();                                        ///Initialize the filter plugin
    void (*fptr_finalize)();                                    ///Finalize (=destroy) the filter plugin
    void (*fptr_reset)();                                       ///Reset the filter plugin
    uint32_t priority;                                          ///Priority assigned to this filter to control the execution order
    bool active;                                                ///True => this filter is active
public:

    /**
     * Set the filename of the filter plugin this object should load.
     */
    void setFileName(char* fileName);

    /**
     * Prepare the filter:
     * - The filename that has been previously set by setFileName is opened
     * - The file is imported usign dlopen()
     * - The functions to be imported from the shared object are retrived using dlsym() and the function pointers are set
     * - Data such as the name of the filter is read from the filter
     */
    void prepare();

    /**
     * Execute a filter.
     * This means that CSI data belonging to one frame is passed to the filter for processing. The filter plugin
     * will received a pointer on a CSIData strucuture. It may read and modify the values in this struct. The modifications
     * are read back by WirelessEye.
     */
    void execute(CSIData* data);

    /* Get a list of paramters the filter provides. A string is copied into the data-parameter. Data needs to be a buffer with a length given by the The maximum length of the list is given by CSI_FILTER_NAME_PARMETER_LIST_STLEN macro.
     *
     * Format of the parameter string:
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
     * Multiple lines are separated by newlines ("\n").
     * Do not use whitespaces before/after the commas or newlines.
     */
    void getParameterList(char* list);

   /**
    * \brief  Set the  value of the parameter identified by the string "name" of the filter. The value needs to be encoded as a string in the "value" parameter.
    *
    * GUI and the filter can exchange parameter values, and the purpose of this function is setting a parameter value of the filter plugin.
    * All parameters are exchanged via strings.
    * Every parameter has a name (string) and a value (string), which form a key-value pair.
    * The parameter is identified via its name. The value in the string can either be numeric (e.g., "12345") or
    * a string in the broader sense (e.g., "CSI is pretty cool").
    *
    *
    * The setParameter() - function sets the parameter with title "parameter" to the value "value". It is called by the GUI to communicate a parameter value to the filter.
    * If the GUI tries to set the value of a parameter that does not exist, "value" should remain unchanged.
    * Both for "parameter" and "value", a buffer of length CSI_FILTER_NAME_PARMETER_STLEN bytes needs to be defined, as defined in CSIFIlter.h.
    */
    void setParameter(char* name, char* value);

    /**
     * Read the  value of the parameter identified by the string "name" of the filter. The value needs to be encoded as a string in the "value" parameter.
     * Both for "parameter" and "value", a buffer of length CSI_FILTER_NAME_PARMETER_STLEN bytes needs to be defined, as defined in CSIFIlter.h.
     */
    void getParameter(char* name, char* value);

    /**
     * Initialize the filter plugin function. The filter init()-function of the filter is called.
     */
    void initialize();

   /**
   * Finalize the filter plugin function. The filter_finalize()-function of the filter is called.
   */
    void finalize();

   /**
   * Reset the filter plugin function. The reset()-function of the filter is called.
   */
    void reset();

   /**
   * Activate (parmeter = true) or deactivate (parameter = false) this filter plugin.
   */
    void setActive(bool);

   /**
   * Returns true, if this filter is activated.
   */
    bool getActive();

    /**
     * Read the priority value of this filter. This is an integer between 1 and 100. A higher priority means earlier execution.
     */
    uint32_t getPriority();

    /**
     * Write the priority value of this filter. This is an integer between 1 and 100. A higher priority means earlier execution.
     */
    void setPriority(uint32_t priority);

    /**
     * Read the filename of this filter
     */
    QString getFileName();

    /**
     * Read the name of this filter
     */
    QString getName();

    /**
     *  Read the description string of this filter
     */
    QString getDescription();

    CSIFilterObj();
    ~CSIFilterObj();
};


#endif /* CSIFILTEROBJ_H_ */
