#!/usr/bin/python

##
## Sinelon
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

# fade the full strip
def fadeToBlackBy(stripLeds, byInt):
        for j in range(0,len(stripLeds)):
                stripLeds[j] = fadeToBlackByColor(stripLeds[j], 40)

def show(stripLeds, indexLast, direction):
        # update the next moving pixels
        pixels = [{}]*10
        for j in range(0,10):
                index = (indexLast-j*direction)%60
                pixels[j] = { 'index':index, 'color': rgb2hex(stripLeds[index][0], stripLeds[index][1], stripLeds[index][2]) }
        payload = { 'pixels': pixels }
        # logger.info("Pixels %s" % (json.dumps(payload)))
        r = requests.post('http://192.168.1.50/pixels/set', data = json.dumps(payload) )

leds = [(0,0,0)] * 60

for y in range(0,10):    
        # choose color
        color = getColor()
        # logger.info("Round %d = color = (%d,%d,%d)" % (y, color[0], color[1], color[2]))

        # moving chase
        for i in range(0,60):
                fadeToBlackBy(leds, 40)
                # assign color to next led
                leds[i] = color
                # logger.info("Index %d - Color %d %d %d" % (i, color[0], color[1], color[2]))
                show(leds, i, 1)
        
        color = getColor()
        for i in reversed(range(0,60)):
                fadeToBlackBy(leds, 40)
                # assign color to next led
                leds[i] = color
                # logger.info("Index %d - Color %d %d %d" % (i, color[0], color[1], color[2]))
                show(leds, i, -1)

# cleanup - fade to black                
for i in range(0,10):
        fadeToBlackBy(leds, 40)
        show(leds, i, 1)