#!/usr/bin/env python
#########################
# Backup mainDAQ codafiles to raid from e906daq1's local disk.
# Remove codafiles from e906daq1's local disk if they are backed up.
#------------------------
# Brian Tice - tice@anl.gov
# Nov 17, 2013
#########################
import os, sys, hashlib, shutil, optparse, glob, time
import logging, operator

#################
#define constants
#################
#file stuff
ORIG_DIR = "/localdata/codadata/"   #where are original files
RAID_DIR = "/data3/data/mainDAQ/"  #where is the raid backup
FILENAME_PATTERN = "run*.dat"                                   #coda filename pattern

#units
B_TO_GB = 1. / (1024*1024*1024)
DAY_TO_S = 24*60*60

#global variable to be controlled by script command line flag.  Do not change the value here (keep it False)
TESTING = False

#this dict is used to parse user verbosity options
userVerbosity = { "Silent":0, "Error":1, "Warning": 2,  "Info":3,  "Debug":4 }
userVerbPrint = "%s" % sorted( userVerbosity.iteritems(), key=operator.itemgetter(1) )



def GetChecksum(filename):
  if not os.path.isfile(filename):
    raise Exception( "Cannot form checksum for file that does not exist: %(filename)s" % locals() )
  return hashlib.md5( open(filename, 'rb').read() ).hexdigest()


#define CodaFile class
class CodaFile:
  """Knows the filename on daq, raid, and dcache.  Knows if file is successfully backed up on raid and dcache.  Performs backup and removal.  Stores errors."""
  def __init__(self, filename, use_md5):
    """Construct the CodaFile object to know all its possible locations"""

    #not sure what to do when handed a filename with no file
    if not os.path.isfile(filename):
      raise Exception( "Cannot create CodaFile if file doesn't exist: %s" % filename )

    self.use_md5 = use_md5

    #populate filenames
    self.basename = os.path.basename(filename)
    self.orig_file = os.path.join( ORIG_DIR, self.basename )
    self.raid_file = os.path.join( RAID_DIR, self.basename )

    #get ready to compile error messages
    self.errors = []

    #store file attributes
    self.orig_size  = os.path.getsize( self.orig_file ) * B_TO_GB
    self.orig_mtime = os.stat( self.orig_file ).st_mtime
    self.raid_size  = 0
    self.raid_mtime = 0

    #checksums may be used to make sure files are identical
    self.orig_md5 = None
    self.raid_md5 = None

    if self.use_md5:
      self.orig_md5 = GetChecksum( self.orig_file )

    self.is_on_raid = self.CheckRaid()

  def __str__(self):
    rval = " ===> " + self.__class__.__name__ + ": " + self.basename + "\n"
    info = []
    info.append( "\tOriginal file: " + self.orig_file )
    if self.is_on_raid:
      info.append( "Raid file: " + self.raid_file )
    else:
      info.append( "File is not backed up to raid" )
    info.append( "File size: %.2f" % self.orig_size )
    if len(self.errors)==0:
      info.append("No Errors")
    else:
      info.append( "Errors: " + " | ".join(self.errors) )
    rval += "; ".join( info )
    return rval

  def CheckRaid(self):
    """check that the raid copy is the same as the original"""
    global logger
    if os.path.exists( self.raid_file ):
      logger.debug("Backup file exists at %s" % self.raid_file )
      #file is on raid, but is it really the same file?
      self.raid_size  = os.path.getsize( self.raid_file ) * B_TO_GB
      self.raid_mtime= os.stat( self.raid_file ).st_mtime
      if self.raid_size != self.orig_size:
        self.errors.append("Size mismatch between original and raid copy.")
      if self.use_md5:
        self.raid_md5 = GetChecksum( self.raid_file )
        if self.raid_md5 != self.orig_md5:
          self.errors.append( "Checkum mismatch between original and raid copy." )
      return True
    else:
      logger.debug("No backup file exists at %s" % self.raid_file )
      return False


  def CopyToRaid(self, force=False):
    """Copy the original to raid and check the result"""
    if not force and os.path.isfile( self.raid_file ):
      errors.append( "You must use 'force' to copy a file to raid if it already exists there" )
      return False
    try:
      if not TESTING:
        print "Do not copy to raid disk yet because another script is doing that."
        shutil.copyfile( self.orig_file, self.raid_file )
    except Exception, e:
      errors.append( "Exception in CopyToRaid: %s", e.what() )
      return False
    return self.CheckRaid()


##############################
##############################
# Begin main
##############################
##############################

# user options
parser = optparse.OptionParser()

parser.add_option( "--copy", dest="copy_to_raid", default=False, action="store_true", \
    help="Copy a backup from localdata to raid.")

parser.add_option( "--remove", dest="remove_old_originals", default=False, action="store_true", \
    help="Remove old files on localdata area if they have been successfully backed up.")

parser.add_option( "--show", dest="show", default=False, action="store_true", \
    help="Show the status of files on the local disk.")

parser.add_option( "--removal_age", dest="removal_age", default=30, type="float", \
    help="Number of days to wait before removing backed-up originals (default=%default)")

parser.add_option( "--backup_age", dest="backup_age", default=0.25, \
    help="Number of days to wait before backing up the most recently edited file.  Avoids backing up the live file. (default=%default)" )

parser.add_option( "--use_md5", dest="use_md5", default=False, action="store_true", \
    help="Use md5 checksum to verify contents of backup are the same as original.  Provides a better check than just size, but takes a long time.")

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

#transfer test option to global variable
TESTING = opts.test

logger.info( "-"*6 + "Start running at "+time.ctime()+ "-"*6 )

#find files
filenames = [ f for f in glob.glob( os.path.join( ORIG_DIR, FILENAME_PATTERN ) ) ]
logger.debug( "Found %d files in localdata" % len(filenames) )

#create CodaFiles
files = [ CodaFile(f, opts.use_md5) for f in filenames ]
#sort CodaFiles old->new
files.sort( key = lambda f: f.orig_mtime )

#do not use the most recent file if it has been modified recently
if the_time - files[-1].orig_mtime < opts.backup_age*DAY_TO_S:
  logger.info( "Skipping most recent file %s because it was modified %.1f hr ago." % (files[-1].basename, (the_time - files[-1].orig_mtime)/(60*60) ) )
  files.pop()

#do raid backup and clean localdata
for f in files:
  if opts.copy_to_raid and not f.is_on_raid:
    logger.info( "Copying %s to raid." % f.basename )
    if not f.CopyToRaid():
      logger.error( "Could not copy %s to raid.  Try again later" % f.basename )
      continue
    else:
      logger.debug( "Copy of %s to raid successful." % f.basename )

  if opts.remove_old_originals:
    deltaT = float(the_time - f.orig_mtime)
    oldT   = float(opts.removal_age*DAY_TO_S)
    #delete files that are backed up and there are no errors (like checksum fail)
    if f.is_on_raid and 0 == len(f.errors) and oldT < deltaT:
      logger.info( "Deleting successully backed up old file: %s" % f.basename )
      if not TESTING:
        os.remove( f.orig_file )

#print a final status
if opts.show:
  for f in files:
    print f
else:
  bad_files = [ f for f in files if len(f.errors) != 0 ]
  if len(bad_files) == 0:
    logger.info( "No errors were found." )
  else:
    logger.info( "Found %d bad backups: " % len(bad_files) )
    for f in bad_files:
      logger.info( "   %s -> %s" % (f.basename, f.errors) )
