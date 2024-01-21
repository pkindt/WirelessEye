/*
 * CSIFilterObj.cpp
 *
 *  Jul. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */



#include "CSIFilterObj.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

CSIFilterObj::CSIFilterObj(){
  prepared = false;
  strcpy(fileName,"");
  priority = 0;
  active = false;
}

void CSIFilterObj::setFileName(char* fileName){
  strncpy(this->fileName,fileName,CSI_FILTER_NAME_STLEN);
}

void CSIFilterObj::prepare(){
  char buf[CSI_FILTER_NAME_STLEN];
  do_handle = dlopen(fileName,RTLD_LAZY);
  if(do_handle == NULL){
    fprintf(stderr, "Error loading CSIFilter: %s\n", dlerror());
    return;
  }

  //load pointers
  fptr_getName = (void (*)(char*)) dlsym(do_handle,"filter_getName");

  if(fptr_getName == NULL){
    printf("Could not load filter_getName() from library: %s\n",dlerror());
    return;
  }

  fptr_getDesc = (void (*)(char*)) dlsym(do_handle,"filter_getDescription");
  if(fptr_getDesc == NULL){
    printf("Could not load filter_getDescription() from library: %s\n",dlerror());
    return;
  }
  fptr_execute = (void (*)(CSIData*)) dlsym(do_handle,"filter_run");
  if(fptr_execute == NULL){
    printf("Could not load filter_run() from library: %s\n",dlerror());
    return;
  }
  fptr_getParameter = (void (*)(char*, char*)) dlsym(do_handle, "filter_getParameter");
  if(fptr_getParameter == NULL){
    printf("Could not load filter_getParameter() from library: %s\n",dlerror());
    return;
  }
  fptr_setParameter = (void (*)(char*, char*)) dlsym(do_handle, "filter_setParameter");
  if(fptr_setParameter == NULL){
    printf("Could not load filter_setParameter() from library: %s\n",dlerror());
    return;
  }
  fptr_getParameterList = (void (*)(char*)) dlsym(do_handle, "filter_getParameterList");
  if(fptr_getParameterList == NULL){
    printf("Could not load filter_getParameterList() from library: %s\n",dlerror());
    return;
  }
  fptr_init = (void (*)()) dlsym(do_handle,"filter_init");
  if(fptr_init == NULL){
    printf("Could not load filter_init() from library: %s\n",dlerror());
    return;
  }
  fptr_finalize = (void (*)()) dlsym(do_handle,"filter_finalize");
  if(fptr_finalize == NULL){
    printf("Could not load filter_finalize() from library: %s\n",dlerror());
    return;
  }
  fptr_reset = (void (*)()) dlsym(do_handle,"filter_reset");
  if(fptr_reset == NULL){
    printf("Could not load filter_reset() from library: %s\n",dlerror());
    return;
  }
  fptr_getName(this->filterName);
  fptr_getDesc(this->filterDescription);
  fptr_getParameterList(this->filterParameterList);
  printf("Successfully loaded Filter : %s\n",filterName);

  prepared = true;
}

void CSIFilterObj::initialize(){
  if(prepared){
    fptr_init();
  }
}
void CSIFilterObj::execute(CSIData* data){
  if(this->active){
    fptr_execute(data);
  }
}

void CSIFilterObj::getParameter(char* name, char* value){
  if(prepared){
    fptr_getParameter(name,value);
  }
}


void CSIFilterObj::setParameter(char* name, char* value){
  if(prepared){
    fptr_setParameter(name,value);
  }
}

void CSIFilterObj::getParameterList(char* list){
  if(prepared){
    fptr_getParameterList(list);
  }
}
void CSIFilterObj::reset(){
  if(prepared){
    fptr_reset();
  }
}
void CSIFilterObj::finalize(){
  if(prepared){
    fptr_finalize();
  }
}

CSIFilterObj::~CSIFilterObj(){
  if(prepared){
    printf("Filter %x finalizing...\n");
    fptr_finalize();
    dlclose(do_handle);
    prepared = false;

  }
}

QString CSIFilterObj::getFileName(){
  return QString(fileName);
}

QString CSIFilterObj::getName(){
  if(prepared){
    fptr_getName(filterName);
    return QString(filterName);
  }else{
    return QString(" - filter not yet prepared or erroneous - ");
  }
}
QString CSIFilterObj::getDescription(){

  if(prepared){
    return QString(filterDescription);
  }else{
    return QString(" - filter not yet prepared or erroneous - ");
  }

}

uint32_t CSIFilterObj::getPriority(){
  return priority;
}

void CSIFilterObj::setPriority(uint32_t priority){
  this->priority = priority;
}

void CSIFilterObj::setActive(bool active){
  if(prepared){
    if((!this->active)&&(active)){
      fptr_init();
    }else if((this->active)&&(!active)){
      fptr_finalize();
    }
    this->active = active;
  }
}

bool CSIFilterObj::getActive(){
  return this->active;
}
