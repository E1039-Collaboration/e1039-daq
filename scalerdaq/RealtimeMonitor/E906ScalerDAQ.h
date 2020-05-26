using namespace std;
// Written by Kaz - 3/6/2012

#ifndef __E906SCALERDAQ__
#define __E906SCALERDAQ__

#include "E906ET_globals.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include "TROOT.h"
#include "TFile.h"
#include "time.h"
#include "TTree.h"
#include "TH1F.h"
#include "TSQLServer.h"
#include "TSQLResult.h"
#include "TSQLRow.h"
#include <stdint.h>
#include "TDatime.h"
#include <TCanvas.h>
#include <TH1.h>
#include <TStyle.h>
#include <TApplication.h>
#include <TDirectory.h>
#include <TPaveText.h>
#include <TVirtualFFT.h>
#include <TStopwatch.h>
#include "Rtypes.h"
#include "TMath.h"
#include "TROOT.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//#define pulse_frequency 7300 //hz
class E906ScalerDAQ{
 private:

  //ROOT canvas and art nonsense
  TCanvas *c1;
  TPad *gpad;
  TPaveText *pavement,*pavement60hz;
  TDirectory *dir;
  //histograms
  TH1F *histo_raw_spectrum;
  TH1F *histo_for_fft;
  TH1F *histozoom_lvl1;
  TH1F *histozoom_lvl2;
  TH1F *histo60hz[10];
  TH1F *fft;
  TH1F *fft_log; //histograms -- initialize inside constructor
  Int_t histo60hz_array[10][200];
  
  ULong_t *PhysicsEvent;//scaler events.  Data gets filled into here
  ULong_t Last_PhysicsEvent[100];
  //running sums
  Float_t dutyfactor_includeMI;//includes main injector bunch fill ratio
  Float_t dutyfactor_noMI;
  Float_t sumhits,sumhits_squared,sumclock,normalization,sum_over_60hz,max60hz;
  Int_t sc_ch[3][32];
  Int_t scalerstatus[3][32];
  //

  //control variables
  Int_t reference_index;//this identifies where 0xe906f002 is in the datastream.
  Int_t bin60hz,num60hz_frames,histo60hz_counter;
  Int_t start_60hz,eventcounter,eventsThisRun;
  Int_t wait_counter,counter_60hz,new60hz_cycle_flag; //wait after end of spill, counter within the 60hz cycle, and flag denoting a new 60hz cycle(i.e Chucks 60hz pulse)
  Int_t killscript;
  Int_t numskipped,numspills;
  Int_t slowcontrol_counter;
  Bool_t updateflag,plask_flag,runnumber_flag;
  TStopwatch *timer;
  //

  //strings/texts for tpavetext and filename outputs
  char scalernames[3][32][30];
  char variablename[3][32][30];
  TString dutyfactorS,accS,dutyfactorS2;
  char timestamp[40];
  char timestamp_int[30];
  char timestamp_linux[30];
  char datestamp[40],dutyfactor_char[100],dutyfactor_char2[100],acc_char[40];
  TString targetstring[10];
  struct stat date_buf;
  //
  /* int sock; */
  /* struct sockaddr_in server; */
  FILE *fp, *fp2;//system command handle to get runnumber
  Float_t turn13,bunch13,nbsyd,nm3sem;// main injector parameters
  Int_t targbit,runnumber_int,runnumber_prev,evnum_int,transient_flag;
  Char_t runnumber[10],evnum[20];
  ofstream debugout;
  
  Float_t Y4DF;
  Float_t coda_Isum, coda_Isquared_sum,coda_Nevts;//These are running sums accumulated inside of CODA (see gen_int_list.crl for the scaler daq under /usr/local/coda/2.6.1/extensions/e906/)
  Float_t coda_dutyfactor;

  Bool_t eosflag;
  Int_t eos_slowcontrol_count;
  Int_t rootspills;//new rootfile every 60 spills
  Int_t spillcounter,spillcounter_previous,samecounter;
  Char_t text[200];
  Long_t RFintegral;

 public:
  E906ScalerDAQ();//constructor
  virtual ~E906ScalerDAQ();//destructor

  void E906DAQ_getrunnumber();
  void E906DAQ_accumulate_runningsum();
  void E906DAQ_fillhistograms();

  void E906DAQ_subtract_lastevent();//only used if the scalers are NOT reset after each 7.3kHz trigger

  void E906DAQ_EOS_report();
  Bool_t E906DAQ_endofspill_check();
  Bool_t E906DAQ_slowcontrol_check();//see if the EOS seen by the time slowcontrol has arrived 

  void E906DAQ_skip_check();
  void E906DAQ_update_counters();
  Bool_t E906DAQ_return_eosflag();



  void E906DAQ_fft();
  void E906DAQ_dutyfactor();
  void E906DAQ_Get_ACNET();
  void E906DAQ_updateplots(Bool_t EOS);
  void E906DAQ_reset_at_newspill();
  void E906DAQ_reset_at_newrun();
  void E906DAQ_make_legend();
  Int_t E906DAQ_killscript();
  void E906DAQ_save();
  Int_t E906DAQ_goodevent_check();
  Int_t E906DAQ_transient_flag();
  void E906DAQ_getdata(ULong_t *kazdata);
  Int_t E906DAQ_return_eventcounter();
  void E906DAQ_output_EPICS();
  void E906DAQ_output_root();
  void E906DAQ_output_ascii();
  void E906DAQ_retrieve_coda_sums();

};
#endif
