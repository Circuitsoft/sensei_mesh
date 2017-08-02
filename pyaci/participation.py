#!/usr/bin/env python
import re
from datetime import datetime
import json

# Reads output from upload.py
def main(filename):
    records = []
    for line in open(filename):
        # evt = AciEventUpdate(1) SensorValues: sensor_id:15 proximity_ids:[12, 20, 24, 0, 0] proximity_rssi:[53, 55, 57, 0, 0] battery:194, accel:(37, 0, 144), status:0, valid_time:1501623900
        m = re.search('AciEventUpdate.*SensorValues.*sensor_id:(\d+) .*valid_time:(\d+)$', line)

        if m:
            timestamp = datetime.utcfromtimestamp(int(m.group(2))).isoformat()
            sensor_col = 's' + m.group(1)
            records.append({'ts':timestamp, sensor_col:1})

    print json.dumps(records)

if __name__=='__main__':
    import sys
    filename = sys.argv[1]
    main(filename)
