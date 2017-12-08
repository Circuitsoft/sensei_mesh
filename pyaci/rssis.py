#!/usr/bin/env python3 -u

from aci_serial import AciUart
from datetime import datetime
import sys

aci = AciUart.AciUart(port='/dev/ttyUSB0', baudrate=115200)
outdat = open("rssi.csv", "a")

if 0 == outdat.tell():
    # If new CSV, write new data
    print('HH:MM:SS.mmmmmm', 'RSSI (dBm)',
          sep=',', end='\n', file=outdat)

for evt in aci.get_packet_from_uart():
    if evt[1] == 0x60: # AciEventAppEvt
        rssi = -evt[3] # RSSI in dBm of Heartbeat
        ts = datetime.now()
        print('%02d:%02d:%02d.%06d' % (ts.hour, ts.minute, ts.second, ts.microsecond), rssi,
              sep=',', end='\n', file=outdat)
        print('RSSI: %ddBm\r' % rssi, end='\r', flush=True)
