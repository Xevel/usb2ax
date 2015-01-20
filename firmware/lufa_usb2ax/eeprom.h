/*
 * eeprom.h
 *
 * Created: 18/01/2015 13:34:41
 *  Author: Xevel
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_
#include <stdint.h>

uint8_t eeprom_init();
uint8_t eeprom_load();
void eeprom_save();
void eeprom_clear();

#endif /* EEPROM_H_ */