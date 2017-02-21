#!/bin/bash

response=data/camera/response_canon.m

# separate into three channel
tail -n+7 $response > tmp/tmp_response.dat
head -n65536 tmp/tmp_response.dat > tmp/tmp_response0.dat
head -n131079 tmp/tmp_response.dat | tail -n65536 > tmp/tmp_response1.dat
head -n196622 tmp/tmp_response.dat | tail -n65536 > tmp/tmp_response2.dat



plotswitch=""
if [ "$1" = "noplot" ] ; then
  plotswitch='#'
fi 

# fit polynomial using gnuplot
for channel in 0 1 2
do
 result[$channel]=`echo "f(x) = a + b*x + c*x**2 + d*x**3 + e*x**4 + f*x**5 + g*x**6 + h*x**7; fit f(x) 'tmp/tmp_response${channel}.dat' u 3:2 via a,b,c,d,e,f,g,h; $plotswitch plot f(x),'tmp/tmp_response${channel}.dat' u 3:2 w l" | gnuplot -p  2>&1 | grep -A10 resultant | tail -n 10`
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
 echo -n `head -n1  tmp/tmp_response0.dat | cut -d' ' -f1`
 echo -n ", "
 echo -n `head -n1  tmp/tmp_response1.dat | cut -d' ' -f1`
 echo -n ", "
 echo -n `head -n1  tmp/tmp_response2.dat | cut -d' ' -f1`
echo ']' 

echo -n 'max: [ '
 echo -n `tail -n1  tmp/tmp_response0.dat | cut -d' ' -f1`
 echo -n ", "
 echo -n `tail -n1  tmp/tmp_response1.dat | cut -d' ' -f1`
 echo -n ", "
 echo -n `tail -n1  tmp/tmp_response2.dat | cut -d' ' -f1`
echo ']' 


