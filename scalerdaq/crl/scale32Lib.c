/******************************************************************************
*
*  scale32Lib.c  -  Driver library for readout of the Scale 32 Counter
*                   using a VxWorks 5.2 orlater based Single Board computer. 
*
*  Author: David Abbott 
*          Jefferson Lab Data Acquisition Group
*          March 2000
*
*  Revision  1.0 - Initial Revision
*
*
*/

#include "vxWorks.h"
#include "stdio.h"
#include "string.h"
#include "logLib.h"
#include "taskLib.h"
#include "intLib.h"
#include "vxLib.h"

/* Include scaler definitions */
#include "scale32Lib.h"

IMPORT  STATUS sysBusToLocalAdrs(int, char *, char **);

/* Define global variables */
int Nscale32 = 0;
volatile struct scale32_struct *s32p[20];       /* pointers to Scaler memory map */
                                 

/*******************************************************************************
*
* scale32Init - Initialize Scale 32 library. Disable/Clear Counters
*
*
* RETURNS: OK, or ERROR if the scaler address is invalid.
*/

STATUS 
scale32Init (UINT32 addr, UINT32 addr_inc, int nscalers)
{
  int ii, res, rdata, errFlag = 0;
  unsigned long laddr;
  
  
  /* Check for valid address */
  if(addr==0) {
    printf("scal32Init: ERROR: Must specify a local VME-based (A32) address for Scaler 0\n");
    return(ERROR);
  }else{
    if((addr_inc==0)||(nscalers==0))
      nscalers = 1; /* assume only one scaler to initialize */

    Nscale32 = 0;
    
    /* get the scaler's address */
    res = sysBusToLocalAdrs(0x09,(char *)addr,(char **)&laddr);
    if (res != 0) {
      printf("scale32Init: ERROR in sysBusToLocalAdrs(0x09,0x%x,&laddr) \n",addr);
      return(ERROR);
    } else {
      for (ii=0;ii<nscalers;ii++) {
	s32p[ii] = (struct scale32_struct *)(laddr + SCAL_BASE_ADDR_OFFSET + ii*addr_inc);
	/* Check if Board exists at that address */
	res = vxMemProbe((char *) s32p[ii],0,4,(char *)&rdata);
	if(res < 0) {
	  printf("scale32Init: ERROR: No addressable board at addr=0x%x\n",(UINT32) s32p[ii]);
	  s32p[ii] = NULL;
	  errFlag = 1;
	  break;
	}
	Nscale32++;
	printf("Initialized Scaler ID %d at address 0x%08x \n",ii,(UINT32) s32p[ii]);
      }
    }	
  }
  
  /* Disable/Clear all salers */
  for(ii=0;ii<Nscale32;ii++) {
    s32p[ii]->inhib = SCAL_ALL_MASK;    /* Inhibit counters */ 
    s32p[ii]->clr[0] = SCAL_ALL_MASK;   /* Set reset mask */
    s32p[ii]->etat = SCAL_ETAT_CRST0;   /* reset */
    s32p[ii]->etat = 0;
  }
    
  if(errFlag > 0) {
    printf("scal32Init: ERROR: Unable to initialize all Scalers\n");
    if(Nscale32 > 0)
      printf("scal32Init: %d Scaler(s) successfully initialized\n",Nscale32);
    return(ERROR);
  } else {
    return(OK);
  }
}

/*******************************************************************************
*
* scalPrint - Print scaler values to standard out
*
*  RETURNS: N/A
*
*/
void 
scale32Print(int id, int latch)
{
  int ii;

  if((id<0) || (s32p[id] == NULL)) {
    printf("scale32Print: ERROR : Scaler %d not initialized \n",id);
    return;
  }

  if(latch > 0) {
    s32p[id]->latch = 0xffffffff;
    printf("   Scaler ID %d Data (Latched):\n",id);
  }else{
    printf("   Scaler ID %d Data :\n",id);
  }

  for(ii=0;ii<32;ii+=4) {
    printf(" SCAL%02d: %10d  SCAL%02d: %10d  SCAL%02d: %10d  SCAL%02d: %10d \n",
	   ii,  (UINT32) (s32p[id]->scaler[_I_(ii)]),
	   ii+1,(UINT32) (s32p[id]->scaler[_I_(ii+1)]),
	   ii+2,(UINT32) (s32p[id]->scaler[_I_(ii+2)]),
	   ii+3,(UINT32) (s32p[id]->scaler[_I_(ii+3)]));
  }

  if(latch)
    s32p[id]->latch = 0;

}

UINT32
scale32Return(int id, int channelnum){
  return s32p[id]->scaler[channelnum];
}
/*******************************************************************************
*
* scale32Clear - Clear scaler Values
*
*
*  RETURNS: N/A
*
*/
void 
scale32Clear(int id, int creg)
{

  if((id<0) || (s32p[id] == NULL)) {
    logMsg("scale32Clear: ERROR : Scaler %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  /* Set default Clear/Reset register to 0 */
  if((creg<0) || (creg>3))
    creg = 0;
  
  s32p[id]->etat = 1<<creg;
  s32p[id]->etat = 0;

}

/*******************************************************************************
*
* scale32CLR - Program and/or readback CLR registers
*
*
*  RETURNS: value in specified CLR register
*
*/
UINT32 
scale32CLR(int id, int creg, UINT32 cmask)
{
  UINT32 rmask;

  if((id<0) || (s32p[id] == NULL)) {
    logMsg("scale32CLR: ERROR : Scaler %d not initialized \n",id,0,0,0,0,0);
    return(0);
  }

  /* Set default Clear/Reset register to 0 */
  if((creg<0) || (creg>3))
    creg = 0;

  if(cmask > 0) {
    BITSWAP(cmask);
    s32p[id]->clr[creg] = cmask;
  }
  rmask = s32p[id]->clr[creg];
  BITSWAP(rmask);
  
  return(rmask);
}

/*******************************************************************************
*
* scale32Input - Read Input register
*
*
*  RETURNS: value in specified INPUT register
*
*/
UINT32 
scale32Input(int id, int pattern)
{
  UINT32 rmask;

  if((id<0) || (s32p[id] == NULL)) {
    logMsg("scale32Input: ERROR : Scaler %d not initialized \n",id,0,0,0,0,0);
    return(0);
  }

  if(pattern > 0)
    s32p[id]->etat = SCAL_ETAT_PAT;
  else
    s32p[id]->etat = 0;

  rmask = s32p[id]->input;
  BITSWAP(rmask);

  return(rmask);
}

/*******************************************************************************
*
* scale32Disable - Disable Scaler counters
*
*
*  RETURNS: N/A
*
*/
void 
scale32Disable(int id, unsigned long imask)
{

  if((id<0) || (s32p[id] == NULL)) {
    logMsg("scale32Disable: ERROR : Scaler %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  /* Set default Disable to All scalers */
  if(imask == 0){
    imask = 0xffffffff;
  }else{
    BITSWAP(imask);
  }
  
  s32p[id]->inhib = imask;
}

/*******************************************************************************
*
* scale32Enable - Enable Scaler counters
*
*
*  RETURNS: N/A
*
*/
void 
scale32Enable(int id, unsigned long imask)
{

  if((id<0) || (s32p[id] == NULL)) {
    logMsg("scale32Enable: ERROR : Scaler %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  /* Set default Enable to All scalers */
  if(imask == 0) {
    imask = 0xffffffff;
  }else{
    BITSWAP(imask);
  }

  s32p[id]->inhib = ~imask;
}

/*******************************************************************************
*
* scale32Latch - Latch Scaler counters
*
*
*  RETURNS: N/A
*
*/
void 
scale32Latch(int id, unsigned long lmask)
{

  if((id<0) || (s32p[id] == NULL)) {
    logMsg("scale32Latch: ERROR : Scaler %d not initialized \n",id,0,0,0,0,0);
    return;
  }

  if(lmask > 0)
    BITSWAP(lmask);
  
  s32p[id]->latch = lmask;
}

/*******************************************************************************
*
* scalRead - Read scaler values to specified address
*
*  RETURNS: number of scaler values read
*
*/
int
scale32Read(int id, UINT32 rmask, UINT32 *data, UINT32 latch)
{
  int ii, jj, lock_key, nvalues = 0;

  if((id<0) || (s32p[id] == NULL)) {
    logMsg("scale32Read: ERROR : Scaler %d not initialized \n",id,0,0,0,0,0);
    return(-1);
  }

  if((latch > 0)&&(latch != SCAL_ALL_MASK)) {
    BITSWAP(latch);
    s32p[id]->latch = latch;
  }else if(latch == SCAL_ALL_MASK) {
    SCALER_LATCH_ALL(id);
  }

  /* Sort rmask to determine which scalers to read */
  lock_key = intLock();
  jj=0;
  for (ii=31;ii>=0;ii--) {
    if(((rmask)&(1<<jj)) > 0) {
      *data++ = s32p[id]->scaler[ii];
      nvalues++;
    }      
    jj++;
  }
  intUnlock(lock_key);

  if(latch)
    SCALER_UNLATCH_ALL(id);


  return(nvalues);
}

/*******************************************************************************
*
* scale32Status - Return scaler counter status information
*
*
*  RETURNS: N/A
*
*/
void 
scale32Status(int id, int sflag)
{
  int ii, lock_key;
  UINT32 jj=0, enabled, valover, overflow, latched, polarity;
  char *pol = NULL;

  if((id<0) || (s32p[id] == NULL)) {
    printf("scale32Status: ERROR : Scaler %d not initialized \n",id);
    return;
  }

  /* Get all Scaler information */
  lock_key = intLock();

  enabled  = s32p[id]->inhib;
  latched  = s32p[id]->latch;
  polarity = s32p[id]->polar;
  valover = s32p[id]->val_over;
  s32p[id]->val_over = SCAL_ALL_MASK;
  overflow = s32p[id]->over;
  s32p[id]->val_over = valover;
  
  intUnlock(lock_key);

  /* Print info to standard out */
  printf("Scaler ID %d Status :\n",id);
  printf("            0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 ");
  printf("16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31\n");

  /* Check if Enabled */
  printf(" Enabled :");
  for(ii=31;ii>=0;ii--) {
    jj = (~enabled)&(1<<ii) ? 1 : 0 ;
    if(ii == 0) 
      printf("%3d\n",jj);
    else
      printf("%3d",jj);
  }

  /* Check if Overflowed */
  printf(" OverFlow:");
  for(ii=31;ii>=0;ii--) {
    jj = (overflow)&(1<<ii) ? 1 : 0 ;
    if(ii == 0) 
      printf("%3d\n",jj);
    else
      printf("%3d",jj);
  }

  /* Check if Latched */
  printf(" Latched :");
  for(ii=31;ii>=0;ii--) {
    jj = (latched)&(1<<ii) ? 1 : 0 ;
    if(ii == 0) 
      printf("%3d\n",jj);
    else
      printf("%3d",jj);
  }


  /* Check Active Edge */
  printf(" Polarity:");
  for(ii=31;ii>=0;ii--) {
    pol = (polarity)&(1<<ii) ? " -\0" : " +\0" ;
    if(ii == 0) 
      printf("%3s\n",pol);
    else
      printf("%3s",pol);
  }

}





