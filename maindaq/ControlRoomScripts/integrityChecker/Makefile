# Makefile for CODA classes
# R. Michaels, rom@jlab.org, Feb, 2001
# Updated evio library, Oct 19, 2001
#
# Test Executibles:
# tstcoda  --  test of File or ET connection.
# etclient --  test of ET connection for online data.
#
#
# All the root stuff could be discarded (with a little surgery
# on the sources), but I leave it in the Makefile, which was
# taken from another project.
#
# Note regarding software from DAQ group
# evio.h, evio.C, swap_util.C, et.h are taken from DAQ group intact
# (only a few tiny mod's by me).  For this reason I did not rename
# those modules with prefix "THa".
#
# You need an environment variable to define ROOT. E.g.
#   ROOTSYS = /apps/root/2.25-03/root
# User must have LD_LIBRARY_PATH = $ROOTSYS/lib:$LD_LIBRARY_PATH
#
# Use this if compiling online code (ET system)
# User must have LD_LIBRARY_PATH = $CODA/$OSNAME/lib:$LD_LIBRARY_PATH
#export ONLINE = 0

# Use this if profiling (note: it slows down the code)
# export PROFILE = 1

# To make standalone, independent of root CINT macros
export STANDALONE = 1
export OSNAME := $(shell uname)

ifeq ($(OSNAME),SunOS)

   ROOTCFLAGS    = $(shell root-config --cflags)
   ROOTLIBS      = $(shell root-config --libs)
   ROOTGLIBS     = $(shell root-config --glibs)
   CXX           = CC
	 #   CXXFLAGS      = -O  -DSUNVERS -I$(ROOTSYS)/root/include
   CXXFLAGS      = -O  -DSUNVERS
   LD            = CC
   LDFLAGS       = -g -D
   SOFLAGS       = -G
   CXXFLAGS     += $(ROOTCFLAGS)
   EVIO_LIB=libevio.a
   GLIB =  -lm -lc -lgen -lw -lsocket -lnsl -ldl
   SLIB = -L/opt/SUNWspro/SC4.2/lib -lF77 -lM77 -lsunmath

	 # all this needed for ET...  up to comment below
   ET_AC_FLAGS = -D_REENTRANT -D_POSIX_THREAD_SEMANTICS
   ET_CFLAGS = -mt -fast -xO5  $(ET_AC_FLAGS) -DSUNVERS
   CODA = /adaqfs/coda/2.2
	 #   LIBET = $(CODA)/SunOS/lib/libet.so
   LIBET = ./lib/libet.so
   ONLIBS = $(LIBET) -lposix4 -lnsl -lsocket -lresolv
   LIBS = $(EVIO_LIB) $(GLIB)
   ifdef ONLINE
     ALL_LIBS = $(LIBS) $(ONLIBS)
	 # ... finished, what ET needs.
   else
     ALL_LIBS = $(LIBS)
   endif
   ALL_LIBS += $(ROOTLIBS) $(ROOTGLIBS)
endif

ifeq ($(OSNAME),Linux)

   ROOTLIBS      = $(shell root-config --libs)
   ROOTGLIBS     = $(shell root-config --glibs)

   CXX           = g++
   CXXFLAGS      = -Wall -std=c++11 -fno-rtti -fno-exceptions -fPIC \
                   -DLINUXVERS -I$(ROOTSYS)/include -O

	 # Linux with egcs
   INCLUDES      = -I$(ROOTSYS)/include
   CXX           = g++
   CXXFLAGS      = -O -std=c++11 -Wall -fno-exceptions -fPIC $(INCLUDES)
   LD            = g++
   LDFLAGS       = -std=c++11
   SOFLAGS       = -shared

   LIBS          = $(ROOTLIBS)
   GLIBS         = $(ROOTGLIBS) -L/usr/X11R6/lib -lXpm -lX11

   EVIO_LIB=libevio.a
   ALL_LIBS = $(EVIO_LIB) $(GLIBS) $(ROOTLIBS)

   # ONLIBS is needed for ET
   ET_AC_FLAGS = -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS
   ET_CFLAGS = -02 -fPIC $(ET_AC_FLAGS) -DLINUXVERS

   # CODA may be an environment variable.  Typical examples
   #  CODA = /adaqfs/coda/2.2
   #  CODA = /data7/user/coda/2.2
   CODA = /usr/local/coda/2.6

	 #  LIBET = $(CODA)/lib/libet.so
   LIBET = ./lib/libet.so
   ONLIBS = $(LIBET) -lieee -lpthread -ldl -lresolv

   ifdef ONLINE
      ALL_LIBS += $(ONLIBS)
   endif

   ifdef PROFILE
      CXXFLAGS += -pg
   endif
endif

ifeq ($(OSNAME),Darwin)

   ROOTCFLAGS    = $(shell root-config --cflags)
   ROOTLIBS      = $(shell root-config --libs)
   ROOTGLIBS     = $(shell root-config --glibs)

   CXX           = g++

	 # Linux with egcs
   CXX           = g++
   CXXFLAGS      += $(ROOTCFLAGS)
   LD            = g++
   LDFLAGS       =
   SOFLAGS       = -shared

   LIBS          = $(ROOTLIBS)
   GLIBS         = $(ROOTGLIBS) -L/usr/X11R6/lib -lXpm -lX11

   EVIO_LIB=libevio.a
   ALL_LIBS = $(EVIO_LIB) $(GLIBS) $(ROOTLIBS)

   # ONLIBS is needed for ET
   ET_AC_FLAGS = -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS
   ET_CFLAGS = -02 -fPIC $(ET_AC_FLAGS) -DLINUXVERS

   # CODA may be an environment variable.  Typical examples
   #  CODA = /adaqfs/coda/2.2
   #  CODA = /data7/user/coda/2.2
   CODA = /usr/local/coda/2.6

	 #  LIBET = $(CODA)/lib/libet.so
   LIBET = ./lib/libet.so
   ONLIBS = $(LIBET) -lieee -lpthread -ldl -lresolv

   ifdef ONLINE
      ALL_LIBS += $(ONLIBS)
   endif

   ifdef PROFILE
      CXXFLAGS += -pg
   endif
endif


SRC = THaEtClient.C THaCodaFile.C THaCodaData.C
HEAD = $(SRC:.C=.h)
DEPS = $(SRC:.C=.d)
DECODE_OBJS = $(SRC:.C=.o)

ifdef STANDALONE
  CXXFLAGS += -DSTANDALONE
endif

all: integrityChecker libevio.a libcoda.a

integrityChecker: integrityChecker.o THaCodaFile.o THaCodaData.o DslTdc.h THaCodaFile.h THaCodaData.h libevio.a
	$(CXX) $(CXXFLAGS) -o $@ integrityChecker.o THaCodaFile.o THaCodaData.o $(ALL_LIBS)

# Here we build a library with all this stuff
libcoda.a: $(DECODE_OBJS) clean_evio evio.o swap_util.o
	rm -f $@
	ar cr $@ $(DECODE_OBJS) evio.o swap_util.o

# Below is the evio library, which comes rather directly
# from CODA group with minor tweaking by R. Michaels & O. Hansen.
libevio.a: clean_evio evio.o swap_util.o
	rm -f $@
	ar cr $@ evio.o swap_util.o

evio.o: evio.C
	$(CXX) -c  $<

swap_util.o: swap_util.C
	$(CXX) -c  $<

clean:  clean_evio
	rm -f *.o *.a core *~ *.d *.out *.tar etclient tdccoda tstio  integrityChecker

realclean:  clean
	rm -f *.d

clean_evio:
	rm -f evio.o swap_util.o

.SUFFIXES:
.SUFFIXES: .c .cc .cpp .C .o .d

%.o:	%.C
	$(CXX) $(CXXFLAGS) -c $<

%.d:	%.C
	@echo Creating dependencies for $<
	@$(SHELL) -ec '$(CXX) -MM $(CXXFLAGS) -c $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(DEPS)
