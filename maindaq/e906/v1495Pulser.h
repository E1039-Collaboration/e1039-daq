/**************************************************************************
*
*  v1495Lib.h  - Header for v1495 Library
*
*
*  Author: Shiuan-Hal , Shiu 
*          
*          August  2010
*
*/


/* Define Structure for access to Local Memory map*/
/* $$ means don't using now! */

struct v1495_csr{
  volatile unsigned short reg[8];   
 };

#define v1495_MAX_BOARDS 5


/* Define Bit Masks */
#define v1495_START_RUN               0x1000
#define v1495_MULTI_HITS              0x0100


/* Define some macros */
#define v1495_RunMode_Start(id)    {v1495_p[id]->reg[0] |=  v1495_START_RUN;}     
#define v1495_RunMode_Auto(id)     {v1495_p[id]->reg[0] &= ~v1495_START_RUN;}

#define v1495_PulseMode_Rotate(id) {v1495_p[id]->reg[0] &= ~v1495_MULTI_HITS;}
#define v1495_LEMO_Enable(id)      {v1495_p[1d]->reg[3] = 1;}
#define v1495_LEMO_Disable(id)     {v1495_p[1d]->reg[3] = 0;}
#define v1495_Enable_Cable1(id)    {v1495_p[id]->reg[4] =0xffff;}
#define v1495_Enable_Cable2(id)    {v1495_p[id]->reg[5] =0xffff;}
#define v1495_Enable_Cable3(id)    {v1495_p[id]->reg[6] =0xffff;}
#define v1495_Enable_Cable4(id)    {v1495_p[id]->reg[7] =0xffff;}
#define v1495_Disable_Cable1(id)    {v1495_p[id]->reg[4] =0x0;}
#define v1495_Disable_Cable2(id)    {v1495_p[id]->reg[5] =0x0;}
#define v1495_Disable_Cable3(id)    {v1495_p[id]->reg[6] =0x0;}
#define v1495_Disable_Cable4(id)    {v1495_p[id]->reg[7] =0x0;}


