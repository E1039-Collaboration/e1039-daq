/* # 1 "gen_int_list.c" */
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
void gen_int_list__init (rolp)
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
	  strcpy(rol_name__, "GEN_USER" );
	  rolp->listName = rol_name__;
	  printf("Init - Initializing new rol structures for %s\n",rol_name__);
	  strcpy(name, rolp->listName);
	  strcat(name, ":pool");
	  rolp->pool  = partCreate(name, 40000  , 1024 ,1);
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
/* # 6 "gen_int_list.c" 2 */
/* # 1 "GEN_source.h" 1 */
static int GEN_handlers,GENflag;
static int GEN_isAsync;
static unsigned int *GENPollAddr = ((void *) 0) ;
static unsigned int GENPollMask;
static unsigned int GENPollValue;
static unsigned long GEN_prescale = 1;
static unsigned long GEN_count = 0;
/* # 1 "sis3610.h" 1 */
struct s3610_struct {
  volatile unsigned int csr;
  volatile unsigned int id;
  volatile unsigned int d_out;
  volatile unsigned int jk_out;
  volatile unsigned int d_in;
  volatile unsigned int l_in;
  volatile unsigned int blank[18];
  volatile unsigned int reset;
};
/* # 33 "GEN_source.h" 2 */
extern int s3610IntID;
extern int s3610IntLevel;
extern int s3610IntVec;
extern struct s3610_struct *s3610p[];
/* # 1 "sis3600.h" 1 */
struct s3600_struct {
  volatile unsigned int csr;        
  volatile unsigned int id;         
  volatile unsigned int fc;         
  volatile unsigned int ofr;        
  volatile unsigned int wf;         
  volatile unsigned int blank1[3];  
  volatile unsigned int cf;         
  volatile unsigned int vmenc;      
  volatile unsigned int ennc;       
  volatile unsigned int dinc;       
  volatile unsigned int bcf;        
  volatile unsigned int bvmenc;     
  volatile unsigned int bennc;      
  volatile unsigned int bdinc;      
  volatile unsigned int blank2[4];  
  volatile unsigned int enfc;       
  volatile unsigned int difc;       
  volatile unsigned int blank3[2];  
  volatile unsigned int reset;      
  volatile unsigned int blank4;     
  volatile unsigned int geop;       
};
struct s3600_data {
  volatile unsigned int data[64];   
};
/* # 38 "GEN_source.h" 2 */
  extern struct s3600_data *s3600d[];
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
/* # 40 "GEN_source.h" 2 */
extern struct scale32_struct *s32p[20];        
/* # 1 "dsTDC.h" 1 */
struct tdc_struct{
  volatile unsigned int csr;               
  volatile unsigned int baseAddr;          
  volatile unsigned int exMemDataLatch;     
  volatile unsigned int fifoReadWrite;    
 };
struct tdc_data{
  volatile unsigned int data[64];          
};
/* # 42 "GEN_source.h" 2 */
extern struct tdc_data *tdcd[];
extern struct tdc_struct *tdcp[];
extern void tdcEclEx(int id);
/* # 1 "dsLatchCard.h" 1 */
struct latch_struct{
  volatile unsigned int csr;              
  volatile unsigned int id;               
  volatile unsigned int fifoStatusReg;    
  volatile unsigned int outputPulseReg;   
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
struct latch_data{
  volatile unsigned int data[64];   
};
/* # 47 "GEN_source.h" 2 */
extern struct latch_data *latchd[];
void
GEN_int_handler()
{
  s3610p[ s3610IntID ]->id &= ~0x00000800  ;
  theIntHandler(GEN_handlers);                    
}
static void
gentriglink(int code, VOIDFUNCPTR isr)
{
  int ii, int_vec;
  int_vec =  0xe0 ;
  s3610IntSet(0,5 ,int_vec,0);
  s3610CSR(0,(0x400  | 0x800 ));
  if(code == 2) {
    for(ii=0;ii<4;ii++) {
      int_vec = 0xe0  + (1<<ii);
      if(intDisconnect(int_vec) != 0) {
	printf("ERROR disconnecting Interrupt\n");
      }
      if((intConnect(int_vec,isr,code)) != 0) {
	printf("ERROR in intConnect()\n");
      }
    }
  }else{
    int_vec = 0xe0  + 1;
    if(intDisconnect(int_vec) != 0) {
      printf("ERROR disconnecting Interrupt\n");
    }
    if((intConnect(int_vec,isr,code)) != 0) {
      printf("ERROR in intConnect()\n");
    }
  }
}
static void 
gentenable(int code, int intMask)
{
  s3610IntEnable(intMask);
}
static void 
gentdisable(int code, int intMask)
{
  s3610IntDisable(intMask);
}
static unsigned int
genttype(int code)
{
  unsigned int tt=0;
  if(code == 2) {
    tt = (s3610CSR(0) & 0x000000f0 )>>4 ;
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
/* # 7 "gen_int_list.c" 2 */
int count=0 ;
int ev_type ;
int Isquared_sum=0 ;
int Isum=0 ;
int Nevts=0 ;
extern int bigendian_out;
extern unsigned int vxTicks;
unsigned long testgrass=0;
static void __download()
{
    daLogMsg("INFO","Readout list compiled %s", DAYTIME);
    *(rol->async_roc) = 0;  
  {   
  s3610Init(0x2800 ,0,0);
  scale32Init(0x08000000,0x10000,3 );
bigendian_out = 0;
    daLogMsg("INFO","User Download Executed");
  }   
}       
static void __prestart()
{
{ dispatch_busy = 0; bzero((char *) evMasks, sizeof(evMasks)); bzero((char *) syncTRtns, 
sizeof(syncTRtns)); bzero((char *) ttypeRtns, sizeof(ttypeRtns)); bzero((char *) Tcode, sizeof(Tcode)); 
wrapperGenerator = 0; theEvMask = 0; currEvMask = 0; trigId = 1; poolEmpty = 0; __the_event__ = (DANODE *) 0; 
input_event__ = (DANODE *) 0; } ;     *(rol->nevents) = 0;
  {   
unsigned long jj, adc_id;
int inscale ;
    daLogMsg("INFO","Entering User Prestart");
    { GEN_handlers =0;GEN_isAsync = 0;GENflag = 0;} ;
{ void usrtrig ();void usrtrig_done (); doneRtns[trigId] = (FUNCPTR) ( usrtrig_done ) ; 
trigRtns[trigId] = (FUNCPTR) ( usrtrig ) ; Tcode[trigId] = ( 1 ) ; ttypeRtns[trigId] = genttype ; {printf("linking async GEN trigger to id %d 
\n", trigId ); GEN_handlers = ( trigId );GEN_isAsync = 1;gentriglink( 1 ,GEN_int_handler);} 
;trigId++;} ;     {evMasks[ 1 ] |= (1<<( GEN_handlers ));} ;
    daLogMsg("INFO","User Prestart Executed");
{ 
for (inscale=0;inscale< 3 ;inscale++) {
  scale32Enable(inscale,0);
  scale32CLR(inscale,0,1);
}
 } 
    daLogMsg("INFO","User Prestart DONE");
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
int inscale ;
{ 
 for (inscale=0;inscale< 3 ;inscale++) {
     scale32Disable(inscale,0);
 }
 } 
    daLogMsg("INFO","User End Executed");
   gentdisable(  1   ,   1   );  ;
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
   gentdisable(  1   ,   1   );  ;
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
int currtime ;
  int ii;
   gentenable(  1   ,   1   );  ;
    daLogMsg("INFO","Entering User Go");
  s3610Status(0,0);
    daLogMsg("INFO","2Entering User Go");
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
 int ii,jj;
 int inscale;
  rol->dabufp = (long *) 0;
{	{if(__the_event__ == (DANODE *) 0 && rol->dabufp == ((void *) 0) ) { {{ ( __the_event__ ) = 0; if (( 
&( rol->pool ->list) )->c){ ( &( rol->pool ->list) )->c--; ( __the_event__ ) = ( &( rol->pool 
->list) )->f; ( &( rol->pool ->list) )->f = ( &( rol->pool ->list) )->f->n; }; if (!( &( rol->pool ->list) 
)->c) { ( &( rol->pool ->list) )->l = 0; }} ;} ; if(__the_event__ == (DANODE *) 0) { logMsg ("TRIG ERROR: no pool buffer 
available\n"); return; } rol->dabufp = (long *) &__the_event__->length; if (input_event__) { 
__the_event__->nevent = input_event__->nevent; } else { __the_event__->nevent = *(rol->nevents); } } } ; 
StartOfEvent[event_depth__++] = (rol->dabufp); if(input_event__) {	*(++(rol->dabufp)) = (( EVTYPE ) << 16) | (( 0x01 ) << 8) | 
(0xff & (input_event__->nevent));	} else {	*(++(rol->dabufp)) = (syncFlag<<24) | (( EVTYPE ) << 16) 
| (( 0x01 ) << 8) | (0xff & *(rol->nevents));	}	((rol->dabufp))++;} ; {	long *StartOfBank; StartOfBank = (rol->dabufp); *(++(rol->dabufp)) = ((( EVTYPE ) << 16) | ( 
0x01 ) << 8) | ( 0 );	((rol->dabufp))++; ;   *rol->dabufp++ = 0xe906f002;
{ 
  *rol->dabufp++ = 0x08010000;
  for (ii=17;ii<32;ii++) {
    *rol->dabufp++ = s32p[0]->scaler[~( ii )&(31) ];
  }
  *rol->dabufp++ = s32p[2]->scaler[~( 16 )&(31) ];	
  if(s32p[0]->scaler[~( 21 )&(31) ]<1&&s32p[0]->scaler[~( 22 )&(31) ]<1){  
     Isum += s32p[0]->scaler[~( 27 )&(31) ];
Isquared_sum += (s32p[0]->scaler[~( 27 )&(31) ])*(s32p[0]->scaler[~( 27 )&(31) ]);      Nevts++;
  } 
  if(s32p[0]->scaler[~( 21 )&(31) ]>0){ 
       printf ("This is a BOS event\n");
  	for(inscale=1;inscale< 3 ;inscale++){
  	    scale32Enable(inscale,0);
  	    scale32Clear(inscale,1);
	}
  }
  if(s32p[2]->scaler[~( 16 )&(31) ]>0){ 
   printf ("This is a EOS event\n");
  	for(inscale=1;inscale< 3 ;inscale++){
    	    scale32Disable(inscale,0);
 	}
	for(ii=1;ii< 3 ;ii++){
		*rol->dabufp++ = 0x08010000+ (ii*0x10000);
	  	for (jj=0;jj<32;jj++) {
    		    *rol->dabufp++ = s32p[ii]->scaler[~( jj )&(31) ];
  		}
  }
	*rol->dabufp++ = 0xe906f007;
	*rol->dabufp++ = Isum;
	*rol->dabufp++ = Isquared_sum;
	*rol->dabufp++ = Nevts;
	Isum = 0;
	Isquared_sum = 0;
	Nevts=0;
	for(inscale=1;inscale< 3 ;inscale++){
		scale32Enable(inscale,0);	
  		scale32Clear(inscale,1);	
        }
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
int inscale;
int ii;
  s3610IntAck(1 );
{ 
  scale32Enable(0,0);
  scale32Clear(0,1);
 } 
  }   
}  
static void __status()
{
  {   
  }   
}  
