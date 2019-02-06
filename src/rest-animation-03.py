#!/usr/bin/python

##
## Fade-In Fade-Out RGB
##

import requests
import json
import time
import logging

def rgb2hex(r, g, b):
    return '0x{:02x}{:02x}{:02x}'.format(r, g, b)

# logging
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

# payload = { 'pixels': [] }
# logger.info(json.dumps(payload))
# time.sleep(0.02)

ratio = 3

def getColor(channel): 
    if channel == 1: 
        return rgb2hex(i*ratio,0,0)
    elif channel == 2: 
        return rgb2hex(0, i*ratio, 0)
    else: 
        return rgb2hex(0, 0, i*ratio)    


for j in range(1,4):    
    for i in range(0, int(256/ratio)):
        colorStr = getColor(j)
        # logger.info("Color: %s" % (colorStr))
        r = requests.post("http://192.168.1.50/pixels/reset?color=%s" % (colorStr), data = {} )

    for i in reversed(range(0,int(256/ratio))):
        colorStr = getColor(j)
        # logger.info("Color: %s" % (colorStr))
        r = requests.post("http://192.168.1.50/pixels/reset?color=%s" % (colorStr), data = {} )
