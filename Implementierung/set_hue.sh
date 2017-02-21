#!/bin/sh

# sets the Hue lamp brightness via curl 

hostname=honkytonk
user=newdeveloper

#mired color temperature: 153 (6500K) - 500 (2000K)
ct=153

#brightness (0-255)
brightness=$1
if [ -z "$brightness" ] ; then
 brightness=10
fi

stat=0

for lamp_id in 7 8 ; do
 ret=`curl -s --request PUT --data "{\"on\":true, \"ct\":$ct,\"bri\":$brightness}" http://$hostname/api/$user/lights/$lamp_id/state`
 if [[ ! $ret =~ success ]] ; then
   stat=-1
 fi

done

exit $stat
