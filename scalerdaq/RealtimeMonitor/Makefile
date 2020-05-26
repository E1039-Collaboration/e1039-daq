
ROOTCFLAGS    = $(shell root-config --cflags)
ROOTLIBS      = $(shell root-config --glibs)  
ETLIBS = -L/software/etSpy -let
MYOS := $(subst -,,$(shell uname))
MYHOST := $(shell uname -n)

ifeq ($(MYOS),Linux)
  CXXFLAGS      = -fPIC -Wall -I/software/etSpy
  CC = gcc
  CXX = g++
  LD = gcc		
  OPTIM=  -O2 -fexpensive-optimizations -funroll-loops
  CXXFLAGS +=$(ROOTCFLAGS)
  LIBS = $(LIBM) $(ROOTLIBS) $(ETLIBS)
endif

CXXFILES=  E906ScalerDAQ.C E906ET_netclient.C E906FFT.C
CXXOBJ= E906ScalerDAQ.o E906ET_netclient.o E906FFT.o

all:  E906ScalerDAQ E906ET_netclient E906FFT

E906ET_netclient: $(CXXOBJ)
	$(CXX) $(CXXFLAGS)  $(CXXOBJ) $(OPTIM) $(LIBS)

E906ET_netclient.o: E906ET_netclient.h E906ET_globals.h E906ET_netclient.C
	perl -wpi -e 's!.*Makefile.auto.1!  char hostname[]=\"$(MYHOST)\"; //Makefile.auto.1!' E906ET_netclient.C
	$(CXX) $(CXXFLAGS)  -c E906ET_netclient.C -o E906ET_netclient.o

E906ScalerDAQ: $(CXXOBJ)
	$(CXX) $(CXXFLAGS)   $(CXXOBJ) $(OPTIM) $(LIBS)

E906ScalerDAQ.o: E906ET_globals.h E906ScalerDAQ.h E906ScalerDAQ.C

E906FFT: $(CXXOBJ)
	$(CXX) $(CXXFLAGS) -o E906FFT $(CXXOBJ) $(OPTIM) $(LIBS)

E906FFT.o:  E906ET_globals.h E906FFT.C

clean:
	@rm -f $(CXXOBJ) *~  core
cleanall:
	@rm -f $(CXXOBJ) *.out *.root *.log  \
	writecoda_test *~  core


