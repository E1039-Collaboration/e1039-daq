using namespace std;
// Written by Kaz - 3/6/2012

#ifndef __E906ET_NETCLIENT__
#define __E906ET_NETCLIENT__

#include "E906ET_globals.h"
#include <iostream>
#include <fstream>

//ROOT libraries
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
#include <TPaveText.h>
#include <TVirtualFFT.h>
#include "Rtypes.h"
#include "TMath.h"
#include "TROOT.h"

//ET_netclient required libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include "et.h"

/* recent versions of linux put float.h (and DBL_MAX) in a strange place */
#define DOUBLE_MAX   1.7976931348623157E+308

/* macros for swapping 2bytes and 4bytes */
#define SSWAP(x)  ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))
#define LSWAP(x)  ((((x) & 0x000000ff) << 24) | \
                   (((x) & 0x0000ff00) <<  8) | \
                   (((x) & 0x00ff0000) >>  8) | \
                   (((x) & 0xff000000) >> 24))

class E906ET_NETCLIENT{
 private:

  et_sys_id	 id;
  et_statconfig	 sconfig;
  et_stat_id	 my_stat;
  et_att_id	 my_att;
  et_openconfig  openconfig;
  int		 i, j,k, err, status, nread, swap;
#if 0
  int            intdata, *pintdata;
#endif
  int		 writeIt, printIt, control[];
  et_event       *evs[2000];
  struct timespec twait;
  char		 *pdata;
  char station[512];
  
  int             nevents, neventstotal, iterations;
  double          freq, freq_tot, freq_avg;
  struct timeval  t1, t2;
  double          time;
  
  ofstream out;
  unsigned long int *pInt;
  //  ULong_t *kazdata;//make sure this is a Ulong int to prevent overflow (it's 8 digits in hex max)
  Int_t event_counter;

 public:
  E906ET_NETCLIENT();//constructor
  ~E906ET_NETCLIENT();//destructor

  void E906ET_netclient_config(const char *host);
  void E906ET_netclient_close();
  // void E906ET_reset_array();

    //Single retrieval
  //  void E906ET_netclient_status();
  int E906ET_netclient_alive();
  int E906ET_netclient_error_reporting();
  int E906ET_netclient_return_nevents(Int_t eventcounter);

  ULong_t *E906ET_netclient_returnarray(Int_t eventnum);
  void E906ET_netclient_postprocess();
  //

};
#endif
