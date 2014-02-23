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

/** \file
 *
 *  Header file for USB2AX.c.
 */

#ifndef _USB_USB2AX_H_
#define _USB_USB2AX_H_

/* Includes: */
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include "Descriptors.h"

#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/Misc/RingBuffer.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

#define bitSet(value, bit)       ((value) |= (1UL << (bit))) 
#define bitClear(value, bit)     ((value) &= ~(1UL << (bit)))
#define min(x,y)                 ( ((x)<(y)) ? (x) : (y) )
#define max(x,y)                 ( ((x)>(y)) ? (x) : (y) )

/* Macros: */
/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
#define LEDMASK_USB_NOTREADY     0

/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
#define LEDMASK_USB_ENUMERATING  LEDS_LED2

/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
#define LEDMASK_USB_READY        LEDS_LED1

/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
#define LEDMASK_USB_ERROR        LEDS_ALL_LEDS

// Passthrough modes
#define AX_PASSTHROUGH  0   // pass data directly from serial to USB
#define AX_DIVERT       1   // keep data and don't pass it to USB

extern bool passthrough_mode; // determines if data from the USART is passed to the USB or diverted for local processing
extern uint8_t local_rx_buffer[];
extern volatile uint8_t local_rx_buffer_count;

extern volatile uint8_t usart_timer; // timer for RX read timeout
#define   USART_TIMEOUT  50   //  1ms
#define    SEND_TIMEOUT  4    //  80us
#define RECEIVE_TIMEOUT  100  //  2ms

//Dynamixel device Control table
#define MODEL_NUMBER_L      0x01
#define MODEL_NUMBER_H      0x42  // arbitrary model number that should not collide with the ones from Robotis
#define FIRMWARE_VERSION    0x04  // Firmware version, needs to be updated with every new release

// used for debug
#define hwb_setup bitSet(DDRD,PORTD7) 
#define hwb_up    bitSet(PORTD,PORTD7)
#define hwb_down  bitClear(PORTD,PORTD7)

/* Function Prototypes: */
void setup_hardware(void);
void init_serial(long baud);
void serial_write(uint8_t data);
void setRX(void);
void setTX(void);
void pass_bytes(uint8_t nb_bytes);
void process_incoming_USB_data(void);
void cdc_send_byte(uint8_t data);
void send_USB_data(void);

void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);

void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);
    
#endif

