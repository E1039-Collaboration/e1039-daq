#!/usr/bin/env python
import time
#from subprocess import check_output
import subprocess

VERBOSE = False

def GetHostTime(login):
  """ssh using this login and get the time in seconds since epoch"""
  print "Getting time for",login
  cmd = ["ssh", login, 'date +"%s"']
  ret = subprocess.Popen( cmd, stdout=subprocess.PIPE).communicate()[0].strip()
  print
  return int(ret)


LOGINS = []
#LOGINS.append("e906daq@e906-db1.fnal.gov")#user does not exist
LOGINS.append("e906daq@e906sc1.fnal.gov")
LOGINS.append("e906daq@e906sc2.fnal.gov")
LOGINS.append("e906daq@e906beam1.fnal.gov")
LOGINS.append("e906daq@e906beam2.fnal.gov")
LOGINS.append("e906daq@e906daq1.fnal.gov")

#could add these later if necessary
#they are harder because they use different usernames
#LOGINS.append("pi@e906pi1")
#LOGINS.append("shift@e906-monitor")

#record the start time
start_time = int(time.time())

print
print "Checking logins.  You may have to enter passwords."
print "Don't worry, I'll use that to calibrate the time."
print

#for each host, get the time
checks = []
for login in LOGINS:
  try:
    hosttime = GetHostTime(login)
  except:
    print "Could not get time with login ", login
    continue

  host = login.split("@")[1]
  host = host.split(".")[0]
  localtime = int(time.time())

  #how much time on this machine between start and now?
  offset = localtime - start_time
  #what did this host's clock say at start?
  caltime = hosttime - offset
  #how far off was that clock from this clock?
  jitter = caltime - start_time

  #store it all
  timecheck = { "host":host, "hosttime":hosttime, "localtime":localtime, "caltime":caltime, "jitter": jitter }
  checks.append(timecheck)
  if VERBOSE:
    print timecheck


#now print the jitter of each
print
print "=== RESULTS ==="
for check in checks:
  host   = check["host"]
  jitter = check["jitter"]
  if jitter != 0:
    print "WARNING: Host %(host)s is offset by %(jitter)s from the local machine" % locals()
  else:
    print "Host %(host)s is accurate to within a second" % locals()

