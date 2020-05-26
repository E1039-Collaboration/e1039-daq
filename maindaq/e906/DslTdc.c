/******************************************************************************
*
*  dsLatchCard -  Driver library for operation of DaShung's I/O Latch
*                 using a VxWorks 5.4 or later based Single Board computer. 
*
*  Author: David Abbott 
*          Jefferson Lab Data Acquisition Group
*          April 2003
*
*  Revision  1.0 - Initial Revision
*
*  Jia-Ye Chen, July 2010
*  Revision  03.0 - New version for Da-Shung CR board  
*  Jia-Ye Chen, January 2011
*  Revision  04.0 - New version for Da-Shung CR board version 2
*
*  Changed by Jia-Ye's new code fix the timing problem
*
*  Grass Wang, June 2011
*  Revision  Latch-TDC - New version for Da-Shung CR board Latch-TDC version 
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
#include "DslTdc.h"

/* Prototypes */
void dsTdcInt (void);
void dsTdcFFEnable(int id, int val);
void dsTdcFFDisable(int id, int val);
void dsTdcFFReset(int id, int val);
void grassWriteFile (char* filename, char* InputWord);
void dsTdcASDQ(int id, int iruns, int iOR, int iclock);
/* Define external Functions */
IMPORT STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT STATUS intDisconnect(int);
IMPORT STATUS sysIntEnable(int);
IMPORT STATUS sysIntDisable(int);
IMPORT STATUS sysVmeDmaSend(UINT32, UINT32, int, BOOL);

/* Define Interrupts variables */
/* == running flag == */
BOOL              dsTdcIntRunning  = FALSE;
/* == id number of ADC generating interrupts == */
int               dsTdcIntID       = -1;
/* == user interrupt service routine == */
LOCAL VOIDFUNCPTR dsTdcIntRoutine  = NULL;
/* == arg to user routine == */
LOCAL int         dsTdcIntArg      = 0;
/* == Number of Events to generate Interrupt == */
LOCAL int         dsTdcIntEvCount  = 0;
/* == default VME interrupt level == */
LOCAL UINT32      dsTdcIntLevel    = DSTDC_VME_INT_LEVEL;
/* == default interrupt Vector == */
LOCAL UINT32      dsTdcIntVec      = DSTDC_INT_VEC;

/* Define global variables */
/* == pointers to ADC memory map == */
volatile struct dsTdc2_data *dsTdcd[DSTDC_MAX_BOARDS];
/* == pointers to ADC memory map == */
volatile struct dsTdc2_struct *dsTdcp[DSTDC_MAX_BOARDS];
/* == Number of ADCs in Crate == */
int NdsTdc = 0;
/* == Count of interrupts from ADC == */
int dsTdcIntCount = 0;
/* == Count of Events taken by ADC (Event Count Register value) == */
int dsTdcEventCount[DSTDC_MAX_BOARDS];
/* == Count of events read from specified ADC == */
int dsTdcEvtReadCnt[DSTDC_MAX_BOARDS];
int dsTdcRunFlag = 0;
/* == Pointer to local memory for histogram data == */
int *dsTdcHistData = 0;
/* == Semephore for Task syncronization == */
SEM_ID dsTdcSem;

/*******************************************************************************
*
* dsTdcInit - Initialize SIS 3600 Library. 
*
* RETURNS: OK, or ERROR if the address is invalid or board is not present.
*
*******************************************************************************/

STATUS dsTdcInit (UINT32 addr, UINT32 addr_inc, int nmod)
{
  int ii, res, resd, errFlag=0;
  int icheck=0;
  unsigned int laddr;
  unsigned int addrd;
  unsigned int laddrd;

  addrd = addr + 0x100; 

  if ( icheck!=0 ){
    printf("dsTdcInit check 1-1 : addr     = %d\n",addr);
    printf("dsTdcInit check 1-2 : addr_inc = %d\n",addr_inc);
    printf("dsTdcInit check 1-3 : addrd    = %d\n",addrd);
  }

  /* Check for valid address */
  if ( addr==0 ){
    printf("dsTdcInit: ERROR: Must specify a Bus (VME-based A16) address for the DSTDC\n");
    return(ERROR);
  } else if( addr>0xffffffff ){ /* A32 Addressing */
    printf("dsTdcInit: ERROR: A32/A24 Addressing not supported for the SIS 3600\n");
    return(ERROR);
  } else { 

    /* assume only one ADC to initialize */
    if ( (addr_inc==0)||(nmod==0) ) nmod = 1;
    
    /* get the ADC address (09=original, 0d=blt) */
    /* 0x09 is A32 non privileged data access    */
    res = sysBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr);
    if ( icheck!=0 ) printf("dsTdcInit check 1-4 :  res = %d\n",res);

    resd = sysBusToLocalAdrs(0x09,(char *)addrd,(char **)&laddrd);
    if ( icheck!=0 ) printf("dsTdcInit check 1-5 : resd = %d\n",resd);

    if ( res!=0 ){
      printf("dsTdcInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",addr);
      printf("dsTdcInit: ERROR res=%d\n",res);
      return(ERROR);
    }

    if ( resd!=0 ){
      printf("dsTdcInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddrd) \n",addrd);
      printf("dsTdcInit: ERROR resd=%d\n",resd);
      return(ERROR);
    }
  } /* --- IF addr --- */

  NdsTdc=0;
  for ( ii=0; ii<nmod; ii++ ){
    dsTdcp[ii] = (struct dsTdc2_struct *)( laddr  + ii*addr_inc );
    dsTdcd[ii] = (struct dsTdc2_data *)(   laddrd + ii*addr_inc );

    NdsTdc++;
    printf("Initialized DSTDC ID %d at address dsTdcp 0x%08x \n"
	   ,ii,(UINT32) dsTdcp[ii]);
  }

  /* Disable/Clear all I/O Modules */
  for ( ii=0; ii<NdsTdc; ii++ ){
    dsTdcp[ii]->reset=1;
  }
  
  /* Initialize Interrupt variables */
  dsTdcIntID      = -1;
  dsTdcIntRunning = FALSE;
  dsTdcIntLevel   = DSTDC_VME_INT_LEVEL;
  dsTdcIntVec     = DSTDC_INT_VEC;
  dsTdcIntRoutine = NULL;
  dsTdcIntArg     = 0;
  dsTdcIntEvCount = 0;

  if ( errFlag>0 ){
    printf("dsTdcInit: ERROR: Unable to initialize all Modules\n");

    if ( NdsTdc>0 )
      printf("dsTdcInit: %d DSTDC(s)  successfully initialized\n",NdsTdc);

    return(ERROR);
  } else {
    return(OK);
  } /* --- IF errFlag --- */
}

/* ========================================================================== */
void dsTdcStatus(int id)
{
  unsigned int sreg, sreg2, mid, fifoStatus;

  if ( (id<0) || (dsTdcp[id]==NULL) ){
    printf("dsTdcStatus: ERROR : DSTDC id %d not initialized \n",id);
    return;
  }

  /* --- Get info from Module --- */
  sreg       = dsTdcp[id]->csr;
  sreg2      = dsTdcp[id]->csr2;
  mid        = dsTdcp[id]->id;
  fifoStatus = dsTdcp[id]->fifoStatusReg;

  printf("\n\n");
  printf("================================================ \n");
  printf("DSTDC STATUS   id %d at base address 0x%x \n",id,(UINT32) dsTdcp[id]);
  printf("DSTDC STATUS data %d at base address 0x%x \n",id,(UINT32) dsTdcd[id]);
  printf("------------------------------------------------ \n");
  printf("dsTdcStatus CSR                   : %x\n",sreg);
  printf("dsTdcStatus CSR2                  : %x\n",sreg2);
  printf("dsTdcStatus PCB number            : %x\n",mid);
  printf("dsTdcStatus FIFO status register  : %x\n",fifoStatus);
  printf("================================================ \n\n\n");
}

/* ========================================================================== */
void dsTdcLedSet(int id)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcSetLED ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  dsTdcp[id]->csr = userLED;
  return;
}

/* ========================================================================== */
void dsTdcLedClear(int id)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcClearLED ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  dsTdcp[id]->csr = fifoTestMode;
  return;
}

/* ========================================================================== */
void dsTdcSetFifoStatus(int id, int iopt, int icheck)
{
  unsigned int csrStatus0, csrStatus1, csrStatus2, csrStatus3;
  unsigned int fifoStatus;

  csrStatus0 = 0;
  csrStatus1 = 0;
  csrStatus2 = 0;
  csrStatus3 = 0;

  if ( icheck==1 ) printf("check1 : %x\n",dsTdcp[id]->csr);
  csrStatus0 = dsTdcp[id]->csr;
  if ( icheck==1 ) printf("check2 : %x\n",csrStatus0);

  if ( icheck==1 ) printf("check3 : %x\n",fifoStatusModeMask);
  csrStatus1 = ( dsTdcp[id]->csr ) & fifoStatusModeMask;
  if ( icheck==1 ) printf("check4 : %x\n",csrStatus1);

  if ( icheck==1 ) printf("check5 : %x\n",csrStatus2);
  if ( iopt==1 ){
    csrStatus2 = csrStatus1 + fifoStatusMode1;  // "00"
  } else if ( iopt==2 ){
    csrStatus2 = csrStatus1 + fifoStatusMode2;  // "01"
  } else if ( iopt==3 ){
    csrStatus2 = csrStatus1 + fifoStatusMode3;  // "10"
  } else if ( iopt==4 ){
    csrStatus2 = csrStatus1 + fifoStatusMode4;  // "11"
  }

  if ( icheck==1 ) printf("check6 : %x\n",csrStatus2);
  dsTdcp[id]->csr = csrStatus2;
  if ( icheck==1 ) printf("check7 : %x\n",dsTdcp[id]->csr);

  csrStatus3 = dsTdcp[id]->csr;
  printf("FIFO Status Mode : %x\n",csrStatus3);

  fifoStatus = dsTdcp[id]->fifoStatusReg;
  printf("FIFO Status      : %x\n",fifoStatus);
}

/* ========================================================================== */
void dsTdcFifoTestEnable(int id)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcFifoTest ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  dsTdcp[id]->fifoTestEnable  = 1;
}

/* ========================================================================== */
void dsTdcFifoTestDisable(int id)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcFifoTest ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  dsTdcp[id]->fifoTestDisable = 1;
}

/* ========================================================================== */
void dsTdcFifoClear(int id)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcFifoClear ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  dsTdcp[id]->clr = 1;
}

/* ========================================================================== */
void dsTdcTrigEnable(int id)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcTrigSwitch ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  dsTdcp[id]->trigEnable  = 1;
}

/* ========================================================================== */
void dsTdcTrigDisable(int id)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcTrigSwitch ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  dsTdcp[id]->trigDisable = 1;
}

/* ========================================================================== */
void dsTdcReset(int id)
{
  if ( ( id<0 )||( dsTdcp[id]==NULL ) ){
    logMsg("dsTdcReset: ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  dsTdcp[id]->reset = 1;
}

/* ========================================================================== */
void dsTdcStrobeMode(int id)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcModeSwitch ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  dsTdcp[id]->csr = 0x0000;
  return;
}

/* ========================================================================== */
void dsTdcEdgeMode(int id, int iopt)
{
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcModeSwitch ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  if ( iopt==1 ){
    dsTdcp[id]->csr = onEdgeMode1;  // fifoStatus = "00"
  } else if ( iopt==2 ){
    dsTdcp[id]->csr = onEdgeMode2;  // fifoStatus = "01"
  } else if ( iopt==3 ){
    dsTdcp[id]->csr = onEdgeMode3;  // fifoStatus = "10"
  } else if ( iopt==4 ){
    dsTdcp[id]->csr = onEdgeMode4;  // fifoStatus = "11"
  }

  return;
}

/* ========================================================================== */
int dsTdcFifoDummyRead(int id)
{
  int icycle=0;

  unsigned int res1[8], res2[8];

  int iCapture=0;
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcReadFifo ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }

  for (icycle=0; icycle<8; icycle++ ){
    res1[icycle] = 0;
    res2[icycle] = 0;
  }

  // printf("=== Fifo Read ===\n");

  printf("0x ");
  // for (icycle=0; icycle<8; icycle++ ){
  for (icycle=0; icycle<8; icycle++ ){

    // res1[icycle] = ( dsTdcd[id]->data[icycle] ) & 0xffffffff;  /* --- CH 01~32 --- */
    // res1[icycle] = ( dsTdcd[id]->data[0] ) & 0xffffffff;  /* --- CH 01~32 --- */

    res1[icycle] = ( dsTdcd[id]->data[0] );  /* --- CH 01~32 --- */

    // res2[icycle] = ( dsTdcd[id]->data[0] ) & 0xffffffff;  /* --- CH 01~32 --- */
    // printf("Ch %2d~%2d = 0x %8x\n",(icycle*8)+7,(icycle*8),res1[icycle]);
    // printf("part %d = 0x %8x ",icycle+1,res1[icycle]);
    // printf("%8x %8x ",res1[icycle],res2[icycle]);

    printf("%08x ",res1[icycle]);  // change

    if ( ( res1[icycle]!=0xffffffff ) && ( res1[icycle]>0 ) ) iCapture=1;

  } // Loop icycle

  return iCapture;
}


/* ========================================================================== */
int dsTdcFifoBLKRead(int id)
{
  int icycle=0;
  unsigned int res1[8], res2[8];
  int iCapture=0;
  int DataSize=8;
  UINT32 *mydata=NULL,*mydataloop=NULL;

  mydata = malloc(DataSize);   
  bzero(mydata,DataSize);    

  sysVmeDmaSend(mydata,dsTdcd[id]->data[0],DataSize,0);  

  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcReadFifo ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }
  printf("0x ");
  for (icycle=0; icycle<8; icycle++ ){
    mydataloop=mydata+1;
    res1[icycle]=*mydataloop;

    printf("%08x ",res1[icycle]);  // change
    
    if ( ( res1[icycle]!=0xffffffff ) && ( res1[icycle]>0 ) ) iCapture=1;

  }

  // printf("=== Fifo Read ===\n");


  return iCapture;
}

/* ========================================================================== */
void dsTdcFifoRead(int id, int numEvents)
{
  int ii=0;
  unsigned int res1[64], res2[64];

  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcReadFifo ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  if ( numEvents >=64 ){
    logMsg("dsTdcReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return;
  }

  printf("=== Fifo Read ===\n");
  for ( ii=0; ii<numEvents; ii++ ){
    res1[ii] = ( dsTdcd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( dsTdcd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */
    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);
  }
  printf("\n");
  return;
}

/* ========================================================================== */
int dsTdcFifoRead2(int id, int numEvents)
{
  int ii=0, checkValue=0;
  unsigned int res1[64], res2[64];

  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcReadFifo ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }

  if ( numEvents >=64 ){
    logMsg("dsTdcReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return 0;
  }

  printf("=== Fifo Read ===\n");
  for ( ii=0; ii<numEvents; ii++ ){
    res1[ii] = ( dsTdcd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( dsTdcd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */
    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);

    if ( ( res1[ii] == 0xfeff7fff ) && ( res2[ii] == 0xff7ffffe ) ){
      checkValue = checkValue + 1;
    } // IF res1 ... and res2 ...
  }

  // printf("\n");

  if ( checkValue > 4 ) return 1;
  else                  return 0;

}

/* ========================================================================== */
int dsTdcFifoReadASDQ(int id, int numEvents)
{
  int ii=0, checkValue=0;
  unsigned int res1[64], res2[64];

  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcReadFifo ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }

  if ( numEvents >=64 ){
    logMsg("dsTdcReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return 0;
  }

  printf("=== Fifo Read ===\n");
  for ( ii=0; ii<numEvents; ii++ ){
    res1[ii] = ( dsTdcd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( dsTdcd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */
    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);

    if ( ( res1[ii] == 0x55555555 ) && ( res2[ii] == 0x55555555 ) ){
      checkValue = checkValue + 1;
    } // IF res1 ... and res2 ...
  }

  // printf("\n");

  if ( checkValue > 4 ) return 1;
  else                  return 0;

}


/********************************************************************************/
int dsTdcFifoReadHodo(int id, int numEvents)
{
  int ii=0, checkValue=0,iRepeat=0;
  unsigned int res1[64], res2[64],res1old=0,res2old=0;

  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcReadFifo ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }

  if ( numEvents >=64 ){
    logMsg("dsTdcReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return 0;
  }

  printf("=== Fifo Read ===\n");
  
  for ( ii=0; ii<numEvents; ii++ ){
    
    res1[ii] = ( dsTdcd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( dsTdcd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */

    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);

    if (  ( res1[ii] == 0xffffffff ) && ( res2[ii] == 0xffffffff ) ){

    } else {
      if (  ( (res1[ii]==res1old)&&(res1[ii]!=0) ) ||( (res2[ii]==res2old)&&(res2[ii]!=0) )  ){
	iRepeat=iRepeat+1;
      }      
      if(  ( (res1[ii]>=1)&&(res1[ii]!=0xffffffff) )||( (res2[ii]>=1)&&(res2[ii]!=0xffffffff) )  ){
	checkValue = checkValue + 1;
      }
    } // IF res1 +res2 ...
    
    res1old=res1[ii];
    res2old=res2[ii];
  }//for numevents

  printf("iRepeat=%d\n",iRepeat);

  if (( checkValue > (numEvents-3))&&   (iRepeat<numEvents-1)) return 1;
  else                                                         return 0;
}

int dsTdcFifoReadCh(int id, int numEvents, int iCh)
{
  int ii=0, checkValue=0, iiCh=0;
  unsigned int res1[64], res2[64];
  unsigned int resCheck;
  if ( (id<0) || (dsTdcp[id]==NULL) ){
    logMsg("dsTdcReadFifo ERROR : DSTDCCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }
  
  if ( numEvents >=64 ){
    logMsg("dsTdcReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return 0;
  }
  
  printf("=== Fifo Read ===\n");
  for ( ii=0; ii<numEvents; ii++ ){
    res1[ii] = ( dsTdcd[id]->data[0] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( dsTdcd[id]->data[0] ) & 0xffffffff;  /* --- CH 33~64 --- */
    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);
    if ( ( res1[ii] == 0xffffffff ) || ( res2[ii] == 0xffffffff ) ){
      if (iCh>=32) {
	iiCh=iCh-32;
	if ( res2[ii] == 0xffffffff ) {
	  resCheck=0;
	}else{
	  resCheck=0x1<<iiCh & res2[ii]; 
	}
      }else{
	iiCh=iCh;
	if ( res1[ii] == 0xffffffff ) {
	  resCheck=0;
	}else{
	  resCheck=0x1<<iiCh & res1[ii]; 
	}
      }
      
      if( resCheck>=1){
	checkValue = checkValue + 1;
      }
    }
  }   
    
  if ( checkValue > 1 ) return 1;
  else                  return 0;
}

/* ========================================================================== */
void dsTdcDelayTime(int id, int DelayTime)
{
  dsTdcp[id]->csr2 = DelayTime<<8;

  printf("DelayTime=%d, CSR2 = %x\n",DelayTime,dsTdcp[id]->csr2);
}

/* ========================================================================== */
void dsTdcDummyRunTest(int id, int DelayTime, int iCheck, int isleep)
{
  int  j, iClock, iCapture;

  int TDCon[64];

  // dsTdcEdgeMode(id,2);
  dsTdcDelayTime(id,DelayTime);

  dsTdcFifoClear(id);

  dsTdcTrigEnable(id);
 
  sleep(isleep);

  // for ( i=0; i<16; i++) printf(" "); // not set i<5, must be > 10
  // printf("\n");
  
  dsTdcTrigDisable(id);

  for ( iClock=0; iClock<64; iClock++ ){

    TDCon[iClock]=0;

    printf("\nT %2d ",iClock+1);

    iCapture=dsTdcFifoDummyRead(id);

    if ( iCapture==1 ){
      // printf("iCapture=1!!!!!! Something in TDC in %D clock!\n",DelayTime+iClock);
      TDCon[iClock]=1;
    }

  } // Loop irun

  printf("data position\n");
  for ( j=0; j<64; j++ ){
    if ( j % 10 == 0 ) printf(" ");
    printf("%d",TDCon[j]);
  } // Loop j

  if (iCheck==1){
    for ( j=0; j<64; j++ ){
      if ( j % 10 == 0 ) printf(" ");
      printf("TDCon[%d],%d\n",j,TDCon[j]);
    } // Loop j
    
  }

  printf("\n");
  
  return;
}

/* ========================================================================== */
void dsTdcDummyRunTestPart1(int id, int DelayTime)
{
  dsTdcDelayTime(id,DelayTime);

  dsTdcFifoClear(id);

  dsTdcTrigEnable(id);

}

/* ========================================================================== */
void dsTdcDummyRunTestPart2(int id)
{
  int j, iClock, iCapture;
  int TDCon[64];

  dsTdcTrigDisable(id);

  for ( iClock=0; iClock<64; iClock++ ){

    TDCon[iClock]=0;

    printf("\nT %2d ",iClock+1);

    iCapture=dsTdcFifoDummyRead(id);

    if ( iCapture==1 ) TDCon[iClock]=1;

  } // Loop iClock

  printf("\n\ndata position\n");
  for ( j=0; j<64; j++ ){
    if ( j % 10 == 0 ) printf(" ");
    printf("%d",TDCon[j]);
  } // Loop j

  printf("\n");
  return;
}

/* ========================================================================== */

/* ========================================================================== */
void dsTdcBLKTestPart2(int id)
{
  int j, iClock, iCapture;
  int TDCon[64];

  dsTdcTrigDisable(id);

  for ( iClock=0; iClock<64; iClock++ ){

    TDCon[iClock]=0;

    printf("\nT %2d ",iClock+1);

    iCapture=dsTdcFifoBLKRead(id);

    if ( iCapture==1 ) TDCon[iClock]=1;

  } // Loop iClock

  printf("\n\ndata position\n");
  for ( j=0; j<64; j++ ){
    if ( j % 10 == 0 ) printf(" ");
    printf("%d",TDCon[j]);
  } // Loop j

  printf("\n");
  return;
}

/* ========================================================================== */
void dsTdcPulseDisplay(int id, int DelayTime, int isleep, int iChoose)
{
  int iClock, icycle, ibin, iCh, iRebin;
  int tdcOutput[64][64];
  int pulse[4][64];
  int yield=0, iyield;
  unsigned int output;
  unsigned int output1[8];

#define mask1 0x0000000f
#define mask2 0x000000f0
#define mask3 0x00000f00
#define mask4 0x0000f000
#define mask5 0x000f0000
#define mask6 0x00f00000
#define mask7 0x0f000000
#define mask8 0xf0000000

  dsTdcDelayTime(id,DelayTime);

  dsTdcFifoClear(id);

  dsTdcTrigEnable(id);
 
  sleep(isleep);

  dsTdcTrigDisable(id);

  for ( iClock=0; iClock<64; iClock++ ){

    // printf("\nT %2d ",iClock+1);

    output = 0;
    for (icycle=0; icycle<8; icycle++ ){

      output     = ( dsTdcd[id]->data[0] );
      output1[0] = ( output & mask1 ) % 16;
      output1[1] = ( output & mask2 ) / 16;
      output1[2] = ( output & mask3 ) / 16 / 16;
      output1[3] = ( output & mask4 ) / 16 / 16 / 16;
      output1[4] = ( output & mask5 ) / 16 / 16 / 16 / 16;
      output1[5] = ( output & mask6 ) / 16 / 16 / 16 / 16 / 16;
      output1[6] = ( output & mask7 ) / 16 / 16 / 16 / 16 / 16 / 16;
      output1[7] = ( output & mask8 ) / 16 / 16 / 16 / 16 / 16 / 16 / 16;

      /*
	printf("%08x \n",output);  // change
	printf("%d ",output1[0]);  // change
	printf("%d ",output1[1]);  // change
	printf("%d ",output1[2]);  // change
	printf("%d ",output1[3]);  // change
	printf("%d ",output1[4]);  // change
	printf("%d ",output1[5]);  // change
	printf("%d ",output1[6]);  // change
	printf("%d \n",output1[7]);  // change
      */

      for ( ibin=0; ibin<8; ibin++ ){

	iRebin = ibin + icycle * 8;

	if ( output1[ibin] != 15 ) tdcOutput[iClock][iRebin] = output1[ibin];
	else                       tdcOutput[iClock][iRebin] = 9;

      } // Loop ibin : each TDC bin

    } // Loop icycle

  } // Loop iClock

  /* --- display all channels of all TDC bins --- */
  printf("\n");
  printf("  CH 0        0        1        2        3        4        4        5       \n");
  printf("     1        9        7        5        3        1        9        7       \n");
  printf("----------------------------------------------------------------------------");
  for ( iClock=0; iClock<64; iClock++ ){
    printf("\nT %2d",iClock+1);
    for ( iCh=0; iCh<64; iCh++ ){
      if ( iCh % 8 == 0 ) printf(" ");
      printf("%d",tdcOutput[iClock][iCh]);
    } // Loop iCh
  } // Loop iClock

  /* --- display specific channel --- */
  for ( iClock=0; iClock<64; iClock++ ){
    yield = tdcOutput[iClock][iChoose-1];

    if ( yield == 0 || yield == 9 ){
      pulse[0][iClock] = 0;
      pulse[1][iClock] = 0;
      pulse[2][iClock] = 0;
      pulse[3][iClock] = 0;
    } else if ( yield == 1 ){
      pulse[0][iClock] = 0;
      pulse[1][iClock] = 1;
      pulse[2][iClock] = 2;
      pulse[3][iClock] = 2;
    } else if ( yield == 2 ){
      pulse[0][iClock] = 0;
      pulse[1][iClock] = 0;
      pulse[2][iClock] = 1;
      pulse[3][iClock] = 2;
    } else if ( yield == 3 ){
      pulse[0][iClock] = 0;
      pulse[1][iClock] = 0;
      pulse[2][iClock] = 0;
      pulse[3][iClock] = 1;
    } else if ( yield == 4 ){
      pulse[0][iClock] = 2;
      pulse[1][iClock] = 2;
      pulse[2][iClock] = 2;
      pulse[3][iClock] = 2;
    } else if ( yield == 5 ){
      pulse[0][iClock] = 3;
      pulse[1][iClock] = 0;
      pulse[2][iClock] = 0;
      pulse[3][iClock] = 0;
    } else if ( yield == 6 ){
      pulse[0][iClock] = 2;
      pulse[1][iClock] = 3;
      pulse[2][iClock] = 0;
      pulse[3][iClock] = 0;
    } else if ( yield == 7 ){
      pulse[0][iClock] = 2;
      pulse[1][iClock] = 2;
      pulse[2][iClock] = 3;
      pulse[3][iClock] = 0;
    } // IF yield
  } // Loop iClock

  /* --- display specific channel : idea 1 --- */
  printf("\n\n");
  printf(" ========================================================================\n");
  printf("   << TDC 4-phase output of Channel %2d >>\n\n",iChoose);
  printf("   Pattern Display (1)\n\n");
  printf("       ---------1----1----2----2----3----3----4----4----5----5----6---6\n");
  printf("       ----5----0----5----0----5----0----5----0----5----0----5----0---4");
  for ( iyield=3; iyield>=0; iyield-- ){
    printf("\n   Q-%1d ",iyield);

    for ( iClock=0; iClock<64; iClock++ ){
      if ( pulse[iyield][iClock]==0) printf(".");
      else                           printf("1");
    } // Loop iClock

  } // Loop iyield
  printf("\n\n");

  /* === display specific channel : idea 2 === TDC 01 <-> 16 */
  printf("   Pattern Display (2)\n\n");
  printf("   0---0---0---0---0---0---0---0---0---1---1---1---1---1---1---1---\n");
  printf("   1---2---3---4---5---6---7---8---9---0---1---2---3---4---5---6---\n   ");

  for ( iClock=0; iClock<16; iClock++ ){
    for ( iyield=0; iyield<4; iyield++ ){

      if ( pulse[iyield][iClock]==0){
	printf(".");
      } else if ( pulse[iyield][iClock]==1){
	printf("/");
      } else if ( pulse[iyield][iClock]==2){
	printf("-");
      } else if ( pulse[iyield][iClock]==3){
	printf("\\");
      } // IF pulse[iyield][iClock]

    } // Loop iyield
  } // Loop iClock
  printf("\n\n");

  /* === display specific channel : idea 2 === TDC 17 <-> 32 */
  printf("   1---1---1---2---2---2---2---2---2---2---2---2---2---3---3---3---\n");
  printf("   7---8---9---0---1---2---3---4---5---6---7---8---9---0---1---2---\n   ");

  for ( iClock=16; iClock<32; iClock++ ){
    for ( iyield=0; iyield<4; iyield++ ){

      if ( pulse[iyield][iClock]==0){
	printf(".");
      } else if ( pulse[iyield][iClock]==1){
	printf("/");
      } else if ( pulse[iyield][iClock]==2){
	printf("-");
      } else if ( pulse[iyield][iClock]==3){
	printf("\\");
      } // IF pulse[iyield][iClock]

    } // Loop iyield
  } // Loop iClock
  printf("\n\n");

  /* === display specific channel : idea 2 === TDC 33 <-> 48 */
  printf("   3---3---3---3---3---3---3---4---4---4---4---4---4---4---4---4---\n");
  printf("   3---4---5---6---7---8---9---0---1---2---3---4---5---6---7---8---\n   ");

  for ( iClock=32; iClock<48; iClock++ ){
    for ( iyield=0; iyield<4; iyield++ ){

      if ( pulse[iyield][iClock]==0){
	printf(".");
      } else if ( pulse[iyield][iClock]==1){
	printf("/");
      } else if ( pulse[iyield][iClock]==2){
	printf("-");
      } else if ( pulse[iyield][iClock]==3){
	printf("\\");
      } // IF pulse[iyield][iClock]

    } // Loop iyield
  } // Loop iClock
  printf("\n\n");

  /* === display specific channel : idea 2 === TDC 49 <-> 64 */
  printf("   4---5---5---5---5---5---5---5---5---5---5---6---6---6---6---6---\n");
  printf("   9---0---1---2---3---4---5---6---7---8---9---0---1---2---3---4---\n   ");

  for ( iClock=48; iClock<64; iClock++ ){
    for ( iyield=0; iyield<4; iyield++ ){

      if ( pulse[iyield][iClock]==0){
	printf(".");
      } else if ( pulse[iyield][iClock]==1){
	printf("/");
      } else if ( pulse[iyield][iClock]==2){
	printf("-");
      } else if ( pulse[iyield][iClock]==3){
	printf("\\");
      } // IF pulse[iyield][iClock]

    } // Loop iyield
  } // Loop iClock
  printf("\n\n ========================================================================\n");
  printf("\n");
}

/* ========================================================================== */
void dsTdcORoperation(int id,int  iopt1,int iopt2)
{
  // return;
}

/* ========================================================================== */
void dsTdcRunTestASDQ(int id, int iOR1, int iOR2, int runs, int iCheck)
{
  int i, j, k, irun, iopt1, iopt2, iCapture, ifound;
  int iClock, iClock1, iClock2, iClock3, iClock4, iClock5, iClock6;
  int windowWidth[21] = {1,1,1,1,1,1,2,2,2,2,2,3,3,3,3,4,4,4,5,5,6};
  int windowShift[21] = {0,1,2,3,4,5,0,1,2,3,4,0,1,2,3,0,1,2,0,1,0};
  int or2output[21][64];

  iCapture = 0;
  iClock   = 0;
  iClock1  = 0;
  iClock2  = 0;
  iClock3  = 0;
  iClock4  = 0;
  iClock5  = 0;
  iClock6  = 0;

  dsTdcEdgeMode(id,2);

  /* == check OR searching combination == */
  if ( iOR1==0 && iOR2==0 ){
    printf("\n dsTdcRunTest(int id, int iOR1, int iOR2, int runs, int iCheck)\n\n"
	   "     id : dsTdc card ID, starts from 0\n"
	   "   iOR1 : 21 (standard)\n"
	   "   iOR2 :  0 (standard)\n"
	   "   runs : number of iterations\n"
	   " iCheck : start the check point\n"
	   );
    return;
  }

  /* == reset window == */
  for ( i=0; i<21; i++ ){
    for ( j=0; j<64; j++ ){
      or2output[i][j]=0;
    } // Loop j
  } // Loop i

  /* == number of iteration == */
  for ( irun=1; irun<=runs; irun++ ){

    /* == iOR == */
    for ( iopt1=iOR1; iopt1>iOR2; iopt1-- ){

      if ( iopt1==21 ){
	iClock1 =  0;
	iClock2 = 64;
      } else {
	iClock1 = iClock3;
	iClock2 = iClock4;
      } // IF iopt1==21

      /* == iDelay == */
      iCapture = 0;
      ifound   = 0;
      for ( iopt2=iClock1; iopt2<iClock2; iopt2++ ){

	dsTdcORoperation(id, iopt1, iopt2);
	printf("\nrun %3d iopt1 %2d iopt2 %2d dsTdcStatus CSR2 : %x "
	       "iClock1 %2d iClock2 %2d\n"
	       ,irun,iopt1,iopt2,dsTdcp[id]->csr2,iClock1,iClock2);

	/* --- taking data --- */
	dsTdcTrigEnable(id);
	for ( i=0; i<16; i++) printf(" "); // not set i<5, must be > 10
	printf("\n");
	dsTdcTrigDisable(id);

	iCapture=dsTdcFifoReadASDQ(id,5);
	if ( iCapture==1 ){
	  iClock = iopt2 - 6 + windowShift[iopt1-1];
	  ifound = ifound + 1;

	  if ( iopt1==21){
	    if ( ifound==1 ) iClock3 = iClock + 1;
	    iClock4 = iClock + 7;
	  } // IF iopt1==21

	  iClock5 = iClock + 1;
	  iClock6 = iClock + windowWidth[iopt1-1];

	  if ( iCheck==1 ) printf("iClock %2d iClock5 %2d iClock6 %2d windowWidth %2d\n"
				  ,iClock,iClock5,iClock6,windowWidth[iopt1-1]);

	  for ( k=iClock5; k<=iClock6; k++ ){

	    if ( k>=0 && k<=63 ) or2output[iopt1-1][k] = 1;

	    /* == check point ==*/
	    if ( iCheck==1 ){
	      printf("\niopt1-1 %2d k %2d fire %d\n",iopt1-1,k,or2output[iopt1-1][k]);
	      for ( i=20; i>=iOR2; i-- ){
		printf("\nor-%2d ",i+1);
		for ( j=0; j<64; j++ ){
		  if ( j % 10 == 0 ) printf(" ");
		  printf("%d",or2output[i][j]);
		} // Loop j
		if ( (i==20) || (i==18) || (i==15) || (i==11) || (i==6) ) printf("\n");
	      } // Loop i
	      sleep(1);
	    } // IF iCheck==1

	  } // Loop k

	  if ( iopt1 < 21 ){
	    if ( ( iClock < iClock3 ) || ( iClock > iClock4 ) ){
	      printf("\nRun %3d iopt1 %2d iopt2 %2d\n",irun,iopt1,iopt2);
	      printf("iClock  %2d width %2d\n",iClock,windowWidth[iopt1-1]);
	      printf("iClock1 %2d iClock2 %2d\n",iClock1,iClock2);
	      printf("iClock3 %2d iClock4 %2d\n",iClock3,iClock4);
	      printf("iClock5 %2d iClock6 %2d\n",iClock5,iClock6);
	      printf("\n ???????? ERROR at iopt1 %2d iopt2 %2d??????\n\n"
		     ,iopt1,iopt2);
	      //exit(0);
	    } // IF iopt2 < iClock3 || iopt2 > iClock4
	  } // IF iopt < 21
	} // IF iCapture==1

	dsTdcFifoClear(id);

      } // Loop iopt2
    } // Loop iopt1
  } // Loop irun

  /* == check or2output == */
  for ( i=20; i>=0; i-- ){
    printf("\nor-%2d ",i+1);
    for ( j=0; j<64; j++ ){
      if ( j % 10 == 0 ) printf(" ");
      printf("%d",or2output[i][j]);
    } // Loop j
    if ( (i==20) || (i==18) || (i==15) || (i==11) || (i==6) ) printf("\n");
  } // Loop i
  printf("\n");

  return;
}
/****************************************************************************/
void dsTdcRunTestHodo(int id, int iOR1, int iOR2, int runs, int iCheck)
{
  int i, j, k, irun, iopt1, iopt2, iCapture, ifound;
  int iClock, iClock1, iClock2, iClock3, iClock4, iClock5, iClock6;
  int windowWidth[21] = {1,1,1,1,1,1,2,2,2,2,2,3,3,3,3,4,4,4,5,5,6};
  int windowShift[21] = {0,1,2,3,4,5,0,1,2,3,4,0,1,2,3,0,1,2,0,1,0};
  int or2output[21][64];

  iCapture = 0;
  iClock   = 0;
  iClock1  = 0;
  iClock2  = 0;
  iClock3  = 0;
  iClock4  = 0;
  iClock5  = 0;
  iClock6  = 0;

  dsTdcEdgeMode(id,2);

  /* == check OR searching combination == */
  if ( iOR1==0 && iOR2==0 ){
    printf("\n dsTdcRunTest(int id, int iOR1, int iOR2, int runs, int iCheck)\n\n"
	   "     id : dsTdc card ID, starts from 0\n"
	   "   iOR1 : 21 (standard)\n"
	   "   iOR2 :  0 (standard)\n"
	   "   runs : number of iterations\n"
	   " iCheck : start the check point\n"
	   );
    return;
  }

  /* == reset window == */
  for ( i=0; i<21; i++ ){
    for ( j=0; j<64; j++ ){
      or2output[i][j]=0;
    } // Loop j
  } // Loop i

  /* == number of iteration == */
  for ( irun=1; irun<=runs; irun++ ){

    /* == iOR == */
    for ( iopt1=iOR1; iopt1>iOR2; iopt1-- ){

      if ( iopt1==21 ){
	iClock1 =  0;
	iClock2 = 64;
      } else {
	iClock1 = iClock3;
	iClock2 = iClock4;
      } // IF iopt1==21

      /* == iDelay == */
      iCapture = 0;
      ifound   = 0;
      for ( iopt2=iClock1; iopt2<iClock2; iopt2++ ){

	dsTdcORoperation(id, iopt1, iopt2);
	printf("\nrun %3d iopt1 %2d iopt2 %2d dsTdcStatus CSR2 : %x "
	       "iClock1 %2d iClock2 %2d\n"
	       ,irun,iopt1,iopt2,dsTdcp[id]->csr2,iClock1,iClock2);

	/* --- taking data --- */
	dsTdcTrigEnable(id);
	for ( i=0; i<16; i++) printf(" "); // not set i<5, must be > 10
	printf("\n");
	dsTdcTrigDisable(id);

	iCapture=dsTdcFifoReadHodo(id,5);
	if ( iCapture==1 ){
	  iClock = iopt2 - 6 + windowShift[iopt1-1];
	  ifound = ifound + 1;

	  if ( iopt1==21){
	    if ( ifound==1 ) iClock3 = iClock + 1;
	    iClock4 = iClock + 7;
	  } // IF iopt1==21

	  iClock5 = iClock + 1;
	  iClock6 = iClock + windowWidth[iopt1-1];

	  if ( iCheck==1 ) printf("iClock %2d iClock5 %2d iClock6 %2d windowWidth %2d\n"
				  ,iClock,iClock5,iClock6,windowWidth[iopt1-1]);

	  for ( k=iClock5; k<=iClock6; k++ ){

	    if ( k>=0 && k<=63 ) or2output[iopt1-1][k] = 1;

	    /* == check point ==*/
	    if ( iCheck==1 ){
	      printf("\niopt1-1 %2d k %2d fire %d\n",iopt1-1,k,or2output[iopt1-1][k]);
	      for ( i=20; i>=iOR2; i-- ){
		printf("\nor-%2d ",i+1);
		for ( j=0; j<64; j++ ){
		  if ( j % 10 == 0 ) printf(" ");
		  printf("%d",or2output[i][j]);
		} // Loop j
		if ( (i==20) || (i==18) || (i==15) || (i==11) || (i==6) ) printf("\n");
	      } // Loop i
	      sleep(1);
	    } // IF iCheck==1

	  } // Loop k

	  if ( iopt1 < 21 ){
	    if ( ( iClock < iClock3 ) || ( iClock > iClock4 ) ){
	      printf("\nRun %3d iopt1 %2d iopt2 %2d\n",irun,iopt1,iopt2);
	      printf("iClock  %2d width %2d\n",iClock,windowWidth[iopt1-1]);
	      printf("iClock1 %2d iClock2 %2d\n",iClock1,iClock2);
	      printf("iClock3 %2d iClock4 %2d\n",iClock3,iClock4);
	      printf("iClock5 %2d iClock6 %2d\n",iClock5,iClock6);
	      printf("\n ???????? ERROR at iopt1 %2d iopt2 %2d??????\n\n"
		     ,iopt1,iopt2);
	      //exit(0);
	    } // IF iopt2 < iClock3 || iopt2 > iClock4
	  } // IF iopt < 21
	} // IF iCapture==1

	dsTdcFifoClear(id);

      } // Loop iopt2
    } // Loop iopt1
  } // Loop irun

  /* == check or2output == */
  for ( i=20; i>=0; i-- ){
    printf("\nor-%2d ",i+1);
    for ( j=0; j<64; j++ ){
      if ( j % 10 == 0 ) printf(" ");
      printf("%d",or2output[i][j]);
    } // Loop j
    if ( (i==20) || (i==18) || (i==15) || (i==11) || (i==6) ) printf("\n");
  } // Loop i
  printf("\n");

  return;
}

/* ========================================================================== */
void dsTdcRunTestCh(int id, int iCh, int runs)
{
  int iOR1=0,iOR2=21,iCheck=1;
  int i, j, k, irun, iopt1, iopt2, iCapture, ifound;
  int iClock, iClock1, iClock2, iClock3, iClock4, iClock5, iClock6;
  int windowWidth[21] = {1,1,1,1,1,1,2,2,2,2,2,3,3,3,3,4,4,4,5,5,6};
  int windowShift[21] = {0,1,2,3,4,5,0,1,2,3,4,0,1,2,3,0,1,2,0,1,0};
  int or2output[21][64];

  iCapture = 0;
  iClock   = 0;
  iClock1  = 0;
  iClock2  = 0;
  iClock3  = 0;
  iClock4  = 0;
  iClock5  = 0;
  iClock6  = 0;

  //  dsTdcInit(0x09000000,0x100000,1);
  dsTdcEdgeMode(id,2);

  /* == check OR searching combination == */
  if ( iOR1==0 && iOR2==0 ){
    printf("\n dsTdcRunTest(int id, int iOR1, int iOR2, int runs, int iCheck)\n\n"
	   "     id : dsTdc card ID, starts from 0\n"
	   "   iOR1 : 21 (standard)\n"
	   "   iOR2 :  0 (standard)\n"
	   "   runs : number of iterations\n"
	   " iCheck : start the check point\n"
	   );
    return;
  }

  /* == reset window == */
  for ( i=0; i<21; i++ ){
    for ( j=0; j<64; j++ ){
      or2output[i][j]=0;
    } // Loop j
  } // Loop i

  /* == number of iteration == */
  for ( irun=1; irun<=runs; irun++ ){

    /* == iOR == */
    for ( iopt1=21; iopt1>0; iopt1-- ){

      if ( iopt1==21 ){
	iClock1 =  0;
	iClock2 = 64;
      } else {
	iClock1 = iClock3;
	iClock2 = iClock4;
      } // IF iopt1==21

      /* == iDelay == */
      iCapture = 0;
      ifound   = 0;
      for ( iopt2=iClock1; iopt2<iClock2; iopt2++ ){

	dsTdcORoperation(id, iopt1, iopt2);
	printf("\nrun %3d iopt1 %2d iopt2 %2d dsTdcStatus CSR2 : %x "
	       "iClock1 %2d iClock2 %2d\n"
	       ,irun,iopt1,iopt2,dsTdcp[id]->csr2,iClock1,iClock2);

	/* --- taking data --- */
	dsTdcTrigEnable(id);
	for ( i=0; i<16; i++) printf(" "); // not set i<5, must be > 10
	printf("\n");
	dsTdcTrigDisable(id);

	iCapture=dsTdcFifoReadCh(id,5,iCh);
	if ( iCapture==1 ){
	  iClock = iopt2 - 6 + windowShift[iopt1-1];
	  ifound = ifound + 1;
	  printf("iCapture=1!!!!!!!!!\n");
	  if ( iopt1==21){
	    if ( ifound==1 ) iClock3 = iClock + 1;
	    iClock4 = iClock + 7;
	  } // IF iopt1==21

	  iClock5 = iClock + 1;
	  iClock6 = iClock + windowWidth[iopt1-1];

	  if ( iCheck==1 ) printf("iClock %2d iClock5 %2d iClock6 %2d windowWidth %2d\n"
				  ,iClock,iClock5,iClock6,windowWidth[iopt1-1]);

	  for ( k=iClock5; k<=iClock6; k++ ){
	    if ( k>=0 && k<=63 ) or2output[iopt1-1][k] = 1;
	    /* == check point ==*/
	    if ( iCheck==1 ){
	      printf("\niopt1-1 %2d k %2d fire %d\n",iopt1-1,k,or2output[iopt1-1][k]);
	      for ( i=20; i>=iOR2; i-- ){
		printf("\nor-%2d ",i+1);
		for ( j=0; j<64; j++ ){
		  if ( j % 10 == 0 ) printf(" ");
		  printf("%d",or2output[i][j]);
		} // Loop j
		if ( (i==20) || (i==18) || (i==15) || (i==11) || (i==6) ) printf("\n");
	      } // Loop i
	      sleep(1);
	    } // IF iCheck==1

	  } // Loop k

	  if ( iopt1 < 21 ){
	    if ( ( iClock < iClock3 ) || ( iClock > iClock4 ) ){
	      printf("\nRun %3d iopt1 %2d iopt2 %2d\n",irun,iopt1,iopt2);
	      printf("iClock  %2d width %2d\n",iClock,windowWidth[iopt1-1]);
	      printf("iClock1 %2d iClock2 %2d\n",iClock1,iClock2);
	      printf("iClock3 %2d iClock4 %2d\n",iClock3,iClock4);
	      printf("iClock5 %2d iClock6 %2d\n",iClock5,iClock6);
	      printf("\n ???????? ERROR at iopt1 %2d iopt2 %2d??????\n\n"
		     ,iopt1,iopt2);
	      // exit(0);
	    } // IF iopt2 < iClock3 || iopt2 > iClock4
	  } // IF iopt < 21
	} // IF iCapture==1

	dsTdcFifoClear(id);

      } // Loop iopt2
    } // Loop iopt1
  } // Loop irun

  /* == check or2output == */
  for ( i=20; i>=0; i-- ){
    printf("\nor-%2d ",i+1);
    for ( j=0; j<64; j++ ){
      if ( j % 10 == 0 ) printf(" ");
      printf("%d",or2output[i][j]);
    } // Loop j
    if ( (i==20) || (i==18) || (i==15) || (i==11) || (i==6) ) printf("\n");
  } // Loop i
  printf("\n");
  return;
}

void dsTdcASDQ(int id, int iruns, int iOR, int iclock)
{
  // unsigned int res1[64], res2[64];
  // int i;
  dsTdcReset(id);
 
  dsTdcTrigDisable(id);
  dsTdcFifoClear(id);
  dsTdcEdgeMode(id,2);
  dsTdcORoperation(id,iOR,iclock);
  dsTdcTrigEnable(id);
  sleep(1);
  dsTdcTrigDisable(id);
  dsTdcFifoRead(id,iruns);
}

int dsTdcBroadID(UINT32 addr, UINT32 addr_inc, int id)
{
  int broadid;
  broadid=addr+(addr_inc*id);
  return broadid;
}

int dsTdcFifoStatus(int id){
  int istatus;
  istatus=(dsTdcp[id]->fifoStatusReg)&(fifoStatusMask);
  return istatus;
}
