/******************************************************************************
*
*  scale32Lib.h  -  Driver library header file for readout of a Scale 32
*                   Counter using a VxWorks 5.2 or later based single board 
*                   computer.
*
*  Author: David Abbott and Balinda Moreland
*          Jefferson Lab Data Acquisition Group
*          June 2000
*/
typedef unsigned int UINT32;
typedef int STATUS;
/* Define a Structure for access to  Scaler */
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

/* Define default interrupt vector/level */
#define SCAL_INT_VEC      0xaa
#define SCAL_VME_INT_LEVEL   4

/* Define offset from Base address where anything useful is */
#define SCAL_BASE_ADDR_OFFSET      0x8000

/* Define ETAT register bits */
#define SCAL_ETAT_CRST0           0x01
#define SCAL_ETAT_CRST1           0x02
#define SCAL_ETAT_CRST2           0x04
#define SCAL_ETAT_CRST3           0x08
#define SCAL_ETAT_PAT             0x10
#define SCAL_ETAT_GOVER           0x20

/* Define Masks */
#define SCAL_ALL_MASK          0xffffffff

/* Define useful macros */
#define _I_(ii)     ~(ii)&(31)
#define BITSWAP(a)  {int xx; \
                     unsigned long b=0; \
                     for(xx=0;xx<32;xx++) \
                       { if((a&(1<<xx)) > 0) \
                           {b |= (1<<(31-xx));} \
                       } \
                     a=b; }
#define SCALER_LATCH_ALL(id)     s32p[id]->latch = 0xffffffff
#define SCALER_UNLATCH_ALL(id)   s32p[id]->latch = 0;


/* Function Prototypes */
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
/*void scale32Delay(int id, int time, int latch)
STATUS vxMemProbe(char* adrs, int mode, int length, char* pVal);
 */



 


