#!/usr/bin/env python3

from argparse import ArgumentParser
from aci import AciCommand
from aci_serial import AciUart
import time
import sensei_cmd

def configure_sensor(serial_device, sensor_id, serial_enabled, channel, sleep_enabled):
    aci = AciUart.AciUart(port=serial_device, baudrate=115200)
    # Wait for serial connection to be ready
    time.sleep(2)
    cmd = sensei_cmd.SetConfig(sensor_id, serial_enabled, channel, sleep_enabled)
    data = cmd.serialize()
    aci.write_aci_cmd(AciCommand.AciAppCommand(data=data,length=len(data)+1))

    # Wait for flash to be written
    time.sleep(2)
    aci.write_aci_cmd(AciCommand.AciRadioReset())

    aci.stop()

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("-d", "--device", dest="device", required=True,
                        help="Serial device, e.g. /dev/cu.usbserial-DO00C2G2")
    parser.add_argument('id', type=int, help='the id to assign this sensor')
    parser.add_argument('--no-sleeping', dest='sleep_enabled', action='store_false')
    parser.set_defaults(sleep_enabled=True)
    parser.add_argument('--no-serial', dest='serial_enabled', action='store_false')
    parser.set_defaults(serial_enabled=True)
    parser.add_argument('--channel', type=int,
                        help='bluetooth channel of sensei network: should be 1-39 ' \
                        + '(one of 37,38,39 usually best)')
    parser.set_defaults(channel=38)
    options = parser.parse_args()
    configure_sensor(options.device, options.id, options.serial_enabled,
                     options.channel, options.sleep_enabled)
