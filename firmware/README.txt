ParanoidStudio
==============

Firmwares:
----------
/lufa_usb2ax:         Default firmware, used to communicate with Dynamixels. TX and RX must be tied together (JP1 closed, which is by default).
/lufa_usb2ax_serial:  USB to Serial firmware. TX and RX must be untied by cutting the trace between the pads of JP1.

Se changelog.txt for changes to the default firmware.

How to install the firmware?
----------------------------
Upload the .hex file using Atmel FLIP or dfu-programmer.
For a firmware update, see online help at www.xevelabs.com.

How to build the firmware?
--------------------------
Requirements: ATMEL Studio 6.x, potentially LUFA extension for ATMEL studio (from the extension store) 
- open the solution and compile.
