import logging
from aci import AciCommand
from struct import *

MAX_DATA_LENGTH = 30

def AciEventDeserialize(pkt):


    eventLUT = {
        0x81: AciDeviceStarted,
        0x82: AciEchoRsp,
        0x84: AciCmdRsp,
        0xB3: AciEventNew,
        0xB4: AciEventUpdate,
        0xB5: AciEventConflicting,
        0xB6: AciEventTX,
        0x60: AciEventAppEvt

    }

    opcode = pkt[1]
    if opcode in eventLUT:
        return eventLUT[opcode](pkt)
    else:
        return AciEventPkt(pkt)

def AciStatusLookUp(StatusCode):
    StatusCodeLUT = {
        0x00: "SUCCESS",
        0x80: "ERROR_UNKNOWN",
        0x81: "ERROR_INTERNAL",
        0x82: "ERROR_CMD_UNKNOWN",
        0x83: "ERROR_DEVICE_STATE_INVALID",
        0x84: "ERROR_INVALID_LENGTH",
        0x85: "ERROR_INVALID_PARAMETER",
        0x86: "ERROR_BUSY",
        0x87: "ERROR_INVALID_DATA",
        0x90: "ERROR_PIPE_INVALID",
        0xF0: "RESERVED_START",
        0xFF: "RESERVED_END"
    }
    if StatusCode in StatusCodeLUT:
        return StatusCodeLUT[StatusCode]
    else:
        return "UNKNOWN ERROR: 0x%02x" %StatusCode

class AciEventPkt(object):
    Len = 1
    OpCode = 0x00
    Data = []
    def __init__(self, pkt):
        self.Len = pkt[0]
        if self.Len == 0 or self.Len > MAX_DATA_LENGTH:
            logging.error("Invalid length: %d, pkt: %s", self.Len, str(pkt) )
        else:
            try:
                self.OpCode = pkt[1]
                if self.Len > 1:
                    self.Data = pkt[2:]
            except:
                logging.error('Packet size must be > 1, packet contents: %s', str(pkt))

    def __repr__(self):
        return "%s length:%d opcode:0x%02x data:%s" % (self.__class__.__name__,
                                                       self.Len, self.OpCode, self.Data)

class AciDeviceStarted(AciEventPkt):
    #OpCode = 0x81
    def __init__(self,pkt):
        super(AciDeviceStarted, self).__init__(pkt)
        if self.Len != 4:
            logging.error("Invalid length for %s event: %s",
                               self.__class__.__name__, str(pkt))
        else:
            self.OperatingMode = pkt[2]
            self.HWError = pkt[3]
            self.DataCreditAvailable = pkt[4]

    def __repr__(self):
        return "%s length:%d opcode:0x%02x operating_mode:0x%02x hw_error:0x%02x " \
             + "data_credit_available:0x%02x" % (self.__class__.__name__,
                                                 self.Len, self.OpCode,
                                                 self.OperatingMode, self.HWError,
                                                 self.DataCreditAvailable)

class AciEchoRsp(AciEventPkt):
    #OpCode = 0x82
    def __init__(self,pkt):
        super(AciEchoRsp, self).__init__(pkt)

class AciCmdRsp(AciEventPkt):
    #OpCode = 0x84
    def __init__(self,pkt):
        super(AciCmdRsp, self).__init__(pkt)
        if self.Len < 3:
            logging.error("Invalid length for %s event: %s",
                               self.__class__.__name__, str(pkt))
        else:
            self.CommandOpCode = pkt[2]
            self.StatusCode = pkt[3]
            self.Data = pkt[4:]

    def __repr__(self):
        return "%s length:%d opcode:0x%02x command_opcode:%s status_code:%s data:%s" % (
                    self.__class__.__name__, self.Len, self.OpCode,
                    AciCommand.AciCommandLookUp(self.CommandOpCode),
                    AciStatusLookUp(self.StatusCode), self.Data)

class SensorValues(object):
    STATUS_JOSTLE_FLAG = 0x01

    def __init__(self, sensor_id, data):
        self.sensor_id = sensor_id
        if len(data) == 19:
            self.proximity_ids = data[0:5]
            self.proximity_rssi = data[5:10]
            self.battery = data[10]
            self.accel_x = data[11]
            self.accel_y = data[12]
            self.accel_z = data[13]
            self.status = data[14]
            self.valid_time = unpack('<i',bytearray(data[15:19]))[0]
            self.is_valid = True
        else:
            self.is_valid = False
            self.status = None
            self.data = data
    def __repr__(self):
        if self.is_valid:
            return "SensorValues: sensor_id:{sensor_id} proximity_ids:{proximity_ids} " \
                 + "proximity_rssi:{proximity_rssi} battery:{battery}, accel:(" \
                 + "{accel_x}, {accel_y}, {accel_z}), status:{status}, " \
                 + "valid_time:{valid_time}".format(**vars(self))
        else:
            return "SensorValues: invalid data from sensor {sensor_id}: ({data})"\
                    .format(**vars(self))

class AciEventNew(AciEventPkt):
    #OpCode = 0xB3
    def __init__(self,pkt):
        super(AciEventNew, self).__init__(pkt)
        if self.Len < 3:
            logging.error("Invalid length for %s event: %s",
                               self.__class__.__name__, str(pkt))
        else:
            self.ValueHandle = (pkt[3] << 8) + pkt[2]
            self.VersionDelta = (pkt[5] << 8) + pkt[4]
            self.Data = pkt[6:]

    def is_sensor_update(self):
        return self.ValueHandle >> 8 == 0x01

    def sensor_values(self):
        return SensorValues(self.ValueHandle & 0xff, self.Data)

    def __repr__(self):
        if self.is_sensor_update():
            return "%s(%d) %s" % (self.__class__.__name__, self.VersionDelta,
                                  self.sensor_values())
        else:
            return "%s length:%d opcode:0x%02x value_handle:0x%04x data:%s" % (
                self.__class__.__name__, self.Len, self.OpCode,
                self.ValueHandle, self.Data)

class AciEventUpdate(AciEventNew):
    #OpCode = 0xB4
    def __init__(self,pkt):
        super(AciEventUpdate, self).__init__(pkt)

class AciEventConflicting(AciEventNew):
    #OpCode = 0xB5
    def __init__(self,pkt):
        super(AciEventConflicting, self).__init__(pkt)

class AciEventTX(AciEventNew):
    #OpCode = 0xB6
    def __init__(self,pkt):
        super(AciEventTX, self).__init__(pkt)

class HeartbeatMsg(object):
    def __init__(self, data):
        if len(data) != 18:
            raise ValueError("Error: expected 18 bytes, got %s" % data)
        (self.rssi, self.received_at, self.received_at_ms, self.local_clock_version,
         self.sensor_id, self.epoch_seconds, self.epoch_ms, self.clock_version
         ) = unpack('<BiHHBiHH', bytearray(data))

    def __repr__(self):
        return "Heartbeat: rssi:{rssi} sensor_id:{sensor_id} epoch:{epoch_seconds} " \
             + "ms:{epoch_ms} clock_version:{clock_version} received_at:{received_at} " \
             + "received_at_ms:{received_at_ms} " \
             + "local_clock_version:{local_clock_version}".format(**vars(self))

class AciEventAppEvt(AciEventPkt):
    APP_EVENT_OPCODE_HEARTBEAT = 0x01

    #OpCode = 0x60
    def __init__(self,pkt):
        super(AciEventAppEvt, self).__init__(pkt)
        self.app_evt_opcode = self.Data[0]

    def is_heartbeat(self):
        return self.app_evt_opcode == AciEventAppEvt.APP_EVENT_OPCODE_HEARTBEAT

    def heartbeat_msg(self):
        return HeartbeatMsg(self.Data[1:])

    def __repr__(self):
        if self.is_heartbeat():
            return repr(self.heartbeat_msg())
        #if self.app_opcode == APP_EVENT_OPCODE_HEARTBEAT:
