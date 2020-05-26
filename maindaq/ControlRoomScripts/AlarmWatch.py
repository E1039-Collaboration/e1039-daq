#!/usr/bin/env python
import pygtk
pygtk.require('2.0')
import gtk

import os, glob, time, re, datetime
import DAQUtils

#CONSTANTS
BYTE_TO_GB    = 1. / (1024*1024*1024)
MAX_GB        = 0.99

TIME_BETWEEN_CHECKS = 5 #in seconds
STATUS_MONITOR_WARN_TIME = 60 #if status monitor has not updated in this number of seconds, sound alarm

#font sizes for gtk are regular font size point times 1000
HEADER_TEXT_SIZE = 72000 # = 72pt
BODY_TEXT_SIZE   = 14000

#define sounds
ALARM_NOISE = "~/sounds/alarm.mp3"
EPICS_VOICE = "~/sounds/voice_epics.mp3"
STATUS_MONITOR_VOICE = "~/sounds/voice_statmon.mp3"

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
      time.sleep(.05)


def CloseWindow(d1=None, d2=None):
    gtk.main_quit()
    d2.destroy()

class Prompt:
  def __init__( self):
    self.title = "Alarm Watch Prompt"
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
    lines = self.GetStandardLines( )
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
    gtk.main_quit()
    if d2:
      d2.destroy()
    self.window = None
    self.button = None

  def IsPoppedUp(self):
    return self.window is not None

  def GetStandardLines(self):
    raise Exception("Attempted to call Prompt::GetStandardLines on pure virtual base class.  You must implement this in a derived class.")

class EPICSPrompt( Prompt ):
  """A prompt to tell the shifter that the target epics flag is true"""
  def __init__(self):
    #initialize base class
    Prompt.__init__(self)

    #settings for this class
    self.title = "Problem with EPICS on Target Computer"
    self.button_label = "OK.  I fixed it."

  def GetStandardLines( self ):
#    lines = ['<span color="red" size="%d"><b>Problem with EPICS on Target Computer</b></span>' % HEADER_TEXT_SIZE]
    lines = ['<span color="red" size="%d"><b>Problem with Target Computer</b></span>' % HEADER_TEXT_SIZE]
    lines.append( '<span size="%d">' % BODY_TEXT_SIZE )
    lines.append( "Slowcontrols cannot find the BOS/EOS signal on the Target computer." )
    lines.append( "Check the <a href='http://e906-gat6.fnal.gov:8081/SeaQuestDAQStatus/'>DAQ Status page</a> to see if the Target EPICS problem still exists." )
    lines.append( "Make sure the target computer is on.  Turn it on if it is not on." )
    lines.append( "Look for a terminals open on the target computer running 'EPICS.bat' and 'perl.exe'.  Close these terminals if they exists." )
    lines.append( "Restart EPICS on the target computer by double clicking 'start_epics.pl' on the desktop." )
    lines.append( "Call a slowcontrol and/or target expert if the problem persists." )
    lines.append( '</span>' )
    return lines
      

class StatusMonitorPrompt( Prompt ):
  """A prompt to tell the shifter that the status monitor is not running"""
  def __init__(self):
    #initialize base class
    Prompt.__init__(self)

    #settings for this class
    self.title = "Problem with DAQStatus Monitor"
    self.button_label = "OK.  We are monitoring the DAQ status again."

  def GetStandardLines( self ):
    lines = ['<span color="red" size="%d"><b>Problem with Status Monitor</b></span>' % HEADER_TEXT_SIZE]
    lines.append( '<span size="%d">' % BODY_TEXT_SIZE )
    lines.append( "The DAQStatus monitor script has not performed a check recently." )
    lines.append( "Check the <a href='http://e906-gat6.fnal.gov:8081/SeaQuestDAQStatus/'>DAQ Status page</a> to see if the page is really updating." )
    lines.append( " ... See 'Last Updated' in the StatusMonitor Status block." )
    lines.append( "Refer to <a href='http://seaquest-docdb.fnal.gov:8080/cgi-bin/ShowDocument?docid=1033'>DAQ How To document in docdb 1033</a> for how to fix the status_monitor.py script under 'DAQ Status Page is Not Updating'." )
    lines.append( '</span>' )
    return lines


##############################
## MAIN
##############################

epicsPrompt = EPICSPrompt()
statMonPrompt = StatusMonitorPrompt()


#must setup EPICS
DAQUtils.UseGatEPICS()
epicsTimeout = 10 #use very long timeout to avoid false positives from slow connections


while True:
  #-----
  #check the value of ALARM_TARGET_EPICS in EPICS
  alarmTargetEPICS = DAQUtils.GetFromEPICS( "ALARM_TARGET_EPICS", epicsTimeout )

  #if not 0, then alarm
  if alarmTargetEPICS != "0":
    PlaySound( ALARM_NOISE )
    PlaySound( EPICS_VOICE, inBG = True )
    extra_lines = [ "Value of ALARM_TARGET_EPICS was '%s'" % alarmTargetEPICS ]
    if alarmTargetEPICS == "":
      extra_lines.append( " ... It was empty, which means that this script could not read the EPICS value from e906-gat3, refer to <a href='http://seaquest-docdb.fnal.gov:8080/cgi-bin/ShowDocument?docid=1033'>DAQ How To document in docdb 1033</a> for help." )
    epicsPrompt.Popup( extra_lines )
  else:
    print "Target computer OK"


  #-----
  #check the age of the DAQStatusMonitor logfile to make sure it is running
  #directories and files are named by date with fixed width numbers, so sorting
  logDir = "/data2/log/slowcontrol/status_monitor"
  
  oldestDir = sorted( os.listdir(logDir) )[-1]
  oldestDirFull = os.path.join( logDir, oldestDir )

  oldestFile = sorted( os.listdir(oldestDirFull) )[-1]
  oldestFileFull = os.path.join( oldestDirFull, oldestFile )
  print oldestFileFull

  age = int(time.time()) - os.path.getmtime(oldestFileFull)
  if STATUS_MONITOR_WARN_TIME < age:
    PlaySound( ALARM_NOISE )
    PlaySound( STATUS_MONITOR_VOICE, inBG = True )
    extra_lines = []
    extra_lines.append( "Status monitor last updated %ds ago." % age )
    statMonPrompt.Popup( extra_lines )
  else:
    print "Status monitor OK.  Last updated %ds ago" % age

  time.sleep( TIME_BETWEEN_CHECKS )
