/*
 * CSIFilterGUIManager.h
 *
 * July 2021,  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#include "CSIFilterManager.h"
#include <QString>
#include <QVector>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QFrame>
#include <QFormLayout>
#include <QObject>
#include <QMutex>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QGridLayout>
#include "IndepWindowWidget.h"

#ifndef FILTERS_CSIFILTERGUIMANAGER_H_
#define FILTERS_CSIFILTERGUIMANAGER_H_
#define CSI_FILTER_PATH "src/filters"
class MainWindow;

/**
 * \brief A dynamic GUI for interactively selecting and configuring filter plugins.
 * This class creates a dynamic GUI to control all filter plugins. It interacts with the CSIFilterManager to control the filters.
 * For each filter, a title, a description, a priority spinbox and an activation checkbox is shown. In addition, every filter plugin may have
 * configurable parameters, for which widgets are created.
 */
class CSIFilterGUIManager: public QObject{

  Q_OBJECT

private:
  CSIFilterManager *manager;                    ///Pointer to a CSI filter manager, which keeps track of all filters and handls the communication with them
  QString path;                                 ///(Relative) path to where the compiled plugins are located
  QVector<QFrame*> framesOuter;                 ///Outer frames (= the outernmost border) of the elements of each filter
  QVector<QSpinBox*> sbPiorities;               ///Each filter has a spinbox to adjust its priority
  QVector<QCheckBox*> utilizeCbs;               ///Each filter has a checkbox to activate or deactivate it
  QVector<QToolButton*> expandBtns;             ///Each filter has an expand dropdown-arrow to extend the size of the displayed area, such that its parameters can be shown
  QVector<QScrollArea*> expandScrollAreas;      ///The extended area (i.e., when the dropdown arrow has been clicked) is scrollable
  QSpacerItem *vspacer;                         ///A vertical spacer that is placed below all filters. It ensures that the outer frames of all filters are aligned at the top of the window and not in the vertical middle
  MainWindow *mw;                               ///A pointer to the mainw indow
  QVector<CSIFilterObj*> *flist;                ///A pointer to a list of actual filters (i.e., the filters itself and not their graphical represenation)
  QMutex mutex;                                 ///A mutex for securing multithreaded operation
  QVector<QWidget*> paramWidgets;               ///Every filter can have different parameters, which implies different widgets of different types. This vector stores pointers to all of these widgets.
  QVector<uint32_t> widgetType;                 ///0=> string, 1=> bool, 2=> integer, 3=>floating point
  QVector<CSIFilterObj*> paramToFilter;         ///Every parameter widget- irrespective of the filter plugin to which it belongs, obtains an ID, which is identical to its index in paramWidgets.
                                                ///This vector contains a pointer to the corresponding filter object for each such ID.
                                                ///Hence, if we receive a signal from any  parameter widget (i.e, a spinbox that controls some filter parameter), we can identify to which filter it belongs.
                                                ///Since one filter might have multiple parameters, the same pointer might be stored in this vector multiple times in a row.
  QVector<QString> paramStrings;                ///Every parameter of a particular filter is identified by a string. This vectors contains all such strings sorted by the same order as paramWidgets.
                                                ///The number of elements in paramWidgets and paramStrings are equal, since every parameter has a widget.
  bool built;
  IndepWindowWidget* independentWindow;         ///The list of filters can be detached to get its own window. For this purpose, it is encapsulated in independentWindow. If the detached filters are closed, we also close the CSI GUI

  /**
   * Destroy the dynamic GUI
   */
  void finalize();

public:
  CSIFilterGUIManager();
  ~CSIFilterGUIManager();

  /**
   * Set a pointer tot he main window. Call this before using any functionality
   */
  void setMainWindow(MainWindow *mw);

  /**
   * Run the configured filter pipeline once for the given CSI data.
   * The input data will be modified by the filters.
   */
  void applyFilterPipeline(CSIData* data);

  /**
   * Attach the widget of filters, which had been previously detached, to the main window of the GUI.
   * */
  void attachFilters();

  /**
   * Returns a pointer to the filter manager
   */
  CSIFilterManager* getFilterManager();

public slots:

  /**
   *  Iterate through all checkboxes for activating filters and update the status in the filter manager.
   */
  void updateActivations();

  /**
   *  Iterate through all priorities of filters and update them in the filter manager.
   */
  void updatePriorities();

  /**
   *  Build the dynamic GUI based on the set of available filters
   */
  void buildGUI();

  /**
   * Called when a widget controlling an integer parameter is modified.
   */
  void paramUpdateInteger(int value);

  /**
   * Called when a widget controlling an string parameter is modified.
   */
  void paramUpdateString(QString value);

  /**
   *  Called when a widget controlling a boolean parameter is modified.
   */
  void paramUpdateBoolean(bool activated);

  /**
   *  Called when a widget controlling a floating point parameter is modified.
   */
  void paramUpdateFloat(double value);

  /** Set the path in which the compiled filter plugins are located.
   * path can either be an absolute or relative path
   */
  void setFilterPath(const QString& path);

  /**
   *  When an arrow has been clicked to expand or collapse the parameters of a filter
   */
  void expandArrowHandler();

  /**
   * Toggle between "filters attached" or "filters detached" from the main window.
   */
  void attachDetach();
};



#endif /* FILTERS_CSIFILTERGUIMANAGER_H_ */
