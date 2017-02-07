/*!
* \file sermon.c
* \brief Serial line profiler
*
* \author Kamil Rezac
*  URL: http://robotika.cz/
*  
*  Revision: 1.0
*  Date: 2009/11/07
*/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "global.h"

#define USART_BUFFER_SIZE 254 
static uint8_t usartBuffer[USART_BUFFER_SIZE];

// working positions in the buffers
static uint8_t volatile usartBufferIndex;
static uint8_t volatile timeStamp;
static uint8_t volatile complete;

//! initialize FSM
void init(void) {
	// Initialize UART:
	// enable USART module and USART interrupts 
	UCSRB = BV(RXCIE) | BV(UDRIE) | BV(RXEN) | BV(TXEN);

	// set baudrate
	uint16_t baudrate = ((16000000L+(115200L*8L))/(115200L*16L)-1);
	UBRRL = baudrate;
	#ifdef UBRRH
	UBRRH = baudrate >> 8;
	#endif

	// initialize buffer indices
	usartBufferIndex = 0;

	// initialize timestamping variable
	timeStamp = 0;
	complete = 0;

	// initialize timer
	TCNT0 = 0;
	TCCR0 = 4; // CLK/256
	//sbi(TIMSK, TOIE0);	

	// enable interrupts
	sei();
}

//ISR(INT0_vect) {
//	timeStamp++;
//}

/// USART receive interrupt routine
ISR(USART_RXC_vect) {
	// read byte from USART register
	uint8_t data = UDR;

	if (complete)
		return;

	if (usartBufferIndex < USART_BUFFER_SIZE) {
		usartBuffer[usartBufferIndex++] = data;
		usartBuffer[usartBufferIndex++] = TCNT0;
		TCNT0 = 0;
	} else {
		// buffer full, disable receiving, start transmit
		UCSRB = BV(UDRIE) | BV(TXEN); // disable RX TODO: clear RX interrupt flag
		cbi(UCSRA, RXC);
		complete = 1;
		usartBufferIndex = 0;
		UDR = usartBuffer[usartBufferIndex++];
	}
}

/// USART transmit data register empty interrupt routine
ISR(USART_UDRE_vect) {
	if (!complete)
		return;
	if (usartBufferIndex < USART_BUFFER_SIZE)
		UDR = usartBuffer[usartBufferIndex++];
}

int main(void) {
	init();
	while(1);
}
