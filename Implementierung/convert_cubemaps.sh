#!/bin/bash

for e in ennis doge2 glacier grace-new pisa teststripes uffizi-large ; do 
 ./lightstage 0  data/camera/lenovo_parameters.yml data/marker/testcube.dat 50 data/display/e330/parameters.yml 1366 768  data/envmap/$e.exr  1 0; 
 mv tmp/envmap.exr data/envmap/${e}_cube1k.exr
done
