/*
 * CheckableComboBox.cpp
 *
 * July 2021,  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <QStandardItem>
#include <QToolButton>
#include <QLayout>
#include <QWidget>
#include <QMoveEvent>
#include "checkableComboBox.h"
#include <stdio.h>
checkableComboBox::checkableComboBox(QWidget* parent) :
QFrame(parent)
{
  ly = new QHBoxLayout(this);
  this->setLayout(ly);

  //list of Q Standard item widgets
  lv = new listWidget(this);
  lv->setModel(&model);

  //expandable button
  expandBtn = new QToolButton(this);
  expandBtn->setArrowType(Qt::DownArrow);

  //the layout shall have the list on the left and the expand button right of it
  ly->addStretch();
  ly->addWidget(lv);
  ly->addWidget(expandBtn);

   model.setColumnCount(1);             //It's a one-column list

  //Insert a dummy item "No Filter" to the list
  QStandardItem* item;
  item = new QStandardItem();
  item->setText("No Filter");
  item->setCheckable(true);
  item->setCheckState(Qt::Checked);
  model.appendRow(item);

  //Set size and scrollbar policies
  this->setMaximumHeight(45);
  this->setMinimumHeight(45);
  this->setMinimumWidth(200);
  lv->setMinimumWidth(150);
  lv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  lv->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  lv->setCBX(this);
  this->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  lv->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::SubWindow);


  this->setFrameShape(QFrame::Box);                     //Should have a visible border
  detached = false;

  //connect signals
  connect(expandBtn,SIGNAL(clicked()),this,SLOT(toggleDetached()));
  connect(lv,SIGNAL(triggerAttachment()),this,SLOT(attach()));
  connect(&model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateFilters()));
  this->setToolTip("Only display/export frames from certain MAC addresses. The scope of the filter can be adjusted in the settings.");
  this->setStatusTip("Only display/export frames from certain MAC addresses. The scope of the filter can be adjusted in the settings.");

}

/*Add a previously unseen MAC address to the list of MAC addresses that can be selected*/
void checkableComboBox::addMAC(QString MAC){
  QStandardItem* item;
  if( model.findItems(MAC).length() == 0){
    item = new QStandardItem();
    item->setText(MAC);
    item->setCheckable(true);
    model.appendRow(item);
  }
}

/* Delete all MAC addresses in the list*/
void checkableComboBox::resetMACs(){
  model.clear();
  QStandardItem* item;                          //This is a standard item, i.e., a widget that can become checkable and can go to to a QStandardItemModel
   item = new QStandardItem();
   item->setText("No Filter");
   item->setCheckable(true);
   item->setCheckState(Qt::Checked);
   model.appendRow(item);
}
checkableComboBox::~checkableComboBox(){
if(!detached){
  ly->removeWidget(lv);
}
delete lv;
delete ly;              //will also delete spacer and expandBtn
}

/*Attach the previously detached widget -> the widget will again become part of the main window*/
void checkableComboBox::attach(){
  detached = false;
  lv->setMaximumWidth(16777215);
  expandBtn->setArrowType(Qt::DownArrow);
  lv->setMinimumHeight(0);
  ly->insertWidget(1,lv);
  lv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  lv->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::SubWindow);

  this->setMinimumWidth(200);
  this->setMaximumWidth(16777215);

}

/*Toggle the detach state. If detached, the widget expans and "hovers" over the main window to show additional MAC addresses.*/
void checkableComboBox::toggleDetached(){
  if(!detached){
    detached = true;
    lv->setMaximumWidth(lv->width());
    this->setMaximumWidth(this->width());
    this->setMinimumWidth(this->width());
    QPoint pos = lv->mapToGlobal(QPoint(0,0));
    lv->setWindowFlags(Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|Qt::Dialog);

    lv->move(pos);
    lv->setVisible(true);
    lv->setMaximumHeight(250);
    lv->setMinimumHeight(200);
    lv->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    expandBtn->setArrowType(Qt::UpArrow);
    lv->setFocusProxy(this);
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();
  }else{
    attach();
  }
}

/*Returns true, if the widget is detached, false, otherwise*/
bool checkableComboBox::getDetached(){
    return detached;
}

/*Returns a pointer on the list widget carrying the selected MAC address*/
listWidget* checkableComboBox::getLv(){
  return lv;
};

/*The widget has been moved*/
void checkableComboBox::moveEvent(QMoveEvent * event){
  if(detached){
    lv->move(lv->mapToGlobal(QPoint(0,0)) + (event->pos()-event->oldPos()));
  }
}


/*Returns true, if  certain MAC (given by the strinc "MAC") is seleceted*/
bool checkableComboBox::isMACActive(const QString& MAC){
  if(model.item(0)->checkState() == Qt::Checked){
    return true;
  }
  QList<QStandardItem *> list = model.findItems(MAC);
  if(list.length() == 0){
    return false;
  }
  if(list[0]->checkState() == Qt::Checked){
    return true;
  }
  return false;
}


/*Update the list of valid MAC addresses in the filter manager.
* It will create a QStringList and emit the updateMacFilterList() signal */
void checkableComboBox::updateFilters(){
    QStringList filterList;
    for(uint32_t i = 0; i < model.rowCount();i++){
      if(model.item(i)->checkState() == Qt::Checked){
        filterList.append(model.item(i)->text());
      }
    }
    emit updateMacFilterList(filterList);
}

