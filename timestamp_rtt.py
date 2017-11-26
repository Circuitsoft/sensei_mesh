#!/usr/bin/env python
import subprocess
import datetime

proc = subprocess.Popen(['JLinkRTTClient'],stdout=subprocess.PIPE)
while True:
  line = proc.stdout.readline()
  timestamp = datetime.datetime.utcnow().strftime('%H:%M:%S.%f')[:-3]
  print(timestamp + " " + line.rstrip())
