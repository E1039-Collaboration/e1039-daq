/**************************************************************************
*
*  v1495Lib.h  - Header for v1495 Library
*
*
*  Author: Shiuan-Hal , Shiu 
*          
*          August  2010
*
* Shift v1495_data{} up to 0x1000 and above.  REM - 2013-03-14
* Added struct for pulser data memory blocks. REM - 2013-06-10
*
*/

#ifndef V1495TRASHLIB_H
#define V1495TRASHLIB_H

/* vxheaders for BSP */
#include "../vxheaders/vxWorks.h"
#include "../vxheaders/stdio.h"
#include "../vxheaders/string.h"
#include "../vxheaders/logLib.h"
#include "../vxheaders/loadLib.h"
#include "../vxheaders/taskLib.h"
#include "../vxheaders/intLib.h"
#include "../vxheaders/iv.h"
#include "../vxheaders/semLib.h"
#include "../vxheaders/vxLib.h"
#include "../vxheaders/unistd.h"
#include "../vxheaders/math.h"
#include "../vxheaders/time.h"		
#include "../vxheaders/stdlib.h"		
#include "../vxheaders/fcntl.h"

// magic numbers are evil and bad
#define USERLAND_OFFSET 0x1000
#define CAENLAND_OFFSET 0x8000
#define USER_MEMREAD_OFFSET 0x1100
#define USER_TDC_OFFSET 0x1200
#define USER_PULSEMEM_OFFSET 0x1400
#define DUMMY_REVISION 0x00000400
#define LEVEL_0_REVISION 0x00000440
#define LEVEL_1_REVISION 0x00000480

// TODO need better descriptive language
#define TIMING_ADDRESS_OFFSET 0x2000
#define PATTERN_2_LINES 128
#define PULSE_PATTERN_LINES 256
#define PULSE_PATTERN_COLUMNS 6
#define TDC_CIP_TAG 0x7fff
#define RUN_SIGNAL_START 0x8000
#define RUN_SIGNAL_END 0x0000

// seem ok for now
#define MAX_NUM_1495s 9
#define NUM_DUMMY_BOARDS 4
#define NUM_LVL_0_BOARDS 4
#define NUM_LVL_1_BOARDS 1
#define MAX_FILE_NAME_LENGTH 60
#define TDC_STOP_SIGNAL 0x0002
#define PULSE_ON_SIGNAL 0x8000
#define PULSE_RESET_SIGNAL 0x0000
#define PULSER_WAKEUP_SIGNAL 0x0001
#define PULSER_SLEEP_SIGNAL 0x0000
#define SECONDS_WAIT_TIME 0 
#define NANOS_WAIT_TIME 100000
#define PULSE_BLOCK_LENGTH 256
#define TDC_BUFFER_DEPTH 256
#define v1495_MAX_BOARDS 5
#define RELOAD_SIGNAL 0x1
#define MAX_INPUT_CH 96 

/* Declare global variables */
volatile unsigned globvar;
volatile struct v1495_data *v1495[v1495_MAX_BOARDS];	/* pointers to v1495 memory map */
volatile struct v1495_vmestruct *v1495s[v1495_MAX_BOARDS];	/* pointers to v1495 memory map */
volatile struct v1495_memreadout *v1495m[v1495_MAX_BOARDS];	/* pointers to v1495 memory map */
volatile struct v1495_tdcreadout *v1495t[v1495_MAX_BOARDS];	/* pointers to v1495 tdc map */
volatile struct v1495_pulse *v1495p[v1495_MAX_BOARDS];	/* pointers to v1495 pulse map */
volatile struct v1495_delay *v1495d[v1495_MAX_BOARDS];	/* pointers to v1495 timing delay map */

/* Define Structure for access to Local Memory map*/
/* $$ means don't using now! */
// TODO these are all incorrect names taken from the COIN LOGIC reference
// design, but they all serve different purposes and need to be renamed to
// reflect this fact
struct v1495_data {
  volatile unsigned short a_sta_l;  /* now is internal memory addr readout (0x1000)*/
  volatile unsigned short a_sta_h;         /* now ??? undefine(0x1002) */
  volatile unsigned short b_sta_l;        /* B port status (low)(0x1004) */
  volatile unsigned short b_sta_h;        /* B port status (high)(0x1006) */
  volatile unsigned short c_sta_l;        /* C port status (low)(0x1008) */
  volatile unsigned short c_sta_h;        /* C port status (high)(0x100A) */
  volatile unsigned short a_mask_l;       /* $$a mask register (low)(0x100C) */
  volatile unsigned short a_mask_h;       /* $$a mask register (high)(0x100E) */
  volatile unsigned short b_mask_l;       /* $$b mask register (low)(0x1010) */
  volatile unsigned short b_mask_h;       /* $$b mask register (high)(0x1012) */
  volatile unsigned short c_mask_l;       /* $$c mask register (low)(0x1014) */
  volatile unsigned short c_mask_h;       /* $$c mask register (high)(0x1016) */
  volatile unsigned short gatewidth;    /* $$gate width(0x1018) */
  volatile unsigned short c_ctrl_l;  /* now is internal memory addr ctrl (0x101A) */
  volatile unsigned short c_ctrl_h;     /* now is internal memory ctrl (0x101C) */
  volatile unsigned short mode;         /* $$mode(0x101E) */
  volatile unsigned short scratch;      /* scratch(0x1020) */
  volatile unsigned short g_ctrl;       /* $$g ctrl only bit 0 work(0x101E) */
  volatile unsigned short d_ctrl_l;       /* $$d ctrl (low)(0x1024) */
  volatile unsigned short d_ctrl_h;       /* $$d ctrl (high)(0x1026) */
  volatile unsigned short d_data_l;       /* $$d data (low)(0x1028) */
  volatile unsigned short d_data_h;       /* $$d data (high)(0x102A) */
  volatile unsigned short e_ctrl_l;       /* $$e ctrl (low)(0x102C) */
  volatile unsigned short e_ctrl_h;       /* $$e ctrl (high)(0x102E) */
  volatile unsigned short e_data_l;       /* $$e data (low)(0x1030) */
  volatile unsigned short e_data_h;       /* $$e data (high)(0x1032) */
  volatile unsigned short f_ctrl_l;       /* $$f ctrl (low)(0x1034) */
  volatile unsigned short f_ctrl_h;       /* $$f ctrl (high)(0x1036) */
  volatile unsigned short f_data_l;       /* $$f data(0x1038) */
  volatile unsigned short f_data_h;       /* $$f Set to 0x0001 to start pulser  data(0x103A) */
  volatile unsigned short revision;     /* revision(0x103C) */
  volatile unsigned short pdl_ctrl;     /* $$pdl control(0x103E) */
  volatile unsigned short pdl_data;     /* $$pdl data(0x1040) */
  volatile unsigned short d_id;     /* d id code(0x1042) */
  volatile unsigned short e_id;     /* e id code(0x1044) */
  volatile unsigned short f_id;     /* f id code(0x1046) */

};

struct v1495_vmestruct {
  volatile unsigned short ctrlr;     /* $$ control register (0x8000)*/
  volatile unsigned short statusr;   /* $$ status register (0x8002)*/
  volatile unsigned short int_lv;    /* $$ interrupt Level (0x8004)*/
  volatile unsigned short int_ID;    /* $$ interrupt Lv ID (0x8006)*/
  volatile unsigned short geo_add;   /* $$ geo address register (0x8008)*/
  volatile unsigned short mreset;    /* module reset (0x800A)*/
  volatile unsigned short firmware;  /* $$ firmware revision (0x800C)*/
  volatile unsigned short svmefpga;  /* $$ select vme fpga (0x800E)*/
  volatile unsigned short vmefpga;   /* $$ vme fpga flash (0x8010)*/
  volatile unsigned short suserfpga; /* $$ select user fpga (0x8012)*/
  volatile unsigned short userfpga;  /* $$ user fpga flash (0x8014)*/
  volatile unsigned short fpgaconf;  /* user fpga configuration (0x8016)*/
  volatile unsigned short scratch16; /* scratch16 (0x8018)*/
  volatile unsigned int scratch32;       /* dcratch32 (0x8020)*/
};

struct v1495_memreadout {
  volatile unsigned short mem[128];     /* $$ memorr readout?(0x1100~0x11FF)*/
};

struct v1495_tdcreadout {
  volatile unsigned short tdc[TDC_BUFFER_DEPTH];     /* $$ tdc readout?(0x1200~0x13FF)*/
};

struct v1495_pulse {
  volatile unsigned short pulse1[PULSE_BLOCK_LENGTH];  /*pulser data block #1 (0x1400-0x15ff) */
  volatile unsigned short pulse2[PULSE_BLOCK_LENGTH];  /*pulser data block #2 (0x1600-0x17ff) */
  volatile unsigned short pulse3[PULSE_BLOCK_LENGTH];  /*pulser data block #3 (0x1800-0x19ff) */
  volatile unsigned short pulse4[PULSE_BLOCK_LENGTH];  /*pulser data block #4 (0x1a00-0x1bff) */
  volatile unsigned short pulse5[PULSE_BLOCK_LENGTH];  /*pulser data block #5 (0x1c00-0x1dff) */
  volatile unsigned short pulse6[PULSE_BLOCK_LENGTH];  /*pulser data block #6 (0x1e00-0x1fff) */
};

struct v1495_delay {
  volatile unsigned short delay[MAX_INPUT_CH];  /* $$ timing delay (0x2000~0x2060)*/
};

STATUS v1495Init(UINT32 first_addr, UINT32 addr_inc, int num_boards);
STATUS v1495StatusUSER (int id);
STATUS v1495StatusCAEN (int id);
STATUS v1495StatusMEM (int id, int ch);
STATUS v1495StatusTDC (int id, int ch);
STATUS v1495StatusPulser (int id, int ipulser, int ch);
STATUS v1495StatusDelay (int id, int ch);
STATUS v1495TimewindowSet(int id, unsigned short val);
unsigned short v1495TimewindowRead(unsigned id);
STATUS v1495Timeset(int num_channels, unsigned id);
STATUS v1495TimesetQuick (int num_channels, unsigned id);
STATUS v1495ActivatePulser(unsigned id);
STATUS v1495DeactivatePulser(unsigned id);
STATUS v1495PatternSet(unsigned level, int id, const char* filename);
void dummydelay(unsigned delay);
STATUS v1495Run(unsigned id);
void v1495PulserGo(unsigned id);
void L2ContPulse(unsigned id);
void L2SinglePulse(unsigned id);
void L2KludgeSinglePulse(unsigned id);
void L0ContPulse(unsigned id);
void L2startStopTest(int id0, int id2, int win0, int win2);
void L0lonelyPulse(int id0, int win0);
void L2L0startTest(int id0, int id2, int win0, int win2);
unsigned short v1495TDCcount(unsigned id);
int g1(int id, int row);

STATUS v1495PulseWriteReadTest(unsigned id, unsigned num_writes, unsigned delay,
    unsigned verbose);

void v1495Reload(unsigned id);

void v1495TDCdump(unsigned id);
void v1495Pulsedump(unsigned id);
STATUS checkId(unsigned id);


void v1495RoadPulserAll(int id0, int id1, int id2, char charge, const int numFiles,
			const int iter, int win0, int win1, int win2);
void v1495RoadPulserInit();
void v1495RoadPulserTimingSet(int id0, int id1, int id2, int win0, int win1, int win2 );
void v1495RoadPulserWakeup(int id0 );
void v1495RoadPulserPatSet(int id0, int id2, char charge, char TorB, int iFile);
void v1495RoadPulserCompare(int id0, int id1, int id2, char charge,  char TorB, int iFile,const int iter);
void v1495RoadPulserCompareQuick(int id0, int id1, int id2, const int iter);


#endif //V1495TRASHLIB_H//
