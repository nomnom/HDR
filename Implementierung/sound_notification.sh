#!/bin/bash

# simple audio notification  script
log=log/sound_notification.log

file=""

# indices from C code enum SOUND in src/lighstage/util.h
case $1 in
 
  0) file=buzzer.wav ;;   # ERROR
  1) file=poke.wav ;; # WARNING
  2) file=beep_low.wav ;; # START
  3) file=sonar.wav ;;  # SEARCH
  4) file=shutter.wav ;; # CAPTURE_START
  5) file=notification.wav ;;   # CAPTURE_END
  6) file=success.wav ;; # FINISH
  7) file=closer.wav ;; # Come closer
  8) file=away.wav ;; # move away
  9) file=angle.wav ;; # wrong angle
  10) file=beep_high.wav ;; # process start
  11) file=beep_low.wav ;; # process complete
  12) file=position.wav ;; # bad position
  13) file=overlap.wav;; # overlap 
  14) file=more.wav;; # more overlap required
  15) file=less.wav;; # less overlap required
  16) file=one.wav;; 
  17) file=two.wav;;
  18) file=three.wav;;
  19) file=four.wav;;
  20) file=five.wav;;
  21) file=six.wav;; # six degrees
  22) file=seven.wav;; # seven degrees
  23) file=eight.wav;; # eight degrees
  24) file=nine.wav;; # nine degrees
  25) file=ten.wav;; # ten degrees
  26) file=left.wav;; 
  27) file=right.wav;;
  28) file=top.wav;;

  *) echo "unknown sound id: $1" >> $log; exit -1
esac
echo `date` play data/sounds/$file >> $log

if [ `uname` = "Darwin" ] ; then
  afplay data/sounds/$file 2> /dev/null &
else
  aplay data/sounds/$file 2> /dev/null &
fi

exit 0
