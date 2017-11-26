import time
import logging
import traceback
import threading
import collections
import sys
from queue import *
from serial import Serial
from aci import AciEvent, AciCommand

logging.getLogger(__name__)

EVENT_QUEUE_BUFFER_MAX_SIZE = 100

class AciDevice(object):
    def __init__(self, device_name):
        self.device_name = device_name
        self._pack_recipients = []
        self._cmd_recipients  = []
        self.lock = threading.Event()
        self.events_queue = Queue(maxsize = EVENT_QUEUE_BUFFER_MAX_SIZE)

    # Returns list of events
    def get_events(self, timeout=1):
        events = []
        while len(events) < EVENT_QUEUE_BUFFER_MAX_SIZE and not self.events_queue.empty():
            evt = self.events_queue.get_nowait()
            if evt:
                events.append(evt)

        if len(events) == 0 and timeout > 0:
            try:
                evt = self.events_queue.get(timeout=timeout)
                if evt:
                    events.append(evt)
            except Empty:
                pass

        return events

    def AddPacketRecipient(self, function):
        self._pack_recipients.append(function)

    def AddCommandRecipient(self, function):
        self._cmd_recipients.append(function)

    def ProcessPacket(self, packet):
        self.lock.set()
        for fun in self._pack_recipients[:]:
            try:
                fun(packet)
            except:
                logging.error('Exception in pkt handler %r', fun)
                logging.error('traceback: %s', traceback.format_exc())

    def ProcessCommand(self, command):
        for fun in self._cmd_recipients[:]:
            try:
                fun(command)
            except:
                logging.error('Exception in pkt handler %r', fun)
                logging.error('traceback: %s', traceback.format_exc())

    # Returns list of events
    def write_aci_cmd(self, cmd, timeout=1):
        events = []
        if isinstance(cmd,AciCommand.AciCommandPkt):
            self.WriteData(cmd.serialize())
            events = self.get_events(timeout)
            if len(events) == 0:
                logging.info('cmd %s, timeout waiting for event' % (cmd.__class__.__name__))
        else:
            logging.error('The command provided is not valid: %s\nIt must be an instance of the AciCommandPkt class (or one of its subclasses)', str(cmd))
        return events



class AciUart(threading.Thread, AciDevice):
    def __init__(self, port, baudrate=115200, device_name=None, rtscts=False):
        threading.Thread.__init__(self)
        self.daemon = True
        if not device_name:
            device_name = port
        AciDevice.__init__(self, device_name)

        self._write_lock = threading.Lock()

        logging.debug("log Opening port %s, baudrate %s, rtscts %s", port, baudrate, rtscts)
        self.serial = Serial(port=port, baudrate=baudrate, rtscts=rtscts, timeout=0.1)

        self.keep_running = True
        self.start()

    def __del__(self):
        self.stop()

    def stop(self):
        self.keep_running = False

    def get_packet_from_uart(self):
        tmp = None
        byte_is_escape = False
        while self.keep_running:
            byte = self.serial.read(1)
            if len(byte):
                byte = byte[0]
                if (byte & 0xf0) == 0x70:
                    # Framed byte
                    if byte == 0x71:
                        tmp = b''
                        byte_is_escape = False
                    if byte == 0x72:
                        byte_is_escape = True
                    if tmp and byte == 0x73:
                        yield tmp
                        tmp = None
                else:
                    if tmp != None:
                        if byte_is_escape:
                            tmp += bytearray([byte ^ 0x20])
                            byte_is_escape = False
                        else:
                            tmp += bytearray([byte])

    def run(self):
        for pkt in self.get_packet_from_uart():
            try:
                pkt = list(pkt)
                if len(pkt) < 2:
                    logging.error('Invalid packet: %r', pkt)
                    continue
                parsedPacket = AciEvent.AciEventDeserialize(pkt)
            except Exception:
                logging.error('Exception with packet %r', pkt)
                logging.error('traceback: %s', traceback.format_exc())
                parsedPacket = None

            if parsedPacket:
                try:
                    self.events_queue.put_nowait(parsedPacket)
                except Full:
                    logging.error('event queue full')
                    pass
                logging.debug('parsedPacket %r %s', parsedPacket, parsedPacket)
                self.ProcessPacket(parsedPacket)

        self.serial.close()
        logging.debug("exited read event")

    def WriteData(self, data):
        data = bytearray(data)
        with self._write_lock:
            if self.keep_running:
                self.serial.write(bytearray([0x71]))
                for b in data:
                    if (b & 0xf0) == 0x70:
                        self.serial.write(bytearray([0x72]))
                        b = b ^ 0x20
                    self.serial.write(bytearray([b]))
                self.serial.write(bytearray([0x73]))
                self.ProcessCommand(data)

    def __repr__(self):
        return '%s(port="%s", baudrate=%s, device_name="%s")' % (self.__class__.__name__, self.serial.port, self.serial.baudrate, self.device_name)
