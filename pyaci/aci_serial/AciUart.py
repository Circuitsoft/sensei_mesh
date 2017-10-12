import time
import logging
import traceback
import threading
import collections
from queue import *


from serial import Serial
from aci import AciEvent, AciCommand

EVT_Q_BUF = 32

class AciDevice(object):
    def __init__(self, device_name):
        self.device_name = device_name
        self._pack_recipients = []
        self._cmd_recipients  = []
        self.lock = threading.Event()
        self.events = list()

    @staticmethod
    def Wait(self, timeout=1):
        if len(self.events) == 0:
            self.lock.wait(timeout)
        self.lock.clear()

        if len(self.events) == 0:
            return None
        else:
            event = self.events[:]
            self.events.clear()
            return event

    def AddPacketRecipient(self, function):
        self._pack_recipients.append(function)

    def AddCommandRecipient(self, function):
        self._cmd_recipients.append(function)

    def ProcessPacket(self, packet):
        self.events.append(packet)
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

    def write_aci_cmd(self, cmd):
        retval = None
        if isinstance(cmd,AciCommand.AciCommandPkt):
            self.WriteData(cmd.serialize())
            retval = self.Wait(self)
            if retval == None:
                logging.info('cmd %s, timeout waiting for event',
			     cmd.__class__.__name__)
        else:
            logging.error('The command provided is not valid: %s\n' \
			+ 'It must be an instance of the AciCommandPkt class ' \
			+ '(or one of its subclasses)', str(cmd))
        return retval



class AciUart(threading.Thread, AciDevice):
    def __init__(self, port, baudrate=115200, device_name=None, rtscts=False):
        self.events_queue = Queue(maxsize = EVT_Q_BUF)
        threading.Thread.__init__(self)
        self.daemon = True
        if not device_name:
            device_name = port
        AciDevice.__init__(self, device_name)

        self._write_lock = threading.Lock()

        logging.debug("log Opening port %s, baudrate %s, rtscts %s",
                      port, baudrate, rtscts)
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
        return '%s(port="%s", baudrate=%s, device_name="%s")' % (self.__class__.__name__,
                self.serial.port, self.serial.baudrate, self.device_name)
