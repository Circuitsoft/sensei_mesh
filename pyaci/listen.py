#!/usr/bin/env python3

from argparse import ArgumentParser
from aci import AciCommand
from aci_serial import AciUart
import time
import sensei_cmd

def listen(serial_device):
    aci = AciUart.AciUart(port=serial_device, baudrate=115200)
    # Wait for serial connection to be ready
    time.sleep(2)
    while True:
        event = aci.events_queue.get()
        print(str.format("%.3f %s" %(time.time(), event)))

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("-d", "--device", dest="device", required=True, help="Serial device, e.g. /dev/cu.usbserial-DO00C2G2")
    options = parser.parse_args()
    listen(options.device)
