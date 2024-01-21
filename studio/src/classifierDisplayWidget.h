/*
 * classifierDisplayWidget.h
 *
 * Nov. 2020,  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CLASSIFIERDISPLAYWIDGET_H_
#define CLASSIFIERDISPLAYWIDGET_H_


#include <QtGui>
#include <QWidget>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include <QSlider>
#include <inttypes.h>
#include <QPainter>
#include "CSIData.h"
#include "ui_mainwindow.h"
class MainWindow;


using namespace std;
/** \brief A widget to paint CSI classification results.
 *
 * We assume that the output of a classifier is an integer value within a given range, which represents the actual class.
 * Each classifier will be depicted by a bar. The height of each bar depicts the classification result, which is always an integer.
 * The color of the bar represents certainty of the classification, which is a floating point value between 0 and 1.
 */
class classifierDisplayWidget : public QWidget{

  Q_OBJECT

private:
        MainWindow* mw;                         ///Pointer to the main window
        QMutex *mutex;                          ///A mutex to protect this class against access from multiple concurrent threads
        QImage *img;                            ///QImage to draw into
        uint32_t nClassifiers;                  ///The number of classifiers that we are supposed to display, which needs to be set via setNClassifiers()
        uint32_t binWidth;                      ///The width of a bar. It is determined by the difference in time between to subsequent classifier results, and the number of arrived CSI frames in the meantime.
        QPainter *painter;                      ///A QPainter to draw
        uint32_t* lastOutputs;                  ///An array of the previously received classification results. Until new data has received, this data is drawn as a sort of "preview" for the current data.
                                                ///Once new data has arrived, these "previews" will be redrawn with the actual data
        uint32_t maxHeight;                     ///The maximum height of a bar. Depends on the window height and the number of classifiers (and hence, bars)
       uint32_t timeStepsSinceLastData;         ///The number of time-steps since the last classification result has arrived. The time is advanced by "1" whenever a news WiFi frame (and hence, a bunch of CSI data) arrives.
       bool dirtyFlag;                          ///When true, the entire image has to be redrawn. Otherwise, we keep whatever has been drawn before and just add to the already positioned pixels.
       int maxValue;                            ///The maximum value the classifier might ouput. To be set using setMaxValue().
public:
        classifierDisplayWidget(QWidget* parent = 0);
        ~classifierDisplayWidget();

        /**
         * The widget needs to be redrawn.
         * */
        virtual void paintEvent(QPaintEvent* ev);

        /**
         * Set the number of classifiers to be displayed. A bar is drawn for each of them
         * */
        void setNClassifiers(uint32_t nClassifiers);

        /**
         *  Set a pointer to the main window.
         */
        void setMainWindow(MainWindow* mw);

        /**
         * The widget has been resized. This has implications e.g., on maxValue.
         */
        void resizeEvent(QResizeEvent* event);

public slots:

        /**
         * Reset everything to its default values. e.g., setNClassifiers() and setMaxValue() need to be called again
         */
        void reset();

        /**
         *  Add one unit of time, i.e., scroll the figure by one pixel to the left. This needs to be called whenever a new WiFi frame and hence new CSI data arrives.
         */
        void addTime();

        /** Add a new classifier output.
         * @parameters:
         *      - classifID: The ID of the classifier. This is a counting number between 0 and the number of classifiers - 1
         *      - classNo: The class number that the classifier has determined based on the CSI data.
         *      - certainty: A value between 0 and 1. 0 means that the classifier is entirely uncertain, 1 means its is fully shure that the determined class is correct.
         * E.g. classifID=0 estimates classNo=1 with a certainty of 0.95.
         * */
        void addClassifierOutput(unsigned int classifID, unsigned int classNo, float certainty);

        /*
         * Set the maximum class the classifier might deliver.
         */
        void setMaxValue(int maxValue);

};
#endif
