#! /usr/bin/env python
import serial
import time
import thread
import threading

#This test application pings and moves servo #14. Use only on a servo with nothing mounted on it!
#Requires PySerial

com_port = "COM19"
baudrate = 1000000

ser = serial.Serial(com_port, baudrate)  # open serial port

stop_flag = threading.Event()

def read_and_print():
    global ser, stop_flag
    while not stop_flag.is_set():
        print hex(ord(ser.read(1)))
        time.sleep(0.001)
        
plop = thread.start_new_thread(read_and_print, ())


ser.write('\xFF\xFF\x0E\x02\x01\xEE') #ping
time.sleep(0.05)

ser.write('\xFF\xFF\x0E\x04\x03\x19\x01\xCF') #set torque
time.sleep(0.5)

try:
    while True:
        ser.write('\xFF\xFF\x0E\x07\x03\x1E\x00\x02\x00\x02\xC5') #Movet to ...
        time.sleep(1)
        ser.write('\xFF\xFF\x0E\x07\x03\x1E\x00\x01\x00\x02\xC6') #Movet to ...
        time.sleep(1)
except:
    stop_flag.set()
