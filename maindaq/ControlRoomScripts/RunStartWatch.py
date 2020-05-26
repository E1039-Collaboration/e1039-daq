#!/usr/bin/env python
import pygtk
pygtk.require('2.0')
import gtk

import os, glob, time, re, datetime
import DAQUtils


#CONSTANTS
BYTE_TO_GB    = 1. / (1024*1024*1024)
MAX_GB        = 6.0 #1.5

SLEEP_AFTER_EOS = 35   #wait this many seconds after EOS before acting

SLEEP_AFTER_NOTHING_SECONDS = 1  #1 second (we wait for EOS)
SLEEP_AFTER_PROMPT_SECONDS = 3*60 #3 minutes
SIZE_DIFF_LIMIT = 1000.0/1024/1024 #100kb  #if a file changes by less, consider it unchanged (in GB)

MIN_G2SEM = 1E9

FILEPATT      = "/localdata/codadata/run*.dat"

#font sizes for gtk are regular font size point times 1000
HEADER_TEXT_SIZE = 72000 # = 72pt
BODY_TEXT_SIZE   = 14000

#define sounds
ALARM_NOISE = "~/sounds/alarm.mp3"
ALARM_VOICE = "~/sounds/voice_no_events.mp3"
NEWRUN_NOISE = "~/sounds/coin_sound.mp3"
NEWRUN_VOICE = "~/sounds/voice_start_new_run.mp3"
NEWRUN_AGAIN_VOICE = "~/sounds/voice_forgot_to_start_run.mp3"
CANT_CHECK_VOICE = "~/sounds/cant_check_voice.mp3"

def Log( string ):
  timestamp = datetime.datetime.now()
  print timestamp.strftime( "%H:%M:%S" ) + " - " + string


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


def CloseWindow(d1=None, d2=None):
    gtk.main_quit()
    d2.destroy()

class Prompt:
  def __init__( self):
    self.title = "Run Watch Prompt"
    self.button_label = "OK"
    self.window = None
    self.button = None

  def Popup( self, runnum = None, size = None, extra_lines = [] ):
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
    lines = self.GetStandardLines( runnum, size )
    lines.extend( extra_lines )
    lines.append( "Message generated: %s" % datetime.datetime.now().strftime("%a, %b %d - %H:%M") )
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

  def GetStandardLines(self, runnum, size):
    raise Exception("Attempted to call Prompt::GetStandardLines on pure virtual base class.  You must implement this in a derived class.")

class NewRunPrompt( Prompt ):
  """A prompt to tell the shifter that it is time to start a new run"""
  def __init__(self):
    #initialize base class
    Prompt.__init__(self)

    #settings for this class
    self.title = "Start a New Run"
    self.button_label = "OK.  I started a new run."

  def GetStandardLines( self, runnum, size ):
    lines = ['<span color="red" size="%d"><b>Start a new run</b></span>' % HEADER_TEXT_SIZE]
    lines.append( '<span size="%d">' % BODY_TEXT_SIZE )
    lines.append( 'Run %(runnum)d is getting big (%(size).2fG)' % locals() )
    lines.append( '</span>' )
    return lines
    
class CantCheckPrompt( Prompt ):
  """A prompt to tell the shifter that there is a problem with this very script"""
  def __init__(self):
    #initialize base class
    Prompt.__init__(self)

    #settings for this class
    self.title = "RunWatchStart has a problem"
    self.button_label = "Acknowledged.  Go back to checking my run."

  def GetStandardLines( self, runnum, size ):
    lines = ['<span color="red" size="%d"><b>%s</b></span>' % ( HEADER_TEXT_SIZE, self.title ) ]
    lines.append( '<span size="%d">' % BODY_TEXT_SIZE )
    lines.append( 'Something caused this script to fail.' )
    lines.append( 'If this keeps happening, there may be a problem.  Contact experts.' )
    lines.append( 'Current run %(runnum)d has size (%(size).2fG)' % locals() )
    lines.append( '</span>' )
    return lines


class DeadRunPrompt( Prompt ):
  """A prompt to tell the shifter that the most recent coda file is not growing"""
  def __init__(self):
    #initialize base class
    Prompt.__init__(self)

    #settings for this class
    self.title = "Check the current run!"
    self.button_label = "OK. We are taking data again."

  def GetStandardLines( self, runnum, size ):
    lines = ['<span color="red" size="%d"><b>Check the current run</b></span>' % HEADER_TEXT_SIZE]
    lines.append( '<span size="%d">' % BODY_TEXT_SIZE )
    lines.append( "There may be something wrong.  Check the run control." )
    lines.append( 'Current run %(runnum)d has size (%(size).2fG)' % locals() )
    lines.append( "\n" )
    lines.append( "<b>Is the beam down?</b>Note this in the elog and check the MCR elog for details." )
    lines.append( "<b>Did you forget to press start?</b>  The 'run state' will read 'prestarted'.  If so, press start now.  Make sure events are read the next spill.  If you get no events, start a new run." )
    lines.append( '<b>Is there no prestart event?</b>  The "Total events" will be 0.  If so, try to reset and start a new run.  If there are failures, consult the <a href="http://twiki.npl.illinois.edu/bin/view/E906/WhatToDoWhenROCGoesDown">What To Do When ROC Goes Down</a> on the Twiki.' )
    lines.append( '</span>' )
    return lines


##############################
## MAIN
##############################

#keep track of file size
prev_run = -999
prev_size = -999

#keep track of if I told you to start a new run
promted_for_new_run = False

prompts = []
newRunPrompt = NewRunPrompt()
deadRunPrompt = DeadRunPrompt()
cantCheckPrompt = CantCheckPrompt()
prompts.append(newRunPrompt)
prompts.append(deadRunPrompt)
prompts.append(cantCheckPrompt)

DAQUtils.UseGatEPICS()

print "Starting RunStartWatch..."
print "Wait for the next EOS..."

while True:
  #look for EOS flag
  #eosflag = DAQUtils.GetFromEPICS( "E906EOS" )
  eosflag = DAQUtils.GetFromEPICS( "EOS" )
  if eosflag is "1":
    print "EOS = 1"
    time.sleep(SLEEP_AFTER_EOS)
  else:
    time.sleep(1)
    continue

  files = [ f for f in glob.glob( FILEPATT ) ]
  if len(files) < 1:
    break
  files.sort(key=os.path.getmtime)
  recentFile = files[-1]
  size = os.path.getsize( recentFile ) * BYTE_TO_GB
  runnum = -1
  m = re.search( r'_(\d{6}).dat$', recentFile )
  if m:
    runnum = int( m.group(1) )
  percentdone = 100*size / MAX_GB
  os.system( "caput -t MAIN_DAQ_FILESIZE %f > /dev/null 2>&1" % size )
  Log( "Most recent run %(runnum)d has size %(size).2fG (%(percentdone).0f%% done)" % locals() )

  if runnum == prev_run:
    #sam run, check that it's OK

    #CHECK FOR PROBLEM
    # if beam and no file size increase there is a problem
    # return value from EPICS is empty string if there is a problem    
    size_diff = size - prev_size
    #g2sem  = DAQUtils.GetFromEPICS('G2SEM')
    g2sem  = DAQUtils.GetFromEPICS('S:G2SEM')
    try:
      g2sem  = float(g2sem)
      if size_diff < SIZE_DIFF_LIMIT and MIN_G2SEM < g2sem:
        diff_in_kb = size_diff * 1024*1024
        PlaySound( ALARM_NOISE )
        PlaySound( ALARM_VOICE, inBG = True )
        deadRunPrompt.Popup( runnum, size, ["Saw beam with %(g2sem).2E at G2SEM but filesize increased only by %(diff_in_kb).2f kB." % locals() ] )
        time.sleep( SLEEP_AFTER_PROMPT_SECONDS )
    except ValueError:
      PlaySound( ALARM_NOISE )
      PlaySound( CANT_CHECK_VOICE, inBG = True )
      cantCheckPrompt.Popup( runnum, size, ["Got bad value from EPICS: '%s' for g2sem." % ( str(g2sem) )] )

    #did you claim you started a new run
    if prompted_for_new_run:
      PlaySound( ALARM_NOISE )
      PlaySound( NEWRUN_AGAIN_VOICE, inBG = True )
      Log( "Hey!  You should have started a new run already" )
      newRunPrompt.Popup( runnum, size, ["Hey!  You should have started a new run already.  Definitely do it now."] )
      continue
  else:
    #found a new run
    Log("Looks like you started a new run %(runnum)d" % locals() )
    prev_run = runnum
    prompted_for_new_run = False

    #clear all prompts
    for prompt in prompts:
      if prompt.IsPoppedUp():
        prompt.Close()

  prev_size = size

  if size > MAX_GB:
    # create a popup menu
    PlaySound( NEWRUN_NOISE )
    PlaySound( NEWRUN_VOICE, inBG = True )
    newRunPrompt.Popup( runnum, size )
    prompted_for_new_run = True
    time.sleep( SLEEP_AFTER_PROMPT_SECONDS )
  else:
    time.sleep( SLEEP_AFTER_NOTHING_SECONDS )
