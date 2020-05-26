#!/usr/bin/env python
import os, sys, glob

################################
#explanation of directories:
#
#src: .cxx files in src will be compiled and linked into a shared lib
#   we read each file for includes to create dependencies
#
#BeamDAQ: for header files
#
#programs: .cxx files in run will be compiled into executables and any config files
#
#lib: directory will be created for libBeamDAQ.so
#
#bin: for .o and executables


#sniff src files
srcfiles = sorted( glob.glob("src/*.cxx") )

#sniff for programs
progsrc = sorted( glob.glob("programs/*.cxx") )

#for each src file create a .o file
srcobjs  =  [ f.replace(".cxx",".o") for f in srcfiles ]
srcobjs  =  [ f.replace("src/","bin/") for f in srcobjs ]

progobjs  = [ f.replace(".cxx",".o") for f in progsrc ]
progobjs  = [ f.replace("programs/","bin/") for f in progobjs ]
progexecs = [ f.replace(".o","") for f in progobjs ]

allsrc = list(srcfiles)
allsrc.extend(progsrc)

allobjs = list(srcobjs)
allobjs.extend(progobjs)

#make dependency make for all src files
depmap = {}
max_lines = 100 #only search for #includes in this many lines
for srcfile in allsrc:
  deps = []
  nlines = 0
  for line in open(srcfile):
    line = line.strip()

    if nlines == max_lines:
      break
    nlines+=1

    if "#include" in line:
      #get header by splitting on white space
      header = line.split()[1]
      #then removing surrounding "" or <>
      header = header[1:-1]

      #if header file is local, then add it to dependecies
      if os.path.isfile(header) and header not in deps:
        deps.append(header)

  if len(deps) > 0:
    depmap[srcfile] = deps

makefile = open("Makefile","w")
makefile.write("""
#######################################
# Autogenerated by GenerateMakefile.py
#######################################
CXX = g++
CXXFLAGS      = -O3 -Wall -fPIC -I./
LDFLAGS += -shared

ROOTCFLAGS    = $(shell root-config --cflags)
ROOTLIBS      = $(shell root-config --glibs)

CXXFLAGS += $(ROOTCFLAGS)
LDLIBS   += $(ROOTLIBS)

#add boost
LDLIBS   += -lboost_thread-mt -lboost_serialization -lboost_filesystem-mt -lboost_system-mt

""")

makefile.write("TARGETSO = lib/libBeamDAQ.so\n" )
makefile.write("PROGRAMS = %s\n" % " ".join(progexecs) )
makefile.write( "all: $(PROGRAMS)\n\n")

#compile all src files
for i in range( 0, len(allsrc) ):
  src = allsrc[i]
  obj = allobjs[i]
  deps = ""
  if src in depmap.keys():
    deps = " ".join(depmap[src])

  makefile.write("""
%(obj)s: %(src)s %(deps)s
\tif [ ! -d bin ]; then mkdir bin; fi
\t$(CXX) $(CXXFLAGS) -c %(src)s -o %(obj)s

""" % locals() )


#link all src files into the so
srcobjs_str = " ".join(srcobjs)
makefile.write("""
$(TARGETSO): %(srcobjs_str)s
\tif [ ! -d lib ]; then mkdir lib; fi
\t$(CXX) $(LDFLAGS) -o $(TARGETSO) %(srcobjs_str)s $(LDLIBS)

""" % locals() )

#link programs to the so
for i in range(0, len(progexecs) ):
  prog = progexecs[i]
  progobj = progobjs[i]
  makefile.write("""
%(prog)s: $(TARGETSO) %(progobj)s
\t$(CXX) -o %(prog)s %(progobj)s $(CXXFLAGS) -L./lib -lBeamDAQ $(LDLIBS)

""" % locals() )

#clean by removing lib and bin
makefile.write("""
clean:
\t-rm -f lib/* bin/*
""")

makefile.close()