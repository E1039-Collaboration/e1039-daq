#!/usr/bin/env python
import pygtk
pygtk.require('2.0')
import gtk

import os
import re
import sys
import time
import glob
import datetime
import DAQUtils
import subprocess

SLEEP_AFTER_EOS = 35   #wait this many seconds after EOS before acting
rocID = 0

#font sizes for gtk are regular font size point times 1000
HEADER_TEXT_SIZE = 72000 # = 72pt
BODY_TEXT_SIZE   = 14000

#dataLocation = '/home/e1039daq/crlLiuK/integrityChecker/'
dataLocation = '/localdata/codadata/'
executilble = '/home/e1039daq/crlLiuK/integrityChecker/integrityChecker'
alarmSound = '/home/e1039daq/sounds/alarm.mp3'
logfile = '/home/e1039daq/crlLiuK/integrityChecker/IntegrityWatch.log'

def PlaySound(soundfile, inBG = False):
    soundfile = os.path.expandvars(soundfile) #expand environmentals
    soundfile = os.path.expanduser(soundfile) #expand tilde
    if os.path.isfile(soundfile):
        cmd = "mpg123 -q --buffer 1024 %s " % soundfile
        print ('\033[1;30m now play the alarm sound!!\033[0m!')
        if inBG:
            cmd += "&"
        os.system(cmd)
    else:
        print "PlaySound - Warning - No sound file found at %s. Here are beeps." % soundfile
        for i in range(10):
            print "\a"
            time.sleep(.2)

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

class TDCdataPrompt( Prompt ):
  """A prompt to tell the shifter that the TDC data format has some problem """
  def __init__(self):
    #initialize base class
    Prompt.__init__(self)

    #settings for this class
    self.title = "Problem with TDC Data Format"
    self.button_label = "OK. I fixed it."

  def GetStandardLines( self ):
    lines = ['<span color="red" size="%d"><b>Problem with TDC Data Format</b></span>' % HEADER_TEXT_SIZE]
    lines.append( '<span size="%d">' % BODY_TEXT_SIZE )
    lines.append( "There is a problem with the TDC data format." )
    #lines.append( "Check the <a href='http://e906-gat6.fnal.gov:8081/SeaQuestDAQStatus/'>DAQ Status page</a> to see if the problem still exists." )
    lines.append( "Check the <a href='http://e906-gat6.fnal.gov:8081/SpinQuestDAQStatus/'>DAQ Status page</a> to see if the problem still exists." )
    lines.append( "Make sure the Main DAQ is running." )
    lines.append( "Look for a terminals open on the e1039daq1 computer running 'IntegrityWatch.py'. Check the information printed in red text to confirm the problem." )
    lines.append( "Restart the Main DAQ on the e1039daq1 computer." )
    lines.append( "Call a DAQ expert if the problem persists." )
    lines.append( '</span>' )
    return lines


def timestamp():
    stamp = time.strftime('%Y-%m-%d %H:%M:%S')
    return stamp

def checkRoc(res):
    """ Check the data validity in on ROC"""
    global rocID
    rocinfo = res.strip().split(':')
    rocID = int(rocinfo[0].strip().split()[1])
    nTDCs = int(rocinfo[0].strip().split()[2])
    if len(rocinfo) != nTDCs+1:
        return False

    nEvents = []
    nErrors1 = []
    nErrors2 = []
    for tdc in rocinfo[1:]:
        tdcinfo = [int(item.strip()) for item in tdc.strip().split()]
        nEvents.append(tdcinfo[1])
        nErrors1.append(tdcinfo[2])
        nErrors2.append(tdcinfo[3])

    if rocID == 14:
        for nevt in nEvents[1:-1]:
            if nevt != nEvents[0]:
                return False
	if abs(nEvents[0] - nEvents[-1]) > 100:
		return False
    else:
        for nevt in nEvents[1:]:
            if nevt != nEvents[0]:
                return False

    for i in range(len(nErrors1)):
        if nErrors1[i] > 10 or nErrors2[i] > 10:
            if rocID == 14 and i == 6 and nErrors1[i] > 10 and nErrors1[i] < 100:
                return True
            else:
                return False

    return True

def runCheck(filename, spillID):
    global rocID
    print timestamp(), '- Checking', filename, 'with minimum spillID ', spillID
    proc = subprocess.Popen('%s %s %d' % (executilble, filename, spillID), shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    results, err = proc.communicate()
    #print proc.returncode, len(err), err
    
    results_clean = results.replace('ARM', '').replace('dead', '').replace('BOS', '').replace('EOS', '').replace('Spill', '').replace('targetPos', '').replace('ROC', '').replace('on', '')
    #print results_clean, len(results_clean), results_clean.strip(), len(results_clean.strip())
   # print re.match('^[a-zA-Z]+$', results_clean), len(err)
    
 
    if (re.match('^[a-zA-Z]+$', results_clean))!=None or len(err) != 0:
        print '   File too small or currupted. Wait for another spill and see\n', results
        print '   If this keeps happening and the file size is smaller than 1GB, start a new run.'
        return 0, False, None
        #print '   Note: The warning sound is suppressed temporarily on 2017-Feb-22.\n'
        #return 0, True
    elif len(results) < 10:
        return 0, True, None

    AllGood = True
    spillID = 0
    nSpills = 0
    badROC=None
    for roc in results.strip().split('\n'):
    	#print roc
        if 'Normal' in roc:
            continue
        elif 'Spill' in roc:
            spillID = int(roc.strip().split()[1])
            nSpills = nSpills + 1
    	elif 'ARM' in roc:
            AllGood = False
            print '   Spill %d Error: %s' % (spillID, roc)
            print '   If this line shows up in more than 2 consecutive checks, power cycle this crate. '
        elif any(l.isalpha() for l in roc.replace('ROC', '')):
            print '   Does not understand the integrityChecker output:', roc
            print '   If this keeps happening, record the error message and post it to ELOG'
    	elif not checkRoc(roc):
            AllGood = False
            print '   Spill %d Error: %s' % (spillID, roc)
            print '   If this line shows up in more than 2 consecutive checks, power cycle this crate. '
            badROC = rocID
    #print AllGood
    if AllGood:
        print timestamp(), '-', nSpills, 'spills checked, all Good.'
    return spillID, AllGood, badROC

DAQUtils.UseGatEPICS()
os.environ["EPICS_CA_ADDR_LIST"] = "e1039gat1.fnal.gov"

print "Starting IntegrityWatch..."
print "Wait for the next EOS..."

# main executilble
def main(silent):
    previousFile = ''
    previousSize = 0
    spillID = 0
    tdcdataPrompt = TDCdataPrompt()    

    while True:
        #look for EOS flag
        eosflag = DAQUtils.GetFromEPICS( "EOS" )
	if eosflag is "1":
          print "EOS = 1"
          time.sleep(SLEEP_AFTER_EOS)
        else:
          time.sleep(0.5)
          continue

        # check the latest run
        #print "start check"
        filename = max([os.path.join(dataLocation, f) for f in os.listdir(dataLocation) if f.endswith('.dat') and f.startswith('run_') and os.path.getsize(os.path.join(dataLocation, f)) > 20000000], key = os.path.getctime)

        # if this is a new file
        if filename != previousFile:
            spillID = 0
            previousFile = filename
            previousSize = 0

        # run check
        currentSize = os.path.getsize(filename)
        if currentSize - previousSize > 5000000:
            try:
                spillID, status, badROC = runCheck(filename, spillID)
            except:
                print timestamp(), '- this script is going to crash, please record the error, restart script after a new run is started, or call expert'
                PlaySound(alarmSound)
                extra_lines = []
                if(badROC!=None):
                    extra_lines.append( "ROC %d has problem." %badROC )
                tdcdataPrompt.Popup( extra_lines )
                sys.exit(1)

            if (not status) and (not silent):
                for i in range(2):
                    PlaySound(alarmSound)
                    extra_lines = []
                    if(badROC!=None):
                        extra_lines.append( "ROC %d has problem." %badROC )
                    tdcdataPrompt.Popup( extra_lines )               
      
            # update the file size
            previousSize = os.path.getsize(filename)

        # sleep for a while
        #time.sleep(120)

if __name__ == '__main__':
    silent = False
    if len(sys.argv) > 1:
        print 'Warning: IntegrityWatch started in silent mode.'
        silent = True
    main(silent)
