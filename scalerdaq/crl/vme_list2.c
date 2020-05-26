#define ROL_NAME__ "VME1"
#define MAX_EVENT_LENGTH 1024
#define MAX_EVENT_POOL   500
#define VME
#define TRIG_SUPV
#define INIT_NAME vme_list2__init
#include <rol.h>
#include <VME_source.h>
#define TIR_ADDR 0x0ed0
struct vme_ts2 *tsP;
extern int bigendian_out;
static void __download()
{
    daLogMsg("INFO","Readout list compiled %s", DAYTIME);
#ifdef POLLING___
   rol->poll = 1;
#endif
    *(rol->async_roc) = 0; /* Normal ROC */
  {  /* begin user */
unsigned long res;
  bigendian_out = 0;
  tsInit(0,0);
  tirInit(TIR_ADDR);
    daLogMsg("INFO","User Download Executed");

  }  /* end user */
} /*end download */     

static void __prestart()
{
CTRIGINIT;
    *(rol->nevents) = 0;
  {  /* begin user */
    daLogMsg("INFO","User Prestart 2 Executed");

    daLogMsg("INFO","Entering User Trigger Prestart");

    VME_INIT;
    CTRIGRSA(VME,1,usrtrig,usrtrig_done);
    CRTTYPE(1,VME,1);
    daLogMsg("INFO","Starting Trigger Supervisor Prestart========================================");

{/* inline c-code */
 
  /*trigger supervisor code taken from example ts2_util.crl
;
  tsReset(0);                   /* Initialize */
  tsCsr2Set(TS_CSR2_LOCK_ALL);  /* ROC Lock mode on all BRANCHES */
  tsEnableInput(0xfff,0);       /* Enable all Trigger inputs in non-strobe mode */
  tsRoc(0,0,0,1);               /* Enable ACK 0,1 on BRANCH 1  and ACK 2 on Branch 2*/

/* Synchronization programming */
  tsSync(100);                      /* schedule Sync every 100th physics trigger */
  tsCsr2Set(TS_CSR2_ENABLE_SYNC);   /* Enable Scheduled syncs */ 

/* Set nominal Level 2/3 timer values */
  tsTimerWrite(TS_TIMER_L2A,0x05);   /* Level 2 Timer 40ns/count */
  tsTimerWrite(TS_TIMER_L3A,0x05);   /* Level 3 Timer 40ns/count */

/* Front End Busy timer 40ns/count */
  tsTimerWrite(TS_TIMER_FB,250);         /* 250 = 10 microsec */   
  tsCsr2Set(TS_CSR2_ENABLE_FB_TIMER);    /* Enable FEB Timer */

/*  Construct TS memory data  ---  in the following model, all trigger patterns 
    that form the memory address are assigned to trigger class 1.  For those 
    trigger patterns with a single hit, the ROC code is set to be the trigger 
    input number.  Otherwise, the ROC code is set to 0xE.  All LEVEL 1 ACCEPT 
    signals are asserted for every pattern.  */
   
  tsMemInit(0);

/* Fix special cases - both inputs 1 and 2 firing - type 13 (0xd) */
  tsMemWrite(3,0xffd3);

/* Set specific input prescale factors */
  tsPrescale(1,0);
  tsPrescale(2,0);

 
 }/*end inline c-code */
    daLogMsg("INFO","Ending Trigger Supervisor Prestart");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
    *(rol->nevents) = 0;
    rol->recNb = 0;
} /*end prestart */     

static void __end()
{
  {  /* begin user */
  CDODISABLE(VME,1,0);
    daLogMsg("INFO","User End Executed");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /* end end block */

static void __pause()
{
  {  /* begin user */
  tsStop(1);
  CDODISABLE(VME,1,0);
    daLogMsg("INFO","User Pause Executed");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /*end pause */
static void __go()
{

  {  /* begin user */
    daLogMsg("INFO","Entering User Go");

    tsGo(1);
  CDOENABLE(VME,1,0);
  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
}

void usrtrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
    long EVENT_LENGTH;
  {  /* begin user */
unsigned long ii, event_ty, event_no;
 event_ty = EVTYPE;
 event_no = *rol->nevents;
 rol->dabufp = (long *) 0;
    CEOPEN(EVTYPE, BT_UI4);
    CBWRITE32(0xda000022); 
  rol->dabufp += 20;
    CBWRITE32(0xda0000ff); 
    CECLOSE;
  }  /* end user */
} /*end trigger */

void usrtrig_done()
{
  {  /* begin user */
  }  /* end user */
} /*end done */

void __done()
{
poolEmpty = 0; /* global Done, Buffers have been freed */
  {  /* begin user */
  CDOACK(VME,1,0);
  }  /* end user */
} /*end done */

static void __status()
{
  {  /* begin user */
  }  /* end user */
} /* end status */

