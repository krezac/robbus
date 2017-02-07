//----- Include Files ---------------------------------------------------------
#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)

#include "global.h"		// include our global settings
#include "timer.h"		// include timer function library (timing, PWM, etc)
#include "uart.h"
#include "robbus.h"

static uint8_t outData[ROBBUS_OUTGOING_SIZE];

static uint8_t* messageHandler(uint8_t *inData) {
	uint8_t i;
	PORTB = ~inData[0];
	outData[0] = PINC;
	return outData;
}

static void init(void)
{
	// initialize library units
	Robbus_Init(messageHandler);

	DDRB = 0xff;
	PORTB = 0xfe;
	PORTC = 0xff;

	sei();
}

//----- Begin Code ------------------------------------------------------------


int main(void)
{
	init();

	while(1) 
	{
		// nothing to do in main loop
		asm volatile("wdr");
	}

	return 0;
}
