/*
 * IndepWindowWidget.h
 * Dummy wrapper widget for CSIFilterGUIManager.
 *
 *  Jul. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef INDEPWINDOWWIDGET_H_
#define INDEPWINDOWWIDGET_H_
#include <QMainWindow>

/**
 * \brief Dummy wrapper widget for CSIFilterGUIManager.
 *
 * The only purpose of this widget is to create a new, stand-anlone window that hosts CSIFilterGUIManager when being detached.
 *
 */
class IndepWindowWidget: public QMainWindow{

  Q_OBJECT

private:
  void closeEvent(QCloseEvent *event);
public:

  signals:
  /**
   * This signal is emitted when the detached window is getting closed, i.e., the user has clicked the close button.
   */
  void aboutToClose();

};




#endif /* INDEPWINDOWWIDGET_H_ */
