# bgbootload
Secure OTA BLE bootloader for SiLabs EFR32BG chips and BGM111 modules
## Overview
This project was created to provide an open-source alternative to the closed source OTA (Over The Air) bootloader provided by Silicon Labs for their Blue Gecko chips and modules. This bootloader has been tested with the BGM111A BLE module, but should work with any of the EFR32BG series chips. Since this has been written, Silicon Labs has released v2.3 of the BLE stack, which includes a newer bootloader that does provide encryption. Its merits compared to this project have not been evaluated.

## Why an open source bootloader?
The SiLabs BLE v2.1.1.0 stack is closed source - the rationale for this is understandable, but most of the OTA bootloader is built into the stack, and uses a proprietary file format, and is thus quite opaque in its operation. It's not clear what encryption (if any) or end-to-end data validation is used, and there are difficulties in using it with GCC rather than the expensive IAR compiler. An open-source bootloader can be customised to individual requirements, uses a non-proprietary file format, and can use verifiable encryption methods.
## Configuration
The BLE stack used is V2.1.1.0. The stack is provided by SiLabs as a pre-compiled BLOB (binstack.o) and a symbol file (stack.a.) The stack must be loaded into flash at a fixed address (0x4000) and is used by the bootloader and any applications by making calls using the addresses in stack.a. What this means is that binstack.o and stack.a are linked with the bootloader, but the application program need only link with stack.a, since the stack BLOB is already loaded with the bootloader.

The SiLabs bootloader module is not used in this configuration.
## Project layout
This repository includes 3 separate projects.
* bootload - this is the code for the the bootloader itself, which incorporates the BLE stack
* utils - one main utility which converts compiled applications into an encrypted firmware file
* app - a sample application which can be flashed to a device using the bootloader

## Requirements
The projects all build using CMake, and are compatible with the CLion IDE, though this is not required. Testing has been done on MacOS only, but there should be little difficulty in building on Linux or Windows/Cygwin. Required tools and libraries are:
* An ARM GCC compiler - the version used is the arm-none-eabi variant, version 4.9_2015q3.
* OpenSSL libraries - used for AES encryption and SHA-256 hashing on the build host.
* libuuid - for manipulating UUIDs on the build host.
* Host C compiler - for building the utilities
* A bootloader client - this is an app running on e.g. Android which can send firmware files to the bootloader for flashing into RAM. A usable client is included in this repository as an APK - no source provided.

A set of configuration files provide the encryption key and BLE Service UUID used to define the bootload service. If you were to use this bootloader for your own applications, you should customise the encryption key and service UUID to ensure that your application can only be loaded onto a device running your bootloader. The bootloader sets the debug lock to prevent the decryption key being read from the chip.

## Memory usage
The SiLabs BLE stack reserves some memory for its own purposes. The stack code loads into flash from 0x4000 to 0x20D00, and reserves RAM from 20000000 to 20002FFF. The bootloader is linked with binstack.o to place the BLE stack in the correct location, and the bootloader code
itself is linked in the area from 0 to 4000, leaving flash memory from 21000 onwards available for the application. RAM from 20003000 is used by the application and the bootloader, but only one of these will be running at any given time, so all the RAM not reserved by the stack is available to the application.
## Boot sequence
On reset, the bootloader gains control. It checks for the presence of an application program (by a magic number located at a known address.) If an application is not present, or the reset was of a type other than power on and there is a special key in memory, it sets a flag which will prevent the application program from being run. This enables the bootloader to be loaded without an application program or allows the application to programmatically invoke the bootloader.
The bootloader then initialises the BLE stack, following which it regains control, then either starts the application program, or starts up its own BLE service. The bootloader service UUID is stored in a firmware file, so the bootloader client knows which service to look for. There are two BLE characteristics, one for control and one for data which are used to transfer the application to the bootloader. The bootloader decrypts the data using the CRYPTO hardware, and verifies a SHA-256 hash to confirm successful and complete programming. If all this is successful it will enable the application program, which will run on the next reset.
## Using the bootloader client app
An Android app is included in this repository as an APK. This can be sideloaded onto an Android device that supports BLE to load firmware files into a target device.  To test this out you should build the bootloader as provided here, and flash it to your device with JTAG/SWD. Build the sample app and copy the app.fmw file to your Android device. Then run the OTA DFU app you just sideloaded and using the "Choose Device" button find the advertising target device, called BG_OTA_DFU. Tap on this, then use the "Choose File" button to choose the firmware file (Dropbox can be handy for this.) Then the "Flash Device" button should be enabled, and pressing it will send the firmware file to the target. On successful completion it will reset, and the application program will start running, advertising itself as BG_APP.
## Re-entering DFU mode
Once you have your application running, to re-enter DFU mode it needs to do something - the sample app has a characteristic for this purpose, and the OTA DFU Android app recognises this and will show a button "Enter DFU mode" when it detects this characteristic. Tapping the button will cause the application to reboot to DFU mode, then the BG_OTA_DFU device will reappear.
## Customising
TODO

