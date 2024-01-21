/*
 * classifierDisplayWidget.cpp
 *
 *  Jul. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */


#include <QtGui>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QWheelEvent>
#include <QTimer>
#include <iostream>
#include "classifierDisplayWidget.h"
#include "mainwindow.h"
using namespace std;
#define PREVIEW 1
#define CLASSIF_MARGIN 10
classifierDisplayWidget::classifierDisplayWidget(QWidget* parent)
{
  binWidth = 0;
  nClassifiers = 0;
  lastOutputs = NULL;
  maxHeight = 0;
  timeStepsSinceLastData = 0;
  maxValue = 10;
dirtyFlag = true;
  printf("Construction @ %x\n",this);
  if (parent != NULL) {
    //resize(parent->size());
    cout << "Resize:" << endl;
    cout << parent->size().width() << endl;
    cout << parent->size().height() << endl;
  } else {
    cout << "parent is NULL" << endl;
    exit(1);
  }
  mw = NULL;
  this->mutex = new QMutex();
  if(this->mutex == NULL){
    printf("Error: Mutex creation failed.");
    exit(1);
  };
  this->mutex->unlock();

  img = new QImage(width(),height(),QImage::Format_RGB32);
  painter = new QPainter();
  this->setAutoFillBackground(true);
  QPalette pal = this->palette();
  pal.setColor(QPalette::Normal, QPalette::Window, Qt::white);
  this->setPalette(pal);
  this->setBackgroundRole(QPalette::Window);
  this->show();


}

classifierDisplayWidget::~classifierDisplayWidget(){
  if(mutex != NULL){
    delete mutex;
    mutex = NULL;
  }
    delete painter;
  delete img;
}

/*Set the number of classifiers to be displayed. A bar is drawn for each of them*/
void classifierDisplayWidget::setNClassifiers(uint32_t nClassifiers){
  this->nClassifiers = nClassifiers;
}

/*Set the maximum class the classifier might deliver.*/
void classifierDisplayWidget::setMaxValue(int maxValue){
  this->maxValue = maxValue;
}

/*Set a pointer to the main window.*/
void classifierDisplayWidget::setMainWindow(MainWindow* mw ) {
  this->mw= mw;
  cout<<"mainWindow set"<<endl;

}

/*The widget needs to be redrawn.*/
void classifierDisplayWidget::paintEvent(QPaintEvent* ev) {
  if ((mw == NULL)||(img == NULL)) {
    return;
  }
  if(mutex != NULL){
    mutex->lock();
  }
  QPainter p(this);
  p.drawImage(0,0,*img);
  p.end();
  if(mutex != NULL){
    mutex->unlock();
  }

}
void classifierDisplayWidget::addTime(){
  if(mutex != NULL){
    mutex->lock();
  }
  //just add a bit of time
  binWidth = binWidth + 1;
  painter->begin(img);

  //scrolle everything by one pixel to the left
  painter->drawImage(-1,0,*img);

  //Preview: extend the previously seen result to the most recent point in time until new calssification data arrives.
#if PREVIEW
  if(lastOutputs != NULL){
    if(timeStepsSinceLastData >= 5){
      for(uint32_t i = 0; i < nClassifiers; i++){
        QPen penOld = painter->pen();
        painter->setPen(QPen(Qt::gray, 1));
        painter->drawLine(img->width() - 3 , img->height() - CLASSIF_MARGIN - (i)*(maxHeight + CLASSIF_MARGIN)-lastOutputs[i],img->width()-2, img->height() - CLASSIF_MARGIN - (i)*(maxHeight + CLASSIF_MARGIN)-lastOutputs[i]);
        painter->setPen(penOld);
      }
    }
  }
#endif
  painter->end();
  timeStepsSinceLastData++;
  if(mutex != NULL){
    mutex->unlock();
  }
}


/* Add a new classifier output.
 * Parameters:
 *      - classifID: The ID of the classifier. This is a counting number between 0 and the number of classifiers - 1
 *      - classNo: The class number that the classifier has determined based on the CSI data.
 *      - certainty: A value between 0 and 1. 0 means that the classifier is entirely uncertain, 1 means its is fully shure that the determined class is correct.
 * E.g. classifID=0 estimates classNo=1 with a certainty of 0.95.*/
void classifierDisplayWidget::addClassifierOutput(unsigned int classifID, unsigned int classNo, float certainty){
  static uint32_t binWidthToUse = 0;
  int32_t color;
  if(mutex != NULL){
    mutex->lock();
  }

  if(classifID == 0){
    binWidthToUse = binWidth;
  }
  if((classifID+1 > nClassifiers)||(dirtyFlag)){
    painter->begin(img);
    painter->setBrush(QBrush(QColor(0,0,0)));
    painter->setPen(Qt::black);
    painter->drawRect(0,0,img->width(), img->height());
    painter->end();
    nClassifiers = classifID+1;

    binWidth = 0;
    timeStepsSinceLastData = 0;
    dirtyFlag = false;
    delete lastOutputs;
    lastOutputs = NULL;

    if(mutex != NULL){
      mutex->unlock();
    }
    return;
  }
  if(lastOutputs == NULL){
        lastOutputs = new uint32_t[nClassifiers];
        maxHeight = img->height() / nClassifiers - CLASSIF_MARGIN;
        fprintf(stderr, "max height: %d\n",maxHeight);
        printf("new\n");
      }
  color = 255.0*certainty/mw->getUI()->dsbMaxClassCertainty->value();

  if(color > 255){
    color = 255;
  }
  if(color < 0){
    color = 0;
  }
  uint32_t height ;
  if(classNo <= maxValue){
    height = ((float) classNo / (float) maxValue) * maxHeight;
  }else{
    height = maxHeight;
  }

  if(lastOutputs != NULL){
    lastOutputs[classifID] = height;
  }
  painter->begin(img);
#if PREVIEW
  painter->setBrush(QBrush(QColor(0,0,0)));
  painter->setPen(Qt::black);
  painter->drawRect(img->width() - binWidthToUse - 2, img->height() - CLASSIF_MARGIN - (classifID)*(maxHeight + CLASSIF_MARGIN),binWidthToUse,-maxHeight);
#endif
  painter->setBrush(QBrush(QColor((uint8_t)(255 - color), (uint8_t) (color),0)));
  painter->setPen(Qt::white);
  painter->drawRect(img->width() - binWidthToUse - 2, img->height() - CLASSIF_MARGIN - (classifID)*(maxHeight + CLASSIF_MARGIN),binWidthToUse-3,-height);
  painter->end();
  if(classifID == nClassifiers-1){
    binWidth = 0;
  }
  if(mutex != NULL){
    mutex->unlock();
  }
}

/*The widget has been resized. This has implications e.g., on maxValue.*/
void classifierDisplayWidget::resizeEvent(QResizeEvent* event){
  printf("CT resize event begin\n");
  printf("new dimension: dx= %u, dy=%u\n",event->size().width(), event->size().height());
  if(this->mutex != NULL){
    this->mutex->lock();
  }
delete img;
delete painter;
  img = new QImage(event->size().width(),event->size().height(),QImage::Format_RGB32);
  img->fill(QColor(0, 0, 0, 0));

  painter = new QPainter();

  if(lastOutputs != NULL){
    delete lastOutputs;
  }
  lastOutputs = NULL;
  if(this->mutex != NULL){
    this->mutex->unlock();
  }
  printf("CT resize event end\n");


}


/* Reset everything to its default values. e.g., setNClassifiers() and setMaxValue() need to be called again*/

void classifierDisplayWidget::reset(){
  if(this->mutex != NULL){
    this->mutex->lock();
  }
  binWidth = 0;
  nClassifiers = 0;
  dirtyFlag = true;
  delete lastOutputs;
  lastOutputs = NULL;
  maxHeight = 0;
  timeStepsSinceLastData = 0;
  if(this->mutex != NULL){
    this->mutex->unlock();
  }
}


