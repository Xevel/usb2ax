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
 *  Main source file for the USB2AX project. This file contains the main tasks of
 *  the application and is responsible for the initial application hardware configuration.
 *  The USB2AX sends data from the USB to the serial port and sends back data coming
 *  from the serial port to the USB. It can also understand some instructions to perform
 *  higher-level task, like reading data from multiple servos by itself.
 */

#include "USB2AX.h"
#include "AX.h"
#include "reset.h"
#include <util/delay.h>

/*TODO list for the firmware:
- make the timeout lengths R/W parameters. Maybe set a minimum to avoid blocking the input...
- instead of arbitrarily limiting the number of bytes and of servos in sync_read, limit bytes*servos instead...
- there's been reports of the LED turning off when the computer goes to sleep and not turning on again when woke up...
- try to fill the IN bank as the bytes arrives instead of at the last moment, see if there is something to be gained...
- proper Doxygen doc instead of a bunch of random comments here and there
- rework the timers to have something (maybe)faster and less ugly
Done:
* baud rate: use frequency doubling if needed (example : 57200 should become 57142, not around 58823)
*/

/** LUFA CDC Class driver interface configuration and state information. */
USB_ClassInfo_CDC_Device_t USB2AX_CDC_Interface =
    {
        .Config =
        {
            .ControlInterfaceNumber         = 0,
            .DataINEndpoint                 =
            {
                .Address                = CDC_TX_EPADDR,
                .Size                   = CDC_TXRX_EPSIZE,
                .Banks                  = 1,
            },
            .DataOUTEndpoint                =
            {
                .Address                = CDC_RX_EPADDR,
                .Size                   = CDC_TXRX_EPSIZE,
                .Banks                  = 1,
            },
            .NotificationEndpoint           =
            {
                .Address                = CDC_NOTIFICATION_EPADDR,
                .Size                   = CDC_NOTIFICATION_EPSIZE,
                .Banks                  = 1,
            },
        },
    };

// Sending data to USB
RingBuffer_t	ToUSB_Buffer;  // Circular buffer to hold data before it is sent to the host.
static uint8_t  ToUSB_Buffer_Data[128]; // Underlying data buffer for \ref ToUSB_Buffer, where the stored bytes are located.
uint8_t needEmptyPacket = false; // flag used when an additional, empty packet needs to be sent to properly conclude an USB transfer

// Buffer used when diverting USART data for local processing
uint8_t	local_rx_buffer[AX_BUFFER_SIZE];
volatile uint8_t local_rx_buffer_count = 0;

// Pass through 
bool passthrough_mode = AX_PASSTHROUGH; // determines if data from the USART is passed to the USB or diverted for local processing

// Dynamixel packet parser stuff
// AX receive states
#define AX_SEARCH_FIRST_FF   0
#define AX_SEARCH_SECOND_FF  1
#define AX_SEARCH_SYNC_ID    2
#define AX_SEARCH_LENGTH     3
#define AX_SEARCH_COMMAND    4
#define AX_SEARCH_RESET      5
#define AX_SEARCH_BOOTLOAD   6
#define AX_GET_PARAMETERS    7
#define AX_SEARCH_READ       8

uint8_t ax_state = AX_SEARCH_FIRST_FF; // current state of the Dynamixel packet parser state machine
uint16_t ax_checksum = 0;
uint8_t rxbyte[AX_SYNC_READ_MAX_DEVICES + 8]; // buffer where currently processed data are stored when looking for a Dynamixel packet, with enough space for longest possible sync read request
uint8_t rxbyte_count = 0;   // number of used bytes in rxbyte buffer


uint8_t needs_bootload = false; // In EVENT_CDC_Device_LineEncodingChanged, this flag is set when the baudrate is at a pre-defined value


// timeout
volatile uint8_t receive_timer = 0; // timer for Dynamixel packet reception from USB
volatile uint8_t    send_timer = 0; // timer for sending data to the PC
volatile uint8_t   usart_timer = 0; // timer for RX read timeout


int main(void){
    setup_hardware();
    //hwb_setup;
	RingBuffer_InitBuffer(&ToUSB_Buffer, ToUSB_Buffer_Data, sizeof(ToUSB_Buffer_Data));
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
    sei();

    // enable pull-up on the TX pin to prevent spurious signal when in RX mode.
    bitClear(DDRD,3);
    bitSet( PORTD,3);
    
    for (;;){
        while (USB_DeviceState != DEVICE_STATE_Configured); // wait for device to be configured

        // get bytes from USB
        process_incoming_USB_data();
        
        send_USB_data();
        
        USB_USBTask();
    }
}

void cdc_send_byte(uint8_t data){ // TODO inline ?
	// Careful when calling this outside of the RX ISR: we should NEVER call it when the RX ISR is enabled (risk of corruption of the buffer)
	// So the best way to do it, if it was needed to call it while RX interrupt is enabled, would be to disable it, call this, then re-enable it immediatly... and hope no char was lost
	// BUT anyway it should never happen : it would just corrupt the datastream to have multiple sources writing to it at the same time... so this warning is useless. 
	RingBuffer_Insert(&ToUSB_Buffer, data);
	send_timer = 0;
}

void send_USB_data(void){
	// process outgoing USB data
	Endpoint_SelectEndpoint(CDC_TX_EPADDR); // select IN endpoint to restore its registers
	if ( Endpoint_IsINReady() ){ // if we can write on the outgoing data bank
		uint8_t BufferCount = RingBuffer_GetCount(&ToUSB_Buffer);
		
		if (BufferCount) {
			// if there are more bytes in the buffer than what can be put in the data bank OR there are a few bytes and they have been waiting for too long
			if ( BufferCount >= CDC_TXRX_EPSIZE || send_timer > SEND_TIMEOUT ){
				send_timer = 0;
				
				// load the IN data bank until full or until we loaded all the bytes we know we have
				uint8_t nb_to_write = min(BufferCount, CDC_TXRX_EPSIZE );					
				while (nb_to_write--){
                	uint8_t Data = RingBuffer_Remove(&ToUSB_Buffer);
					Endpoint_Write_8(Data);
				}
				
				// if the bank is full (== we can't write to it anymore), we might need an empty packet after this one
				needEmptyPacket = ! Endpoint_IsReadWriteAllowed();
				
				Endpoint_ClearIN(); // allow the hardware to send the content of the bank
			}
		} else if (needEmptyPacket) {
			// send an empty packet to end the transfer
			needEmptyPacket = false;
			Endpoint_ClearIN(); // allow the hardware to send the content of the bank
		}
		
	}
}

void _blih(void){ // TODO find a name for this function!
    // if the last byte read is not what it's expected to be, but is a 0xFF, it could be the first 0xFF of an incoming command
    if (rxbyte[rxbyte_count-1] == 0xFF){
		// we trade the latest 0xFF received against the first one that is necessarily in the first position of the buffer
        pass_bytes(rxbyte_count-1); // pass the discarded data, except the last 0xFF
        ax_state = AX_SEARCH_SECOND_FF;
        rxbyte_count = 1; // keep the first 0xFF in the buffer
		receive_timer = 0;
    } else {
        pass_bytes(rxbyte_count);
        ax_state = AX_SEARCH_FIRST_FF;
    }
}


#define PACKET_FIRST_0XFF  0
#define PACKET_SECOND_0XFF 1
#define PACKET_ID          2
#define PACKET_LENGTH      3
#define PACKET_INSTRUCTION 4

void process_incoming_USB_data(void){
	uint8_t USB_nb_received = CDC_Device_BytesReceived (&USB2AX_CDC_Interface);
	
	if (USB_nb_received>0){
        setTX();
        
	    for( uint8_t i = 0; i < USB_nb_received ; i++ ){
            switch (ax_state){
                case AX_SEARCH_FIRST_FF:
                    rxbyte[PACKET_FIRST_0XFF] = CDC_Device_ReceiveByte(&USB2AX_CDC_Interface);
                    if (rxbyte[PACKET_FIRST_0XFF] == 0xFF){
                        ax_state = AX_SEARCH_SECOND_FF;
                        rxbyte_count = 1;
                        receive_timer = 0;
                    } else {
                        serial_write(rxbyte[0]);
                    }
                    break;
                            
                case AX_SEARCH_SECOND_FF:
                    rxbyte[rxbyte_count++] = CDC_Device_ReceiveByte(&USB2AX_CDC_Interface);
                    if (rxbyte[PACKET_SECOND_0XFF] == 0xFF){
                        ax_state = AX_SEARCH_SYNC_ID;
                        receive_timer = 0;
                    } else {
                        pass_bytes(rxbyte_count);
                        ax_state = AX_SEARCH_FIRST_FF;
                    }
                    break;
                            
                case AX_SEARCH_SYNC_ID:
                    rxbyte[rxbyte_count++] = CDC_Device_ReceiveByte(&USB2AX_CDC_Interface);
                    if (rxbyte[PACKET_ID] == AX_ID_DEVICE || rxbyte[PACKET_ID] == AX_ID_BROADCAST ){
                        ax_state = AX_SEARCH_LENGTH;
                        receive_timer = 0;
                    } else if (rxbyte[PACKET_ID] == 0xFF){
						rxbyte_count--;
						serial_write(0xFF);
					    receive_timer = 0;
					}else {
                        _blih();
                    }
                    break;
                            
                case AX_SEARCH_LENGTH:
                    rxbyte[rxbyte_count++] = CDC_Device_ReceiveByte(&USB2AX_CDC_Interface);
                    if (rxbyte[PACKET_LENGTH] > 1 && rxbyte[PACKET_LENGTH] < (AX_SYNC_READ_MAX_DEVICES + 4)){  // reject message if too short or too big for rxbyte buffer)
                        ax_state = AX_SEARCH_COMMAND;
                        receive_timer = 0;
                    } else {
                        _blih();
                    }
                    break;
                            
                case AX_SEARCH_COMMAND:
                    rxbyte[rxbyte_count++] = CDC_Device_ReceiveByte(&USB2AX_CDC_Interface);
                    if (rxbyte[PACKET_INSTRUCTION] == AX_CMD_SYNC_READ){
                        ax_state = AX_GET_PARAMETERS;
                        ax_checksum =  rxbyte[PACKET_ID] + AX_CMD_SYNC_READ + rxbyte[PACKET_LENGTH];
                        receive_timer = 0;
                    } else if(rxbyte[PACKET_ID] == AX_ID_DEVICE){ 
				        if (rxbyte[PACKET_INSTRUCTION] == AX_CMD_RESET){
                            ax_state = AX_SEARCH_RESET;
                            LEDs_TurnOnLEDs(LEDS_LED2);
                            receive_timer = 0;
                        } else if (rxbyte[PACKET_INSTRUCTION] == AX_CMD_BOOTLOAD){
                            ax_state = AX_SEARCH_BOOTLOAD;
                            receive_timer = 0;
                        } else if (rxbyte[PACKET_INSTRUCTION] == AX_CMD_READ_DATA) {
				            ax_state = AX_GET_PARAMETERS;
                            ax_checksum = AX_ID_DEVICE + AX_CMD_READ_DATA + rxbyte[PACKET_LENGTH];
						    receive_timer = 0;
                        } else {
                            _blih();
                        }
				    } else { 					
                        _blih();
                    }
                    break;
                            
                case AX_SEARCH_RESET:
                    rxbyte[5] = CDC_Device_ReceiveByte(&USB2AX_CDC_Interface);
                    if (((AX_ID_DEVICE + 2 + AX_CMD_RESET + rxbyte[5]) % 256) == 255){
                        LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
                        Jump_To_Reset(false);
                    } else {
                        _blih();
                    }
                    break;
                        
                case AX_SEARCH_BOOTLOAD:
                    rxbyte[5] = CDC_Device_ReceiveByte(&USB2AX_CDC_Interface);
                    if (((AX_ID_DEVICE + 2 + AX_CMD_BOOTLOAD + rxbyte[5]) % 256) == 255){
                        LEDs_TurnOffLEDs(LEDS_LED2);
                        LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
                        Jump_To_Reset(true);
                    } else {
                        _blih();
                    }
                    break;
				
                case AX_GET_PARAMETERS:
                    rxbyte[rxbyte_count] = CDC_Device_ReceiveByte(&USB2AX_CDC_Interface);
                    ax_checksum += rxbyte[rxbyte_count] ;
					rxbyte_count++;
                    receive_timer = 0;
                    if(rxbyte_count >= (rxbyte[PACKET_LENGTH] + 4)){ // we have read all the data for the packet
                        if((ax_checksum%256) != 255){  // ignore message if checksum is bad
                            _blih();
                        } else {
						    if (rxbyte[PACKET_INSTRUCTION] == AX_CMD_SYNC_READ){
                                if((rxbyte[6] == 0) || (rxbyte[6] > 6)){  // maximum of 6 bytes to read, else to big for return packet
                                    axStatusPacket(AX_ERROR_RANGE, NULL, 0);
                                } else {
                                    sync_read(&rxbyte[5], rxbyte[PACKET_LENGTH] - 2);
								}									
                            } else { // Read Data
						        local_read(rxbyte[5], rxbyte[6]);
						    }
						    ax_state = AX_SEARCH_FIRST_FF;													
                        }
                    }
                    break;
                        
                default:
                    break;
            }
        }
	    setRX();
	}
	
	// Timeout on state machine while waiting on further USB data
    if(ax_state != AX_SEARCH_FIRST_FF){
        if (receive_timer > RECEIVE_TIMEOUT){
            setTX();
			pass_bytes(rxbyte_count);
            ax_state = AX_SEARCH_FIRST_FF;
			setRX();
		}
    }
	
}

void pass_bytes(uint8_t nb_bytes){    
    for (uint8_t i = 0; i < nb_bytes; i++){
        serial_write(rxbyte[i]);
    }
}


void setRX(void) {
	loop_until_bit_is_set( UCSR1A, TXC1 ); // wait until last byte has been sent (it will set USART Transmit Complete flag)
	
    // enable RX and RX interrupt, disable TX and all TX interrupt
    UCSR1B = ((1 << RXCIE1) | (1 << RXEN1));
}

void setTX(void) {
    // enable TX, disable RX and all RX interrupt
    UCSR1B = (1 << TXEN1);
}


void init_serial(long baud){
    UCSR1B = 0; // disable USART emitter and receiver, as well as all relative interrupts 
		// (Must turn off USART before reconfiguring it, otherwise incorrect operation may occur)
    UCSR1A = 0; // no frequency doubling, disable Multi-processor communication mode
    UCSR1C = (1<<UCSZ11) | (1<<UCSZ10); // Asynchronous USART, no parity, 1 bit stop, 8-bit (aka 8N1)
	
	
    // we want the actual baud rate to be as close as possible to the theoretical baud rate,
    // and at the same time we want to avoid using frequency doubling if possible, since it
    // cuts by half the number of sample the receiver will use, and makes it more vulnerable
    // to baud rate and clock inaccuracy.
    int32_t ubbr = SERIAL_UBBRVAL(baud);
    int32_t br = F_CPU /(16UL*(ubbr+1));

    int32_t ubbr2x = SERIAL_2X_UBBRVAL(baud);
    int32_t br2x = F_CPU /(8UL*(ubbr2x+1));

    if ( max(baud,br) - min(baud,br) <= max(baud,br2x) - min(baud,br2x) ){
        UBRR1 = ubbr; // set UART baud rate
    } else {
        UBRR1 = ubbr2x;
        bitSet(UCSR1A, U2X1);
    }
    // Another way to put it: if br and br2x are equal, choose the one with the highest stability (without U2X),
    // else choose the closest to the mark (and it will necessarily be the one with U2X).

    // enable RX and RX interrupt, disable TX and all TX interrupt
    UCSR1B = ((1 << RXCIE1) | (1 << RXEN1));
}


// Sends data directly out of serial port
void serial_write(uint8_t data){
    loop_until_bit_is_set(UCSR1A, UDRE1); //wait until the TX data registry can accept new data
    
    // Load the next byte from the USART transmit buffer into the USART
    UDR1 = data;            // transmit data
	bitSet(UCSR1A, TXC1);   // clear USART Transmit Complete flag
}


/** ISR to manage the reception of data from the serial port, placing received bytes into a buffer
 *  for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK){
    uint8_t ReceivedByte = UDR1;
	if ( passthrough_mode == AX_PASSTHROUGH ){
		cdc_send_byte(ReceivedByte);
	} else {
		if (local_rx_buffer_count < AX_BUFFER_SIZE){
			local_rx_buffer[local_rx_buffer_count++] = ReceivedByte;
		}
		usart_timer = 0;
	}
}

// global timer
ISR(TIMER0_COMPA_vect, ISR_BLOCK){
	receive_timer++;
    send_timer++;
	usart_timer++;
}


/** Configures the board hardware and chip peripherals. */
void setup_hardware(void){
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    LEDs_Init();
    USB_Init();

    // Start the global timer 
    TCCR0A = (1 << WGM01); // CTC mode
    TCCR0B = 1 << CS01; // clock/8 pre-scaler
    OCR0A = 0x27;  // ticks every 20us
	TIMSK0 |= (1 << OCIE0A);  // enable Timer Compare A interrupt
}


/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void){
    LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void){
    LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void){
    bool ConfigSuccess = true;

    ConfigSuccess &= CDC_Device_ConfigureEndpoints(&USB2AX_CDC_Interface);

    LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void){
    CDC_Device_ProcessControlRequest(&USB2AX_CDC_Interface);
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo){
    // Set the baud rate, ignore the rest of the configuration (parity, char format, number of bits in a byte) since the servos only understand 8N1

    init_serial(CDCInterfaceInfo->State.LineEncoding.BaudRateBPS);
    
    // If the baudrate is at this special (and unlikely) value, it means that we want to trigger the bootloader
    if (CDCInterfaceInfo->State.LineEncoding.BaudRateBPS == 1200){
        needs_bootload = true;
    }
}



// *********************  Soft reset/bootloader functionality *******************************************
// The bootloader can be ran by opening the serial port at 1200bps. Upon closing it, the reset process will be initiated
// and two seconds later, the board will re-enumerate as a DFU bootloader.


// adapted from http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=1008792#1008792
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo){
    static bool PreviousDTRState = false;
    bool        CurrentDTRState  = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR);

    /* Check how the DTR line has been asserted */
    if (PreviousDTRState && !(CurrentDTRState) ){
        // PreviousDTRState == True AND CurrentDTRState == False
        // Host application has Disconnected from the COM port
        
        if (needs_bootload){
            Jump_To_Reset(true);
        }
    }
    PreviousDTRState = CurrentDTRState;
}
// ******************************************************************************************************

