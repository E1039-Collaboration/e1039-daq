#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/DAQUtil.h"
#include <TROOT.h>
#include <TCanvas.h>

#include <boost/algorithm/string.hpp> //for strip
#include <wordexp.h>     //to expand environmental variables in string

using namespace std;


DAQUtil& DAQUtil::Get()
{
  static DAQUtil singleton;
  return singleton;
}


int DAQUtil::GetRunNumber() const
{
  static int lastRunNumber = 0;
  int runNumber = lastRunNumber;

  char tmpchar[30];
  FILE *fp2 = popen("/usr/bin/rsh -n -l e1039daq e1039daq1.sq.pri 'plask -rt Spin -spRunNumber'","r");
  while(fgets(tmpchar,sizeof(tmpchar)-1,fp2)!=NULL){
     cout << "RUNNUMBER!!!!" << tmpchar << endl;
    //todo: parse in a way that lets me get unsigned int
    runNumber = atoi(tmpchar);
  }
  pclose(fp2);

  //if the retrieved runnumber is garbage (i.e 0), then get the last runnumber from the runnumber.dat file
  if(runNumber < lastRunNumber)
  {
    cout << "WARNING: run number from plask is garbage (" << runNumber << ", last spill's run was " << lastRunNumber << ").  Getting run number from file." << endl;
    ifstream runnumber_in;
    runnumber_in.open("runnumber.dat");
    runnumber_in >> runNumber;
    runnumber_in.close();
    cout << "      Run number from file is " << runNumber << ".  Should be same as last spill's run number (" << lastRunNumber << ")" << endl;
  }

  //write this runnnumer to file and remember it for next time
  if( runNumber != lastRunNumber )
  {
    ofstream runnumber_out;
    runnumber_out.open("runnumber.dat");
    runnumber_out << runNumber;
    runnumber_out.close();
    lastRunNumber = runNumber;
  }

  return runNumber;
}

int DAQUtil::GetSpillNumber() const
{
  int spillNumber;
  ifstream inspillcount;
  inspillcount.open( c_SpillCounterFile.c_str() );
  inspillcount >> spillNumber;
  inspillcount.close();
  return spillNumber;
}

bool DAQUtil::IsDAQBetweenRuns() const
{
  TString state;
  char tmpchar[64];
  FILE *fp2 = popen("/usr/bin/rsh -n -l e1039daq e1039daq1 'plask -rt Spin -spState'","r");
  while(fgets(tmpchar,sizeof(tmpchar)-1,fp2)!=NULL){
    //remove leading and trailing whitespace - better with boost
    string tmpString = tmpchar;
    boost::algorithm::trim(tmpString) ;
    state = tmpString;
  }
  pclose(fp2);

  //anything but active, prestarted, *ing
  bool betweenRuns = true;
  if( state == "active" )
    betweenRuns = false;
  else if( state == "prestarted" )
    betweenRuns = false;
  else if( state.EndsWith("ing") )
    betweenRuns = false;

  return betweenRuns;
}


bool DAQUtil::ReadEPICS( const std::string& name, int &rval ) const
{
  rval = -1;
  const size_t buffsize = 10;
  char buffer[buffsize];

  FILE *pipe = popen( Form("caget -t %s", name.c_str() ),"r");
  if (!pipe) 
  {
    cout << "DAQUtil::ReadEPICS - ERROR - Could not execute caget." << endl;
    return false;
  }
  while( fgets(buffer, buffsize, pipe) != NULL )
    rval = atoi(buffer);

  pclose(pipe);
  return true;
}

bool DAQUtil::WriteEPICS( const std::string& name, const std::string& val ) const
{
  int callRval = system( Form( "caput -t %s %s", name.c_str(), val.c_str() ) );

  if( 0 != callRval )
  {
    cout << "ERROR - DAQUtil::WriteEPICS - Return value " << callRval << " when writing value " << val << " to variable " << name << endl;
    return false;
  }

  return true;
}

bool DAQUtil::ReadACNET( const std::string& name, int& rval ) const
{
  rval = -1;
  const size_t buffsize = 32;
  char buffer[buffsize];

  FILE *pipe = popen( Form("acnet -t %s", name.c_str() ),"r");
  if (!pipe)
  {
    cout << "DAQUtil::ReadACNET - ERROR - Could not execute acnet." << endl;
    return false;
  }
  while( fgets(buffer, buffsize, pipe) != NULL )
    rval = atoi(buffer);

  pclose(pipe);
  return true;
}

bool DAQUtil::ReadACNET( const std::string& name, double& rval ) const
{
  rval = -1.;
  const size_t buffsize = 32;
  char buffer[buffsize];

  FILE *pipe = popen( Form("acnet -t %s", name.c_str() ),"r");
  if (!pipe)
  {
    cout << "DAQUtil::ReadACNET - ERROR - Could not execute acnet." << endl;
    return false;
  }
  while( fgets(buffer, buffsize, pipe) != NULL )
    rval = atof(buffer);

  pclose(pipe);
  return true;
}

bool DAQUtil::WriteACNET( const std::string& name, const std::string& val ) const
{
  int callRval = system( Form( "acnetput %s %s", name.c_str(), val.c_str() ) );

  if( 0 != callRval )
  {
    cout << "ERROR - DAQUtil::WriteACNET - Return value " << callRval << " when writing value " << val << " to variable " << name << endl;
    return false;
  }

  return true;
}



int DAQUtil::PrepareDir( const std::string& dir ) const
{
  int madedir = 0;
  if( 0 != system( Form( "test -d %s", dir.c_str() ) ) ){
    cout << "INFO - DAQUtil::PrepareDir - directory " << dir << " does not exist.  create it..." << endl;
    madedir = system( Form( "mkdir -m 755 -p %s", dir.c_str() ) );
    if( 0 != madedir )
    {
      cout << "ERROR -DAQUtil::PrepareDir - could not make top level directory: " << dir << endl;
    }
  }
  return madedir;
}

std::string DAQUtil::ExpandEnvironmentals( const std::string& s ) const
{
  // expand any environment variables in the file name
  wordexp_t exp_result;
  if (wordexp(s.c_str(), &exp_result, 0) != 0)
  {
    cout << "DAQUtil::ExpandEnvironmentals - ERROR - Could not expand environmental variables.  Returning the original string: " << s << endl;
    return s;
  }

  //convert to string and return
  return string( exp_result.we_wordv[0] );
}


TCanvas* DAQUtil::GetOrCreateCanvas( const std::string& name ) const
{
  cout << "DAQUtil::GetOrCreateCanvas( " << name << ")" << endl;
  //if canvas already exists, get it
  TCanvas *can = dynamic_cast<TCanvas*>( gROOT->FindObject( name.c_str() ) );
  if( NULL == can )
  {
    //if the canvas didn't exist, then create it specifically by name
    if( c_beamTextCanvasName == name )
    {
      can = new TCanvas(c_beamTextCanvasName.c_str(), "BeamDAQ Text", 5, 5, 600, 800);
    }
    else if( c_beamFramesCanvasName == name )
    {
      can = new TCanvas(c_beamFramesCanvasName.c_str(), "BeamDAQ Frames", 5, 300, 1200, 1000);
    }
    
    
    else if( c_beamFrames2CanvasName == name )
    {// new frame 
      //can = new TCanvas(c_beamFrames2CanvasName.c_str(), "BeamDAQ Frames2", 5, 300, 1200, 1000);
      can = new TCanvas(c_beamFrames2CanvasName.c_str(), "BeamDAQ Frames2", 5, 300, 1100, 850);
    
    }
    
    else if( c_beamAnalysisCanvasName == name )
    {
      can = new TCanvas(c_beamAnalysisCanvasName.c_str(), "BeamDAQ Analysis", 400, 5, 400, 600);
    }
    else
      cout << "DAQUtil::GetOrCreateCanvas - ERROR - Can't find canvas and don't know how to create one with name: " << name << ". Here's a NULL pointer." << endl;
  }
  return can;
}
