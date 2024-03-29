Preparing The Raspberry Pi 4B using Nexmon 
==============================================

# Generic Information # 
Before WirelessEye can be used, Nexmon needs to be installed on a Raspberry Pi. This documents concerns setting up a fresh Raspberry Pi installation
including Nexmon.

## Possibilities ##
There are multiple possibilities of setting up a Nexon-based Raspberry Pi. In particular, we are aware of the following ones.

- Using the description on the [Nexmon CSI Respository](https://github.com/seemoo-lab/nexmon_csi/):
  This description is the most comprehensive one, which explains every aspect in more detail than the descriptions in this document.
  Use this if you need custom setups, or if you indend to use different hardware than a Raspberry Pi 4B.
  However, compared to the other options, it requires a higher degree of expertise to get Nexmon running.
- Using pre-built binaries:
  You will find pre-built binaries of Nexmon on different GitHub repositories. They potentially provide the least cumbersome and fast method to install Nexmon.
- Using this document:
  In this tutorial, we describe how to install Nexmon on a Raspberry Pi 4B. We don't give any detailed explanations, but account for every single required step.
  Our method resolves multiple problems, e.g., the missing kernel headers for older Linux kernels. It is designed to be as simple as possible, but restricted
  to a fresh installation of Nexmon on a Raspberry Pi 4B.

## Problems when Installing Nexmon ##
While the installation of Nexmon is described in detail on the [Nexmon CSI respository](https://github.com/seemoo-lab/nexmon_csi/), 
a few new problems arise during its installation, which have most likely not been present at the time at which the instructions were written.
In particular,
  - The tools contained in Nexmon require Python 2, whereas more recent Linux distribtions come with Python 3
  - The Linux kernel headers for the older kernels are no longer available using the apt package tool

# Downloading and Installing a Linux Distribution #

We recommend downloading Raspbian [Lite 2022-01-28](https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2022-01-28/). For 
creating a memory card for the Raspberry Pi, we recommend using the [Raspberry Pi Imager](https://www.raspberrypi.com/software/).
Make sure that SSH is activated.

# Preparing and Setting Up Nexmon #
Please follow the steps below only if you know what you are doing. They will make deep changes to your Raspberry, e.g., setting /usr/bin/python from Pyhton 3 Python 2. It will also
permanently replace your radio firmware with a patched one.
Hence, you can try them on a fresh installation of Raspbian. Never perform those steps below on a Raspberry Pi installation you intend to use for anything else than WiFi sensing.
Your system might not function properly anymore after having executed them. All commands are intended to be executed via SSH on the Raspberry Pi.

1) Become Superuser
   ```
   sudo su
   ```

3) Install all Required Packages
```
apt-get update 
apt install -y raspberrypi-kernel-headers git libgmp3-dev gawk qpdf bison flex make libncurses5-dev
apt install -y automake autoconf libtool texinfo curl
apt install -y git bc bison flex libssl-dev
apt install -y python2 tcpdump
```
3) Nexmon works with Python 2. Per default, Python 3 is launched when executing /usr/bin/python. We hence make /usr/bin/python point to Python 2.7
```
rm /usr/bin/python
ln -s /usr/bin/python2.7 /usr/bin/python
```

4) Our distribution needs the headers for an older kernel, which are no longer available using the apt package tool. We use
   [RPI-source](https://github.com/RPi-Distro/rpi-source) to download and install the old kernel headers.
```
wget https://raw.githubusercontent.com/RPi-Distro/rpi-source/master/rpi-source
python3 ./rpi-source
```

5) Nexmon can now be installed, as described in the [Nexmon CSI Respository](https://github.com/seemoo-lab/nexmon_csi/).
   We here describe a minimialistic set of required steps.
```
git clone https://github.com/seemoo-lab/nexmon.git
cd nexmon
cd buildtools/isl-0.10
./configure
make
make install
ln -s /usr/local/lib/libisl.so /usr/lib/arm-linux-gnueabihf/libisl.so.10
cd ../mpfr-3.1.4
autoreconf -i -f
./configure
make
make install
ln -s /usr/local/lib/libmpfr.so /usr/lib/arm-linux-gnueabihf/libmpfr.so.4
cd ../..
source setup_env.sh
make
cd utilities
make install
cd ..
make
cd patches/bcm43455c0/7_45_189
git clone https://github.com/seemoo-lab/nexmon_csi.git
cd nexmon_csi
make install-firmware
cd utils/makecsiparams
make
make install
cd ../..
```
6) Nexmon has now been installed and has patched your radio firmware. However, it will reload the original, unpatched firware after the next reboot. The following commands can be used to 
 permanently replace the original radio firmware with the patched one.

```
mv /lib/modules/`uname -r`/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/brcmfmac.ko /lib/modules/`uname -r`/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/brcmfmac_backup_nonexmon.ko
cp brcmfmac_5.10.y-nexmon/brcmfmac.ko /lib/modules/`uname -r`/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/
depmod -a
```

For your convenience, you can copy the following code into a file to be executed as a shell-script on your Raspberry Pi, which will carry out all steps described so far. It will in addition also download WirelessEye and will compile the CSI server.
Only use this script, if you are sure what you are doing (and what every step of this script does)!
```
#!/bin/bash

# WARNING: Don't run these commands until you fully understood what they are doing.
# In particular, be aware that...
# - They can severely damage your system
# - They will permanently replace your WiFi kernel module by the nexmon-patched one
# - They will download additional code
# - They are designed to be used on raspberry Pi 4B on only the following image: https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2022-01-28/
#    they will not work with any other system/distribution/hardware, and will potentially cause damage to any other hardware
# It might even damage your raspberry Pi!

echo "The commands in this script can damage your raspberry pi installation. Are you sure to you want to continue? Type 'yes' to continue'"
read result
if[$result != "yes"]
 exit 1
fi
set -e

apt-get update 

apt install -y raspberrypi-kernel-headers git libgmp3-dev gawk qpdf bison flex make libncurses5-dev

apt install -y automake autoconf libtool texinfo curl

apt install -y git bc bison flex libssl-dev

apt install -y python2 tcpdump

rm /usr/bin/python

ln -s /usr/bin/python2.7 /usr/bin/python

wget https://raw.githubusercontent.com/RPi-Distro/rpi-source/master/rpi-source

python3 ./rpi-source

git clone https://github.com/seemoo-lab/nexmon.git

cd nexmon

cd buildtools/isl-0.10

./configure

make

make install

ln -s /usr/local/lib/libisl.so /usr/lib/arm-linux-gnueabihf/libisl.so.10

cd ../mpfr-3.1.4

autoreconf -i -f

./configure

make

make install

ln -s /usr/local/lib/libmpfr.so /usr/lib/arm-linux-gnueabihf/libmpfr.so.4
 
cd ../..

source setup_env.sh

make

cd utilities

make install

cd ..

make
 
cd patches/bcm43455c0/7_45_189

git clone https://github.com/seemoo-lab/nexmon_csi.git

cd nexmon_csi

make install-firmware

cd utils/makecsiparams

make

make install

cd ../..

mv /lib/modules/`uname -r`/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/brcmfmac.ko /lib/modules/`uname -r`/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/brcmfmac_backup_nonexmon.ko

cp brcmfmac_5.10.y-nexmon/brcmfmac.ko /lib/modules/`uname -r`/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/

depmod -a

cd ../../../../..

git clone https://github.com/pkindt/WirelessEye.git

cd WirelessEye/CSIServer_ng

make
```

# Testing Nexmon #
The commands to start capturing are described in the [Nexmon CSI Repository](https://github.com/seemoo-lab/nexmon_csi/). For your convenience, we have modified, adjusted and extended them to create a script
that configures Nexmon to listen on a certain WiFi channel with a certain bandwith. You can copy the follwoing code into a file called <i>scan_wifi.sh</i> on your Raspberry Pi.

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

#command (mostly)according to: https://github.com/seemoo-lab/nexmon_csi/
iw phy phy0 interface add mon type monitor

ifconfig mon up


#command (mostly)according to: https://github.com/seemoo-lab/nexmon_csi/. One spatial stream, one core.
nexutil -Iwlan0 -s500 -l50 -b -v`makecsiparams -c $1/$2 -N 1 -C 1` 

```

You can now make this script executable by tying:
`chmod +x scan_wifi.sh`

To make Nexmon listen to channel 1 with a bandwith of 20 MHz, please type
`./scan_wifi.sh 1 20`

To see if packets are captured, type `tcpdump -i wlan0  | grep 5500`.
If there is traffic on channel 1, you should now see some captured WiFi frames.
After having [installed WirelessEye](/Readme.md), you can now also see the data in WirelessEye. Make sure to
set the CSI bandwith to 20 MHz in the settings tab of WirelessEye if your Nexmon-patched firmware captures using 20 MHz.
 
# Making WirelessEye Run after Reboot #
You might want to 1) make Nexmon capture and 2) run WirelessEye's CSI server after every reboot.
Assuming you have the scan_wifi.sh script in <i>/home/pi</i> and the CSIServer in /home/pi/WirelessEye/CSIServer_ng,
add the following two lines towards the end (but before the last line containing <i>exit</i>) of the file <i>/etc/rc.local</i> on your Raspberry Pi.
```
/home/pi/scan_wifi.sh 1 20
/home/pi/WirelessEye/CSIServer_ng/CSIServer &
```


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
