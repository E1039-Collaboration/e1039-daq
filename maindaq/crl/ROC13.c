/* # 1 "ROC13.c" */
/* # 1 "/usr/local/coda/2.6.1/common/include/rol.h" 1 */
typedef void (*VOIDFUNCPTR) ();
typedef int (*FUNCPTR) ();
typedef unsigned long time_t;
typedef struct semaphore *SEM_ID;
static void __download ();
static void __prestart ();
static void __end ();
static void __pause ();
static void __go ();
static void __done ();
static void __status ();
static int theIntHandler ();
/* # 1 "/usr/local/coda/2.6.1/common/include/libpart.h" 1 */
/* # 1 "/usr/local/coda/2.6.1/common/include/mempart.h" 1 */
typedef struct danode			       
{
  struct danode         *n;	               
  struct danode         *p;	               
  struct rol_mem_part   *part;	                   
  int                    fd;		       
  char                  *current;	       
  unsigned long          left;	               
  unsigned long          type;                 
  unsigned long          source;               
  void                   (*reader)();          
  long                   nevent;               
  unsigned long          length;	       
  unsigned long          data[1];	       
} DANODE;
typedef struct alist			       
{
  DANODE        *f;		               
  DANODE        *l;		               
  int            c;			       
  int            to;
  void          (*add_cmd)(struct alist *li);      
  void          *clientData;                 
} DALIST;
typedef struct rol_mem_part *ROL_MEM_ID;
typedef struct rol_mem_part{
    DANODE	 node;		 
    DALIST	 list;		 
    char	 name[40];	 
    void         (*free_cmd)();  
    void         *clientData;     
    int		 size;		 
    int		 incr;		 
    int		 total;		 
    long         part[1];	 
} ROL_MEM_PART;
/* # 62 "/usr/local/coda/2.6.1/common/include/mempart.h" */
/* # 79 "/usr/local/coda/2.6.1/common/include/mempart.h" */
/* # 106 "/usr/local/coda/2.6.1/common/include/mempart.h" */
extern void partFreeAll();  
extern ROL_MEM_ID partCreate (char *name, int size, int c, int incr);
/* # 21 "/usr/local/coda/2.6.1/common/include/libpart.h" 2 */
/* # 67 "/usr/local/coda/2.6.1/common/include/rol.h" 2 */
/* # 1 "/usr/local/coda/2.6.1/common/include/rolInt.h" 1 */
typedef struct rolParameters *rolParam;
typedef struct rolParameters
  {
    char          *name;	      
    char          tclName[20];	       
    char          *listName;	      
    int            runType;	      
    int            runNumber;	      
    VOIDFUNCPTR    rol_code;	      
    int            daproc;	      
    void          *id;		      
    int            nounload;	      
    int            inited;	      
    long          *dabufp;	      
    long          *dabufpi;	      
    ROL_MEM_PART  *pool;              
    ROL_MEM_PART  *output;	      
    ROL_MEM_PART  *input;             
    ROL_MEM_PART  *dispatch;          
    volatile ROL_MEM_PART  *dispQ;    
    unsigned long  recNb;	      
    unsigned long *nevents;           
    int           *async_roc;         
    char          *usrString;	      
    void          *private;	      
    int            pid;               
    int            poll;              
    int primary;		      
    int doDone;			      
  } ROLPARAMS;
/* # 70 "/usr/local/coda/2.6.1/common/include/rol.h" 2 */
extern ROLPARAMS rolP;
static rolParam rol;
extern int global_env[];
extern long global_env_depth;
extern char *global_routine[100];
extern long data_tx_mode;
extern int cacheInvalidate();
extern int cacheFlush();
static int syncFlag;
static int lateFail;
/* # 1 "/usr/local/coda/2.6.1/common/include/BankTools.h" 1 */
static int EVENT_type;
long *StartOfEvent[32 ],event_depth__, *StartOfUEvent;
/* # 78 "/usr/local/coda/2.6.1/common/include/BankTools.h" */
/* # 95 "/usr/local/coda/2.6.1/common/include/BankTools.h" */
/* # 107 "/usr/local/coda/2.6.1/common/include/BankTools.h" */
/* # 120 "/usr/local/coda/2.6.1/common/include/BankTools.h" */
/* # 152 "/usr/local/coda/2.6.1/common/include/BankTools.h" */
/* # 186 "/usr/local/coda/2.6.1/common/include/BankTools.h" */
/* # 198 "/usr/local/coda/2.6.1/common/include/BankTools.h" */
/* # 217 "/usr/local/coda/2.6.1/common/include/BankTools.h" */
/* # 92 "/usr/local/coda/2.6.1/common/include/rol.h" 2 */
/* # 1 "/usr/local/coda/2.6.1/common/include/trigger_dispatch.h" 1 */
static unsigned char dispatch_busy; 
static int intLockKey,trigId;
static int poolEmpty;
static unsigned long theEvMask, currEvMask, currType, evMasks[16 ];
static VOIDFUNCPTR wrapperGenerator;
static FUNCPTR trigRtns[32 ], syncTRtns[32 ], doneRtns[32 ], ttypeRtns[32 ];
static unsigned long Tcode[32 ];
static DANODE *__the_event__, *input_event__, *__user_event__;
/* # 50 "/usr/local/coda/2.6.1/common/include/trigger_dispatch.h" */
/* # 141 "/usr/local/coda/2.6.1/common/include/trigger_dispatch.h" */
static void cdodispatch()
{
  unsigned long theType,theSource;
  int ix, go_on;
  DANODE *theNode;
  dispatch_busy = 1;
  go_on = 1;
  while ((rol->dispQ->list.c) && (go_on)) {
{ ( theNode ) = 0; if (( &rol->dispQ->list )->c){ ( &rol->dispQ->list )->c--; ( theNode ) = ( 
&rol->dispQ->list )->f; ( &rol->dispQ->list )->f = ( &rol->dispQ->list )->f->n; }; if (!( &rol->dispQ->list 
)->c) { ( &rol->dispQ->list )->l = 0; }} ;     theType = theNode->type;
    theSource = theNode->source;
    if (theEvMask) { 
      if ((theEvMask & (1<<theSource)) && (theType == currType)) {
	theEvMask = theEvMask & ~(1<<theSource);
	intUnlock(intLockKey); ;
	(*theNode->reader)(theType, Tcode[theSource]);
	intLockKey = intLock(); ;
	if (theNode)
	if (!theEvMask) {
	 if (wrapperGenerator) {event_depth__--; *StartOfEvent[event_depth__] = (long) (((char *) 
(rol->dabufp)) - ((char *) StartOfEvent[event_depth__]));	if ((*StartOfEvent[event_depth__] & 1) != 0) { 
(rol->dabufp) = ((long *)((char *) (rol->dabufp))+1); *StartOfEvent[event_depth__] += 1; }; if 
((*StartOfEvent[event_depth__] & 2) !=0) { *StartOfEvent[event_depth__] = *StartOfEvent[event_depth__] + 2; (rol->dabufp) = 
((long *)((short *) (rol->dabufp))+1);; };	*StartOfEvent[event_depth__] = ( 
(*StartOfEvent[event_depth__]) >> 2) - 1;}; ;	 	 rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ; 	}
      } else {
	{if(! ( &rol->dispQ->list )->c ){( &rol->dispQ->list )->f = ( &rol->dispQ->list )->l = ( 
theNode );( theNode )->p = 0;} else {( theNode )->p = ( &rol->dispQ->list )->l;( &rol->dispQ->list 
)->l->n = ( theNode );( &rol->dispQ->list )->l = ( theNode );} ( theNode )->n = 0;( &rol->dispQ->list 
)->c++;	if(( &rol->dispQ->list )->add_cmd != ((void *) 0) ) (*(( &rol->dispQ->list )->add_cmd)) (( 
&rol->dispQ->list )); } ; 	go_on = 0;
      }
    } else { 
      if ((1<<theSource) & evMasks[theType]) {
	currEvMask = theEvMask = evMasks[theType];
	currType = theType;
      } else {
        currEvMask = (1<<theSource);
      }
      if (wrapperGenerator) {
	(*wrapperGenerator)(theType);
      }
      (*(rol->nevents))++;
      intUnlock(intLockKey); ;
      (*theNode->reader)(theType, Tcode[theSource]);
      intLockKey = intLock(); ;
      if (theNode)
	{ if (( theNode )->part == 0) { free( theNode ); theNode = 0; } else { {if(! ( & theNode ->part->list 
)->c ){( & theNode ->part->list )->f = ( & theNode ->part->list )->l = ( theNode );( theNode )->p = 0;} 
else {( theNode )->p = ( & theNode ->part->list )->l;( & theNode ->part->list )->l->n = ( theNode );( & 
theNode ->part->list )->l = ( theNode );} ( theNode )->n = 0;( & theNode ->part->list )->c++;	if(( & 
theNode ->part->list )->add_cmd != ((void *) 0) ) (*(( & theNode ->part->list )->add_cmd)) (( & theNode 
->part->list )); } ; } if( theNode ->part->free_cmd != ((void *) 0) ) { (*( theNode ->part->free_cmd)) ( theNode 
->part->clientData); } } ;       if (theEvMask) {
	theEvMask = theEvMask & ~(1<<theSource);
      } 
      if (!theEvMask) {
	rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ;       }
    }  
  }
  dispatch_busy = 0;
}
static int theIntHandler(int theSource)
{
  if (theSource == 0) return(0);
  {  
    DANODE *theNode;
    intLockKey = intLock(); ;
{{ ( theNode ) = 0; if (( &( rol->dispatch ->list) )->c){ ( &( rol->dispatch ->list) )->c--; ( 
theNode ) = ( &( rol->dispatch ->list) )->f; ( &( rol->dispatch ->list) )->f = ( &( rol->dispatch ->list) 
)->f->n; }; if (!( &( rol->dispatch ->list) )->c) { ( &( rol->dispatch ->list) )->l = 0; }} ;} ;     theNode->source = theSource;
    theNode->type = (*ttypeRtns[theSource])(Tcode[theSource]);
    theNode->reader = trigRtns[theSource]; 
{if(! ( &rol->dispQ->list )->c ){( &rol->dispQ->list )->f = ( &rol->dispQ->list )->l = ( 
theNode );( theNode )->p = 0;} else {( theNode )->p = ( &rol->dispQ->list )->l;( &rol->dispQ->list 
)->l->n = ( theNode );( &rol->dispQ->list )->l = ( theNode );} ( theNode )->n = 0;( &rol->dispQ->list 
)->c++;	if(( &rol->dispQ->list )->add_cmd != ((void *) 0) ) (*(( &rol->dispQ->list )->add_cmd)) (( 
&rol->dispQ->list )); } ;     if (!dispatch_busy)
      cdodispatch();
    intUnlock(intLockKey); ;
  }
}
static int cdopolldispatch()
{
  unsigned long theSource, theType;
  int stat = 0;
  DANODE *theNode;
  if (!poolEmpty) {
    for (theSource=1;theSource<trigId;theSource++){
      if (syncTRtns[theSource]){
	if ( theNode = (*syncTRtns[theSource])(Tcode[theSource])) {
	  stat = 1;
	  {  
	    intLockKey = intLock(); ;
	    if (theNode) 
	 {{ ( theNode ) = 0; if (( &( rol->dispatch ->list) )->c){ ( &( rol->dispatch ->list) )->c--; ( 
theNode ) = ( &( rol->dispatch ->list) )->f; ( &( rol->dispatch ->list) )->f = ( &( rol->dispatch ->list) 
)->f->n; }; if (!( &( rol->dispatch ->list) )->c) { ( &( rol->dispatch ->list) )->l = 0; }} ;} ; 	    theNode->source = theSource; 
	    theNode->type = (*ttypeRtns[theSource])(Tcode[theSource]); 
	    theNode->reader = trigRtns[theSource]; 
	 {if(! ( &rol->dispQ->list )->c ){( &rol->dispQ->list )->f = ( &rol->dispQ->list )->l = ( 
theNode );( theNode )->p = 0;} else {( theNode )->p = ( &rol->dispQ->list )->l;( &rol->dispQ->list 
)->l->n = ( theNode );( &rol->dispQ->list )->l = ( theNode );} ( theNode )->n = 0;( &rol->dispQ->list 
)->c++;	if(( &rol->dispQ->list )->add_cmd != ((void *) 0) ) (*(( &rol->dispQ->list )->add_cmd)) (( 
&rol->dispQ->list )); } ; 	    if (!dispatch_busy) 
	      cdodispatch();
	    intUnlock(intLockKey); ;
	  }
	}
      }
    }   
  } else {
    stat = -1;
  }
  return (stat);
}
/* # 99 "/usr/local/coda/2.6.1/common/include/rol.h" 2 */
static char rol_name__[40];
static char temp_string__[132];
static void __poll()
{
    {cdopolldispatch();} ;
}
void ROC13__init (rolp)
     rolParam rolp;
{
      if ((rolp->daproc != 7 )&&(rolp->daproc != 6 )) 
	printf("rolp->daproc = %d\n",rolp->daproc);
      switch(rolp->daproc) {
      case 0 :
	{
	  char name[40];
	  rol = rolp;
	  rolp->inited = 1;
	  strcpy(rol_name__, "VME1" );
	  rolp->listName = rol_name__;
	  printf("Init - Initializing new rol structures for %s\n",rol_name__);
	  strcpy(name, rolp->listName);
	  strcat(name, ":pool");
	  rolp->pool  = partCreate(name, 49152  , 400 ,1);
	  if (rolp->pool == 0) {
	    rolp->inited = -1;
	    break;
	  }
	  strcpy(name, rolp->listName);
	  strcat(name, ":dispatch");
	  rolp->dispatch  = partCreate(name, 0, 32, 0);
	  if (rolp->dispatch == 0) {
	    rolp->inited = -1;
	    break;
	  }
	  strcpy(name, rolp->listName);
	  strcat(name, ":dispQ");
	  rolp->dispQ = partCreate(name, 0, 0, 0);
	  if (rolp->dispQ == 0) {
	    rolp->inited = -1;
	    break;
	  }
	  rolp->inited = 1;
	  printf("Init - Done\n");
	  break;
	}
      case 9 :
	  rolp->inited = 0;
	break;
      case 1 :
	__download();
	break;
      case 2 :
	__prestart();
	break;
      case 4 :
	__pause();
	break;
      case 3 :
	__end();
	break;
      case 5 :
	__go();
	break;
      case 6 :
	__poll();
	break;
      case 7 :
	__done();
	break;
      default:
	printf("WARN: unsupported rol action = %d\n",rolp->daproc);
	break;
      }
}
/* # 7 "ROC13.c" 2 */
/* # 1 "/usr/local/coda/2.6.1/common/include/VME_source.h" 1 */
static int VME_handlers,VMEflag;
static int VME_isAsync;
static unsigned long VME_prescale = 1;
static unsigned long VME_count = 0;
extern int sysBusToLocalAdrs(int space,long *localA,long **busA);
struct vme_tir {
    unsigned short tir_csr;
    unsigned short tir_vec;
    unsigned short tir_dat;
    unsigned short tir_oport;
    unsigned short tir_iport;
  };
struct vme_scal {
    unsigned short reset;
    unsigned short blank1[7];
    unsigned short bim[8];
    unsigned short blank2[16];
    unsigned long preset[16];
    unsigned long scaler[16];
    unsigned short blank3[29];
    unsigned short id[3];
  };
volatile struct vme_ts  *ts;
struct vme_tir *tir[2];
struct vme_scal *vscal[32];
struct vme_scal *vlscal[32];
volatile unsigned long *tsmem;
unsigned long ts_memory[4096];
/* # 1 "scale32Lib.h" 1 */
typedef unsigned int UINT32;
typedef int STATUS;
struct scale32_struct {
  volatile unsigned long scaler[32];
  volatile unsigned long over;
  volatile unsigned long input;
  volatile unsigned long clr[4];
  volatile unsigned long polar;
  volatile unsigned long val_over;
  volatile unsigned long test;
  volatile unsigned long etat;
  volatile unsigned long inhib;
  volatile unsigned long latch;
};
STATUS scale32Init (UINT32 addr, UINT32 addr_inc, int nscalers);
void scale32Print(int id, int latch);
void scale32Clear(int id, int creg);
UINT32 scale32CLR(int id, int creg, UINT32 cmask);
UINT32 scale32Input(int id, int pattern);
void scale32Disable(int id, unsigned long imask);
void scale32Enable(int id, unsigned long imask);
void scale32Test(int id, unsigned long tmask);
void scale32Latch(int id, unsigned long lmask);
int scale32Read(int id, UINT32 rmask, UINT32 *data, UINT32 latch);
void scale32Status(int id, int sflag);
/* # 95 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
extern struct scale32_struct *s32p[20];
/* # 1 "/usr/local/coda/2.6.1/extensions/e906/dsTDC.h" 1 */
void tdcInit(unsigned long, unsigned long, int);
int CodaGetFIFOUnReadPoint(int);
void tdcEclEx(int id);
struct tdc_struct{
  volatile unsigned int csr;               
  volatile unsigned int baseAddr;          
  volatile unsigned int exMemDataLatch;     
  volatile unsigned int fifoReadWrite;    
 };
struct tdc_data{
  volatile unsigned int data[64];          
};
/* # 99 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
extern struct tdc_data *tdcd[];
extern struct tdc_struct *tdcp[];
extern void tdcEclEx(int id);
/* # 1 "/usr/local/coda/2.6.1/extensions/e906/DslTdc.h" 1 */
struct dsTdc2_struct{
  volatile unsigned int csr;              
  volatile unsigned int id;               
  volatile unsigned int fifoStatusReg;    
  volatile unsigned int csr2;             
  volatile unsigned int fifoTestEnable;   
  volatile unsigned int fifoTestDisable;  
  volatile unsigned int blank1[2];        
  volatile unsigned int clr;              
  volatile unsigned int blank2;           
  volatile unsigned int trigEnable;       
  volatile unsigned int trigDisable;      
  volatile unsigned int blank3[8];        
  volatile unsigned int reserved1;        
  volatile unsigned int reserved2;        
  volatile unsigned int blank4[2];        
  volatile unsigned int reset;            
  volatile unsigned int blank5;           
  volatile unsigned int genOutputPulse;   
  volatile unsigned int dummy[37];        
  volatile unsigned int Event[64];        
};
struct dsTdc2_data{
  volatile unsigned int data[64];   
};
/* # 104 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
extern struct dsTdc2_data *dsTdcd[];
extern struct dsTdc2_struct *dsTdcp[];
/* # 1 "/usr/local/coda/2.6.1/extensions/e906/CR_Read.h" 1 */
STATUS CR_Init (UINT32 addr, UINT32 addr_inc, int nmod);
void CR_Status(int id, int iword);
void CR_DataInit(int id, int csr);
void CR_FifoRead(int id, int ii);
struct Read_reg_struct{
  volatile unsigned int reg[64];              
};
struct Read_dp_data{
  volatile unsigned int dp[32767 ];              
};
struct Read_scalar_struct{
  volatile unsigned int scalar[512];              
};
struct Read_data{
  volatile unsigned int data[1024 *2];   
};
/* # 108 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
extern struct Read_reg_struct *CR_p[];
extern struct Read_data *CR_d[];
extern struct Read_scalar_struct*CR_s[];
/* # 1 "/usr/local/coda/2.6.1/extensions/e906/SL_ScalerLatcher.h" 1 */
STATUS SL_Init (UINT32 addr);
void SL_Status();
void SL_DataInit();
void SL_Scalar_Switch(int scalarID);
void SL_WR_Reg(int regaddr, int reg_value);
int SL_RD_Reg(int regaddr);
void SL_ScalarDisplay();
void SL_DataDisplay(int part);
struct SL_csr{
  volatile unsigned int csr[8];              
};
struct SL_data{
  volatile unsigned int data[512 ];   
};
/* # 113 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
extern struct SL_data *SL_d;
extern struct SL_csr *SL_p;
int Clear64[64];
void Clear64Init()
{
  int iclean=0;
  for (iclean=0;iclean<64;iclean++){
    Clear64[iclean]=0;
  }
}
/* # 1 "/usr/local/coda/2.6.1/extensions/e906/v1495Pulser.h" 1 */
struct v1495_csr{
  volatile unsigned short reg[8];   
 };
/* # 127 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
extern struct v1495_csr  *v1495_p[];
/* # 1 "/usr/local/coda/2.6.1/extensions/ppcTimer_6100/ppcTimer.h" 1 */
typedef struct ppcTB {
  unsigned int tbH;
  unsigned int tbL;
} PPCTB;
void ppcTimeBaseGet(PPCTB *tb);
unsigned int ppcTimeBaseFreqGet();
int ppcTimeBaseFreqSet(unsigned int freq);
void ppcTimeBaseZero();
double ppcTimeBaseDuration(PPCTB *t1, PPCTB *t2);
void ppcUsDelay (unsigned int delay);
/* # 131 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
extern void ppcTimeBaseGet(PPCTB *tb);
extern unsigned int ppcTimeBaseFreqGet();
extern int ppcTimeBaseFreqSet(unsigned int freq);
extern void ppcTimeBaseZero();
extern double ppcTimeBaseDuration(PPCTB *t1, PPCTB *t2);
extern void ppcUsDelay (unsigned int delay);
/* # 1 "/usr/local/coda/2.6.1/common/include/tsUtil.h" 1 */
volatile struct vme_ts2 {
    volatile unsigned int csr;       
    volatile unsigned int csr2;       
    volatile unsigned int trig;
    volatile unsigned int roc;
    volatile unsigned int sync;
    volatile unsigned int trigCount;
    volatile unsigned int trigData;
    volatile unsigned int lrocData;
    volatile unsigned int prescale[8];
    volatile unsigned int timer[5];
    volatile unsigned int intVec;
    volatile unsigned int rocBufStatus;
    volatile unsigned int lrocBufStatus;
    volatile unsigned int rocAckStatus;
    volatile unsigned int userData[2];
    volatile unsigned int state;
    volatile unsigned int test;
    volatile unsigned int blank1;
    volatile unsigned int scalAssign;
    volatile unsigned int scalControl;
    volatile unsigned int scaler[18];
    volatile unsigned int scalEvent;
    volatile unsigned int scalLive[2];
    volatile unsigned int id;
  } VME_TS2;
struct ts_state {
  char go[12];
  char trigger[12];
  int ready;
  int busy;
  int seq;
  int sync;
  int feb;
  int inhibit;
  int clear;
  int l1e;
  char buffers[8];
  int branch[5];
  int strobe[5];
  int ack[5];
};    
int tsInit(unsigned int addr, int iflag);
int tsMemInit();
int tsLive(int sflag);
void tsLiveClear();
unsigned int tsCsr(unsigned int Val);
unsigned int tsCsr2Set(unsigned int cval);
unsigned int tsCsr2Clear(unsigned int cval);
void tsClearStatus();
unsigned int tsEnableInput(unsigned int Val, int tflag);
unsigned int tsRoc(unsigned char b4,unsigned char b3,unsigned char b2,unsigned char b1); unsigned int tsSync(unsigned int Val);
unsigned int tsPrescale(int Num, unsigned int Val);
unsigned int tsTimerWrite(int Num, unsigned int Val);
unsigned int tsMemWrite(int Num, unsigned int Val);
void tsGo(int iflag);
void tsStop(int iflag);
void tsReset(int iflag);
unsigned int tsStatus (int iflag);
void tsTimerStatus();
unsigned int tsState(int iflag);
unsigned int tsScalRead(int Num, int sflag);
void tsScalClear(unsigned int mask);
void tsScalLatch();
void tsScalClearAll();
unsigned int tsFifoRead(int rflag);
unsigned int tsFifoCount();
/* # 145 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
extern struct vme_ts2 *tsP;
/* # 1 "/usr/local/coda/2.6.1/common/include/vme_triglib.h" 1 */
unsigned int tirVersion = 0;  
int
tirInit(unsigned int tAddr)
{
  int stat = 0;
  unsigned long laddr;
  unsigned short rval;
  if (tAddr == 0) {
    tAddr = 0x0ed0 ;
  }
  stat = sysBusToLocalAdrs(0x29,tAddr,&laddr);
  if (stat != 0) {
     printf("tirInit: ERROR: Error in sysBusToLocalAdrs res=%d \n",stat);
  } else {
     printf("TIR address = 0x%x\n",laddr);
     tir[1] = (struct vme_tir *)laddr;
  }
  stat = vxMemProbe((char *)laddr,0,2,(char *)&rval);
  if (stat != 0) {
    printf("tirInit: ERROR: TIR card not addressable\n");
    return(-1);
  } else {
    tir[1]->tir_csr = 0x80;  
    tirVersion = (unsigned int)rval;
    printf("tirInit: tirVersion = 0x%x\n",tirVersion);
  }
  return(0);
}
static inline void 
vmetriglink(int code, VOIDFUNCPTR isr)
{
  switch (code) {
  case 1 :
    if(tir[1]) {
      tir[1]->tir_csr = 0x80;
      tir[1]->tir_vec = 0xec ;
    }else{
      printf(" vmetriglink: ERROR: tir[1] uninitialized\n");
      return;
    }
    break;
  case 2 :
    if(tsP) {
      tsP->intVec = 0xec ;
    }else{
      printf(" vmetriglink: ERROR: ts uninitialized\n");
      return;
    }    
    break;
  default:
    printf(" vmetriglink: ERROR: source type %d undefined\n",code);
    return;
  }
  if((intDisconnect(((VOIDFUNCPTR *) ( 0xec  )) ) !=0))
     printf("Error disconnecting Interrupt\n");
  intConnect(((VOIDFUNCPTR *) ( 0xec  )) ,isr,0);
}
static inline void 
vmetenable(int code, unsigned int intMask)
{
 int lock_key;
  lock_key = intLock();
 if(code == 1 )
   sysIntEnable(5 );
 if(code == 2 ) {
   tsP->intVec = 0xec ;
   sysIntEnable(5 );
 }
  intEnable(11);          
 if(code == 1 )
   tir[1]->tir_csr = 0x6;
 if(code == 2 )
   tsP->csr2 |= 0x1800;
  intUnlock(lock_key);
/* # 189 "/usr/local/coda/2.6.1/common/include/vme_triglib.h" */
}
static inline void 
vmetdisable(int code,unsigned int intMask)
{
 if(code == 1 )
   tir[1]->tir_csr = 0x80;
 if(code == 2 )
   tsP->csr2 &= ~(0x1800);
}
static inline void 
vmetack(int code, unsigned int intMask)
{
 if(code == 1 )
   tir[1]->tir_dat = 0x8000;
 if(code == 2 )
   tsP->lrocBufStatus = 0x100;
}
static inline unsigned long 
vmettype(int code)
{
 unsigned long tt;
 if(code == 1 ) {
   tt = (((tir[1]->tir_dat)&0x3c)>>2);
   syncFlag = (tir[1]->tir_dat)&0x01;
   lateFail = ((tir[1]->tir_dat)&0x02)>>1;
 }
 if(code == 2 ) {
   tt = (((tsP->lrocData)&0xfc)>>2);
   syncFlag = (tsP->lrocData)&0x01;
   lateFail = ((tsP->lrocData)&0x02)>>1;
 }
  return(tt);
}
static inline int 
vmettest(int code)
{
  unsigned short sval=0;
  unsigned int   lval=0;
 if(code == 1 ) {
   sval = tir[1]->tir_csr;
   if( (sval != 0xffff) && ((sval&0x8000) != 0) ) {
     return (1);
   } else {
     return (0);
   }
 }
 if(code == 2 ) {
   lval = tsP->lrocBufStatus;
   if( (lval != 0xffffffff) && ((lval&0x8000) != 0) ) {
     return (1);
   } else {
     return (0);
   }
 }
  return(0);
}
/* # 149 "/usr/local/coda/2.6.1/common/include/VME_source.h" 2 */
void VME_int_handler()
{
  theIntHandler(VME_handlers);                    
}
/* # 8 "ROC13.c" 2 */
struct vme_ts2 *tsP;
extern unsigned int vxTicks;
int event_no;
int event_ty;
extern int bigendian_out;
int ii;
int ARM_enable = 1;
int UP_Limit = 275;
int Low_Limit = 150;
int TDCHardID[7] = {28, 26, 30, 32, 33, 34, 128};
int TDCBoardID = 0x09000000;
int csr = 0xffff0060;   
int MultiHitSetup = 0x0;
int TimeWindowOn = 1;
int LimitReg;
int TDCSetup;
int nFlushes = 0;
int BeamOn = 1;
static void __download()
{
    daLogMsg("INFO","Readout list compiled %s", DAYTIME);
    *(rol->async_roc) = 0;  
  {   
unsigned long res;
  bigendian_out = 0;
  tirInit(0x0ed0 );
LimitReg = Low_Limit + (UP_Limit << 16) + (TimeWindowOn << 31) + (ARM_enable << 27);   CR_Init(TDCBoardID, 0x1000000, 3 );
    daLogMsg("INFO","User Download TIR Executed");
  }   
}       
static void __prestart()
{
{ dispatch_busy = 0; bzero((char *) evMasks, sizeof(evMasks)); bzero((char *) syncTRtns, 
sizeof(syncTRtns)); bzero((char *) ttypeRtns, sizeof(ttypeRtns)); bzero((char *) Tcode, sizeof(Tcode)); 
wrapperGenerator = 0; theEvMask = 0; currEvMask = 0; trigId = 1; poolEmpty = 0; __the_event__ = (DANODE *) 0; 
input_event__ = (DANODE *) 0; } ;     *(rol->nevents) = 0;
  {   
    daLogMsg("INFO","User Prestart TIR Executed");
    daLogMsg("INFO","Entering User Trigger Prestart");
    { VME_handlers =0;VME_isAsync = 0;VMEflag = 0;} ;
{ void usrtrig ();void usrtrig_done (); doneRtns[trigId] = (FUNCPTR) ( usrtrig_done ) ; 
trigRtns[trigId] = (FUNCPTR) ( usrtrig ) ; Tcode[trigId] = ( 1 ) ; ttypeRtns[trigId] = vmettype ; {printf("Linking async VME trigger to id %d 
\n", trigId ); VME_handlers = ( trigId );VME_isAsync = 1;vmetriglink( 1 ,VME_int_handler);} 
;trigId++;} ;     {evMasks[ 1 ] |= (1<<( VME_handlers ));} ;
    daLogMsg("INFO","Ending TIR Prestart");
  rol->dabufp = (long *) 0;
{	{if(__user_event__ == (DANODE *) 0) { {{ ( __user_event__ ) = 0; if (( &( rol->pool ->list) 
)->c){ ( &( rol->pool ->list) )->c--; ( __user_event__ ) = ( &( rol->pool ->list) )->f; ( &( rol->pool 
->list) )->f = ( &( rol->pool ->list) )->f->n; }; if (!( &( rol->pool ->list) )->c) { ( &( rol->pool ->list) 
)->l = 0; }} ;} ; if(__user_event__ == (DANODE *) 0) { logMsg ("TRIG ERROR: no pool buffer 
available\n"); return; } rol->dabufp = (long *) &__user_event__->length; __user_event__->nevent = -1; } } ; 
StartOfUEvent = (rol->dabufp); *(++(rol->dabufp)) = ((( 132 ) << 16) | ( 0x01 ) << 8) | (0xff & 0 
);	((rol->dabufp))++;} ; { 
    *rol->dabufp++ = rol->pid;
    for(ii = 0; ii < 3 ; ++ii) {
      *rol->dabufp++ = 0xe906f011;  
      *rol->dabufp++ = TDCBoardID + (ii << 24) + TDCHardID[ii];   
      CR_Reset(ii);
      *rol->dabufp++ = csr;   
      *rol->dabufp++ = MultiHitSetup;
      *rol->dabufp++ = LimitReg;
      CR_WR_Reg(ii, 5, LimitReg);
      CR_WR_Reg(ii, 3, MultiHitSetup);  
      CR_WR_Reg(ii, 6, 0);
      DP_Write(ii, 0xe906000f, 0x7ffe, 0x7ffe);  
      if(1  == 1) {
        CR_Scalar_Switch(ii, 1);
        CR_ScalarInit(ii, 1);
        CR_ScalarInit(ii, 0);
      }
    }   
 } 
{ *StartOfUEvent = (long) (((char *) (rol->dabufp)) - ((char *) StartOfUEvent));	if 
((*StartOfUEvent & 1) != 0) { (rol->dabufp) = ((long *)((char *) (rol->dabufp))+1); *StartOfUEvent += 1; }; if 
((*StartOfUEvent & 2) !=0) { *StartOfUEvent = *StartOfUEvent + 2; (rol->dabufp) = ((long *)((short *) 
(rol->dabufp))+1);; };	*StartOfUEvent = ( (*StartOfUEvent) >> 2) - 1; if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __user_event__ );( 
__user_event__ )->p = 0;} else {( __user_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __user_event__ );( &(rol->output->list) )->l = ( __user_event__ );} ( __user_event__ )->n = 
0;( &(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( 
&(rol->output->list) )->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __user_event__ )->part == 0) { free( 
__user_event__ ); __user_event__ = 0; } else { {if(! ( & __user_event__ ->part->list )->c ){( & __user_event__ 
->part->list )->f = ( & __user_event__ ->part->list )->l = ( __user_event__ );( __user_event__ )->p = 0;} else 
{( __user_event__ )->p = ( & __user_event__ ->part->list )->l;( & __user_event__ ->part->list 
)->l->n = ( __user_event__ );( & __user_event__ ->part->list )->l = ( __user_event__ );} ( 
__user_event__ )->n = 0;( & __user_event__ ->part->list )->c++;	if(( & __user_event__ ->part->list 
)->add_cmd != ((void *) 0) ) (*(( & __user_event__ ->part->list )->add_cmd)) (( & __user_event__ 
->part->list )); } ; } if( __user_event__ ->part->free_cmd != ((void *) 0) ) { (*( __user_event__ 
->part->free_cmd)) ( __user_event__ ->part->clientData); } } ;	} __user_event__ = (DANODE *) 0; }; ;     daLogMsg("INFO","Prestart Done.");
  }   
if (__the_event__) rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ;     *(rol->nevents) = 0;
    rol->recNb = 0;
}       
static void __end()
{
  {   
unsigned long trig_count;
{ 
  for(ii = 0; ii < 3 ; ++ii) {
    CR_FastTrigDisable(ii, csr);
    if(1  == 1) {
      CR_Scalar_Switch(ii, 1);  
      CR_ScalarDisplay(ii, 0);  
    }
  }
 } 
   vmetdisable(   1  ,   0  );  ;
    daLogMsg("INFO","User End Executed VME TIR");
  }   
if (__the_event__) rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ; }  
static void __pause()
{
  {   
    daLogMsg("INFO","User Pause Executed");
   vmetdisable(   1  ,   0  );  ;
  }   
if (__the_event__) rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ; }  
static void __go()
{
  {   
   vmetenable(   1  ,   0  );  ;
    daLogMsg("INFO","Entering User Go");
{ 
  BeamOn = 1;
  for(ii = 0; ii < 3 ; ++ii) CR_WR_Reg(ii, 7, 0);
  for(ii = 0; ii < 3 ; ++ii) {
    CR_p[ii]->reg[1] = csr;
    DP_Write(ii, 0xe9068000 + 1542 , 0x7ffe, 0x7ffe);    
    CR_FastTrigEnable(ii, csr);
	if(1  == 1) {
	  CR_ScalarDisplay(ii, 0);  
	  CR_Scalar_Switch(ii, 0);  
	}
  }
 } 
    daLogMsg("INFO","Finish User Go");
  }   
if (__the_event__) rol->dabufp = ((void *) 0) ; if (__the_event__) { if (rol->output) { {if(! ( 
&(rol->output->list) )->c ){( &(rol->output->list) )->f = ( &(rol->output->list) )->l = ( __the_event__ );( 
__the_event__ )->p = 0;} else {( __the_event__ )->p = ( &(rol->output->list) )->l;( &(rol->output->list) 
)->l->n = ( __the_event__ );( &(rol->output->list) )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( 
&(rol->output->list) )->c++;	if(( &(rol->output->list) )->add_cmd != ((void *) 0) ) (*(( &(rol->output->list) 
)->add_cmd)) (( &(rol->output->list) )); } ; } else { { if (( __the_event__ )->part == 0) { free( __the_event__ 
); __the_event__ = 0; } else { {if(! ( & __the_event__ ->part->list )->c ){( & __the_event__ 
->part->list )->f = ( & __the_event__ ->part->list )->l = ( __the_event__ );( __the_event__ )->p = 0;} else {( 
__the_event__ )->p = ( & __the_event__ ->part->list )->l;( & __the_event__ ->part->list )->l->n = ( 
__the_event__ );( & __the_event__ ->part->list )->l = ( __the_event__ );} ( __the_event__ )->n = 0;( & 
__the_event__ ->part->list )->c++;	if(( & __the_event__ ->part->list )->add_cmd != ((void *) 0) ) (*(( & 
__the_event__ ->part->list )->add_cmd)) (( & __the_event__ ->part->list )); } ; } if( __the_event__ 
->part->free_cmd != ((void *) 0) ) { (*( __the_event__ ->part->free_cmd)) ( __the_event__ ->part->clientData); 
} } ;	} __the_event__ = (DANODE *) 0; } if(input_event__) { { if (( input_event__ )->part == 0) { 
free( input_event__ ); input_event__ = 0; } else { {if(! ( & input_event__ ->part->list )->c ){( & 
input_event__ ->part->list )->f = ( & input_event__ ->part->list )->l = ( input_event__ );( input_event__ 
)->p = 0;} else {( input_event__ )->p = ( & input_event__ ->part->list )->l;( & input_event__ 
->part->list )->l->n = ( input_event__ );( & input_event__ ->part->list )->l = ( input_event__ );} ( 
input_event__ )->n = 0;( & input_event__ ->part->list )->c++;	if(( & input_event__ ->part->list )->add_cmd 
!= ((void *) 0) ) (*(( & input_event__ ->part->list )->add_cmd)) (( & input_event__ ->part->list 
)); } ; } if( input_event__ ->part->free_cmd != ((void *) 0) ) { (*( input_event__ 
->part->free_cmd)) ( input_event__ ->part->clientData); } } ; input_event__ = (DANODE *) 0; } {int ix; if 
(currEvMask) {	for (ix=0; ix < trigId; ix++) {	if (currEvMask & (1<<ix)) (*doneRtns[ix])(); } if 
(rol->pool->list.c) { currEvMask = 0; __done(); rol->doDone = 0; } else { poolEmpty = 1; rol->doDone = 1; } } } ; }
void usrtrig(unsigned long EVTYPE,unsigned long EVSOURCE)
{
    long EVENT_LENGTH;
  {   
unsigned long ii, jj, retVal, maxWords, nWords, remBytes;
  int Cnt, totalDMAwords;
  int DP_Bank;
  unsigned int* DMAaddr;
  long tmpaddr1, tmpaddr2;
  event_ty = EVTYPE;
  event_no = *rol->nevents;
  rol->dabufp = (long*)0;
{	{if(__the_event__ == (DANODE *) 0 && rol->dabufp == ((void *) 0) ) { {{ ( __the_event__ ) = 0; if (( 
&( rol->pool ->list) )->c){ ( &( rol->pool ->list) )->c--; ( __the_event__ ) = ( &( rol->pool 
->list) )->f; ( &( rol->pool ->list) )->f = ( &( rol->pool ->list) )->f->n; }; if (!( &( rol->pool ->list) 
)->c) { ( &( rol->pool ->list) )->l = 0; }} ;} ; if(__the_event__ == (DANODE *) 0) { logMsg ("TRIG ERROR: no pool buffer 
available\n"); return; } rol->dabufp = (long *) &__the_event__->length; if (input_event__) { 
__the_event__->nevent = input_event__->nevent; } else { __the_event__->nevent = *(rol->nevents); } } } ; 
StartOfEvent[event_depth__++] = (rol->dabufp); if(input_event__) {	*(++(rol->dabufp)) = (( EVTYPE ) << 16) | (( 0x01 ) << 8) | 
(0xff & (input_event__->nevent));	} else {	*(++(rol->dabufp)) = (syncFlag<<24) | (( EVTYPE ) << 16) 
| (( 0x01 ) << 8) | (0xff & *(rol->nevents));	}	((rol->dabufp))++;} ; {	long *StartOfBank; StartOfBank = (rol->dabufp); *(++(rol->dabufp)) = ((( EVTYPE ) << 16) | ( 
0x01 ) << 8) | ( 0 );	((rol->dabufp))++; ;   *rol->dabufp++ = vxTicks;
{ 
  if(event_ty == 14 && BeamOn == 1) {   
    DP_Bank = ((event_no - 1) & 0xf) << 10;
    for(ii = 0; ii < 3 ; ++ii) {
      if(0  == 1) {
        *rol->dabufp++ = 0xe906f010;  
        *rol->dabufp++ = TDCBoardID + (ii << 24);
        *rol->dabufp++ = CR_d[ii]->data[0];
        maxWords = 257;	 
        DMAaddr = TDCBoardID + (ii << 24) + 0x20000 + (DP_Bank << 2);       
        tmpaddr1 = rol->dabufp;
        tmpaddr2 = DMAaddr;
if(((tmpaddr1 & 4) >> 2) != ((tmpaddr2 & 4) >> 2)) *rol->dabufp++ = 0xe906e906;         retVal = sysVmeDmaSend(rol->dabufp, DMAaddr, maxWords << 2, 0);
        if(retVal < 0) {
          logMsg("ERROR in DMA transfer Initialization 0x%x\n", retVal);
          *rol->dabufp++ = 0xda010bad;
        } else {
          remBytes = sysVmeDmaDone(0, 0);
          if(remBytes < 0) {                     
            logMsg("ERROR during DMA transfer 0x%x\n", remBytes);
            *rol->dabufp++ = 0xda020bad;
          } else if(remBytes == 0) {         
            rol->dabufp += maxWords;
          } else {                             
            nWords = maxWords - (remBytes >> 2);
            rol->dabufp += nWords;
          }
        } 
        *rol->dabufp++ = 0xe906c0da;
      } else if(0  == 2) {
        *rol->dabufp++ = 0xe906f010;  
        *rol->dabufp++ = TDCBoardID + (ii << 24);
        *rol->dabufp++ = CR_d[ii]->data[0];
        *rol->dabufp++ = 0xe906c0da;
      }
    } 
    for(ii = 0; ii < 3 ; ++ii) CR_WR_Reg(ii, 7, event_no);
  } else if(event_ty == 12) {     
    for(ii = 0; ii < 3 ; ++ii) CR_FastTrigDisable(ii, csr);
    if(BeamOn == 1) {
      for(ii = 0; ii < 3 ; ++ii) DP_Write(ii, 0xe9060001, 0x7ffe, 0x7ffe);
    }
    nFlushes = 0;
    BeamOn = 0;
    logMsg("Received EOS event! Will start off-beam transfer...\n");
  } else if(event_ty == 11) {    
    for(ii = 0; ii < 3 ; ++ii) CR_WR_Reg(ii, 7, event_no);
    for(ii = 0; ii < 3 ; ++ii) CR_FastTrigEnable(ii, csr);
    if(BeamOn == 0) {
      for(ii = 0; ii < 3 ; ++ii) DP_Write(ii, 0xe9060000, 0x7ffe, 0x7ffe);
    }
    BeamOn = 1;
    logMsg("Received BOS event! Will start on-beam transfer...\n");
  } else if(event_ty == 10 && nFlushes < 9000  && BeamOn == 0) {
    ++nFlushes;
    for(ii = 0; ii < 3 ; ++ii) {
      if(nFlushes < 9000 ) {
        Cnt = DP_Read(ii, 0);
        *rol->dabufp++ = 0xe906f018;
        *rol->dabufp++ = (((TDCBoardID + (ii << 24)) & 0xffff0000) + Cnt);   
        if(Cnt > 0 && Cnt < 1542  + 5) {
          DMAaddr = TDCBoardID + (ii << 24) + 0x20000;
          tmpaddr1 = rol->dabufp;
          tmpaddr2 = DMAaddr;
if(((tmpaddr1 & 4) >> 2) != ((tmpaddr2 & 4) >> 2)) *rol->dabufp++ = 0xe906e906;           retVal = sysVmeDmaSend(rol->dabufp, DMAaddr, Cnt << 2, 0);
          if(retVal < 0) {
            *rol->dabufp++ = 0xda010bad;
          } else {
            remBytes = sysVmeDmaDone(0, 0);
            if(remBytes < 0) {
              *rol->dabufp++ = 0xda020bad;
            } else if(remBytes == 0) {
              rol->dabufp += Cnt;
            } else {
              nWords = Cnt - (remBytes >> 2);
              rol->dabufp += nWords;
            }
          }
        }
        DP_Write(ii, 0xe9060002, 0x7ffe, 0x7ffe);
      } else {
        DP_Write(ii, 0xe9060003, 0x7ffe, 0x7ffe);
        *rol->dabufp++ = 0xe906f019;
        *rol->dabufp++ = TDCBoardID + (ii << 24);
        *rol->dabufp++ = DP_Read(ii, 0x7ffa);
        DP_Write(ii, 0, 0x7ffa, 0x7ffa);
      }
    }
    *rol->dabufp++ = 0xe906c0da;
  }
 } 
*StartOfBank = (long) (((char *) (rol->dabufp)) - ((char *) StartOfBank));	if ((*StartOfBank 
& 1) != 0) { (rol->dabufp) = ((long *)((char *) (rol->dabufp))+1); *StartOfBank += 1; }; if 
((*StartOfBank & 2) !=0) { *StartOfBank = *StartOfBank + 2; (rol->dabufp) = ((long *)((short *) 
(rol->dabufp))+1);; };	*StartOfBank = ( (*StartOfBank) >> 2) - 1;}; ; {event_depth__--; *StartOfEvent[event_depth__] = (long) (((char *) (rol->dabufp)) - ((char 
*) StartOfEvent[event_depth__]));	if ((*StartOfEvent[event_depth__] & 1) != 0) { 
(rol->dabufp) = ((long *)((char *) (rol->dabufp))+1); *StartOfEvent[event_depth__] += 1; }; if 
((*StartOfEvent[event_depth__] & 2) !=0) { *StartOfEvent[event_depth__] = *StartOfEvent[event_depth__] + 2; (rol->dabufp) = 
((long *)((short *) (rol->dabufp))+1);; };	*StartOfEvent[event_depth__] = ( 
(*StartOfEvent[event_depth__]) >> 2) - 1;}; ;   }   
}  
void usrtrig_done()
{
  {   
  }   
}  
void __done()
{
poolEmpty = 0;  
  {   
   vmetack(   1  ,   0  );  ;
  }   
}  
static void __status()
{
  {   
  }   
}  
