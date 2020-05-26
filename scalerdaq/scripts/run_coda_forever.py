#/usr/bin/env python
####################################
## This code is designed to keep the scaler DAQ running continuously
## ad infinitum.  
## Written by Kaz - 3/25/2013
## Updated by Brian Tice - 7/14/2014 - tice@anl.gov
####################################
import os, subprocess, sys
from time import sleep
from datetime import datetime
from math import floor

#set to true to print lots of debugging stuff
verbose = True#False

sys.stdout = open('/home/e906daq/templog_scaler.txt', 'a')

#todo This class is general and could be used elsewhere
class RunControl:
  """A class to help interface with the RunControl"""
  def __init__(self, session="Sea2sc", runtype="Sea2sc" ):
    self.session = session
    self.runtype = runtype


  #enum isn't really supported in python 2 so this class thing is the best you can do
  class Status:
    """Define enum of status states"""
    Booted = 0
    Connected = 1
    Configured = 2
    Downloaded = 3
    Prestarted = 4
    Active = 5
    Ending = 6
    Ended = 7
    Failed = 8
    Unknown = 9


  def GetStatus(self):
    """Use plask to get the status of the run control.  Returns the status using 'emum' type."""
    output = subprocess.Popen(["plask", "-rt", self.runtype, "-spState"], stdout=subprocess.PIPE).communicate()[0].strip().lower()
    if verbose:
      print "Output of checking state = ",output
    if "booted" == output:
      return RunControl.Status.Booted
    elif "connected" == output:
      return RunControl.Status.Connected
    elif "configured" == output:
      return RunControl.Status.Configured
    elif "downloaded" == output:
      return RunControl.Status.Downloaded
    elif "prestarted" == output:
      return RunControl.Status.Prestarted
    elif "active" == output:
      return RunControl.Status.Active
    elif "ending" == output:
      return RunControl.Status.Ending
    elif "ended" == output:
      return RunControl.Status.Ended
    elif "failed" == output:
      return RunControl.Status.Failed
    else:
      return RunControl.Status.Unknown

  def Configure(self):
    """Configure rcgiu.  Return True if things worked."""
    output = subprocess.Popen(["plask", "-s", self.session, "-rt", self.runtype, "-configure"], stdout=subprocess.PIPE).communicate()[0].strip()
    if verbose:
      print "Output from configure command: ",output
    if "transition failed" in output.lower():
      return False
    else:
      return True

  def Download(self):
    """Download rcgiu.  Return True if things worked."""
    output = subprocess.Popen(["plcmd", "-rt", self.runtype, "-download"], stdout=subprocess.PIPE).communicate()[0].strip()
    if verbose:
      print "Output from download command: ",output
    #todo: how to check for error
    return True

  def Prestart(self):
    """Prestart rcgiu.  Return True if things worked."""
    output = subprocess.Popen(["plcmd", "-rt", self.runtype, "-prestart"], stdout=subprocess.PIPE).communicate()[0].strip()
    if verbose:
      print "Output from prestart command: ",output
    #todo: how to check for error
    return True

  def Go(self):
    """Start recording data in rcgiu (go command).  Return True if things worked."""
    output = subprocess.Popen(["plcmd", "-rt", self.runtype, "-go"], stdout=subprocess.PIPE).communicate()[0].strip()
    if verbose:
      print "Output from go command: ",output
    #todo: how to check for error
    return True

  def End(self):
    """End in rcgiu.  Return True if things worked."""
    output = subprocess.Popen(["plcmd", "-rt", self.runtype, "-end"], stdout=subprocess.PIPE).communicate()[0].strip()
    if verbose:
      print "Output from end command: ",output
    #todo: how to check for error
    return True


  def Reset(self):
    """Issues a reset command.  Return True if things worked."""
    output = subprocess.Popen(["plcmd", "-rt", self.runtype, "-reset"], stdout=subprocess.PIPE).communicate()[0].strip()
    if verbose:
      print "Output from reset command: ",output
    #todo: what indicates error
    return True

  def GetSecondsSinceRunStart(self):
    """How many seconds have passed since this run was started?"""
    output  = subprocess.Popen( ["plask", "-rt", self.runtype, "-spRunStartTime"], stdout=subprocess.PIPE).communicate()[0].strip()
    if verbose:
      print "Run start time is:",output
    FMT = '%H:%M:%S'
    tstart = datetime.strptime(output, FMT)

    #put timenow into the same formatting, this removes the date comonent and assumes both datetimes happen on the same day
    tdnow = datetime.now()
    tnowstr = "%02d:%02d:%02d" % ( tdnow.hour, tdnow.minute, tdnow.second )
    tnow = datetime.strptime( tnowstr, FMT )

    diff = tnow - tstart
    return diff.seconds


def WaitForEOS( bufferSeconds = 12 ):
  """Continually check for EOS.  When you get it wait for some buffer time and return True."""
  maxSeconds = 5 * 60 #give up time in seconds
  print "start wating for EOS..."
  tdstart = datetime.now()
  while True:
    output = subprocess.Popen(["caget", "-t", "EOS"], stdout=subprocess.PIPE).communicate()[0].strip()
    if "1" in output:
      sleep( bufferSeconds )
      return True
    #if more than 5 minutes have gone by, assume there is no EOS coming in.
    #give up on looking for EOS.
    tdiff = datetime.now() - tdstart
    if tdiff.seconds > maxSeconds:
      print "Have not seen EOS after waiting %d seconds.  Give up." % tdiff.seconds
      break
  return False


##################################
# Main script starts here
##################################

#create an interface to RunControl for Sea2sc (scaler DAQ)
rcgui = RunControl()

#goal is to take 60 minute runs
nMinutesPerRun =5

#number of seconds it takes to run the plask command
#account for these when deciding when to start a new run
secondsPerPlask = 0.75

#how often should we print progress during the active phase?
#once every 60 seconds, convert to number of plask checks
printFreq = int( floor( 60 / ( 1 + secondsPerPlask ) ) )

#how many times have we seen the active state in a row
nActiveChecks = 0

#how many times in a row have we seen unknown status?
nUnknown = 0

#how many times have we seen failure in a row
nFailed = 0

#run forever
while True:
  rcstat = rcgui.GetStatus()
  sys.stdout.flush()

  if rcstat == RunControl.Status.Unknown:
    nUnknown += 1
    print "Warning - Got an unknown run control status.  Probably the run control is in transition."

    #if we get too many unknowns, then there is probably some problem and we should quit
    if nUnknown > 15:
      print "ERROR - Got unknown run control status %d times in a row, quitting." % nUnknown
      sys.exit(1)

    sleep( 1 )
    continue
  else:
    nUnknown = 0

  #Look for failed state
  if rcstat == RunControl.Status.Failed:
    nFailed += 1
    print "Run control is in a failed state for %d check(s).  Reset run control." % nFailed
    rcgui.Reset()
    continue
  else:
    nFailed = 0
    

  #Take action appropriate for the current status
  if rcstat == RunControl.Status.Booted or rcstat == RunControl.Status.Connected:
    print "In booted state.   Configuring..."
    isOK =  rcgui.Configure()
    if not isOK:
      print "Problem configuring.  Reset the run control."
      rcgui.Reset()
  elif rcstat == RunControl.Status.Failed:
    nFailed += 1
    print "In failed state.  Sleep for 3s then configure..."
    sleep(3)
    isOK = rcgui.Configure()
    if not isOK:
      print "Problem configuring.  Reset the run control."
      rcgui.Reset()
    else:
      nFailed = 0
  elif rcstat == RunControl.Status.Configured:
    print "In configured state.  Downloading..."
    isOK = rcgui.Download()

  elif rcstat == RunControl.Status.Downloaded:
    print "In downloaded state.  Prestarting..."
    #sleep(3)
    isOK = rcgui.Prestart()

  elif rcstat == RunControl.Status.Ended:
    print "In ended state.  Prestarting..."
    sleep(3)
    isOK = rcgui.Prestart()

  elif rcstat == RunControl.Status.Ending:
    print "In ending.  Waiting..."
    sleep(2)
    #perhaps count number of times we see this state and complain if ending takes too long

  elif rcstat == RunControl.Status.Prestarted:
    print "In prestarting state.  Time to start taking data.  Go..."
    #sleep(3)
    isOK = rcgui.Go()
    #reset the active minutes counter
    nActiveChecks = 0

  elif rcstat == RunControl.Status.Active:
    nSecondsActive = rcgui.GetSecondsSinceRunStart()
    nMinutesActive = int( floor(nSecondsActive / 60. ) )
    #round to nearest 
    if nActiveChecks % printFreq == 0:
      print "In active state for %d minutes." % nMinutesActive
    if nMinutesActive >= nMinutesPerRun:
      #if the run is as long as desired, wait for EOS and end the run
      print "    This is time to start a new run."
      #waits for EOS + some buffer time
      WaitForEOS()
      print "    Got EOS.  End the run."
      isOK = rcgui.End()
      nActiveChecks = 0 #just in case something goes wrong in transition
    else:
      #if the run isn't long enough, increment and wait another minute
      nActiveChecks += 1
      sleep( 1 )
