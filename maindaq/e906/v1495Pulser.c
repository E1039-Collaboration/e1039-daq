/******************************************************************************
*
* v1495Lib.c  -  Driver library for operation of v1495 general purpose vme board
*                  using a VxWorks 5.4 or later based Single Board computer. 
*
*  Author: Shiuan-Hal, Shiu
*          
*          August 2010
*
*  Revision  1.0 - Initial Revision
*  Revision  1.1 - Modify some function in 2011
*
*/

#include "vxWorks.h"
#include "stdio.h"
#include "logLib.h"
#include "taskLib.h"
#include "intLib.h"
#include "iv.h"
#include "semLib.h"
#include "vxLib.h"
#include "unistd.h"

#include "time.h"		/*for the purpose to using nanosleep */
#include "stdlib.h"		/*random */
#include "ctype.h"  
#include "ioLib.h"  
#include "loadLib.h"  
#include "memLib.h"  
#include "remLib.h"  
#include "string.h"  
#include "symLib.h"  
#include "ftpLib.h"  
#include "sysLib.h"  
#include "sysSymTbl.h"  
#include "usrLib.h"  
#include "version.h"  
#include "fppLib.h"  
#include "dirent.h"  
#include "sys/stat.h"  
#include "errnoLib.h"  
#include "fcntl.h"  
#include "fioLib.h"  
#include "shellLib.h"  
#include "hostLib.h"  
#include "pathLib.h"  
#include "iosLib.h"  
#include "taskArchLib.h"  
#include "kernelLib.h"  
#include "usrLib.h"  
#include "objLib.h"  
#include "private/cplusLibP.h"  
#include "netDrv.h"  
#include "smObjLib.h"  
#include "private/taskLibP.h"  
#include "private/funcBindP.h"  
#include "nfsLib.h"  
#include "dosFsLib.h"  
/* Include definitions */
#include "v1495Pulser.h"
#define MAXLINE         80  /* max line length for input to 'm' routine */  
/* Define external Functions */
IMPORT STATUS sysBusToLocalAdrs (int, char *, char **);
IMPORT STATUS intDisconnect (int);
IMPORT STATUS sysIntEnable (int);
IMPORT STATUS sysIntDisable (int);

/* Define global variables */
int Nv1495 = 0;			/* Number of v1495s in Crate */
volatile struct v1495_csr *v1495_p[v1495_MAX_BOARDS];	/* pointers to v1495 memory map */
volatile struct Short_data *Short_d[v1495_MAX_BOARDS];
volatile struct Long_data *Long_d[v1495_MAX_BOARDS];
struct timespec tim, tim2;



//#define max1 128



/*******************************************************************************
* v1495Init - Initialize v1495 Library. 
*
* RETURNS: OK, or ERROR if the address is invalid or board is not present.
********************************************************************************/

STATUS
v1495PulserInit (UINT32 addr, UINT32 addr_inc, int nmod)
{
  int ii, res, errFlag=0 ,rdata= 0;
  unsigned int laddr;


  addr = addr + 0x101a;
  /* Check for valid address */
  if (addr == 0)
    {
      printf
	("v1495Init: ERROR: Must specify a Bus (VME-based A24/32) address for the v1495\n");
      return (ERROR);
    }
  else if (addr > 0xffffffff)
    {				/* more then A32 Addressing */
      printf ("v1495Init: ERROR: Addressing not supported for this v1495\n");
      return (ERROR);
    }
  else
    {
      if ((addr_inc == 0) || (nmod == 0))
	nmod = 1;		/* assume only one v1495 to initialize */
      /* get the v1495 address */
      res = sysBusToLocalAdrs (0x39, (char *) addr, (char **) &laddr);	/*0x39 is A32 non privileged data access */
      /*res,resl,resm,rest stands for different memory blocks */

      if (res != 0)		/*sysBusToLocalAdrs will return 0 if it work */
	{
	  printf
	    ("v1495PulserInit: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",
	     addr);
	  printf ("v1495PulserInit: ERROR res=%d\n", res);
	  return (ERROR);
	}
    }

  Nv1495 = 0;
  /*for multiple boards */
  for (ii = 0; ii < nmod; ii++)
    {
      v1495_p[ii] = (struct v1495_csr *) (laddr + ii * addr_inc);
    
      /* Check if Board exists at that address */

      res =
	vxMemProbe ((char *) &(v1495_p[ii]->reg[0]), VX_READ, 2,
		    (char *) &rdata);
      /*vxMemProbe will return 0 if it work */
      if (res < 0)
	{
	  printf ("v1495: ERROR: No addressable board at addr=0x%x\n",
		  (UINT32) v1495_p[ii]);
	  v1495_p[ii] = NULL;
	  errFlag = 1;
	  break;
	}

    
      Nv1495++;
      printf ("Initialized v1495 ID %d at address 0x%08x \n", ii,
	      (UINT32) v1495_p[ii]);
    
    }

  if (errFlag > 0)
    {
      printf ("v1495Init: ERROR: Unable to initialize all Modules\n");
      
      if (Nv1495 > 0)
	{
	  printf ("v1495Init: %d v1495(s) successfully initialized\n", Nv1495);
	}
	  return (ERROR);
    }else
    {
      return (OK);
    }

}


void v1495_RunMode_Start(int id)    {v1495_p[id]->reg[0] |=  v1495_START_RUN;}
void v1495_RunMode_Auto(int id)     {v1495_p[id]->reg[0] &= ~v1495_START_RUN;}
void v1495_PulseMode_Rotate(int id) {v1495_p[id]->reg[0] &= ~v1495_MULTI_HITS;}
void v1495_LEMO_Enable(int id)      {v1495_p[id]->reg[8] =1;}
void v1495_LEMO_Disable(int id)     {v1495_p[id]->reg[8] =0;}
void v1495_Enable_Cable1(int id)    {v1495_p[id]->reg[11] =0xffff;}
void v1495_Enable_Cable2(int id)    {v1495_p[id]->reg[12] =0xffff;}
void v1495_Enable_Cable3(int id)    {v1495_p[id]->reg[15] =0xffff;}
void v1495_Enable_Cable4(int id)    {v1495_p[id]->reg[16] =0xffff;}
void v1495_Disable_Cable1(int id)    {v1495_p[id]->reg[11] =0x0;}
void v1495_Disable_Cable2(int id)    {v1495_p[id]->reg[12] =0x0;}
void v1495_Disable_Cable3(int id)    {v1495_p[id]->reg[15] =0x0;}
void v1495_Disable_Cable4(int id)    {v1495_p[id]->reg[16] =0x0;}


/*******************************************************************************
* v1495Status - The status of v1495
*
* RETURNS: prints some status of v1495 on screen
********************************************************************************/
void
v1495_PulserStatus (int id)
{

  if ((id < 0) || (v1495_p[id] == NULL))
    {
      printf ("v1495Status: ERROR : v1495 id %d not initialized \n", id);
      return;
    }

  /* Get info from Module register */

    printf ("reg MODE  , 0x%x= 0x%x\n",0x101a, v1495_p[id]->reg[0]);
    printf ("reg EXIT  , 0x%x= 0x%x\n",0x101c, v1495_p[id]->reg[1]);
    printf ("reg TRIG  , 0x%x= 0x%x\n",0x1028, v1495_p[id]->reg[7]);
    printf ("reg AUX   , 0x%x= 0x%x\n",0x102a, v1495_p[id]->reg[8]);
    printf ("reg CABLE1, 0x%x= 0x%x\n",0x1030, v1495_p[id]->reg[11]);
    printf ("reg CABLE2, 0x%x= 0x%x\n",0x1032, v1495_p[id]->reg[12]);
    printf ("reg CABLE3, 0x%x= 0x%x\n",0x1038, v1495_p[id]->reg[15]);
    printf ("reg CABLE4, 0x%x= 0x%x\n",0x103a, v1495_p[id]->reg[16]);
  return;
}

void
v1495_ExitPulser(int id){
  v1495_p[id]->reg[1]=1;
  tim.tv_sec = 0;
  tim.tv_nsec = 1000;
  nanosleep(&tim , &tim2);
  v1495_p[id]->reg[1]=0;
  return;
}
void v1495_PulseMode_MultiHit(int id, int nPulse,int nCycle) {
  v1495_p[id]->reg[0] |= v1495_MULTI_HITS;
  v1495_p[id]->reg[0]= ( v1495_p[id]->reg[0] &0xff00 )+ (0xf0 & (nCycle<<4) )+(0xf & nPulse); 
  return;
}

void v1495_Trig_Setup(int id, int nCycle){
  v1495_p[id]->reg[7]= ( v1495_p[id]->reg[7] & 0xff00)+ (0xff &nCycle);
  return;
}

void v1495_T2P_Setup(int id, int nCycle){
  v1495_p[id]->reg[7]= ( v1495_p[id]->reg[7] & 0xff)+ ((0xff &nCycle)<<8);
  return;
}

void v1495_Enable_AllCh(int id){
  v1495_Enable_Cable1(id); 
  v1495_Enable_Cable2(id); 
  v1495_Enable_Cable3(id); 
  v1495_Enable_Cable4(id); 
  return;
}

void v1495_Disble_AllCh(int id){
  v1495_Disable_Cable1(id);
  v1495_Disable_Cable2(id);
  v1495_Disable_Cable3(id);
  v1495_Disable_Cable4(id);
  return;
}

void v1495_Wr(int ireg, int ivalue){
  v1495_p[0]->reg[ireg]= ivalue;
  return;
}

int v1495_Rd(int ireg){
  return(v1495_p[0]->reg[ireg]);
}







/******************************************************************************* 
 * 
 * getHex - convert a hex string into a 64 bit value 
 * 
 * This function converts a string containing hex digits into a binary value. 
 * The values is returned in the locations pointed to by pHiValue and pLoValue. 
 * These values can be concatenated together to produce a 64 bit value. 
 * 
 * RETURNS: OK or ERROR 
 * 
 * INTERNALS: Ideally this function should be replaced by a version of scanf 
 * that supports long longs. 
 * This function is derived from scanNum in fioLib.c 
 * 
 * NOMANUAL 
 */  
  
LOCAL STATUS getHex  
(  
 char *pStr,     /* string to parse */  
 ULONG *pHiValue,    /* where to store high part of result */  
 ULONG *pLoValue /* where to store low part of result */  
   )  
{  
  int         dig;                    /* current digit */  
  BOOL        neg     = FALSE;        /* negative or positive? */  
  FAST char * pCh     = pStr;         /* pointer to current character */  
  FAST int    ch      = *pCh;         /* current character */  
  FAST ULONG  hiValue = 0;        /* high part of value accumulator */  
  FAST ULONG  loValue = 0;        /* low part of value accumulator */  
   
  /* check for sign */  
   
  if (ch == '+' || (neg = (ch == '-')))  
    ch = *++pCh;  
   
  /* check for optional or 0x */  
   
  if (ch == '0')  
    {  
      ch = *++pCh;  
      if (ch == 'x' || ch == 'X')  
        ch = *++pCh;  
    }  
   
  /* scan digits */  
  while (ch != '\0')  
    {  
      if (isdigit (ch))  
	dig = ch - '0';  
      else if (islower (ch))  
	dig = ch - 'a' + 10;  
      else if (isupper (ch))  
	dig = ch - 'A' + 10;  
      else  
	break;  
   
      if (dig >= 16)  
	break;  
   
      /* assume that accumulator parts are 32 bits long */  
      hiValue = (hiValue * 16) + (loValue >> 28);  
      loValue = loValue * 16 + dig;  
   
      ch = *++pCh;  
    }  
   
   
  /* check that we scanned at least one character */  
   
  if (pCh == pStr) {  
    return (ERROR);  
  }  
   
  /* return value to caller */  
   
  if (neg) {  
    /* assume 2's complement arithmetic */  
    hiValue = ~hiValue;  
    loValue = ~loValue;  
    if (++loValue == 0)  
      hiValue++;  
  }  
  
  *pHiValue = hiValue;  
  *pLoValue = loValue;  
  
  return (ch != '\0' ? ERROR : OK);  
}  


STATUS Set_Mem( void *adrs,     /* address to change */  
	     int  width      /* width of unit to be modified (1, 2, 4, 8) */  ){
  char *filename="SetMem.txt";
  FILE *pFile;
  int readover=0;
  static void *lastAdrs;  /* last location modified */  
  static int  lastWidth = 2;  /* last width - default to 2 */  
  ULONG loValue;      /* low part of value found in line */  
  unsigned int SubAddr,DataInput;
  unsigned int laddr;
  int res;  
  pFile = fopen (filename,"r");


  if (adrs == 0){
      printf
        ("v1495Init: ERROR: Must specify a Bus (VME-based A24/32) address for the v1495\n");
      return (ERROR);
  }
  else if (adrs > 0xffffffff){                           /* more then A32 Addressing */
      printf ("v1495Init: ERROR: Addressing not supported for this v1495\n");
      return (ERROR);
  }
  else{

      res = sysBusToLocalAdrs (0x39, (char *) adrs, (char **) &laddr);  /*0x39 is A32 non privileged data access */
      /*res,resl,resm,rest stands for different memory blocks */

      if (res != 0)             /*sysBusToLocalAdrs will return 0 if it work */
        {
          printf
            ("v1495PulserInit: ERROR in sysBusToLocalAdrs(0x39,0x%x,&laddr) \n",
             adrs);
          printf ("v1495PulserInit: ERROR res=%d\n", res);
          return (ERROR);
        }
  }
  
  if (width != 0)     /* check valid width and set the default */  
    {  

      if ( width != 2 && width != 4) width = 4;  
      lastWidth = width;  
    }  
  
  /* round down to appropriate boundary */  



  
  do{  
    /* prompt for substitution according to width */  
    fscanf(pFile,"%x %x\n",&SubAddr,&DataInput);
    lastAdrs =laddr +SubAddr;  
    loValue=DataInput;
       
    /* get substitution value: 
     *   skip empty lines (CR only); 
     *   quit on end of file or invalid input; 
     *   otherwise put specified value at address 
     */  
    
    switch (lastWidth)  
      {  
      case 2:  
	*(USHORT *)lastAdrs = (USHORT) loValue;  
	break;  
      case 4:  
	*(ULONG *)lastAdrs = (ULONG) loValue;  
	  break;  
      default:  
	*(ULONG *)lastAdrs = (ULONG) loValue;  
	break;  
      }  
    switch (lastWidth)  
      {  
      case 2:  
	printf ("%08x: %08x :  %04x", (int) lastAdrs,(int) adrs+SubAddr, *(USHORT *)lastAdrs);  
	break;  
      case 4:  
	printf ("%08x: %08x :  %08x", (int) lastAdrs,(int) adrs+SubAddr, *(ULONG *)lastAdrs);  
	break;  
      default:  
	printf ("%08x:  %08x-", (int) lastAdrs,(int) adrs+SubAddr, *(ULONG *)lastAdrs);  
	break;  
      }  

    if(feof(pFile)!=0) {
      //      printf("read over,event number up now=%d\n",event-1);
      fflush(stdout);
      readover=1;

    }
    printf ("\n");  
  }while(readover==0);
  
  
  printf ("\n");  
  fclose (pFile);
  
}



STATUS Rd_Mem (void *addr,     /* address to change */
	int N_rows,
	int N_read,
	int  width     /* width of unit to be modified (1, 2, 4, 8) */    ) 
{
  int res,ii;
  USHORT *tmpShort;       /* temporary short word pointer */  
  ULONG *tmpLong;     /* temporary long word pointer */  
  FILE *pFile;
  unsigned int laddr;
  static int  lastWidth = 2;  /* last width - default to 2 */
  char *filename="OutputMem.txt";
  static void *lastAdrs = 0; /* last location displayed */  

  pFile = fopen (filename,"w+");
  res = sysBusToLocalAdrs (0x39, (char *) addr, (char **) &laddr);  /*0x39 is A32 non privileged data access */
 
  printf("addr=%x, laddr=%x\n",addr,laddr);
  if (width != 0)     /* check valid width and set the default */
    {
      if ( width != 2 && width != 4)
        width = 2;
      lastWidth = width;
    }


  lastAdrs = (void *)((int)laddr);  
  switch (lastWidth)
    {
    case 2:
      Short_d[0]= (struct Short_data *) (laddr);
      break;
    case 4:
    default:
      Long_d[0]= (struct Long_data *) (laddr);
      break;
    } 
  
  for(ii=0;ii<N_read;ii++){
    if (ii%N_rows==0) fprintf (pFile,"\n") ;
    if (ii%N_rows==0) printf ("\n") ;
    //    printf("lastAdrs=%x\n",lastAdrs);  
    switch (lastWidth)
      {
      case 2:
	fprintf(pFile,"%04x ",Short_d[0]->reg[ii] );
	printf("%04x.",Short_d[0]->reg[ii] );  
        break;
      case 4:
      default:
	fprintf(pFile,"%04x ",Long_d[0]->reg[ii] );
	printf("%04x.",Long_d[0]->reg[ii] );  
	break;
      }

  }////for N_read
  fclose (pFile);

}


