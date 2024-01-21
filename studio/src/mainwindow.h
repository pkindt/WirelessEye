/*
 * mainwindow.h
 * Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "networkThread.h"                              ///The data is received, processed and dispatched here.
#include "displayWidget.h"                              ///Data is displayed here
#include "classifierThread.h"                           ///A thread controling the data received from the classifier
#include "classifierDisplayWidget.h"                    ///A widget displaying the data from the classifier
#include "dialogabout.h"                                ///The "about" dialog
#include "CSIFilterGUIManager.h"                        ///Builds and controls the adaptive GUI of the filter manager
#include "checkableComboBox.h"                          ///A checkable combo box for the MAC filter
namespace Ui {
class MainWindow;
}

/**
 * \brief The main winow of WirelessEye Studio
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    char classifierBuf[CLASSIFIER_RCV_BUF_LEN];

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


     /**
      *  Returns a pointer to the UI
      */
    Ui::MainWindow* getUI();

    /**
     * Returns a pointer to the display widget for the amplitude
     */
    displayWidget* getdwA();

    /**
     *  Returns a pointer to the display widget for the phase
     */
    displayWidget* getdwP();

    /**
     *  Returns a pointer to the display widget for RSSI
     */
    displayWidget* getdwRSSI();

    /**
     *  Returns a pointer to the classifier display widget
     */
    classifierDisplayWidget* getCDW();

    /**
     *  Returns a pointer to the classifier thread
     */
    classifierThread* getCT();

    /**
     * Returns a pointer to the filter gui manager
     */
    CSIFilterGUIManager* getFGM();

    /**
     *  Returns a pointer to the MAC filter selection widget
     */
    checkableComboBox* getCBX();

    /**
     *  Processes the command line arguments when 2 parameters are given on the command line (url/IP of the server, recorded bandwidth)
     *  @parameters:
     *  url: The hostname or IP address of WirelessEye studio
     *  bw: The bandwith that has been configured in Nexmon
     */
    void proccessCmdLineArguments(char* url,char* bw);

    /**
     *  Processes the command line arguments when 3 parameters are given on the command line (url/IP of the server, recorded bandwidth)
     *  @parameters:
     *  url: The hostname or IP address of WirelessEye studio
     *  bw: The bandwith that has been configured in Nexmon
     *  bwDisplay: The bandwith to display the CSI. Can differ from the bandwidth using which Nexmon captures
     */
    void proccessCmdLineArguments(char* url,char* bw, char* bwDisplay);
    /**
    *  Processes the command line arguments when 4 parameters are given on the command line (url/IP of the server, recorded bandwidth)
    *  @parameters:
    *  url: The hostname or IP address of WirelessEye studio
    *  bw: The bandwith that has been configured in Nexmon
    *  bwDisplay: The bandwith to display the CSI. Can differ from the bandwidth using which Nexmon captures
    *  bwExport: The bandwith to export any CSI data. Can differ from the bandwidth using which Nexmon captures
    */
    void proccessCmdLineArguments(char* url,char* bw, char* bwDisplay, char* bwExport);


    /**
     *  The GUI has been resized
     */
    void resizeEvent(QResizeEvent* event);

    /**
     *  The GUI is moved
     */
    void moveEvent(QMoveEvent * event);

    /**
     * The GUI has been closed
     */
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui  ;                       ///the UI
    networkThread *nt;                          ///the network thread class for receiving CSI data
    displayWidget* dwA;                         ///display widget amplitude
    displayWidget* dwP;                         ///display widget phase
    displayWidget* dwRSSI;                      ///display widget RSSI
    classifierDisplayWidget* cdw;               ///classifier display widget
    classifierThread *ct;                       ///thread to read dat afrom the classifier
    QTimer* animTimer;                          ///timer that triggers the refreshing of all visualization widgets
    QTimer updateLayoutTimer;                   ///timer to update the layout after a resize event
    QThread *nt_thread;                         ///the *actual thread* hosting the network thread class
    bool isStarted;                             ///has the data streaming from the WiFi SoC started?
    DialogAbout da;                             ///a dialog widget to show some information
    CSIFilterGUIManager *fgm;                   ///a GUI Manager for building the widgets to control the filters
    checkableComboBox *cbx;                     ///list of MAC addresses that can be selected.




public slots:
    void startStreaming();                      ///Start the streaming of CSI data from the WiFi SoC
    void streamStartStop(bool started);         ///Toggle steaming
    void animate();                             ///Update the animation

    void recordButtonHandler();                 ///Record-Button clicked -> recording starts or stops
    void updateDisplayWidgetSize();             ///Update the size of the display widget
    void startClassifier();                     ///Start the classifier
    void classifierStartedStopped(bool started);///The classifier has either started (-> started == true) or stopped (-> started == false)
    void updateClassifierData(const QString&);  ///The most recent data output from  the classifier is displayed in the GUI
    void updateDisplayPeriod();                 ///The refresh rate of all visualizations has been changed
    void activateAmplitudeScaling();            ///Activates that the amplitude color scale is adjusted when an amplitude exceeding any previously seen maximum amplitude arrives
    void activatePhaseScaling();                ///Activates that the phase color scale is adjusted when an phaseexceeding any previously seen maximum phase arrives
    void showHidePhase(bool shown);             ///Toggle showing/hiding the phase display widget
    void showHideClassifier(bool shown);        ///Toggle showing/hiding the classifier display widget
    void showHideRSSI(bool shown);              ///Toggle showing/hiding the RSSI display widget
    void showHideAmplitude(bool shown);         ///Toggle showing/hiding the amplitude display widget
    void updateBandwidthHandling();             ///The selected bandwidth for display or export or for the input stream has changed

   signals:
   void stopStreaming();                        ///Stop streaming data from the WiFi SoC
   void startClassifierThread();                ///Start the classifier thread
   void stopClassifierThread();                 ///Stop the classifier
   void resizedwA(int width, int height);       ///Resize amplitude display widget
   void resizedwP(int width, int height);       ///Resize phase display widget
   void resizedwRSSI(int width, int height);    ///Resize RSSI widget
   void resizecdw(int width, int height);       ///Resize classifier display widget

};

#endif // MAINWINDOW_H
