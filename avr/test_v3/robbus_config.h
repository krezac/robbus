/*!
 * * \file robbus_config.h
 * * \brief Robbus configuration file
 * *
 * * \author Kamil Rezac
 * *  URL: http://robotika.cz/
 * *  
 * *  Revision: 1.0
 * *  Date: 2009/10/30
 * */

#ifndef ROBBUS_CONFIG_H
#define ROBBUS_CONFIG_H

/// clock speed
#define ROBBUS_CPU_FREQ 18432000L

/// bus baudrate
#define ROBBUS_BAUDRATE 115200L

/// initial robbus address of the device (if not able to read from the EEPROM)
#define ROBBUS_INITIAL_ADDRESS 'r'

/// address in EEPROM where device address is stored
#define ROBBUS_EEPROM_DATA_ADDRESS 0x04

/// input buffer size. Change to match the incomming payload size
#define ROBBUS_INCOMMING_SIZE 1

/// output buffer size. Change to match the outgoing payload size
#define ROBBUS_OUTGOING_SIZE 1

#endif
