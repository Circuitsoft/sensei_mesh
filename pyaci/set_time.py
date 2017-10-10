#!/usr/bin/env python3

from argparse import ArgumentParser
from aci import AciCommand
from aci_serial import AciUart
import time
import sensei_cmd

def set_time(serial_device):
    aci = AciUart.AciUart(port=serial_device, baudrate=115200)
    # Wait for serial connection to be ready
    time.sleep(2)
    cmd = sensei_cmd.SetTime()
    data = cmd.serialize()
    aci.write_aci_cmd(AciCommand.AciAppCommand(data=data,length=len(data)+1))
    aci.stop()

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("-d", "--device", dest="device", required=True,
                        help="Serial device, e.g. /dev/cu.usbserial-DO00C2G2")
    options = parser.parse_args()
    set_time(options.device)
