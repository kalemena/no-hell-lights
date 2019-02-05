#!/usr/bin/python

##
## Sinelon Effect
##
import requests
import json
import time

r = requests.get('http://192.168.1.50/status')
# r.json()

#r = requests.post('http://192.168.1.50/settings?animation-mode=autoplay')
r = requests.post('http://192.168.1.50/settings?animation-mode=paint')
# r.json()

r = requests.post('http://192.168.1.50/pixels/reset', data = {})
# r.json()
 
# payload = "{ \"pixels\": [ { \"index\":1, \"color\": 0x123456 }, { \"index\": 2, \"color\": 0x987654 } ] }"
# r = requests.post('http://192.168.1.50/pixels/set', data = payload )

for y in range(0,10):
 for i in range(0,60):
  # time.sleep(0.10)
  payload = { 'pixels': [ { 'index':i, 'color': 0x000000 }, { 'index': i+1, 'color': 0x987654 } ] }
  r = requests.post('http://192.168.1.50/pixels/set', data = json.dumps(payload) )

 for i in range(0,60):
  # time.sleep(0.10)
  payload = { 'pixels': [ { 'index':60-i, 'color': 0x000000 }, { 'index': 60-i-1, 'color': 0x987654 } ] }
  r = requests.post('http://192.168.1.50/pixels/set', data = json.dumps(payload) )


