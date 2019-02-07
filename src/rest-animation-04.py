#!/usr/bin/python

##
## Fade-Out Random Color
##

import requests
import json
import time
import logging
import random

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

def getColor():
    return (random.randint(0,256), random.randint(0,256), random.randint(0,256))

def fadeToBlackByColor(color, byInt):
    return (max(0, color[0] - byInt), max(0, color[1] - byInt), max(0, color[2] - byInt))

for y in range(0,10):
    
    # choose color
    color = getColor()
    logger.info("Round %d = color = (%d,%d,%d)" % (y, color[0], color[1], color[2]))
    
    while (color[0] > 0 or color[1] > 0 or color[2] > 0):
        # set color
        colorStr = rgb2hex(color[0], color[1], color[2])
        logger.info("Color %d %d %d = %s" % (color[0], color[1], color[2], colorStr))
        r = requests.post("http://192.168.1.50/pixels/reset?color=%s" % (colorStr), data = {} )
        color = fadeToBlackByColor(color, 1)
        # time.sleep(0.05)
