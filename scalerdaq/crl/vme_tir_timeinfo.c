#define ROL_NAME__ "VME1"
#define MAX_EVENT_LENGTH 1024
#define MAX_EVENT_POOL   500
#define VME
#define TRIG_SUPV
#define INIT_NAME vme_tir_timeinfo__init
#include <rol.h>
#include <VME_source.h>
#define TIR_ADDR 0x0ed0
struct vme_ts2 *tsP;
extern int bigendian_out;
extern unsigned int vxTicks;
static void __download()
{
    daLogMsg("INFO","Readout list compiled %s", DAYTIME);
#ifdef POLLING___
   rol->poll = 1;
#endif
    *(rol->async_roc) = 0; /* Normal ROC */
  {  /* begin user */
unsigned long res;
  scale32Init(0x08000000,0x10000,1);
  bigendian_out = 0;
  tirInit(TIR_ADDR);
    daLogMsg("INFO","User Download TIR Executed");

  }  /* end user */
} /*end download */     

static void __prestart()
{
CTRIGINIT;
    *(rol->nevents) = 0;
  {  /* begin user */
    daLogMsg("INFO","User Prestart TIR Executed");

    daLogMsg("INFO","Entering User Trigger Prestart");

    VME_INIT;
    CTRIGRSA(VME,1,usrtrig,usrtrig_done);
    CRTTYPE(1,VME,1);
  sysClkRateSet(100);
    daLogMsg("INFO","Set Clk Rate to 100Hz");

  scale32Enable(0,0);
  scale32CLR(0,0,1);
  scale32Clear(0,1);
    daLogMsg("INFO","Ending TIR Prestart");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
    *(rol->nevents) = 0;
    rol->recNb = 0;
} /*end prestart */     

static void __end()
{
  {  /* begin user */
unsigned long trig_count;
  scale32Disable(0,0);
  CDODISABLE(VME,1,0);
    daLogMsg("INFO","User End Executed VME TIR");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /* end end block */

static void __pause()
{
  {  /* begin user */
  CDODISABLE(VME,1,0);
    daLogMsg("INFO","User Pause Executed");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /*end pause */
static void __go()
{

  {  /* begin user */
int currtime ;
    daLogMsg("INFO","Entering User Go");

  CDOENABLE(VME,1,0);
currtime = vxTicks ;
{/* inline c-code */
   

  printf("vxTicks = %d\n", currtime);
   
 }/*end inline c-code */
    daLogMsg("INFO","Finish User Go");

  scale32Clear(0,1);
  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
}

void usrtrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
    long EVENT_LENGTH;
  {  /* begin user */
unsigned long ii, event_ty, event_no;
int currtime ;
 event_ty = EVTYPE;
 event_no = *rol->nevents;
 rol->dabufp = (long *) 0;
    CEOPEN(EVTYPE, BT_UI4);
  currtime=time();
{/* inline c-code */
 
//  *rol->dabufp++ = currtime;
  *rol->dabufp++ = vxTicks;
  for (ii=0;ii<32;ii++) {
    *rol->dabufp++ = s32p[0]->scaler[_I_(ii)];
//    printf("value %02d = %02d\n",ii,s32p[0]->scaler[_I_(ii)]);
  }
  
 
 }/*end inline c-code */
    CECLOSE;
  }  /* end user */
} /*end trigger */

void usrtrig_done()
{
  {  /* begin user */
  scale32Clear(0,1);
  scale32Clear(0,1);
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

