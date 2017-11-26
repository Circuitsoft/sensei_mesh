#!/usr/bin/env python3 -u

from argparse import ArgumentParser
from aci import AciCommand
from aci_serial import AciUart
from aci import AciEvent
from aci.AciEvent import SensorValues
from queue import Empty
import sys
import logging
import time
import sensei_cmd
from sensei import *
import datetime
import yaml
from os.path import expanduser
import os.path

root = logging.getLogger()
root.setLevel(logging.INFO)

ch = logging.StreamHandler(sys.stdout)
ch.setLevel(logging.INFO)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)
root.addHandler(ch)

class Uploader(object):
    # Synchronize once every minute
    TIME_SYNC_INTERVAL=60
    NO_DATA_TIMEOUT=35

    def __init__(self, sensei_config, options):
        self.sensei_config = sensei_config
        self.options = options

        self.aci = None
        api_url = sensei_config["server"]["url"] + 'api/v1/'
        self.restart_serial()
        self.api = Api(api_url, sensei_config["server"]["username"], sensei_config["server"]["password"])
        self.classroom_id = sensei_config["classroom_id"]
        self.updates = []

    def handle_heartbeat(self, hb):
        if hb.epoch_seconds != hb.received_at or abs(time.time() - hb.epoch_seconds) > 5:
          print(str.format("Sensor %d clock offset detected; issuing sync_time." %(hb.sensor_id)))
          self.sync_time()

    def event_is_sensor_update(self, evt):
        return (isinstance(evt, AciEvent.AciEventNew) or
               isinstance(evt, AciEvent.AciEventUpdate)) and evt.is_sensor_update()

    def handle_events(self, events):
        for evt in events:
            print(str(evt))
            if self.event_is_sensor_update(evt):
                update = evt.sensor_values()
                if update.is_valid:
                    self.updates.append(update)
            elif isinstance(evt, AciEvent.AciEventAppEvt) and evt.is_heartbeat():
                self.handle_heartbeat(evt.heartbeat_msg())

    def get_events(self):
        events = self.aci.get_events(timeout=5)
        if len(events) > 0:
            self.last_event = time.time()
            self.handle_events(events)

    def run_app_command(self, command):
        data = command.serialize()
        events = self.aci.write_aci_cmd(AciCommand.AciAppCommand(data=data,length=len(data)+1))
        self.handle_events(events)
        return events

    def sync_time(self):
        self.last_time_sync = time.time()
        self.run_app_command(sensei_cmd.SetTime())

    def get_config(self):
        return self.run_app_command(sensei_cmd.GetConfig())

    def radio_obs_from_update(self, update):
        if not update.is_valid:
            return []
        obs = []
        for remote_id, rssi in zip(update.proximity_ids, update.proximity_rssi):
            ob_time = datetime.datetime.utcfromtimestamp(update.valid_time)
            if remote_id > 0 and rssi > 0:
                obs.append(RadioObservation(self.classroom_id, update.sensor_id, remote_id, ob_time, -rssi))
        return obs

    def accelerometer_measurement_from_update(self, update):
        if update.is_valid and (update.accel_x or update.accel_y or update.accel_z):
            ob_time = datetime.datetime.utcfromtimestamp(update.valid_time)
            return AccelerometerObservation(self.classroom_id, update.sensor_id,
                ob_time, update.accel_x, update.accel_y, update.accel_z)

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
                print(str.format("Exception while %s: %s" %(description, repr(e))))
                num_retries = num_retries - 1
                time.sleep(sleep_duration)


    def upload_radio_observations(self, obs):
        self.handle_exceptions_with_sleep_retry(lambda: self.api.upload_radio_observations(obs), 1, 3, "uploading radio obs")

    def upload_accelerometer_observations(self, obs):
        self.handle_exceptions_with_sleep_retry(lambda: self.api.upload_accelerometer_observations(obs), 1, 3, "uploading accelerometer obs")

    def upload_accelerometer_event(self, event_type, sensor_id, valid_time):
        ob_time = datetime.datetime.utcfromtimestamp(valid_time)
        events = [AccelerometerEvent(self.classroom_id, sensor_id, ob_time, event_type)]
        self.handle_exceptions_with_sleep_retry(lambda: self.api.upload_accelerometer_events(events), 1, 3, "uploading accelerometer obs")

    def handle_updates_from_serial(self):
        if not self.options.dry_run:
            obs = [self.radio_obs_from_update(update) for update in self.updates]
            flattened_obs = [ob for sublist in obs for ob in sublist]
            if len(flattened_obs) > 0:
                self.upload_radio_observations(flattened_obs)
            accelerometer_obs = []
            for update in self.updates:
                ob = self.accelerometer_measurement_from_update(update)
                if ob:
                    accelerometer_obs.append(ob)
                if update.status & SensorValues.STATUS_JOSTLE_FLAG:
                    self.upload_accelerometer_event('jostle', update.sensor_id, update.valid_time)
                    print("Jostle from %d" %(update.sensor_id))
            if len(accelerometer_obs) > 0:
                self.upload_accelerometer_observations(accelerometer_obs)

    def run(self):
        # Wait for serial connection to be ready
        time.sleep(3)

        # Sync time
        print("Syncing time")
        self.sync_time()

        while True:
            self.get_events()
            if len(self.updates) > 0:
                self.handle_updates_from_serial()
            else:
                time.sleep(0.5)

            if time.time() - self.last_time_sync > Uploader.TIME_SYNC_INTERVAL:
                self.sync_time()

            if time.time() - self.last_event > Uploader.NO_DATA_TIMEOUT:
                self.restart_serial()

if __name__ == '__main__':

    parser = ArgumentParser()
    parser.add_argument("-c", "--config", dest="config", help="Configuration file, e.g. ~/.sensei.yaml")
    parser.add_argument("-d", "--dry-run", dest="dry_run", action='store_const', const=True,
                        help="Dry run. Do not actually upload anything")
    options = parser.parse_args()

    config_path = options.config or expanduser("~") + "/.sensei.yaml"
    if os.path.isfile(config_path):
        with open(config_path, 'r') as stream:
            try:
                sensei_config = yaml.load(stream)
                uploader = Uploader(sensei_config, options)
                uploader.run()
            except yaml.YAMLError as exc:
                print(exc)
    else:
        print(str.format("Please configure settings in %s" %(config_path)))
        exit(-1)
