#!/bin/bash
set -v

# EPICS
export EPICS_BASE=~pi/epics/base
export EPICS_HOST_ARCH=linux-arm
export EPICS_CA_ADDR_LIST=e906sc1.fnal.gov
export PATH=~pi/epics/bin/$EPICS_HOST_ARCH:$PATH
#end EPICS

#export DISPLAY=localhost:0.0

# tell display not to sleep
xset s noblank
xset s off
xset -dpms

chromium  --proxy-server=e906-gat2.fnal.gov:3128 http://e906-gat6.fnal.gov:8081/SeaQuestAcc >& /dev/null &

StripTool -xrm 'StripTool.StripGraph.geometry:800x200+450+050'  /home/pi/SeaQuestDesk/St1.stp >& /home/pi/SeaQuestDesk/log/St1.log &
sleep 3

StripTool -xrm 'StripTool.StripGraph.geometry:800x200+450+250'  /home/pi/SeaQuestDesk/target.stp >& /home/pi/SeaQuestDesk/log/target.log &
sleep 3

StripTool -xrm 'StripTool.StripGraph.geometry:800x200+450+450'  /home/pi/SeaQuestDesk/dutyFactor.stp >& /home/pi/SeaQuestDesk/log/dutyFactor.log &
sleep 3

StripTool -xrm 'StripTool.StripGraph.geometry:800x200+450+650'  /home/pi/SeaQuestDesk/Magnets.stp >& /home/pi/SeaQuestDesk/log/magnet.log &
sleep 3

StripTool -xrm 'StripTool.StripGraph.geometry:800x200+450+850'  /home/pi/SeaQuestDesk/XHodo.stp >& /home/pi/SeaQuestDesk/log/XHodo.log &
sleep 3


xclock -geometry 400x400+0+0 >& /dev/null &

screen -dmS CherenkovInfo root ~/SeaQuestDesk/root/displayQIE.C

