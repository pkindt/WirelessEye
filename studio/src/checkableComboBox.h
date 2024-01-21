/*
 * CheckableComboBox.h
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
#ifndef CHECKABLECOMBOBOX_H_
#define CHECKABLECOMBOBOX_H_

#include <QComboBox>
#include <QListView>
#include <QObject>
#include <QStandardItemModel>
#include <QWidget>
#include <QSpacerItem>
#include <QToolButton>
#include <QHBoxLayout>
#include <QFrame>
#include <QStringList>
#include "listWidget.h"



/**
 * \brief A checkable combobox for selecting MACs to be included.
 *
 * The widget has two modes, viz., "attached" and "detached". When attached, the widget is part of the main window.
 * When detached, it hovers above the main window and becomes larger.
 *
 * The list of MAC addresses is displayed by a listWidget. This listWidget displays QStandardItems, that are
 * organized by a QStandardItemModel. See https://doc.qt.io/qt-5/qstandarditemmodel.html for details.
 */

class checkableComboBox: public QFrame{

  Q_OBJECT

private:
  QStandardItemModel model;                     ///Some sort of container to store abstract "items". See https://doc.qt.io/qt-5/qstandarditemmodel.html
  listWidget *lv;                               ///A list widget to show multiple widgets underneath each other. It will carry the QStandardItemModel
  QHBoxLayout *ly;                              ///A horizontal layout to show the list of widgets (lv) and the dropdown button underneath
  QToolButton *expandBtn;                       ///An dropdown button to expand the widget
  bool detached;                                ///When detached (dropdown button pressed), the widget will expand "hover" over the main window

public:

  checkableComboBox(QWidget* parent=NULL);
  ~checkableComboBox();

  /**
   * Returns true, if the widget is detached, false, otherwise
   */
  bool getDetached();

  /**
   * Returns a pointer on the list widget carrying the selected MAC address
   */
  listWidget* getLv();

  /*
   * The widget has been moved
   */
  void moveEvent(QMoveEvent * event);

  /**
   * Returns true, if  certain MAC (given by the strinc "MAC") is seleceted
   */
  bool isMACActive(const QString& MAC);

public slots:

  /**
   * Add a previously unseen MAC address to the list of MAC addresses that can be selected
   */
  void addMAC(QString MAC);

  /**
   * Toggle the detach state. If detached, the widget expans and "hovers" over the main window to show additional MAC addresses.
   */
  void toggleDetached();

  /**
   * Attach the previously detached widget -> the widget will again become part of the main window
   */
  void attach();

  /**
   * Update the list of valid MAC addresses in the filter manager.
   * It will create a QStringList and emit the updateMacFilterList() signal
   */
  void updateFilters();

  /**
   *  Delete all MAC addresses in the list
   */
  void resetMACs();

  signals:

  /**
   * "export" the lsit of selected MAC addresses
   */
  void updateMacFilterList(QStringList);

};
#endif /* CHECKABLECOMBOBOX_H_ */
