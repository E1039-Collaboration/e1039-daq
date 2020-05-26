#!/bin/bash

# EPICS
export EPICS_BASE=~pi/epics/base
export EPICS_HOST_ARCH=linux-arm
export EPICS_CA_ADDR_LIST=192.168.24.14
export PATH=~pi/epics/bin/$EPICS_HOST_ARCH:$PATH
#end EPICS

#export DISPLAY=localhost:0.0

#chromium  http://e906-gat3.fnal.gov:8081/SeaQuestAcc >& /dev/null &

medm -x /data2/e906daq/scalerDAQ/run2_scaler1.adl >& ~/SeaQuestDesk/log/scaler1.log &

medm -x /data2/e906daq/scalerDAQ/run2_scaler2.adl >& ~/SeaQuestDesk/log/scaler2.log &

StripTool -xrm 'StripTool.StripGraph.geometry:600x200+700+0'  /home/pi/SeaQuestDesk/dutyFactor.stp >& /home/pi/SeaQuestDesk/log/dutyFactor.log &

StripTool -xrm 'StripTool.StripGraph.geometry:600x200+700+200'  /home/pi/SeaQuestDesk/Magnets.stp >& /home/pi/SeaQuestDesk/log/magnet.log &

StripTool -xrm 'StripTool.StripGraph.geometry:600x200+700+400'  /home/pi/SeaQuestDesk/XHodo.stp >& /home/pi/SeaQuestDesk/log/XHodo.log &

