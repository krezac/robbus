/*
  Robbus.h - Library for Robbus communication protocol.
  Created by Kamil Rezac, October 18, 2011.
  Released into the public domain.
*/
#ifndef Robbus_h
#define Robbus_h

#include "WProgram.h"

// abstract parent class for communication wrappers
class RobbusCommWrapper
{
	public:
		RobbusCommWrapper() {}
		virtual void begin() = 0;
		virtual int available() = 0;
		virtual int read() = 0;
		virtual void write(byte) = 0;

};

class RobbusLib
{
	public:
		RobbusLib();
		void begin(RobbusCommWrapper* commWrapperPtr, byte address, byte inDataSize, byte outDataSize, byte* (*)(byte*));
		void process();
	private:
		// data fields
		RobbusCommWrapper* commWrapper;
		byte* (*commandHandler)(byte*);
		byte robbusState;    //! state of the processing state machine
		byte payloadLength;
		byte checkSum;
		byte deviceAddress;
		byte* usartBuffer;
		byte usartBufferSize;
		byte usartBufferIndex;
		byte incomingDataSize;
		byte outgoingDataSize;

		// private functions
		byte doServiceCommand(void);
		byte sendWrapped(byte c);		
};

// "singleton"
extern RobbusLib Robbus;

#if defined(UBRRH) || defined(UBRR0H)
class RobbusCommWrapper_Serial : public RobbusCommWrapper
{
	public:
		RobbusCommWrapper_Serial() { }
		virtual void begin() { Serial.begin(115200); }
		virtual int available() { return Serial.available(); }
		virtual int read() { return Serial.read(); }
		virtual void write(byte data) { Serial.write(data); }
};
#endif

#if defined(UBRR1H) || defined(_USART_H_)
class RobbusCommWrapper_Serial1 : public RobbusCommWrapper
{
	public:
		RobbusCommWrapper_Serial1() { }
		virtual void begin() { Serial1.begin(115200); }
		virtual int available() { return Serial1.available(); }
		virtual int read() { return Serial1.read(); }
		virtual void write(byte data) { Serial1.write(data); }
};
#endif

#if defined(UBRR2H) || defined(_USART_H_)
class RobbusCommWrapper_Serial2 : public RobbusCommWrapper
{
	public:
		RobbusCommWrapper_Serial2() { }
		virtual void begin() { Serial2.begin(115200); }
		virtual int available() { return Serial2.available(); }
		virtual int read() { return Serial2.read(); }
		virtual void write(byte data) { Serial2.write(data); }
};
#endif

#if defined(UBRR3H) || defined(_USART_H_)
class RobbusCommWrapper_Serial3 : public RobbusCommWrapper
{
	public:
		RobbusCommWrapper_Serial3() { }
		virtual void begin() { Serial3.begin(115200); }
		virtual int available() { return Serial3.available(); }
		virtual int read() { return Serial3.read(); }
		virtual void write(byte data) { Serial3.write(data); }
};
#endif

#if defined(_USART_H_) && defined(STM32_HIGH_DENSITY)
// maple only
class RobbusCommWrapper_Serial4 : public RobbusCommWrapper
{
	public:
		RobbusCommWrapper_Serial4() { }
		virtual void begin() { Serial4.begin(115200); }
		virtual int available() { return Serial4.available(); }
		virtual int read() { return Serial4.read(); }
		virtual void write(byte data) { Serial4.write(data); }
};

class RobbusCommWrapper_Serial5 : public RobbusCommWrapper
{
	public:
		RobbusCommWrapper_Serial5() { }
		virtual void begin() { Serial5.begin(115200); }
		virtual int available() { return Serial5.available(); }
		virtual int read() { return Serial5.read(); }
		virtual void write(byte data) { Serial5.write(data); }
};
#endif

#if defined(_USB_SERIAL_H_)
class RobbusCommWrapper_SerialUSB : public RobbusCommWrapper
{
  public:
    RobbusCommWrapper_SerialUSB() { }
    virtual void begin() { SerialUSB.begin(); }
    virtual int available() { return SerialUSB.available(); }
    virtual int read() { return SerialUSB.read(); }
    virtual void write(byte data) { SerialUSB.write(data); }
};
#endif

#endif