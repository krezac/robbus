#!/usr/bin/python

import serial
import time

def main(argv = None):
	if len(argv) < 2:
		print "Usage: loadtest.py iterations"
		return
	iters = int(argv[1])
	print "Running", iters, "iterations"
	ser = serial.Serial('/dev/robbus', 115200, timeout=1)
	requestStr = ""
	requestStr += chr(2)
	requestStr += chr(114)
	requestStr += chr(0)
	requestStr += chr(5)
	requestStr += chr(18)
	requestStr += chr(123)

	expectedReply = requestStr
	expectedReply += chr(2)
	expectedReply += chr(242)
	expectedReply += chr(0)
	expectedReply += chr(5)
	expectedReply += chr(63)
	expectedReply += chr(206)

	failCount = 0

	for i in range(iters):
		ser.write(requestStr)
		#print "sent"
		replyStr = ser.read(len(expectedReply))
		#if len(replyStr) < len(expectedReply):
		#	replyStr += ser.read(len(expectedReply)-len(replyStr))
		if replyStr != expectedReply:
			#print "fail", len(replyStr)
			failCount = failCount+1
	ser.close()
	print "Fails ", failCount
	return 0

if __name__ == "__main__":
	from sys import argv
	main(argv)
