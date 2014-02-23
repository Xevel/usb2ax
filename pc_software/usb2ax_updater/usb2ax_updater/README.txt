usb2ax_updater
==============

Makes it easy to put an USB2AX in bootloader mode and then flash it.

Uses dfu-programmer for flashing. A copy of the version it has been tested with is enclosed.

How to build:
- compile
- copy dfu-programmer next to the binary

How to use:
- run usb2ax_updater.exe
- plug an USB2AX
- select the corresponding COM port
- click "Run bootloader"
- select the new firmware to use (can be downloaded from www.xevelabs.com or built from sources)
- click "Flash USB2AX"