#!/bin/tcsh 
#

set DIR = "/data2/e906daq/coda/run_descriptor/"
set ExeDIR = "/home/e1039daq/seaquest-daq/ControlRoomScripts"

set file = `ls -ltr $DIR | tail -1 | awk '{print $9}' `
set run = `echo $file | awk -F "." '{print $1}' | awk -F "_" '{print $3}'`
set crntRun = `cat currentRun`

#if ($run > $crntRun) then
if ( 1 ) then
   # There is new run started!
   echo "New run started. run = "$run"; crntRun = "$crntRun
   set users = `grep username $DIR$file | head -1 | cut -d' ' -f3-`
   echo $users
   set keywords = `grep keyword $DIR$file | head -1 | cut -d' ' -f3-`
   echo $keywords
   set comments = `grep comment $DIR$file | head -1 | cut -d' ' -f3-`
   echo $comments
   echo $run > currentRun
   #./ecl_post.py -f $file -r $run > $ExeDIR/autoLog.txt
endif

#
# END
