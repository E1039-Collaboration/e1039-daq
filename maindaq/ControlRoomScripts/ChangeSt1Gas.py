#!/usr/bin/env python
from datetime import datetime
import pygtk
pygtk.require('2.0')
import gtk

import os, glob, time, re

#define sounds
FOURWAY = "~/sounds/4way.mp3"
THREEWAY = "~/sounds/3way.mp3"
ALARM_VOICE = "~/sounds/gas-v1.mp3"
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
#      print "got here"
    os.system(cmd)
    time.sleep(.1)
  else:
    print "PlaySound - Warning - No sound file found at %s.  Here are beeps." % soundfile
    for i in range(10):
      print "\a"
      time.sleep(.1)
   
def CloseWindow(d1=None, d2=None):
    gtk.main_quit()
    d2.destroy()

class Prompt:
  def __init__( self):
    self.title = "Station 1 Gas Prompt"
    self.button_label = "OK"
    self.window = None
    self.button = None

  def Popup( self, extra_lines = [] ):
    self.window = gtk.Window( gtk.WINDOW_TOPLEVEL )
    self.window.set_default_size(200, 200)
    self.window.set_title(self.title)
    self.window.connect('destroy', gtk.main_quit)

    #create the main box
    main_box = gtk.VBox()
    self.window.add(main_box)

    #create a label
    label = gtk.Label()
    label.set_use_markup(True)
    lines = self.GetStandardLines()
    lines.extend( extra_lines )
    lines.append( "Message generated: %s" % datetime.now().strftime("%a, %b %d - %H:%M") )
    markup = "\n".join(lines)
    label.set_markup( markup )

    #add label to a EventBox then main box
    tmpbox = gtk.EventBox()
    tmpbox.add(label)
    main_box.pack_start(tmpbox, True, True, 0)

    #create a button
    markup_button_label = '<span size="%d">%s</span>' % (BODY_TEXT_SIZE, self.button_label)
    self.button = gtk.Button( label=markup_button_label)
    self.button.child.set_use_markup(True)
    self.button.connect( "clicked", self.Close, self.window )

    #add button to a EventBox then main box
    tmpbox = gtk.EventBox()
    tmpbox.add(self.button)
    main_box.pack_start(tmpbox, True, True, 0)

    self.window.show_all()
    gtk.main()

  def Close(self, d1 = None, d2 = None):
    if not self.window.emit("delete-event", gtk.gdk.Event(gtk.gdk.DELETE)):
      self.window.destroy()
    #gtk.main_quit()
    if d2:
      d2.destroy()
    self.window = None
    self.button = None

  def IsPoppedUp(self):
    return self.window is not None

  def GetStandardLines(self):
    raise Exception("Attempted to call Prompt::GetStandardLines on pure virtual base class.  You must implement this in a derived class.")

class ThreeWayPrompt( Prompt ):
  """A prompt to tell the shifter that it is time to change the gas to three way mix"""
  def __init__(self):
    #initialize base class
    Prompt.__init__(self)

    #settings for this class
    self.title = "Change Station 1 Gas"
    self.button_label = "OK.  I changed the gas."

  def GetStandardLines( self ):
    lines = ['<span color="red" size="%d"><b>Change Station 1 Gas</b></span>' % HEADER_TEXT_SIZE]
    lines.append( '<span size="%d">' % BODY_TEXT_SIZE )
    lines.append( "\n" )
    lines.append( "Change to 3-WAY mix (18pm - evening)." )
    lines.append( '</span>' )
    return lines

class FourWayPrompt( Prompt ):
  """A prompt to tell the shifter that it is time to change the gas to four way mix"""
  def __init__(self):
    #initialize base class
    Prompt.__init__(self)

    #settings for this class
    self.title = "Change Station 1 Gas"
    self.button_label = "OK.  I changed the gas."

  def GetStandardLines( self ):
    lines = ['<span color="red" size="%d"><b>Change Station 1 Gas</b></span>' % HEADER_TEXT_SIZE]
    lines.append( '<span size="%d">' % BODY_TEXT_SIZE )
    lines.append( "\n" )
    lines.append( "Change to 4-WAY mix (10am - morning)." )
    lines.append( '</span>' )
    return lines



##############################
## MAIN
##############################

promted_for_new_run = False

prompts = []
fourWayPrompt = FourWayPrompt()
threeWayPrompt = ThreeWayPrompt()
prompts.append(fourWayPrompt)
prompts.append(threeWayPrompt)

print "Starting ChangeSt1Gas..."

while True:
 timestr = time.strftime("%H%M%S")
# runnum = time.strftime("%H:%M:%S")
 size = 0
# now = datetime.now()
 print timestr
 time.sleep(0.25)
# print now

 #if timestr == "114405" or timestr == "180000":
 if timestr == "095500":
  PlaySound( FOURWAY )
  fourWayPrompt.Popup()
# newRunPrompt.Popup( Time, "hour", ["Change Station 1 gas."] )
#  newRunPrompt.Popup()
#  print "Please restart this script: ./ChangeSt1Gas.py"
  time.sleep( SLEEP_AFTER_PROMPT_SECONDS )
  continue

 if timestr == "175500":
  PlaySound( THREEWAY )
  threeWayPrompt.Popup()
# newRunPrompt.Popup( Time, "hour", ["Change Station 1 gas."] )
#  newRunPrompt.Popup()
#  print "Please restart this script: ./ChangeSt1Gas.py"
  time.sleep( SLEEP_AFTER_PROMPT_SECONDS )
  continue

