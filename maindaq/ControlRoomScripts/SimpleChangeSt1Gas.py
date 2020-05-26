#!/usr/bin/env python
from datetime import datetime
import pygtk
pygtk.require('2.0')
import gtk

import os, glob, time, re
import DAQUtils

#define sounds
#TO4WAY = "~/sounds/changeto4way.mp3"
#TO3WAY = "~/sounds/changeto3way.mp3"
ALARM_VOICE = "~/sounds/st1gas_v2.mp3"
SLEEP_AFTER_PROMPT_SECONDS = 10

#font sizes for gtk are regular font size point times 1000
HEADER_TEXT_SIZE = 72000 # = 72pt
BODY_TEXT_SIZE   = 14000

def PlaySound( soundfile, inBG = False ):
  soundfile = os.path.expandvars(soundfile) #expand environmentals
  soundfile = os.path.expanduser(soundfile) #expand tilde
  if os.path.isfile(soundfile):
    cmd = "mpg123 -q --buffer 1024 %s " % soundfile
    if inBG:
      cmd += "&"
    os.system(cmd)
  else:
    print "PlaySound - Warning - No sound file found at %s.  Here are beeps." % soundfile
    for i in range(10):
      print "\a"
      time.sleep(.1)
    

##############################
## MAIN
##############################

#prompts = []
#newRunPrompt = NewRunPrompt()
#prompts.append(newRunPrompt)

print "Starting ChangeSt1Gas..."

while True:
 timestr = time.strftime("%H%M%S")
# now = datetime.now()
 print timestr
# print now

 if timestr == "100000" or timestr == "180000":
  PlaySound( ALARM_VOICE )
# newRunPrompt.Popup( Time, "hour", ["Change Station 1 gas."] )
#  newRunPrompt.Popup()
  print "Please restart this script: ./ChangeSt1Gas.py"
#  time.sleep( SLEEP_AFTER_PROMPT_SECONDS )
  continue







