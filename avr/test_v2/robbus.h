/*!
* \file fsm.h
* \brief FSM for incomming data processing
*
*  URL: http://robotika.cz/
*  
*  Revision: 1.0
*  Date: 2006/01/29
*/

#ifndef FSM_H
#define FSM_H

#include <avr/io.h>

#include "robbus_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t* (*PtrFuncPtr_t)(uint8_t*);

//! initialize FSM
void Robbus_Init(PtrFuncPtr_t cmdHandler);

#ifdef __cplusplus
}
#endif

#endif

