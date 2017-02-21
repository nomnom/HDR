#!/bin/bash

xdotool mousemove 10000 10000

thresh=120
cam=1
 ./artoolkit_test data/camera/logitech_hd_parameters.yml --multi data/marker/stage.dat $thresh false true $cam
