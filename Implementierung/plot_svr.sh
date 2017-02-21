#!/bin/bash

# plot response curve from yml
 
response=$1
if [ -z "$response" ]; then echo "Please specify the svr responsecurve (yml format) with the first argument"; exit 1; fi

#curve index we want to plot
if [ -z "$2" ]; then echo "Please specify the index of the curve to plot with the second argument"; exit 1; fi

for index in $*; do
 if [ $index = "$1" ] ; then continue; fi

  for channel in 1 2 3; do
    # clean yml file and extract every third row (=one channel)
    grep -A3071 response_$index:  $response  | sed -s 's/,/\n/g' | tr -d "[] response_" | sed '/^$/d' |  sed -s "s/$index://g" |  sed -s 's/-0/E-0/g' | sed -n "$channel~3p"  > tmp/tmp_plot_svr_${channel}_$index.dat
  
   plotargs="$plotargs './tmp/tmp_plot_svr_${channel}_$index.dat' u 1 w l ls 1,"
  done

done
  plotargs=${plotargs%,}
  echo "plot $plotargs;" | gnuplot -p

