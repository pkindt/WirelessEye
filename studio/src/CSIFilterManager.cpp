/*
 * CSIFilterManager.cpp
 *
 * July 2021,  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CSIFilterManager.h"
#include <QDir>
#include <QtAlgorithms>
#include <iostream>
using namespace std;
struct sortStruct;

/**
 * A struct to be passed to qSort(). It implements  the operator  operator<, such that filters can be sorted by priorities.
 */
struct sortStruct{
  uint32_t filterID;
  uint32_t priority;
  bool operator<(const sortStruct & comp) const{
    return(this->priority < comp.priority);
  }
};

void CSIFilterManager::loadFilterList(const QString& path){
  CSIFilterObj* filter;
  mutex.lock();
  for(uint32_t i =0; i < filters.length(); i++){
    delete filters[i];
  }
  filters.clear();
  this->path = path;
  QDir directory(path);
  QStringList fileNames = directory.entryList(QStringList("*.cfi"), QDir::Files,QDir::Name);
  for(uint32_t i = 0; i < fileNames.length(); i++){
    filter = new CSIFilterObj();
    filter->setFileName((QString(path).append("/").append(fileNames[i])).toLocal8Bit().data());
    filter->prepare();
    filters.append(filter);
  }
  mutex.unlock();
}

CSIFilterManager::CSIFilterManager(){
  mutex.unlock();
}

CSIFilterManager::~CSIFilterManager(){
  mutex.lock();

  for(uint32_t i =0; i < filters.length(); i++){
    delete filters[i];
  }

  filters.resize(0);
  mutex.unlock();
}


QVector<CSIFilterObj*>* CSIFilterManager::getFilterList(){
  return &filters;
}


void CSIFilterManager::updatePriorities(){
  mutex.lock();

  //Create a vector of type sortStruct and fill the priorities. This vector can then be
  //sorted by filter priorities.
  QVector<sortStruct> st(filters.length());
  for(uint32_t i = 0; i < filters.length(); i++){
    st[i].filterID = i;
    st[i].priority = filters[i]->getPriority();
  }
  qSort(st);

  //Create a vector of filter IDs sorted by execution order
  priorityVector.resize(filters.length());
  for(uint32_t i = 0; i < filters.length(); i++){
    priorityVector[i] = st[i].filterID;
  }
  mutex.unlock();

}

void CSIFilterManager::applyFilterPipeline(CSIData* data){
  mutex.lock();

  for(uint32_t i = 0; i < filters.length(); i++){
    filters[priorityVector[i]]->execute(data);
  }
  mutex.unlock();

}

