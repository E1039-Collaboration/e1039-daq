#!/bin/bash
#

export ORIG_DIR="/localdata/codadata/"   #where are original files
export RAID_DIR="/data2/data/mainDAQ/"   #where is the raid backup

/home/e906daq/seaquest-daq/ControlRoomScripts/rsync.sh $ORIG_DIR $RAID_DIR

#END
