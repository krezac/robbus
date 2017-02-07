/*
	Robbus.h - Library for Robbus communication protocol.
	Created by Kamil Rezac, October 18, 2011.
	Released into the public domain.
*/
#include <stdlib.h>

#include "WProgram.h"
#include "Robbus.h"

#define ROBBUS_EEPROM_DATA_ADDRESS 4
#define ROBBUS_MIN_BUFFER_SIZE 4

#define SERVICE_PACKET_HEAD 0x01
#define REGULAR_PACKET_HEAD 0x02
#define GROUP_PACKET_HEAD 0x03

#define SUBPACKET_ECHO 'e'
#define SUBPACKET_DESCRIPTION 'd'
#define SUBPACKET_CHANGE_ADDRESS 'a'

// shadowing the memory space
#define receivedAddress usartBufferIndex

// message processing machine state
// RX states - bits 2:0
enum RxStateEnum  {
	RX_STATE_READY = 0x00,
	
	// group stuff
	RX_STATE_WAIT_FOR_GROUP_ADDRESS = 0x01,
	RX_STATE_WAIT_FOR_GROUP_MASK = 0x02,
	
	// regular usage
	RX_STATE_WAIT_FOR_ADDRESS = 0x03,
	RX_STATE_WAIT_FOR_LENGTH = 0x04,
	RX_STATE_WAIT_FOR_DATA = 0x05,
	RX_STATE_WAIT_FOR_CHECKSUM = 0x06
};

// TX states - bits 4:3
enum TxStateEnum  {
	TX_STATE_READY = 0x00,
	TX_STATE_SEND_ADDRESS = 0x08,
	TX_STATE_SEND_LENGTH = 0x10,
	TX_STATE_SEND_DATA = 0x18
};

// flags used during processing
// flags - bits 7:5
#define RX_FLAG_SPECIAL_CHAR            0x20
#define RX_FLAG_SERVICE_PACKET          0x40
#define RX_FLAG_GROUP_PACKET            0x80

#define RX_STATE_MASK 0x07
#define TX_STATE_MASK 0x18

#define changeRxState(newState) robbusState=(robbusState&~RX_STATE_MASK)|(newState)
#define changeTxState(newState) robbusState=(robbusState&~TX_STATE_MASK)|(newState)

#define getRxState() (robbusState&RX_STATE_MASK)
#define getTxState() (robbusState&TX_STATE_MASK)

#define getFlag(flag)   (robbusState&(flag))
#define setFlag(flag)   (robbusState|=(flag))
#define clearFlag(flag) (robbusState&=~(flag))

// packet special character prefix and shift
#define SPECIAL_CHAR_PREFIX 0x00
#define SPECIAL_CHAR_SHIFT 0x04

// max character being prefixed
#define SPECIAL_CHAR_MAX GROUP_PACKET_HEAD

// value added to the address while composing the reply 
#define ADDRESS_REPLY_MASK 0x80

#define checkSumInit() checkSum = 0
#define checkSumAdd(data) checkSum += data;


// Constructor /////////////////////////////////////////////////////////////////
RobbusLib::RobbusLib()
{
}

// Public functions ////////////////////////////////////////////////////////////
void RobbusLib::begin(RobbusCommWrapper* commWrapperPtr, byte address, byte inDataSize, byte outDataSize, byte* (*handler)(byte*))
{		  
	commWrapper = commWrapperPtr;
	deviceAddress = address;
	commandHandler = handler;
	incomingDataSize = inDataSize;
	outgoingDataSize = outDataSize;	

	// TODO this is nasty but I haven't found a better way
	inDataSize = inDataSize > ROBBUS_MIN_BUFFER_SIZE ? inDataSize : ROBBUS_MIN_BUFFER_SIZE;	
	usartBufferSize = inDataSize > outDataSize ? inDataSize:outDataSize; 
	usartBuffer = (byte*) malloc(usartBufferSize * sizeof(byte)); 

	commWrapper->begin();

	// initialize state machine
	robbusState = 0;

	// initialize buffer indices
	usartBufferIndex = 0;
}

void RobbusLib::process()
{
	if (!commWrapper->available()) {
		// nothing to receive...
		// if we have something to send, do it now

		switch(getTxState()) {
			case TX_STATE_READY:    // nothing to send, return
				return; 
			case TX_STATE_SEND_ADDRESS:
				sendWrapped(deviceAddress | ADDRESS_REPLY_MASK); // no need to check the special characters
				changeTxState(TX_STATE_SEND_LENGTH);
				break;
			case TX_STATE_SEND_LENGTH:
				if (sendWrapped(payloadLength))
				{
				usartBufferIndex = 0;
				changeTxState(TX_STATE_SEND_DATA);
				}
				break;
			case TX_STATE_SEND_DATA:
				if (usartBufferIndex < payloadLength) {
					if (sendWrapped(usartBuffer[usartBufferIndex]))
						usartBufferIndex++;
				} else { // checksum
					if (sendWrapped(-checkSum))
						changeTxState(TX_STATE_READY);
				}
				break;
			default:
				changeTxState(TX_STATE_READY);
		}	
		return;	
	}
	

	// some bytes in receive buffer, so process one
	byte data = commWrapper->read();

	// special characters handling
	if (data == SERVICE_PACKET_HEAD) {                      // service packet
		setFlag(RX_FLAG_SERVICE_PACKET);                // set flag
		clearFlag(RX_FLAG_GROUP_PACKET); // clear flags
		changeRxState(RX_STATE_WAIT_FOR_ADDRESS);       // and process as regular
		return;                                         // and leave processing
	} else if (data == GROUP_PACKET_HEAD) {                 // group packet (will contain mask byte)
		setFlag(RX_FLAG_GROUP_PACKET);                  // set flag
		clearFlag(RX_FLAG_SERVICE_PACKET); // clear flags
		changeRxState(RX_STATE_WAIT_FOR_GROUP_ADDRESS); // and wait for composite address (address-mask)
		return;                                         // and leave processing
	} else if (data == REGULAR_PACKET_HEAD) {               // regular packet
		clearFlag(RX_FLAG_SERVICE_PACKET|RX_FLAG_GROUP_PACKET); // clear flags
		changeRxState(RX_STATE_WAIT_FOR_ADDRESS);       // and wait for the address
		return;                                         // and leave processing
	} else if (data == SPECIAL_CHAR_PREFIX) {               // special character received, set flag, do not change state
		setFlag(RX_FLAG_SPECIAL_CHAR);                  // only set flag
		return;                                         // and leave processing
	}
	
	if (getFlag(RX_FLAG_SPECIAL_CHAR)) {                    // previous was special character
		clearFlag(RX_FLAG_SPECIAL_CHAR);                // clear the flag
		data -= SPECIAL_CHAR_SHIFT;                     // and correct data byte value
	}
	
	switch (getRxState()) {
		// group sequence       
		case RX_STATE_WAIT_FOR_GROUP_ADDRESS:
			if (data & ADDRESS_REPLY_MASK) {
				changeRxState(RX_STATE_READY); // reply from someone (even me ;), ignore rest of packet
			} else {
				receivedAddress = data;
				checkSumInit();
				checkSumAdd(data);
				changeRxState(RX_STATE_WAIT_FOR_GROUP_MASK);
			}
			break;
	
		case RX_STATE_WAIT_FOR_GROUP_MASK:
			if ((data & receivedAddress) != (data & deviceAddress)) {
				changeRxState(RX_STATE_READY); // reply from someone, ignore rest of packet
			} else {
				receivedAddress = data;
				checkSumAdd(data);
				changeRxState(RX_STATE_WAIT_FOR_LENGTH);
			}
			break;

		// regulsr sequence     
		case RX_STATE_WAIT_FOR_ADDRESS:
			if (data & ADDRESS_REPLY_MASK || data != deviceAddress) {
				changeRxState(RX_STATE_READY); // reply from someone, or for another one ignore rest of packet
			} else {
				checkSumInit();
				checkSumAdd(data);
				changeRxState(RX_STATE_WAIT_FOR_LENGTH);
			}
			break;
		case RX_STATE_WAIT_FOR_LENGTH:
			checkSumAdd(data);
			payloadLength = data; // ommit the opcode
			usartBufferIndex = 0;
			changeRxState(RX_STATE_WAIT_FOR_DATA);
			break;
		
		case RX_STATE_WAIT_FOR_DATA:
			if (usartBufferIndex < usartBufferSize) {
				usartBuffer[usartBufferIndex] = data;
				checkSum += data;
				usartBufferIndex++;
			}
			
			if (usartBufferIndex == payloadLength) {
				changeRxState(RX_STATE_WAIT_FOR_CHECKSUM);
			}       
			break;
		
		case RX_STATE_WAIT_FOR_CHECKSUM:
		        
			if (((byte)(data + checkSum)) == 0) {

				// checksum ok, do action
				if (getFlag(RX_FLAG_SERVICE_PACKET)) {
					// process service packet
					if(!doServiceCommand()) {
						changeRxState(RX_STATE_READY);
						return;
					}
				} else {
					byte i;
					// process regular packet
					byte* replyData = commandHandler(usartBuffer);
					
					// copy user data to uart buffer
					for (i = 0; i < outgoingDataSize; i++)
						usartBuffer[i] = replyData[i];
					
					payloadLength = outgoingDataSize;
				}
				
				// if not group packet, send reply
				if (!(getFlag(RX_FLAG_GROUP_PACKET)))
				{
					// initialize checksum
					checkSumInit();
					
					// set tx machine state
					changeTxState(TX_STATE_SEND_ADDRESS);
					clearFlag(RX_FLAG_SPECIAL_CHAR);
					
					// and push first byte into usart register
					commWrapper->write(getFlag((RX_FLAG_SERVICE_PACKET) ? SERVICE_PACKET_HEAD : REGULAR_PACKET_HEAD));
				}
			}
			changeRxState(RX_STATE_READY);
			break;
	
		default:
			// should never happen ;-)
			changeRxState(RX_STATE_READY);
			break;
	}
}

// Private functions ///////////////////////////////////////////////////////////
byte RobbusLib::doServiceCommand(void) {
	byte newAddress;
	switch (usartBuffer[0])
	{
		case SUBPACKET_DESCRIPTION:
			usartBuffer[0] = incomingDataSize;
			usartBuffer[1] = outgoingDataSize;
			payloadLength = 2;
			return 1;
		case SUBPACKET_ECHO:
			return 1;
		case SUBPACKET_CHANGE_ADDRESS:
			newAddress = usartBuffer[1];
			if (usartBuffer[2] != deviceAddress || usartBuffer[3] != (deviceAddress ^ newAddress))
				return 0;
			// TODO FIXME, EEPROM not found -- EEPROM.write(ROBBUS_EEPROM_DATA_ADDRESS+0, 'R'); 
			// TODO FIXME, EEPROM not found -- EEPROM.write(ROBBUS_EEPROM_DATA_ADDRESS+1, newAddress); 
			payloadLength = 2;
			return 1;
		default:
			return 0;
	}
}

byte RobbusLib::sendWrapped(byte c)
{
	if (c > SPECIAL_CHAR_MAX) {
		commWrapper->write(c);
	} else {
		if (getFlag(RX_FLAG_SPECIAL_CHAR)) {
			commWrapper->write((byte)(c + SPECIAL_CHAR_SHIFT));
			clearFlag(RX_FLAG_SPECIAL_CHAR);
		} else {
			commWrapper->write((byte)SPECIAL_CHAR_PREFIX);
			setFlag(RX_FLAG_SPECIAL_CHAR);
			return 0;
		}
	}
	checkSumAdd(c);
	return 1;
}

// Preinstantiate Objects //////////////////////////////////////////////////////
RobbusLib Robbus = RobbusLib();


// END /////////////////////////////////////////////////////////////////////////
