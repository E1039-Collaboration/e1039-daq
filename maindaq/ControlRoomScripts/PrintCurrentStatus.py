#!/usr/bin/env python
#####################
# Gather info about current running status
# and print to screen.
# Useful to check things that are not in db.
##
# Brian Tice - tice@anl.gov - Feb 16, 2014
####################
import datetime

def GetCurrentFirmware():
  """Get the current firmware by sniffing the copy line of the CurrentFirmware file"""
  current_fname = "/seaquest/trigger/firmware/current"
  l1 = None
  l2 = None
  for line in open(current_fname):
    line = line.strip()
    if l1 is None and "L1 roadset" in line:
      l1 = line.split(":")[1]
    elif l2 is None and "L2 version" in line:
      l2 = line.split(":")[1]
    if l1 is not None and l2 is not None:
      break
  if l1 is None:
    l1 = "Not found"
  if l2 is None:
    l2 = "Not found"
  return "L1: %(l1)s | L2: %(l2)s" % locals()


timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
firmware = GetCurrentFirmware()




print "="*48
print "Current Run Status at",timestamp
print "-"*48
print "Firmware:", firmware
print "="*48
