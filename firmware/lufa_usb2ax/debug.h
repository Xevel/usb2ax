/*
 * debug.h
 *
 * Created: 17/01/2015 03:56:41
 *  Author: Xevel
 */ 


#ifndef DEBUG_H_
#define DEBUG_H_

#define USE_DEBUG_PINS (1)

#if(USE_DEBUG_PINS)

    void init_debug(){
        // put all the debug pins in output mode
        bitSet(DDRB,0);
        bitSet(DDRB,1);
        bitSet(DDRB,2);
        bitSet(DDRB,3);
    }

    #define up0 bitSet(PORTB, 0)
    #define up1 bitSet(PORTB, 1)
    #define up2 bitSet(PORTB, 2)
    #define up3 bitSet(PORTB, 3)

    #define dw0 bitClear(PORTB, 0)
    #define dw1 bitClear(PORTB, 1)
    #define dw2 bitClear(PORTB, 2)
    #define dw3 bitClear(PORTB, 3)

#else

    #define init_debug()
    #define up0
    #define up1
    #define up2
    #define up3
    #define dw0
    #define dw1
    #define dw2
    #define dw3

#endif



#endif /* DEBUG_H_ */