/**
 *  main.c
 *
 *  Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>
 *
 *  This file is part of WirelessEye.
 *
 *  WirelessEye is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *  WirelessEye is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License along with WirelessEye. If not, see <https://www.gnu.org/licenses/>.
 */
#include "mainwindow.h"
#include <QApplication>
#include <stdio.h>
int main(int argc, char *argv[])
{
  if((argc > 1)&&(argc < 3)){
    printf("Usage: Either call without arguments, or,\n");
    printf("CSIGUI hostname [ChannelBandwith] [ChannelBandwidthDisplay] [ChannelBandwidthExport]\n");
    printf("E.g.: CSIGUI 192.168.0.8 80 20 80\n");
    exit(1);
  }
   QApplication a(argc, argv);
    MainWindow w;
    w.show();

    if(argc == 3){
        w.proccessCmdLineArguments(argv[1],argv[2]);
    }
    if(argc == 4){
        w.proccessCmdLineArguments(argv[1],argv[2], argv[3]);
    }
    if(argc == 5){
        w.proccessCmdLineArguments(argv[1],argv[2], argv[3], argv[4]);
    }
    return a.exec();
}
