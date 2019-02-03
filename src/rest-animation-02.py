import requests
import json
import time
import logging

logger = logging.getLogger('esp8266')

stderr_log_handler = logging.StreamHandler()
logger.addHandler(stderr_log_handler)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
stderr_log_handler.setFormatter(formatter)

logger.setLevel('INFO')

r = requests.get('http://192.168.1.50/status')
# r.json()

#r = requests.post('http://192.168.1.50/settings?animation-mode=autoplay')
r = requests.post('http://192.168.1.50/settings?animation-mode=paint')
# r.json()

r = requests.post('http://192.168.1.50/pixels/reset', data = {})
# r.json()

payload = { 'pixels': [] }
logger.info(json.dumps(payload))

for i in range(0,60):
 r = requests.post('http://192.168.1.50/pixels/reset?color=0xFFFFFF', data = {} )
 time.sleep(0.02)
 r = requests.post('http://192.168.1.50/pixels/reset?color=0x000000', data = {} )
 time.sleep(0.02)

