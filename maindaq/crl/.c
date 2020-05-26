#define ROL_NAME__ "VME1"
#define MAX_EVENT_LENGTH 20000
#define MAX_EVENT_POOL   4000
#define VME
#define TRIG_SUPV
#define INIT_NAME __init
#include <rol.h>
#include <VME_source.h>
#define V1495TDC_LVL0_on 0
#define V1495TDC_LVL1_on 0
#define V1495TDC_LVL2_on 0
#define V1495NonBendOff 1
#define PULSER_MODE_on 0
#define V1495splatBlock_on 0
const NV1495L0=2;
const NV1495L1=2;
const NV1495L2=1;
const NV1495 = 5;
int V_DELAY[5];
STATUS err;
const TIR_ADDR = 0x0ed0;
int ii;
struct vme_ts2 *tsP;
extern unsigned int vxTicks;
int event_ty;
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
  tirInit(TIR_ADDR);
{/* inline c-code */
 
  v1495Init(0x04000000,0x100000,NV1495);

 for(ii=0;ii<NV1495;ii++){
  v1495Reload(ii);
 }
 
 }/*end inline c-code */
    daLogMsg("INFO","5 v1495 Boards are Initialized");

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

 rol->dabufp = (long *) 0;
  UEOPEN(132,BT_UI4,0);
{/* inline c-code */
 
  *rol->dabufp++ =rol->pid;

// nwuerfel try old delays -> can we see anything??? might have to scan...
// 128ns wide TDC windows

  V_DELAY[0]=0x0100;
  V_DELAY[1]=0x0100;
  V_DELAY[2]=0x012b;
  V_DELAY[3]=0x012b;
  V_DELAY[4]=0x0121;

/* testing
  V_DELAY[0]=0x0100;
  V_DELAY[1]=0x0100;
  V_DELAY[2]=0x0120;
  V_DELAY[3]=0x0120;
  V_DELAY[4]=0x0120;
*/

// 64ns wide TDC windows
/*
  V_DELAY[0]=0x0233;
  V_DELAY[1]=0x0233;
  V_DELAY[2]=0x0233;
  V_DELAY[3]=0x0233;
  V_DELAY[4]=0x0232;
  V_DELAY[5]=0x0232;
  V_DELAY[6]=0x0232;
  V_DELAY[7]=0x0232;
  V_DELAY[8]=0x0224;
*/

// 32ns wide TDC windows
/*
  V_DELAY[0]=0x032e;
  V_DELAY[1]=0x032e;
  V_DELAY[2]=0x032e;
  V_DELAY[3]=0x032e;
  V_DELAY[4]=0x032d;
  V_DELAY[5]=0x032d;
  V_DELAY[6]=0x032d;
  V_DELAY[7]=0x032d;
  V_DELAY[8]=0x0323;
*/

  for (ii=0;ii<NV1495;ii++){ 
    V_DELAY[ii]=V_DELAY[ii]+0x1000;
    err = v1495Timeset(96,ii);
    /*if(err == ERROR){
        printf("Failed to timeset board %d\n",i);
	return ERROR;
    }*/
    err = v1495TimewindowSet(ii,V_DELAY[ii]);
    /*if(err == ERROR){
        printf("Failed to timewindowset board %d\n",i);
	return ERROR;
    }*/
  } 
 
 }/*end inline c-code */
  UECLOSE;
    daLogMsg("INFO","Prestart  1");

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
    daLogMsg("INFO","User Pause Executed");

  CDODISABLE(VME,1,0);
  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
} /*end pause */
static void __go()
{

  {  /* begin user */
  CDOENABLE(VME,1,0);
    daLogMsg("INFO","Entering User Go");

{/* inline c-code */
 
// for now TDC on LC is bunko
for (ii=0;ii<NV1495-1;ii++){ 
   v1495Run(ii);
}

 
 }/*end inline c-code */
    daLogMsg("INFO","Finish User Go");

  }  /* end user */
    if (__the_event__) WRITE_EVENT_;
}

void usrtrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
    long EVENT_LENGTH;
  {  /* begin user */
unsigned long ii, iii, event_no, data_addr, retVal, maxWords, nWords, remBytes, TmpV1495Count;
 int TmpData;
 long tmpaddr1,tmpaddr2;
 event_ty = EVTYPE;
 event_no = *rol->nevents;
 rol->dabufp = (long *) 0;
    CEOPEN(EVTYPE, BT_UI4);
    CBOPEN(EVTYPE, BT_UI4, 0);
 *rol->dabufp++ = vxTicks;
{/* inline c-code */
 	

// NO CLUE what the event_ty for NIM24 is....
// after speaking with KUN ev_ty 14 is event per event readout
// rest of exp usually runs on buffered mode => readout event ty 10
// ok for debugging but need mem mezz card for ev ty 14

if (event_ty ==14){

  TmpV1495Count=0x1495fa11;
  

   for (ii=0;ii<(NV1495L0+NV1495L1);ii++){ 	
     *rol->dabufp++ = 0x13378eef;
     *rol->dabufp++ = v1495RevisionRead(ii);
     *rol->dabufp++ =   v1495TimewindowRead(ii);  // Delay time, for debug
     TmpV1495Count=v1495TDCcount(ii); 
     *rol->dabufp++ = TmpV1495Count ;       //# ofword in buffer
     *rol->dabufp++ =   v1495CommonstopRead(ii); //trigger timing information
     if (TmpV1495Count!=0xd1ad){      	
       for(iii=0;iii<TmpV1495Count;iii++){
 	 *rol->dabufp++ = v1495TDCReadout(ii,iii);
       }
     }
   }

    ii = NV1495L0 + NV1495L1; 	
     *rol->dabufp++ =0xe906dead;
     *rol->dabufp++ =   v1495RevisionRead(ii);   // broad id read   
     *rol->dabufp++ =   v1495TimewindowRead(ii);  
     TmpV1495Count=v1495TDCcount(ii); 
     *rol->dabufp++ = TmpV1495Count ;       //# ofword in buffer
     *rol->dabufp++ =   v1495CommonstopRead(ii); //trigger timing information
     if (TmpV1495Count!=0xd1ad){      	
       for(iii=0;iii<TmpV1495Count;iii++){
 	 *rol->dabufp++ = v1495TDCReadout(ii,iii);
       }
     }

 *rol->dabufp++ =0x1039c0da;    
}

 
 }/*end inline c-code */
    CBCLOSE;
    CECLOSE;
  }  /* end user */
} /*end trigger */

void usrtrig_done()
{
  {  /* begin user */
{/* inline c-code */
 
if(event_ty ==14){
    for (ii=0;ii<NV1495;ii++){
      v1495Run(ii);
    }
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

