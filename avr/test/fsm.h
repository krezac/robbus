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

#ifdef __cplusplus
extern "C" {
#endif

//! initialize FSM
void fsmInit(uint8_t lsb, uint8_t (*cmd_func)(uint8_t*, uint8_t));

//! redirects command processing to a user function
void fsmSetCommandHandler(uint8_t (*cmd_func)(uint8_t*, uint8_t));

#ifdef __cplusplus
}
#endif

#endif

