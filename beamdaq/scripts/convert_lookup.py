#!/usr/bin/env python
#####################
# Converts a text file holding lines of "qieVal convertedVal" into a C array
#####################

import os, sys

arrayName = "QIE_Conversion_Map_fC"   #name used for the output object/file
inputFile = "lookup_table_fC.dat"     #name of the input file
meaning   = "QIE bin to linearized fC" #description of the conversion
dataType  = "float"         #type converted to
nBins     = 256                    #number of qie bins, this should never changed

vals = []
for line in open(inputFile):
  line = line.strip()
  key,val = line.split()
  vals.append(val)

if len(vals) != nBins:
  sys.exit( "ERROR - Should have %d values but found %d" % ( nBins, len(vals) ) )

fout = open( "%(arrayName)s.cc" % locals(), "w" )
fout.write( "///%(meaning)s\n" % locals() )
fout.write( "const %(dataType)s %(arrayName)s[%(nBins)d] = \n" % locals() )
fout.write( "{\n" )
for ibin in range(0,nBins):
  val = vals[ibin]

  #omit comma from the last
  if ibin == nBins-1:
    fout.write( "  %(val)s //idx=%(ibin)d\n" % locals() )
  else:
    fout.write( "  %(val)s, //idx=%(ibin)d\n" % locals() )

fout.write( "}; //end of definition of %(arrayName)s\n" % locals() )
fout.close()
