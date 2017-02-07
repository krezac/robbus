//----- Include Files ---------------------------------------------------------
#include <avr/io.h>		// include I/O definitions (port names, pin names, etc)

#include "global.h"		// include our global settings
#include "timer.h"		// include timer function library (timing, PWM, etc)
#include "uart.h"
#include "fsm.h"

static uint8_t messageHandler(uint8_t *data, uint8_t dataSize) {
	if (dataSize > 0) {
		PORTB = ~data[0];
		
		data[0] = PINC;
		return 1;
	}
	return 0;
}

static void init(void)
{
	// initialize library units
	uartInit();
	uartSetBaudRate(115200);

	fsmInit(ROBBUS_ADDR_BYTE_0, messageHandler);

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
