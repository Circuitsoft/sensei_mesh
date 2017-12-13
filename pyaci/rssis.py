#!/usr/bin/env python3 -u

AREA_SENSOR_UART='/dev/cu.SLAB_USBtoUART'
POZYX_UART='/dev/cu.usbmodem144231'

from aci_serial import AciUart
from datetime import datetime
import sys
import threading

aci = AciUart.AciUart(port=AREA_SENSOR_UART, baudrate=115200)
outdat = open("rssi.csv", "a")

if 0 == outdat.tell():
    # If new CSV, write new data
    print('HH:MM:SS.mmmmmm', 'RSSI (dBm)', 'Distance (mm)',
          sep=',', end='\n', file=outdat)

last_rssi = last_dist = ''

def write_sample(rssi='', dist=''):
    print('%02d:%02d:%02d.%06d' % (ts.hour, ts.minute, ts.second, ts.microsecond), rssi, dist,
          sep=',', end='\n', file=outdat)
    if rssi:
        last_rssi = rssi
    if dist:
        last_dist = dist
    print('RSSI: %sdBm    Dist: %smm\r' % (last_rssi, last_dist), end='\r', flush=True)

def read_dists():
    sp = AciUart.Serial(POZYX_UART, baudrate=115200)
    try:
        while True:
            line = sp.read_until(terminator=b'\n')
            line.strip()
            line = '%5d' % int(line)
            write_sample(dist=line)
    except:
        pass

dist_thr = threading.Thread(target=read_dists)
dist_thr.start()

for evt in aci.get_packet_from_uart():
    if evt[1] == 0x60: # AciEventAppEvt
        rssi = -evt[3] # RSSI in dBm of Heartbeat
        ts = datetime.now()
        write_sample('%2d' % rssi)

dist_thr.join()
