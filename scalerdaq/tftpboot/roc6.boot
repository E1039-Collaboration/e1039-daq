# Boot file for CODA ROC 2.6
# PowerPC version

#loginUserAdd "abbottd","yzzbdbccd"
loginUserAdd "e906daq"
# Add route to outside world (from 29 subnet to 120 subnet)
#mRouteAdd("129.57.120.0","129.57.29.1",0xfffffc00,0,0)

# Load host table
#< /daqfs/home/abbottd/VXKERN/vxhosts.boot
#set up names for daq hosts
hostAdd "e906daq1.fnal.gov","192.168.24.1"

# Setup environment to load coda_roc
putenv "MSQL_TCP_HOST=e906daq1.fnal.gov"

putenv "EXPID=e906"
putenv "TCL_LIBRARY=/usr/local/coda/2.6/common/lib/tcl7.4"
putenv "ITCL_LIBRARY=/usr/local/coda/2.6/common/lib/itcl2.0"
putenv "DP_LIBRARY=/usr/local/coda/2.6/common/lib/dp"
putenv "SESSION=Sea2"
#putenv "SESSION=test1"

# Load ppcTimer Library (set for 133MHz MV6100)
#ld < /daqfs/mizar/home/abbottd/vxWorks/ppcTimer/ppcTimer.o
#ppcTimeBaseFreqSet(133333332)


# Load Tempe DMA Library (for MV6100 CPUs)
#
#  To configure : usrVmeDmaConfig( addrType, dataType)
#     where addrType = 0 (A16)  1 (A24) or 2 (A32)
#           dataType = 0 (D16)  1 (D32)    2 (BLK32)
#                      3 (MBLT) 4 (2eVME)  5 (2eSST)
#
ld< /usr/local/coda/2.6/extensions/tempeDma/usrTempeDma.o
usrVmeDmaConfig(2,2)

# Load Universe DMA Library
#ld< /usr/local/coda/2.5/extensions/universeDma/universeDma.o.mv5100
#initialize (no interrupts)
#sysVmeDmaInit(1) 
# Set for 64bit PCI transfers
#sysVmeDmaSet(4,1)
# A32 VME Slave
#sysVmeDmaSet(11,2)
# BLK32 (4) or MBLK(64) (5) VME transfers
#sysVmeDmaSet(12,4)

sysClkRateSet(100)

# Load SIS3320 QDC library
#ld< /mizar/home/abbottd/vxWorks/sis3300/sis3320Lib.o
#s3320Init(0x08000000,0,1,0)

# Load SIS3610 QDC library
#ld< /daqfs/mizar/home/abbottd/vxWorks/sis3610/sis3610Lib.o
ld< /home/e906daq/coda/2.6/extensions/e906/sis3610Lib.o
#ld< /home/e906daq/coda/2.6/sis3610/sis3610Lib.o
s3610Init(0x2800,0x1000,1)

# Load SIS3600 QDC library
#ld< /usr/local/coda/2.6/sis3610/sis3600Lib.o
#ld< /home/e906daq/coda/2.6/sis3610/sis3600Lib.o
ld< /home/e906daq/coda/2.6/extensions/e906/sis3600Lib.o
s3600Init(0x11113800,0x1000,1)

# Load trigger interface utilities
ld < /home/e906daq/coda/2.6/VXWORKSPPC55/lib/tsUtil.o
ld < /home/e906daq/coda/2.6/extensions/vmeIntLib/vmeIntLib.o

# Load scaler library
#ld</home/e906daq/coda/2.6/extensions/sis3610/scale32Lib.o
#ld</home/e906daq/scale32Lib.o
ld</home/e906daq/scale32Lib_15.o
scale32Init(0x08000000,0x10000,2)

# Load TDC library
ld< /home/e906daq/coda/2.6/extensions/e906/dsTDC.o

# Load Latch card library
ld< /home/e906daq/coda/2.6/extensions/e906/dsLatchCard.o

#load For Read Vx File
#ld< /data/slowcontrols/Grass/ReadFileVx/ReadFileVx.o

# Load cMsg Stuff
cd "/usr/local/coda/2.6/VXWORKSPPC"
ld< lib/libcmsgRegex.so
ld< lib/libcmsg.so

cd "/usr/local/coda/2.6/VXWORKSPPC55/bin"
ld < coda_roc_rc3

# Spawn tasks
#taskSpawn ("ROC",200,8,250000,coda_roc,"-i","-s","SeaQuest","-objects","ROCe906 ROC")
#taskSpawn ("ROC",200,8,250000,coda_roc,"","-s","SeaQuest","-objects","ROCe906 ROC")
taskSpawn ("ROC",200,8,250000,coda_roc,"","-s","Sea2","-objects","ROC6 ROC")