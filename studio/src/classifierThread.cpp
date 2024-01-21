/*
 * classifierThread.cpp
 *
 *  Created on: 14.01.2021
 *      Author: Philipp H. Kindt <philipp.kindt@tum.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#include "classifierThread.h"
#include <stdio.h>
#include "mainwindow.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "networkThread.h"

#define USE_WRT true


classifierThread::classifierThread(){
#if USE_WRT
  wrt = NULL;
#endif

  connect(this,SIGNAL(finished()),this,SLOT(stopped()));
  connect(this,SIGNAL(hasStopped()), this,SLOT(stopped()));


  running = false;
  buf[0] = '\0';
  pid = 0;
}
classifierThread::~classifierThread(){
 mutex.unlock();
  printf("CT: Destroy\n");
  if(pid > 0){
    kill(pid,SIGTERM);
  }
#if USE_WRT
  if(wrt != NULL){
    delete wrt;
    wrt = NULL;
  }
#endif
  printf("CT: Destroyed\n");

}
void classifierThread::setCommand(const QString &Command){
  this->Command = Command;
}
void classifierThread::setArguments(const QString &Arguments){
  this->Arguments = Arguments;
}

void classifierThread::setMainWindow(MainWindow* mw){
  this->mw = mw;
}

void classifierThread::addData(const QString& data){
  if(!running){
    return;
  }
  ;
#if USE_WRT
  if(wrt != NULL){
    wrt->addData(data);
  }
#else
  write(pipe_fds_parent2Child[1],data.toLocal8Bit().data(),data.length());

#endif
}

void classifierThread::run(){
  int32_t nBytesRead;
  printf("starting classifier thread\n");
  running = true;

  pipe2(pipe_fds_child2Parent,O_DIRECT);
  pipe2(pipe_fds_parent2Child,O_DIRECT);

  pid = fork();
  if(pid == 0){
    /*newly forked child */
    //-U - t
    fprintf(stderr,"executing cmd: %s with args %s\n",Command.toUtf8().data(),Arguments.toUtf8().data());
    char* cmd[] = {Command.toUtf8().data(),Arguments.toUtf8().data(), NULL};
    dup2(pipe_fds_child2Parent[1],1);
    dup2(pipe_fds_parent2Child[0],0);

    close(pipe_fds_child2Parent[0]);
    close(pipe_fds_parent2Child[1]);
    char buf[3];

    if(execvp(cmd[0],cmd)!=0){
      fprintf(stderr,"Could not execute command\n");
      perror("execvp");
      close(pipe_fds_child2Parent[1]);
      close(pipe_fds_parent2Child[0]);
      buf[0] = 0xde;
      buf[1] = 0xad;
      buf[2] = 0xbe;
      buf[3] = 0xef;
      write(1,buf,4);
      emit startedStopped(false);

      return;
    }
  }else if(pid == -1){
    running = false;
    printf("running - false\n");
    return;

  }
  nBytesRead  = read(pipe_fds_child2Parent[0],buf,3);
  if((nBytesRead <= 0)||(buf[0] != (char) 0xca)||(buf[1] != (char) 0xff)||(buf[2] != (char) 0xee)){
    fprintf(stderr,"Communication with child process failed -  %u bytes read - %x,%x,%x.\n",nBytesRead,(uint8_t) buf[0],(uint8_t)  buf[1],(uint8_t)  buf[2]);
    return;
  }

  /*parent - this process!*/
#if USE_WRT
  wrt = new classifierWrThread();
  wrt->setMainWindow(mw);
  wrt->setFD(pipe_fds_parent2Child[1]);
    wrt->start();
#endif
  connect(this, SIGNAL(addDataToCDW(unsigned int, unsigned int, float)), this->mw->getCDW(), SLOT(addClassifierOutput(unsigned int, unsigned int, float)));
  connect(this, SIGNAL(resetCDW()), this->mw->getCDW(), SLOT(reset()));
  emit resetCDW();

  fprintf(stderr,"emitting start\n");
  emit startedStopped(true);
  while(1){

    nBytesRead = read(pipe_fds_child2Parent[0],localBuf,CLASSIFIER_RCV_BUF_LEN-1);

    //    fprintf(stderr,"r:[%s]\n",localBuf);
    if(nBytesRead <= 0){
      fprintf(stderr, "read failed - process no longer there\n");
      emit hasStopped();

      return;
    }
    localBuf[nBytesRead] = '\0';

    mutex.lock();
    memcpy(buf,localBuf,nBytesRead + 1);

    mutex.unlock();

   parseData(buf);
#if DATA_EXCHANGE_THROUGH_QT_SIGNALS
    emit dataReady(QString(localBuf));
#else
    mw->getUI()->lClassifierOutput->setText(QString(localBuf));
#endif

  }
  emit startedStopped(false);

}

void classifierThread::parseData(char* data){
  uint8_t classifID = 0;
  unsigned long peopleCnt;
  float certainty;
  //    printf("data: %s\n",data);
  char* ptr = strtok(data,":");

  bool start = true;
  while(1){
    if(!start){
      ptr = strtok(NULL,":");
    }
    if(ptr == NULL){
      break;
    }
    //     printf("str1 = %s\n",ptr);
    peopleCnt = strtoul(ptr,NULL,10);
    ptr = strtok(NULL,":");
    if(ptr != NULL){
      //        printf("str2 = %s\n",ptr);
      ptr[2] = ',';             //eventual dot separator => comma separator
      certainty = atof(ptr);
#if DATA_EXCHANGE_THROUGH_QT_SIGNALS
      emit addDataToCDW(classifID,  peopleCnt, certainty);
#else
      this->mw->getCDW()->addClassifierOutput(classifID,  peopleCnt, certainty);
#endif
      classifID++;
      start = false;
    }else{
      break;
    }

  }
}
void classifierThread::getData(char* bufDest,uint32_t maxLength){
  mutex.lock();
  uint32_t size = strlen(buf)+1;
  if(size > maxLength){
    size = maxLength-1;
  }
  memcpy(bufDest,buf,size);
  bufDest[size] = '\0';
  mutex.unlock();
}
void classifierThread::stopClassifier(){
  fprintf(stderr,"CT: termination...\n");

  if(running){
    fprintf(stderr,"stopping thread\n");
    running = false;
    if(!kill(pid,9)){
      perror("kill()");
    }
    this->terminate();
  }
}

void classifierThread::stopped(){
  fprintf(stderr,"CT thread terminated\n");
  running = false;
  if(wrt != NULL){
    wrt->stop(false);
    wrt = NULL;
  }
  emit startedStopped(false);
}

bool classifierThread::getStatus(){
  return running;
}

