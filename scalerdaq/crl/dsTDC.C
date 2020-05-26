/******************************************************************************
*
*  dsTDC -  Driver library for operation of DaShung's TDC
*                 using a VxWorks 5.4 or later based Single Board computer. 
*
*  Author: David Abbott 
*          Jefferson Lab Data Acquisition Group
*          April 2003
*
*  Revision  1.0 - Initial Revision
*
*  Grass, July 2010
*  Revision  02.0 - New version for Da-Shung TDC board  
*
*/

#include "vxWorks.h"
#include "stdio.h"
#include "string.h"
#include "logLib.h"
#include "taskLib.h"
#include "intLib.h"
#include "iv.h"
#include "semLib.h"
#include "vxLib.h"
#include "unistd.h"

/* Include definitions */
#include "dsTDC.h"

/* Prototypes */

extern "C" void tdcReset(int id);
extern "C" void tdcStatus (int id);
extern "C" int GetFIFOUnReadPoint(int id);
extern "C" void tdcReadUnReadFIFO(int id);
extern "C" void tdcSetTimeWindow(int id, int val);
extern "C" void tdcResetTDC(int id);
extern "C" void tdcResetTimeCount(int id);
extern "C" void tdcResetFifoReadPoint(int id);
extern "C" void tdcSetStopECL(int id);
extern "C" void tdcSetStopNIM(int id);
extern "C" int tdcReadFIFO(int id, unsigned int val);
extern "C" void tdcReadFIFOTest(int id);
extern "C" void tdcEclEx(int id);
extern "C" void tdcNIMEx(int id);
extern "C" STATUS tdcInit(UINT32 addr, UINT32 addr_inc, int nmod);
extern "C" int CodaGetFIFOUnReadPoint(int id);
extern "C" void tdcEclExDelay(int id, int nsec);
//Void grassWriteFile (char* filename, char* InputWord);

/* Define external Functions */
IMPORT  "C" STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT  STATUS intDisconnect(int);
IMPORT  STATUS sysIntEnable(int);
IMPORT  STATUS sysIntDisable(int);


/* Define global variables */
int Ntdc = 0;                                           /* Number of ADCs in Crate */
volatile struct tdc_data *tdcd[TDC_MAX_BOARDS];     /* pointers to ADC memory map */
volatile struct tdc_struct *tdcp[TDC_MAX_BOARDS];   /* pointers to ADC memory map */




/*******************************************************************************
*
* tdcInit - Initialize DS TDC Library. 
*
* RETURNS: OK, or ERROR if the address is invalid or board is not present.
********************************************************************************/

STATUS tdcInit(UINT32 addr, UINT32 addr_inc, int nmod){
  unsigned int rdatad;
  // int boardID = 0;
  unsigned int ii, res, resd, rdata,  errFlag = 0;
  unsigned int laddr;
  unsigned int addrd;
  unsigned int laddrd;

  addrd=addr+0x800; 

  /* Check for valid address */
  if (addr==0){
    printf("tdcInit: ERROR: Must specify a Bus (VME-based A24) address for the TDC\n");
    return(ERROR);
  } else if(addr > 0xffffff){ /* A24 Addressing */
    printf("tdcInit: ERROR: A32 Addressing not supported for the TDC\n");
    return(ERROR);
  } else { 

    /* assume only one TDC to initialize */
    if ( (addr_inc==0)||(nmod==0) ) nmod = 1;
    
    /* 0x39 is A24 non privileged data access    */
    res = sysBusToLocalAdrs(0x39,(char *)addr,(char **)&laddr);
      
    printf("TDC initialize check 1 ...\n");

    resd = sysBusToLocalAdrs(0x39,(char *)addrd,(char **)&laddrd);
    if ( res!=0 ) {
      printf("tdcInit: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",addr);
      printf("tdcInit: ERROR res=%d\n",res);
      errFlag = 1;
      return(ERROR);
    }

    if ( resd!=0 ) {
      printf("tdcInit: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddrd) \n",addrd);
      printf("tdcInit: ERROR resd=%d\n",resd);
      errFlag = 1;
      return(ERROR);
    }
  } /* --- IF addr --- */

  Ntdc=0;
  for ( ii=0; ii<nmod; ii++ ){

    tdcp[ii] = (struct tdc_struct *)(laddr + ii*addr_inc);
    tdcd[ii] = (struct tdc_data *)(laddrd + ii*addr_inc);
    /* Check if Board exists at that address */
    res = vxMemProbe((char *) &(tdcp[ii]->baseAddr),VX_READ,4,(char *)&rdata);
    if(res < 0) 
	{
	  printf("TDCInit: ERROR1: No addressable board at addr=0x%x\n",(UINT32) tdcp[ii]);
	  tdcp[ii] = NULL;
	  errFlag = 1;
	  break;
	} 

    
    resd = vxMemProbe((char *) &(tdcd[ii]->data[1]),VX_READ,4,(char *)&rdatad);
    if(resd < 0) 
      {
	printf("tdcInit: ERROR2: No addressable board at addr=0x%x\n",(UINT32) tdcd[ii]);
	errFlag = 1;
	break;
      }
    
    Ntdc++;
    printf("Initialized TDC ID %d at address 0x%08x \n",ii,(UINT32) tdcp[ii]);
  }

  /* Disable/Clear all I/O Modules */
  for( ii=0; ii<Ntdc; ii++ ){
    tdcReset(ii);
    printf("TDC initialize check 2 ...\n");
  }
  

  if ( errFlag>0 ){
    printf("tdcInit: ERROR: Unable to initialize all Modules\n");

    if( Ntdc>0 )
      printf("tdcInit: %d TDC(s)  successfully initialized\n",Ntdc);
    return(ERROR);

  } else {
    return(OK);
  }
}


void
tdcStatus (int id)

{

  unsigned int sreg, mid, fifoPoint;
  int timeWindowStatus,numEventStatus,fifoWritePoint,fifoReadPoint;


  if((id<0) || (tdcp[id] == NULL)) {
    printf("tdcStatus: ERROR : TDC id %d not initialized \n",id);
    return;
  }

  /* Get info from Module */
  sreg     = tdcp[id]->csr;
  mid      = tdcp[id]->baseAddr;
  fifoPoint= tdcp[id]->fifoReadWrite;

  timeWindowStatus    = (sreg&TIME_WIN_REG);
  numEventStatus  = (sreg&NUM_OF_EVENT_IN_FIFO)>>24;
  fifoWritePoint  = (fifoPoint&FIFO_WRITE_POINT);
  fifoReadPoint   = (fifoPoint&FIFO_READ_POINT)>>16;
  


  printf("\nSTATUS for TDC id %d at base address 0x%x \n",id,(UINT32) tdcp[id]);
  printf("\nSTATUS for TDC data %d at base address 0x%x \n",id,(UINT32) tdcd[id]);
  printf("------------------------------------------------ \n");
  printf("\n");

  printf("  MODULE   base address = 0x%08x \n",mid);
  printf("\n");

  printf("  STATUS   time window  = 0x%08x or %d\n",timeWindowStatus,timeWindowStatus);
  printf("  STATUS   number of events in FIFO = %d\n",numEventStatus);
  printf("  STATUS   FIFO read Point= %d\n",fifoReadPoint);
  printf("  STATUS   FIFO write Point= %d\n",fifoWritePoint);
  if (fifoPoint==0x8000){
    printf(" Nothing in FIFO!!\n");
  }

}
int GetFIFOUnReadPoint(int id){
  unsigned int fifoPoint;
  //  unsigned int unreadfifopoint=0;
  int unreadfifopoint=0;
  int fifoWritePoint,fifoReadPoint;

  fifoPoint= tdcp[id]->fifoReadWrite;
  fifoWritePoint  = (fifoPoint&FIFO_WRITE_POINT);
  fifoReadPoint   = (fifoPoint&FIFO_READ_POINT)>>16;

  unreadfifopoint=fifoWritePoint-fifoReadPoint;
  if (fifoWritePoint==0x8000 && fifoReadPoint==0){
    printf("Please Enable TDC first\n");
  }else{
    printf("FIFO_READ_POINT=%d,   =0x%x\n",fifoReadPoint,fifoReadPoint);
    printf("FIFO_WRITE_POINT=%d,   =0x%x\n",fifoWritePoint,fifoWritePoint);
    printf("FIFO UnREAD =%d,   =0x%x\n",unreadfifopoint,unreadfifopoint);
  }
  return(unreadfifopoint);
}

int CodaGetFIFOUnReadPoint(int id){
  unsigned int fifoPoint;
  //  unsigned int unreadfifopoint=0;
  int unreadfifopoint=0;
  int fifoWritePoint,fifoReadPoint;

  fifoPoint= tdcp[id]->fifoReadWrite;
  fifoWritePoint  = (fifoPoint&FIFO_WRITE_POINT);
  fifoReadPoint   = (fifoPoint&FIFO_READ_POINT)>>16;
  /*
  if (fifoPoint==0x8000){
    unreadfifopoint=0;
  }else{
    unreadfifopoint=fifoWritePoint-fifoReadPoint;
  }
  */
  unreadfifopoint=fifoWritePoint-fifoReadPoint;

  return(unreadfifopoint);
}

void tdcReadUnReadFIFO(int id){
  int ii=1,tmpNum=0;
  tmpNum=GetFIFOUnReadPoint(id);
  const int unreadfifo=tmpNum;
  unsigned int res[unreadfifo];
  printf("unread fifo= %d",unreadfifo);
  for (ii=1;ii<=unreadfifo;ii++){
    //res[ii] = (tdcd[id]->data[ii])&0xffffffff;
    res[ii] = (tdcd[id]->data[0])&0xffffffff;
    printf ("data[%d]=0x%x\n",ii,res[ii]);
  }
  return;
}
void tdcReset(int id){

  if((id<0) || (tdcp[id] == NULL)) {
    logMsg("tdcReset: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  tdcp[id]->csr = RESET_EVERYTHING;
}

void tdcSetTimeWindow(int id, int val){

  if((id<0) || (tdcp[id] == NULL)) {
    logMsg("tdcReset: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }


  tdcp[id]->csr = ((val&TIME_WIN_REG));
}


void tdcResetTDC(int id){
  int val;
  if((id<0) || (tdcp[id] == NULL)) {
    logMsg("tdcReset: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  val=1;

  tdcp[id]->csr = ((val&0xf)<<24);
}

void tdcResetTimeCount(int id){
  int val;
  if((id<0) || (tdcp[id] == NULL)) {
    logMsg("tdcReset: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  val=1;

  tdcp[id]->csr = ((val&0xf)<<25);
}


void tdcResetFifoReadPoint(int id){
  int val;
  if((id<0) || (tdcp[id] == NULL)) {
    logMsg("tdcReset: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  val=1;
  
  tdcp[id]->csr = ((val&0xf)<<26);
}

void tdcSetStopECL(int id){
  int val;
  if((id<0) || (tdcp[id] == NULL)) {
    logMsg("tdcReset: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  val=1;
  
  tdcp[id]->csr = ((val&0xf)<<30);
}

void tdcSetStopNIM(int id){
  int val;
  if((id<0) || (tdcp[id] == NULL)) {
    logMsg("tdcReset: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  val=0;
  
  tdcp[id]->csr = ((val&0xf)<<30);
}


int tdcReadFIFO(int id, unsigned int val){
  unsigned int res;
  if((id<0) || (tdcp[id] == NULL)) {
    logMsg("tdcReadFIFO: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,\
	   0,0);
    return(ERROR);
  }
  res = (tdcd[id]->data[val])&0xffffffff;

  return(res);
}


/****************************** under this line are all test function *************************************************/
void tdcReadFIFOTest(int id){

  int ii=1;
  unsigned int res[64];

  if((id<0) || (tdcd[id] == NULL)) {
    logMsg("tdcReadFIFOTest: ERROR : TDCCARD id %d not initialized \n",id,0,0,0,\
	   0,0);
    return;
  }

  for (ii=1;ii<64;ii++){
    res[ii] = (tdcd[id]->data[ii])&0xffffffff;
    printf ("data[%d]=0x%x\n",ii,res[ii]);
  }
  return;
}


void tdcEclEx(int id){

  tdcReset(id);
  //tdcSetStopECL(id);
  //tdcSetTimeWindow(id,80000);
  tdcp[id]->csr = 0x40012345;
  return;
}


void tdcEclExDelay(int id, int nsec){
  for(int i=0;i<nsec;i++){
  }
  tdcReset(id);
  //tdcSetStopECL(id);
  //tdcSetTimeWindow(id,80000);
  tdcp[id]->csr = 0x40012345;
  return;
}

void tdcNIMEx(int id){

  tdcReset(id);
  //tdcSetStopECL(id);
  //tdcSetTimeWindow(id,80000);
  tdcp[id]->csr =0x00012345;

  return;
}


void grassWriteFile (char* filename, char* InputWord){
  FILE * pFile;
  pFile = fopen (filename,"w");
  fprintf (pFile, InputWord);
  fclose (pFile);
}
