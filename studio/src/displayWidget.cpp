/*
 * displayWidget.cpp
 *
 *  Nov. 2020,  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
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
#include "displayWidget.h"
#include "mainwindow.h"
#include <float.h>
using namespace std;


displayWidget::displayWidget(QWidget* parent)
{
  printf("Construction @ %x\n",this);
  mw = NULL;
  this->mutex = new QMutex();
  if(this->mutex == NULL){
    printf("error: Mutex creation failed.");
    exit(1);
  };
  this->mutex->unlock();
  maxValue = DBL_MIN;
  minValue = DBL_MAX;

  upperValueBound = 0;
  lowerValueBound = 0;
  scaleFactor = 1;
  subCarrierIndex = 0;
  img = NULL;
  painter = NULL;
  dataOffset = 0.0;
  this->setAutoFillBackground(true);
  QPalette pal = this->palette();
  pal.setColor(QPalette::Normal, QPalette::Window, Qt::white);
  this->setPalette(pal);
  this->setBackgroundRole(QPalette::Window);
  this->show();
  scrollAmount = 0;
  autoScaling = true;
  flatCurve = false;
  scrollDelay = 1;
}

displayWidget::~displayWidget(){
  if(mutex != NULL){
    delete mutex;
    mutex = NULL;
  }
  delete img;
  delete painter;
}

/**
 * Set a pointer to the main window. Do this first before doing anything else
 */
void displayWidget::setMainWindow(MainWindow* mw ) {
  this->mw= mw;
  cout<<"mainWindow set"<<endl;

}

/**
 * Set the number of subcarriers to  NCSISamples
 */
void displayWidget::setNCSISamples(uint32_t NCSISamples){
  if(mutex != NULL){
    mutex->lock();
  }
  this->NCSISamples = NCSISamples;
  if(mutex != NULL){
    mutex->unlock();
  }
}

/**
 * The widget needs to be (re-)painted
 */
void displayWidget::paintEvent(QPaintEvent* ev) {
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


/**
 * Set a scroll delay. If scrollDelay is set to N > 1, we
 * scroll the entire plot by N pixels after the data of N consecutive frames have been recieved.
 * Values of scrollDelay > 1 can increase the performance. But they make the plot look less fluent.
 */
void displayWidget::setScrollDelay(int scrollDelay){
  this->scrollDelay = scrollDelay;
}


/**
 * Add a sample of CSI data.
 * This has to be done once for each subcarrier in each frame in increasing order.
 * The subCarrierIndex - variable is used to keep track of the current subcarrier index to assign the data to
 * csi is not necessarily CSI data (and in fact never represents an entire complex CSI value). It can be RSSI, CSI amplitude or phase.
 */
void displayWidget::addData(double data){
  double color, ub, lb;
  if(mutex != NULL){
    mutex->lock();
  }
  if(flatCurve){
    // "Flat" curves = Curves in 2D => RSSI
    if(autoScaling){
      if(data < minValue){
        minValue = data;
      }
      if(data > maxValue){
        maxValue = data;
      }
    }
    color = (data - minValue) * (double) (255.0)/(maxValue - minValue);
    if(color > 255){
      color = 255;
    }
    if(color < 0){
      color = 0;
    }
    painter->begin(img);
    if(img != NULL){
      img->setPixelColor(img->width()-scrollDelay + scrollAmount-1,((double)(data - minValue)) * ((double) (img->height() - 1.0))/((double)(maxValue - minValue)),QColor(255-color,color, 0));
      img->setPixelColor(img->width()-scrollDelay + scrollAmount-1,((double)(data - minValue)) * ((double) (img->height() - 1.9))/((double)(maxValue - minValue) +1),QColor(255-color,color, 0));
      painter->drawImage(-scrollDelay,0,*img);
    }
    painter->end();
  }else{
    // "Plastic" curves = Curves in 3D with different colors => CSI
    if(img == NULL){
      if(mutex != NULL){
        mutex->unlock();
      }
      return;
    }
    data = data + dataOffset;
    if(autoScaling){
      if(fabs(data) < minValue){
        minValue = fabs(data);
      }
      if(fabs(data) > maxValue){
        maxValue = fabs(data);
      }
    }
    uint32_t scaleFactor =  (img->height()/NCSISamples);

    ub = (double) upperValueBound/1000.0 * (maxValue - minValue) + minValue;
    lb = (double) lowerValueBound/1000.0 * (maxValue - minValue) + minValue;

    /* Goals:
     * 1) Chop off everything below lb and above rb
     * 2) bring range to lie within -255, 255
     *
     * to 1) This would scale between 0...1
     *  color = (data - minValue) / (maxValue - minValue)
     * -> modify to "artificial" min and max values
     *  color = (data - lb) / (ub - lb)
     * ... and chop of what now lies below 0 or above 1
     * to 2): Now rescale: color= color * 510 - 255;
     */
    if(data >= 0){
      color = ((data - lb)/(ub - lb)) * 255.0;
    }else{
      color = -((fabs(data) - lb)/(ub - lb)) * 255.0;

    }
    if(color < -255.0){
      color = -255.0;
    }
    if(color > 255.0){
      color = 255.0;
    }
    /* old color scaling:
     *  color = ((double) fabs(data) - lb) * 255.0/(maxValue - minValue)* (maxValue / (ub - lb));
     */
    painter->begin(img);
    for(uint8_t i = 0;i < scaleFactor;i++){
      if(img != NULL){
        if(color > 0){
          img->setPixelColor(img->width()-scrollDelay + scrollAmount-1, (subCarrierIndex)*scaleFactor + i,QColor((uint8_t)(color),0, (uint8_t) (255.0 - color)));
        }else{
          img->setPixelColor(img->width()-scrollDelay + scrollAmount-1, (subCarrierIndex)*scaleFactor + i,QColor(0,(uint8_t)(color),(uint8_t) (255.0 - color)));
        }
      };
    };
    subCarrierIndex++;
    if(subCarrierIndex >= NCSISamples){                      //when the last sample has arrived
      subCarrierIndex = 0;
      if(scrollAmount >= scrollDelay-1){
        painter->drawImage(-scrollDelay,0,*img);
        //      painter->setBrush(QBrush(QColor(0,0,0)));
        //     painter->drawRect(img->width() - scrollDelay,0,scrollDelay,img->height());
        scrollAmount = 0;
      }else{
        scrollAmount++;
      }
    };
    painter->end();
  }
  if(mutex != NULL){
    mutex->unlock();
  }
}

/**
 * Set the CSI data for an entire frame as an alterantive to the subcarrier-wise addData().
 * - data is an array of values to be displayed for an entire fram
 * - nsamples is the number of values in data. This should be equal to the number of subcarriers
 */
void displayWidget::addDataForEntireFrame(double* data, int nSamples){
  double color, ub, lb;
  if(flatCurve){
    printf("addDataForEntireFrame() not implemented for flat curves.\n");
    return;
  }
  if(nSamples != NCSISamples){
    printf("Skipping display data sicne nSamples != NCSISamples. Probably, it has changed recently\n");
    return;
  }
  if(mutex != NULL){
    mutex->lock();
  }
  if(img == NULL){
    if(mutex != NULL){
      mutex->unlock();
    }
    return;
  }
  painter->begin(img);
  for(uint32_t j= 1; j < nSamples; j++){
    data[j] = data[j] + dataOffset;

    if(autoScaling){
      if(fabs(data[j]) < minValue){
        minValue = fabs(data[j]);
      }
      if(fabs(data[j]) > maxValue){
        maxValue = fabs(data[j]);
      }
    }
    uint32_t scaleFactor = (img->height()/NCSISamples);

    ub = (double) upperValueBound/1000.0 * (maxValue - minValue) + minValue;
    lb = (double) lowerValueBound/1000.0 * (maxValue - minValue) + minValue;

    /* Goals:
     * 1) Chop off everything below lb and above rb
     * 2) bring range to lie within -255, 255
     * 3) If color is negative, treat like a positive value of same magnitude, but paint in green
     *
     * to 1) This would scale between 0...1
     *  color = (data - minValue) / (maxValue - minValue)
     * -> modify to "artificial" min and max values
     *  color = (data - lb) / (ub - lb)
     * ... and chop of what now lies below 0 or above 1
     * to 2): Now rescale: color= color * 255;
     */
    if(data[j] > 0){
      color = ((data[j] - lb)/(ub - lb)) * 255;
    }else{
      color = -((fabs(data[j]) + lb)/(ub - lb)) * 255;
    }
    if(color < -255.0){
      color = -255.0;
    }
    if(color > 255.0){
      color = 255.0;
    }
    for(uint8_t i = 0;i < scaleFactor;i++){
      if(img != NULL){
        if(color > 0){
          img->setPixelColor(img->width()-scrollDelay + scrollAmount-1, (j)*scaleFactor + i,QColor((uint8_t)(color),0, (uint8_t) (255.0 - color)));
        }else{
          img->setPixelColor(img->width()-scrollDelay + scrollAmount-1, (j)*scaleFactor + i,QColor(0,(uint8_t)(color),(uint8_t) (255.0 - color)));
        }
      };
    };
  };
  if(scrollAmount >= scrollDelay-1){
    painter->drawImage(-scrollDelay,0,*img);
    //      painter->setBrush(QBrush(QColor(0,0,0)));
    //     painter->drawRect(img->width() - scrollDelay,0,scrollDelay,img->height());
    scrollAmount = 0;
  }else{
    scrollAmount++;
  }
  painter->end();
  if(mutex != NULL){
    mutex->unlock();
  }
}

/**
 * The widget needs to be resized
 */
void displayWidget::resizeEvent(QResizeEvent* event){
  cout<<"DW resize event @"<<this<<endl;
  printf("new dimension: dx= %u, dy=%u\n",event->size().width(), event->size().height());
  if(this->mutex != NULL){
    this->mutex->lock();
  }
  delete img;
  delete painter;
  cout<<"CR new image"<<endl;
  img = new QImage(event->size().width(),event->size().height(),QImage::Format_RGB32);
  painter = new QPainter();
  img->fill(QColor(0, 0, 0, 0));

  if(this->mutex != NULL){
    this->mutex->unlock();
  }

}


/**
  * Set the scale factor to scale the height of the plot
  */
void displayWidget::setScaleFactor(double scaleFactor){
  if(this->mutex != NULL){
    this->mutex->lock();
  }

  this->scaleFactor = scaleFactor;
  if(this->mutex != NULL){
    this->mutex->unlock();
  }
}

/**
 * Reset the plot
 */
void displayWidget::reset(){
  subCarrierIndex = 0;
  maxValue = FLT_MIN;
  minValue = FLT_MAX;

}

/**
 * The widget rescales the range of colors to a desired range. All values above and below this range are truncated to the max/min color, and
 * the values in between are displayed in color shades. In this way, the range of interesting values to be displayed can be selected.
 * This function sets the upper bound of this range.
 */
void displayWidget::setUB(int value){
  this->upperValueBound = value;
}

/**
  * The widget rescales the range of colors to a desired range. All values above and below this range are truncated to the max/min color, and
  * the values in between are displayed in color shades. In this way, the range of interesting values to be displayed can be selected.
  * This function sets the lower bound of this range.
  */
void displayWidget::setLB(int value){
  this->lowerValueBound = value;
}

/**
 * Sets the data offset.
 * The data offset is added to each value to be displayed.
 * E.g., when setting dataOffset to the mean of a signal to be plotted, it will be plotted around 0.
 */
void displayWidget::setDataOffset(double offset){
  this->dataOffset = offset;
}

/**
 * Read the data offset. See setDataOffset()
 */
double displayWidget::getDataOffset(){
  return dataOffset;
}

/**
 * The widget keeps track of the minimum and maximum observed value so far to scale the data to this range.
 * This function resets the previously observed values
 */
void displayWidget::resetMaximumValue(){
  maxValue = FLT_MIN;
  minValue = FLT_MAX;
}

/**
 * Get the maximum value to be displayed so far
 */
double displayWidget::getMaximumValue(){
  return maxValue;
}

/**
 * Activate and deactivate the automatic tracking of the highest value observed so far.
 */
void displayWidget::setAutoScaling(bool onOff){
  this->autoScaling = onOff;
}

/**
 * Reset the subCarrierIndex value, which is used to track to which subcarrier the data of addData is assigned.
 */
void displayWidget::resetSubCarrierIndex(){
  subCarrierIndex = 0;
}

/**
 * Set the type of the curve.
 * If flatCurve == true, then we plot a 2D curve (e.g., RSSI).
 * Otherwise, we plot a 3D curve, e.g., CSI, in which the 3rd dimension is encoded by the color
 */
void displayWidget::setFlatCurve(bool flatCurve){
  this->flatCurve = flatCurve;
}


