#!/usr/bin/env python
import os, datetime

#configure
runname = "run3"
logfile = "/home/e906daq/%(runname)s.log" % locals()
backupdir_root = "/data2/log/shifter_log/%(runname)s" % locals()


#for dir and file names
timestamp = datetime.datetime.now().strftime('%Y.%m.%d_%H.%M')
monthdir = datetime.datetime.now().strftime('%Y.%m')
backupdir = os.path.join(backupdir_root, monthdir)
backupfile = os.path.join( backupdir, "%(runname)s.log.%(timestamp)s" % locals() )

#make dirs (if needed) and copy file
if not os.path.isdir( backupdir):
  os.makedirs( backupdir )
os.system( "cp %(logfile)s %(backupfile)s" % locals() )

#print result
print "Created backup file at %(backupfile)s" % locals()
