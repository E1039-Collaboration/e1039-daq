#!/bin/tcsh 
#

set DIR = "/data2/e906daq/coda/run_descriptor/"
set ExeDIR = "/home/e1039daq/seaquest-daq/ControlRoomScripts"

set file = `ls -ltr $DIR | tail -1 | awk '{print $9}' `
set run = `echo $file | awk -F "." '{print $1}' | awk -F "_" '{print $3}'`
set crntRun = `cat currentRun`

if ($run > $crntRun) then
   # There is new run started!
   setenv http_proxy e906-gat6.fnal.gov:3128
   setenv HTTP_PROXY e906-gat6.fnal.gov:3128
   setenv https_proxy e906-gat6.fnal.gov:3128
   setenv HTTPS_PROXY e906-gat6.fnal.gov:3128
   setenv ftp_proxy e906-gat6.fnal.gov:3128

   echo $run > currentRun
   ./ecl_post.py -f $file -r $run > $ExeDIR/autoLog.txt
   #./ecl_post.py -f $file -r $run -s $users -k $keywords -c $comments 

endif

#
# END
