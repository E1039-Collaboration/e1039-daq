#!/bin/bash
####################################
# Continuously check that certain programs are running
# -
# Runs an infinite loop with periodic checks.
# Check that each process is running.
# If process is not running, 
#   start a screen session to run it
# -
# Scroll down to MAIN for the important stuff
# ----------------------------------
# This should run as e906sc2 on e906sc2
# For extra protection, run a cron to make sure this program is alive
# ----------------------------------
# Brian Tice - tice@anl.gov
# April 16, 2014
# Zongwei Zhang - zhangzw@hust.edu.cn
# Aug 16, 2019
####################################


#######################
###########################
# Helper functions
###########################
#######################

###################################
#Grep for a process, return the number of instances running
# Args:
#    $1 = process to grep for [required]
#    $2 = user who runs the process [default=root]
# Return:
#    nonnegative integer = number of instances of that user running that process
#    -1 = argument problem
IsRunning() {
  if [ -z "$1" ]; then
    echo "ERROR: Must pass program name as argument to IsRunning().  Return -1."
    return -1
  fi

  #get args
  prog="$1"
  user=${2:-root}

  #how many processes contain the phrase in prog
  #  exclude instances in grep, editor and screen
  nproc=`ps -U $user -u $user ux | grep -v grep | grep -v emacs | grep -v vim | grep -v nano | grep -v SCREEN | grep -c $prog`
  return $nproc
}

###################################
#Start a detached screen session
# Args:
#    $1 = screen session name
#    all others = program and arguments
# Returns:
#    0 = all OK
#    1 = error with arguments
StartScreen() {
  if [ -z "$1" ]; then
    echo "ERROR: Must pass session name as argument to StartScreen.  Return 1."
    return 1
  fi
  session="$1"
  shift #throw away first argument so $@ is now program to run with params
  echo "STARTING SCREEN"
  echo screen -dmS $session "$@"
  screen -dmS $session "$@"
  return 0
}

###################################
#Check to see if a process is running
# if it isn't, then start a screen session to run it
# Args:
#   $1 = process name to grep for
#   $2 = user who is running the process
#   $3 = name for screen session
#   all subsequent = program to run with arguments
# Return:
#   0 = all OK
#   1 = arguments problem
#   2 = program not running
StartIfStopped(){
  if [ "$#" -lt "4" ]; then
    echo StartIfStopped required at least 4 arguments.  See in-code documentation.
    return 1
  fi
  searchfor="$1"
  user="$2"
  session="$3"
  shift 3    #now $@ is the program to run with args

  IsRunning $searchfor $user
  nproc=$?
  echo Number of $searchfor processes for $user: $nproc

  if [ "0" -eq "$nproc" ]; then
    echo Starting session $session to run command \'$@\'.
    StartScreen $session "$@"
    IsRunning $searchfor $user
    nproc=$?
    if [ "1" -ne "$nproc" ]; then
      rval=3
      echo "ERROR: Failed to start session"
    fi
  elif [ "1" -eq "$nproc" ]; then
    echo Process is already running for command \'$@\'.
  elif [ "1" -lt "$nproc" ]; then
    echo WARNING: Is $nproc too many instances for command \'$@\'.
    rval=2
  else
    echo ERROR: nproc=$nproc. Could not check $searchfor to find processes for command \'$@\'.
    rval=1
  fi
  return $rval
}

#######################
#######################
# MAIN
#######################
#######################

###
#control variables
###
session_prefix="e906loop" #prefix for screen session names

user="e906daq"     #for now assume all processes run under the same user

check_period=10 #number of seconds between running the checks

#Programs to keep runing
check_run_coda_forever=true

#environment
# source .tcshrc and convert to bash
# we need the environment setting to do this
# - snipped from stackoverflow "how to source a cash script in bash to set environment"
while read cmd var val; do
  if [[ $cmd == "setenv" ]]; then
    eval "export $var=$val"
  fi
done < ~/.tcshrc
while read cmd var val; do
  if [[ $cmd == "setenv" ]]; then
    eval "export $var=$val"
  fi
done < ~/dosetupcoda261
#source /data2/e906daq/seaquest-daq/SlowControl/scripts/setup_slowcontrols.sh
#source /data2/e1039/daq/seaquest-daq/SlowControl/scripts/setup_slowcontrols.sh
source /data2/e1039/daq/slowcontrols/scripts/setup_slowcontrols.sh

email_recipients="tice.physics@gmail.com"
email_frequency=15*6 #number of checks between sending email for problems (e.g nMinutes*(60/$check_period) )

nchecks=0  #number of iterations


allOK=true #is everything OK?
nchecks_notOK=0 #number of checks that have been bad

#loop forever
while [ 1 ]; do
  let nchecks=nchecks+1
  echo ====================================
  echo Running check number $nchecks at `date`

  #Reset the error checking variables
  lastOK=$allOK
  allOK=true
  email_message=""

  echo -------------
  program="run_coda_forever.py"
  session="${session_prefix}.${program}"
  if [ "$check_run_coda_forever" = true ]; then
    echo Checking $program...
#    StartIfStopped $program $user $session python /home/e906daq/seaquest-daq/E906ScalerDAQ/scripts/$program
    StartIfStopped $program $user $session python /data2/e1039/daq/seaquest-daq/E906ScalerDAQ/scripts/$program
    rval=$?
    if [ "$rval" -ne "0" ]; then
      allOK=false
      email_message+="Error with program $program.  Return value of StartIfStopped was $rval.\n"
    fi
  else
    echo Do not check $program.
  fi

  #if there was a problem notify DAQ experts
  if [ "$allOK" = false ]; then
    let nchecks_notOK=nchecks_notOK+1
    emailNow=$(( $nchecks_notOK  % $email_frequency ))
    if [ "$emailNow" -eq "1" ]; then
      email_message+="This is check number $nchecks_notOK that has had a problem.\n"
      #echo -e "$email_message" | mail -s "ScalerDAQ Master Loop Problem" "$email_recipients"
    fi
  else
    nchecks_notOK=0
  fi

  sleep $check_period
done
