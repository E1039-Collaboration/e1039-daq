#!/usr/bin/env python
###################################
# This script checks the size of
# recent coda files.
# It shows:
#   - number of files
#   - total file size
#   - avg file size
# for run*dat in the local coda file area.
#
##
# Nov 16, 2013
# Brian G. Tice
# tice@anl.gov
###################################
import os, time, stat, glob


#define constnats
CODADATA_DIR = "/localdata/codadata/"
DAY_IN_S = 60*60*24
B_TO_GB = 1. / (1024*1024*1024)

files = [ f for f in glob.glob( os.path.join(CODADATA_DIR,"run_*.dat") ) ]
nfiles = len(files)
sum = 0.

sumtoday = 0.
nfilestoday = 0

timenow = time.time()

for f in files:
  fsize = os.path.getsize(f) * B_TO_GB
  age = timenow - os.stat(f)[stat.ST_MTIME]
  sum += fsize

  if age < DAY_IN_S:
    sumtoday += fsize
    nfilestoday += 1

avg = sum / nfiles
avgtoday = sumtoday / nfilestoday

print """
=== Summary of coda files in %(CODADATA_DIR)s ===
TOTAL      - %(nfiles)d files using %(sum).1f GB, avg size %(avg).1f GB
Past 24 hr - %(nfilestoday)d files using %(sumtoday).1f GB, avg size %(avgtoday).1f GB""" % locals()
