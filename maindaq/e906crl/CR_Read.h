/******************************************************************\
*                                                                  *
*  DslTdc.h  - Header for Da-Shung's Latch-TDC Card VME           *
*                   I/O Register Library                           *
*                                                                  *
*  Author: David Abbott                                            *
*          Jefferson Lab Data Acquisition Group                    *
*          April 2003                                              *
*                                                                  *
*  dsLatchCard.h - Header for Jin's Latch Card                *
*             Multi Event Latch Library                            *
*                                                                  *
*  Author: 
*          Grass Wang					 	   *
*          Jun. 2012						   *  
*          *                                                                  *
* This header file is modified from the dsLatchCard.h              * 
\******************************************************************/

STATUS CR_Init (UINT32 addr, UINT32 addr_inc, int nmod);
void CR_Status(int id, int iword);
void CR_DataInit(int id, int csr);
void CR_FifoRead(int id, int ii);

/* --- Define Structure for access to Local Memory map --- */
#define DSTDC_MAX_BOARDS 20
#define DATA_BUF_SIZE 1024
#define DP_BUF_SIZE 32767
struct Read_reg_struct{
  volatile unsigned int reg[64];             // 0x000  Control and Status Register
  //reg[0]= version number 0x000
  //reg[1]= csr 0x004
  //reg[2]= dummy 0x008=0xaaaaaaaa
  //reg[3]= 0x00c //multihit setup//////reset and clear 0x1001;
  //reg[4]= 0x010 number of events
  //reg[5]= 0x014 time window// high limit/// low limit//
  //reg[6]= 0x018 0->regular DP, 1-> DP test, after read DP address, that DP=0xe906+addr
  //reg[7]= 0x01c CodaEventID

};

struct Read_dp_data{
  volatile unsigned int dp[DP_BUF_SIZE];             // 0x000  Control and Status Register
};


struct Read_scalar_struct{
  volatile unsigned int scalar[512];             // 0x000  Control and Status Register
};

struct Read_data{
  //  volatile unsigned int data[DATA_BUF_SIZE];  // 0x1000~0x1FFC  4MB FIFO memory space, (32Kx32)
  volatile unsigned int data[DATA_BUF_SIZE*2];  // 0x1000~0x1FFC +0x2000~0x2FFC(test) 8MB FIFO memory space, (32Kx32 + 32Kx32)
};



