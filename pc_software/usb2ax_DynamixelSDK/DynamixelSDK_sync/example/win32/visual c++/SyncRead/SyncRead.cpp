// Windows version
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <conio.h>
#include <dynamixel.h>

#pragma comment(lib, "dynamixel.lib")


// Control table address
#define P_GOAL_POSITION_L	30
#define P_GOAL_POSITION_H	31
#define P_GOAL_SPEED_L		32
#define P_GOAL_SPEED_H		33
#define P_PRESENT_POSITION_L	36

// Defulat setting
#define DEFAULT_PORTNUM		120 // COM3
#define DEFAULT_BAUDNUM		1 // 1Mbps


#define NB_ACTUATOR		8 // Number of actuator
//int ids[NB_ACTUATOR] = {  1, 2, 3, 4, 5, 6, 7, 8 };
int ids[NB_ACTUATOR] = {  50, 51, 52, 53, 60, 61, 62, 63 };
int nb_to_move = NB_ACTUATOR;

void PrintCommStatus(int CommStatus);
void PrintErrorCode();

	int CommStatus;

void sync_write_all(int pos){
	dxl_sync_write_start( P_GOAL_POSITION_L, 2 );
	for (int i = 0; i < nb_to_move; i++ ){
		dxl_sync_write_push_id( ids[i] );
		dxl_sync_write_push_word( pos );
	}
	dxl_sync_write_send();
}

void sync_read (){
	dxl_sync_read_start( P_PRESENT_POSITION_L, 2 );
	for (int i = 0; i < nb_to_move; i++ ){
		dxl_sync_read_push_id( ids[i] );
	}
	dxl_sync_read_send();			
	CommStatus = dxl_get_result();
	if( CommStatus == COMM_RXSUCCESS )
	{
		for (int i = 0; i < nb_to_move; i++ ){
			printf( "%i=%i, ", ids[i] , dxl_sync_read_pop_word());
		}
	}
	else
	{
		PrintCommStatus(CommStatus);
	}
	
	printf("\n");

}

void move_all_to(int pos){
	printf( "Set Goal_position = %i\n", pos );
	sync_write_all(pos);
	for (int m = 0; m<100; m++ ){
		sync_read();
	}
	Sleep( 200 );
}




int main()
{
	// Open device
	if( dxl_initialize(DEFAULT_PORTNUM, DEFAULT_BAUDNUM) == 0 )
	{
		printf( "Failed to open USB2Dynamixel!\n" );
		printf( "Press any key to terminate...\n" );
		_getch();
		return 0;
	}
	else
		printf( "Succeed to open USB2Dynamixel!\n" );
	
	// Set goal speed	
	dxl_write_word( BROADCAST_ID, P_GOAL_SPEED_L, 0 );
	// Set goal position
	dxl_write_word( BROADCAST_ID, P_GOAL_POSITION_L, 512 );

	while(1)
	{
		printf( "Press any key to continue!(press ESC to quit)\n" );
		if(_getch() == 0x1b)
			break;

		move_all_to(470);
		move_all_to(560);
		printf( "\n" );
	}

	dxl_terminate();
	printf( "Press any key to terminate...\n" );
	_getch();

	return 0;
}


// Print communication result
void PrintCommStatus(int CommStatus)
{
	switch(CommStatus)
	{
	case COMM_TXFAIL:
		printf("COMM_TXFAIL: Failed transmit instruction packet!\n");
		break;

	case COMM_TXERROR:
		printf("COMM_TXERROR: Incorrect instruction packet!\n");
		break;

	case COMM_RXFAIL:
		printf("COMM_RXFAIL: Failed get status packet from device!\n");
		break;

	case COMM_RXWAITING:
		printf("COMM_RXWAITING: Now recieving status packet!\n");
		break;

	case COMM_RXTIMEOUT:
		printf("COMM_RXTIMEOUT: There is no status packet!\n");
		break;

	case COMM_RXCORRUPT:
		printf("COMM_RXCORRUPT: Incorrect status packet!\n");
		break;

	default:
		printf("This is unknown error code!\n");
		break;
	}
}

// Print error bit of status packet
void PrintErrorCode()
{
	if(dxl_get_rxpacket_error(ERRBIT_VOLTAGE) == 1)
		printf("Input voltage error!\n");

	if(dxl_get_rxpacket_error(ERRBIT_ANGLE) == 1)
		printf("Angle limit error!\n");

	if(dxl_get_rxpacket_error(ERRBIT_OVERHEAT) == 1)
		printf("Overheat error!\n");

	if(dxl_get_rxpacket_error(ERRBIT_RANGE) == 1)
		printf("Out of range error!\n");

	if(dxl_get_rxpacket_error(ERRBIT_CHECKSUM) == 1)
		printf("Checksum error!\n");

	if(dxl_get_rxpacket_error(ERRBIT_OVERLOAD) == 1)
		printf("Overload error!\n");

	if(dxl_get_rxpacket_error(ERRBIT_INSTRUCTION) == 1)
		printf("Instruction code error!\n");	
}