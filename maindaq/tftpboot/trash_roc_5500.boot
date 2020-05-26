# Boot file for CODA ROC 2.6.1
# removed unnecessary garbage
# trashman 
# Noah Wuerfel nwuerfel@umich.edu goblu
# ~ AP AP AP AP ~
# PowerPC version

loginUserAdd "e906daq"
# Add route to outside world (from 29 subnet to 120 subnet)
mRouteAdd("224.0.0.0","192.168.24.38",0xf0000000,0,0)
#where the subnet 129.57.29.0 should be replaced by your local subnet.


# Load host table
#< /daqfs/home/abbottd/VXKERN/vxhosts.boot
#set up names for daq hosts
hostAdd "e1039testdaq.fnal.gov","192.168.24.200"
hostAdd "e1039testdaq","192.168.24.200"


# Setup environment to load coda_roc
putenv "MSQL_TCP_HOST=e1039testdaq.fnal.gov"
putenv "EXPID=e1039test"
putenv "TCL_LIBRARY=/usr/local/coda/2.6.1/common/lib/tcl7.4"
putenv "ITCL_LIBRARY=/usr/local/coda/2.6.1/common/lib/itcl2.0"
putenv "DP_LIBRARY=/usr/local/coda/2.6.1/common/lib/dp"
putenv "SESSION=1039test"

# Load Universe DMA Library
ld< /usr/local/coda/2.6.1/extensions/universeDma/universeDma.o.mv5100
#initialize (no interrupts)
sysVmeDmaInit(1) 
# Set for 64bit PCI transfers
sysVmeDmaSet(4,1)
# A32 VME Slave
sysVmeDmaSet(11,2)
# D32(3) or  BLK32 (4) or MBLK(64) (5) VME transfers
sysVmeDmaSet(12,4)
#VxTick for 10 millsec pr tick
sysClkRateSet(100)

# Load SIS3610 QDC library
ld< /usr/local/coda/2.6.1/extensions/e906/sis3610Lib.o
s3610Init(0x2800,0x1000,1)

# Load trigger interface utilities
ld < /usr/local/coda/2.6.1/VXWORKSPPC55/lib/tsUtil.o
ld < /usr/local/coda/2.6.1/extensions/vmeIntLib/vmeIntLib.o

# Load TDC library
ld< /usr/local/coda/2.6.1/extensions/e906/dsTDC.o
ld< /usr/local/coda/2.6.1/extensions/e906/DslTdc.o
ld< /usr/local/coda/2.6.1/extensions/e906/CR_Read.o
ld< /usr/local/coda/2.6.1/extensions/e906/SL_ScalerLatcher.o

# Load cMsg Stuff
cd "/usr/local/coda/2.6.1/cMsg/vxworks-ppc"
ld< lib/libcmsgRegex.o
ld< lib/libcmsg.o

cd "/usr/local/coda/2.6.1/VXWORKSPPC55/bin"
ld< v1495.o

ld< /home/e906daq/tftpboot/coda_roc_rc3.6

cd "/home/e906daq/noahKnowsBest/vme_workdir"
ld< /home/e906daq/noahKnowsBest/trash_drive/lib/v1495trashLib.o
ld< /home/e906daq/noahKnowsBest/trash_drive/lib/v1495testBench.o
ld< /home/e906daq/noahKnowsBest/trash_drive/lib/testFun.o
