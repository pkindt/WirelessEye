/**
 * networkThread.cpp
 * This file implements the data management and processing in WirelessEye, i.e., streaming from the Raspi,
 * MAC filtering, splitting the data into amplitude and phase, executing the filter pipeline, exporting to files, to display widgets and to the classifierWRThread for real-time classification.
 *
 *
 *  Nov. 2020, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <stdio.h>
#include <dlfcn.h>
#include "debug.h"
#include "displayWidget.h"
#include "mainwindow.h"
#include "CSIData.h"
#include "classifierThread.h"
#include "CSIFilterManager.h"
#include "networkThread.h"

#define CLASSIFIER_ACCUM_BUF_LEN CLASSIFIER_RCV_BUF_LEN
//#define DEBUG(...) printf(__VA_ARGS__)
#define DEBUG(...)
using namespace std;

networkThread::networkThread(){
  status = false;
  recording = false;
  s_udp = NULL;
  s = NULL;
  nBytesRead = 0;
  MACFilterLiveExport = true;
  MACFilterRecording = true;
  classifierThreadActive = false;
  filterManager = NULL;
  CSIDataLen = 0;
  CSIDataLenDisplay = 0;
  CSIDataLenExport = 0;
  displayAmplitude = false;
  displayPhase = false;
  displayRSSI = false;
  displayClassifier = false;
}

networkThread::~networkThread(){
  // printf("-DESTROY %x-\n",this->thread()->currentThreadId());
  if(status){
    status = false;
  }
  if(s_udp != NULL){
    s_udp->moveToThread(this->thread());
    s_udp->abort();
  }
  if(s != NULL){
    s->moveToThread(this->thread());
    s->abort();
  }
  delete s_udp;
  delete s;
  // printf("-DESTROYED %x-\n",this->thread()->currentThreadId());

}


/**
 * Start streaming data from the Raspi
 */
void networkThread::operate(){
  //  cout<<"initiating.. "<<endl;
  connect(this, SIGNAL(addTimeToCDW()),this->mw->getCDW(),SLOT(addTime()));
  connect(this, SIGNAL(addDataToClassifierThread(const QString&)), this->mw->getCT(), SLOT(addData(const QString&)));
  connect(this, SIGNAL(addDataToRSSIDisplayWidget(double)), this->mw->getdwRSSI(), SLOT(addData(double)));
  connect(this, SIGNAL(addDataArrayToDisplayWidget(double*, int)), this->mw->getdwA(), SLOT(addDataForEntireFrame(double*, int)));
  connect(this, SIGNAL(addDataArrayToPhaseDisplayWidget(double*, int)), this->mw->getdwP(), SLOT(addDataForEntireFrame(double*, int)));

  int16_t nBytes;
  char buf[RCV_BUF_LEN];
  UDPStreaming = mw->getUI()->rbConnectionUDP->isChecked();

  s_udp = new QUdpSocket(this);
  s = new QTcpSocket(this);
  nBytesRead = 0;
  /* Open Socket - UDP or TCP */
  if(UDPStreaming){
    if(!s_udp->bind(QHostAddress::Any,UDP_PORT)){
      cout<<"BIND failed."<<endl;
      emit streamingStartedStopped(false);
      emit finished();
      stopAndDestroy();
      return;
    }
    connect(s_udp, SIGNAL(readyRead()),this,SLOT(readyRead()));
    connect(s_udp, SIGNAL(disconnected()),this,SLOT(stop()));

  }else{
    connect(s,SIGNAL(readyRead()),this,SLOT(readyRead()));
    connect(s,SIGNAL(disconnected()),this,SLOT(stop()));
    s->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    s->connectToHost(host,CSI_PORT);
    if(!s->waitForConnected(500)){
      cout<<"Failed to connect"<<endl;
      emit streamingStartedStopped(false);
      emit finished();
      stopAndDestroy();
      return;
    }

  }
  cout<<"connected"<<endl;
  status = true;
  emit streamingStartedStopped(true);


}

/**
 * Handle readyRead() of the UDP or TCP socket. This means that new data is available at the socket, which neads to be read.
 */
void networkThread::readyRead(){
  static uint32_t cnt = 0;

  static struct timespec timeNow;
  static struct timespec_16bytes timeNow16;
  uint32_t bytesToRead = CSIDataLen*4+18 + sizeof(struct timespec_16bytes);
  int32_t bytesReadThis;
  /*
  if(!status){
    return;
  }
   */
  if(UDPStreaming){
    do{
      nBytesRead = s_udp->readDatagram(buf,CSIDataLen*4+18);

      clock_gettime(CLOCK_REALTIME,&timeNow);
      DEBUG("read %lli bytes\n",(int64_t)nBytesRead);
      if(!processData(buf,timeNow)){
        cout<<"Data Processing has failed."<<endl;
        exit(1);
        return;;
      }
    }while (s_udp->hasPendingDatagrams());

    return;
  }else{
    do{
      bytesReadThis = s->read(buf + nBytesRead,bytesToRead-nBytesRead);
      if(bytesReadThis==0){
        printf("error reading from socket\n");
        exit(1);
      }
      if(bytesReadThis < 0){
        printf("error reading from socket\n");
        exit(1);
      }

      nBytesRead += bytesReadThis;
      if(nBytesRead > bytesToRead){
        printf("Something is wrong - read more than bytes than needed. Read: %u - should be: %u\n",nBytesRead,bytesToRead);
        exit(1);
      }
      if(nBytesRead == bytesToRead){
        cnt = cnt + 1;

        nBytesRead = 0;

        /*
     for(uint32_t i = 0; i < nBytesRead; i++){
       printf("%u -> %x\n",i,buf[i]);
     }
         */

        timeNow16 = *((struct timespec_16bytes*) buf);
        timeNow.tv_sec = be64toh(timeNow16.tv_sec);
        timeNow.tv_nsec = be64toh(timeNow16.tv_nsec);
        DEBUG("read %lli bytes\n",(int64_t)nBytesRead);
        if(!processData(buf+sizeof(struct timespec_16bytes),timeNow)){
          cout<<"Data Processing has failed."<<endl;
          stop();
          return;;
        }
      }
    }while(s->bytesAvailable() >= bytesToRead);
  }
}

bool networkThread::processData(char* buf, struct timespec timeNow){
  static CSIData data_Display;                  //Data to show in visualisation
  static CSIData data_Export;                   //Data to export to Files/Classifier
  static QString MACStr;                        //String buffer for MAC addresses
  static char MACBuf[50];                       //Char buffer for MAC addresses
  static int16_t real;                          //Real part of CSI
  static int16_t imag;                          //Imaginary part of CSI
  static double magnitude;                      //CSI magnitude
  static double phase;                          //CSI phase
  static char timestamp[100];                   //Buffer for timestamp in string format
  static char fileBuf_CT_accum_Recording[CLASSIFIER_ACCUM_BUF_LEN];     //Accumulated filebuffer for recording - an entry for the recorded file will be prepared in memory here
  uint32_t wrPointerfileBuf_CT_accum_Recording = 0;                     //Write pointer for this file buffer
  static char fileBuf_CT_accum_LiveExport[CLASSIFIER_ACCUM_BUF_LEN];    //Accumulated filebuffer for live recording
  uint32_t wrPointerfileBuf_CT_accum_LiveExport = 0;                    //Write pointer for this file buffer
  static char fileBuf_Record[FILEBUF_LEN];                              //Buffer for temp data for recording
  static char fileBuf_LiveExport[FILEBUF_LEN];                          //Buffer for temp data for live export
  static double exchangeBuf_amplitudes[256];                            //Data buffer for exchaning data with the display widgets
  static double exchangeBuf_phases[256];                                //Data buffer for exchaning data with the display widgets
  static struct timespec_16bytes timeNow16;                             //Timespec function

  //fill timespec with current time
  timeNow16.tv_sec = timeNow.tv_sec;
  timeNow16.tv_nsec = timeNow.tv_nsec;

  //initialize buffers for recording/live export
  strcpy(fileBuf_CT_accum_Recording,"");
  strcpy(fileBuf_CT_accum_LiveExport,"");
  uint32_t strlen_filebuf;


  DEBUG("processing.\n");

  #if CSI_CONTAINS_RSSI
  // We expect the data from the Rapberry Pi to contain RSSI. The code below
  // prints soem debug data and then inserts it into the data_Display structure
  if((buf[0] != 0x11)||(buf[1]!=0x11)){
    uint32_t numbytes = 4*CSIDataLen + 18+16;
    for(uint32_t i = 0; i < numbytes/10; i++){
      printf("%x|%x|%x|%x|%x|%x|%x|%x|%x|%x\n",(uint8_t) buf[10*i],(uint8_t)buf[10*i+1],(uint8_t)buf[10*i+2],(uint8_t)buf[10*i+3],(uint8_t)buf[10*i+4],(uint8_t)buf[10*i+5],(uint8_t)buf[10*i+6],(uint8_t)buf[10*i+7],(uint8_t)buf[10*i+8],(uint8_t)buf[10*i+9]);
    }
    for(uint32_t i = (numbytes/10)*10; i < numbytes; i++){
      printf("%x|",(uint8_t) buf[i]);
    }
    printf("\n");

    cout<<"Does not appear to be CSI data containing RSSI - magic value missing. Dropping frame."<<endl;
    printf("%x %x\n", buf[0], buf[1]);
    return false;                        //false will cause the connection to abort.
  }
  data_Display.RSSI = (double)((int8_t) buf[2]);
  data_Display.frame_control = buf[3];

#else
  // We don't expect RSSI data
  if((buf[0] != 0x11)||(buf[1]!=0x11)||(buf[2]!= 0x11)||(buf[3]!=0x11)){
    cout<<"Does not appear to be CSI data - magic value missing."<<endl;
    printf("%x %x %x %x\n", buf[0], buf[1],buf[2],buf[3]);
    return false;
  }
  data_Display.RSSI = 0;
  data_Display.frame_control = 0;

#endif


  //Create a timestamp string
  gmtime_r(&(timeNow.tv_sec),&timeNowLocal);
  sprintf(timestamp, "%04u-%02u-%02u %02u:%02u:%02u:%06u"             //format specified by Florenc
          ,timeNowLocal.tm_year + 1900
          ,timeNowLocal.tm_mon+1
          ,timeNowLocal.tm_mday
          ,timeNowLocal.tm_hour+1
          ,timeNowLocal.tm_min
          ,timeNowLocal.tm_sec
          ,timeNow.tv_nsec/1000);
  data_Display.timeStamp = timeNowLocal;
  DEBUG("Timestamp: %s\n", timestamp);


  //Create
  memcpy(data_Display.senderMAC, (buf+4), 6);
  DEBUG("MAC: %c:%c:%c:%c:%c:%x\n", data_Display.senderMAC[0],data_Display.senderMAC[1],data_Display.senderMAC[2],data_Display.senderMAC[3],data_Display.senderMAC[4],data_Display.senderMAC[5]);
  sprintf(MACBuf,"%x:%x:%x:%x:%x:%x", data_Display.senderMAC[0],data_Display.senderMAC[1],data_Display.senderMAC[2],data_Display.senderMAC[3],data_Display.senderMAC[4],data_Display.senderMAC[5]);
  MACStr = MACBuf;                      //Create QString from character array

  emit addMAC(QString(MACStr));         //Add MAC to the list of known MACs


  //Fill remaining parts of data_Display and data_Export fields
  DEBUG("a -> %d, b->%d\n",buf[10],buf[11]);
  data_Display.seqNr = (((uint16_t) buf[10])<<8)|(((uint16_t) buf[11]));
  DEBUG("%u %u\n",(uint8_t) buf[10],(uint8_t) buf[11]);
  DEBUG("seqNr: %u\n",data_Display.seqNr);
  data_Display.streamNr = ((uint8_t)(buf[12]))|((uint8_t) (buf[13])<<8);
  DEBUG("streamNr = %u\n",data_Display.streamNr);
  data_Display.chanSpec = ((uint8_t)(buf[14]))|((uint8_t) (buf[15])<<8);
  DEBUG("chanSpec = %x\n",data_Display.chanSpec);
  data_Display.chipVersion = ((uint8_t)(buf[16]))|((uint8_t) (buf[17])<<8);
  DEBUG("chipVersion = %u\n",data_Display.chipVersion);
  int16_t* payloadPointer = (int16_t*) (buf + 18);

  data_Display.nSubCarriers_orig = CSIDataLen;
  data_Export.nSubCarriers_orig = CSIDataLen;
  data_Export.timeStamp = data_Display.timeStamp;
  memcpy(data_Export.senderMAC,data_Display.senderMAC,6);
  data_Export.RSSI = data_Display.RSSI;
  data_Export.frame_control = data_Display.frame_control;
  data_Export.seqNr = data_Display.seqNr;
  data_Export.streamNr = data_Display.streamNr;
  data_Export.chanSpec = data_Display.chanSpec;

  data_Export.chipVersion = data_Display.chipVersion;
  if(CSIDataLenDisplay > CSIDataLen){
    data_Display.nSubCarriers = CSIDataLen;
  }else{
    data_Display.nSubCarriers = CSIDataLenDisplay;
  }
  if(CSIDataLenExport > CSIDataLen){
    data_Export.nSubCarriers = CSIDataLen;
  }else{
    data_Export.nSubCarriers = CSIDataLenExport;
  }


  uint32_t beginD, endD;
  uint32_t beginE, endE;


  //reduce bandwidth if the bandwidth using which CSI has been captured
  if(CSIDataLen == 256){
    if(CSIDataLenDisplay == 64){
      //80 MHz => 20MHz
      beginD = 128;
      endD = 191;
    }else if(CSIDataLenDisplay == 128){
      //80 MHz => 20MHz
      beginD = 128;
      endD = 255;
    }else{
      beginD = 0;
      endD = 255;
    }

    if(CSIDataLenExport== 64){
      //80 MHz => 20MHz
      beginE = 128;
      endE = 191;
    }else if(CSIDataLenExport== 128){
      //80 MHz => 20MHz
      beginE = 128;
      endE = 255;
    }else{
      beginE = 0;
      endE = 255;
    }

  }
  if(CSIDataLen == 128){
    if(CSIDataLenDisplay == 64){
      //80 MHz => 20MHz
      beginD = 64;
      endD = 127;
    }else{
      beginD = 0;
      endD = 127;
    }

    if(CSIDataLenExport== 64){
      //80 MHz => 20MHz
      beginE = 64;
      endE = 127;
    }else{
      beginE = 0;
      endE = 127;
    }
  }

  if(CSIDataLen == 64){
    beginD = 0;
    endD = 127;
    beginE  = 0;
    endE = 127;
  }
  uint32_t begin, end;
  if(beginD > beginE){
    begin = beginE;
  }else{
    begin = beginD;
  }
  if(endD > endE){
    end = endD;
  }else{
    end = endE;
  }

  //Compute amplitude and phase and fill it into data_Display and data_Export structures
  for(uint16_t cnt = begin; cnt <= end; cnt++){
    real =  payloadPointer[2*cnt + 0];
    imag =  payloadPointer[2*cnt + 1];
    magnitude = sqrt((((double) real)*((double) real)) + (((double) imag)*((double) imag)));
    phase = atan2(double(imag),(double) real);
    if((cnt >= beginD)&&(cnt <= endD)){
      data_Display.amplitude[cnt - beginD] = magnitude;
      data_Display.phase[cnt - beginD] = phase;
    }
    if((cnt >= beginE)&&(cnt <= endE)){
      data_Export.amplitude[cnt - beginE] = magnitude;
      data_Export.phase[cnt - beginE] = phase;
    }
  }

  //Filters obtain both data_Display and data_Export in an alternating manner.
  //To allow a filter to distinguish between "having been called for display" and "having been called for export",
  //We extend the MAC address by 1 byte and hence allow the filter to distinguish.
  //Reason: Many filter plugins use time-series methods and observe different. They store the "memory" of e.g., an exponentialeach MAC
  //smoothing-based filter individually for each MAC. Calling them twice for the same MAC would disturb the filter. In this way, the filter
  //sees two different MACs and does not get disturbed.
#if DIFFERENT_MACS_IN_FILTER_FOR_DISPLAY_AND_LIVE_EXPORT
  data_Display.senderMAC[6] = 0;
  data_Export.senderMAC[6] = 1;
#else
  data_Display.senderMAC[6] = 0;
  data_Export.senderMAC[6] = 0;

#endif


  /* Apply filter pipeline */
  if(filterManager != NULL){
    filterManager->applyFilterPipeline(&data_Display);
    filterManager->applyFilterPipeline(&data_Export);
  }





  /*
   * Preparation of per-frame (and not per sub-carrier) information for recording. Per frame data is normally data such as e.g., the timestamp.
   */
  if(recording){
    if(this->mw->getUI()->rbFileFormatCSVCompact->isChecked()){
      //** Compact CSV Format**//

      //Compact format. Only add the data unique per frame
      sprintf(fileBuf_Record,"%s;%s;%.10f;%u",timestamp,MACBuf,data_Export.RSSI,data_Export.frame_control);
      if(wrPointerfileBuf_CT_accum_Recording + strlen(fileBuf_Record)+1 >= FILEBUF_LEN){
        cerr<<"Warning - File buffer is full. Please report this as a bug. Exiting.\n";
        exit(1);
      }

      memcpy((char*)(fileBuf_CT_accum_Recording+wrPointerfileBuf_CT_accum_Recording), fileBuf_Record,strlen(fileBuf_Record)+1);
      wrPointerfileBuf_CT_accum_Recording += strlen(fileBuf_Record);


    }else if(this->mw->getUI()->rbFileFormatBinary->isChecked()){
      //** Binary Format**//

      //timestamp
      if(wrPointerfileBuf_CT_accum_Recording + sizeof(struct timespec_16bytes) >= FILEBUF_LEN){
        cerr<<"Warning - File buffer is full. Please report this as a bug. Exiting.\n";
        exit(1);
      }
      memcpy((char*)(fileBuf_CT_accum_Recording+wrPointerfileBuf_CT_accum_Recording), (char*) &(timeNow16),sizeof(struct timespec_16bytes));
      wrPointerfileBuf_CT_accum_Recording += sizeof(struct timespec_16bytes);

      //MAC
      if(wrPointerfileBuf_CT_accum_Recording + 6 >= FILEBUF_LEN){
        cerr<<"Warning - File buffer is full. Please report this as a bug. Exiting.\n";
        exit(1);
      }
      memcpy((char*)(fileBuf_CT_accum_Recording+wrPointerfileBuf_CT_accum_Recording), (char*) &(data_Export.senderMAC), 6);  //6 bytes are the actual MAC, data_Export.senderMAC contains an additional byte to distinguish between export and displaying
      wrPointerfileBuf_CT_accum_Recording += 6;                 //6 bytes are the actual MAC, data_Export.senderMAC contains an additional byte to distinguish between export and displaying

      //RSSI
      if(wrPointerfileBuf_CT_accum_Recording +sizeof(data_Display.RSSI) >= FILEBUF_LEN){
        cerr<<"Warning - File buffer is full. Please report this as a bug. Exiting.\n";
        exit(1);
      }
      memcpy((char*)(fileBuf_CT_accum_Recording+wrPointerfileBuf_CT_accum_Recording), (char*) &(data_Export.RSSI),sizeof(data_Display.RSSI));
      wrPointerfileBuf_CT_accum_Recording += sizeof(data_Export.RSSI);

      //frame control
      if(wrPointerfileBuf_CT_accum_Recording +sizeof(data_Export.frame_control) >= FILEBUF_LEN){
        cerr<<"Warning - File buffer is full. Please report this as a bug. Exiting.\n";
        exit(1);
      }
      memcpy((char*)(fileBuf_CT_accum_Recording+wrPointerfileBuf_CT_accum_Recording), (char*) &(data_Export.frame_control),sizeof(data_Display.frame_control));
      wrPointerfileBuf_CT_accum_Recording += sizeof(data_Export.frame_control);
    }

    //** Simple CSV Format**//
    //-> data will only be created per-subcarrier

  }

  //Preparation of  per sub-carrier information for recording.
  //The data in fileBuf_* will be later added to wrPointerfileBuf_CT_accum_Recording/wrPointerfileBuf_CT_accum_LiveExport


  for(uint16_t cnt = 0; cnt < CSIDataLenExport; cnt++){

    //for live export + simple CSV, which share the same format
    sprintf(fileBuf_LiveExport,"%s;%s;%d;%.10f;%.10f;%.10f;%u\n",timestamp,MACBuf,cnt,data_Export.amplitude[cnt],data_Export.phase[cnt],data_Export.RSSI, data_Export.frame_control);


    if(recording){

      if(this->mw->getUI()->rbFileFormatCSVSimple->isChecked()){
        //** Simple CSV Format**//
        //use the 'simple' format as for live-export

        //since both buffers have the same size, this cannot fail
        memcpy(fileBuf_Record,fileBuf_LiveExport,strlen(fileBuf_LiveExport)+1);

      }else if(this->mw->getUI()->rbFileFormatCSVCompact->isChecked()){
        //** Compact CSV Format**//

        if(cnt == CSIDataLenExport-1){
          //with newline
          sprintf(fileBuf_Record,";%.10f;%.10f\n",data_Export.amplitude[cnt],data_Export.phase[cnt]);
        }else{
          //no newline
          sprintf(fileBuf_Record,";%.10f;%.10f",data_Export.amplitude[cnt],data_Export.phase[cnt]);
        }
      }else{
        //** Binary Format**//
        if(wrPointerfileBuf_CT_accum_Recording +sizeof(data_Export.amplitude[cnt]) >= FILEBUF_LEN){
          cerr<<"Warning - File buffer is full. Please report this as a bug. Exiting.\n";
          exit(1);
        }
        memcpy((char*)(fileBuf_CT_accum_Recording+wrPointerfileBuf_CT_accum_Recording), (char*) &(data_Export.amplitude[cnt]),sizeof(data_Export.amplitude[cnt]));
        wrPointerfileBuf_CT_accum_Recording += sizeof(data_Export.amplitude[cnt]);

        if(wrPointerfileBuf_CT_accum_Recording + sizeof(data_Export.phase[cnt]) >= FILEBUF_LEN){
          cerr<<"Warning - File buffer is full. Please report this as a bug. Exiting.\n";
          exit(1);
        }
        memcpy((char*)(fileBuf_CT_accum_Recording+wrPointerfileBuf_CT_accum_Recording), (char*) &(data_Export.phase[cnt]),sizeof(data_Export.phase[cnt]));
        wrPointerfileBuf_CT_accum_Recording += sizeof(data_Export.phase[cnt]);
        strcpy(fileBuf_Record,"");
      }
    }

    //Live Export
    if(classifierThreadActive){
      if((!MACFilterLiveExport)||(isMACActive(MACStr))){
        strlen_filebuf = strlen(fileBuf_LiveExport);
        if(wrPointerfileBuf_CT_accum_LiveExport + strlen_filebuf + 1 <= CLASSIFIER_ACCUM_BUF_LEN){
          memcpy((char*)(fileBuf_CT_accum_LiveExport+wrPointerfileBuf_CT_accum_LiveExport), fileBuf_LiveExport,strlen_filebuf+1);
          wrPointerfileBuf_CT_accum_LiveExport += strlen_filebuf;
        }else{
          printf("err - buffer for classifier thread overfull\n");
          exit(1);
        }
      }
    }

    //Recording
    if(recording){
      // Reasons not to add data related to this frame to the final buffer to be written into the file
      // 1) The MAC filter is active and the selected MAC is not included, or,
      // 2) we are in binary format - the buffer is already filled in this case
      if(!(this->mw->getUI()->rbFileFormatBinary->isChecked())){
        if((!MACFilterRecording)||(isMACActive(MACStr))){
          strlen_filebuf = strlen(fileBuf_Record);
          //     printf("filter: %u - found: %u -  adding :%s\n",MACFilterRecording,isMACActive(MACStr), MACStr.toUtf8().data());
          if(wrPointerfileBuf_CT_accum_Recording + strlen_filebuf + 1 <= CLASSIFIER_ACCUM_BUF_LEN){
           if(strlen_filebuf > 0){
              memcpy((char*)(fileBuf_CT_accum_Recording+wrPointerfileBuf_CT_accum_Recording), fileBuf_Record,strlen_filebuf+1);
              wrPointerfileBuf_CT_accum_Recording += strlen_filebuf;
           }
          }else{
            printf("err - filebuf overfull - %u/%u bytes written\n", wrPointerfileBuf_CT_accum_Recording + strlen_filebuf + 1 ,CLASSIFIER_ACCUM_BUF_LEN);
            exit(1);
          }

        }
      }
    }
  }

  //Export time for classifier
  if(classifierThreadActive){
    if(((!MACFilterLiveExport)||(isMACActive(MACStr)))&&(displayClassifier)){
#if DATA_EXCHANGE_THROUGH_QT_SIGNALS
      emit addTimeToCDW();
#else
      this->mw->getCDW()->addTime();
#endif
    }
  }


  //RSSI to RSSI display widget
#ifdef CSI_CONTAINS_RSSI
  if((isMACActive(MACStr))&&(displayRSSI)){
#if DATA_EXCHANGE_THROUGH_QT_SIGNALS
    emit addDataToRSSIDisplayWidget((double) data_Display.RSSI);
#else
    this->mw->getdwRSSI()->addData(data_Display.RSSI);
#endif
  }
#endif


  //display amplitude and phase
  if(isMACActive(MACStr)){
    memcpy(exchangeBuf_amplitudes, data_Display.amplitude, CSIDataLenDisplay*sizeof(double));
    memcpy(exchangeBuf_phases, data_Display.phase, CSIDataLenDisplay*sizeof(double));
#if DATA_EXCHANGE_THROUGH_QT_SIGNALS
    if(displayAmplitude){
      emit addDataArrayToDisplayWidget(exchangeBuf_amplitudes,CSIDataLenDisplay);
    }
    if(displayPhase){
      emit addDataArrayToPhaseDisplayWidget(exchangeBuf_phases,CSIDataLenDisplay);
    }
#else
    if(displayAmplitude){
      this->mw->getdwA()->addDataForEntireFrame(exchangeBuf_amplitudes,CSIDataLenDisplay);
    }
    if(displayPhase){
      this->mw->getdwP()->addDataForEntireFrame(exchangeBuf_phases,CSIDataLenDisplay);
    }
#endif


    // Export to classifier
    if((classifierThreadActive)&&(wrPointerfileBuf_CT_accum_LiveExport > 0)){
#if DATA_EXCHANGE_THROUGH_QT_SIGNALS
      emit addDataToClassifierThread(QString(fileBuf_CT_accum_LiveExport));
#else
      this->mw->getCT()->addData(fileBuf_CT_accum_LiveExport);
#endif
    }
  }


  //do the actual recodging

  if((recording)&&(wrPointerfileBuf_CT_accum_Recording > 0)){
    if((!MACFilterRecording)||(isMACActive(MACStr))){

      if(file->write(fileBuf_CT_accum_Recording, wrPointerfileBuf_CT_accum_Recording)<=0){
        cout<<"Error writing file"<<endl;;
        this->stopRecording();
        return false;
      }
    }
    wrPointerfileBuf_CT_accum_Recording = 0;
  }

  return true;
}


/**
 * Sets IP address or hostname of the Raspi
 */
void networkThread::setAddr(const QString &addr){
  cout<<"Address: "<<addr.toUtf8().data()<<endl;
  host = addr;
}

/**
 * Returns the status - which is true, when connected to the Raspi, false otherwise
 */
bool networkThread::getStatus(){
  return status;
}

/**
 * Stop streaming and finalize the network thread.
 */
void networkThread::stopAndDestroy(){
  stop();
  this->~networkThread();
}

/**
 * Initiate the stop of data streaming from the Raspi.
 */
void networkThread::stop(){
  if(status){
    status = false;
    disconnect(this,SLOT(stop()));
    // finish will trigger QThread::quit(), which will call the destructor of this class within the right thread context.
    // hence, open sockets are
    emit streamingStartedStopped(false);
    emit finished();
  }
}


/**
 * Set pointer to the main window. Do this before doing anything else.
 */
void  networkThread::setMainWindow(MainWindow* mw){
  this->mw = mw;
}

/**
 * Start recording data into a file
 */
void networkThread::startRecording(){
  if(!status){
    cout<<"cannot start recording - no connection to CSI Server\n"<<endl;
    return;
  }
  QString filename;
  if(mw->getUI()->rbFilenameStatic->isChecked()){
    filename = mw->getUI()->leStaticFilename->text();

  }else{
    filename = "CSI_";
    filename.append(QDate::currentDate().toString("MMMM_d_yy"));
    filename.append("_");
    filename.append(QTime::currentTime().toString());
    if(this->mw->getUI()->rbFileFormatBinary->isChecked()){
      filename.append(".wbin");
    }else{
      filename.append(".csv");
    }
  }

  file = new QFile(filename);
  char header[5000];
  char tmp1[20], tmp2[20];
  if(this->mw->getUI()->rbFileFormatCSVSimple->isChecked()){
    strcpy(header,"timestamp;MAC;subcarrier;amplitude;phase;RSSI;frame_control\n");
    cout<<"Recording to file '"<<filename.toUtf8().data()<<"' in simple CSV format"<<endl;
  }else if(this->mw->getUI()->rbFileFormatCSVCompact->isChecked()){
    strcpy(header,"timestamp;MAC;RSSI;frame_control");
    for(uint16_t cnt = 0; cnt < CSIDataLenExport; cnt++){
      sprintf(tmp1, ";a%u",cnt);
      sprintf(tmp2, ";p%u",cnt);
      strcat(header,tmp1);
      strcat(header,tmp2);
    }
    sprintf(tmp1, "\n");
    strcat(header,tmp1);
    cout<<"Recording to file '"<<filename.toUtf8().data()<<"' in compact CSV format"<<endl;
  }else{
    strcpy(header,"WifEyeBinary");
    memcpy(header + 12, (char*) &CSIDataLenExport, sizeof(CSIDataLenExport));
    cout<<"Recording to file '"<<filename.toUtf8().data()<<"' in WifEyeBinary format"<<endl;
  }
  if (!file->open(QIODevice::WriteOnly|QIODevice::Unbuffered)){
    cout<<"Could not create file"<<endl;
    file->close();
    delete file;
    return;
  }


  bool success;
  if(this->mw->getUI()->rbFileFormatBinary->isChecked()){
    success = (file->write(header, 12+sizeof(CSIDataLenExport)) > 0);
  }else{
    success = (file->write(header, strlen(header)) > 0);
  }
  if(!success){
    cout<<"Could not write to file"<<endl;
    file->close();
    delete file;
    return;
  }

  recording = true;
}

/**
 * Stop recording data into a file
 */
void networkThread::stopRecording(){
  if(recording){
    recording = false;
    file->close();
    delete file;
    cout<<"Recording stopped."<<endl;
  }else{
    cout<<"Not recording."<<endl;
  }
}

/**
 * Set a pointer to the filter manager that handles the processing filter plugins.
 */
void networkThread::setFilterManager(CSIFilterManager* manager){
  this->filterManager = manager;
}

/**
 * Activate/deactivate MAC filtering for recording.
 */
void networkThread::setMACFilterRecording(bool active){
  printf("MAC Filter Recording: %u\n",active);
  this->MACFilterRecording = active;
}

/**
 * Activate/deactivate MAC filtering for live export.
 */
void networkThread::setMACFilterLiveExport(bool active){
  printf("MAC Filter LiveExport: %u\n",active);

  this->MACFilterLiveExport = active;
}


/**
 * Add "MAC" to the list of MAC addresses to be considered for displaying/export.
 */
void networkThread::addMACToFilter(QString MAC){
  MACFilterList.append(MAC);
}

/**
 * Reset the list of MAC addresses to be considered for displaying/export.
 */
void networkThread::resetMACFilter(QString MAC){
  MACFilterList.clear();
}

/**
 * Set a list of MAC addresses to be considered for displaying/export.
 */
void networkThread::setMACFilterList(QStringList filters){
  MACFilterList = filters;
}

/**
 * Query if some MAC address is on the list of non-filtered MAC addresses
 */
bool networkThread::isMACActive(QString MAC){
  QString noFilter = "No Filter";
  for(uint32_t cnt = 0; cnt < MACFilterList.length(); cnt++){
    if((MACFilterList[cnt] == MAC)||(MACFilterList[cnt] == noFilter)){
      //  printf("MAC found: %s - %s\n", MACFilterList[cnt].toUtf8().data(), MAC.toUtf8().data());
      return true;
    }
  }
  return false;
}

/**
 * Notify the networkThread if the classifierThread (i.e., the thread reading the input from the classfier) is active.
 */
void networkThread::setClassifierThreadActive(bool active){
  classifierThreadActive = active;
}

/**
 * Set the number of subcarriers in the input data.
 */
void networkThread::setNCSISamples(uint32_t NCSISamples){
  CSIDataLen = NCSISamples;
  printf("Native CSI data length set to: %u\n",NCSISamples);
}

/**
 * Set the number of subcarriers to display.
 */
void networkThread::setNCSISamplesDisplay(uint32_t NCSISamples){
  printf("DisplayCSI data length set to: %u\n",NCSISamples);
  CSIDataLenDisplay = NCSISamples;
}

/**
 * Set the number of subcarriers to export.
 */
void networkThread::setNCSISamplesExport(uint32_t NCSISamples){
  printf("Export CSI data length set to: %u\n",NCSISamples);
  CSIDataLenExport = NCSISamples;
}


/**
 * Notfiy the network thread that amplitude displaying has been activated. If active==true, then data will be streamed to the amplitude display widget.
 */
void networkThread::setDisplayAmplitude(bool active){
  this->displayAmplitude = active;
}

/**
 * Notfiy the network thread that phase displaying has been activated. If active==true, then data will be streamed to the phase display widget.
 */
void networkThread::setDisplayPhase(bool active){
  this->displayPhase = active;
}

/**
 * Notfiy the network thread that RSSI displaying has been activated. If active==true, then data will be streamed to the RSSI display widget.
 */
void networkThread::setDisplayRSSI(bool active){
  this->displayRSSI = active;
}

/**
 * Notfiy the network thread that classification output displaying has been activated. If active==true, then data will be streamed to the classification output display widget.
 */
void networkThread::setDisplayClassifier(bool active){
  this->displayClassifier = active;
}
