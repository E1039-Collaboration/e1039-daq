#!/usr/bin/env python

import os
import re
import sys
import time
import subprocess

dataLocation = '/localdata/codadata/'
executilble = '/home/e906daq/crlLiuK/integrityChecker/integrityChecker'
alarmSound = '/home/e906daq/sounds/alarm.mp3'
logfile = '/home/e906daq/crlLiuK/integrityChecker/IntegrityWatch.log'

def PlaySound(soundfile, inBG = False):
    soundfile = os.path.expandvars(soundfile) #expand environmentals
    soundfile = os.path.expanduser(soundfile) #expand tilde
    if os.path.isfile(soundfile):
        cmd = "mpg123 -q --buffer 1024 %s " % soundfile
        if inBG:
            cmd += "&"
        os.system(cmd)
    else:
        print "PlaySound - Warning - No sound file found at %s. Here are beeps." % soundfile
        for i in range(10):
            print "\a"
            time.sleep(.2)

def timestamp():
    stamp = time.strftime('%Y-%m-%d %H:%M:%S')
    return stamp

def checkRoc(res):
    """ Check the data validity in on ROC"""

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
    else:
        for nevt in nEvents[1:]:
            if nevt != nEvents[0]:
                return False

    for i in range(len(nErrors1)):
        if nErrors1[i] > 10 or nErrors2[i] > 10:
            if rocID == 14 and i == 6 and nErrors1[i] > 10:
                return True
            else:
                return False

    return True

def runCheck(filename, spillID):
    print timestamp(), '- Checking', filename, 'with minimum spillID ', spillID
    proc = subprocess.Popen('%s %s %d' % (executilble, filename, spillID), shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    results, err = proc.communicate()
    #print proc.returncode, len(err), err

    results_clean = results.replace('ARM', '').replace('dead', '').replace('BOS', '').replace('EOS', '').replace('Spill', '').replace('targetPos', '').replace('ROC', '').replace('on', '')
    #print results_clean, len(results_clean), results_clean.strip(), len(results_clean.strip())
    #print re.match('^[a-zA-Z]+$', results_clean), len(err)
    if any(l.isalpha() for l in results_clean) or len(err) != 0:
    	print '   File too small or currupted. Wait for another spill and see\n', results
    	print '   If this keeps happening and the file size is smaller than 1GB, start a new run.'
        return 0, False
        #print '   Note: The warning sound is suppressed temporarily on 2017-Feb-22.\n'
        #return 0, True
    elif len(results) < 10:
        return 0, True

    AllGood = True
    spillID = 0
    nSpills = 0
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

    #print AllGood
    if AllGood:
        print timestamp(), '-', nSpills, 'spills checked, all Good.'
    return spillID, AllGood

# main executilble
def main(silent):
    previousFile = ''
    previousSize = 0
    spillID = 0
    while True:
        # check the latest run
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
                spillID, status = runCheck(filename, spillID)
            except:
                print timestamp(), '- this script is going to crash, please record the error, restart script after a new run is started, or call expert'
                PlaySound(alarmSound)
                sys.exit(1)

            if (not status) and (not silent):
                for i in range(2):
                    PlaySound(alarmSound)

            # update the file size
            previousSize = os.path.getsize(filename)

        # sleep for a while
        time.sleep(120)

if __name__ == '__main__':
    silent = False
    if len(sys.argv) > 1:
        print 'Warning: IntegrityWatch started in silent mode.'
        silent = True
    main(silent)
