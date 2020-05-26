/******************************************************************************
*
*  sis3610Lib.c  -  Driver library for operation of Struck 3610 I/O Latch
*                  using a VxWorks 5.4 or later based Single Board computer. 
*
*  Author: David Abbott 
*          Jefferson Lab Data Acquisition Group
*          April 2003
*
*  Revision  1.0 - Initial Revision
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

/* Include definitions */
#include "sis3610.h"

/* Prototypes */
void s3610Int (void);
void s3610FFEnable(int id, int val);
void s3610FFDisable(int id, int val);
void s3610FFReset(int id, int val);



/* Define external Functions */
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT  STATUS intDisconnect(int);
IMPORT  STATUS sysIntEnable(int);
IMPORT  STATUS sysIntDisable(int);

/* Define Interrupts variables */
BOOL              s3610IntRunning  = FALSE;                    /* running flag */
int               s3610IntID       = -1;                       /* id number of ADC generating interrupts */
LOCAL VOIDFUNCPTR s3610IntRoutine  = NULL;                     /* user interrupt service routine */
LOCAL int         s3610IntArg      = 0;                        /* arg to user routine */
LOCAL int         s3610IntEvCount  = 0;                        /* Number of Events to generate Interrupt */
LOCAL UINT32      s3610IntLevel    = S3610_VME_INT_LEVEL;      /* default VME interrupt level */
LOCAL UINT32      s3610IntVec      = S3610_INT_VEC;            /* default interrupt Vector */


/* Define global variables */
int Ns3610 = 0;                                               /* Number of ADCs in Crate */
volatile struct s3610_struct *s3610p[S3610_MAX_BOARDS];       /* pointers to ADC memory map */
int s3610IntCount = 0;                                        /* Count of interrupts from ADC */
int s3610EventCount[S3610_MAX_BOARDS];                        /* Count of Events taken by ADC (Event Count Register value) */
int s3610EvtReadCnt[S3610_MAX_BOARDS];                        /* Count of events read from specified ADC */
int s3610RunFlag = 0;
int *s3610HistData = 0;    /* Pointer to local memory for histogram data*/

SEM_ID s3610Sem;                               /* Semephore for Task syncronization */

/*******************************************************************************
*
* s3610Init - Initialize SIS 3610 Library. 
*
*
* RETURNS: OK, or ERROR if the address is invalid or board is not present.
*/

STATUS 
s3610Init (UINT32 addr, UINT32 addr_inc, int nmod)
{
  int ii, res, rdata, errFlag = 0;
  int boardID = 0;
  unsigned int laddr;

  
  /* Check for valid address */
  if(addr==0) {
    printf("s3610Init: ERROR: Must specify a Bus (VME-based A16) address for the SIS3610\n");
    return(ERROR);
  }else if(addr > 0x0000ffff) { /* A16 Addressing */
    printf("s3610Init: ERROR: A32/A24 Addressing not supported for the SIS 3610\n");
    return(ERROR);
  }else{ /* A16 Addressing */
    if((addr_inc==0)||(nmod==0))
      nmod = 1; /* assume only one ADC to initialize */

    /* get the ADC address */
    res = sysBusToLocalAdrs(0x29,(char *)addr,(char **)&laddr);
    if (res != 0) {
      printf("s3610Init: ERROR in sysBusToLocalAdrs(0x29,0x%x,&laddr) \n",addr);
      return(ERROR);
    }
  }

  Ns3610 = 0;
  for (ii=0;ii<nmod;ii++) {
    s3610p[ii] = (struct s3610_struct *)(laddr + ii*addr_inc);
    /* Check if Board exists at that address */
    res = vxMemProbe((char *) &(s3610p[ii]->id),VX_READ,4,(char *)&rdata);
    if(res < 0) {
      printf("s3610Init: ERROR: No addressable board at addr=0x%x\n",(UINT32) s3610p[ii]);
      s3610p[ii] = NULL;
      errFlag = 1;
      break;
    } else {
      /* Check if this is a Model 3610 */
      boardID =  rdata&S3610_BOARD_ID_MASK;
      if(boardID != S3610_BOARD_ID) {
	printf(" ERROR: Board ID does not match: %d \n",boardID);
	return(ERROR);
      }
    }
    Ns3610++;
    printf("Initialized SIS3610 ID %d at address 0x%08x \n",ii,(UINT32) s3610p[ii]);
  }


  
  /* Disable/Clear all I/O Modules */
  for(ii=0;ii<Ns3610;ii++) {
    s3610p[ii]->reset = 1;
  }

  /* Initialize Interrupt variables */
  s3610IntID = -1;
  s3610IntRunning = FALSE;
  s3610IntLevel = S3610_VME_INT_LEVEL;
  s3610IntVec = S3610_INT_VEC;
  s3610IntRoutine = NULL;
  s3610IntArg = 0;
  s3610IntEvCount = 0;
    

  if(errFlag > 0) {
    printf("s3610Init: ERROR: Unable to initialize all Modules\n");
    if(Ns3610 > 0)
      printf("s3610Init: %d SIS3610(s) successfully initialized\n",Ns3610);
    return(ERROR);
  } else {
    return(OK);
  }
}


void
s3610Status (int id, int sflag)
{

  unsigned int sreg, mid, doutput, dinput, linput;
  int iLevel, iVec, iMode, iEnable, iSource, iValid;
  int ffStatus, uoutStatus, ledStatus, latchStatus;

  if((id<0) || (s3610p[id] == NULL)) {
    printf("s3610Status: ERROR : SIS3610 id %d not initialized \n",id);
    return;
  }

  /* Get info from Module */
  sreg    = s3610p[id]->csr;
  mid     = s3610p[id]->id;
  doutput = (s3610p[id]->d_out)&0xffff;
  dinput  = (s3610p[id]->d_in)&0xffff;
  linput  = (s3610p[id]->l_in)&0xffff;

  iLevel  = (mid&S3610_INT_LEVEL_MASK)>>8;
  iVec    = (mid&S3610_INT_VEC_MASK);
  iEnable = (mid&S3610_INT_ENABLE);
  iMode   = (sreg&S3610_STATUS_INT_MODE)>>1;
  iSource = (sreg&S3610_INT_SOURCE_ENABLE_MASK)>>20;
  iValid  = (sreg&S3610_INT_SOURCE_VALID_MASK)>>28;

  ffStatus    = (sreg&S3610_FF_ENABLE_MASK)>>16;
  uoutStatus  = (sreg&S3610_USER_OUTPUT_MASK)>>4;
  ledStatus   = (sreg&S3610_STATUS_USER_LED);
  latchStatus = (sreg&S3610_LATCH_STROBE_MASK)>>2;

  printf("\nSTATUS for SIS3610 id %d at base address 0x%x \n",id,(UINT32) s3610p[id]);
  printf("------------------------------------------------ \n");

  if(iEnable) {
    printf(" Interrupts Enabled  - Mode = %d  (0:RORA 1:ROAK)\n",iMode);
    printf(" VME Interrupt Level: %d   Vector: 0x%x \n",iLevel,iVec);
    printf(" Sources Enabled: 0x%x  Sources Valid: 0x%x \n",iSource,iValid);
  } else {
    printf(" Interrupts Disabled - Mode = %d  (0:RORA 1:ROAK)\n",iMode);
    printf(" VME Interrupt Level: %d   Vector: 0x%x \n",iLevel,iVec);
    printf(" Sources Enabled: 0x%x  Sources Valid: 0x%x \n",iSource,iValid);
  }
  printf("\n");
  printf("  MODULE ID   register = 0x%08x \n",mid);
  printf("\n");

  printf("  STATUS      register = 0x%08x \n",sreg);
  printf("      User Led   Enabled : %d \n",ledStatus);
  printf("      User Output Status : 0x%x \n",uoutStatus);
  printf("      Flip-Flops Enabled : 0x%x \n",ffStatus);
  printf("      Latch Strobe Status: 0x%x \n",latchStatus);

  printf("\n");
  printf("  Direct Output Register = 0x%04x \n",doutput);
  printf("  Direct Input Register  = 0x%04x \n",dinput);
  printf("  Latched Input Register = 0x%04x \n",linput);


}


/* Delcare Key Address functions */

void
s3610Reset(int id)
{
  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610Reset: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }
  s3610p[id]->reset = 1;
}


/* Acquisition/Mode Functions */

int
s3610IntSet(int id, int iLevel, int iVec, int iSource)
{
  int iVal=0;

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610IntSet: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  /* If interrupts are already enabled - don't touch anything */
  if((s3610p[id]->csr)&S3610_INT_ENABLE) {
    logMsg("s3610IntSet: ERROR : Interrupts are already enabled\n",0,0,0,0,0,0);
    return(ERROR);
  }

  if(iLevel)
    iVal = (iLevel<<8);
  else
    iVal = (s3610IntLevel<<8);

  if(iVec)
    iVal |= ((iVec&0xf0)+1);
  else
    iVal |= s3610IntVec;

  s3610p[id]->id = iVal;

  if(iSource) 
    s3610p[id]->csr = ((iSource&0xf)<<20);

  s3610IntID = id;

  return(iVal);
}


void
s3610IntAck(int intMask)
{

  if(s3610IntID<0) {
    logMsg("s3610IntEnable: ERROR : Interrupt Module ID not set\n",0,0,0,0,0,0);
    return;
  }

  /* Reset Flip-Flop */
  s3610FFReset(s3610IntID,intMask);

  /* Enable interrupts */
  S3610_IRQ_ENABLE(s3610IntID);

}


void 
s3610Int (void)
{
  
  /* Disable interrupts */
  sysIntDisable(s3610IntLevel);
  S3610_IRQ_DISABLE(s3610IntID);

  s3610IntCount++;
 
  if (s3610IntRoutine != NULL)  {     /* call user routine */
    (*s3610IntRoutine) (s3610IntArg);
  }else{
    if((s3610IntID<0) || (s3610p[s3610IntID] == NULL)) {
      logMsg("s3610Int: ERROR : SIS3610 id %d not initialized \n",s3610IntID,0,0,0,0,0);
      return;
    }
  }

  /* Acknowledge interrupt - re-enable */
  s3610IntAck(1);

  /* Re-Enable interrupts on CPU */
  sysIntEnable(s3610IntLevel);

}


STATUS 
s3610IntConnect (int id, VOIDFUNCPTR routine, int arg, UINT16 level, UINT16 vector)
{

  if(s3610IntRunning) {
    printf("s3610IntConnect: ERROR : Interrupts already Initialized for SIS3610 id %d\n",
	   s3610IntID);
    return(ERROR);
  }

  s3610IntRoutine = routine;
  s3610IntArg = arg;

  /* Check for user defined VME interrupt level and vector */
  if(level == 0) {
    s3610IntLevel = S3610_VME_INT_LEVEL; /* use default */
  }else if (level > 7) {
    printf("s3610IntConnect: ERROR: Invalid VME interrupt level (%d). Must be (1-7)\n",level);
    return(ERROR);
  } else {
    s3610IntLevel = level;
  }

  if(vector == 0) {
    s3610IntVec = S3610_INT_VEC;  /* use default */
  }else if ((vector < 32)||(vector>255)) {
    printf("s3610IntConnect: ERROR: Invalid interrupt vector (%d). Must be (32<vector<255)\n",vector);
    return(ERROR);
  }else{
    s3610IntVec = (vector&0xf0) + 1;
  }
      
  /* Connect the ISR */
#ifdef VXWORKSPPC
  if((intDisconnect((int)INUM_TO_IVEC(s3610IntVec)) != 0)) {
    printf("s3610IntConnect: ERROR disconnecting Interrupt\n");
    return(ERROR);
  }
#endif
  if((intConnect(INUM_TO_IVEC(s3610IntVec),s3610Int,s3610IntArg)) != 0) {
    printf("s3610IntConnect: ERROR in intConnect()\n");
    return(ERROR);
  }


  s3610IntSet(id, s3610IntLevel, s3610IntVec, 0);
  return (OK);
}



void
s3610IntEnable(int intMask)
{

  if(s3610IntID<0) {
    logMsg("s3610IntEnable: ERROR : Interrupt Module ID not set\n",0,0,0,0,0,0);
    return;
  }

  if(s3610IntRunning) {
    logMsg("s3610IntEnable: ERROR : Interrupts already Enabled for SIS3610 id %d\n",
	   s3610IntID,0,0,0,0,0);
    return;
  }

  sysIntDisable(s3610IntLevel);


  if(intMask == 0) { /* Check if any sources are enabled */
    if( ((s3610p[s3610IntID]->csr)&S3610_INT_SOURCE_ENABLE_MASK) == 0) {
      logMsg("s3610IntEnable: ERROR : No Interrupt sources are specified\n",0,0,0,0,0,0);
      return;
    }
  } else {
    /* Enable Interrupts */
    s3610p[s3610IntID]->csr = (intMask&0xf)<<20;
    s3610FFEnable(s3610IntID,(intMask&0xf));
  }


  S3610_IRQ_ENABLE(s3610IntID);
  s3610IntRunning = 1;
  sysIntEnable(s3610IntLevel);

  return;
}

void
s3610RORA()
{

  if(s3610IntID<0) {
    logMsg("s3610RORA: ERROR : Interrupt Module ID not set\n",0,0,0,0,0,0);
    return;
  }

  if(s3610IntRunning) {
    logMsg("s3610RORA: ERROR : Interrupts Enabled for SIS3610 id %d\n",
	   s3610IntID,0,0,0,0,0);
    return;
  }

  s3610p[s3610IntID]->csr = S3610_CNTL_SET_RORA;

  return;
}

void
s3610ROAK()
{

  if(s3610IntID<0) {
    logMsg("s3610ROAK: ERROR : Interrupt Module ID not set\n",0,0,0,0,0,0);
    return;
  }

  if(s3610IntRunning) {
    logMsg("s3610ROAK: ERROR : Interrupts Enabled for SIS3610 id %d\n",
	   s3610IntID,0,0,0,0,0);
    return;
  }

  s3610p[s3610IntID]->csr = S3610_CNTL_SET_ROAK;

  return;
}


void
s3610IntDisable(int intMask)
{

  if(s3610IntID<0) {
    logMsg("s3610IntEnable: ERROR : Interrupt Module ID not set\n",0,0,0,0,0,0);
    return;
  }


  sysIntDisable(s3610IntLevel);
  S3610_IRQ_DISABLE(s3610IntID);
  s3610IntRunning = 0;

  if(intMask) {
    s3610p[s3610IntID]->csr = (intMask&0xf)<<28;
    s3610FFDisable(s3610IntID,(intMask&0xf));
  }

  return;

}


   /* Control Functions */


unsigned int
s3610CSR(int id, unsigned int val)
{
  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610CSR: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return(0);
  }

  if(val)
    s3610p[id]->csr = val;

  return(s3610p[id]->csr);

}

void
s3610SetLED(int id)
{
  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610SetLED: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  s3610p[id]->csr = S3610_CNTL_SET_USER_LED;

  return;
}

void
s3610ClearLED(int id)
{
  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610ClearLED: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  s3610p[id]->csr = S3610_CNTL_CLR_USER_LED;

  return;
}


void
s3610FFEnable(int id, int val)
{

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610FFEnable: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  if(val == 0) /* Enable control input 1 in FF mode only */
    val=1; 

  s3610p[id]->csr = ((val&0xf)<<16);

  return;
}

void
s3610FFDisable(int id, int val)
{

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610FFDisable: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  if(val == 0) /* Disable control input 1 FF mode only */
    val = 1;

  s3610p[id]->csr = ((val&0xf)<<24);

  return;
}

void
s3610FFReset(int id, int val)
{

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610FFReset: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  if(val == 0) /* Reset control input 1 FF mode only */
    val = 1;

  s3610p[id]->csr = ((val&0xf)<<12);

  return;
}



   /* Readout Functions */

int
s3610ReadOutput(int id)
{
  int res=0;

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610ReadOutput: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  res = (s3610p[id]->d_out)&0xffff;

  return(res);
}

int
s3610ReadInput(int id)
{
  int res=0;

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610ReadInput: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  res = (s3610p[id]->d_in)&0xffff;

  return(res);
}

int
s3610ReadLatch(int id)
{
  int res=0;

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610ReadLatch: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return(ERROR);
  }

  res = (s3610p[id]->l_in)&0xffff;

  return(res);
}

void
s3610WriteOutput(int id, unsigned int val)
{

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610WriteOutput: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  s3610p[id]->d_out = val;

  return;
}

void
s3610WriteBits(int id, unsigned int val)
{

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610WriteBits: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  s3610p[id]->jk_out = val;

  return;
}

void
s3610ClearBits(int id, unsigned int val)
{

  if((id<0) || (s3610p[id] == NULL)) {
    logMsg("s3610ClearBits: ERROR : SIS3610 id %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  if(val == 0) 
    val = 0xffff;

  s3610p[id]->jk_out = (val<<16);

  return;
}


