#!/bin/bash

#end EPICS

#export DISPLAY=localhost:0.0

#tell the rpi to look into this directory for fonts
export SeaDeskfp=~/SeaQuestDesk
xset +fp $SeaDeskfp
xset fp rehash

FONT="-misc-liberation sans-bold-r-normal--0-0-0-0-p-0-iso8859-1"
echo $FONT
medm -displayFont "$FONT" -dg 760x480+0+0   -x run2_scaler1.adl >& ~/SeaQuestDesk/log/scaler1.log &
medm -displayFont "$FONT" -dg 760x480+0+600 -x run2_scaler2.adl >& ~/SeaQuestDesk/log/scaler2.log &

