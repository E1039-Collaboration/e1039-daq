/******************************************************************\
*                                                                  *
*  dsTDC.h  - Header for Da-Shung's TDC VME                        *
*                   I/O Register Library                           *
*                                                                  *
*  Author: David Abbott                                            *
*          Jefferson Lab Data Acquisition Group                    *
*          April 2003                                              *
*                                                                  *
*                                                                  *
*  Author: Grass Wang					 	   *
*          July 2010						   *  
*                                                                  *
\******************************************************************/

/* --- Define Structure for access to Local Memory map --- */

struct tdc_struct{
  volatile unsigned int csr;              /*  0x000  Control and Status Register                */
  volatile unsigned int baseAddr;         /*  0x004  Return Base Address Switches              */
  volatile unsigned int exMemDataLatch;    /*  0x008  Return External Memory Data Latch (Last Data Read)*/
  volatile unsigned int fifoReadWrite;   /*  0x00C  Return FIFO Read & Write Pointers. (0xRRRRWWWW)   */
 };

struct tdc_data{
  volatile unsigned int data[64];         /*  0x800~0x1FF  128KB FIFO memory space, (32Kx32)    */
};

#define TDC_MAX_BOARDS     10

/* Define default interrupt vector/level */
#define TDC_INT_VEC      0xe0
#define TDC_VME_INT_LEVEL   5

/* Control/Status Write Register bits */
#define TIME_WIN_REG             0x00ffffff  //read & write
#define RESET_TDC                0x01000000
#define RESET_TIME_COUNT         0x02000000
#define RESET_FIFO_READ_POINT    0x04000000
#define RESET_EVERYTHING         0x07000000
#define ENABLE_EXTERNAL_MEM_TEST 0x08000000 
#define STOP_SET_ECL             0x40000000  //set 0 is NIM, set 1 is ECL
#define STOP_SET_NIM             0x00000000  //set 0 is NIM, set 1 is ECL
#define RUN_TEST_INPUT           0x80000000

/* Control/Status Read Register bits */
#define NUM_OF_EVENT_IN_FIFO     0x3f000000

/*FIFO read and write status*/
#define FIFO_READ_POINT          0xffff0000
#define FIFO_WRITE_POINT         0x0000ffff
