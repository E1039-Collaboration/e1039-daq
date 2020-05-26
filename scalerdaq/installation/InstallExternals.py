#!/usr/bin/env python
import os, sys, optparse
from contextlib import contextmanager

######################
@contextmanager
def pushd(newDir, createNew = False):
  """Move to a new directory (create if needed and desired) and then run other code at yield.  Return to original directory when done.
     See http://stackoverflow.com/questions/6194499/python-os-system-pushd"""
  newDir = os.path.expandvars(newDir)
  #create the new directory if it doesn't exist and the user allows creation
  if createNew and not os.path.isdir(newDir):
    os.makedirs(newDir)
  #complain and quit if the directory doesn't exist
  if not os.path.isdir(newDir):
    raise Exception( "pushd: You cannot change to directory that does not exist: %(newDir)s" % locals() )
  #save current dir and move to new one
  previousDir = os.getcwd()
  os.chdir(newDir)
  yield
  os.chdir(previousDir)


def GetFile(f):
  #set proxy if necessary.  set to None if no proxy is needed.
  proxy = "e906-gat2.fnal.gov:3128"
  cmd = "wget"
  if proxy:
    cmd += " -e https_proxy=%s -e use_proxy=on" % proxy
  cmd += " " + f
  os.system(cmd)

def UnpackFile(f):
  basename = os.path.basename(f)
  cmd = None
  if basename.endswith("tar.gz"):
    cmd = "tar -xzvf"
  elif basename.endswith(".tar"):
    cmd = "tar -xz" 

  if cmd:
    os.system( cmd + " " + basename )
    os.unlink(basename)
  else:
    print "No file named '%s' to unpack" % basename

def Make(allow_multicore=True, extra_args=""):
  """Run make with standard flags"""
  make_cmd = "make " + extra_args
  if allow_multicore:
    if os.environ.has_key("N_MAKE_CORES") and os.getenv("N_MAKE_CORES")!="":
      ncores = int( os.getenv("N_MAKE_CORES") )
      if ncores > 0:
        make_cmd += " -j%d" % ncores
  print make_cmd
  os.system(make_cmd)

#####################
# Install ROOT
#####################
def InstallROOT(prefix):
  root_v = "5.34.18"
  root_sys = os.path.join( prefix, "root" )
  print "Installing root version %s at %s" % (root_v, root_sys)

  with pushd(prefix, createNew=True):
    tarfile = "http://root.cern.ch/download/root_v%s.source.tar.gz" % root_v
    GetFile(tarfile)
    UnpackFile(tarfile)
    os.chdir( root_sys )
    os.system( "./configure --enable-soversion --enable-fftw3 --enable-mysql" )
    Make()

  print "="*48
  print "ROOT installation should be complete.  Add the following to your .tcshrc or .bashrc:"
  print "	- Define ROOTSYS as %s" % root_sys
  print "	- Add $ROOTSYS/bin to PATH"
  print "	- Add $ROOTSYS/lib to LD_LIBRARY_PATH"



#######################
#Install etSpy
#######################
def InstallETSpy(prefix):
  print "Installing etSpy at %s" % (prefix)

  with pushd(prefix, createNew=True):
    tarfile = "https://cdcvs.fnal.gov/redmine/attachments/download/25031/etSpy_2015-4-30.tar.gz"
    GetFile(tarfile)
    UnpackFile(tarfile)
    os.chdir( "etSpy" )
    Make(allow_multicore=False)

  print "="*48
  print "etSpy installation should be complete."


#######################
#Install CODA
#######################
def InstallCODA( prefix ):
  print "Installing CODA at /usr/local (you may have to adjust permissions or run as root)..."
  prefix = "/usr/local"
# note that this is always installed at /usr/local.
# it doesn't HAVE to be there may ROC and CODA config files expect it to be there, so it is easier this way.

  with pushd(prefix, createNew=True):
    tarfile = "https://cdcvs.fnal.gov/redmine/attachments/download/25035/coda_scalerDAQ_2015-04-30.tar.gz"
    GetFile(tarfile)
    UnpackFile(tarfile)
    os.system( "chown -R e906daq:e906daq coda" )
    os.system( "chmod -R 777 coda" )

#likewise, we put the tftpboot files in ~e906daq because that's where config scripts like them
  print "Not installing tftpboot files in e906daq's home area"
  with pushd("/home/e906daq", createNew):
    tarfile = "https://cdcvs.fnal.gov/redmine/attachments/download/25041/tftpboot_2015-04-30.tar.gz"
    GetFile(tarfile)
    UnpackFile(tarfile)
    os.system( "chown -R e906daq:e906daq tftpboot" )
    os.system( "chmod -R 777 tftpboot" )

  print "="*48
  print "CODA installation should be complete.  Make the following changes:"
  print "Change host instances to this hostname by changing...."
  print "	- <containerHost> instances in clientRegistration.xml"
  print "	- platformHost and dbUrl in setup.xml"
  print "	(both files in %s/coda/2.6.1/afecs/cool/e906sc" % prefix
  print "Change host instances in relevant files in ~e906daq/tftpboot/roc*.boot:"
  print "	- hostAdd should use the correct hostname and ip address"
  print "	- putenv \"MYSQL_TCP_HOST\" should use the correct hostname"
  print "Edit ~/dosetupcoda261 to define CODA as '%s/coda/2.6.1'" % prefix


#######################
##
# Main
##
#######################

usage = """Usage: %prog [options] [pacakge]

Install the selected packages in the target location.
You can only install ony package at a time because there are some instructions for you to follow at the end of installation.
"""
parser = optparse.OptionParser( usage=usage )

optgroup = optparse.OptionGroup( parser, "Options" )
optgroup.add_option( "--prefix", dest="prefix", default="/software", help="Packages will be installed with this prefix [default=%default]" )
parser.add_option_group(optgroup)

packgroup = optparse.OptionGroup( parser, "Packages (pick only one)" )
packgroup.add_option( "--root", dest="root", default=False, action="store_true", help="Install ROOT" )
packgroup.add_option( "--etSpy", dest="etSpy", default=False, action="store_true", help="Install etSpy" )
packgroup.add_option( "--coda", dest="coda", default=False, action="store_true", help="Install CODA" )
parser.add_option_group(packgroup)

if len(sys.argv) < 2:
  parser.parse_args( ["--help"] )

(opts, args) = parser.parse_args(sys.argv)

n_packs = opts.root + opts.etSpy + opts.coda
if n_packs !=1:
  sys.exit( "ERROR: You must select exactly 1 package" )

if opts.root:
  InstallROOT( prefix = opts.prefix )

if opts.etSpy:
  InstallETSpy( prefix = opts.prefix )

if opts.coda:
  InstallCODA( prefix = opts.prefix )
