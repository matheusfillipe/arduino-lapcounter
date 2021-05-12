#!/bin/python3
import time
import sys
import serial
import signal

args = sys.argv
usb = args[1] if len(args) > 1 else '/dev/ttyUSB0'

ser = serial.Serial(usb, 9600)
time.sleep(2)
def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    ser.close()
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)
while True:
    msg = ser.readline()
    try:
        print(msg.decode().rstrip())
    except:
        print(msg)
