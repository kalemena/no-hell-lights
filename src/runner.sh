#!/bin/bash

docker run -it --rm \
    --device /dev/video0 \
    -v $PWD:/notebooks \
    kalemena/opencv:4.0.1 bash /notebooks/opencv-match-pixels.sh
