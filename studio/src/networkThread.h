/**
 * networkThread.h
 * Data management and processing in WirelessEye. This file coordinates the entire dataflow.
 *
 *  Nov. 2020, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#ifndef _NETWORK_THREAD_H
#define _NETWORK_THREAD_H
#define UDP_PORT 5500                           ///The UDP port of Nexmon. Only used when we directly stream data from the Raspberry without the CSIServer (and hence WirelessEye directly runs on the Raspi).
#define CSI_PORT 5501                           ///The port of the CSI Server from which we obtain the data
#define HEADER_OFFSET 18                        ///18 bytes of a packet belong to the header
#define RCV_BUF_LEN 4*256+HEADER_OFFSET+16      ///80 MHZ channel has 256 samples a 4 byte. Then we need HEADER_OFFSET for the header from nexmon and 16 bytes fo the timestamp. + 100 just for safety
#define FILEBUF_LEN (10*1024)                   ///The length of the buffer to write data into a file. This should exceed the size of the CSI-rleated data belonging to one WiFi frame
#define CSI_CONTAINS_RSSI true                  ///If the Nexmon has been additionally pateched (see README.md) to also provide RSSI, then set this to true.
#define DATA_EXCHANGE_THROUGH_QT_SIGNALS false  ///If true, we don't directly call functions belonging to another thread but excessively use QT signals instead. No reason to do this in the current version of WirelessEye, since this will hamper the performance.
#define DIFFERENT_MACS_IN_FILTER_FOR_DISPLAY_AND_LIVE_EXPORT true       ///Support different MACS in the filter plugins for live export and for displaying. This is realized by adding an additional byte to the MAC, which indicates
                                                                        ///whether a filter is called for displaying or for live export. If this is disactivated, all filters that treat the input as a time series (e.g., exponential smoothing) get disturbed by being called twice in a row for the same MAC.
                                                                        ///Only disadvantage of activating this: The MAC address the filters ``see'' is not the actual MAC, since one additional byte is appended.

#include <QThread>
#include <inttypes.h>
#include <QString>
#include <QObject>
#include <QTcpSocket>
#include "CSIData.h"
#include <QUdpSocket>
#include <QFile>
#include <sys/time.h>
#include "CSIFilterManager.h"
#include <QStringList>

/**
 * Struct timespec has a platform-dependent length. We always use the 16-byte-version and hence define it explicitly here.
 */
struct timespec_16bytes{
  uint64_t tv_sec;
  uint64_t tv_nsec;
};

class MainWindow;

/**
 * This class implements the entire data management and processing in WirelessEye, i.e., streaming from the Raspi,
 * MAC filtering, splitting the data into amplitude and phase, executing the filter pipeline, streaming data to files, to display widgets and to the classifierWRThread for real-time export.
 */
class networkThread: public QObject{
  Q_OBJECT

  private:
  char buf[RCV_BUF_LEN];                        ///A buffer for storing the data initially read from the socket
  QString host;                                 ///String containing the hostname of the Rapsi
  MainWindow* mw;                               ///Pointer to the main window
  bool status;                                  ///true => we are connected to the Nexmon firmware. false otherwise.
  QTcpSocket* s;                                ///A socket for contacting the CSI server
  QUdpSocket* s_udp;                            ///A UDP socket for directly contacting the Nexmon firmware, if we directly run on a Raspi
  QFile* file;                                  ///A file to record data to
  uint32_t nBytesRead;                          ///Number of bytes read
  struct tm timeNowLocal;                       ///Timestamp on this machine
  struct timespec timeNow;                      ///A timestamp in the local struct timespec format
  struct timespec_16bytes timeNow16;            ///A timestamp that is always in a 16 bit timespec format
  QStringList MACFilterList;                    ///A list of allowed (e.g., non-filtered) MACs
  bool MACFilterRecording;                      ///Use MAC filtering for recodring files
  bool MACFilterLiveExport;                     ///Use MAC filtering for live export
  bool classifierThreadActive;                  ///True, if the classifier is running. False, otherwise
  uint32_t CSIDataLen;                          ///Number of subcarriers in input data
  uint32_t CSIDataLenDisplay;                   ///Number of subcarriers in data for displaying
  uint32_t CSIDataLenExport;                    ///Number of subcarriers in data for export (recodring + liveExport)
  CSIFilterManager* filterManager;              ///The filter manager controls all preprocessing plugins
  bool recording;                               ///True, if we are currently recording to a file-
  bool displayAmplitude;                        ///True, if we are currently displaying the CSI amplitude
  bool displayPhase;                            ///True, if we are currently displaying the CSI phase
  bool displayRSSI;                             ///True, if we are currently displaying the RSSI
  bool displayClassifier;                       ///True, if we are currently displaying the classifier output
  bool UDPStreaming;                            ///True, when the data comes directly from Nexmon using UDP (i.e., when we directly run the Raspi). False, if TCP streaming is selected.

  /**
   *  Do all data management and processing. This is a large procedure. Needs to be refined and separated into different entities in the future.
   *
   */
  bool processData(char* buf, struct timespec timeNow);

  public:
  networkThread();
  ~networkThread();

  /**
   * Sets IP address or hostname of the Raspi
   */
  void setAddr(const QString &addr);

  /**
   * Returns the status - which is true, when connected to the Raspi, false otherwise
   */
  bool getStatus();

  /**
   * Set pointer to the main window. Do this before doing anything else.
   */
  void setMainWindow(MainWindow* mw);

  /**
   * Start recording data into a file
   */
  void startRecording();

  /**
   * Stop recording data into a file
   */
  void stopRecording();

  /**
   * Stop streaming and finalize the network thread.
   */
  void stopAndDestroy();

  /**
   * Query if some MAC address is on the list of non-filtered MAC addresses
   */
  bool isMACActive(QString MAC);


  signals:
  void streamingStartedStopped(bool started);           ///Streaming has been started (started == true) or stopped (stared == false)
  void finished();                                      ///Streaming has ended
  void addTimeToCDW();                                  ///Add one unit of time to the classifier display widget. One time unit is the reception of one single WiFi frame.
  void addDataToDisplayWidget(double);                  ///Send data of a single subcarrier to amplitude display widget. This mechanism is only used if DATA_EXCHANGE_THROUGH_QT_SIGNALS==true. Otherwise, a direct function call is used instead of a QT signal.
  void addDataToPhaseDisplayWidget(double);             ///Send data of a single subcarrier to phase display widget. This mechanism is only used if DATA_EXCHANGE_THROUGH_QT_SIGNALS==true. Otherwise, a direct function call is used instead of a QT signal. It's more performant to always send the data of an entire frame instead (see below).
  void addDataToRSSIDisplayWidget(double);              ///Send data of a single subcarrier  to display widget. This mechanism is only used if DATA_EXCHANGE_THROUGH_QT_SIGNALS==true. Otherwise, a direct function call is used instead of a QT signal. It's more performant to always send the data of an entire frame instead (see below).
  void addDataArrayToDisplayWidget(double*, int);       ///Send the data of an entire frame to amplitude display widget. This mechanism is only used if DATA_EXCHANGE_THROUGH_QT_SIGNALS==true. Otherwise, a direct function call is used instead of a QT signal. It's more performant to always send the data of an entire frame instead (see below).
  void addDataArrayToPhaseDisplayWidget(double*, int);  ///Send the data of an entire frame to phase display widget. This mechanism is only used if DATA_EXCHANGE_THROUGH_QT_SIGNALS==true. Otherwise, a direct function call is used instead of a QT signal. It's more performant to always send the data of an entire frame instead (see below).
  void addDataToClassifierThread(const QString&);       ///Send data to the classifier thread. The string sent should be in "simple" .csv format.
  void addMAC(QString);                                 ///Add a certain MAC address to the list of known mMACS

  public slots:

  /**
   * Handle readyRead() of the UDP or TCP socket. This means that new data is available at the socket, which neads to be read.
   */
  void readyRead();

  /**
   * Initiate the stop of data streaming from the Raspi.
   */
  void stop();

  /**
   * Start streaming data from the Raspi
   */
  void operate();

  /**
   * Set a pointer to the filter manager that handles the processing filter plugins.
   */
  void setFilterManager(CSIFilterManager* manager);

  /**
   * Activate/deactivate MAC filtering for recording.
   */
  void setMACFilterRecording(bool active);

  /**
   * Activate/deactivate MAC filtering for live export.
   */
  void setMACFilterLiveExport(bool active);

  /**
   * Add "MAC" to the list of MAC addresses to be considered for displaying/export.
   */
  void addMACToFilter(QString MAC);

  /**
   * Reset the list of MAC addresses to be considered for displaying/export.
   */
  void resetMACFilter(QString MAC);

  /**
   * Set a list of MAC addresses to be considered for displaying/export.
   */
  void setMACFilterList(QStringList filters);

  /**
   * Notify the networkThread if the classifierThread (i.e., the thread reading the input from the classfier) is active.
   */
  void setClassifierThreadActive(bool active);

  /**
   * Set the number of subcarriers in the input data.
   */
  void setNCSISamples(uint32_t NCSISamples);

  /**
   * Set the number of subcarriers to display.
   */
  void setNCSISamplesDisplay(uint32_t NCSISamples);

  /**
   * Set the number of subcarriers to export.
   */
  void setNCSISamplesExport(uint32_t NCSISamples);

  /**
   * Notfiy the network thread that amplitude displaying has been activated. If active==true, then data will be streamed to the amplitude display widget.
   */
  void setDisplayAmplitude(bool active);

  /**
   * Notfiy the network thread that phase displaying has been activated. If active==true, then data will be streamed to the phase display widget.
   */
  void setDisplayPhase(bool active);

  /**
   * Notfiy the network thread that RSSI displaying has been activated. If active==true, then data will be streamed to the RSSI display widget.
   */
  void setDisplayRSSI(bool active);

  /**
   * Notfiy the network thread that classification output displaying has been activated. If active==true, then data will be streamed to the classification output display widget.
   */
  void setDisplayClassifier(bool active);
};



#endif
