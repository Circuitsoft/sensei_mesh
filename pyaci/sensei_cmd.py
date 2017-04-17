import time
import struct

class SetTime(object):
    OpCode = 0x02

    def __init__(self, time_to_set=None):
        if (time_to_set != None):
            self.epoch = time_to_set
        else:
            self.epoch = time.time()

    def serialize(self):
        ms = int(self.epoch % 1 * 1000)
        print("epoch = {epoch}".format(epoch=self.epoch))
        #print("Waiting {ms}ms".format(ms=(1000-ms)))
        return bytearray([self.OpCode]) + int(self.epoch).to_bytes(4, byteorder='little') + ms.to_bytes(2, byteorder='little')

class SetConfig(object):
    OpCode = 0x03

    def __init__(self, sensor_id=0, serial_enabled=False, mesh_channel=38):
        self.sensor_id = sensor_id
        self.serial_enabled = serial_enabled
        self.mesh_channel=mesh_channel

    def serialize(self):
        return struct.pack("BBBB", self.OpCode, self.sensor_id, self.serial_enabled, self.mesh_channel)

class GetConfig(object):
    OpCode = 0x04

    def serialize(self):
        return struct.pack("B", self.OpCode)