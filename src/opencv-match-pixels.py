#!/usr/bin/python

import requests
import json
import cv2
import time
import logging
import random

def get_image():
 retval, im = camera.read()
 return im

def get_warm_up_image():
 # Warmup
 for i in range(5):
  temp = get_image()
  # Save result
  #file = "/notebooks/test-capture-hot-point/image%d.png" % (i)
  #cv2.imwrite(file, temp)
 return get_image()

def rgb2hex(r, g, b):
    return '0x{:02x}{:02x}{:02x}'.format(r, g, b)

# logging
logger = logging.getLogger('esp8266')
stderr_log_handler = logging.StreamHandler()
logger.addHandler(stderr_log_handler)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
stderr_log_handler.setFormatter(formatter)
logger.setLevel('INFO')

# Open Camera
camera = cv2.VideoCapture(0)
# Set definition
camera.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 1024)
time.sleep(2)
# camera.set(15, -8.0)

# Image to update
base_image = get_warm_up_image()

r = requests.get('http://192.168.1.50/status')
r = requests.post('http://192.168.1.50/settings?animation-mode=paint')

for i in range(0,60):
 # time.sleep(3)
 print("Setting pixel... %d" % (i))
 r = requests.post('http://192.168.1.50/pixels/reset', data = {})
 pixels = [{ 'index':i, 'color': rgb2hex(255, 255, 255) }]
 payload = { 'pixels': pixels }
 # logger.info("Pixels %s" % (json.dumps(payload)))
 r = requests.post('http://192.168.1.50/pixels/set', data = json.dumps(payload) )

 print("Capturing image... %d" % (i))
 capture = get_warm_up_image()
 
 # Convert and process
 gray = cv2.cvtColor(capture, cv2.COLOR_BGR2GRAY)
 gray = cv2.GaussianBlur(gray, (19,19),0)

 # Find spot
 (minVal, maxVal, minLoc, maxLoc) = cv2.minMaxLoc(gray)

 # Materialize spot
 cv2.circle(base_image,(maxLoc),10,(0,255,0),-1)
 font = cv2.FONT_HERSHEY_SIMPLEX
 cv2.putText(base_image, "%d" % (i), (maxLoc), font, 0.5, (255,0,0), 1, cv2.LINE_AA)

#  file = "/notebooks/image_%d.png" % (i)
#  cv2.imwrite(file, capture)

# Save result
file = "/notebooks/image.png"
cv2.imwrite(file, base_image)

# Cleanup
del(camera)

