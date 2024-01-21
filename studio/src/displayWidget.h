/*
 * displayWidget.h
 * A widget to display CSI and RSSI data.
 *
 *  Nov. 2020,  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DISPLAYWIDGET_H_
#define DISPLAYWIDGET_H_


#include <QtGui>
#include <QWidget>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include <QSlider>
#include <inttypes.h>
#include "CSIData.h"
#include "ui_mainwindow.h"
class MainWindow;

using namespace std;

/**
 * \brief A widget to display CSI and RSSI data
 *
 */
class displayWidget : public QWidget{

  Q_OBJECT

private:
        MainWindow* mw;                 ///Pinter to the mainw ondow
        double scaleFactor;             ///A scale factor to adjust the height of the plot to the available space
        QMutex* mutex;                  ///A mutex to protect this class fromt he concurrent access by multiple threads
        double maxValue;                ///The maximum value to be displayed we have seen so far, used for scaling the color values
        double minValue;                ///The minimum value to be displayed we have seen so far, used for scaling the color values
        QImage *img;                    ///Pointer to QImage to plot into
        uint32_t subCarrierIndex;       ///We get the data to be plotted sequentially for all subcarriers. addData() is called once for every subcarrier.
                                        ///Thsi value "counts" through all subcarriers to assign the data to the right subcarrier.
        double upperValueBound;         ///The upper value selected by the sliders to rescale the color range to a range of interest
        double lowerValueBound;         ///The lower value selected by the sliders to rescale the color range to a range of interest
        double dataOffset;              ///An offset to be added to each value to be displayed. E.g., when setting  dataOffset to the mean of a signal to be plotted, it will be plotted around 0
        uint32_t scrollAmount;          ///For performance reasons, we do not scroll the data to be plotted by one pixel for every arrived frame. Instead we can configure to scroll after every N frames by N pixels, where N is the accumulated number of pixels in this amount of time. This variable counts how much we have to scroll (i.e., N in our example),.
        bool autoScaling;               ///If true, we automatically adjust lowerValueBound and upperValueBound during operation to the minimum/maximum values we have observed so far.
        QPainter* painter;              ///QPainter to plot our stuff
        bool flatCurve;                 ///true => our curve is "flat", hence, we plot a traditional 2D curve, such as RSSI.
                                        ///false=> we plot a 3D-curve, e.g., CSI using different color codes
        uint32_t NCSISamples;           ///The number of CSI samples per frame, i.e., the number of subcarriers
        uint32_t scrollDelay;           ///After how many received frames we have to scroll the entire plot. See subCarrierIndex for a deeper understanding
public:
        displayWidget(QWidget* parent = 0);
        ~displayWidget();

        /**
         * The widget needs to be (re-)painted
         */
        virtual void paintEvent(QPaintEvent* ev);

        /**
         * Set a pointer to the main window. Do this first before doing anything else
         */
        void setMainWindow(MainWindow* mw);

        /**
         * The widget needs to be resized
         */
        void resizeEvent(QResizeEvent* event);

        /**
         * Set the scale factor to scale the height of the plot
         */
        void setScaleFactor(double scaleFactor);

        /**
         * Reset the plot
         */
        void reset();

        /**
         * Sets the data offset.
         * The data offset is added to each value to be displayed.
         * E.g., when setting dataOffset to the mean of a signal to be plotted, it will be plotted around 0.
         */
        void setDataOffset(double offset);

        /**
         * Read the data offset. See setDataOffset()
         */
        double getDataOffset();

        /**
         * Get the maximum value to be displayed so far
         */
        double getMaximumValue();

        /**
         * Set the type of the curve.
         * If flatCurve == true, then we plot a 2D curve (e.g., RSSI).
         * Otherwise, we plot a 3D curve, e.g., CSI, in which the 3rd dimension is encoded by the color
         */
        void setFlatCurve(bool flatCurve);

public slots:

        /**
         * Add a sample of CSI data.
         * This has to be done once for each subcarrier in each frame in increasing order.
         * The subCarrierIndex - variable is used to keep track of the current subcarrier index to assign the data to
         * csi is not necessarily CSI data (and in fact never represents an entire complex CSI value). It can be RSSI, CSI amplitude or phase.
         */
        void addData(double csi);

        /**
         * Set the CSI data for an entire frame as an alterantive to the subcarrier-wise addData().
         * - data is an array of values to be displayed for an entire fram
         * - nsamples is the number of values in data. This should be equal to the number of subcarriers
         */
        void addDataForEntireFrame(double* data, int nSamples);

        /**
         * The widget keeps track of the minimum and maximum observed value so far to scale the data to this range.
         * This function resets the previously observed values
         */
        void resetMaximumValue();

        /**
         * Activate and deactivate the automatic tracking of the highest value observed so far.
         */
        void setAutoScaling(bool);

        /**
         * Reset the subCarrierIndex value, which is used to track to which subcarrier the data of addData is assigned.
         */
        void resetSubCarrierIndex();

        /**
         * Set the number of subcarriers to  NCSISamples
         */
        void setNCSISamples(uint32_t NCSISamples);

        /**
         * Set a scroll delay. If scrollDelay is set to N > 1, we
         * scroll the entire plot by N pixels after the data of N consecutive frames have been recieved.
         * Values of scrollDelay > 1 can increase the performance. But they make the plot look less fluent.
         */
        void setScrollDelay(int scrollDelay);

        /**
         * The widget rescales the range of colors to a desired range. All values above and below this range are truncated to the max/min color, and
         * the values in between are displayed in color shades. In this way, the range of interesting values to be displayed can be selected.
         * This function sets the upper bound of this range.
         */
       void setUB(int value);

       /**
         * The widget rescales the range of colors to a desired range. All values above and below this range are truncated to the max/min color, and
         * the values in between are displayed in color shades. In this way, the range of interesting values to be displayed can be selected.
         * This function sets the lower bound of this range.
         */
        void setLB(int value);

};
#endif
