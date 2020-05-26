#!/usr/bin/env python
#########################
# Backup mainDAQ codafiles to raid from e906daq1's local disk.
# Remove codafiles from e906daq1's local disk if they are backed up.
#------------------------
# Brian Tice - tice@anl.gov
# Nov 17, 2013
#########################
import os, sys, hashlib, shutil, optparse, glob, time
from re import match
import logging, operator
import FileBackupTool as fbt

#this dict is used to parse user verbosity options
userVerbosity = { "Silent":0, "Error":1, "Warning": 2,  "Info":3,  "Debug":4 }
userVerbPrint = "%s" % sorted( userVerbosity.iteritems(), key=operator.itemgetter(1) )

##############################
##############################
# Begin main
##############################
##############################

fileTypes = ["scaler", "main"]

# user options
parser = optparse.OptionParser()

parser.add_option( "--file-type", "--type", dest="file_type", default=None, \
    help = "What type of files do you want to use [required].  Options = %s" % str(fileTypes) )

parser.add_option( "--copy-to-raid", dest="copy_to_raid", default=False, action="store_true", \
    help="Copy a backup to raid.")

parser.add_option( "--copy-to-pnfs", dest="copy_to_pnfs", default=False, action="store_true", \
    help="Copy a backup to PNFS.")

parser.add_option( "--remove-raw", dest="remove_old_raw", default=False, action="store_true", \
    help="Remove old raw files on localdata area if they have been successfully backed up to raid and tape (where applicable).")

parser.add_option( "--remove-raid", dest="remove_old_raid", default=False, action="store_true", \
    help="Remove old files on raid backup area if they have been successfully backed up to tape.")

parser.add_option( "--show", dest="show", default=False, action="store_true", \
    help="Show the status of files on the local disk.")

parser.add_option( "--show-md5", dest="show_md5", default=False, action="store_true", \
    help="Check the md5 checksums for errors even if nothing will be removed (implies --show)." )

parser.add_option( "--removal-age", dest="removal_age", default=14, type="float", \
    help="Number of days to wait before removing backed-up originals (default=%default)")

parser.add_option( "--backup-age", dest="backup_age", default=0.25, \
    help="Number of days to wait before backing up the most recently edited file.  Avoids backing up the live file. (default=%default)" )

parser.add_option( "--min-run", dest="min_run", default=0, type="int", \
    help="Minimum run number to consider." )

parser.add_option( "--max-run", dest="max_run", default=999999, type="int", \
    help="Maximum run number to consider." )

parser.add_option( "--test", dest="test", default=False, action="store_true", \
    help="Test mode.  Go through normal operations and print, but do not move or delete anything.")

parser.add_option( "--logfile", dest="logfile", default=None, \
    help="Write log messaes to this file instead of screen.")

parser.add_option("--verbosity", dest="verbosity", default=userVerbosity["Info"], type="int" , \
    help="Verbosity level integer level (default=%%default) %s"%userVerbPrint)

#if no args are given, print help
if len(sys.argv) == 1:
  parser.parse_args( ["--help"] )

#get the args
(opts,args) = parser.parse_args()

#get current time
the_time = time.time()


####################
#Set up message logs
#get a logger for this script
if opts.logfile:
  #make sure file it writable
  try:
    testfile = open(opts.logfile, "a")
    testfile.close()
  except:
    sys.exit("ERROR - Cannot open write to proposed logfile: " + opts.logfile )
  logging.basicConfig( format="%(name)s - %(levelname)s - %(message)s", level=logging.INFO, filename=opts.logfile )
else:
  logging.basicConfig( format="%(name)s - %(levelname)s - %(message)s", level=logging.INFO)
logger = logging.getLogger("BackupCoda")


#########################
#set logger verbosity
#########################
if opts.verbosity == userVerbosity["Silent"]:
  logging.disable( logging.CRITICAL ) #disable all messages
elif opts.verbosity == userVerbosity["Error"]:
  logger.setLevel( logging.ERROR )
elif opts.verbosity == userVerbosity["Warning"]:
  logger.setLevel( logging.WARNING )
elif opts.verbosity == userVerbosity["Info"]:
  logger.setLevel( logging.INFO )
elif opts.verbosity == userVerbosity["Debug"]:
  logger.setLevel( logging.DEBUG )
else:
  logger.critical( "I do not understand the verbosity you specified.  Choose one from %s." % userVerbPrint )
  sys.exit()

logger.info( "-"*6 + "Start running at "+time.ctime()+ "-"*6 )

#apply choice of data type
indir = None
filepat = None
MyFileClass = None
if opts.file_type == "main":
  indir = "/codadata"
  filepat = "run_d{8}.dat"
  MyFileClass = fbt.MainDAQDataFile
elif opts.file_type == "scaler":
  indir = "/scaler_codadata"
  filepat = "scaler_\d+.dat"
  MyFileClass = fbt.ScalerDAQDataFile
else:
  sys.exit("ERROR: You can an unkown file type (%s).  Use the --file-type flag to select one from (%s)." % (opts.file_type, str(fileTypes) ) )

#show-checksum implies show
if opts.show_md5:
  opts.show = True

#find the files
filenames = []
for root,dirs,files in os.walk(indir):
  for name in files:
    if not match( filepat, name ):
      continue
    filenames.append( os.path.join(root,name) )

logger.debug( "Found %d files in localdata" % len(filenames) )

#create CodaFiles of appropriate type
files = [ MyFileClass( f, logger=logger, test=opts.test) for f in filenames ]

#restrict the run range
#note: -1 is stored for run parameter in case of failure to find run number 
files = [ f for f in files if opts.min_run <= f.run and f.run <= opts.max_run ]

if len(files) == 0:
  sys.exit( "Found no matching files." )

#sort CodaFiles run number
files.sort( key = lambda f: f.run )

#do not use the most recent file if it has been modified recently
if the_time - files[-1].orig_mtime < opts.backup_age*fbt.DAY_TO_S:
  logger.info( "Skipping most recent file %s because it was modified %.1f hr ago." % (files[-1].basename, (the_time - files[-1].orig_mtime)/(60*60) ) )
  files.pop()

#do requested operations for each file
for f in files:
  if opts.show_md5:
    f.CalculateChecksums()

  #copy to backup
  #to raid if a known raid location exists and the file doesn't
  if opts.copy_to_raid and f.raid_file and f.orig_file != f.raid_file:
    logger.info( "Copying %s to raid." % f.basename )
    if not f.CopyToRaid():
      logger.error( "Could not copy %s to raid.  Try again later" % f.basename )
      continue
    else:
      logger.debug( "Copy of %s to raid successful." % f.basename )

  #to pnfs if a known raid location exists and the file doesn't
  if opts.copy_to_pnfs and f.pnfs_file and f.orig_file != f.pnfs_file:
    logger.info( "Copying %s to pnfs." % f.basename )
    if not f.CopyToPNFS():
      logger.error( "Could not copy %s to pnfs.  Try again later" % f.basename )
      continue
    else:
      logger.debug( "Copy of %s to pnfs successful." % f.basename )

  if opts.remove_old_raw and f.is_on_raw:
    deltaT = float(the_time - f.raw_mtime)
    oldT   = float(opts.removal_age*fbt.DAY_TO_S)
    #delete files that are backed up and there are no errors (like checksum fail)
    if f.CanRemoveRaw() and oldT < deltaT:
      logger.info( "Deleting successully backed up old file: %s" % f.basename )
      if not opts.test:
        os.remove( f.raw_file )

  if opts.remove_old_raid and f.is_on_raid:
    deltaT = float(the_time - f.raid_mtime)
    oldT   = float(opts.removal_age*fbt.DAY_TO_S)
    #delete files that are backed up and there are no errors (like checksum fail)
    if f.CanRemoveRaid() and oldT < deltaT:
      logger.info( "Deleting successully backed up old file: %s" % f.basename )
      if not opts.test:
        os.remove( f.raid_file )

#print a final status
if opts.show:
  for f in files:
    logger.info(f)
else:
  bad_files = [ f for f in files if len(f.errors) != 0 ]
  if len(bad_files) == 0:
    logger.info( "No errors were found." )
  else:
    logger.info( "Found %d bad backups: " % len(bad_files) )
    for f in bad_files:
      logger.info( "   %s -> %s" % (f.basename, f.errors) )
