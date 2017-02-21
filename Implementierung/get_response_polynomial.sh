#!/bin/bash

# calculate response curve points from recorded screen images
./evaluate_display --response experiments/lenovo/final_2/5_ramp_h2_undist.exr data/display/e330/r_white.exr data/display/e330/r_black.exr 0.5 15 2 2 tmp/tmp_response.dat >/dev/null 2>/dev/null
if [ ! $? -eq 0 ] ; then 
  echo "Error running evaluate_display"
  exit -1
fi

plotswitch=""
if [ "$1" = "noplot" ] ; then
  plotswitch='#'
fi 

# fit 5th degree polynomial using gnuplot
for channel in 0 1 2
do
 # r(x)
 col=`echo $channel + 2 | bc`
# result[$channel]=`echo "f(x) = a + b*x + c*x**2 + d*x**3 + e*x**4 + f*x**5; fit f(x) 'tmp/tmp_response.dat' u 1:$col via a,b,c,d,e,f; plot f(x),'tmp/tmp_response.dat' u 1:$col w l" | gnuplot -p  2>&1 | grep -A10 resultant | tail -n 10`
 
 result[$channel]=`echo "f(x) = a + b*x + c*x**2 + d*x**3 + e*x**4 + f*x**5 + g*x**6 + h*x**7; fit f(x) 'tmp/tmp_response.dat.$channel' u 1:2 via a,b,c,d,e,f,g,h; $plotswitch plot f(x),'tmp/tmp_response.dat.$channel' u 1:2 w l" | gnuplot -p  2>&1 | grep -A10 resultant | tail -n 10`

 ## inverse r^-1(x): doesn't work so well with low order polynomials
 #result=`echo "f(x) = a + b*x + c*x**2 + d*x**3 + e*x**4 + f*x**5; fit f(x) 'tmp/tmp_response.dat.$channel'  u 1:2 via a,b,c,d,e,f; plot f(x), 'tmp/tmp_response.dat.$channel' u 1:2 w l" | gnuplot -p  2>&1 | grep -A10 resultant | tail -n 10`

 # poly 2nd degree
 #result=`echo "f(x) = a + b*x + c*x**2; fit f(x) 'tmp/tmp_response.dat'  u 1:$channel via a,b,c; set xrange[0:255]; set yrange[0:1]; plot f(x)" | gnuplot -p 2>&1 | grep -A10 resultant | tail -n 10`
   
done

echo '%YAML:1.0'
echo -n 'poly7: [ '

# grep parameters
for var in a b c d e f g h
do
  for c in 0 1 2
  do
    value=`echo "${result[$c]}" | grep -e "^${var}.*=.*" | cut -d'=' -f2`
    echo -n $value
    if [ ! "$var" = "h" ] || [ ! "$c" = "2" ] ; then
      echo -n ', '
    fi
  done
done

echo ' ]'

echo -n 'min: [ '
 echo -n `head -n1  tmp/tmp_response.dat.0 | cut -d' ' -f1`
 echo -n ", "
 echo -n `head -n1  tmp/tmp_response.dat.1 | cut -d' ' -f1`
 echo -n ", "
 echo -n `head -n1  tmp/tmp_response.dat.2 | cut -d' ' -f1`
echo ']' 

echo -n 'max: [ '
 echo -n `tail -n1  tmp/tmp_response.dat.0 | cut -d' ' -f1`
 echo -n ", "
 echo -n `tail -n1  tmp/tmp_response.dat.1 | cut -d' ' -f1`
 echo -n ", "
 echo -n `tail -n1  tmp/tmp_response.dat.2 | cut -d' ' -f1`
echo ']' 


