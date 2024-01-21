/*
 * classifierThread.h
 * Jan 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _CLASSIFIER_THREAD_H
#define _CLASSIFIER_THREAD_H

#include <QThread>
#include <inttypes.h>
#include <QString>
#include <QObject>
#include "classifierWrThread.h"
#include <QMutex>



class MainWindow;

/**
  * \brief Controls the thread that reads the outputs from the classifier.
 *
 * Controls the thread that reads the outputs from the classifier.
 * This thread also launches the classifier.
 * It does not write data to the classifier. This is either (depending on the configuration macros) the network thread, or a separate classifier write thread (classifierWrThread)
 *
 */
class classifierThread: public QThread{
  Q_OBJECT

  private:
  MainWindow* mw;                       ///Pointer to the main window
  classifierWrThread* wrt;              ///Pointer to the classifier write thread, who is (depending on the configuration macrods) resposible for streaming data to the classifier
  bool running;                         ///Flag if the thread (and hence the classifier) is running
  QString Command;                      ///The command to be executed to launch the classifier
  QString Arguments;                    ///Arguments of this command
  int pipe_fds_child2Parent[2];         ///A pipeline from the classifier to us
  int pipe_fds_parent2Child[2];         ///A pipeline from our process to that of the classifier.
  pid_t pid;                            ///The process ID of the forked classifier
  char buf[CLASSIFIER_RCV_BUF_LEN];
  char localBuf[CLASSIFIER_RCV_BUF_LEN];
  QMutex mutex;
  public:
  classifierThread();
  ~classifierThread();

  /**
   * Set the command to be executed as our classiifer
   */
  void setCommand(const QString &Command);

  /**
   * Set the command line arguments of the executable
   */
  void setArguments(const QString &Arguments);

  /**
   * Run the classiifier
   */
  void run() override;

  /**
   * Set a pointer to the main window
   */
  void setMainWindow(MainWindow* mw);

  /**
   * Parse the data received from the classifier.
   */
  void parseData(char* data);

  /**
   * Returns true, if the classifier is running.
   * */
  bool getStatus();

  /** The classifier will provide its output in the form of a string
   * This function copies the most recently received string into bufDest.
   * MaxLength should be equal to the number of bytes allocated in bufDest.
   * MaxLength
   */
  void getData(char* bufDest,uint32_t maxLength);


  signals:

  /**
   * We emit startedStopped(true), when the classifier has been started. We emit startedStopped(false), when it stops
   */
  void startedStopped(bool);

  /**
   * We emit hasStopped() in case of failure to read fromt the classifier, e.g., when it has died
   */
  void hasStopped();

  /**
   * This signal is emitted when new data is ready.
   */
  void dataReady(const QString&);

  /**
   * This signal is emitted to add data tot he classifier display widget*
   * classifID: Counting number of the classifier
   * classNo:   Class it has identified
   * certainty: The certainty of this classification
   */
  void addDataToCDW(unsigned int classifID, unsigned int classNo, float certainty);

  /*Reset classifier display widget.*/
  void resetCDW();

  public slots:

  /**
   * Add data to be sent to the classifier. It will usually be passed on to the classifier WR thread.
   * Params:
   * data: A string to be sent to stdin of the classifier
   */
  void addData(const QString& data);
  /**
   * Stop the classifier
   */
  void stopClassifier();

  /**
   *  Event callback if the classifier thead has stopped
   */
  void stopped();
};



#endif
