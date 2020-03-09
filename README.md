# Intech Studio firmware repository
![img](https://intech.studio/assets/image/grid_magnetic_interface_1.jpg)

## Instructions
This document will walk you through the process of updating the firmware on your IntechGrid midicontroller. Please read through the  entire document beforehand in order to avid making any mistakes.
 
 The process will not require any special tools, you will only need
 - A computer running Windows, Mac or Linux
 - The Grid controller you wish to update
 - A USB Type C data cable
 
This tutorial will walk you through the following steps:
 - Enumerate in bootloader mode
 - Check the bootloader version on the controller
 - Update the bootloader if it is outdated
 - Upload the new application firmware

 
 
## Enumerating the bootloader

Holding down the mapmode button on the side of the controller while plugging in the USB C cable will boot the device in bootloader mode. All of the LEDs on the user interface should display a dim red color. This means that the device booted successfully, but the host computer has not recognized the device yet. You can release the mapmode button. Once the device is configured on the host, all of the LEDs turn green. On the host computer the grid controller now shows up as a removable storage device named "GRID".

## Checking the bootloader version
Once the controller succesfully enumerated as a removable storage device, you can check the version of the bootloader. If you browse the device you will find 3 files on it:
- CURRENT.UF2 is the binary image file of the current firmware
- INDEX.HTM is a readme file that redirects to this instruction page
- INFO_UF2.TXT contains the bootloader version and other basic information about the controller

Open INFO_UF2.TXT and compare the contents to the latest version quoted here:

```
UF2 Bootloader v3.3.0-6-g9262ab2-dirty SFHWRO
Model: Bootloader 20191211
Board-ID: SAMD51N20A-GRID
```

## Updating the bootloader
Please verify the Bootloader version before before uploading a new firmware image to your controller. The bootloader is maintained in a separate repository:
[grid-uf2](https://github.com/intechstudio/grid-uf2/releases/tag/v4.3.3-8)

## Updating the firmware
Download the firmware image from the repository and unzip the archive. The firmware binary is contained in a single .uf2 file. Once the bootloader is enumerated copy the firmware image to the removable storage device called GRID. After a successful update, the controller will reset and boot the newly installed firmware. Make sure to repeat this process for all of your Grid modules, making sure that they are all running the latest firmware.

## Stable Build
Grid firmware v1.0.0 is the latest stable relese. Plese visit the [releases](https://github.com/intechstudio/grid-fw/releases)
page for more information.

## Latest Build
Nightly builds of the Grid firmware are available for those who wish to experiment with the latest features.
Please check the related commit messages to learn more about the current build.

[![GitHub version](https://badge.fury.io/gh/intechstudio%2Fgrid-fw.svg)](https://github.com/intechstudio/grid-fw)
[![Only 32 Kb](https://badge-size.herokuapp.com/intechstudio/grid-fw/master/grid_toplevel/grid_toplevel_release.uf2)](https://github.com/intechstudio/grid-fw/tree/master/grid_toplevel/grid_toplevel_release.uf2)

