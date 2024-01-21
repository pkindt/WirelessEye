/*
 * listWidget.h
 *
 *  Jul. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LISTWIDGET_H_
#define LISTWIDGET_H_

#include <QListView>
#include <QWidget>
class checkableComboBox;                //fwd declaration

/**
 * \brief a customized QListView widget, which is used by the checkableComboBox class. The checkable combobox class will used this
 * as a list of items. Basiecally, this is just a wrapper around QListView which cateches some events.
 */
class listWidget: public QListView{

  Q_OBJECT

private:
  checkableComboBox* cbx;
public:
  listWidget(QWidget* parent);
  void leaveEvent(QEvent *event);
  void setCBX(checkableComboBox* cbx);
  void closeEvent(QCloseEvent *event);

  signals:
  void triggerAttachment();
};

#endif /* LISTWIDGET_H_ */
