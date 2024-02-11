# Preparing The Raspberry Pi 4B using Nexmon #
## Possibilities ##
Before WirelessEye can be used, Nexmon needs to be installed on the Raspberry Pi.
There are multiple possibilities of setting up a Nexom-based Raspberry Pi. In particular
there are the following possibilities:

- Using the description on the [Nexmon CSI respository | https://github.com/seemoo-lab/nexmon_csi/]
  This description is the most comprehensive, which explains every aspect in more detail than the description in this document.
  Use this if you need custom setups, or if you indend to use a different hardware than a Raspberry Pi 4B.
  However, compared to the other options, it requires a higher degree of expertise to get things running.
- Using pre-built binaries
  You will find pre-build binaries of Nexmon on different GitHub repositories. This is potentially the least cumbersome and fast method to install Nexmon.
- Using this document
  In this tutorial, we describe how to install Nexmon on a Raspberry Pi 4B. We don't give any detailed explanations, but account for every single required step.
  Our method resolves multiple problems, e.g., the missing kernel headers for older Linux kernels.

## Problems of Installing Nexmon ##
While the installation of Nexmon has been described in Detail on the [Nexmon CSI respository | https://github.com/seemoo-lab/nexmon_csi/], 
a few new problems arise during installation, which have most likely not being present at the time at which the instructions were written.
In particular,
  - The tools on Nexmon require Python 2, whereas more recent Linux distribtions come with Python 3
  - The Linux kernel headers for the older kernels are no longer available using the apt package tool

## Downloading and installing a Distribution ##

We recommend downloading Raspbian [Lite 2022-01-28 | https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2022-01-28/]
