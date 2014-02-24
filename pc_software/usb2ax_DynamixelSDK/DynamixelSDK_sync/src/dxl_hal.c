// windows version
// by windows serial programming
#include <windows.h>
#include <stdio.h>
#include "dxl_hal.h"

#define LATENCY_TIME		(16) //ms	(USB2Serial Latency timer)
#define IN_TRASFER_SIZE		(512) //unsigned char

HANDLE ghSerial_Handle = INVALID_HANDLE_VALUE; // Serial port handle
float gfByteTransTime = 0.0f;
float gfRcvWaitTime = 0.0f;
LARGE_INTEGER gStartTime;


int dxl_hal_open( int devIndex, float baudrate )
{
	// Opening device
	// devIndex: Device index
	// baudrate: Real baudrate (ex> 115200, 57600, 38400...)
	// Return: 0(Failed), 1(Succeed)

	DCB Dcb;
	COMMTIMEOUTS Timeouts;
	DWORD dwError;
	char PortName[15];

	dxl_hal_close();

	// Make real port name
	sprintf_s(PortName, 15, "\\\\.\\COM%d", devIndex);
	// Open serial device
	ghSerial_Handle = CreateFile( PortName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( ghSerial_Handle == INVALID_HANDLE_VALUE )
		return 0;

	// Setting communication property
	Dcb.DCBlength = sizeof(DCB);
	if( GetCommState( ghSerial_Handle, &Dcb ) == FALSE )
		goto DXL_HAL_OPEN_ERROR;
	
	// Set baudrate
	gfByteTransTime = 1000.0f / baudrate * 10.0f; // 1000/baudrate(bit per msec) * 10(start bit + data bit + stop bit)
	Dcb.BaudRate			= (DWORD)baudrate;	
	Dcb.ByteSize			= 8;					// Data bit = 8bit
	Dcb.Parity				= NOPARITY;				// No parity
	Dcb.StopBits			= ONESTOPBIT;			// Stop bit = 1
	Dcb.fParity				= NOPARITY;				// No Parity check
	Dcb.fBinary				= 1;					// Binary mode
	Dcb.fNull				= 0;					// Get Null byte
	Dcb.fAbortOnError		= 1;
	Dcb.fErrorChar			= 0;
	// Not using XOn/XOff
	Dcb.fOutX				= 0;
	Dcb.fInX				= 0;
	// Not using H/W flow control
	Dcb.fDtrControl			= DTR_CONTROL_DISABLE;
	Dcb.fRtsControl			= RTS_CONTROL_DISABLE;
	Dcb.fDsrSensitivity		= 0;
	Dcb.fOutxDsrFlow		= 0;
	Dcb.fOutxCtsFlow		= 0;
	if( SetCommState( ghSerial_Handle, &Dcb ) == FALSE )
		goto DXL_HAL_OPEN_ERROR;

	if( SetCommMask( ghSerial_Handle, 0 ) == FALSE ) // Not using Comm event
		goto DXL_HAL_OPEN_ERROR;
	if( SetupComm( ghSerial_Handle, 4096, 4096 ) == FALSE ) // Buffer size (Rx,Tx)
		goto DXL_HAL_OPEN_ERROR;
	if( PurgeComm( ghSerial_Handle, PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR ) == FALSE ) // Clear buffer
		goto DXL_HAL_OPEN_ERROR;
	if( ClearCommError( ghSerial_Handle, &dwError, NULL ) == FALSE )
		goto DXL_HAL_OPEN_ERROR;
	
	if( GetCommTimeouts( ghSerial_Handle, &Timeouts ) == FALSE )
		goto DXL_HAL_OPEN_ERROR;
	// Timeout (Not using timeout)
	// Immediatly return
	Timeouts.ReadIntervalTimeout = 0;
	Timeouts.ReadTotalTimeoutMultiplier = 0;
	Timeouts.ReadTotalTimeoutConstant = 1; // must not be zero.
	Timeouts.WriteTotalTimeoutMultiplier = 0;
	Timeouts.WriteTotalTimeoutConstant = 0;
	if( SetCommTimeouts( ghSerial_Handle, &Timeouts ) == FALSE )
		goto DXL_HAL_OPEN_ERROR;
	
	return 1;

DXL_HAL_OPEN_ERROR:
	dxl_hal_close();
	return 0;
}

void dxl_hal_close()
{
	// Closing device
	if(ghSerial_Handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle( ghSerial_Handle );
		ghSerial_Handle = INVALID_HANDLE_VALUE;
	}
}

void dxl_hal_clear(void)
{
	// Clear communication buffer
	PurgeComm( ghSerial_Handle, PURGE_RXABORT|PURGE_RXCLEAR );
}

int dxl_hal_tx( unsigned char *pPacket, int numPacket )
{
	// Transmiting date
	// *pPacket: data array pointer
	// numPacket: number of data array
	// Return: number of data transmitted. -1 is error.
	DWORD dwToWrite, dwWritten;

	dwToWrite = (DWORD)numPacket;
	dwWritten = 0;

	if( WriteFile( ghSerial_Handle, pPacket, dwToWrite, &dwWritten, NULL ) == FALSE )
		return -1;
	
	return (int)dwWritten;
}

int dxl_hal_rx( unsigned char *pPacket, int numPacket )
{
	// Recieving date
	// *pPacket: data array pointer
	// numPacket: number of data array
	// Return: number of data recieved. -1 is error.
	DWORD dwToRead, dwRead;

	dwToRead = (DWORD)numPacket;
	dwRead = 0;

	if( ReadFile( ghSerial_Handle, pPacket, dwToRead, &dwRead, NULL ) == FALSE )
		return -1;

	return (int)dwRead;
}

void dxl_hal_set_timeout( int NumRcvByte )
{
	// Start stop watch
	// NumRcvByte: number of recieving data(to calculate maximum waiting time)
	QueryPerformanceCounter( &gStartTime );
	gfRcvWaitTime = (float)(gfByteTransTime*(float)NumRcvByte + 2*LATENCY_TIME + 2.0f);
}

int dxl_hal_timeout(void)
{
	// Check timeout
	// Return: 0 is false, 1 is true(timeout occurred)
	LARGE_INTEGER end, freq;
	double time;

	QueryPerformanceCounter( &end );
	QueryPerformanceFrequency( &freq );

	time = (double)(end.QuadPart - gStartTime.QuadPart) / (double)freq.QuadPart;
	time *= 1000.0;

	if( time > gfRcvWaitTime )
		return 1;
	else if( time < 0 )
		QueryPerformanceCounter( &gStartTime );

	return 0;
}