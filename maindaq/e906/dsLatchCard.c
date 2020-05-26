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
#include "dsLatchCard.h"

/* Prototypes */
void latchInt (void);
void latchFFEnable(int id, int val);
void latchFFDisable(int id, int val);
void latchFFReset(int id, int val);
void grassWriteFile (char* filename, char* InputWord);

/* Define external Functions */
IMPORT STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT STATUS intDisconnect(int);
IMPORT STATUS sysIntEnable(int);
IMPORT STATUS sysIntDisable(int);

/* Define Interrupts variables */
BOOL              latchIntRunning  = FALSE; // running flag
int               latchIntID       = -1;    // id number of ADC generating interrupts
LOCAL VOIDFUNCPTR latchIntRoutine  = NULL;  // user interrupt service routine
LOCAL int         latchIntArg      = 0;     // arg to user routine
LOCAL int         latchIntEvCount  = 0;     // Number of Events to generate Interrupt
LOCAL UINT32      latchIntLevel    = LATCH_VME_INT_LEVEL; // default VME interrupt level
LOCAL UINT32      latchIntVec      = LATCH_INT_VEC;       // default interrupt Vector

/* Define global variables */
volatile struct latch_data *latchd[LATCH_MAX_BOARDS];   // pointers to ADC memory map
volatile struct latch_struct *latchp[LATCH_MAX_BOARDS]; // pointers to ADC memory map
int Nlatch = 0;                                         // Number of ADCs in Crate
int latchIntCount = 0;                                  // Count of interrupts from ADC
int latchEventCount[LATCH_MAX_BOARDS]; // Count of Events taken by ADC (Event Count Register value)
int latchEvtReadCnt[LATCH_MAX_BOARDS]; // Count of events read from specified ADC
int latchRunFlag = 0;
int *latchHistData = 0;                // Pointer to local memory for histogram data
SEM_ID latchSem;                       // Semephore for Task syncronization

/*******************************************************************************
*
* latchInit - Initialize SIS 3600 Library. 
*
* RETURNS: OK, or ERROR if the address is invalid or board is not present.
********************************************************************************/

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
      printf("latchInit: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddrd) \n",addrd);
      printf("latchInit: ERROR resd=%d\n",resd);
      return(ERROR);
    }
  } /* --- IF addr --- */

  Nlatch=0;
  for ( ii=0; ii<nmod; ii++ ){
    latchp[ii] = (struct latch_struct *)( laddr  + ii*addr_inc );
    latchd[ii] = (struct latch_data *)(   laddrd + ii*addr_inc );

    Nlatch++;
    printf("Initialized LATCH ID %d at address latchp 0x%08x \n",ii,(UINT32) latchp[ii]);
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


void latchStatus(int id)
{
  unsigned int sreg, mid, fifoStatus, outputPulseStatus;

  if ( (id<0) || (latchp[id]==NULL) ){
    printf("latchStatus: ERROR : LATCH id %d not initialized \n",id);
    return;
  }

  /* --- Get info from Module --- */
  sreg              = latchp[id]->csr;
  mid               = latchp[id]->id;
  fifoStatus        = latchp[id]->fifoStatusReg;
  outputPulseStatus = latchp[id]->outputPulseReg;

  printf("LATCH STATUS   id %d at base address 0x%x \n",id,(UINT32) latchp[id]);
  printf("LATCH STATUS data %d at base address 0x%x \n",id,(UINT32) latchd[id]);
  printf("------------------------------------------------ \n");
  printf("latchStatus CSR                   : %x\n",sreg);
  printf("latchStatus PCB number            : %x\n",mid);
  printf("latchStatus FIFO status register  : %x\n",fifoStatus);
  printf("latchStatus Output pulse register : %x\n",outputPulseStatus);
  printf("------------------------------------------------ \n\n\n");
}

void latchLedSet(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchSetLED ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->csr = userLED;
  return;
}

void latchLedClear(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchClearLED ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->csr = fifoTestMode;
  return;
}

void latchFifoStatus(int id, int iopt)
{
  // unsigned int csrStatus0, csrStatus1, csrStatus2, csrStatus3;
  unsigned int csrStatus0, csrStatus1, csrStatus2;
  unsigned int fifoStatus;

  csrStatus0 = 0;
  csrStatus1 = 0;
  csrStatus2 = 0;
  // csrStatus3 = 0;

  csrStatus0 = latchp[id]->csr;

  csrStatus1 = ( latchp[id]->csr ) & fifoStatusModeMask;

  if ( iopt==1 ){
    csrStatus2 = csrStatus1 + fifoStatusMode1;
  } else if ( iopt==2 ){
    csrStatus2 = csrStatus1 + fifoStatusMode2;
  } else if ( iopt==3 ){
    csrStatus2 = csrStatus1 + fifoStatusMode3;
  } else if ( iopt==4 ){
    csrStatus2 = csrStatus1 + fifoStatusMode4;
  }

  latchp[id]->csr = csrStatus2;

  // csrStatus3 = latchp[id]->csr;

  fifoStatus = latchp[id]->fifoStatusReg;

  //printf("FIFO Status Mode : %x\n",csrStatus3);
  //printf("FIFO Status      : %x\n",fifoStatus);
}

void latchFifoTestEnable(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchFifoTest ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->fifoTestEnable  = 1;
}

void latchFifoTestDisable(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchFifoTest ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->fifoTestDisable = 1;
}


void latchFifoClear(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchFifoClear ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->clr = 1;
}

void latchTrigEnable(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchTrigSwitch ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->trigEnable  = 1;
}

void latchTrigDisable(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchTrigSwitch ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->trigDisable = 1;
}


void latchReset(int id)
{
  if ( ( id<0 )||( latchp[id]==NULL ) ){
    logMsg("latchReset: ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->reset = 1;
}

void latchGenOutputPulse(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchGenOutputPulse ERROR : LATCHCARD id %d not initialized \n"
	   ,id,0,0,0,0,0);
    return;
  }

  latchp[id]->genOutputPulse = 1;
}

void latchStrobeMode(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchModeSwitch ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  // latchp[id]->csr = onStrobeMode;
  latchp[id]->csr = 0x0000;

  return;
}

void latchEdgeMode(int id)
{
  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchModeSwitch ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  latchp[id]->csr = onEdgeMode;

  return;
}

void latchFifoRead(int id)
{
  int ii=0;
  unsigned int res1[64], res2[64];

  if ( (id<0) || (latchp[id]==NULL) ){
    logMsg("latchReadFifoTest ERROR : LATCHCARD id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  for ( ii=0; ii<64; ii++ ){

    res1[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 01~32 --- */
    res2[ii] = ( latchd[id]->data[ii] ) & 0xffffffff;  /* --- CH 33~64 --- */

    printf("data[%3d] = 0x %8x 0x %8x\n",ii,res2[ii],res1[ii]);
  }
  return;
}

void latchRunTest(int id, int count, int iMode)
{
  latchInit(0x09000000,0x100000,1);

  if ( iMode == 0 ){
    latchStrobeMode(id);
  } else {
    latchEdgeMode(id);
  }

  latchTrigEnable(id);

  sleep(count);  /* --- taking data for "count" second --- */

  latchTrigDisable(id);

  latchFifoRead(id);

  latchFifoClear(id);

  return;
}

void latchRunTestAll(int iMode, int count, int iCard1, int iCard2)
{
  int iCard=0;

  latchInit(0x09000000,0x100000,6);

  for ( iCard=iCard1; iCard<=iCard2; iCard++ ){

    if ( iMode == 0 ){
      latchStrobeMode(iCard);
    } else {
      latchEdgeMode(iCard);
    }

    latchTrigEnable(iCard);
  }

  sleep(count);  /* --- taking data for "count" second --- */

  for ( iCard=iCard1; iCard<=iCard2; iCard++ ){

    latchTrigDisable(iCard);

    latchFifoRead(iCard);

    latchFifoClear(iCard);
  }

  return;
}
