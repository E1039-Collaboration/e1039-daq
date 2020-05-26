#define ROL_NAME__ "TS2L"
#define MAX_EVENT_LENGTH 1024
#define MAX_EVENT_POOL   500
/* POLLING_MODE */
#define POLLING___
#define POLLING_MODE
#define VME
#define TRIG_SUPV
#define INIT_NAME vme_ts__init
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
  tsInit(0xed0000,0);
  tirInit(0);
    daLogMsg("INFO","User Download TS Executed");

  }  /* end user */
} /*end download */     

static void __prestart()
{
CTRIGINIT;
    *(rol->nevents) = 0;
  {  /* begin user */
    daLogMsg("INFO","User Prestart TS Executed");

    daLogMsg("INFO","Entering User Trigger Prestart");

    VME_INIT;
    CTRIGRSA(VME,1,ts_trig,ts_trig_done);
    CRTTYPE(1,VME,1);
    daLogMsg("INFO","Starting Trigger Supervisor Prestart========================================");

{/* inline c-code */
 
  /*trigger supervisor code taken from example ts2_util.crl
;
  tsReset(0);                   /* Initialize */
/* ROC lock mode: waits until TS gets a trigger ACK from all TIR to output next trigger */
/* On the TIR, there are 8 pins, suitable for 8 daisy chains per branch.  These pins need to be set to a unique value for each TIR.  The tsROC(Branch4, Branch3, Branch2, Branch1) must be set to the correct value (binary) of the TIR's (ex. 2 TIR on branch 1, with pins 0 and 1 filled, respectively, will be tsROC(0,0,0,3)).   */
//  tsCsr2Set(TS_CSR2_LOCK_ALL);  /* ROC Lock mode on all BRANCHES */
  tsEnableInput(0xfff,0);       /* Enable all Trigger inputs in non-strobe mode */
//  tsRoc(0,0,0,2);               /* Enable ACK 0   on BRANCH 1  and ACK 1 on Branch 2*/
  tsRoc(0,0,0,1);               /* Enable ACK 0   on BRANCH 1  and no ACK on Branch 2*/
//  tsRoc(0,0,0,3);               /* Enable ACK 0   on BRANCH 1  and ACK 1 on Branch 2*/

/* tsSync only matters for buffered mode (not ROC lock mode).  In buffered mode,  each TIR gets a new trigger as soon as the buffer is cleared, so they need to be synched every so often.   */
/* Synchronization programming */
  tsSync(100);                      /* schedule Sync every 100th physics trigger */
  tsCsr2Set(TS_CSR2_ENABLE_SYNC);   /* Enable Scheduled syncs */ 

/* Set nominal Level 2/3 timer values */
  tsTimerWrite(TS_TIMER_L2A,0x05);   /* Level 2 Timer 40ns/count */
  tsTimerWrite(TS_TIMER_L3A,0x05);   /* Level 3 Timer 40ns/count */

/* Front End Busy timer 40ns/count */
  tsTimerWrite(TS_TIMER_FB,250);         /* 250 = 10 microsec */   
//  tsCsr2Set(TS_CSR2_ENABLE_FB_TIMER);    /* Enable FEB Timer */

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
unsigned long trig_count;
  tsStop(1);
    daLogMsg("INFO","User End Executed VME TS");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /* end end block */

static void __pause()
{
  {  /* begin user */
  tsStop(1);
    daLogMsg("INFO","User Pause Executed TS!!!!!!!!!!!!!!!");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /*end pause */
static void __go()
{

  {  /* begin user */
    daLogMsg("INFO","Entering User Go");

  tsGo(1);
  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
}

void ts_trig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
    long EVENT_LENGTH;
  {  /* begin user */
  }  /* end user */
} /*end trigger */

void ts_trig_done()
{
  {  /* begin user */
  }  /* end user */
} /*end done */

void __done()
{
poolEmpty = 0; /* global Done, Buffers have been freed */
  {  /* begin user */
  }  /* end user */
} /*end done */

static void __status()
{
  {  /* begin user */
  }  /* end user */
} /* end status */

