#!/usr/bin/env python2.6

# This script send a command to the USB2AX to put it in bootloader mode.
# Upon running this script, the USB2AX LEDs should turn off and it should re-enumerate
# as ATmega32u2 DFU. It can then be re-programed using the GUI application Atmel FLIP
# or it underlying cli program batchisp.
#
# Remark: starting at v04, simply opening the port at 1200bps and closing it is enough. This script works for all versions.
# Nicolas Saugnier - 2014

#************************************************************************************
# Please set COM_PORT to the name or path to the USB2AX you wish to put in bootloader mode
#Windows example
#COM_PORT = "COM7"
#Linux examples
#COM_PORT = "/dev/ttyACM0"
#COM_PORT = "/dev/serial/by-id/usb-Xevelabs_USB2AX_64033353031351600170-if00"

COM_PORT = "COM7"

#************************************************************************************
# TODO :
# - version that accepts cli args...
# - proper copyright notice
# - error handling (serial package, serial port...)

import serial
bootload_cmd = ''.join(chr(x) for x in [0xff, 0xff, 0xfd, 0x02, 0x08, 0xf8])
ser = serial.Serial(COM_PORT, baudrate=1200)
ser.write(bootload_cmd)
ser.close()
