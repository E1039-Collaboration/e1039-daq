# Boot file for CODA ROC 2.6.1
# PowerPC version

#loginUserAdd "abbottd","yzzbdbccd"
loginUserAdd "e1039daq"
# Add route to outside world (from 29 subnet to 120 subnet)
mRouteAdd("224.0.0.0","192.168.24.0",0xf0000000,0,0)
#where the subnet 129.57.29.0 should be replaced by your local subnet.


# Load host table
#< /daqfs/home/abbottd/VXKERN/vxhosts.boot
#set up names for daq hosts
hostAdd "e1039daq1.fnal.gov","192.168.24.170"

# Setup environment to load coda_roc
putenv "MSQL_TCP_HOST=e1039daq1.fnal.gov"

putenv "EXPID=e1039"
putenv "TCL_LIBRARY=/usr/local/coda/2.6.1/common/lib/tcl7.4"
putenv "ITCL_LIBRARY=/usr/local/coda/2.6.1/common/lib/itcl2.0"
putenv "DP_LIBRARY=/usr/local/coda/2.6.1/common/lib/dp"
putenv "SESSION=Spin"
#putenv "SESSION=Sea2"
#putenv "SESSION=test1"

# Load ppcTimer Library (set for 133MHz MV6100)
#ld < /daqfs/mizar/home/abbottd/vxWorks/ppcTimer/ppcTimer.o
#ppcTimeBaseFreqSet(133333332)


# Load Tempe DMA Library (for MV6100 CPUs)
#ld< /usr/local/coda/2.6.1/extensions/tempeDma/usrTempeDma.o
# Setup Address and data modes for transfers
#
#  usrVmeDmaConfig(addrType, dataType, sstMode);
#
#  addrType = 0 (A16)    1 (A24)    2 (A32)
#  dataType = 0 (D16)    1 (D32)    2 (BLK32) 3 (MBLK) 4 (2eVME) 5 (2eSST)
#  sstMode  = 0 (SST160) 1 (SST267) 2 (SST320)
#usrVmeDmaConfig(2,2,0)

# Load Universe DMA Library
#ld< /usr/local/coda/2.5/extensions/universeDma/universeDma.o.mv5100
ld< /usr/local/coda/2.6.1/extensions/universeDma/universeDma.o.mv5100
#initialize (no interrupts)
sysVmeDmaInit(1) 
# Set for 64bit PCI transfers
sysVmeDmaSet(4,1)
# A32 VME Slave
sysVmeDmaSet(11,2)
# BLK32 (4) or MBLK(64) (5) VME transfers
sysVmeDmaSet(12,4)


#VxTick for 10 millsec pr tick
sysClkRateSet(100)

# Load SIS3320 QDC library
#ld< /mizar/home/abbottd/vxWorks/sis3300/sis3320Lib.o
#s3320Init(0x08000000,0,1,0)

# Load SIS3610 QDC library
ld< /usr/local/coda/2.6.1/extensions/e906/sis3610Lib.o

#s3610Init(0x2800,0x1000,1)

# Load SIS3600 QDC library
ld< /usr/local/coda/2.6.1/extensions/e906/sis3600Lib.o
#s3600Init(0x11113800,0x1000,1)

# Load trigger interface utilities
ld < /usr/local/coda/2.6.1/VXWORKSPPC55/lib/tsUtil.o
ld < /usr/local/coda/2.6.1/extensions/vmeIntLib/vmeIntLib.o

# Load scaler library
#ld</home/e906daq/coda/2.6.1/extensions/sis3610/scale32Lib.o
#ld</home/e906daq/scale32Lib.o
#ld</home/e906daq/scale32Lib_15.o
#scale32Init(0x08000000,0x10000,2)

# Load v1495 Libraries
#ld</usr/local/coda/2.6.1/extensions/e906/v1495.o
#ld</usr/local/coda/2.6.1/extensions/e906/v1495Lib-2011.o

# Load TDC library
#ld< /usr/local/coda/2.6.1/extensions/e906/dsTDC.o
ld< /data2/e906daq/coda/2.6/extensions/e906/dsTDC.o
ld< /usr/local/coda/2.6.1/extensions/e906/DslTdc.o
ld< /usr/local/coda/2.6.1/extensions/e906/CR_Read.o
ld< /usr/local/coda/2.6.1/extensions/e906/SL_ScalerLatcher.o


# Load Latch card library
# ld< /usr/local/coda/2.6.1/extensions/e906/dsLatchCard2.o
ld< /data2/e906daq/coda/2.6/extensions/e906/dsLatchCard2.o
#latchInit(0x09100000,0x100000,2);

#load For Read Vx File
#ld< /data/slowcontrols/Grass/ReadFileVx/ReadFileVx.o

# Load cMsg Stuff
cd "/usr/local/coda/2.6.1/cMsg/vxworks-ppc"
ld< lib/libcmsgRegex.o
ld< lib/libcmsg.o

cd "/usr/local/coda/2.6.1/VXWORKSPPC55/bin"
ld < coda_roc_rc3.6

# Spawn tasks
taskSpawn ("ROC",200,8,250000,coda_roc,"-i","-s","Spin","-objects","ROC20 ROC")
