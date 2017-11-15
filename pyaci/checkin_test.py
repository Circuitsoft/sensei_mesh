#!/usr/bin/env python3

from aci import AciCommand
from aci_serial import AciUart
from aci import AciEvent
from queue import Empty
import time
import sensei_cmd
from sensei import *
import yaml
import os

# Note, this utility uses time since last update received, not since timestamp
# of update.

class CheckinTimer(object):
    # Synchronize once every minute
    TIME_SYNC_INTERVAL=60
    NO_DATA_TIMEOUT=35

    def __init__(self, sensei_config):
        self.sensei_config = sensei_config

        self.aci = None
        self.restart_serial()
        self.sensors = {}
        self.last_header = []
        self.last_header_interval = 0

    def update_sensor(self, sensor_id):
        if sensor_id == 61:
            return
        self.sensors[sensor_id] = time.time()
        new_header = sorted(self.sensors)
        if self.last_header != new_header:
            self.last_header_interval = 0
        self.last_header_interval %= 25
        if 0 == self.last_header_interval:
            print('SensorID:' + '  '.join('%6d' % i for i in new_header))
            self.last_header = new_header
        self.last_header_interval += 1
        t = time.time()
        l = '  '.join('%6.3f' % (t - self.sensors[i])
                        for i in new_header)
        print(time.strftime('%X ') + l)

    def handle_heartbeat(self, hb):
        if hb.epoch_seconds != hb.received_at or abs(time.time() - hb.epoch_seconds) > 5:
          print("Sensor %d clock offset detected; issuing sync_time." % hb.sensor_id)
          self.sync_time()

    def get_sensor_updates(self):
        updates = []
        while True:
            try:
                evt = self.aci.events_queue.get_nowait()
                self.last_event = time.time()
                if isinstance(evt, AciEvent.AciEventUpdate):
                    self.update_sensor(evt.ValueHandle & 0xff)
                if isinstance(evt, AciEvent.AciEventNew) and evt.is_sensor_update():
                    updates.append(evt.sensor_values())
                elif isinstance(evt, AciEvent.AciEventAppEvt) and evt.is_heartbeat():
                    self.handle_heartbeat(evt.heartbeat_msg())
            except Empty:
                break
        return updates

    def run_app_command(self, command):
        data = command.serialize()
        return self.aci.write_aci_cmd(AciCommand.AciAppCommand(data=data,length=len(data)+1))

    def sync_time(self):
        self.last_time_sync = time.time()
        result = self.run_app_command(sensei_cmd.SetTime())

    def get_config(self):
        return self.run_app_command(sensei_cmd.GetConfig())

    def restart_serial(self):
        if self.aci:
            print("Restarting serial connection")
            self.aci.stop()
        else:
            print("Starting serial connection")

        device = self.sensei_config['mesh_network']['serial_path']
        self.aci = AciUart.AciUart(port=device, baudrate=115200)
        self.last_event = time.time()

    def handle_exceptions_with_sleep_retry(self, callable, sleep_duration, num_retries, description):
        while (num_retries > 0):
            try:
                return callable()
            except Exception as e:
                print("Exception while %s: %r" %(description, e))
                num_retries = num_retries - 1
                time.sleep(sleep_duration)


    def run(self):
        # Wait for serial connection to be ready
        time.sleep(3)
        print("Getting config")
        self.get_config()

        # Sync time
        print("Syncing time")
        self.sync_time()

        while True:
            self.get_sensor_updates()
            time.sleep(0.5)

            if time.time() - self.last_time_sync > CheckinTimer.TIME_SYNC_INTERVAL:
                self.sync_time()

            if time.time() - self.last_event > CheckinTimer.NO_DATA_TIMEOUT:
                self.restart_serial()

if __name__ == '__main__':

    config_path = os.path.join(os.path.expanduser("~"), ".sensei.yaml")
    if os.path.isfile(config_path):
        with open(config_path, 'r') as stream:
            try:
                sensei_config = yaml.load(stream)
                uploader = CheckinTimer(sensei_config)
                uploader.run()
            except yaml.YAMLError as exc:
                print(exc)
    else:
        print("Please configure settings in %s" % config_path)
        exit(-1)
