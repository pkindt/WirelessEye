/*
 * mainwindow.cpp
 * Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QScrollBar>

using namespace std;
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
  //setlocale(LC_NUMERIC, "en_US.UTF-8");          //avoid comma as decimal separator
  ui->setupUi(this);
    connect(ui->pbConnect,SIGNAL(clicked()),this,SLOT(startStreaming()));
      isStarted = false;
    dwA = new displayWidget(ui->saAmplitude);
    dwA->setMainWindow(this);
    dwA->setDataOffset(0);
    dwA->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    dwP = new displayWidget(ui->saAmplitude);
    dwP->setMainWindow(this);
    dwP->setDataOffset(0);
    dwP->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    connect(ui->slLBAmplitude, SIGNAL(valueChanged(int)), dwA,SLOT(setLB(int)));
    connect(ui->slUBAmplitude, SIGNAL(valueChanged(int)), dwA,SLOT(setUB(int)));
    connect(ui->slLBPhase, SIGNAL(valueChanged(int)), dwP,SLOT(setLB(int)));
    connect(ui->slUBPhase, SIGNAL(valueChanged(int)), dwP,SLOT(setUB(int)));
    dwA->setLB(ui->slLBAmplitude->value());
    dwA->setUB(ui->slUBAmplitude->value());
    dwP->setLB(ui->slLBPhase->value());
    dwP->setUB(ui->slUBPhase->value());



    dwRSSI = new displayWidget(ui->saRSSI);
    dwRSSI->setMainWindow(this);
    dwRSSI->setDataOffset(0);
    dwRSSI->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    dwRSSI->setFlatCurve(true);
    cdw = new classifierDisplayWidget(ui->saClassif);
    cdw->setMainWindow(this);


    ui->saAmplitude->setWidget(dwA);
    ui->saClassif->setWidget(cdw);
    ui->saPhase->setWidget(dwP);
    ui->saRSSI->setWidget(dwRSSI);

    ct = new classifierThread();
    ct->setMainWindow(this);

    nt = NULL;
    nt_thread = NULL;

    connect(ui->actionQuit,SIGNAL(triggered()), this, SLOT(close()));
    showHideAmplitude(ui->cbDisplayAmplitude->isChecked());
    showHidePhase(ui->cbDisplayPhase->isChecked());
    showHideRSSI(ui->cbDisplayRSSI->isChecked());
    showHideClassifier(ui->cbDisplayClassifierOutput->isChecked());
    connect(ui->cbDisplayAmplitude, SIGNAL(toggled(bool)), this,SLOT(showHideAmplitude(bool)));
    connect(ui->cbDisplayPhase, SIGNAL(toggled(bool)), this,SLOT(showHidePhase(bool)));
    connect(ui->cbDisplayClassifierOutput, SIGNAL(toggled(bool)), this,SLOT(showHideClassifier(bool)));
    connect(ui->cbDisplayRSSI, SIGNAL(toggled(bool)), this,SLOT(showHideRSSI(bool)));



    //animation Timer
    animTimer = new QTimer(this);
    connect(animTimer,SIGNAL(timeout()), this, SLOT(animate()));
    connect(ui->pbResetMaxAmplitude,SIGNAL(clicked()), this, SLOT(activateAmplitudeScaling()));
    connect(ui->pbResetMaxAmplitude,SIGNAL(clicked()), dwA, SLOT(resetMaximumValue()));
    connect(ui->cbUpdateAmplitude,SIGNAL(toggled(bool)),dwA,SLOT(setAutoScaling(bool)));

    connect(ui->pbResetMaxPhase,SIGNAL(clicked()), this, SLOT(activatePhaseScaling()));
    connect(ui->pbResetMaxPhase,SIGNAL(clicked()), dwP, SLOT(resetMaximumValue()));
    connect(ui->cbUpdatePhase,SIGNAL(toggled(bool)),dwP,SLOT(setAutoScaling(bool)));


    connect(ui->pbRecord,SIGNAL(toggled(bool)),this,SLOT(recordButtonHandler()));
    connect(ui->rbWidthAdjust, SIGNAL(toggled(bool)), this, SLOT(updateDisplayWidgetSize()));
    connect(ui->spWidth, SIGNAL(valueChanged(int)), this, SLOT(updateDisplayWidgetSize()));

    connect(ui->sbRefreshPeriod,SIGNAL(valueChanged(int)),this,SLOT(updateDisplayPeriod()));
    connect(&updateLayoutTimer,SIGNAL(timeout()), this, SLOT(updateDisplayWidgetSize()));
    ui->saAmplitude->horizontalScrollBar()->setSliderPosition(ui->saAmplitude->horizontalScrollBar()->maximum());
    ui->saPhase->horizontalScrollBar()->setSliderPosition(ui->saPhase->horizontalScrollBar()->maximum());
    ui->saRSSI->horizontalScrollBar()->setSliderPosition(ui->saRSSI->horizontalScrollBar()->maximum());
    ui->saClassif->horizontalScrollBar()->setSliderPosition(ui->saClassif->horizontalScrollBar()->maximum());


    connect((QObject*) ui->saAmplitude->horizontalScrollBar(),SIGNAL(sliderMoved(int)),(QObject*) ui->saClassif->horizontalScrollBar(),SLOT(setValue(int)));
    connect((QObject*) ui->saAmplitude->horizontalScrollBar(),SIGNAL(sliderMoved(int)),(QObject*) ui->saPhase->horizontalScrollBar(),SLOT(setValue(int)));
    connect((QObject*) ui->saAmplitude->horizontalScrollBar(),SIGNAL(sliderMoved(int)),(QObject*) ui->saRSSI->horizontalScrollBar(),SLOT(setValue(int)));

    connect(ui->sbScrollDelay, SIGNAL(valueChanged(int)), dwA, SLOT(setScrollDelay(int)));
    connect(ui->sbScrollDelay, SIGNAL(valueChanged(int)), dwP, SLOT(setScrollDelay(int)));
    connect(ui->sbScrollDelay, SIGNAL(valueChanged(int)), dwRSSI, SLOT(setScrollDelay(int)));

    updateLayoutTimer.setSingleShot(true);
    updateLayoutTimer.start(1);

    connect(ui->pbRunClassifier, SIGNAL(clicked()),this, SLOT(startClassifier()));
    connect(ct,SIGNAL(startedStopped(bool)),this,SLOT(classifierStartedStopped(bool)));
    connect(ct,SIGNAL(dataReady(const QString&)),this,SLOT(updateClassifierData(const QString&)));
    connect(this,SIGNAL(startClassifierThread()),ct,SLOT(start()));
    connect(this,SIGNAL(stopClassifierThread()), ct, SLOT(stopClassifier()));
    connect(ui->sbMaxClass,SIGNAL(valueChanged(int)),cdw,SLOT(setMaxValue(int)));
    cdw->setMaxValue(ui->sbMaxClass->value());

    connect(ui->actionAbout,SIGNAL(triggered()), &da, SLOT(show()));

    fgm = new CSIFilterGUIManager();
    fgm->setMainWindow(this);
    fgm->buildGUI();
    connect(ui->pbRefreshFilters, SIGNAL(clicked()),fgm,SLOT(buildGUI()));
    connect(ui->leFilterPath,SIGNAL(textChanged(const QString&)), fgm, SLOT(setFilterPath(const QString&)));

    connect(ui->actionDetachFilters,SIGNAL(triggered()), fgm, SLOT(attachDetach()));
    cbx = new checkableComboBox(this);
    ui->buttonBarLayout->addWidget((QWidget*) cbx);
    connect(ui->actionClearMACFilterList, SIGNAL(triggered()), cbx, SLOT(resetMACs()));

}

MainWindow::~MainWindow()
{
  animTimer->stop();
  if(nt != NULL){
 //   nt->disconnect();
    nt->stop();
  //   delete nt_thread;                   //will also free nt

  }

  ct->stopClassifier();
//  delete dwA;
 // delete cdw;
 // delete dwP;
 // delete dwRSSI;
  delete animTimer;
  delete ct;
 // delete cbx;
  delete ui;
delete fgm;
}

void MainWindow::startStreaming(){
  if(!isStarted){
    nt_thread = new QThread;
    nt = new networkThread();
    nt->moveToThread(nt_thread);
    connect(ui->qcBandwidth, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBandwidthHandling()));
    connect(ui->qcBandwidthDisplay, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBandwidthHandling()));
    connect(ui->qcBandwidthExport, SIGNAL(currentIndexChanged(int)), this, SLOT(updateBandwidthHandling()));

    updateBandwidthHandling();

    cout<<"start streaming"<<endl;
    isStarted = true;
    nt->setMainWindow(this);
    nt->setFilterManager(fgm->getFilterManager());
    nt->setMACFilterRecording(ui->cbFilterFileRecording->isChecked());
    nt->setMACFilterLiveExport(ui->cbFilterLiveExport->isChecked());
       connect(nt_thread,SIGNAL(finished()), nt, SLOT(deleteLater()));
       connect( nt_thread,SIGNAL(started()), nt, SLOT(operate()));

    connect(nt,SIGNAL(streamingStartedStopped(bool)),this,SLOT(streamStartStop(bool)));
    connect(nt,SIGNAL(finished()),nt_thread,SLOT(quit()));
    connect(this,SIGNAL(stopStreaming()),nt,SLOT(stop()));
    connect(ui->cbFilterFileRecording,SIGNAL(toggled(bool)), nt, SLOT(setMACFilterRecording(bool)));
    connect(ui->cbFilterLiveExport,SIGNAL(toggled(bool)), nt, SLOT(setMACFilterLiveExport(bool)));

    connect(ct,SIGNAL(startedStopped(bool)), nt, SLOT(setClassifierThreadActive(bool)));
    connect(cbx,SIGNAL(updateMacFilterList(QStringList)),nt,SLOT(setMACFilterList(QStringList)));
    connect(nt,SIGNAL(addMAC(QString)),cbx,SLOT(addMAC(QString)));


    connect(ui->cbDisplayAmplitude, SIGNAL(toggled(bool)), nt,SLOT(setDisplayAmplitude(bool)));
    connect(ui->cbDisplayPhase, SIGNAL(toggled(bool)), nt,SLOT(setDisplayPhase(bool)));
    connect(ui->cbDisplayRSSI, SIGNAL(toggled(bool)), nt,SLOT(setDisplayRSSI(bool)));
    connect(ui->cbDisplayClassifierOutput, SIGNAL(toggled(bool)), nt,SLOT(setDisplayClassifier(bool)));

    nt->setDisplayAmplitude(ui->cbDisplayAmplitude->isChecked());
    nt->setDisplayPhase(ui->cbDisplayPhase->isChecked());
    nt->setDisplayRSSI(ui->cbDisplayRSSI->isChecked());
    nt->setDisplayClassifier(ui->cbDisplayClassifierOutput->isChecked());

    cbx->updateFilters();
    nt->setAddr(ui->leHostname->text());
    printf("launching thread...\n");
    nt_thread->start();

  }else{
    if(nt->getStatus()){
      cout<<"stop streaming"<<endl;
      nt->stop();
      //terminating the thead will destroy nt and nt_thread
      nt = NULL;
    }
  }
}
void MainWindow::streamStartStop(bool started){
  if(started){
    ui->pbConnect->setText("Disconnect");
    ui->pbRecord->setEnabled(true);
   animTimer->start(ui->sbRefreshPeriod->value());
    isStarted = true;
  }else{
    ui->pbConnect->setText("Connect");
    ui->pbRecord->setEnabled(false);
    animTimer->stop();
    isStarted = false;
  }
}
Ui::MainWindow* MainWindow::getUI(){


  return this->ui;

}


displayWidget* MainWindow::getdwA(){
  return dwA;
}
displayWidget* MainWindow::getdwP(){
  return dwP;
}
displayWidget* MainWindow::getdwRSSI(){
  return dwRSSI;
}

classifierDisplayWidget* MainWindow::getCDW(){
  return cdw;
}

void MainWindow::updateDisplayPeriod(){

  animTimer->setInterval(ui->sbRefreshPeriod->value());

}


void MainWindow::animate(){
    dwA->update();
    if(ui->cbDisplayPhase->isChecked()){
     dwP->update();
    }
    if(ui->cbDisplayRSSI->isChecked()){
      dwRSSI->update();
    }

    if(ui->cbDisplayClassifierOutput->isChecked()){
     cdw->update();
   }

}
void MainWindow::recordButtonHandler(){
  if(ui->pbRecord->isChecked()){
    ui->pbRecord->setText("Stop");
    nt->startRecording();
  }else{
    ui->pbRecord->setText("Record");
    nt->stopRecording();
  }
}

void MainWindow::updateDisplayWidgetSize(){
    if(ui->rbWidthAdjust->isChecked()){
        dwA->setMinimumSize(QSize(1,1));
        dwP->setMinimumSize(QSize(1,1));
        cdw->setMinimumSize(QSize(1,1));
        dwRSSI->setMinimumSize(QSize(1,1));
        dwA->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        dwP->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        cdw->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        dwRSSI->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        emit resizecdw(ui->saClassif->width()-10,ui->saClassif->height());
        emit resizedwA(ui->saAmplitude->width()-10,ui->saAmplitude->height());
        emit resizedwP(ui->saPhase->width()-10,ui->saPhase->height());
        emit resizedwRSSI(ui->saRSSI->width()-10,ui->saRSSI->height());
    }else{
      cout<<"Generic Update - static - new size will be "<<ui->spWidth->value()<<"..."<<endl;

        ui->saAmplitude->horizontalScrollBar()->setMaximum(ui->spWidth->value());
        ui->saAmplitude->horizontalScrollBar()->setSliderPosition(ui->saAmplitude->horizontalScrollBar()->maximum());
        ui->saPhase->horizontalScrollBar()->setMaximum(ui->spWidth->value());
        ui->saPhase->horizontalScrollBar()->setSliderPosition(ui->saAmplitude->horizontalScrollBar()->maximum());
        ui->saClassif->horizontalScrollBar()->setMaximum(ui->spWidth->value());
        ui->saClassif->horizontalScrollBar()->setSliderPosition(ui->saAmplitude->horizontalScrollBar()->maximum());
        ui->saRSSI->horizontalScrollBar()->setMaximum(ui->spWidth->value());
        ui->saRSSI->horizontalScrollBar()->setSliderPosition(ui->saRSSI->horizontalScrollBar()->maximum());
        dwA->setMinimumSize(QSize(ui->spWidth->value() - 10,1));
        dwP->setMinimumSize(QSize(ui->spWidth->value() - 10,1));
        cdw->setMinimumSize(QSize(ui->spWidth->value() - 10,1));
        dwRSSI->setMinimumSize(QSize(ui->spWidth->value() - 10,1));
        dwA->setMaximumSize(QSize(ui->spWidth->value() - 10,QWIDGETSIZE_MAX));
        dwP->setMaximumSize(QSize(ui->spWidth->value() - 10,QWIDGETSIZE_MAX));
        cdw->setMaximumSize(QSize(ui->spWidth->value() - 10,QWIDGETSIZE_MAX));
        dwRSSI->setMaximumSize(QSize(ui->spWidth->value() - 10,QWIDGETSIZE_MAX));
        dwP->resize(ui->spWidth->value() - 10,ui->saPhase->height());
        cdw->resize(ui->spWidth->value() - 10,ui->saClassif->height());
        dwA->resize(ui->spWidth->value() - 10,ui->saAmplitude->height());
        dwRSSI->resize(ui->spWidth->value() - 10,ui->saAmplitude->height());


    }


}
void MainWindow::resizeEvent(QResizeEvent* event){
  updateDisplayWidgetSize();
}

void MainWindow::startClassifier(){
  if(ct->getStatus() == false){
    printf("starting classifier from main window\n");
    ui->cbDisplayClassifierOutput->setChecked(true);
    showHideClassifier(true);
    ct->setCommand(ui->leClassifierExecutable->text());
    ct->setArguments(ui->leClassifierArguments->text());
    emit startClassifierThread();
  }else{
    printf("stopping classifier\n");
    emit stopClassifierThread();

  }
}

void MainWindow::classifierStartedStopped(bool started){
  printf("classifierStartedStopped - %u\n",started);
  isStarted = false;
  if(started){
    ui->pbRunClassifier->setText("Stop Classifier");
    ui->pbRunClassifier->setChecked(true);
  }else{
    ui->pbRunClassifier->setText("Run Classifier");
    ui->pbRunClassifier->setChecked(false);
    ui->lClassifierOutput->setText("[Classifier not running]");

  }
}

classifierThread* MainWindow::getCT(){
  return ct;
}


void MainWindow::updateClassifierData(const QString& data){
    ui->lClassifierOutput->setText(data);
}

void MainWindow::activateAmplitudeScaling(){
    ui->cbUpdateAmplitude->setChecked(true);
}
void MainWindow::activatePhaseScaling(){
    ui->cbUpdatePhase->setChecked(true);
}

void MainWindow::showHideAmplitude(bool shown){
  ui->saAmplitude->setHidden(!shown);
  ui->labelAmplitude->setHidden(!shown);
  ui->labelSpacerAmplitudeLeft->setHidden(!shown);
  ui->slLBAmplitude->setHidden(!shown);
  ui->slUBAmplitude->setHidden(!shown);
  ui->cbUpdateAmplitude->setHidden(!shown);
  ui->pbResetMaxAmplitude->setHidden(!shown);

  updateDisplayWidgetSize();
}


void MainWindow::showHidePhase(bool shown){
  ui->saPhase->setHidden(!shown);
  ui->labelPhase->setHidden(!shown);
  ui->labelSubcarrierPhase->setHidden(!shown);
  ui->slLBPhase->setHidden(!shown);
  ui->slUBPhase->setHidden(!shown);
  ui->cbUpdatePhase->setHidden(!shown);
  ui->pbResetMaxPhase->setHidden(!shown);
  updateDisplayWidgetSize();
}
void MainWindow::showHideRSSI(bool shown){
  ui->labelRSSI->setHidden(!shown);
  ui->labelPlaceholderRSSILeft->setHidden(!shown);
  ui->labelPlaceholderRSSIRight->setHidden(!shown);
  ui->saRSSI->setHidden(!shown);
  updateDisplayWidgetSize();
}
void MainWindow::showHideClassifier(bool shown){
  ui->saClassif->setHidden(!shown);
  ui->labelClassif->setHidden(!shown);
  ui->labelClassifSpacer->setHidden(!shown);
  ui->labelClassifOutput->setHidden(!shown);
  updateDisplayWidgetSize();
}


CSIFilterGUIManager* MainWindow::getFGM(){
  return fgm;
}


void MainWindow::closeEvent(QCloseEvent *event){
  fgm->attachFilters();
  if(cbx->getDetached()){
    cbx->attach();
  }  QMainWindow::closeEvent(event);

}

void MainWindow::moveEvent(QMoveEvent * event){
  if(cbx->getDetached()){
    cbx->getLv()->move( cbx->getLv()->mapToGlobal(QPoint(0,0)) + (event->pos()-event->oldPos()));
  }
}


checkableComboBox* MainWindow::getCBX(){
  return cbx;
}
void MainWindow::proccessCmdLineArguments(char* url,char* bw, char* bwDisplay, char* bwExport){
  proccessCmdLineArguments(url, bw, bwDisplay);
  if(QString("80").compare(bwExport)==0){
     ui->qcBandwidthExport->setCurrentIndex(2);
  }else if(QString("40").compare(bwExport)==0){
     ui->qcBandwidthExport->setCurrentIndex(1);
    }else{
     ui->qcBandwidthExport->setCurrentIndex(0);
  }
}
void MainWindow::proccessCmdLineArguments(char* url,char* bw, char* bwDisplay){
  proccessCmdLineArguments(url, bw);
  if(QString("80").compare(bwDisplay)==0){
     ui->qcBandwidthDisplay->setCurrentIndex(2);
  }else if(QString("40").compare(bwDisplay)==0){
     ui->qcBandwidthDisplay->setCurrentIndex(1);
    }else{
     ui->qcBandwidthDisplay->setCurrentIndex(0);
  }
}

void MainWindow::proccessCmdLineArguments(char* url,char* bw){
  ui->leHostname->setText(url);
  if(QString("80").compare(bw)==0){
    ui->qcBandwidth->setCurrentIndex(2);
  }else if(QString("40").compare(bw)==0){
    ui->qcBandwidth->setCurrentIndex(1);
  }else{
    ui->qcBandwidth->setCurrentIndex(0);
  }

}

void MainWindow::updateBandwidthHandling(){

  if(ui->qcBandwidth->currentIndex() == 0){
    //20MHz
    dwA->setNCSISamples(64);
    dwP->setNCSISamples(64);
    nt->setNCSISamples(64);
    nt->setNCSISamplesDisplay(64);
    nt->setNCSISamplesExport(64);
  }else if(ui->qcBandwidth->currentIndex() == 1){
    //40MHz
    if(ui->qcBandwidthDisplay->currentIndex() == 0){
      nt->setNCSISamplesDisplay(64);
      dwA->setNCSISamples(64);
      dwP->setNCSISamples(64);
    }else{
      nt->setNCSISamplesDisplay(128);
      dwA->setNCSISamples(128);
      dwP->setNCSISamples(128);
    }

    if(ui->qcBandwidthExport->currentIndex() == 0){
      nt->setNCSISamplesExport(64);
    }else{
      nt->setNCSISamplesExport(128);
    }
    nt->setNCSISamples(128);

  }else{
    //80MHz
    if(ui->qcBandwidthDisplay->currentIndex() == 0){
      dwA->setNCSISamples(64);
      dwP->setNCSISamples(64);
      nt->setNCSISamplesDisplay(64);
    }else if(ui->qcBandwidthDisplay->currentIndex() == 1){
      dwA->setNCSISamples(128);
      dwP->setNCSISamples(128);
      nt->setNCSISamplesDisplay(128);
    }else{
      dwA->setNCSISamples(256);
      dwP->setNCSISamples(256);
      nt->setNCSISamplesDisplay(256);
    }


    if(ui->qcBandwidthExport->currentIndex() == 0){
      nt->setNCSISamplesExport(64);

    }else if(ui->qcBandwidthExport->currentIndex() == 1){
      nt->setNCSISamplesExport(128);

    }else{
      nt->setNCSISamplesExport(256);
    }
    nt->setNCSISamples(256);

  }


}
