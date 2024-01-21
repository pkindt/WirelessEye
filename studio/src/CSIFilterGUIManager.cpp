/*
 * CSIFilterGUIManager.cpp
 *
 * July 2021,  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#include <QVector>
#include <QLineEdit>
#include "CSIFilterGUIManager.h"
#include "CSIFilter.h"
#include "mainwindow.h"
#include <math.h>
#define ITEMS_PER_BLOCK 1

CSIFilterGUIManager::CSIFilterGUIManager(){
  manager =0;
  path = CSI_FILTER_PATH;                               //path where the compiled plugins are located
  this->mw = NULL;
  mutex.unlock();
  vspacer = NULL;
  built = false;
  independentWindow = NULL;
}

/*Set a pointer tot he main window. Call this before using any functionality*/
void CSIFilterGUIManager::setMainWindow(MainWindow *mw){
  this->mw = mw;
  flist = NULL;
  manager = new CSIFilterManager();
}

/* Build the dynamic GUI based on the set of available filters*/
void CSIFilterGUIManager::buildGUI(){
  static char buf[CSI_FILTER_NAME_PARMETER_STLEN];
  static char listBuf[CSI_FILTER_NAME_PARMETER_LIST_STLEN];
  QStringList parListStrList;
  QStringList tokens;
  if(built){
    finalize();
  }

  //obtain a list of filters from the filter manager. Each filter = one plugin.
  manager->loadFilterList(path);
  flist = manager->getFilterList();

  QFrame* frameOuter, *frameInner, *frameTop;
  QFormLayout *flayout;
  QVBoxLayout *vlayout;
  QGridLayout *glayoutTop;
  QLabel *label_l, *label_r;
  QSpinBox *sbox;
  QCheckBox *cbox;
  QLineEdit *le;
  QDoubleSpinBox *dsb;
  QToolButton *tbtn;
  QScrollArea *sa;
  QSpacerItem* hspacer;
  for(uint32_t i = 0; i < flist->length(); i++){


    //Outer frame. Pointers will be stored for later deletion
    frameOuter = new QFrame();
    framesOuter.append(frameOuter);
    frameOuter->setFrameShape(QFrame::Box);
    frameOuter->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    mw->getUI()->laFilters->addWidget((QWidget*) frameOuter);

    //Vertical layout of outer box
    vlayout = new QVBoxLayout(frameOuter);

    //top frame for name+expansion arrow, prio and activation checkbox
    frameTop = new QFrame(NULL);
    frameTop->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    //grid layout for top frame
    glayoutTop = new QGridLayout(frameTop);

    //Tool button for expansion arrow. Pointers are stored for signal handling.
    tbtn = new QToolButton();
    expandBtns.append(tbtn);
    tbtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tbtn->setArrowType(Qt::ArrowType::RightArrow);
    tbtn->setText(flist->at(i)->getName());
    tbtn->setStyleSheet("font-weight:bold; border:none;");
    tbtn->setCheckable(true);
    tbtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(tbtn, SIGNAL(toggled(bool)), this, SLOT(expandArrowHandler()));
    glayoutTop->addWidget(tbtn,1,1);

    //horiz. space right of filter name
    hspacer = new QSpacerItem(1,1,QSizePolicy::Expanding, QSizePolicy::Expanding);
    glayoutTop->addItem(hspacer,1,2);

    //Prio label
    label_l = new QLabel();
    label_l->setText("Priority");
    label_l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    glayoutTop->addWidget(label_l,1,3);

    //prio spinbox. Pointers are stored for signal handling
    sbox = new QSpinBox();
    sbPiorities.append(sbox);
    sbox->setMinimum(1);
    sbox->setMaximum(1000);
    sbox->setValue(1);
    tbtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(sbox, SIGNAL(valueChanged(int)),this,SLOT(updatePriorities()));
    glayoutTop->addWidget(sbox,1,4);

    //filter activation checkbox
    cbox = new QCheckBox();
    utilizeCbs.append(cbox);
    cbox->setText("Activated");
    cbox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(cbox,SIGNAL(stateChanged(int)),this,SLOT(updateActivations()));
    glayoutTop->addWidget(cbox,1,5);
    vlayout->addWidget(frameTop);

    //scroll area to collapse and expand, containing the filer descriptions & options. Pointers are stored for handling expansion and collapsing
    sa = new QScrollArea();
    sa->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sa->setMaximumHeight(0);
    expandScrollAreas.append(sa);
    vlayout->addWidget(sa);

    //Inner frame, showing filter description & options
    frameInner = new QFrame(frameOuter);
    sa->setWidget(frameInner);
    sa->setWidgetResizable(true);

    //Form layout for filter description & options
    flayout = new QFormLayout(frameInner);

    //Description label left
    label_l = new QLabel(frameOuter);
    label_l->setText("Description:");
    label_l->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    //Description label right
    label_r = new QLabel(frameOuter);
    label_r->setText(flist->at(i)->getDescription());
    flayout->addRow((QWidget*)label_l,(QWidget*) label_r);





    //Set checked by default
    sprintf(buf,"");
    flist->at(i)->getParameter("defaultActive",buf);
    if(strcmp(buf,"1")==0){
      cbox->setChecked(true);
    }

    //set default priorities
    sprintf(buf,"");
    flist->at(i)->getParameter("defaultPriority",buf);
    if(strcmp(buf,"")!=0){
      sbox->setValue(atoi(buf));
    }



    /* Handle filter options */
    sprintf(listBuf,"");
    flist->at(i)->getParameterList(listBuf);
    parListStrList = QString(listBuf).split(QLatin1Char('\n'));
    for(uint32_t j = 0; j < parListStrList.length(); j++){
      tokens = parListStrList[j].split(QLatin1Char(','));
      if(tokens.length() != 7){
        printf("Could not parse parameters of filter [%s] - number of tokens does not match. String to parse: %s\n",flist->at(i)->getName().toLocal8Bit().data(),parListStrList[j].toLocal8Bit().data());
        j =  parListStrList.length();           //abort
        break;
      }else{
      //  printf("Parsing parameter: %s\n",parListStrList[j].toLocal8Bit().data());

        if(tokens[2] == "string"){
          //add a string parameter
          label_l = new QLabel();
          label_l->setText(tokens[1].toLocal8Bit().data());
          le = new QLineEdit();
          paramWidgets.append(le);
          sprintf(buf,"");
          flist->at(i)->getParameter(tokens[0].toLocal8Bit().data(),buf);
          le->setText(buf);
          flayout->addRow((QWidget*)label_l,le);
          widgetType.append(0);         //0=>string
          paramToFilter.append(flist->at(i));
          paramStrings.append(tokens[0]);
          connect(le, SIGNAL(textChanged(const QString&)), this, SLOT(paramUpdateString(QString)));

        }else if(tokens[2] == "bool"){
          //add a string parameter
          label_l = new QLabel();
          label_l->setText(tokens[1].toLocal8Bit().data());
          cbox = new QCheckBox();
          paramWidgets.append(cbox);
          sprintf(buf,"");
          flist->at(i)->getParameter(tokens[0].toLocal8Bit().data(),buf);
          cbox->setText("activated");
          if(strcmp(buf,"1")==0){
            cbox->setChecked(true);
          }else{
            cbox->setChecked(false);
          }
          flayout->addRow((QWidget*)label_l,cbox);
          widgetType.append(1);         //1=>bool
          paramToFilter.append(flist->at(i));
          paramStrings.append(tokens[0]);
          connect(cbox, SIGNAL(toggled(bool)), this, SLOT(paramUpdateBoolean(bool)));
        }else if(tokens[2] == "integer"){
          //add a string parameter
          label_l = new QLabel();
          label_l->setText(tokens[1].toLocal8Bit().data());
          sbox = new QSpinBox();
          paramWidgets.append(sbox);
          sbox->setMinimum(atoi(tokens[3].toLocal8Bit().data()));
          sbox->setMaximum(atoi(tokens[4].toLocal8Bit().data()));
          sprintf(buf,"");
          flist->at(i)->getParameter(tokens[0].toLocal8Bit().data(),buf);
          sbox->setValue(atoi(buf));
          flayout->addRow((QWidget*)label_l,sbox);
          widgetType.append(2);         //2=>integer
          paramToFilter.append(flist->at(i));
          paramStrings.append(tokens[0]);
          connect(sbox,SIGNAL(valueChanged(int)), this, SLOT(paramUpdateInteger(int)));
        }else if(tokens[2] == "float"){
          //add a string parameter
          label_l = new QLabel();
          label_l->setText(tokens[1].toLocal8Bit().data());
          dsb = new QDoubleSpinBox();
          paramWidgets.append(dsb);
          dsb->setMinimum(atof(tokens[3].toLocal8Bit().data()));
          dsb->setMaximum(atof(tokens[4].toLocal8Bit().data()));
          dsb->setDecimals(atof(tokens[5].toLocal8Bit().data()));
          dsb->setSingleStep(1.0/pow(10.0,atof(tokens[5].toLocal8Bit().data())));
          sprintf(buf,"");
          flist->at(i)->getParameter(tokens[0].toLocal8Bit().data(),buf);
          dsb->setValue(atof(buf));
          flayout->addRow((QWidget*)label_l,dsb);
          widgetType.append(3);         //3=>float
          paramToFilter.append(flist->at(i));
          paramStrings.append(tokens[0]);
          connect(dsb,SIGNAL(valueChanged(double)), this, SLOT(paramUpdateFloat(double)));
        }else{
          printf("unknown parameter type: %s\n",tokens[0].toLocal8Bit().data());
        }
      }
    }

  }
  vspacer = new QSpacerItem(1,1,QSizePolicy::Expanding, QSizePolicy::Expanding);
  mw->getUI()->laFilters->addItem(vspacer);
  built = true;
  manager->updatePriorities();
}

/*Run the configured filter pipeline once for the given CSI data.
 *The input data will be modified by the filters.
 */
void CSIFilterGUIManager::applyFilterPipeline(CSIData* data){
  mutex.lock();
  if(manager != NULL){
    manager->applyFilterPipeline(data);
  }
  mutex.unlock();

}

/* When an arrow has been clicked to expand or collapse the parameters of a filter*/
void CSIFilterGUIManager::expandArrowHandler(){
  QObject* senderObj = sender();
   bool found = false;;
   for(uint32_t i= 0; i< expandBtns.length(); i++){
       if(senderObj == expandBtns[i]){
         if(expandBtns[i]->isChecked()){
           expandScrollAreas[i]->setMaximumHeight(16777215);
           expandBtns[i]->setArrowType(Qt::ArrowType::DownArrow);
         }else{
           expandScrollAreas[i]->setMaximumHeight(0);
           expandBtns[i]->setArrowType(Qt::ArrowType::RightArrow);

         }
         found = true;
       }
   }
   if(!found){
     printf("sending widget not found\n");
   }

}

/* Called when a widget controlling an integer parameter is modified.*/
void CSIFilterGUIManager::paramUpdateInteger(int value){
  char buf[CSI_FILTER_NAME_PARMETER_STLEN];
  QObject* senderObj = sender();
  bool found = false;;
  for(uint32_t i= 0; i< paramWidgets.length(); i++){
      if(senderObj == paramWidgets[i]){
        sprintf(buf,"%d",value);
        paramToFilter[i]->setParameter(paramStrings[i].toLocal8Bit().data(),buf);
        found = true;
      }
  }
  if(!found){
    printf("sending widget not found\n");
  }
}

/* Called when a widget controlling a boolean parameter is modified.*/
void CSIFilterGUIManager::paramUpdateBoolean(bool activated){
  printf("paUpdateInt\n");
  char buf[CSI_FILTER_NAME_PARMETER_STLEN];
  QObject* senderObj = sender();
  bool found = false;;
  for(uint32_t i= 0; i< paramWidgets.length(); i++){
      if(senderObj == paramWidgets[i]){
        if(activated){
            sprintf(buf,"1");;
        }else{
          sprintf(buf,"0");;
        }
        paramToFilter[i]->setParameter(paramStrings[i].toLocal8Bit().data(),buf);
        found = true;
      }
  }
  if(!found){
    printf("sending widget not found\n");
  }
}


/* Called when a widget controlling an string parameter is modified.*/
void CSIFilterGUIManager::paramUpdateString(QString value){
  char buf[CSI_FILTER_NAME_PARMETER_STLEN];
  QObject* senderObj = sender();
  bool found = false;;
  for(uint32_t i= 0; i< paramWidgets.length(); i++){
      if(senderObj == paramWidgets[i]){
        sprintf(buf,"%s", value.toLocal8Bit().data());
        paramToFilter[i]->setParameter(paramStrings[i].toLocal8Bit().data(),buf);
        found = true;
      }
  }
  if(!found){
    printf("sending widget not found\n");
  }
}

/* Called when a widget controlling a floating point parameter is modified.*/
void CSIFilterGUIManager::paramUpdateFloat(double value){
  char buf[CSI_FILTER_NAME_PARMETER_STLEN];
  QObject* senderObj = sender();
  bool found = false;;
  for(uint32_t i= 0; i< paramWidgets.length(); i++){
      if(senderObj == paramWidgets[i]){
        sprintf(buf,"%f",value);
        paramToFilter[i]->setParameter(paramStrings[i].toLocal8Bit().data(),buf);
        found = true;
      }
  }
  if(!found){
    printf("sending widget not found\n");
  }
}

/* Iterate through all checkboxes for activating filters and update the status in the filter manager.*/
void CSIFilterGUIManager::updateActivations(){
  for(uint32_t i = 0; i < framesOuter.length(); i++){
    flist->at(i)->setActive(utilizeCbs[i]->isChecked());
   }
}

/* Iterate through all priorities of filters and update them in the filter manager.*/
void CSIFilterGUIManager::updatePriorities(){
  for(uint32_t i = 0; i < framesOuter.length(); i++){
    flist->at(i)->setPriority(sbPiorities[i]->value());
  }
  manager->updatePriorities();
}

//Destroy the GUI
void CSIFilterGUIManager::finalize(){
  if(built){
  mutex.lock();
  built = false;
  if(vspacer != NULL){
    mw->getUI()->laFilters->removeItem(vspacer);
    delete vspacer;
    vspacer = NULL;
  }
  //hspacer will be deleted with frameOuter
  for(uint32_t i=0;i<framesOuter.length(); i++){
    mw->getUI()->laFilters->removeWidget(framesOuter[i]);
    delete framesOuter[i];
  //    All other widgets will be autodeleted on the deletion of the parent (= frameOsuter)

  }
  //delete flist;               //don't delete - the manager will delete this automatically
  framesOuter.resize(0);
  sbPiorities.resize(0);
  utilizeCbs.resize(0);
  paramWidgets.resize(0);
  paramToFilter.resize(0);
  widgetType.resize(0);
  mutex.unlock();
  }
}


/* Set the path in which the compiled filter plugins are located.
 * path can either be an absolute or relative path*/
void CSIFilterGUIManager::setFilterPath(const QString& path){
  this->path = path;
}

CSIFilterGUIManager::~CSIFilterGUIManager(){
printf("FGM destr\n");
  //finalize();         //don't. UI will finalize all children already.
mutex.lock();
  delete manager;
  if(independentWindow != NULL){
    delete independentWindow;
    independentWindow = NULL;
  }
  mutex.unlock();
}

/*Toggle between "filters attached" or "filters detached" from the main window. */
void CSIFilterGUIManager::attachDetach(){
  if(independentWindow == NULL){
    independentWindow = new IndepWindowWidget();
    independentWindow->setCentralWidget(mw->getUI()->saFilters);
    independentWindow->show();
    independentWindow->setWindowTitle("Preprocessing Filter Pipeline");
    independentWindow->setWindowIcon(QIcon(":/icons/icons/preprocessing.svg"));
    independentWindow->setMinimumSize(800,600);
     connect(independentWindow, SIGNAL(aboutToClose()), this, SLOT(attachDetach()));
     mw->getUI()->actionDetachFilters->setChecked(true);
  }else{
      attachFilters();
  }
}

/*Attach the widget of filters, which had been previously detached, to the main window of the GUI.*/
void CSIFilterGUIManager::attachFilters(){
  mw->getUI()->lyFrameFilters->addWidget(mw->getUI()->saFilters);
  mw->getUI()->saFilters->show();
  independentWindow->close();
  //delete independentWindow;                   //no need to destroy - will destroy itself upon closing
  independentWindow = NULL;
  mw->getUI()->actionDetachFilters->setChecked(false);
}

//Returns a pointer to the filter manager */
CSIFilterManager* CSIFilterGUIManager::getFilterManager(){
  return manager;
}
