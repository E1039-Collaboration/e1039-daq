#include "BeamDAQ/BeamDAQGlobals.h"

#include "BeamDAQ/DAQUtil.h"
#include "BeamDAQ/QIEBoardUtil.h"
#include "BeamDAQ/BeamDAQSpill.h"
#include "BeamDAQ/BeamAnalysis.h"

#include <TStopwatch.h>
#include <TApplication.h>
#include <TDatime.h>
#include "time.h"

#include <sys/wait.h>
#include <boost/filesystem.hpp>

using namespace std;


//set to true to read the board once NOW and quit (for testing purposes)
const bool oneShotMode = false;

//spend at least 20 seconds processing before looking for the next spill.
//Avoids problems when the beam is off and there is no data
const double minTimePerSpill = 20.; //in seconds

const string defaultRegFile = ""; //let QIEBoardUtil use its default
const bool verifyReg = true;      //want to know if settings were applied correctly

//int values for quality bits (powers of 2)
const unsigned int qual_dutyFactorProblem = 1;
const unsigned int qual_resetProblem      = 2;
//const unsigned int qual_anExample         = 4;

//if duty factor as percent is above this, there is probably a problem
const double maxDutyFactor = 95.;

//reset the QIE board every 30 minutes
//will not reset if a run is active or if binary file is being written
const double minResetTime = 30. * 60; //in seconds

int main( int argc, char *argv[] )
{
  cout << dec << "Starting BeamDAQ" << endl;

  //start timers and counters
  TStopwatch daqTimer;
  TStopwatch eosTimer;
  eosTimer.Start();
  TStopwatch resetTimer;
  resetTimer.Start();
  
  //start application so the TCanvases are shown
  TApplication theApp("BeamDAQ",&argc,argv);
  
  //initialize QIE board and set user registers
  bool connectOK = QIEBoardUtil::Get().ConnectAllSockets();
  if(!connectOK)
  {
    cout << "Could not make connections to QIE board.  Run 'ResetQIEBoard' and try again." << endl;
    cout << "... if that doesn't work, then make sure nobody else is connected to QIE manually (elog 9459) or call the BeamDAQ expert." << endl;
    return 1;
  }
  
  
  QIEBoardUtil::Get().InitializeRegistry( );
  
  
  QIEBoardUtil::Get().ResetRegistryFromFile( defaultRegFile, verifyReg );
  
  
  
  //test EPICS to identify common error
  {
    int e906eos = 0;
    cout << "Testing EPICS connection.  If the program doesn't print anything for 3 seconds, then quit the program (crtl-c), execute 'caget BOS', restart RunBeamDAQ" << endl;
    DAQUtil::Get().ReadEPICS( "EOS", e906eos );
    cout << "... EPICS is working!" << endl;
  }
  
  //loop to wait for EOS
  int nSpillsSeen = 0, nChecksEos = 0;
  int e906eosSave=0, e906eosNew=0;
  DAQUtil::Get().ReadEPICS( "EOS", e906eosSave );
  while(true)
  {
    int secSinceLastEOS = int( eosTimer.RealTime() );
    eosTimer.Continue();
    
    int e906eos = 0;
    // DAQUtil::Get().ReadEPICS( "E906EOS", e906eos );
    // cout << dec << "EOS flag = " << e906eos << " (" << secSinceLastEOS << "s since last EOS)" << endl;
    DAQUtil::Get().ReadEPICS( "EOS", e906eosNew );
    cout << dec << "EOS's = " << e906eosSave << "," << e906eosNew << ";" << " (" << secSinceLastEOS << "s since last EOS)" << endl;
    //if( 1 == e906eos || oneShotMode )
    //if( e906eosNew > e906eosSave || oneShotMode ) 
    if( e906eosNew ==1 || oneShotMode ) 
    {
      e906eosSave = e906eosNew ; // save the new EOS value
      
      nChecksEos = 0;
      ++nSpillsSeen;
      daqTimer.Start();
      eosTimer.Start();
      
      //create a new spill
      BeamDAQSpill *spill = new BeamDAQSpill();
      
      //get spill variables from ACNET and coda
      spill->SetRunNumber();
      spill->SetSpillNumber();
      
      ////////////////////////////////////
      spill->UpdateACNETVariables();
      ////////////////////////////////////
      
      //fetch data from QIE board and store in spill object
      bool retrieveOK = QIEBoardUtil::Get().Retrieve( spill );
      
      //output serialized spill object every 60 spills or if EPICS tells us to
      int qie2file = 0;
      bool wroteBinary = false;
      DAQUtil::Get().ReadEPICS( "QIE2FILE", qie2file );
      if( 0 != qie2file || 0 == nSpillsSeen%240 || oneShotMode )
      {
        string dirname = Form( "%s/binary/run_%06d", c_BeamDAQ_OUTDIR.c_str(), spill->GetRunNumber() );
        string filename = Form( "%s/BeamDAQ_%06d_%08d.bin", dirname.c_str(), spill->GetRunNumber(), spill->GetSpillNumber() );
        spill->OutputToBinary( filename );
        DAQUtil::Get().WriteEPICS( "QIE2FILE", "0" );
        wroteBinary = true;
      }
      
      //output to db text file for decoder
      {
        string filename = Form( "%s/cerenkov_data/db/db_run%d.dat", c_BeamDAQ_OUTDIR.c_str(), spill->GetRunNumber() );
        spill->OutputToSpillAscii( filename );
      }
      
      //output to ascii text file
      {
        string filename = Form( "%s/ascii/ascii_run%d.dat", c_BeamDAQ_OUTDIR.c_str(), spill->GetRunNumber() );
        //string filename = Form( "%s/ascii/spill_%09d_BeamDAQ.tsv", c_BeamDAQ_OUTDIR.c_str(), spill->GetSpillNumber() );
        spill->OutputToSpillAsciiDataSummary( filename );
      }
      
      //output to slowcontrol_data text file
      {
        //Daily data is stored in a separate folder
        time_t now = time(0);
        tm *now_tm = localtime(&now);

        char dummy[5];
        sprintf(dummy, "%04d", 1900+now_tm->tm_year);
        TString year = dummy;
        sprintf(dummy, "%02d", 1+now_tm->tm_mon);
        TString month = dummy;
        sprintf(dummy, "%02d", now_tm->tm_mday);
        TString day = dummy;
        sprintf(dummy, "%02d", now_tm->tm_hour);
        TString hour = dummy;
        sprintf(dummy, "%02d", now_tm->tm_min);
        TString minute = dummy;
        sprintf(dummy, "%02d", now_tm->tm_sec);
        TString sec = dummy;

        TString ascii2Dir = "/data2/e1039_data/slowcontrol_data";
        TString raidDir = ascii2Dir + "/" +"slowcontrol_" + year + "_" + month + "_" + day;
        //TString raidDir = "slowcontrol_" + year + "_" + month + "_" + day;

        //string filename = Form( "%s/ascii/spill_%09d_BeamDAQ.tsv", c_BeamDAQ_OUTDIR.c_str(), spill->GetSpillNumber() );
        string filename = Form( "%s/spill_%09d_BeamDAQ.tsv", raidDir.Data(), spill->GetSpillNumber() );
        spill->OutputToSpillSlowcontrolData( filename );
      }
      //send relevant variables into EPICS
      spill->OutputToEPICS();
      
      
      
      ////////////////////////////////////////////////////////
      //send relevant variables into ACNET
      //spill->OutputToACNET(); 
      //commented out 1/14/2020
      ////////////////////////////////////////////////////////
      
      
      
      
      //Make ROOT plots and TCanvas and save the images
      spill->OutputToVisualROOT();
      
      double mean_deadtime = -999.;
      
      //Run the analysis if there was beam
      if( spill->NoBeam() )
      {
        cout << "That was an empty spill.  Do not run BeamAnalysis." << endl;
      }
      else
      {
        
        bool doFFT = false;
        string filename = Form( "%s/cerenkov_data/rootfiles/Run_%d/histograms/histograms_run%d.spill%d.root", c_BeamDAQ_OUTDIR.c_str(), spill->GetRunNumber(), spill->GetRunNumber(), spill->GetSpillNumber() );
        AnalyzeSpill( &mean_deadtime,
                     spill, filename, doFFT );
        
        // test; moved to here
        //TDatime dt ;
        //const string dayOfWeekAbbr[] = { "NONE", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
        //const string datestamp = Form("%d-%02d-%02d-%s", dt.GetYear(), dt.GetMonth(), dt.GetDay(), dayOfWeekAbbr[dt.GetDayOfWeek()].c_str()  );
        //const string timestamp = Form("%04d%02d%02d%02d%02d%02d", dt.GetYear(), dt.GetMonth(), dt.GetDay(), dt.GetHour(), dt.GetMinute(), dt.GetSecond() );
        //string filename2 = Form( "/data2/data/beamDAQ/pics/QIE/%s/QIE_%s.png",  datestamp.c_str() , timestamp.c_str() );
        //sNew
        string filename2 = Form( "%s/cerenkov_data/pict/Run_%d/pict_run%d.spill%d.png", c_BeamDAQ_OUTDIR.c_str(), spill->GetRunNumber(), spill->GetRunNumber(), spill->GetSpillNumber() );
        
        //AnalyzeSpill2( spill->NoBeam(), spill, filename2 );
        //AnalyzeSpill2( spill->NoBeam(), spill, filename2 , filename );
        AnalyzeSpill2( spill->NoBeam(), spill, filename2 , filename );
        
      }
      //commented out 1/14/2020
      //spill->OutputToACNET2( mean_deadtime );
      
      //write our spill number to file
      //only do this if things worked.  if spillID gets behind then we get an alarm.
      
      
      if(retrieveOK)
        spill->OutputSpillNumber();
      
      //reset QIE board fully if we have time, haven't done it in a while and are not taking data at MainDAQ
      int secSinceLastReset = int( resetTimer.RealTime() );
      resetTimer.Continue();
      if( (!wroteBinary) && (minResetTime < secSinceLastReset) && DAQUtil::Get().IsDAQBetweenRuns() )
      {
        cout << Form( "Resetting QIE board (%.1fm since last reset).  Takes 10s...", secSinceLastReset/60. ) << endl;
        bool resetOK = QIEBoardUtil::Get().InitializeRegistry( true );
        cout << "Reset is " << (resetOK ? "OK" : "NOT OK") << endl;
        resetTimer.Start();
      }
      
      unsigned int beamDAQStatus = 0;
      
      //check that duty factor is not above 95% or something..  Update EPICS flag
      if( maxDutyFactor < spill->GetDutyFactor() )
      {
        cout << Form("Duty factor dangerously high (%.2f%%).  Warn the people!", spill->GetDutyFactor() ) << endl;
        beamDAQStatus += qual_dutyFactorProblem;
      }
      
      //check that reset is OK.  Update EPICS flag
      bool resetOK = QIEBoardUtil::Get().ResetRegistryFromFile( defaultRegFile, verifyReg);
      if(!resetOK)
      {
        cout << Form("WARNING - There was a problem resetting registry values.  Warn the people!") << endl;
        beamDAQStatus += qual_resetProblem;
      }
      
      //output the beam daq status to epics
      DAQUtil::Get().WriteEPICS( "BEAMDAQ_STATUS", Form("%u", beamDAQStatus ) );
      
      
      cout << dec << "Done processing spill " << ".  That was spill number " << nSpillsSeen << " seen by this program instance." << endl;
      cout << "-------Total Processing Time--------" << endl;
      daqTimer.Stop();
      const double runtime = daqTimer.RealTime();
      cout << "  Runtime = " << runtime << endl;
      daqTimer.Print();
      cout << "------------------------------------" << endl;

      delete spill;

      if(oneShotMode)
        break;

      //make sure we wait long enough for EOS signal to pass
      const double waitTime = minTimePerSpill - runtime;
      if( 0. < waitTime )
      {
        cout << Form("Ran too fast.  Sleeping for %.1fs", waitTime) << endl;
        sleep(waitTime);
      }
    }
    else
    {
      ++nChecksEos;
      sleep(1);
    }
  }

  theApp.Run();

  return 0;
}
