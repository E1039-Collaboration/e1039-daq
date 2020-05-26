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
#include "dsLatchCard2.h"

/* Prototypes */
void latchInt (void);
void latchFFEnable(int id, int val);
void latchFFDisable(int id, int val);
void latchFFReset(int id, int val);
void grassWriteFile (char* filename, char* InputWord);
void latchASDQ(int id, int iruns, int iOR, int iclock);
/* Define external Functions */
IMPORT STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT STATUS intDisconnect(int);
IMPORT STATUS sysIntEnable(int);
IMPORT STATUS sysIntDisable(int);

/* Define Interrupts variables */
/* == running flag == */
BOOL              latchIntRunning  = FALSE;
/* == id number of ADC generating interrupts == */
int               latchIntID       = -1;
/* == user interrupt service routine == */
LOCAL VOIDFUNCPTR latchIntRoutine  = NULL;
/* == arg to user routine == */
LOCAL int         latchIntArg      = 0;
/* == Number of Events to generate Interrupt == */
LOCAL int         latchIntEvCount  = 0;
/* == default VME interrupt level == */
LOCAL UINT32      latchIntLevel    = LATCH_VME_INT_LEVEL;
/* == default interrupt Vector == */
LOCAL UINT32      latchIntVec      = LATCH_INT_VEC;

/* Define global variables */
/* == pointers to ADC memory map == */
volatile struct latch2_data *latchd[LATCH_MAX_BOARDS];
/* == pointers to ADC memory map == */
volatile struct latch2_struct *latchp[LATCH_MAX_BOARDS];
/* == Number of ADCs in Crate == */
int Nlatch = 0;
/* == Count of interrupts from ADC == */
int latchIntCount = 0;
/* == Count of Events taken by ADC (Event Count Register value) == */
int latchEventCount[LATCH_MAX_BOARDS];
/* == Count of events read from specified ADC == */
int latchEvtReadCnt[LATCH_MAX_BOARDS];
int latchRunFlag = 0;
/* == Pointer to local memory for histogram data == */
int *latchHistData = 0;
/* == Semephore for Task syncronization == */
SEM_ID latchSem;

/*******************************************************************************
*
* latchInit - Initialize SIS 3600 Library. 
*
* RETURNS: OK, or ERROR if the address is invalid or board is not present.
*
*******************************************************************************/

STATUS latchInit (UINT32 addr, UINT32 addr_inc, int nmod)
{
  int ii, res, resd, errFlag=0;
  int icheck=0;
  unsigned int laddr;
  unsigned int addrd;
  unsigned int laddrd;

  addrd = addr + 0x100; 

  if ( icheck!=0 ){
    printf("latchInit check 1-1 : addr     = %d\n",addr);
    printf("latchInit check 1-2 : addr_inc = %d\n",addr_inc);
    printf("latchInit check 1-3 : addrd    = %d\n",addrd);
  }

  /* Check for valid address */
  if ( addr==0 ){
    printf("latchInit: ERROR: Must specify a Bus (VME-based A16) address for the LATCH\n");
    return(ERROR);
  } else if( addr>0xffffffff ){ /* A32 Addressing */
    printf("latchInit: ERROR: A32/A24 Addressing not supported for the SIS 3600\n");
    return(ERROR);
  } else { 

    /* assume only one ADC to initialize */
    if ( (addr_inc==0)||(nmod==0) ) nmod = 1;
    
    /* get the ADC address (09=original, 0d=blt) */
    /* 0x09 is A32 non privileged data access    */
    res = sysBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr);
    if ( icheck!=0 ) printf("latchInit check 1-4 :  res = %d\n",res);

    resd = sysBusToLocalAdrs(0x09,(char *)addrd,(char **)&laddrd);
    if ( icheck!=0 ) printf("latchInit check 1-5 : resd = %d\n",resd);

    if ( res!=0 ){
      printf("latchInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",addr);
      printf("latchInit: ERROR res=%d\n",res);
      return(ERROR);
    }

    if ( resd!=0 ){
      printf("latchInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddrd) \n"
	     ,addrd);
      printf("latchInit: ERROR resd=%d\n",resd);
      return(ERROR);
    }
  } /* --- IF addr --- */

  Nlatch=0;
  for ( ii=0; ii<nmod; ii++ ){
    latchp[ii] = (struct latch2_struct *)( laddr  + ii*addr_inc );
    latchd[ii] = (struct latch2_data *)(   laddrd + ii*addr_inc );

    Nlatch++;
    printf("Initialized LATCH ID %d at address latchp 0x%08x \n"
	   ,ii,(UINT32) latchp[ii]);
  }

  /* Disable/Clear all I/O Modules */
  for ( ii=0; ii<Nlatch; ii++ ){
    latchp[ii]->reset=1;
  }
  
  /* Initialize Interrupt variables */
  latchIntID      = -1;
  latchIntRunning = FALSE;
  latchIntLevel   = LATCH_VME_INT_LEVEL;
  latchIntVec     = LATCH_INT_VEC;
  latchIntRoutine = NULL;
  latchIntArg     = 0;
  latchIntEvCount = 0;

  if ( errFlag>0 ){
    printf("latchInit: ERROR: Unable to initialize all Modules\n");

    if ( Nlatch>0 )
      printf("latchInit: %d LATCH(s)  successfully initialized\n",Nlatch);

    return(ERROR);
  } else {
    return(OK);
  } /* --- IF errFlag --- */
}

/* ========================================================================== */
void latchStatus(int id)
{
  unsigned int sreg, sreg2, mid, fifoStatus;

  if ( (id<0) || (latchp[id]==NULL) ){
    printf("latchStatus: ERROR : LATCH id %d not initialized \n",id);
    return;
  }

  /* --- Get info from Module --- */
  sreg       = latchp[id]->csr;
  sreg2      = latchp[id]->csr2;
  mid        = latchp[id]->id;
  fifoStatus = latchp[id]->fifoStatusReg;

  printf("\n\n");
  printf("================================================ \n");
  printf("LATCH STATUS   id %d at base address 0x%x \n",id,(UINT32) latchp[id]);
  printf("LATCH STATUS data %d at base address 0x%x \n",id,(UINT32) latchd[id]);
  printf("------------------------------------------------ \n");
  printf("latchStatus CSR                   : %x\n",sreg);
  printf("latchStatus CSR2                  : %x\n",sreg2);
  printf("latchStatus PCB number            : %x\n",mid);
  printf("latchStatus FIFO status register  : %x\n",fifoStatus);
  printf("================================================ \n\n\n");
}

/* ========================================================================== */
void latchLedSet(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchSetLED ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  latchp[id]->csr = userLED;
  return;
}

/* ========================================================================== */
void latchLedClear(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchClearLED ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  latchp[id]->csr = fifoTestMode;
  return;
}

/* ========================================================================== */
void latchSetFifoStatus(int id, int iopt, int icheck)
{
  unsigned int csrStatus0, csrStatus1, csrStatus2, csrStatus3;
  unsigned int fifoStatus;

  csrStatus0 = 0;
  csrStatus1 = 0;
  csrStatus2 = 0;
  csrStatus3 = 0;

  if ( icheck==1 ) printf("check1 : %x\n",latchp[id]->csr);
  csrStatus0 = latchp[id]->csr;
  if ( icheck==1 ) printf("check2 : %x\n",csrStatus0);

  if ( icheck==1 ) printf("check3 : %x\n",fifoStatusModeMask);
  csrStatus1 = ( latchp[id]->csr ) & fifoStatusModeMask;
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
  latchp[id]->csr = csrStatus2;
  if ( icheck==1 ) printf("check7 : %x\n",latchp[id]->csr);

  csrStatus3 = latchp[id]->csr;
  printf("FIFO Status Mode : %x\n",csrStatus3);

  fifoStatus = latchp[id]->fifoStatusReg;
  printf("FIFO Status      : %x\n",fifoStatus);
}

/* ========================================================================== */
void latchFifoTestEnable(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchFifoTest ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  latchp[id]->fifoTestEnable  = 1;
}

/* ========================================================================== */
void latchFifoTestDisable(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchFifoTest ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  latchp[id]->fifoTestDisable = 1;
}

/* ========================================================================== */
void latchFifoClear(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchFifoClear ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  latchp[id]->clr = 1;
}

/* ========================================================================== */
void latchTrigEnable(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchTrigSwitch ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  latchp[id]->trigEnable  = 1;
}

/* ========================================================================== */
void latchTrigDisable(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchTrigSwitch ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  latchp[id]->trigDisable = 1;
}

/* ========================================================================== */
void latchReset(int id)
{
  if ( ( id<0 )||( latchp[id]==NULL ) ){
    logMsg("latchReset: ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }
  latchp[id]->reset = 1;
}

/* ========================================================================== */
void latchStrobeMode(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchModeSwitch ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  latchp[id]->csr = 0x0000;
  return;
}

/* ========================================================================== */
void latchEdgeMode(int id, int iopt)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchModeSwitch ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  if ( iopt==1 ){
    latchp[id]->csr = onEdgeMode1;  // fifoStatus = "00"
  } else if ( iopt==2 ){
    latchp[id]->csr = onEdgeMode2;  // fifoStatus = "01"
  } else if ( iopt==3 ){
    latchp[id]->csr = onEdgeMode3;  // fifoStatus = "10"
  } else if ( iopt==4 ){
    latchp[id]->csr = onEdgeMode4;  // fifoStatus = "11"
  }

  return;
}

/* ========================================================================== */
void latchFifoRead(int id, int numEvents)
{
  int ii=0;
  unsigned int res1[64], res2[64];

  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchReadFifo ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  if ( numEvents >=64 ){
    logMsg("latchReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return;
  }

  printf("=== Fifo Read ===\n");
  for ( ii=0; ii<numEvents; ii++ ){
    res1[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */
    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);
  }
  printf("\n");


  return;
}

/* ========================================================================== */
int latchFifoRead2(int id, int numEvents)
{
  int ii=0, checkValue=0;
  unsigned int res1[64], res2[64];

  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchReadFifo ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }

  if ( numEvents >=64 ){
    logMsg("latchReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return 0;
  }

  printf("=== Fifo Read ===\n");
  for ( ii=0; ii<numEvents; ii++ ){
    res1[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */
    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);

    if ( ( res1[ii] == 0xfeff7fff ) && ( res2[ii] == 0xff7ffffe ) ){
      checkValue = checkValue + 1;
    } // IF res1 ... and res2 ...
  }

  // printf("\n");

  if ( checkValue > 4 ) return 1;
  else                  return 0;

}
int latchFifoReadGr(int id, int numEvents)
{
  int ii=0, checkValue=0,iRepeat=0;
  unsigned int res1[64], res2[64],res1old[64],res2old[64];

  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchReadFifo ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }

  if ( numEvents >=64 ){
    logMsg("latchReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return 0;
  }

  printf("=== Fifo Read ===\n");
  

  for ( ii=0; ii<numEvents; ii++ ){
    
    res1[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */
    if (res1[ii]==res1old[ii] ||res1[ii]==res1old[ii]){
      iRepeat=iRepeat+1;
    }
      
    res1old[ii]=res1[ii];
    res2old[ii]=res2[ii];
    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);
    if ( ( ( res1[ii] == 0xffffffff ) || ( res2[ii] == 0xffffffff ) )||(res1[ii]!=res2[ii] ) ){
      
    }else {
      
      if( res1[ii]+res2[ii]>=1){
	checkValue = checkValue + 1;
      }
    } // IF res1 +res2 ...
    
  }

  // printf("\n");

  if (( checkValue > 10 )&&   (iRepeat<6)) return 1;
  else                  return 0;

}

int latchFifoReadHodo(int id, int numEvents)
{
  int ii=0, checkValue=0,iRepeat=0;
  unsigned int res1[64], res2[64],res1old[64],res2old[64];

  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchReadFifo ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }

  if ( numEvents >=64 ){
    logMsg("latchReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return 0;
  }

  printf("=== Fifo Read ===\n");
  

  for ( ii=0; ii<numEvents; ii++ ){
    
    res1[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */

      

    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);
    if (  ( res1[ii] == 0xffffffff ) && ( res2[ii] == 0xffffffff ) ){
      
    }else {
      if (  ( (res1[ii]==res1old[ii])&&(res1[ii]!=0) ) ||( (res2[ii]==res2old[ii])&&(res2[ii]!=0) )  ){
	
	iRepeat=iRepeat+1;
      }      
      if(  ( (res1[ii]>=1)&&(res1[ii]!=0xffffffff) )||( (res2[ii]>=1)&&(res2[ii]!=0xffffffff) )  ){
	checkValue = checkValue + 1;
      }
    } // IF res1 +res2 ...
    
    res1old[ii]=res1[ii];
    res2old[ii]=res2[ii];
  }//for numevents

  // printf("\n");

  if (( checkValue > (numEvents-3))&&   (iRepeat<numEvents)) return 1;
  else                  return 0;

}


int latchFifoReadCh(int id, int numEvents, int iCh)
{
  int ii=0, checkValue=0, iiCh=0;
  unsigned int res1[64], res2[64];
  unsigned int resCheck;
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchReadFifo ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return 0;
  }
  
  if ( numEvents >=64 ){
    logMsg("latchReadFifo ERROR : number of events can not greater than or equal to 64\n",id,0,0,0,0,0);
    return 0;
  }
  
  printf("=== Fifo Read ===\n");
  for ( ii=0; ii<numEvents; ii++ ){
    res1[ii] = ( latchd[id]->data[0] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( latchd[id]->data[0] ) & 0xffffffff;  /* --- CH 33~64 --- */
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
  // printf("\n");
    
  if ( checkValue > 1 ) return 1;
  else                  return 0;
  
}


/* ========================================================================== */
void latchORoperation(int id, int iOR, int iDelay)
{
  int          icheck   = 0;
  unsigned int ORwindow = 0; // 4 bits
  unsigned int ORmask   = 0;

  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchORoperation ERROR : LATCHCARD id %d "
	   "not OR operation setting \n",id,0,0,0,0,0);
    return;
  }

  if ( iOR==1 ){
    ORmask = or1ON1;
  } else if ( iOR==2 ){
    ORmask = or1ON2;
  } else if ( iOR==3 ){
    ORmask = or1ON3;
  } else if ( iOR==4 ){
    ORmask = or1ON4;
  } else if ( iOR==5 ){
    ORmask = or1ON5;
  } else if ( iOR==6 ){
    ORmask = or1ON6;
  } else if ( iOR==7 ){
    ORmask = or2ON1;
  } else if ( iOR==8 ){
    ORmask = or2ON2;
  } else if ( iOR==9 ){
    ORmask = or2ON3;
  } else if ( iOR==10 ){
    ORmask = or2ON4;
  } else if ( iOR==11 ){
    ORmask = or2ON5;
  } else if ( iOR==12 ){
    ORmask = or3ON1;
  } else if ( iOR==13 ){
    ORmask = or3ON2;
  } else if ( iOR==14 ){
    ORmask = or3ON3;
  } else if ( iOR==15 ){
    ORmask = or3ON4;
  } else if ( iOR==16 ){
    ORmask = or4ON1;
  } else if ( iOR==17 ){
    ORmask = or4ON2;
  } else if ( iOR==18 ){
    ORmask = or4ON3;
  } else if ( iOR==19 ){
    ORmask = or5ON1;
  } else if ( iOR==20 ){
    ORmask = or5ON2;
  } else if ( iOR==21 ){
    ORmask = or6ON;
  }

  ORwindow = ( iDelay*16*16 + ORmask ) & ( 0xffff );

  if ( icheck==1 ){
    printf("check iOR %d iDelay %d mask %x window %x \n"
	   "CSR2 check1 = %x\n",iOR,iDelay,ORmask,ORwindow,latchp[id]->csr2);
  }

  latchp[id]->csr2 = ORwindow;

  if ( icheck==1 ) printf("CSR2 check2 = %x\n",latchp[id]->csr2);

  return;
}

/* ========================================================================== */
void latchRunTest(int id, int runs)
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

  latchInit(0x09000000,0x100000,1);
  latchEdgeMode(id,2);

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

	latchORoperation(id, iopt1, iopt2);
	printf("\nrun %3d iopt1 %2d iopt2 %2d latchStatus CSR2 : %x\n"
	       ,irun,iopt1,iopt2,latchp[id]->csr2);

	/* --- taking data --- */
	latchTrigEnable(id);
	for ( i=0; i<16; i++) printf(" "); // not set i<5, must be > 10
	printf("\n");
	latchTrigDisable(id);

	iCapture=latchFifoRead2(id,5);
	if ( iCapture==1 ){
	  iClock = iopt2 - 6 + windowShift[iopt1-1];
	  ifound = ifound + 1;

	  if ( iopt1==21){
	    if ( ifound==1 ) iClock3 = iClock;
	    iClock4 = iClock + 5;
	  } // IF iopt1==21

	  iClock5 = iClock;
	  iClock6 = iClock + windowWidth[iopt1-1] -1;
	  for ( k=iClock5; k<=iClock6; k++ ) or2output[iopt1-1][k] = 1;

	  if ( iopt1 < 21 ){
	    if ( ( iClock < iClock3 ) || ( iClock > iClock4 ) ){
	      printf("\nRun %3d iopt1 %2d iopt2 %2d\n",irun,iopt1,iopt2);
	      printf("iClock  %2d width %2d\n",iClock,windowWidth[iopt1-1]);
	      printf("iClock1 %2d iClock2 %2d\n",iClock1,iClock2);
	      printf("iClock3 %2d iClock4 %2d\n",iClock3,iClock4);
	      printf("iClock5 %2d iClock6 %2d\n",iClock5,iClock6);
	      printf("\n ???????? ERROR at iopt1 %2d iopt2 %2d??????\n\n"
		     ,iopt1,iopt2);
	      exit(0);
	    } // IF iopt2 < iClock3 || iopt2 > iClock4
	  } // IF iopt < 21
	} // IF iCapture==1

	latchFifoClear(id);

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
void latchRunTestGr(int id, int runs)
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

  latchInit(0x09000000,0x100000,1);
  latchEdgeMode(id,2);

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

	latchORoperation(id, iopt1, iopt2);
	printf("\nrun %3d iopt1 %2d iopt2 %2d latchStatus CSR2 : %x\n"
	       ,irun,iopt1,iopt2,latchp[id]->csr2);

	/* --- taking data --- */
	latchTrigEnable(id);
	for ( i=0; i<16; i++) printf(" "); // not set i<5, must be > 10
	printf("\n");
	latchTrigDisable(id);

	iCapture=latchFifoReadGr(id,16);
	if ( iCapture==1 ){
	  iClock = iopt2 - 6 + windowShift[iopt1-1];
	  ifound = ifound + 1;

	  if ( iopt1==21){
	    if ( ifound==1 ) iClock3 = iClock;
	    iClock4 = iClock + 5;
	  } // IF iopt1==21

	  iClock5 = iClock;
	  iClock6 = iClock + windowWidth[iopt1-1] -1;
	  for ( k=iClock5; k<=iClock6; k++ ) or2output[iopt1-1][k] = 1;

	  if ( iopt1 < 21 ){
	    if ( ( iClock < iClock3 ) || ( iClock > iClock4 ) ){
	      printf("\nRun %3d iopt1 %2d iopt2 %2d\n",irun,iopt1,iopt2);
	      printf("iClock  %2d width %2d\n",iClock,windowWidth[iopt1-1]);
	      printf("iClock1 %2d iClock2 %2d\n",iClock1,iClock2);
	      printf("iClock3 %2d iClock4 %2d\n",iClock3,iClock4);
	      printf("iClock5 %2d iClock6 %2d\n",iClock5,iClock6);
	      printf("\n ???????? ERROR at iopt1 %2d iopt2 %2d??????\n\n"
		     ,iopt1,iopt2);
	      exit(0);
	    } // IF iopt2 < iClock3 || iopt2 > iClock4
	  } // IF iopt < 21
	} // IF iCapture==1

	latchFifoClear(id);

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
void latchRunTestHodo(int id, int runs)
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

  //  latchInit(0x09000000,0x100000,1);
  latchEdgeMode(id,2);

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

	latchORoperation(id, iopt1, iopt2);
	printf("\nrun %3d iopt1 %2d iopt2 %2d latchStatus CSR2 : %x\n"
	       ,irun,iopt1,iopt2,latchp[id]->csr2);

	/* --- taking data --- */
	latchTrigEnable(id);
	for ( i=0; i<16; i++) printf(" "); // not set i<5, must be > 10
	printf("\n");
	latchTrigDisable(id);

	iCapture=latchFifoReadHodo(id,5);
	if ( iCapture==1 ){
	  iClock = iopt2 - 6 + windowShift[iopt1-1];
	  ifound = ifound + 1;

	  if ( iopt1==21){
	    if ( ifound==1 ) iClock3 = iClock;
	    iClock4 = iClock + 5;
	  } // IF iopt1==21

	  iClock5 = iClock;
	  iClock6 = iClock + windowWidth[iopt1-1] -1;
	  for ( k=iClock5; k<=iClock6; k++ ) or2output[iopt1-1][k] = 1;
	  printf("iCapture=1!!!!!!!!!!!\n");
	  if ( iopt1 < 21 ){
	    if ( ( iClock < iClock3 ) || ( iClock > iClock4 ) ){
	      printf("\nRun %3d iopt1 %2d iopt2 %2d\n",irun,iopt1,iopt2);
	      printf("iClock  %2d width %2d\n",iClock,windowWidth[iopt1-1]);
	      printf("iClock1 %2d iClock2 %2d\n",iClock1,iClock2);
	      printf("iClock3 %2d iClock4 %2d\n",iClock3,iClock4);
	      printf("iClock5 %2d iClock6 %2d\n",iClock5,iClock6);
	      printf("\n ???????? ERROR at iopt1 %2d iopt2 %2d??????\n\n"
		     ,iopt1,iopt2);
	      //      exit(0);
	    } // IF iopt2 < iClock3 || iopt2 > iClock4
	  } // IF iopt < 21
	} // IF iCapture==1

	latchFifoClear(id);

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
void latchRunTestCh(int id, int iCh, int runs)
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

  //  latchInit(0x09000000,0x100000,1);
  latchEdgeMode(id,2);

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

	latchORoperation(id, iopt1, iopt2);
	printf("\nrun %3d iopt1 %2d iopt2 %2d latchStatus CSR2 : %x\n"
	       ,irun,iopt1,iopt2,latchp[id]->csr2);

	/* --- taking data --- */
	latchTrigEnable(id);
	for ( i=0; i<16; i++) printf(" "); // not set i<5, must be > 10
	printf("\n");
	latchTrigDisable(id);

	iCapture=latchFifoReadCh(id,5,iCh);
	if ( iCapture==1 ){
	  iClock = iopt2 - 6 + windowShift[iopt1-1];
	  ifound = ifound + 1;

	  if ( iopt1==21){
	    if ( ifound==1 ) iClock3 = iClock;
	    iClock4 = iClock + 5;
	  } // IF iopt1==21

	  iClock5 = iClock;
	  iClock6 = iClock + windowWidth[iopt1-1] -1;
	  for ( k=iClock5; k<=iClock6; k++ ) or2output[iopt1-1][k] = 1;

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

	latchFifoClear(id);

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

void latchASDQ(int id, int iruns, int iOR, int iclock)
{
 unsigned int res1[64], res2[64];
 int i;
 latchReset(id);
 
 latchTrigDisable(id);
 latchFifoClear(id);
 
 latchEdgeMode(id,2);
 latchORoperation(id,iOR,iclock);
 latchTrigEnable(id);
 sleep(1);
 latchTrigDisable(id);
 latchFifoRead(id,iruns);


}

int latchBroadID(UINT32 addr, UINT32 addr_inc, int id)
{
  int broadid;
  broadid=addr+(addr_inc*id);


  return broadid;

}

int latchFifoStatus(int id){
  int istatus;
  istatus=(latchp[id]->fifoStatusReg)&(fifoStatusMask);
  return istatus;
}
