#!/usr/bin/python3 -u

import json
import redis
from datetime import datetime

class Monitor(object):
    def __init__(self):
        self.redis = redis.Redis()
        self.pubsub = self.redis.pubsub()
        self.pubsub.subscribe('sensor_status')
        subscribe_results = self.pubsub.get_message()

    def run(self):

        while True:
            message = self.pubsub.get_message()
            if message:
                status = json.loads(message['data'].decode("utf-8"))
                now = datetime.now()
                line = "%s: %d total" % (now, len(status))
                for sensor_id, age in status.items():
                    if age > 1:
                        line += " %s:%d" % (sensor_id, age)
                print(line)


if __name__ == '__main__':
    monitor = Monitor()
    monitor.run()
