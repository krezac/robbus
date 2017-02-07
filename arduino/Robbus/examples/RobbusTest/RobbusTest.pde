// include the definition (this is necessary to use Robbus)
#include <Robbus.h>

// declare output data array (application-level code can modify it as needed)
// the length must match the output data length
byte outData[2];      

// instatiate communication wrapper (specify which way to communicate)
// you can use one of the predefined: RobbusCommWrapper_Serial,
// RobbusCommWrapper_Serial1, RobbusCommWrapper_Serial2...
// or define your own (i.e. for ethernet shield). The code will look like:
// class RobbusCommWrapper_MyOwn : public RobbusCommWrapper
// {
//   public:
//     RobbusCommWrapper_MyOwn() { MyCommLayer myCommLayer = MyCommLayer(); }
//     virtual void begin() { myCommLayer.initialize(115200); }
//     virtual int available() { return myCommLayer.areDataReady(); }
//     virtual int read() { return myCommLayer.getData(); }
//     virtual void write(uint8_t data) { myCommLayer.sendData(data); }
//   private:
//     MyCommLayer myCommLayer;
//}; 
// for Maple IDE use RobbusCommWrapper_SerialX where X=(1, 2, 3, USB) instead (Serial w/o suffix is not supported)
RobbusCommWrapper_Serial RobbusOnSerial = RobbusCommWrapper_Serial();

// message handler 
// it will be called on reception of Robbus message
// the parameter is pointer to received data (the length will match incoming data length)
// the return value is pointer to data which will be sent back to master
byte* robbusHandler(byte* data)  
{
	outData[0] = 'K';     // write some output data
	outData[1] = data[0]; // (you can change the data anywhere in the code)
	return outData;       // and return pointer to data to be sent 
}

void setup()
{
	// initialize and start Robbus client code
	// the parameters are:
	// * pointer to communication wrapper
	// * node address (value between 4 and 127)
	// * incoming data length
	// * outgoing data length
	// * pointer to handler function
	Robbus.begin(&RobbusOnSerial, 'a', 1, 2, robbusHandler);  
}

void loop()
{
	// process function MUST be called in the loop otherwise the messages from Robbus will not be processed 
	Robbus.process();
}
