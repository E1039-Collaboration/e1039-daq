/******************************************************************************
*
*  sis3600Lib.c  -  Driver library for operation of Struck 3600 I/O Latch
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
#include "sis3600.h"

/* Prototypes */
void s3600Int (void);
void s3600FFEnable(int id, int val);
void s3600FFDisable(int id, int val);
void s3600FFReset(int id, int val);



/* Define external Functions */
IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);
IMPORT  STATUS intDisconnect(int);
IMPORT  STATUS sysIntEnable(int);
IMPORT  STATUS sysIntDisable(int);

/* Define Interrupts variables */
BOOL              s3600IntRunning  = FALSE;                    /* running flag */
int               s3600IntID       = -1;                       /* id number of ADC generating interrupts */
LOCAL VOIDFUNCPTR s3600IntRoutine  = NULL;                     /* user interrupt service routine */
LOCAL int         s3600IntArg      = 0;                        /* arg to user routine */
LOCAL int         s3600IntEvCount  = 0;                        /* Number of Events to generate Interrupt */
LOCAL UINT32      s3600IntLevel    = S3600_VME_INT_LEVEL;      /* default VME interrupt level */
LOCAL UINT32      s3600IntVec      = S3600_INT_VEC;            /* default interrupt Vector */



/* Define global variables */
int Ns3600 = 0;                                               /* Number of ADCs in Crate */
volatile struct s3600_data *s3600d[S3600_MAX_BOARDS];         /* pointers to ADC memory map */
volatile struct s3600_struct *s3600p[S3600_MAX_BOARDS];       /* pointers to ADC memory map */
int s3600IntCount = 0;                                        /* Count of interrupts from ADC */
int s3600EventCount[S3600_MAX_BOARDS];                        /* Count of Events taken by ADC (Event Count Register value) */
int s3600EvtReadCnt[S3600_MAX_BOARDS];                        /* Count of events read from specified ADC */
int s3600RunFlag = 0;
int *s3600HistData = 0;                                       /* Pointer to local memory for histogram data*/
SEM_ID s3600Sem;                                              /* Semephore for Task syncronization */

/*******************************************************************************
*
* s3600Init - Initialize SIS 3600 Library. 
*
*
* RETURNS: OK, or ERROR if the address is invalid or board is not present.
********************************************************************************/

STATUS 
s3600Init (UINT32 addr, UINT32 addr_inc, int nmod)
{
  int ii, res, resd,  rdata, errFlag = 0;
  /*  unsigned int rdatad;*/
  int boardID = 0;
  unsigned int laddr;
  unsigned int addrd;
  unsigned int laddrd;

  
  addrd=addr+0x100;  

  /* Check for valid address */
  if(addr==0) {
    printf("s3600Init: ERROR: Must specify a Bus (VME-based A16) address for the SIS3600\n");
    return(ERROR);
  }
  else if(addr > 0xffffffff) { /* A32 Addressing */
    printf("s3600Init: ERROR: A32/A24 Addressing not supported for the SIS 3600\n");
    return(ERROR);
  }
  else{ 
    if((addr_inc==0)||(nmod==0))
      nmod = 1; /* assume only one ADC to initialize */
    
    /* get the ADC address */
    res = sysBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr); /*0x09 is A32 non privileged data access */
    resd = sysBusToLocalAdrs(0x09,(char *)addrd,(char **)&laddrd);
    if (res != 0) {
      printf("s3600Init: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",addr);
      printf("s3600Init: ERROR res=%d\n",res);
      return(ERROR);
    }
    if (resd != 0) {
      printf("s3600Init: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddrd) \n",addrd);
      printf("s3600Init: ERROR resd=%d\n",resd);
      return(ERROR);
    }
  }

  Ns3600 = 0;
  for (ii=0;ii<nmod;ii++) 
    {
      s3600p[ii] = (struct s3600_struct *)(laddr + ii*addr_inc);
      s3600d[ii] = (struct s3600_data *)(laddrd + ii*addr_inc);
      /* Check if Board exists at that address */
      res = vxMemProbe((char *) &(s3600p[ii]->id),VX_READ,4,(char *)&rdata);
      /*      resd = vxMemProbe((char *) &(s3600d[ii]->data[1]),VX_READ,4,(char *)&rdatad);*/
      if(res < 0) 
	{
	  printf("s3600Init: ERROR1: No addressable board at addr=0x%x\n",(UINT32) s3600p[ii]);
	  s3600p[ii] = NULL;
	  errFlag = 1;
	  break;
	} 
      else 
	{
	  /* Check if this is a Model 3600 */
	  boardID =  rdata&S3600_BOARD_ID_MASK;
	  if(boardID != S3600_BOARD_ID) 
	    {
	      printf(" ERROR: Board ID does not match: %d \n",boardID);
	      return(ERROR);
	    }
	}
      /*
      if(resd < 0) 
	{
	  printf("s3600Init: ERROR2: No addressable board at addr=0x%x\n",(UINT32) s3600d[ii]);
	  break;
	}
      */ 
      Ns3600++;
      printf("Initialized SIS3600 ID %d at address 0x%08x \n",ii,(UINT32) s3600p[ii]);
    }
  
  
  
  /* Disable/Clear all I/O Modules */
  for(ii=0;ii<Ns3600;ii++) {
    s3600p[ii]->reset = 1;
  }
  
  /* Initialize Interrupt variables */
  s3600IntID = -1;
  s3600IntRunning = FALSE;
  s3600IntLevel = S3600_VME_INT_LEVEL;
  s3600IntVec = S3600_INT_VEC;
  s3600IntRoutine = NULL;
  s3600IntArg = 0;
  s3600IntEvCount = 0;
    

  if(errFlag > 0) {
    printf("s3600Init: ERROR: Unable to initialize all Modules\n");
    if(Ns3600 > 0)
      printf("s3600Init: %d SIS3600(s) successfully initialized\n",Ns3600);
    return(ERROR);
  } else {
    return(OK);
  }
}


void
s3600Status (int id)

{

  unsigned int sreg, mid;
  int iLevel, iVec, fMode, iEnable, iSource, iValid;
  int exStatus, uoutStatus, ledStatus, outputStatus;
  int ver;

  if((id<0) || (s3600p[id] == NULL)) {
    printf("s3600Status: ERROR : SIS3600 id %d not initialized \n",id);
    return;
  }

  /* Get info from Module */
  sreg    = s3600p[id]->csr;
  mid     = s3600p[id]->id;

  iLevel  = (mid&S3600_INT_LEVEL_MASK)>>8;
  iVec    = (mid&S3600_INT_VEC_MASK);
  iEnable = (mid&S3600_INT_ENABLE);
  fMode   = (sreg&S3600_STATUS_FIFO_MODE)>>1;
  iSource = (sreg&S3600_INT_SOURCE_ENABLE_MASK)>>20;
  iValid  = (sreg&S3600_INT_SOURCE_VALID_MASK)>>28;

  exStatus    = (sreg&S3600_FF_ENABLE_MASK)>>16;
  uoutStatus  = (sreg&S3600_USER_OUTPUT_MASK)>>4;
  ledStatus   = (sreg&S3600_STATUS_USER_LED);
  outputStatus = (sreg&S3600_Output_Mode_MASK)>>2;
  
  ver =  (mid&S3600_VERSION_MASK)>>12;

  printf("\nSTATUS for SIS3600 id %d at base address 0x%x \n",id,(UINT32) s3600p[id]);
  printf("------------------------------------------------ \n");

  if(iEnable) {
    printf(" Interrupts Enabled\n");
    printf(" FIFO Test Mode = %d  \n",fMode);
    printf(" VME Interrupt Level: %d   Vector: 0x%x \n",iLevel,iVec);
    printf(" Sources Enabled: 0x%x  Sources Valid: 0x%x \n",iSource,iValid);
  } else {
    printf(" Interrupts Disabled\n");
    printf(" FIFO Test Mode = %d  \n",fMode);
    printf(" VME Interrupt Level: %d   Vector: 0x%x \n",iLevel,iVec);
    printf(" Sources Enabled: 0x%x  Sources Valid: 0x%x \n",iSource,iValid);
  }
  printf("\n");
  printf("  MODULE ID   register = 0x%08x \n",mid);
  printf("      Version : %d \n",ver);
  printf("\n");

  printf("  STATUS      register = 0x%08x \n",sreg);
  printf("      User Led   Enabled : %d \n",ledStatus);
  printf("      User Output Status : 0x%x \n",uoutStatus);
  printf("      External next Enabled : 0x%x \n",exStatus);
  printf("      Output Mode Status: 0x%x \n",outputStatus);

}



void
s3600SetLED(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600SetLED: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0\
	   );
    return;
  }

  s3600p[id]->csr = S3600_CNTL_SET_USER_LED;

  return;
}

void
s3600ClearLED(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600ClearLED: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0\
	   ,0);
    return;
  }

  s3600p[id]->csr = S3600_CNTL_CLR_USER_LED;

  return;
}

void
s3600ExnEnable(int id)
{
  int val;
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600ExnEnable: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0\
	   ,0);
    return;
  }

    val=1;

  s3600p[id]->csr = ((val&0xf)<<16);

  return;
}

void
s3600ExcEnable(int id)
{
  int val;
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600ExcEnable: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0\
           ,0);
    return;
  }

  val=1;

  s3600p[id]->csr = ((val&0xf)<<17);

  return;
}

void
s3600EnIRQ0(int id)
{
  int val;
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600EnIRQ0 ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0\
           ,0);
    return;
  }

  val=1;

  s3600p[id]->csr = ((val&0xf)<<20);

  return;
}

void
s3600EnIRQ1(int id)
{
  int val;
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600EnIRQ1 ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0\
           ,0);
    return;
  }

  val=1;

  s3600p[id]->csr = ((val&0xf)<<21);

  return;
}

void
s3600EnIRQ2(int id)
{
  int val;
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600EnIRQ2 ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0\
           ,0);
    return;
  }

  val=1;

  s3600p[id]->csr = ((val&0xf)<<22);

  return;
}

void
s3600EnIRQ3(int id)
{
  int val;
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600EnIRQ3 ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0\
           ,0);
    return;
  }

  val=1;

  s3600p[id]->csr = ((val&0xf)<<23);

  return;
}


void
s3600Reset(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600Reset: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  s3600p[id]->reset = 1;
}

void
s3600Vmenc(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600Vmenc: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  s3600p[id]->vmenc = 1;
}

void
s3600Ennc(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600Ennc: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  s3600p[id]->ennc = 1;
}

void
s3600Dinc(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600Dinc: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  s3600p[id]->dinc = 1;
}

void
s3600Enfc(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600Enfc: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  s3600p[id]->enfc = 1;
}

void
s3600Difc(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600Difc: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  s3600p[id]->difc = 1;
}

void
s3600Geop(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600Geop: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  s3600p[id]->geop = 1;
}

void
s3600ClearFIFO(int id)
{
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600Reset: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,0,0)\
      ;
    return;
  }
  s3600p[id]->cf = 1;
}


int
/*s3600ReadFIFO(int id, unsigned int val)*/
s3600ReadFIFO(int id, unsigned int val)
{
  /*  unsigned int res;*/
  unsigned int res;
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600ReadFIFO: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,\
	   0,0);
    return(ERROR);
  }
  /*  res[1] = (s3600d[id]->data[1])&0xffff;
      res[2] = (s3600d[id]->data[2])&0xffff;*/
  res = (s3600d[id]->data[val])&0xffffffff;
  /* printf ("data[%d]=%d\n",val,res);*/
  return(res);
}



/******************************** under this line are all test function *************************************************/

void
s3600EnFIFOTest(int id)
{
  int val;
  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600WriteBits: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,\
	   0,0);
    return;
  }
  val=1;
  s3600p[id]->csr = (val&0xf)<<1;

  return;
}

void
s3600ReadFIFOTest(int id)
{
  /*  int res=0;*/
  int ii=1;
  /*  long int res[32];*/
  unsigned int res[64];

  if((id<0) || (s3600d[id] == NULL)) {
    logMsg("s3600ReadFIFOTest: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,\
	   0,0);
    return;
  }

  /*  res[1] = (s3600d[id]->data[1])&0xffff;
      res[2] = (s3600d[id]->data[2])&0xffff;*/
  for (ii=1;ii<32;ii++){
    res[ii] = (s3600d[id]->data[ii])&0xffffffff;
    
    printf ("data[%d]=0x%x\n",ii,res[ii]);
    
  }
  return;
}

void
s3600WriteFIFOTest(int id, unsigned int val)
/*s3600WriteFIFOTest(int id, unsigned char val)*/
{

  if((id<0) || (s3600p[id] == NULL)) {
    logMsg("s3600WriteBits: ERROR : SIS3600 id %d not initialized \n",id,0,0,0,\
	   0,0);
    return;
  }

  s3600p[id]->wf = val;

  return;
}

void s3600test(int id, unsigned int val)
/*void s3600test(int id, unsigned char val)*/
{
  s3600EnFIFOTest(id);
  s3600WriteFIFOTest(id,val);
  s3600ReadFIFOTest(id);
  return;
}

void s3600test10(int id, unsigned int val)
/*void s3600test(int id, unsigned char val)*/
{

  int n=0;
  s3600EnFIFOTest(id);

  for (n=0;n<40;n++){

  s3600WriteFIFOTest(id,n);

  }
  return;
}

void s3600ex(int id)

{
  s3600Reset(id);
  s3600ClearFIFO(id);
  s3600Ennc(id);
  s3600ExnEnable(id);
  return;
}

