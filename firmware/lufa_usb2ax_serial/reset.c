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
/* Stuff about reset and bootloader */
#include "reset.h"
#include <avr/wdt.h>

// Reset/Hardware boot stuff
uint32_t Boot_Key ATTR_NO_INIT;     // variable untouched upon reset. Used to tell the controller if it should run the bootloader or not after a software reset.
#define FLASH_SIZE_BYTES            0x8000  // 32 K bytes of Flash in 32u2
#define BOOTLOADER_SEC_SIZE_BYTES   0x1000  // 2K words of bootloader
#define BOOTLOADER_START_ADDRESS    ((FLASH_SIZE_BYTES - BOOTLOADER_SEC_SIZE_BYTES)>>1)
#define MAGIC_BOOT_KEY              0xBADBEEF // arbitrary value, only used for comparition
  
// From "Entering the Bootloader via Software" in LUFA Library documentation
// http://www.fourwalledcubicle.com/files/LUFA/Doc/120219/html/_page__software_bootloader_start.html  
void Bootloader_Jump_Check(void){
    // If the reset source was the bootloader and the key is correct, clear it and jump to the bootloader
    if ((MCUSR & (1<<WDRF)) && (Boot_Key == MAGIC_BOOT_KEY)){
        Boot_Key = 0;
        ((void (*)(void))BOOTLOADER_START_ADDRESS)(); 
    }
}

void Jump_To_Reset(bool bootload){
    // If USB is used, detach from the bus
    USB_Detach();

    // Disable all interrupts
    cli();

    // Wait two seconds for the USB detachment to register on the host
    for (uint8_t i = 0; i < 128; i++) {
        _delay_ms(16);
    }

    // Set the bootloader key to the magic value and force a reset
    if (bootload) Boot_Key = MAGIC_BOOT_KEY;
    wdt_enable(WDTO_250MS);
    for (;;);
}


