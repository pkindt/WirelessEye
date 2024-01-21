/*
 * classifierThread.cpp
 *
 *  Jan. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#include "classifierWrThread.h"
#include "mainwindow.h"
#include <unistd.h>
classifierWrThread::classifierWrThread(){
  mutex.unlock();
  running = false;
  pipe_fd = 0;
 }
classifierWrThread::~classifierWrThread(){
  mutex.unlock();
  running = false;
  wq.wakeAll();
  printf("CWT: destroy\n");
}
void classifierWrThread::setMainWindow(MainWindow* mw){
  this->mw = mw;
}


void classifierWrThread::addData(const QString& data){
  mutex.lock();
  queue.enqueue(data);
  mutex.unlock();
  wq.wakeAll();
}

  void classifierWrThread::run(){
    printf("CWT starting...\n");
    running = true;
    int nBytes;
    char* str;
    uint32_t len;
    while(1){
      mutex.lock();
        if(!queue.isEmpty()){
          str = queue.head().toLocal8Bit().data();
          len = strlen(str);
          if(len>CLASSIFIER_RCV_BUF_LEN){
            printf("WARNING - string to large. increase CLASSIFIER_RCV_BUF_LEN.\n");
            queue.dequeue();
            continue;
          }
          memcpy(buf,str, len);
          buf[CLASSIFIER_RCV_BUF_LEN - 1] = '\0';

          queue.dequeue();
          mutex.unlock();


          if(pipe_fd > 0){
            nBytes = write(pipe_fd, buf, len);
            //   write(pipe_fd, data, 3);
          }
          if(nBytes < 0){
            printf("write failure - CWT thread terminating\n");
          }
 //         fprintf(stderr,str.toUtf8().data());
        }else{

          wq.wait(&mutex);
          mutex.unlock();
        }

        if(!running){
         return;
       }
      }
  }

  void classifierWrThread::setFD(int fd){
    this->pipe_fd = fd;
  }


void classifierWrThread::stop(bool start){
  if(!start){
    if(running){
      printf("CWT: stopping\n");
      running = false;
      this->quit();
    }
  }
}
