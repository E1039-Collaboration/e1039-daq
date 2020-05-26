#
# Description:  Makefile for SIS 3300 Flash ADC library
#   This driver is specific to VxWorks BSPs and must be compiled
#   with access to vxWorks headers.
#

#Check Operating system we are using
ifndef OSNAME
  OSNAME := $(subst -,_,$(shell uname))
endif

ifndef ARCH
  ARCH = VXWORKSPPC
endif

ifeq ($(OSNAME),SunOS)
LIBDIR = $(CODA)/$(ARCH)/lib

ifeq ($(ARCH),VXWORKSPPC)
INCDIR=$(WIND_BASE)/target/h
CC = ccppc
LD = ldppc
DEFS = -mcpu=604 -DCPU=PPC604 -DVXWORKS -D_GNU_TOOL -DVXWORKSPPC
INCS = -Wall -fno-for-scope -fno-builtin -fvolatile -fstrength-reduce -mlongcall -I. -I$(INCDIR)

endif

CFLAGS = -O $(DEFS) $(INCS)

endif


all: echoarch sis3610Lib.o

sis3610Lib.o: sis3610Lib.c sis3610.h
	$(CC) -c $(CFLAGS) -o $@ sis3610Lib.c

clean:
	rm -f *.o

echoarch:
	echo "Make for $(ARCH)"
