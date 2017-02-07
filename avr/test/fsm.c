/*!
* \file fsm.c
* \brief FSM for incomming data processing
*
*  URL: http://robotika.cz/
*  
*  Revision: 1.0
*  Date: 2006/01/29
*/

#include <string.h>

#include "global.h"
#include "uart.h"

#include "fsm.h"

#define FSM_NO_ALIAS 0x00

// packet start definitions
#define FSM_ALIAS_PACKET 0x01
#define FSM_REGULAR_PACKET 0x02
#define FSM_GROUP_PACKET 0x03

// message processing machine state
 enum FsmStateEnum  {
	FSM_INITIAL_STATE = 1,

	// states being used to assign/release alias
	FSM_WAIT_FOR_SERVICE_ALIAS,
	FSM_WAIT_FOR_ADDR_BYTE_5,
	FSM_WAIT_FOR_ADDR_BYTE_4,
	FSM_WAIT_FOR_ADDR_BYTE_3,
	FSM_WAIT_FOR_ADDR_BYTE_2,
	FSM_WAIT_FOR_ADDR_BYTE_1,
	FSM_WAIT_FOR_ADDR_BYTE_0,
	FSM_WAIT_FOR_SERVICE_CHECKSUM,

	// group stuff
	FSM_WAIT_FOR_GROUP_ALIAS,
	FSM_WAIT_FOR_GROUP_MASK,

	// regular usage
	FSM_WAIT_FOR_ALIAS,
	FSM_WAIT_FOR_LENGTH,
	FSM_WAIT_FOR_DATA,
	FSM_WAIT_FOR_CHECKSUM
};

// packet special character prefix and shift
#define FSM_SPECIAL_PREFIX 0x00
#define FSM_SPECIAL_SHIFT 0x04

// max character being prefixed
#define FSM_SPECIAL_MAX FSM_GROUP_PACKET

// value added to the address while composing the reply	
#define FSM_REPLY_MASK 0x80

static uint8_t handlingSpecial = 0;

static volatile enum FsmStateEnum fsmState;
static uint8_t volatile dataLength;
static uint8_t volatile checkSum;
static uint8_t volatile dataReceived;
static uint8_t volatile deviceAlias;
static uint8_t volatile receivedAlias;
static uint8_t volatile robbusLSB;
static uint8_t volatile groupMessage;

typedef uint8_t (*uint8FuncPtrDataSize)(uint8_t*, uint8_t);
volatile static uint8FuncPtrDataSize commandFunction;

// data buffer
uint8_t dataBuffer[ROBBUS_RX_BUFFER_MAX_SIZE];

// forward declarations
void doCommand(void);
void fsmProcessByte(uint8_t data);

uint8_t emptyFunction(uint8_t *data, uint8_t data_size) { 
	return 0; 
}

#define checkSumInit() checkSum = 0
#define checkSumAdd(data) checkSum += data;

//! initialize FSM
void fsmInit(uint8_t lsb, uint8_t (*cmd_func)(uint8_t*, uint8_t)) {
	uartInit();
	uartSetBaudRate(115200);

	robbusLSB = lsb;
	fsmState = FSM_INITIAL_STATE;
	handlingSpecial = 0;
	deviceAlias = 0;

	// register Rx handler
	uartSetRxHandler(fsmProcessByte);
	fsmSetCommandHandler(cmd_func);
}

//! redirects command processing to a user function
void fsmSetCommandHandler(uint8_t (*cmd_func)(uint8_t*, uint8_t)) {
	// set the receive interrupt to run the supplied user function
	if (cmd_func)
		commandFunction = cmd_func;
	else
		commandFunction = emptyFunction;
}

static void sendWrappedWithCheckSum(uint8_t data) {

	if (data > FSM_SPECIAL_MAX)
		uartAddToTxBuffer(data);
	else {
		uartAddToTxBuffer(FSM_SPECIAL_PREFIX);
		uartAddToTxBuffer(data + FSM_SPECIAL_SHIFT);
	}

	checkSumAdd(data);
}

//! process incoming byte
void fsmProcessByte(uint8_t data) {


	PORTB=PORTB-1;

	// alias assign packet
	if (data == FSM_ALIAS_PACKET) {
		fsmState = FSM_WAIT_FOR_SERVICE_ALIAS;
		checkSumInit(); // initialize checksum counter
		return;
	}

	// alias assign packet
	if (data == FSM_GROUP_PACKET) {
		groupMessage = 1;
		fsmState = FSM_WAIT_FOR_GROUP_ALIAS;
		checkSumInit(); // initialize checksum counter
		return;
	}

	// checking start byte is outside the fsm (because of synchronizing)
	if (data == FSM_REGULAR_PACKET) {
		groupMessage = 0;
		fsmState = FSM_WAIT_FOR_ALIAS;
		checkSumInit(); // initialize checksum counter
		return;
	}

	// handle special character
	if (data == FSM_SPECIAL_PREFIX) {	// special character received, set flag, do not change state
		handlingSpecial = 1;
		return;
	}

	if (handlingSpecial) {			// previous was special, so shift accordingly
		handlingSpecial = 0;
		data -= FSM_SPECIAL_SHIFT;
	}

	switch (fsmState) {

		// assign/release sequence	
		case FSM_WAIT_FOR_SERVICE_ALIAS:
			if (data & FSM_REPLY_MASK) {
				fsmState = FSM_INITIAL_STATE; // reply from someone, ignore rest of packet
			} else {
				receivedAlias = data;
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_ADDR_BYTE_5;
			}
			break;
		case FSM_WAIT_FOR_ADDR_BYTE_5:
			if (data == ROBBUS_ADDR_BYTE_5) {
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_ADDR_BYTE_4; // for me
			} else {
				fsmState = FSM_INITIAL_STATE; // for someone else
			}
			break;
		case FSM_WAIT_FOR_ADDR_BYTE_4:
			if (data == ROBBUS_ADDR_BYTE_4) {
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_ADDR_BYTE_3; // for me
			} else {
				fsmState = FSM_INITIAL_STATE; // for someone else
			}
			break;
		case FSM_WAIT_FOR_ADDR_BYTE_3:
			if (data == ROBBUS_ADDR_BYTE_3) {
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_ADDR_BYTE_2; // for me
			} else {
				fsmState = FSM_INITIAL_STATE; // for someone else
			}
			break;
		case FSM_WAIT_FOR_ADDR_BYTE_2:
			if (data == ROBBUS_ADDR_BYTE_2) {
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_ADDR_BYTE_1; // for me
			} else {
				fsmState = FSM_INITIAL_STATE; // for someone else
			}
			break;
		case FSM_WAIT_FOR_ADDR_BYTE_1:
			if (data == ROBBUS_ADDR_BYTE_1) {
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_ADDR_BYTE_0; // for me
			} else {
				fsmState = FSM_INITIAL_STATE; // for someone else
			}
			break;		
		case FSM_WAIT_FOR_ADDR_BYTE_0:
			if (data == robbusLSB) {
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_SERVICE_CHECKSUM; // for me
			} else {
				fsmState = FSM_INITIAL_STATE; // for someone else
			}
			break;
		case FSM_WAIT_FOR_SERVICE_CHECKSUM:
			
			if (((uint8_t)(data + checkSum)) == 0) {
				deviceAlias = receivedAlias;
				uartAddToTxBuffer(FSM_ALIAS_PACKET);
				checkSumInit();
				uartAddToTxBuffer(receivedAlias | FSM_REPLY_MASK);
				sendWrappedWithCheckSum(ROBBUS_ADDR_BYTE_5);
				sendWrappedWithCheckSum(ROBBUS_ADDR_BYTE_4);
				sendWrappedWithCheckSum(ROBBUS_ADDR_BYTE_3);
				sendWrappedWithCheckSum(ROBBUS_ADDR_BYTE_2);
				sendWrappedWithCheckSum(ROBBUS_ADDR_BYTE_1);
				sendWrappedWithCheckSum(robbusLSB);
				uartAddToTxBuffer(-checkSum);
				uartSendTxBuffer();
			}
			fsmState = FSM_INITIAL_STATE;
			break;

		// group sequence	
		case FSM_WAIT_FOR_GROUP_ALIAS:
			if (data & FSM_REPLY_MASK) {
				fsmState = FSM_INITIAL_STATE; // reply from someone, ignore rest of packet
			} else {
				receivedAlias = data;
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_GROUP_MASK;
			}
			break;

		case FSM_WAIT_FOR_GROUP_MASK:
			if ((data & receivedAlias) != (data & deviceAlias)) {
				fsmState = FSM_INITIAL_STATE; // reply from someone, ignore rest of packet
			} else {
				receivedAlias = data;
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_LENGTH;
			}
			break;
		
		// regulsr sequence	
		case FSM_WAIT_FOR_ALIAS:
			if (data & FSM_REPLY_MASK || data != deviceAlias) {
				fsmState = FSM_INITIAL_STATE; // reply from someone, or for another one ignore rest of packet
			} else {
				checkSumAdd(data);
				fsmState = FSM_WAIT_FOR_LENGTH;
			}
			break;
		case FSM_WAIT_FOR_LENGTH:
			checkSumAdd(data);
			dataLength = data; // ommit the opcode
			dataReceived = 0;
			fsmState = FSM_WAIT_FOR_DATA;
			break;

		case FSM_WAIT_FOR_DATA:
			if (dataReceived < ROBBUS_RX_BUFFER_MAX_SIZE) {
				dataBuffer[dataReceived] = data;
				checkSum += data;
				dataReceived++;
			}
			
			if (dataReceived == dataLength) {
				fsmState = FSM_WAIT_FOR_CHECKSUM;
			}	
			break;

		case FSM_WAIT_FOR_CHECKSUM:
			
			if (((uint8_t)(data + checkSum)) == 0) {
				doCommand();
			}
			fsmState = FSM_INITIAL_STATE;
			break;

		default:
			// should never happen ;-)
			fsmState = FSM_INITIAL_STATE;
		break;
	}
}
/*
static void sendWrappedWithCheckSum(uint8_t data) {

	if (data > FSM_SPECIAL_MAX)
		uartAddToTxBuffer(data);
	else {
		uartAddToTxBuffer(FSM_SPECIAL_PREFIX);
		uartAddToTxBuffer(data + FSM_SPECIAL_SHIFT);
	}

	checkSumAdd(data);
}
*/
void doCommand(void) {
	// do action here
	uint8_t replyDataSize = commandFunction(dataBuffer, dataReceived);
	
	// send reply if not group message
	if (!groupMessage) {
		checkSumInit();
		uartAddToTxBuffer(FSM_REGULAR_PACKET);
		sendWrappedWithCheckSum(FSM_REPLY_MASK | deviceAlias);
		sendWrappedWithCheckSum( replyDataSize );

		uint8_t index = 0; 

		while (index < replyDataSize) {
			sendWrappedWithCheckSum( dataBuffer[index++] );
		}

		uartAddToTxBuffer(-checkSum);

		uartSendTxBuffer();
	}
}


