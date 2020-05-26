/******************************************************************************
*
* Header file for use General USER defined rols with CODA crl (version 2.0)
* 
*   This file implements use of the SIS 3610 Module as a trigger interface
*   (substitute for the JLAB TI Board)
*
*    Supports to Modes of operation
*      code = 1  - Control Input 1 only generates a trigger (type 1 only)
*      code = 2  - Control Inputs 1-4 all will generate a trigger
*                  Trigger type will be determined by input pattern
*
*                             DJA   Nov 2000
*
*******************************************************************************/
#ifndef __GEN_ROL__
#define __GEN_ROL__

static int GEN_handlers,GENflag;
static int GEN_isAsync;
static unsigned int *GENPollAddr = NULL;
static unsigned int GENPollMask;
static unsigned int GENPollValue;
static unsigned long GEN_prescale = 1;
static unsigned long GEN_count = 0;

#define GEN_LEVEL    5
#define GEN_VEC      0xe0



/* Put any global user defined variables needed here for GEN readout */
#include "sis3610.h"
extern int s3610IntID;
extern int s3610IntLevel;
extern int s3610IntVec;
extern struct s3610_struct *s3610p[];
#include "sis3600.h"
  extern struct s3600_data *s3600d[];
#include "scale32Lib.h"
extern struct scale32_struct *s32p[20];       /* pointers to Scaler memory map */
#include "dsTDC.h"
extern struct tdc_data *tdcd[];
extern struct tdc_struct *tdcp[];
extern void tdcEclEx(int id);

#include "dsLatchCard.h"
extern struct latch_data *latchd[];

#include "/usr/local/coda/2.6.1/extensions/ppcTimer_6100/ppcTimer.h"
extern void ppcTimeBaseGet(PPCTB *tb);
extern unsigned int ppcTimeBaseFreqGet();
extern int ppcTimeBaseFreqSet(unsigned int freq);
extern void ppcTimeBaseZero();
extern double ppcTimeBaseDuration(PPCTB *t1, PPCTB *t2);
extern void ppcUsDelay (unsigned int delay);

void
GEN_int_handler()
{
/* Clear inturrupt source (RORA Mode) */
  S3610_IRQ_DISABLE(s3610IntID);
 

  theIntHandler(GEN_handlers);                   /* Call our handler */
}



/*----------------------------------------------------------------------------
  gen_trigLib.c -- Dummy trigger routines for GENERAL USER based ROLs

 File : gen_trigLib.h

 Routines:
           void gentriglink();       link interrupt with trigger
	   void gentenable();        enable trigger
	   void gentdisable();       disable trigger
	   char genttype();          return trigger type 
	   int  genttest();          test for trigger  (POLL Routine)
------------------------------------------------------------------------------*/


static void
gentriglink(int code, VOIDFUNCPTR isr)
{
  int ii, int_vec;

  int_vec =  S3610_INT_VEC;

  /* Connect interrupt with defaults */
  s3610IntSet(0,S3610_VME_INT_LEVEL,int_vec,0);

  /* Clear Latch Status Strobe Bits */
  s3610CSR(0,(S3610_CNTL_CLR_LS0 | S3610_CNTL_CLR_LS1));

  /* Connect the ISR */

  if(code == 2) {
    /*    s3610CSR(0,S3610_CNTL_SET_LS0); */
    for(ii=0;ii<4;ii++) {
      int_vec = S3610_INT_VEC + (1<<ii);

#ifdef VXWORKSPPC
      if(intDisconnect(int_vec) != 0) {
	printf("ERROR disconnecting Interrupt\n");
      }
#endif
      if((intConnect(int_vec,isr,code)) != 0) {
	printf("ERROR in intConnect()\n");
      }
    }

  }else{
    int_vec = S3610_INT_VEC + 1;

#ifdef VXWORKSPPC
    if(intDisconnect(int_vec) != 0) {
      printf("ERROR disconnecting Interrupt\n");
    }
#endif
    if((intConnect(int_vec,isr,code)) != 0) {
      printf("ERROR in intConnect()\n");
    }
  }

}

static void 
gentenable(int code, int intMask)
{

#ifdef POLLING
  GENflag = 1;
#else
  s3610IntEnable(intMask);
#endif

}

static void 
gentdisable(int code, int intMask)
{

#ifdef POLLING
  GENflag = 0;
#else
  s3610IntDisable(intMask);
#endif

}

static unsigned int
genttype(int code)
{
  unsigned int tt=0;

  if(code == 2) {
    tt = (s3610CSR(0) & S3610_USER_OUTPUT_MASK)>>4 ;
  } else {
    tt = 1;
  }

  return(tt);
}

static int 
genttest(int code)
{
  unsigned int ret;

  if((GENflag>0) && (GENPollAddr > 0)) {
    GEN_count++;
    
    ret = (*GENPollAddr)&(GENPollMask);
    if(ret == GENPollValue)
      return(1);
    else
      return(0);

  } else {
    return(0);
  }
}



/* Define CODA readout list specific Macro routines/definitions */

#define GEN_TEST  genttest

#define GEN_INIT { GEN_handlers =0;GEN_isAsync = 0;GENflag = 0;}

#define GEN_ASYNC(code,id)  {printf("linking async GEN trigger to id %d \n",id); \
			       GEN_handlers = (id);GEN_isAsync = 1;gentriglink(code,GEN_int_handler);}

#define GEN_SYNC(code,id)   {printf("linking sync GEN trigger to id %d \n",id); \
			       GEN_handlers = (id);GEN_isAsync = 0;}

#define GEN_SETA(code) GENflag = code;

#define GEN_SETS(code) GENflag = code;

#define GEN_ENA(code,val) gentenable(code, val);

#define GEN_DIS(code,val) gentdisable(code, val);

#define GEN_CLRS(code) GENflag = 0;

#define GEN_GETID(code) GEN_handlers

#define GEN_TTYPE genttype

#define GEN_START(val)	 {;}

#define GEN_STOP(val)	 {;}

#define GEN_ENCODE(code) (code)


#endif

