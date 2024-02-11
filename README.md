WirelessEye - an Interactive Real-Time Workflow for WiFi-Based Sensing
==================================================================

2020 - 2022, Philipp H. Kindt <philipp.kindt@informatik.tu-chemnitz.de>

with contributions from:
- Cristian Turetta <cristian.turetta@univr.it> - Scripts to interface TensorFlow
- Florenc Demrozi <florenc.demrozi@univr.it> - Specification of timestamp format

This software tool accompanies our [paper](https://arxiv.org/abs/2204.01830) titled "WiFiEye -- Seeing over WiFi Made Accessible", to appear
at the WiSense workshop at PerCom 2024

License
-------
Copyright 2022, Philipp H. Kindt.
License: GPL v3, see [COPYING](COPYING)

When using WirelessEye together with Nexmon, please also pay attention to the Nexmon CSI license, see [here](https://github.com/seemoo-lab/nexmon_csi/blob/master/README.md).

If you use this software in your research, please consider citing our [paper](https://arxiv.org/abs/2204.01830).

Getting Started
---------------

# Prerequisites #
To perform WiFi-based sensing using WirelessEye, you need
- A Raspberry PI 4B [with kernel version 5.10](https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2022-01-28/) with Nexmon firmware (see below) 
- A Linux PC or Laptop (e.g., Linux Mint)
- The GNU C compiler (GCC) for PC and Raspberry Pi
- GNU make for PC and Raspberry Pi
- QT library version 5 for PC
- One or multiple standard WiFi APs to create some WiFi signals to capture

# Installation in a Nutshell #
To install WirelessEye, please follow the following steps. Note that your Raspberry Pi also needs to be prepared before running WirelessEye, if it is not already running Nexmon (see next section).

1. Download WirelessEye:
   `git clone https://github.com/pkindt/WirelessEye.git`
2. Go to the WirelessEye/studio subdirectory:
   `cd WirelessEye/studio`
3. Build WirelessEye:
    `make`
   If you get any error message during build, ensure that all prerequisites are fulfilled (i.e., the QT developer packages are installed).
   
5. Run WirelessEye:
   `./WirelessEye
   `
# Components #

WirelessEye consists of the following two pieces of software 

1. CSIServer_ng:
    A simple TCP server to be run on the Raspberry PI. 
    It will receive the UDP broadcasts from Nexmon and make them available over the network via TCP port 5501

2. WirelessEye Studio:
    A Qt GUI to display, record and export CSI data in real-time. To be run on any Linux PC from which the Raspberry Pi that runs the CSIServer is reachable over the network. 


# Preparing the Raspberry Pi #
1. Flash a Nexmon compatible Raspberry Pi OS version, you can download it from [here](https://downloads.raspberrypi.org/raspbian/images/raspbian-2020-02-14/).
   Never upgrade the kernel version.
   
2. Installing Nexmon
   Before using WirelessEye, the Raspberry Pi has to be prepared to run Nexmon firmware patches. For this purpose, configure and run the Nexmon CSI tools, as we describe in detail [here](doc/PreparingTheRaspi.md). Additional
   descriptions can be found in the [Nexmon CSI Repository](https://github.com/seemoo-lab/nexmon_csi).

3. Compiling and running CSIServer_ng

    WirelessEye contains a TCP server to access the CSI data from another computer, which is called CSIServer_ng. It needs to be
    compiled and run. For this purpose, do the follwoing on the Raspberry PI:
    1. Copy te CSIServer_ng folder to the Raspi
    2. In the CSIServer_ng folder, type `make`.
    3. Run the CSI server by the command `./CSIServer`
    4. (Optional) It is recommended to configure Nexmon and run the CSI Server at startup of the Raspberry Pi.
       See [here](https://learn.sparkfun.com/tutorials/how-to-run-a-raspberry-pi-program-on-startup/method-1-rclocal) for more installation how to do this.
       We recommend using the following script, which you can place into your home directory:
       ```
       #!/bin/bash
       if [ "$#" -ne 2 ]
       then
       echo "usage: ./scan_wifi.sh channel bandwith"
       echo "e.g.: ./scan_wifi.sh 1 20"
       exit 1
       fi

       rfkill unblock wlan
       ifconfig wlan0 up

       #command (mostly) according to: https://github.com/seemoo-lab/nexmon_csi/
       iw phy phy0 interface add mon type monitor
    
       ifconfig mon up

       #command (mostly) according to: https://github.com/seemoo-lab/nexmon_csi/. One spatial stream, one core.
       nexutil -Iwlan0 -s500 -l50 -b -v`makecsiparams -c $1/$2 -N 1 -C 1` 
       ```
       Safe this script as <i>scan_wifi.sh</i> in your home-directory (e.g., /home/pi) and make it executable using the follwing command:
       ```
       chmod +x scan_wifi.sh
       ```
       You can now make nexmon capture e.g., on channel 1 using a bandwith of 20 MHz as follows.
       ```
       ./scan_wifi.sh 1 20
       ```
       To make Nexmon start capturing during startup and to start the CSI server, put the following two lines into your file <i>/etc/rc.local</i> (we recommend putting this code towards the end of the file, right before the last line containing <i>exit</i>):
       ```
       /home/pi/scan_wifi.sh 1 20
       /home/pi/WirelessEye/CSIServer_ng/CSIServer &
       ```
       This code assumes that scan_wifi.sh lies in <i>/home/pi</i> and the CSI Server in <i>/home/pi/WirelessEye/CSIServer_ng</i>. Adjust these paths if necessary.

5. Compiling and Running WirelessEye Studio on your PC/laptop
   1. In the folder `WirelessEye`, type `make`
   2. Run WirelessEyeStudio by typing `./WirelessEye`
   3. (Optional) It is possible to also run this directly on the Raspberry Pi. For this purpose, you need to install the QT library developer package onto your Raspberry Pi: 
      ```
      apt install qtbase5-dev 
      ```
# Using WirelessEye Studio #

You can find a good overview on the functionality provided by WirelessEye in our paper. 
When hovering the mouse over some object, a quick description in shown in the status bar.

Here's a quick how-to on using WirelessEye:

1. In the tab _settings->connection_, enter the IP address of hostname of your Raspberry Pi. In _settings->CSI, adjust the bandwith you selected when running Nexmon, e.g., via scan_wifi.sh 
2. In the tab _visualization_, click _connect_. Upon success, the text in the button will change to "connected" and data is being streamed from the Raspberry Pi
3. In the _visualization tab_, empirically select the range of CSI values in which you can see your events of interest
4. When pressing the _record_ button, the CSI data is stored into a file. The filename can either be selected in the _settings_ tab, or will be automatically assigned based on the time and date. 
   The actual filename is shown in the console when recording starts.
   WirelessEye supports 3 different formats for recording, which can be selected in the _settings_ tab. The actual file format is documented in [doc/fileFormats.pdf](doc/fileFormats.pdf).
5. Real-Time export of the CSI data, e.g., to a classifier, can be initiated in the _Real-Time Classification_ tab. More on this is written below in a separate section.

# Real-Time Export #
WirelessEye can stream the preprocessed CSI data to any external program, e.g., a classifier that uses machine learning methods. 
This is controlled form the _Real-Time Classification_ tab.
Here, any external program can be executed. It is possible to specify the command to be executed and its command line parameters.

The CSI data is written to the standard input of the launched executable. The format is the _Simple CSI format_, which is documented in [doc/fileFormats.pdf](doc/fileFormats.pdf).
The classifier can write its classification results to its standard output, which is imported back into WirelessEye. WirelessEye can annotate these results in the real-time visualization.
Though it can only launch one executable, this executable can run multiple classifier. WirelessEye supports displaying the results of multiple classifiers. Hence, the launched executable needs to launch additional
classifiers, or needs to include multiple of them in one executable. The data fromat for importing results back into WirelessEye is also documented in [doc/fileFormats.pdf](doc/fileFormats.pdf).

A pair of scripts for accessing tensorflow to 1) train  a classifier using previously recorded data and 2) perform live classification using the real-time export mechanism is included in WirelessEye. 
They are described below.

# Building Classification Models using Tensorflow #

The follwoing two python scripts for creating and using a classification model using the Tensorflow machine learning framework are provided along with WirelessEye.
 
 - scripts/model_generation.py:
   This script is used for creating a machine learning model, e.g., for recognizing human activity using CSI data.
   It contains multiple configuration parameters that need to be adjusted in the script. 
 - scripts/realtime_classification.py:
   This script can be executed from with WirelessEye to perform real-time classification. It reads the CSI data in real-time and queries a previously    	
   generated model

Prerequisites
-------------
We recommend Python 3.8. The following python modules need to be installed for these scripts to work.
(The first three are included in Linux Mint, the others need to be installed via pip)
 - matplotlib
 - pandas
 - seaborn
 - tensorflow (type `python3.8 -m pip install tensorflow` in a terminal for installation)
 - imblearn (type `python3.8 -m pip install imblearn` in a terminal for installation)
 
Using model_generation.py
-------------------------
`model_generation.py` provides multiple configurable parameters, which need to be adjusted first.
A (mostly reasonable) default value is assigned to each of them. The parameters to be adjusted can be found under the comment _# Settings_ in the 
script code. The following parameters need to be adjusted:
   - _sampling_frequency_: The sampling frequency of the CSI data, i.e., the (average) number of WiFi frames per second.
   - _seconds_: A time window (in seconds) of CSI data to be fed into the model
   - _overlap_: The time by which two adjacent CSI data time windows overlap
   - _training_epochs_: The number of training epochs 
   - _labels_: An array of strings that contain all labels  
   - _path_to_file_: Path to a file recorded in WirelessEye to be analyzed. It needs to be in the simple CSV format.
 
After adjusting these options, the script needs to be executed repeatedly with different previously recorded CSI files. Different files are specified by changing the _path_to_file_ parameter each time. It is assumed that the data contained in a single CSV file belongs to one specific label. 

Starting from a file like this example:

| timestamp   | amplitude_sub<sub>1</sub>  |  amplitude_sub<sub>2</sub> |  amplitude_sub<sub>3</sub> | ... | amplitude_sub<sub>n</sub> | 
| :---:       |:---:|:---:|:---:|:---:|:---:|
|2021-10-22 14:29:39.314396|1.3608 | 1.3839 | 1.4003 | ... | 1.4230 |
|2021-10-22 14:29:39.416798|1.3718 | 1.3932 | 1.4565 | ... | 1.4887 |
|...|...|...|...|...|...|
|2021-10-22 14:32:23.519207|1.3527 | 1.3794 | 1.4233 | ... | 1.4434 |
|...|...|...|...|...|...|

to put labels on a CSI record, you just have to add a column with the desired label. The label marks the activity/status of the environment in a certine time window. The label will be used by the model to recognize the CSI pattern for the respective activity/status of the environment described by the label. The final file sholud be similar to:

| timestamp   | amplitude_sub<sub>1</sub>  |  amplitude_sub<sub>2</sub> |  amplitude_sub<sub>3</sub> | ... | amplitude_sub<sub>n</sub> | label |
| :---:       |:---:|:---:|:---:|:---:|:---:| :---:|
|2021-10-22 14:29:39.314396|1.3608 | 1.3839 | 1.4003 | ... | 1.4230 |walking|
|2021-10-22 14:29:39.416798|1.3718 | 1.3932 | 1.4565 | ... | 1.4887 |walking|
|...|...|...|...|...|...|walking|
|2021-10-22 14:32:23.519207|1.3527 | 1.3794 | 1.4233 | ... | 1.4434 |sitting|
|...|...|...|...|...|...|sitting|

Make sure that data is recorded in the *simple* CSV format. The model is stored in a file called `model.h5`. The model stored in this file can be queried using `realtime_classification.py`, which is described next.

Using realtime_classification.py
--------------------------------
`realtime_classification.py` is called from within WirelessEye Studio. This can be done in the 'Real-Time Classification' tab. Use
`python3.8` as the executable and `realtime_classification.py` as the argument. Adjust the number of classes to the length of the _labels_ array in 
 `model_generation.py` (for the default value in the script, this would be 4). 
 The script will assign a counting number to the sting lables, which are imported back into WirelessEye. E.g., the first label in the _labels_ string of
 `model_generation.py` will be _0_, the second one _1_, the third one _2_,... 

After `realtime_classification.py` has been executed, the _Classifier Output_ plot will be available in WirelessEye Studio and in sync with the CSI data.

# Developing Plugins #
WirelessEye supports plugins to process CSI data. A plugin is a simple C-file. It is complied independently from WirelessEye. 
Developing filter plugins is simple and can be learned within minutes. A filter plugin has to provide a couple of functions, which are being called trough WirelessEye using dynamic linking.

The only code a plugin needs to import from WirelessEye is the structure _CSIData_ from [studio/src/CSIData.h](studio/src/CSIData.h).
A function called _filter_run()_ obtains a pointer to a filled _CSIData_ structure. The processing plugin can modify the data in this structure. WirelessEye studio will read the changes back after _filter_run()_ has finished.

Learning how a filter is written can be done by examining the extensively commented file [studio/src/filters/sample_filter.c](studio/src/filters/sample_filter.c).
This file implements a fully-functional sample filter with minimalistic code effort. Each function that is  contains a detailed description as a comment. It can also serfe as a sceleton for writing a custom filter.
With [studio/src/filters/sample_filter.c](studio/src/filters/sample_filter.c), it is straight-forward two develop a custom plugin - no additional documentation needed.

# Developing for WirelessEye studio #
If you want to modify or extend WirelessEyeStudio, you find a full Doxygen documentation of all files of WirelessEye Studio in the [doc](doc) subdirecory.
To build this documentation, go to the _doc/_ subdirectory. Then type _doxygen_ for building the documentation. Next, go to the _doc/latex/_ subdirectory and type _make_ to compile a PDF document.
Next, you find a documentation in the file _doc/latex/refman.pdf_.

Please also let us know if you'd like to contribute some code to our repository.

# Acknowledgements #
Thanks to Alejandro Masrur, Florenc Demrozi, Cristian Turetta, Graziano Pravadelli, Samarjit Chakraborty and Shengjie Xu for the pleasant interactions related to WirelessEye.

# Known Issues #
- (Uncofirmed) In a longer experiment (continuously recording for 70+ hours), the resulting data file appeared to be malformed. It has yet to be confirmed whether this can be reproduced. 

# Additional Ressources #
- P Kindt, C. Turetta, F. Demrozi, A. Masrur, G. Pravadell, and S. Chakraborty.
  “Wirelesseye: - seeing over wifi made accessible,” in
  2024 IEEE International Conference on Pervasive Computing and Communications Workshops and other Affiliated Events (PerCom Workshops)
- F. Gringoli, M. Schulz, J. Link, and M. Hollick.
  “Free your CSI: A channel state information extraction platform for modern Wi-Fi chipsets”
  in International Workshop on Wireless Network Testbeds, Experimental
  Evaluation & Characterization (WiNTECH), 2019, p. 21–28.
- [ M. Schulz, D. Wegemer, and M. Hollick. (2017) Nexmon: The C-based firmware patching framework (Repository)](https://github.com/seemoo-lab/nexmon)
- [Nexmon CSI Respository](https://github.com/seemoo-lab/nexmon_csi/)
  
