#!/bin/sh

# sets backlight brightness
# first arg is int value 0 .. 100, where 0 is off and 100 is max brightness

# on linux: use acpi (in this case a lenovo e330 laptop)
if [ `uname -s` = "Linux" ] ; then
  # accepts int values 0 .. 4437
  python -c "print int($1 / 100.0 * 4437.0)"  > /sys/class/backlight/intel_backlight/brightness
  exit $?

# on OSX we use the brightness program (located under src/tools/brightness/brightness)
elif [ `uname -s` = "Darwin" ] ; then
 
  # accepts float values 0 .. 1
  ./src/tools/brightness/brightness `python -c "print float($1) / 100.0"`  
  exit $?
 
fi

exit -1
