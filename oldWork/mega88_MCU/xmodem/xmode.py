#!/usr/bin/python
#-*-coding: UTF-8 -*-

import serial
from time import sleep
from xmodem import XMODEM


s = serial.Serial(port='/dev/ttyUSB0', baudrate=9600, bytesize=8, parity='N', stopbits=2, timeout=None, xonxoff=0, rtscts=0)
s.isOpen()

def getc(size, timeout = 1):
    return s.read(size)

def putc(data, timeout = 1):
    return s.wirte(data)

modem = XMODEM(getc, putc)

f = open('TEST.hex', 'rb')
stream = f.readlines()
status = modem.send(stream, retry = 8)
s.close()
stream.close()
