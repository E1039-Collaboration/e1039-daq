#!/bin/bash

# EPICS
export EPICS_BASE=~pi/epics/base
export EPICS_HOST_ARCH=linux-arm
export EPICS_CA_ADDR_LIST=e906sc1.fnal.gov
export PATH=~pi/epics/bin/$EPICS_HOST_ARCH:$PATH
#end EPICS

#export DISPLAY=localhost:0.0

chromium-browser  --proxy-server=e906-gat2.fnal.gov:3128 http://e906-gat6.fnal.gov:8081/SeaQuestAcc >& /dev/null &

#tell the rpi to look into this directory for fonts
export SeaDeskfp=~/SeaQuestDesk
xset +fp $SeaDeskfp
xset fp rehash

medm -dg 650x500+30+0   -x run2_scaler1.adl >& ~/SeaQuestDesk/log/scaler1.log &
medm -dg 650x500+30+540 -x run2_scaler2.adl >& ~/SeaQuestDesk/log/scaler2.log &

StripTool -xrm 'StripTool.StripGraph.geometry:600x170+700+100'  /home/pi/SeaQuestDesk/St1.stp >& /home/pi/SeaQuestDesk/log/St1.log &

StripTool -xrm 'StripTool.StripGraph.geometry:600x170+700+270'  /home/pi/SeaQuestDesk/target.stp >& /home/pi/SeaQuestDesk/log/target.log &

StripTool -xrm 'StripTool.StripGraph.geometry:600x170+700+450'  /home/pi/SeaQuestDesk/dutyFactor.stp >& /home/pi/SeaQuestDesk/log/dutyFactor.log &

StripTool -xrm 'StripTool.StripGraph.geometry:600x170+700+640'  /home/pi/SeaQuestDesk/Magnets.stp >& /home/pi/SeaQuestDesk/log/magnet.log &

StripTool -xrm 'StripTool.StripGraph.geometry:600x170+700+830'  /home/pi/SeaQuestDesk/XHodo.stp >& /home/pi/SeaQuestDesk/log/XHodo.log &


xclock -geometry 400x400+710+0 >& /dev/null &

