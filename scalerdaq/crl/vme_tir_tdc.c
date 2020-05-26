#define ROL_NAME__ "VME1"
#define MAX_EVENT_LENGTH 1024
#define MAX_EVENT_POOL   500
#define VME
#define TRIG_SUPV
#define INIT_NAME vme_tir_tdc__init
#include <rol.h>
#include <VME_source.h>
#define TIR_ADDR 0x0ed0
#define NTDC 3
struct vme_ts2 *tsP;
extern int bigendian_out;
extern struct tdc_data *tdcd[];
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
    daLogMsg("INFO","Ending TIR Prestart");

  tdcInit(0x431000,0x10000,NTDC);
  sysClkRateSet(100);
  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
    *(rol->nevents) = 0;
    rol->recNb = 0;
} /*end prestart */     

static void __end()
{
  {  /* begin user */
unsigned long trig_count;
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
  int ii;
    daLogMsg("INFO","Entering User Go");

  CDOENABLE(VME,1,0);
    daLogMsg("INFO","Finish User Go");

{/* inline c-code */
 
for (ii=0;ii<NTDC;ii++) {
  tdcEclEx(ii);
}
 
 }/*end inline c-code */
  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
}

void usrtrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
    long EVENT_LENGTH;
  {  /* begin user */
unsigned long ii, event_ty, event_no;
 int unreadfifo, id;
 event_ty = EVTYPE;
 event_no = *rol->nevents;
 rol->dabufp = (long *) 0;
    CEOPEN(EVTYPE, BT_UI4);
  *rol->dabufp++ = vxTicks;
{/* inline c-code */
 
for (id=0;id<NTDC;id++){
  *rol->dabufp++ = tdcp[id]->baseAddr;

  unreadfifo=CodaGetFIFOUnReadPoint(id);

  *rol->dabufp++ =CodaGetFIFOUnReadPoint(id);

//The buffer limitation is (256-3=253)
  if(unreadfifo<=516){
    for (ii=1;ii<=unreadfifo;ii++){
      *rol->dabufp++ = (tdcd[id]->data[0])&0xffffffff;
    }
  }

//  unreadfifo=CodaGetFIFOUnReadPoint(id);
//  if(unreadfifo<=20){
//      for (ii=1;ii<=unreadfifo;ii++){
//        *rol->dabufp++ = (tdcd[id]->data[0])&0xffffffff;
//      }
//  }
}

 
 }/*end inline c-code */
    CECLOSE;
  }  /* end user */
} /*end trigger */

void usrtrig_done()
{
  {  /* begin user */
int ii;
{/* inline c-code */
 

for (ii=0;ii<NTDC;ii++) {
  tdcEclEx(ii);
}

 
 }/*end inline c-code */
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

