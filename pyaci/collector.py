#!/usr/bin/python3 -u

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
import json
import yaml
from os.path import expanduser
import os.path
from redis import Redis
from datetime_modulo import datetime, dt
from datetime import timedelta
import struct

root = logging.getLogger()
root.setLevel(logging.INFO)

ch = logging.StreamHandler(sys.stdout)
ch.setLevel(logging.INFO)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
ch.setFormatter(formatter)
root.addHandler(ch)

MOTHERNODE_ID = 61

class MeshControlManager(object):
    def __init__(self, set_value):
        self.set_value = set_value
        self.target_mca_version = 0
        self.sent_yet = False

        self.interval = 10
        self.txpwr = -4
        self.enable_ble = False
        self.sleep_time = 0
        self.wake_time = 0

    def set_mesh_control(self, interval=None, txpwr=None, enable_ble=None,
                               sleep_time=None, wake_time=None):
        if interval is not None:
            self.interval = interval
            self.target_mca_version = 0
        if txpwr is not None:
            self.txpwr = txpwr
            self.target_mca_version = 0
        if enable_ble is not None:
            self.enable_ble = enable_ble
            self.target_mca_version = 0

        new_sleep_time = self.sleep_time
        if isinstance(sleep_time, dt.datetime):
            sleep_time -= datetime(1970,1,1)
            sleep_time = sleep_time.total_seconds() + time.timezone
        if sleep_time is not None:
            sleep_time = int(sleep_time)
            new_sleep_time = sleep_time

        new_wake_time = self.wake_time
        if isinstance(wake_time, dt.datetime):
            wake_time -= datetime(1970,1,1)
            wake_time = wake_time.total_seconds() + time.timezone
        if wake_time is not None:
            wake_time = int(wake_time)
            new_wake_time = wake_time

        if (new_sleep_time > 0 and
            new_wake_time > 0 and
            new_wake_time > new_sleep_time and
            (self.sleep_time != new_sleep_time or
                self.wake_time != new_wake_time)):
            self.sleep_time = new_sleep_time
            self.wake_time = new_wake_time
            self.target_mca_version = 0

        if self.target_mca_version == 0:
            self.send_mesh_control()

    def send_mesh_control(self):
        d = struct.pack("<HbBII", self.interval, self.txpwr, self.enable_ble,
                                  self.sleep_time, self.wake_time)
        self.set_value(0x0201, d)

    def handle_aci_eventupdate(self, sensor_id, sensor_mca):
        if self.target_mca_version == 0:
            if sensor_id == MOTHERNODE_ID:
                self.target_mca_version = sensor_mca
        elif not self.sent_yet:
            if (sensor_id != MOTHERNODE_ID) and (self.target_mca_version > sensor_mca):
                self.send_mesh_control()
                self.sent_yet = True

    def handle_new_collection_period(self):
        self.sent_yet = False

class ScheduleManager(object):
    def __init__(self, schedule):
        if schedule is None:
            # Default schedule is 9-5 weekdays
            schedule = ['9-17']*5 + ['']*2
        def calc_sched(day_idx):
            days = "mon", "tue", "wed", "thu", "fri", "sat", "sun"
            # If day isn't defined, 9-5. If defined and empty, not on that day
            day_sched = schedule.get(days[day_idx], '9-17')
            if not day_sched:
                return None
            day_sched = [int(x) for x in day_sched.split('-')]
            return day_sched

        self.schedule = [calc_sched(d) for d in range(7)]

    def get_next_sleep(self):
        if self.schedule == [None]*7:
            return 0
        sleep_day = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
        sched = None
        while True:
            sched = self.schedule[sleep_day.weekday()]
            if sched is None:
                sleep_day -= timedelta(1)
            else:
                break
        return sleep_day.replace(hour=sched[1])

    def get_next_wake(self):
        if self.schedule == [None]*7:
            return 0
        wake_day = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0) + timedelta(1)
        sched = None
        while True:
            sched = self.schedule[wake_day.weekday()]
            if sched is None:
                wake_day += timedelta(1)
            else:
                break
        return wake_day.replace(hour=sched[0])

class Collector(object):
    # Synchronize once every minute
    TIME_SYNC_INTERVAL = 60
    NO_DATA_TIMEOUT = 35

    # Observed data expected periodicity
    OBS_TIME_PERIOD = timedelta(seconds=10)
    OBS_COLLECTION_MAX_LAG = timedelta(seconds=4)

    def __init__(self, sensei_config, options):
        self.sensei_config = sensei_config
        self.options = options
        self.aci = None
        self.restart_serial()
        self.classroom_id = sensei_config["classroom_id"]
        self.sensor_updates = []
        self.radio_obs = []
        self.accelerometer_obs = []
        self.redis = Redis()
        self.last_published_period = None
        self.mcm = MeshControlManager(self.set_value)
        self.schm = ScheduleManager(sensei_config.get("schedule"))

    def handle_heartbeat(self, hb):
        if hb.epoch_seconds != hb.received_at:
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
                    self.mcm.handle_aci_eventupdate(update.sensor_id, update.mesh_control_version)
                    self.sensor_updates.append(update)
            elif isinstance(evt, AciEvent.AciCmdRsp) and evt.CommandOpCode == AciCommand.AciValueGet.OpCode:
                update = SensorValues(evt.Data[0], evt.Data[2:])
                if update.is_valid:
                    self.mcm.handle_aci_eventupdate(update.sensor_id, update.mesh_control_version)
            elif isinstance(evt, AciEvent.AciEventAppEvt) and evt.is_heartbeat():
                self.handle_heartbeat(evt.heartbeat_msg())

    def get_events(self, timeout=5):
        events = self.aci.get_events(timeout)
        if len(events) > 0:
            self.last_event = time.time()
            self.handle_events(events)

    def run_app_command(self, command):
        data = command.serialize()
        events = self.aci.write_aci_cmd(AciCommand.AciAppCommand(data=data,length=len(data)+1))
        self.handle_events(events)
        return events

    def set_value(self, handle, data):
        events = self.aci.write_aci_cmd(AciCommand.AciValueSet(
            handle=handle, data=data, length=len(data)+3))
        self.handle_events(events)

    def sync_time(self):
        print("Syncing time")
        self.last_time_sync = time.time()
        self.run_app_command(sensei_cmd.SetTime())

    def radio_obs_from_update(self, update):
        if not update.is_valid:
            return []
        obs = []
        for remote_id, rssi in zip(update.proximity_ids, update.proximity_rssi):
            ob_time = datetime.utcfromtimestamp(update.valid_time)
            if remote_id > 0 and rssi > 0:
                obs.append(RadioObservation(self.classroom_id, update.sensor_id, remote_id, ob_time, -rssi))
        return obs

    def accelerometer_measurement_from_update(self, update):
        if update.is_valid and (update.accel_x or update.accel_y or update.accel_z):
            ob_time = datetime.utcfromtimestamp(update.valid_time)
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


    def publish_radio_observations(self, current_obs_period):
        obs_to_publish = [ob for ob in self.radio_obs if ob.observed_at == current_obs_period]
        if len(obs_to_publish) != len(self.radio_obs):
            print("Warning, received out of sync observations for %s: " % current_obs_period)
            for ob in self.radio_obs:
                if ob.observed_at != current_obs_period:
                    print(ob.json_rep())
        if len(obs_to_publish) > 0 and not self.options.dry_run:
            json_obs = [ob.json_rep() for ob in obs_to_publish]
            self.redis.lpush('radio_obs', json.dumps(json_obs))
        self.radio_obs = []

    def publish_accelerometer_observations(self, current_obs_period):
        obs_to_publish = [ob for ob in self.accelerometer_obs if ob.observed_at == current_obs_period]
        if len(obs_to_publish) != len(self.accelerometer_obs):
            print("Warning, received out of sync observations for %s: " % current_obs_period)
            for ob in self.accelerometer_obs:
                if ob.observed_at != current_obs_period:
                    print(ob.json_rep())
        if len(obs_to_publish) > 0 and not self.options.dry_run:
            json_obs = [ob.json_rep() for ob in obs_to_publish]
            self.redis.lpush('accelerometer_obs', json.dumps(json_obs))
        self.accelerometer_obs = []

    def publish_accelerometer_event(self, event):
        if not self.options.dry_run:
            self.redis.lpush('accelerometer_events', json.dumps(event.json_rep()))

    def process_sensor_updates(self):
        for update in self.sensor_updates:
            self.radio_obs.extend(self.radio_obs_from_update(update))
            ob = self.accelerometer_measurement_from_update(update)
            if ob:
                self.accelerometer_obs.append(ob)
            if update.status & SensorValues.STATUS_JOSTLE_FLAG:
                print("Jostle from %d" %(update.sensor_id))
                event_time = datetime.utcfromtimestamp(update.valid_time)
                evt = AccelerometerEvent(self.classroom_id, update.sensor_id, event_time, 'jostle')
                self.publish_accelerometer_event(evt)
        self.sensor_updates = []

    def run(self):
        # Wait for serial connection to be ready
        time.sleep(3)

        # Sync time
        self.sync_time()

        while True:
            # Get any pending events
            self.get_events(timeout=0.5)
            # Some events are sensor updates; process them
            if len(self.sensor_updates) > 0:
                self.process_sensor_updates()

            # Batch any obs up into a frame and publish them locally
            now = datetime.utcnow()
            current_collection_period = now // self.OBS_TIME_PERIOD
            if (current_collection_period != self.last_published_period and
               now - current_collection_period >= self.OBS_COLLECTION_MAX_LAG):
                print("Batching up obs for %s at %s" % (current_collection_period, now))
                self.publish_radio_observations(current_collection_period)
                self.publish_accelerometer_observations(current_collection_period)
                self.last_published_period = current_collection_period
                self.mcm.handle_new_collection_period()
                self.aci.WriteData(AciCommand.AciValueGet(
                    handle=256+MOTHERNODE_ID).serialize())

            if time.time() - self.last_time_sync > self.TIME_SYNC_INTERVAL:
                self.sync_time()
                # set_mesh_control only updates if values are different than
                # previously set, so it's okay to re-set them frequently if
                # they don't change.
                self.mcm.set_mesh_control(sleep_time=self.schm.get_next_sleep(),
                                          wake_time=self.schm.get_next_wake())

            if time.time() - self.last_event > Collector.NO_DATA_TIMEOUT:
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
                collector = Collector(sensei_config, options)
                collector.run()
            except yaml.YAMLError as exc:
                print(exc)
    else:
        print(str.format("Please configure settings in %s" %(config_path)))
        exit(-1)
