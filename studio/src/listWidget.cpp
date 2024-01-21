/*
 * listWidget.cpp
 *
 *  Jul. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */
#include "listWidget.h"
#include "checkableComboBox.h"
#include <QCloseEvent>
  listWidget::listWidget(QWidget* parent = NULL) :
    QListView(parent){
    cbx = NULL;
  }


void listWidget::leaveEvent(QEvent *event){
  if(cbx == NULL){
    return;
  }
  if(cbx->getDetached()){
    emit triggerAttachment();
  }
}


void listWidget::setCBX(checkableComboBox* cbx){
  this->cbx = cbx;
}
void listWidget::closeEvent(QCloseEvent *event){
  if((cbx != NULL)&&(cbx->getDetached())){
    cbx->attach();
  }
  event->ignore();

}
