/*
 * filterManager.h
 *
 *  Jul. 2021, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CSIFILTERMANAGER_H_
#define CSIFILTERMANAGER_H_

#include <QString>
#include <QStringList>
#include <QVector>
#include "CSIFilterObj.h"
#include "CSIFilterManager.h"
#include "CSIData.h"
#include <QMutex>

/**
 * \brief A class handling the entire collection of CSI filter plugins
 *
 * Each CSI filter is represented by an object of type CSIFilter, and this class handles the collection of all such filters.
 * It is resposible for finding and loading all filters found in the filters folder, and for executing the filters and passing the data to each of them in the right order.
 */
class CSIFilterManager{
private:
  QString path;                         ///Path where the filter plugins (.cfi) files are stored
  QStringList fileNames;                ///A list of .cfi files that represent the filter plugins
  QVector<CSIFilterObj*> filters;       ///List of filter objects
  QVector<int> priorityVector;          ///A vector of filter IDs sorted by their execution order (which is given by the priorities)
  QMutex mutex;                         ///A mutex to protect the access from multiple different threads
public:

  CSIFilterManager();
  ~CSIFilterManager();

 /**
  * Identify all .cfi files in the corresponding path and store them as a list.
  * From this list, build the CSIFilter objects.
  */
 void loadFilterList(const QString& path);

 /**
  * Return a list of CSIfilter objects
  */
 QVector<CSIFilterObj*>* getFilterList();

 /**
  * Execute all filters according to their order.
  * data is a pointer to an object containing all CSI data. The filter can read it, and also modify the data.
  * WirelessEye is read the modified data back.
  */
 void applyFilterPipeline(CSIData* data);

 /**
  * Create the priorityVector by soring the filters by their execution order.
  */
 void updatePriorities();
};


#endif /* CSIFILTERMANAGER_H_ */
