using namespace std;
// Written by Kaz - 3/6/2012
#include "E906ScalerDAQ.h"
#include "E906ET_netclient.h"

#include <TStopwatch.h>

int main(int argc, char *argv[])
{
  cout << "Starting " << argv[0] << endl;
  //ROOT style nonsense
  TStyle *mystyle  = new TStyle("mystyle","mystyle");
  mystyle->SetCanvasBorderMode(0);
  mystyle->SetPadBorderMode(0);
  mystyle->SetPadColor(0);
  mystyle->SetLabelOffset(0.02,"X");

  mystyle->SetLabelSize(0.05,"X");
  mystyle->SetLabelSize(0.05,"Y");
  mystyle->SetTitleOffset(0.8,"Y");
  mystyle->SetTitleSize(0.06,"Y");
  mystyle->SetTitleOffset(0.90,"X");
  mystyle->SetTitleSize(0.06,"X");
  mystyle->cd();
  gROOT->ForceStyle();

  cout << "Initializing TApplication" << endl;
  TApplication theApp("App",&argc,argv);
  //Done with ROOT styles

  //Three classes are declared. E906ScalerDAQ takes care of the plotting, 
  //fitting, FFT, and various other things relating to 60Hz timing and spill
  //timing.  E906ET_NETCLIENT (over-the-network) take care of the event-grabbing from
  //the ET.  
  E906ScalerDAQ *daq = new E906ScalerDAQ;//data processing, including dutyfactor calculation, FFT, histogram filling, etc.  This class can be found in E906ScalerDAQ.h/.C.
  E906ET_NETCLIENT *et_netclient = new E906ET_NETCLIENT; //data retrieval via over-the-network ETspy.  This class can be found in E906ET_netclient.h/.C.

  Int_t alive = 0;

  //THE FOLLOWING PART IS CRUCIAL.  Set the ET system handle (important) and the station name (not so important as far as I can tell).  This is hardcoded due to the fact that TApplication.theApp seems to modify the array elements found inside argc and argv, making them useless...the exception is argv[2], which is the ETspy station name
  cout << "Usage:  E906FFT <station_name (def=e906)>"<< endl;

  //use user's station if given, otherwise use e906
  const string stationName = ( argc > 1 ? argv[1] : "e906" );

  /*
     Names are hardcoded as:
     1. ET filehandle is always /tmp/et_sys_Sea2sc for CODA in E906 Scaler DAQ
     2. IPaddress of host running the beam DAQ is static (private network, e906sc1.fnal.gov)
     3. The station name can be anything other than et_sys_Sea2sc 
     */
  cout << "Initiating network ETSpy" << endl;
  cout << "Selected ETSpy Station name is: " <<  stationName << endl;
  et_netclient->E906ET_netclient_config( stationName.c_str() );
  alive = et_netclient->E906ET_netclient_alive();
  //Done setting ET system handle and station name

  cout << "ET alive?..." << endl;
  if(alive)
  {
    cout <<"   ...Definitely alive!" << endl;
  }
  else{
    cout << "***********************************************" << endl;
    cout << "ET not alive.  Check that the ET file handle (/tmp/et_sys_Sea2sc) exists.  If not, restart CODA.   Exiting... " << endl;
    cout << "***********************************************" << endl;
    exit(1);
  }

  Int_t nevents = 0, nIterations = 0;
  ofstream out2;
  out2.open("sumout.out");//only for debugging...not really used otherwise


  while(alive&&!daq->E906DAQ_killscript())
  {//check to see ET is alive, and loop over events

    //how many events are there?
    nevents = et_netclient->E906ET_netclient_return_nevents(daq->E906DAQ_return_eventcounter());

    //loop over events and process
    for(Int_t i=0; i!=nevents; ++i)
    {	
      //load event
      daq->E906DAQ_getdata(et_netclient->E906ET_netclient_returnarray(i));

      if( daq->E906DAQ_goodevent_check() )
      {//event flags are there (0xE906f002 and 0xE906f003)

        if( !daq->E906DAQ_transient_flag() && !daq->E906DAQ_return_eosflag() )
        {//physics event channel doesn't show a transient spike with more than 100k hits
          daq->E906DAQ_accumulate_runningsum();
          daq->E906DAQ_fillhistograms();
        }

        //1 = EOS 0 = not.  Update regardless of whether the EOS is hit or not.  Once every 2000 evts if not?
        daq->E906DAQ_updateplots(daq->E906DAQ_return_eosflag());

        //if end of spill, do fft, calculated duty factor, make legend, update plots, save file, and reset all variables for new spill
        if(daq->E906DAQ_endofspill_check()&&!daq->E906DAQ_slowcontrol_check())
        {
          cout << "EOS bit found  " << daq->E906DAQ_return_eosflag() <<  endl;
          TStopwatch timer;
          timer.Start();

          daq->E906DAQ_retrieve_coda_sums();

          daq->E906DAQ_Get_ACNET();
          daq->E906DAQ_dutyfactor();

          timer.Stop();
          cout << "time between EOS and output to EPICS (s): " << timer.RealTime() << endl;
          timer.Print();
          timer.Continue();
          //timer.Start();
          daq->E906DAQ_output_EPICS();
          daq->E906DAQ_getrunnumber();
          //timer.Stop();
          //cout << "time between EOS and getting spillcount (s): " << timer.RealTime() << endl;
          //timer.Print();
          daq->E906DAQ_fft();

          daq->E906DAQ_make_legend();
          
          timer.Stop();
          cout << "time between EOS and output to root file (s): " << timer.RealTime() << endl;
          timer.Print();

          daq->E906DAQ_output_root();
          daq->E906DAQ_output_ascii();
          daq->E906DAQ_skip_check();

          daq->E906DAQ_updateplots(daq->E906DAQ_return_eosflag());
          daq->E906DAQ_save();//Save png file for webserver output

          daq->E906DAQ_EOS_report();
          ++nIterations;
          cout << "Iteration = " << nIterations << endl;
          daq->E906DAQ_reset_at_newspill(); // reset histograms and running sums
        }//end if EOS
        daq->E906DAQ_update_counters();
      } //end if good event
    }//end loopover nevents

    et_netclient->E906ET_netclient_postprocess();
    alive = et_netclient->E906ET_netclient_alive();
  }//end forever while alive loop

  out2.close();

  theApp.Run();
  cout << "Program finished" << endl;
  return 0;

}
