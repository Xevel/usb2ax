/*
 * eeprom.c
 *
 * Created: 19/01/2015 18:05:59
 *  Author: Xevel
 */ 
#include "eeprom.h"
#include "AX.h"
#include <avr/eeprom.h>

 
#define EE_ADDR_MAGIC_KEY           0x0
#define EE_MAGIC_KEY                0x101BEEF
#define EE_ADDR(x)                  ((void*)( EE_ADDR_MAGIC_KEY + sizeof(EE_MAGIC_KEY) + x ))
#define ADDR_START_EE_SAVE          START_RW_ADDR
#define NB_EE_SAVED_BYTES           (REG_TABLE_SIZE - ADDR_START_EE_SAVE)

uint8_t eeprom_init(){
    eeprom_busy_wait();
    return eeprom_load();
}

uint8_t eeprom_load(){
    if (eeprom_read_dword(EE_ADDR_MAGIC_KEY) == EE_MAGIC_KEY){
        eeprom_read_block( regs + START_RW_ADDR, EE_ADDR(0), NB_EE_SAVED_BYTES );
        return true;
    }
    return false;
}

void eeprom_save(){
    eeprom_update_block( regs + START_RW_ADDR, EE_ADDR(0), NB_EE_SAVED_BYTES );
    eeprom_update_dword( EE_ADDR_MAGIC_KEY, EE_MAGIC_KEY );
}

void eeprom_clear(){
    eeprom_update_dword(EE_ADDR_MAGIC_KEY, 0x0000000);
}
