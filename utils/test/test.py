#!/usr/bin/python

import serial
import time

def sendWrapped(ser,c, sum):
	if ord(c) < 4:
		ser.write("\x00")
		x = ser.read(1)
		print ">", x, ord(x)
		d = chr(ord(c)+4)
		ser.write(d)
		x = ser.read(1)
		print ">", x, ord(x)
	else:
		ser.write(c)
		x = ser.read(1)
		print ">", x, ord(x)
	return sum+ord(c)

def sendMessage(ser,alias,data,mask):
	sum = 0;
	if mask is None:
		ser.write("\x02")     #write regular packet start
	else:
		ser.write("\x03")     #write group packet start
	x = ser.read(1)
	print ">", x, ord(x)
	sum = sendWrapped(ser,alias,sum) 
	print sum
	if mask is not None:
		sum = sendWrapped(ser,mask,sum) 	
		print sum
	sum = sendWrapped(ser,chr(len(data)),sum)
	for c in data:
		sum = sendWrapped(ser,c,sum)	
		print sum
	outSum = (256-(sum % 256)) % 256
	sendWrapped(ser,chr(outSum),sum)

def dewrap_read(ser):
	x = ser.read(1)
	print "<", x, ord(x)
	if ord(x) == 0:
		x = ser.read(1)	
		print "<", x, ord(x)
		return chr(ord(x)-4)
	else:
		return x

# read reply
def readReply(ser):
	# consume outgoing message
	s = ""
	# and read incomming
	print "Receiving data..."
	x = ser.read(1) # head, no need to dewrap
	s += x
	print "<", x, ord(x)
	x = ser.read(1) # head, no need to dewrap
	s += x
	print "<", x, ord(x)
	lenstr = dewrap_read(ser)
	paylen = ord(lenstr);
	s += lenstr
	while paylen > 0:
		s += dewrap_read(ser);
		paylen = paylen-1
	s += dewrap_read(ser) #chsum
	return s

def parseReply(s):
	if len(s) < 3:
		print "Input too short"
		return
	s = s [1:] # cut head
	chSum = sum([ord(c) for c in s]) 
	if sum([ord(c) for c in s]) % 256 != 0:
		print "Invalid checksum", (chSum%256), (256-(chSum%256))
		return -1
	address = chr(ord(s[0]) - 128)
	print "Reply from:", address, " hex:", address.encode("hex")
	replySize = ord(s[1])
	print "      size:", replySize
	if replySize != len (s[2:-1]):
		print "Data size doesn't match real data length"
	data = s[2:-1]
	print "      data:", [ord(i) for i in data], " hex:", data.encode("hex")
	return 0


def main(argv = None):
	if len(argv) < 3:
		print "Usage: test.py address data [mask]"
		print "All address, data and mask are read as hexa values if"
		print "prefixed with \\x (or \\\\x to avoid shell expansion)"
		print "Example (bash): ./test.py KRT1 \\\\x0305"
		return
	
	ser = serial.Serial('/dev/robbus', 115200, timeout=1)
	if argv[1][0:2] == "\\x":
		print "Decoding hex address"
		address = argv[1][2:].decode("hex");
	else:
		address = argv[1];
	if len(address) != 1:
		print "Address must have 1 byte"
		return
	if argv[2][0:2] == "\\x":
		print "Decoding hex data", argv[2]
		data = argv[2][2:].decode("hex");
	else:
		data = argv[2];
	if len(argv) > 3:
		if argv[3][0:2] == "\\x":
			print "Decoding hex mask"
			mask = argv[3][2:].decode("hex");
		else:
			mask = argv[3];
		if len(mask) != 1:
			print "Mask must have 1 byte"
			return
	else:
		mask = None
	print "Sending..."
	sendMessage(ser,address,data,mask)
	ret = parseReply(readReply(ser))
	ser.close()
	return ret

if __name__ == "__main__":
	from sys import argv
	main(argv)
