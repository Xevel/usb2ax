/* ****************************************************************************
Copyright (C) 2014 Nicolas Saugnier (nicolas.saugnier [at] esial {dot} net),
                   Richard Ibbotson (richard.ibbotson [at] btinternet {dot} com)

Date   : 2012/05/06

Based on :
    - USBtoSerial project in the LUFA Library by Dean Camera (original file)
    - USB2AXSYNC by Richard Ibbotson (USB2AX optimizations, sync_read and Dynamixel communication)
    - arbotix_python ROS package and Arbotix Robocontroller firmware by Michael Ferguson (sync_read and part of the Dynamixel communication)

Original copyright notice : 
  Copyright 2013  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*******************************************************************************/


#ifndef AX_H_
#define AX_H_


#include "USB2AX.h"


#define AX_ID_DEVICE        0xFD
#define AX_ID_BROADCAST     0xFE

#define AX_CMD_READ_DATA    0x02
#define AX_CMD_RESET        0x06
#define AX_CMD_BOOTLOAD     0x08 
#define AX_CMD_SYNC_READ    0x84

#define AX_BUFFER_SIZE	            32
#define AX_SYNC_READ_MAX_DEVICES    31

// Error flags for status packets
#define AX_ERROR_INSTRUCTION   0x40  
#define AX_ERROR_CHECKSUM      0x10 
#define AX_ERROR_RANGE         0x08 
#define AX_ERROR_NONE          0x00


void axStatusPacket(uint8_t err, uint8_t* data, uint8_t nb_bytes);  
uint16_t axReadPacket(uint8_t length);
int axGetRegister(uint8_t id, uint8_t addr, uint8_t nb_bytes);
void sync_read(uint8_t* params, uint8_t nb_params);
void local_read(uint8_t addr, uint8_t nb_bytes);

#endif /* AX_H_ */