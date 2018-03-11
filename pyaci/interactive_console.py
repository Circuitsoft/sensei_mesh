#!/usr/bin/env python3

import logging
import IPython
from argparse import ArgumentParser
from traitlets import config
from aci import AciCommand
from aci_serial import AciUart
from queue import *
import time
import sensei_cmd
import struct


class Interactive(object):
    def __init__(self, acidev):
        self.acidev = acidev

    def close(self):
        self.acidev.stop()

    def EventsReceivedGet(self):
        events = []
        while 1:
            try:
                events.append(self.acidev.events_queue.get_nowait())
            except Empty:
                break
        return events

    def DevicePortGet(self):
        return self.acidev.serial.port

    # HCI commands
    def Echo(self, Data):
        self.acidev.write_aci_cmd(AciCommand.AciEcho(
            data=Data, length=(len(Data) + 1)))

    def RadioReset(self):
        self.acidev.write_aci_cmd(AciCommand.AciRadioReset())

    def AppCommand(self, data=[]):
        self.acidev.write_aci_cmd(AciCommand.AciAppCommand(
            data=data, length=len(data) + 1))

    def Init(self, AccessAddress=0x8E89BED6, MinInterval=100, Channel=39):
        self.acidev.write_aci_cmd(AciCommand.AciInit(
            access_address=AccessAddress, min_interval=MinInterval, channel=Channel))

    def ValueSet(self, Handle, Data):
        self.acidev.write_aci_cmd(AciCommand.AciValueSet(
            handle=Handle, data=Data, length=(len(Data) + 3)))

    def ValueEnable(self, Handle):
        self.acidev.write_aci_cmd(AciCommand.AciValueEnable(handle=Handle))

    def ValueDisable(self, Handle):
        self.acidev.write_aci_cmd(AciCommand.AciValueDisable(handle=Handle))

    def ValueGet(self, Handle):
        self.acidev.write_aci_cmd(AciCommand.AciValueGet(handle=Handle))

    def Start(self):
        self.acidev.write_aci_cmd(AciCommand.AciStart())

    def Stop(self):
        self.acidev.write_aci_cmd(AciCommand.AciStop())

    def FlagSet(self, Handle, FlagIndex, FlagValue=True):
        self.acidev.write_aci_cmd(AciCommand.AciFlagSet(
            handle=Handle, flag_index=FlagIndex, flag_value=FlagValue))

    def FlagGet(self, Handle, FlagIndex):
        self.acidev.write_aci_cmd(AciCommand.AciFlagGet(
            handle=Handle, flag_index=FlagIndex))

    def DFUData(self, Data):
        self.acidev.write_aci_cmd(AciCommand.AciDfuData(
            data=Data, length=(len(Data) + 1)))

    def BuildVersionGet(self):
        self.acidev.write_aci_cmd(AciCommand.AciBuildVersionGet())

    def AccessAddressGet(self):
        self.acidev.write_aci_cmd(AciCommand.AciAccessAddressGet())

    def ChannelGet(self):
        self.acidev.write_aci_cmd(AciCommand.AciChannelGet())

    def MinIntervalGet(self):
        self.acidev.write_aci_cmd(AciCommand.AciIntervalMinMsGet())

    # Experimental: to be removed
    def runCommand(self, command):
        self.AppCommand(command.serialize())

    def setTime(self):
        self.runCommand(sensei_cmd.SetTime())

    def setConfig(self, sensor_id, serial_enabled, mesh_channel, sleep_enabled):
        self.runCommand(sensei_cmd.SetConfig(
            sensor_id, serial_enabled, mesh_channel, sleep_enabled))

    def setMeshControl(self, wake_interval, tx_power, ble_enabled):
        power_levels = {
            0: 0x00,
            4: 0x04,
            -30: 0xD8,
            -20: 0xEC,
            -16: 0xF0,
            -12: 0xF4,
            -8: 0xF8,
            -4: 0xFC
        }
        power_code = power_levels.get(tx_power)
        if tx_power == None:
            raise Exception("Bad tx power level specified: %s" % tx_power)

        # little endian - unsigned short, 2x unsigned char
        data = struct.pack("<HBB", wake_interval, power_code, ble_enabled)
        self.ValueSet(sensei_cmd.MESH_HANDLE_MESH_CONTROL, data)

    def getConfig(self):
        self.runCommand(sensei_cmd.GetConfig())


def get_ipython_config(device):
    # import os, sys, IPython

    # os.environ['PYTHONSTARTUP'] = ''  # Prevent running this again
    c = config.get_config()
    c.TerminalInteractiveShell.confirm_exit = False
    c.TerminalInteractiveShell.logstart = True
    c.TerminalInteractiveShell.logfile = 'interactive_aci.ipython.log'
    return c


def start_ipython(options):
    comports = options.device.split(',')
    d = list()
    for dev_com in comports:
        d.append(Interactive(AciUart.AciUart(
            port=dev_com, baudrate=options.baudrate)))

    IPython.embed(config=get_ipython_config(options.device))
    for dev in d:
        dev.close()
    raise SystemExit(0)


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("-d", "--device", dest="device", required=True,
                        help="Device Communication port, e.g. COM216")
    parser.add_argument("-b", "--baudrate", dest="baudrate",
                        required=False, default='115200', help="Baud rate")
    options = parser.parse_args()
    start_ipython(options)
