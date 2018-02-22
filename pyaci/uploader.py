#!/usr/bin/env python3 -u

from argparse import ArgumentParser
import sys, traceback
import time
import sensei_cmd
from sensei import *
import datetime
import yaml
from os.path import expanduser
import os.path
import json
import redis

class Uploader(object):

    def __init__(self, sensei_config, options):
        self.sensei_config = sensei_config
        self.options = options
        api_url = sensei_config["server"]["url"] + 'api/v1/'
        print("Uploading to %s" % api_url)
        self.api = Api(api_url, sensei_config["server"]["username"], sensei_config["server"]["password"])
        self.classroom_id = sensei_config["classroom_id"]
        self.redis = redis.Redis()
        self.processing_suffix = "_in_process"

    def upload_radio_observations(self, obs_data):
        if obs_data == None:
            return False
        ob_json_reps = json.loads(obs_data)
        obs = [RadioObservation.from_json_rep(json_rep) for json_rep in ob_json_reps]
        print("Uploading %d radio observations" % len(obs))
        self.api.upload_radio_observations(obs)
        return True

    def upload_accelerometer_observations(self, obs_data):
        if obs_data == None:
            return False
        ob_json_reps = json.loads(obs_data)
        obs = [AccelerometerObservation.from_json_rep(json_rep) for json_rep in ob_json_reps]
        print("Uploading %d accelerometer observations" % len(obs))
        self.api.upload_accelerometer_observations(obs)
        return True

    def upload_accelerometer_event(self, event_data):
        if event_data == None:
            return False
        event_json_rep = json.loads(event_data)
        event = AccelerometerEvent.from_json_rep(event_json_rep)
        print("Uploading accelerometer event")
        self.api.upload_accelerometer_events([event])
        return True

    def dequeue(self, queue_name, handler, check_for_in_process_data):
        processing_queue_name = queue_name + self.processing_suffix
        rval = False
        if check_for_in_process_data:
            rval = handler(self.redis.lindex(processing_queue_name, 0))
        else:
            rval = handler(self.redis.rpoplpush(queue_name, processing_queue_name))
        self.redis.delete(queue_name + self.processing_suffix)
        return rval

    def run(self):
        check_for_in_process_data = True

        queues = {
            'radio_obs': self.upload_radio_observations,
            'accelerometer_obs': self.upload_accelerometer_observations,
            'accelerometer_events': self.upload_accelerometer_event,
        }

        while True:
            try:
                handled_count = 0
                for (queue_name, handler) in queues.items():
                    if self.dequeue(queue_name, handler, check_for_in_process_data):
                        handled_count += 1
                check_for_in_process_data = False
                if handled_count == 0:
                    print("No data available... sleeping")
                    time.sleep(1)
            except KeyboardInterrupt:
                break;
            except:
                check_for_in_process_data = True
                print('-'*60)
                traceback.print_exc(file=sys.stdout)
                print('-'*60)

            if check_for_in_process_data:
                time.sleep(5) # Don't go crazy if we're hitting errors




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
