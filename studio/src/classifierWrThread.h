/*
 * classifierWrThread.h
 *
 *  Jan. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _CLASSIFIER_WR_THREAD_H
#define _CLASSIFIER_WR_THREAD_H

#include <QThread>
#include <inttypes.h>
#include <QString>
#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

#define CLASSIFIER_RCV_BUF_LEN 500*256

class MainWindow;
/**
 * \brief a thread to stream data to the classifier - without delaying anything else when the classifier stalls.
 */
class classifierWrThread: public QThread{
  Q_OBJECT

  private:
  MainWindow* mw;               ///Pointer to the main window
  bool running;                 ///True, if the thread is running
  int pipe_fd;                  ///A file descriptor for a pipe to write to the classifier
  QQueue<QString> queue;        ///A queue to store data that is yet to be sent to the classifier
  QMutex mutex;                 ///A mutex to protect this class against uncoordinate access from different threads
  QWaitCondition wq;            ///This thread will go asleep when there is no data waiting to be sent out. As soon as addData() is called, this QWaitContition will wakeup the thread again
  char buf[CLASSIFIER_RCV_BUF_LEN];/// A temporary buffer to store what has been taken from the queue most recently
  public:

  classifierWrThread();
  ~classifierWrThread();

  /**
   * Set the file descriptor fd using which we can write to the classifier.
   */
  void setFD(int fd);

  /**
   * Set a pointer to the main widnow
   */
  void setMainWindow(MainWindow* mw);

  /**
   * Run this thread
   */
  void run() override;

  public slots:

  /**
   * Add data to be sent to the classifier. This data is in the form of a strinh written to its standard input.
   */
  void addData(const QString &data);

  /**
   *  Stop this thread.
   */
  void stop(bool);


};



#endif
