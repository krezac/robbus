#!/usr/bin/python

import serial
import time

def main(argv = None):
	ser = serial.Serial('/dev/ttyUSB1', 115200, timeout=30)
	data = ser.read(254)
	i = 0
	while i < len(data):
		print ord(data[i]), ord(data[i+1])
		i = i+2
	ser.close()
	return 0

if __name__ == "__main__":
	from sys import argv
	main(argv)
