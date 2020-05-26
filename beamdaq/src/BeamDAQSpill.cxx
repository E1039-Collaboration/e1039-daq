#include "BeamDAQ/BeamDAQGlobals.h"

#include "BeamDAQ/BeamDAQSpill.h"

#include "BeamDAQ/DAQUtil.h"
#include "BeamDAQ/QIEDataTypes.h"

#include <TROOT.h>
#include <TStopwatch.h>
#include <TDatime.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TGraph.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TDirectory.h>
#include <TFile.h>
#include <iomanip>

#include <boost/filesystem.hpp>

using namespace std;

//local constants
namespace
{
  //put this between columns in mysql ascii files
  const string c_dbColSeperator = " ";
  //put this at the end of a row in mysql ascii files
  const string c_dbRowEnd = "-9999 ";

  //set verbsoe to true for more printing
  const bool verbose = false;

  //crappy hack because root doesn't do DrawClone correctly
  TPaveText *textPave = NULL;
  TPaveText *framePave = NULL;
  TLine     *thresholdLine = NULL;

  //somestimes ACNET fails and causes the program to stall
  const bool c_skipACNET = false;
}

/////////////////////////////////////////////////////////////////////
extern unsigned int ext_nTriggers; // <-- read from QIEBoardUtil.cxx
/////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const BeamDAQSpill& spill )
{
  os << spill.ToString();
  return os;
}

BeamDAQSpill::BeamDAQSpill() :
  m_qieHeader( new QIEHeader ),
  m_qieSettings( new QIESettings ),
  m_inhibitDataVec( new InhibitDataVec ),
  m_triggerDataVec( new TriggerDataVec ),
  m_qieTurnVec( new QIETurnVec ),
  m_goodRF(0),
  m_badRF(0),
  m_inhibitSum( -1. ),
  m_inhibitBucket( -1. ),
  m_busySum( -1. ),
  m_busyBucket( -1. ),
  m_intensitySum( -1. ),
  m_intensitySqSum( -1. )
{
  SetROOTStyle();


  TDatime td;
  m_year   = td.GetYear();
  m_month  = td.GetMonth();
  m_day    = td.GetDay();
  m_hour   = td.GetHour();
  m_minute = td.GetMinute();
  m_second = td.GetSecond();

  cout << "BeamDAQSpill::BeamDAQSpill" << endl;
}

BeamDAQSpill::~BeamDAQSpill() 
{
  //if we shange these type defs to be smart refs instead of bare pointers, then we do not need to do cleanup
  bool usingSmartRef = false;

  if( !usingSmartRef )
  {
    if( 0 != m_qieHeader )
      delete m_qieHeader;

    if( 0 != m_qieSettings )
      delete m_qieSettings;

    if( 0 != m_inhibitDataVec )
    {
      for( InhibitDataVec::iterator i = m_inhibitDataVec->begin(); i != m_inhibitDataVec->end(); ++i )
        delete *i;
      m_inhibitDataVec->clear();
      delete m_inhibitDataVec;
    }

    if( 0 != m_triggerDataVec )
    {
      for( TriggerDataVec::iterator i = m_triggerDataVec->begin(); i != m_triggerDataVec->end(); ++i )
        delete *i;
      m_triggerDataVec->clear();
      delete m_triggerDataVec;
    }

    if( 0 != m_qieTurnVec )
    {
      for( QIETurnVec::iterator i = m_qieTurnVec->begin(); i != m_qieTurnVec->end(); ++i )
        delete *i;
      m_qieTurnVec->clear();
      delete m_qieTurnVec;
    }
  }//end if using smart refs
}

std::string BeamDAQSpill::ToString() const
{
  TString rval = Form("BeamDAQSpill: run %d, spill %d\n", m_runNumber, m_spillNumber );
  rval += Form( 
      "\tACNET = { turn13 : %.1f, bunch13 : %d, nbsyd : %.1f, nm3sem : %.2E, nm2ion : %.2E, G2SEM : %.2E, E906BM : %.2E}\n", 
      m_turn13, m_bunch13, m_nbsyd, m_nm3sem, m_nm2ion, m_g2sem, m_e906bm 
      );
  rval += Form("Charge Norm = %.2f\n", GetChargeNormalization() );

  rval += m_qieHeader->ToString() + "\n";
  rval += m_qieSettings->ToString() + "\n";
  rval += Form( "There are %zu InhibitData.\n", m_inhibitDataVec->size() );
  rval += Form( "There are %zu TriggerData.\n", m_triggerDataVec->size() );
  rval += Form( "There are %zu QIETurns.\n", m_qieTurnVec->size() );

  return string( rval );
}

unsigned int BeamDAQSpill::GetRunNumber() const
{
  return m_runNumber;
}

unsigned int BeamDAQSpill::GetSpillNumber() const
{
  return m_spillNumber;
} 

TDatime BeamDAQSpill::GetTDatime() const
{
  return TDatime( m_year, m_month, m_day, m_hour, m_minute, m_second );
}



bool BeamDAQSpill::OutputToBinary( const std::string& filename ) const
{
  if(verbose)
    cout << "BeamDAQSpill::OutputToBinary - filename = " << filename << endl;

  //create directory if needed
  boost::filesystem::path filepath( filename );
  boost::filesystem::path dirpath = filepath.parent_path();
  DAQUtil::Get().PrepareDir( dirpath.string() );

  //open the file and write to it
  std::ofstream ofs( filename.c_str(), std::ios_base::binary );
  boost::archive::binary_oarchive oa(ofs);
  oa << *this;

  //note that files are closed when they go out of scope at the end of this function
  cout << "BeamDAQSpill::OutputToBinary - Created binary output file: " << filename << endl;

  return true;
}

bool BeamDAQSpill::OutputToSpillAscii( const std::string& filename ) const
{
  if(verbose)
    cout << "BeadDAQSpill::OutputToSpillAscii - filename = " << filename << endl;

  //if file doesn't exist, then create the output directory and add a header line
  std::ofstream ofs;
  bool addHeader = false;
  if( ! boost::filesystem::exists( filename ) )
  {
    cout << "BeadDAQSpill::OutputToSpillAscii - Creating new output ascii file: " << filename << endl;
    boost::filesystem::path filepath( filename );
    boost::filesystem::path dirpath = filepath.parent_path();
    DAQUtil::Get().PrepareDir( dirpath.string() );
    addHeader = true;
  }

  //do this before getting file handle in case it needs to actually do calculations
  const double dutyFactorNoMI = GetDutyFactor();
  const string timestamp = Form("%04d%02d%02d%02d%02d%02d", m_year, m_month, m_day, m_hour, m_minute, m_second );

  //open/create the file in append mode
  ofs.open( filename.c_str(), ofstream::out | ofstream::app);

  //add header line if necessary
  if(addHeader)
  {
    cout << "BeadDAQSpill::OutputToSpillAscii - First time writing to file.  Add header line." << endl;
    ofs
      << "spillcounter" << c_dbColSeperator
      << "timestamp" << c_dbColSeperator
      << "NM3SEM" << c_dbColSeperator
      << "QIE_sum" << c_dbColSeperator
      << "dutyfactor_53MHz" << c_dbColSeperator
      << "inhibit_count" << c_dbColSeperator
      << "inhibit_block_sum" << c_dbColSeperator
      << "trigger_count" << c_dbColSeperator
      << "trigger_sum_no_inhibit" << c_dbColSeperator
      << "Inh_output_delay" << c_dbColSeperator
      << "QIE_inh_delay" << c_dbColSeperator
      << "min_inh_width" << c_dbColSeperator
      << "inh_thres" << c_dbColSeperator
      << "QIE_busy_delay" << c_dbColSeperator
      << "marker_delay" << c_dbColSeperator
      << "QIE_phase_adjust" << c_dbColSeperator
      << c_dbRowEnd << endl;
  }

  ofs
    << m_spillNumber << c_dbColSeperator
    << timestamp << c_dbColSeperator
    << m_nm3sem << c_dbColSeperator
    << m_qieHeader->intensitySum << c_dbColSeperator
    << dutyFactorNoMI << c_dbColSeperator
    << m_inhibitDataVec->size() << c_dbColSeperator
    << m_qieHeader->inhibitSum << c_dbColSeperator
    << m_triggerDataVec->size() << c_dbColSeperator
    << m_qieHeader->busySum << c_dbColSeperator
    << m_qieSettings->thresholdDelay << c_dbColSeperator
    << m_qieSettings->dataSumDelay << c_dbColSeperator
    << m_qieSettings->minInhibitWidth << c_dbColSeperator
    << m_qieSettings->threshold << c_dbColSeperator
    << m_qieSettings->qieDelay << c_dbColSeperator
    << m_qieSettings->markerDelay << c_dbColSeperator
    << m_qieSettings->qieClockPhase << c_dbColSeperator
    << c_dbRowEnd << endl;

  ofs.close();

  cout << "BeadDAQSpill::OutputToSpillAscii - Added to ascii output file: " << filename << endl;

  return true;
}

bool BeamDAQSpill::OutputToSpillSlowcontrolData( const std::string& filename ) const
{
  if(verbose)
        cout << "BeadDAQSpill::OutputToSpillSlowcontrolData - filename = " << filename << endl;

  //if file doesn't exist, then create the output directory and add a header line
  std::ofstream ofs;
  bool addHeader = false;
  if( ! boost::filesystem::exists( filename ) )
  {
    cout << "BeadDAQSpill::OutputToSpillSlowcontrolData - Creating new output ascii file: " << filename << endl;
    boost::filesystem::path filepath( filename );
    boost::filesystem::path dirpath = filepath.parent_path();
    DAQUtil::Get().PrepareDir( dirpath.string() );
    addHeader = true;
  }

  //do this before getting file handle in case it needs to actually do calculations
  const string timestamp = Form("%04d%02d%02d%02d%02d%02d", m_year, m_month, m_day, m_hour, m_minute, m_second );
   
  char timestamp_linux[30];

  const double qieSum     = m_qieHeader->intensitySum;
  const double inhibitSum = m_qieHeader->inhibitSum;
  const double busySum    = m_qieHeader->busySum;
  const double dutyFactorNoMI  = GetDutyFactor( );
  const double busyBucket = GetBusyBucket( );
  const double inhibitBucket = GetInhibitBucket( );
  //const long int goodRF = GetGoodRF();
  //const long int badRF  = GetBadRF();
  const int note = 0;

  //only calculate these if there was beam and the denomintor is not 0
  const double fracInhibit = (1E-6 < qieSum && !NoBeam() ) ? inhibitSum / qieSum : 0.;
  const double fracBusy    = (1E-6 < qieSum && !NoBeam() ) ? busySum / qieSum : 0.;
  const double fracLive    = NoBeam() ? 0 : (1. - fracInhibit - fracBusy);
  const double liveProtons = m_g2sem * fracLive;
  const double liveBeam    = fracLive * m_g2sem;
  const double liveNM3     = fracLive * m_nm3sem;

  
  //open/create the file in append mode
  ofs.open( filename.c_str(), ofstream::out | ofstream::app);

  FILE *fp2 = popen("date \'+%s\'","r");//get timestamp
  while(fgets(timestamp_linux,sizeof(timestamp_linux)-1,fp2)!=NULL){
  }
  timestamp_linux[10] = '\0';
  pclose(fp2);

  ofs << "spillcounter" << "\t" << timestamp_linux << "\t" << m_spillNumber << "\t" << note <<endl;
  ofs << "NM3ION" << "\t" << timestamp_linux << "\t" << m_nm3sem << "\t" << note <<endl;
  ofs << "QIEsum" << "\t" << timestamp_linux << "\t" << m_qieHeader->intensitySum << "\t" << note <<endl;
  ofs << "dutyfactor53MHz" << "\t" << timestamp_linux << "\t" << dutyFactorNoMI << "\t" << note <<endl;
  ofs << "inhibit_count" << "\t" << timestamp_linux << "\t" << m_inhibitDataVec->size() << "\t" << note <<endl;
  ofs << "inhibit_block_sum" << "\t" << timestamp_linux << "\t" << m_qieHeader->inhibitSum << "\t" << note <<endl;
  ofs << "trigger_count" << "\t" << timestamp_linux << "\t" << m_triggerDataVec->size() << "\t" << note <<endl;
  ofs << "trigger_sum_no_inhibit" << "\t" << timestamp_linux << "\t" << m_qieHeader->busySum << "\t" << note <<endl;
  ofs << "Inh_output_delay" << "\t" << timestamp_linux << "\t" << m_qieSettings->thresholdDelay << "\t" << note <<endl;
  ofs << "QIE_inh_delay" << "\t" << timestamp_linux << "\t" << m_qieSettings->dataSumDelay << "\t" << note <<endl;
  ofs << "Min_Inh_Width" << "\t" << timestamp_linux << "\t" << m_qieSettings->minInhibitWidth << "\t" << note <<endl;
  ofs << "Inh_thres" << "\t" << timestamp_linux << "\t" << m_qieSettings->threshold << "\t" << note <<endl;
  ofs << "QIE_busy_delay" << "\t" << timestamp_linux << "\t" << m_qieSettings->qieDelay << "\t" << note <<endl;
  ofs << "Marker_delay" << "\t" << timestamp_linux << "\t" << m_qieSettings->markerDelay << "\t" << note <<endl;
  ofs << "QIE_phase_adjust" << "\t" << timestamp_linux << "\t" << m_qieSettings->qieClockPhase << "\t" << note <<endl;
  ofs << "inhibit_Bucket" << "\t" << timestamp_linux << "\t" << inhibitBucket << "\t" << note <<endl;//inhibitBucket
  ofs << "Busy_Bucket" << "\t" << timestamp_linux << "\t" << busyBucket << "\t" << note <<endl;//m_busyBucket
  ofs << "BEAM_INHIBITED" << "\t" << timestamp_linux << "\t" << 100.*fracInhibit << "\t" << note <<endl;
  ofs << "BEAM_BUSIED" << "\t" << timestamp_linux << "\t" << 100.*fracBusy << "\t" << note <<endl;
  ofs << "LIVE_BEAM" << "\t" << timestamp_linux << "\t" << liveBeam << "\t" << note <<endl;
  ofs << "LIVE_NM3SEM" << "\t" << timestamp_linux << "\t" << liveNM3 << "\t" << note <<endl;
  ofs << "DUTYFACTOR_BEAM" << "\t" << timestamp_linux << "\t" << dutyFactorNoMI << "\t" << note <<endl;

  ofs.close();

  cout << "BeadDAQSpill::OutputToSpillSlowcontrolData - Added to ascii output file: " << filename << endl;

  return true;
}


bool BeamDAQSpill::OutputToSpillAsciiDataSummary( const std::string& filename ) const
{
  if(verbose)
    cout << "BeadDAQSpill::OutputToSpillAsciiDataSummary - filename = " << filename << endl;

  //if file doesn't exist, then create the output directory and add a header line
  std::ofstream ofs;
  bool addHeader = false;
  if( ! boost::filesystem::exists( filename ) )
  {
    cout << "BeadDAQSpill::OutputToSpillAsciiDataSummary - Creating new output ascii file: " << filename << endl;
    boost::filesystem::path filepath( filename );
    boost::filesystem::path dirpath = filepath.parent_path();
    DAQUtil::Get().PrepareDir( dirpath.string() );
    addHeader = true;
  }

  //do this before getting file handle in case it needs to actually do calculations
  const string timestamp = Form("%04d%02d%02d%02d%02d%02d", m_year, m_month, m_day, m_hour, m_minute, m_second );

  const double qieSum     = m_qieHeader->intensitySum;
  const double inhibitSum = m_qieHeader->inhibitSum;
  const double busySum    = m_qieHeader->busySum;
  const double dutyFactorNoMI  = GetDutyFactor( );
  const long int goodRF = GetGoodRF();
  const long int badRF  = GetBadRF();

  //only calculate these if there was beam and the denomintor is not 0
  const double fracInhibit = (1E-6 < qieSum && !NoBeam() ) ? inhibitSum / qieSum : 0.;
  const double fracBusy    = (1E-6 < qieSum && !NoBeam() ) ? busySum / qieSum : 0.;
  const double fracLive    = NoBeam() ? 0 : (1. - fracInhibit - fracBusy);
  const double liveProtons = m_g2sem * fracLive;

  //open/create the file in append mode
  ofs.open( filename.c_str(), ofstream::out | ofstream::app);

  //add header line if necessary
  if(addHeader)
  {
    cout << "BeadDAQSpill::OutputToSpillAsciiDataSummary - First time writing to file.  Add header line." << endl;
    ofs
      << "timestamp" << c_dbColSeperator
      << "spillcounter" << c_dbColSeperator
      << "NM2ION/QIEsum" << c_dbColSeperator
      << "NM3SEM/QIEsum" << c_dbColSeperator
      << "NIM4Beam/QIEsum" << c_dbColSeperator
      << "G2SEM/QIEsum" << c_dbColSeperator
      << "NM2ION(ppp)" << c_dbColSeperator
      << "NM3SEM(ppp)" << c_dbColSeperator
      << "NIM4Beam" << c_dbColSeperator
      << "G2SEM(ppp)" << c_dbColSeperator
      << "QIEsum" << c_dbColSeperator
      << "dutyfactor(%)" << c_dbColSeperator
      << "numinhibit" << c_dbColSeperator
      << "inhibit_block_sum(linear)" << c_dbColSeperator
      << "inhibited_protons(%)" << c_dbColSeperator
      << "numtrigger" << c_dbColSeperator
      << "trigger_sum_no_inhibit(linear)" << c_dbColSeperator
      << "busied_no_inhibit(%)" << c_dbColSeperator
      << "live_G2SEM" << c_dbColSeperator
      << "good_RF" << c_dbColSeperator
      << "bad_RF" << c_dbColSeperator
      << "Ibooster1" << c_dbColSeperator
      << "Ibooster2" << c_dbColSeperator
      << "Ibooster3" << c_dbColSeperator
      << "Ibooster4" << c_dbColSeperator
      << "Ibooster5" << c_dbColSeperator
      << "Ibooster6" << c_dbColSeperator
      << "Ibooster7" << c_dbColSeperator
      << "Inh_output_delay(dec)" << c_dbColSeperator
      << "QIE_inh_delay(dec)" << c_dbColSeperator
      << "Min_Inh_Width(dec)" << c_dbColSeperator
      << "Inh_thres(dec)" << c_dbColSeperator
      << "QIE_busy_delay(dec)" << c_dbColSeperator
      << "Marker_delay(dec)" << c_dbColSeperator
      << "QIE_phase_adjust(dec)" << c_dbColSeperator
      << "Runtime" << c_dbColSeperator
      << c_dbRowEnd << endl;
  }
  
  
  
  
  ofs
    << timestamp << c_dbColSeperator
    << m_spillNumber << c_dbColSeperator
    << m_nm2ion/qieSum << c_dbColSeperator
    << m_nm3sem/qieSum << c_dbColSeperator
    << m_e906bm/qieSum << c_dbColSeperator
    << m_g2sem/qieSum << c_dbColSeperator
    << m_nm2ion << c_dbColSeperator
    << m_nm3sem << c_dbColSeperator
    //only calculate these if there was beam and the denomintor is not 0
    //  const double fracInhibit = (1E-6 < qieSum && !NoBeam() ) ? inhibitSum / qieSum : 0.;
    //    const double fracBusy    = (1E-6 < qieSum && !NoBeam() ) ? busySum / qieSum : 0.;
    //      const double fracLive    = NoBeam() ? 0 : (1. - fracInhibit - fracBusy);
    //        const double liveProtons = m_g2sem * fracLive;
    //
    //          //open/create the file in append mode
    //            ofs.open( filename.c_str(), ofstream::out | ofstream::app);
    //            << m_e906bm << c_dbColSeperator
    << m_g2sem  << c_dbColSeperator
    << qieSum << c_dbColSeperator
    << dutyFactorNoMI << c_dbColSeperator
    << m_inhibitDataVec->size() << c_dbColSeperator
    << inhibitSum << c_dbColSeperator
    << fracInhibit << c_dbColSeperator
    
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////
    //<< m_triggerDataVec->size() << c_dbColSeperator
    <<  ext_nTriggers  << c_dbColSeperator
    //////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////
    
    
    << busySum << c_dbColSeperator
    << fracBusy << c_dbColSeperator
    << liveProtons << c_dbColSeperator
    << goodRF << c_dbColSeperator
    << badRF << c_dbColSeperator
    << 0 << c_dbColSeperator  //these 0's are for booster integrals, which are no longer calculated
    << 0 << c_dbColSeperator
    << 0 << c_dbColSeperator
    << 0 << c_dbColSeperator
    << 0 << c_dbColSeperator
    << 0 << c_dbColSeperator
    << 0 << c_dbColSeperator
    << m_qieSettings->thresholdDelay << c_dbColSeperator
    << m_qieSettings->dataSumDelay << c_dbColSeperator
    << m_qieSettings->minInhibitWidth << c_dbColSeperator
    << m_qieSettings->threshold << c_dbColSeperator
    << m_qieSettings->qieDelay << c_dbColSeperator
    << m_qieSettings->markerDelay << c_dbColSeperator
    << m_qieSettings->qieClockPhase << c_dbColSeperator
    << c_dbRowEnd << endl;
  ofs.close();
  
  ///////////////////////////
  ext_nTriggers = 0; // reset 
  ///////////////////////////
  
  cout << "BeadDAQSpill::OutputToSpillAsciiDataSummary - Added to ascii output file: " << filename << endl;
  
  return true;
}


bool BeamDAQSpill::OutputToRootTrees( const std::string& filename ) const
{
  if(verbose)
    cout << "BeamDAQSpill::OutputToRootTrees -  filename = " << filename << endl;

  //remember the previous write
  static unsigned int prevSpill = -1;
  static unsigned int spillItr  = 0;

  //create output dir if needed
  boost::filesystem::path filepath( filename );
  boost::filesystem::path dirpath = filepath.parent_path();
  DAQUtil::Get().PrepareDir( dirpath.string() );

  //todo don't need to create a new file each run, only when none exists already
  TFile* outfile = new TFile( filename.c_str(), "update");//if no file exists, the file is created.

  //create a TDirectory inside the file for this spill
  //todo: Can we unify these names and use iteration0?, and also fixed width strings?
  TString outdirName;
  if( prevSpill == m_spillNumber ){
    ++spillItr;
    outdirName = Form("Run%d_Spill%d_iteration%d", m_runNumber, m_spillNumber, spillItr);
  }
  else{
    outdirName = Form("Run%d_Spill%d", m_runNumber, m_spillNumber);
    spillItr = 0;
  }

  prevSpill = m_spillNumber;

  //create the TDirectory and make it active
  TDirectory *outdir = outfile->mkdir( outdirName );
  outdir->cd();

  AddSpillTree();
  AddInhibitTree();
  AddTriggerTree();

  //deleting also closes the file and all objects it owns
  delete outfile;

  return true;
}

bool BeamDAQSpill::AddSpillTree() const
{
  cout << "BeamDAQSpill::AddSpillTree" << endl;


  return true;
}

bool BeamDAQSpill::AddInhibitTree() const
{
  cout << "BeamDAQSpill::AddInhibitTree" << endl;

  return true;
}

bool BeamDAQSpill::AddTriggerTree() const
{
  cout << "BeamDAQSpill::AddTriggerTree" << endl;

  return true;
}


bool BeamDAQSpill::OutputToEPICS( ) const
{
  cout << "BeadDAQSpill::OutputToEPICS" << endl;

  const double qieSum     = m_qieHeader->intensitySum;
  const double inhibitSum = m_qieHeader->inhibitSum;
  const double busySum    = m_qieHeader->busySum;
  const double dutyFactor = m_qieHeader->dutyFactor / 10.; //convert to %


  //block sums come from our addition of trigger and inhibits
  const double inhibitBlockSum = GetInhibitBlockSum( );
  const double busyBlockSum    = GetBusyBlockSum( );
  const double dutyFactorNoMI  = GetDutyFactor( );
  const double intensityBlockSum = GetIntensityBlockSum();

  const double fracInhibit = (1E-6 < qieSum && !NoBeam() ) ? inhibitSum / qieSum : 0.;
  const double fracBusy    = (1E-6 < qieSum && !NoBeam() ) ? busySum / qieSum : 0.;
  const double fracLive    = NoBeam() ? 0 : (1. - fracInhibit - fracBusy);
  const double liveBeam    = fracLive * m_g2sem;
  const double liveNM3     = fracLive * m_nm3sem;

  bool isOK = true;
  isOK = DAQUtil::Get().WriteEPICS( "BEAM_INHIBITED", Form("%.2f", 100.*fracInhibit ) ) && isOK;
  isOK = DAQUtil::Get().WriteEPICS( "BEAM_BUSIED", Form("%.2f", 100.*fracBusy) ) && isOK;
  isOK = DAQUtil::Get().WriteEPICS( "LIVE_BEAM", Form("%.2f", liveBeam ) ) && isOK;
  isOK = DAQUtil::Get().WriteEPICS( "LIVE_NM3SEM", Form("%.2f", liveNM3 ) ) && isOK;
  isOK = DAQUtil::Get().WriteEPICS( "DUTYFACTOR_BEAM", Form("%.2f", dutyFactorNoMI ) ) && isOK;

  const double dfDelta = dutyFactorNoMI - dutyFactor;
  const double dfRatio = 1E-6 < dutyFactor ? dutyFactorNoMI/dutyFactor : 0.;
  cout << "==============================" << endl;
  cout << "DEBUG board vs calc: " << endl;
  cout << Form( "  QIE sum = %.2E, onBoard = %.2E, delta = %.2E", intensityBlockSum, qieSum, (intensityBlockSum-qieSum) ) << endl;
  cout << Form( "  Inhibit sum = %.2E, onBoard = %.2E, delta = %.2E", inhibitBlockSum, inhibitSum, (inhibitBlockSum-inhibitSum) ) << endl;
  cout << Form( "  Busy sum = %.2E, onBoard = %.2E, delta = %.2E", busyBlockSum, busySum, (busyBlockSum-busySum) ) << endl;
  cout << Form( "  DutyFactor = %.2f%%, onBoard = %.2f%%, delta = %.2f%%, ratio = %.2f", dutyFactorNoMI, dutyFactor, dfDelta, dfRatio ) << endl;
  cout << "==============================" << endl;

  return isOK;
}

bool BeamDAQSpill::OutputToACNET( ) const
{
  cout << "BeamDAQSpill::OutputToACNET" << endl;

  if( c_skipACNET )
  {
    cout << "SKIPPING BECAUSE ACNET IS FAILING" << endl;
    return true;
  }

  const double dutyFactorNoMI  = GetDutyFactor( );

  const double qieSum     = m_qieHeader->intensitySum;
  const double inhibitSum = m_qieHeader->inhibitSum;
  const double busySum    = m_qieHeader->busySum;

  const double fracInhibit = (1E-6 < qieSum && !NoBeam() ) ? inhibitSum / qieSum : 0.;
  const double fracBusy    = (1E-6 < qieSum && !NoBeam() ) ? busySum / qieSum : 0.;
  const double fracLive    = NoBeam() ? 0 : (1. - fracInhibit - fracBusy);


  bool isOK = DAQUtil::Get().WriteACNET( "F:SQDUTY", Form( "%.2f", dutyFactorNoMI) );
  isOK = DAQUtil::Get().WriteACNET( "F:SQSPIL", Form( "%d", m_spillNumber) ) && isOK;
  isOK = DAQUtil::Get().WriteACNET( "F:SQINH", Form( "%.2f", 100.*fracInhibit ) ) && isOK;
  isOK = DAQUtil::Get().WriteACNET( "F:SQBUSY", Form( "%.2f", 100.*fracBusy ) ) && isOK;
  isOK = DAQUtil::Get().WriteACNET( "F:SQLIVE", Form( "%.2f", 100.*fracLive ) ) && isOK;

  return isOK;
}


bool BeamDAQSpill::OutputToACNET2( double mean_deadtime ) const
{
  cout << endl;
  cout << "BeamDAQSpill::OutputToACNET2" << endl;
  
  if( c_skipACNET )
  {
    cout << "SKIPPING BECAUSE ACNET IS FAILING" << endl;
    return true;
  }
  
// added on Feb.XX 2017
//  isOK = DAQUtil::Get().WriteACNET( "F:SQDEAD", Form( "%.2f", mean_deadtime ) ) && isOK;
  cout << "test -- average of deadtime -- "  <<  "F:SQDEAD --- " << Form( "%.2f", mean_deadtime ) << endl;
  //cout << endl;
  
  
  
  //  isOK = DAQUtil::Get().WriteACNET( "F:SQDEAD", Form( "%.2f", mean_deadtime ) ) && isOK;
  
  bool  isOK = false;
  // added on Feb.XX 2017
  //bool  isOK = DAQUtil::Get().WriteACNET( "F:SQDEAD", Form( "%.2f", mean_deadtime ) ) ;
  
  return isOK;
}





bool BeamDAQSpill::OutputToVisualROOT( const std::string& dirname /* = "" */ ) const
{
  cout << "BeamDAQSpill::OutputToVisualROOT" << endl;
  
  //turn off stats and make title huge (needs to use gStyle)
  const float oldTitleW = gStyle->GetTitleW();
  const float oldTitleH = gStyle->GetTitleH();
  const int oldOptStat = gStyle->GetOptStat();
  const float oldTitleSize = gStyle->GetTitleFontSize();
  gStyle->SetTitleW( .3 );
  gStyle->SetTitleH( .1 );
  gStyle->SetOptStat(0);
  gStyle->SetTitleFontSize(.12);
  
  const double qieSum     = m_qieHeader->intensitySum;
  const double inhibitSum = m_qieHeader->inhibitSum;
  const double busySum    = m_qieHeader->busySum;
  
  //block sums come from our addition of trigger and inhibits
  const double dutyFactorNoMI  = GetDutyFactor( );
  const long int goodRF = GetGoodRF();
  const long int badRF  = GetBadRF();
  
  const double fracInhibit = (1E-6 < qieSum && !NoBeam() ) ? inhibitSum / qieSum : 0.;
  const double fracBusy    = (1E-6 < qieSum && !NoBeam() ) ? busySum / qieSum : 0.;
  const double fracLive    = NoBeam() ? 0 : (1. - fracInhibit - fracBusy);
  
  const double inhibitedProtons = m_g2sem * fracInhibit;
  const double liveProtons      = m_g2sem * fracLive;
  
  const double chargeNorm = GetChargeNormalization();
  
  //todo: write these to a TCanvas for display
  //build our display line by line
  vector<string> lines;
  TDatime dt = GetTDatime();
  lines.push_back( dt.AsString() );
  lines.push_back( Form( "SeaQuest Spill Number: %d", m_spillNumber) );
  if( NoBeam() )
    lines.push_back( "Received No Beam!" );
  lines.push_back( Form("Duty Factor @53MHz = %.2f%%", dutyFactorNoMI) );
  lines.push_back( Form("Beam Inhibited = %.2f%%", 100*fracInhibit ) );
  lines.push_back( Form("Beam Busy      = %.2f%%", 100*fracBusy ) );
  lines.push_back( Form("Beam Live      = %.2f%%", 100*fracLive ) );
  lines.push_back( Form("Turn13 = %.1f, Bunch13 = %d, NBSYD = %.1f", m_turn13, m_bunch13, m_nbsyd ) );
  lines.push_back( Form("G2SEM  = %.2E,  G2SEM/QIESum  = %.2f", m_g2sem, chargeNorm) );
  lines.push_back( Form("NM3SEM = %.2E,  NM3SEM/QIESum = %.2f", m_nm3sem, m_nm3sem/qieSum ) );
  lines.push_back( Form("N4Beam = %.2E,  N4Beam/QIESum = %.2E", m_e906bm, m_e906bm/qieSum ) );
  lines.push_back( Form("NM3SEM/G2SEM = %.2f", m_nm3sem/m_g2sem ) );
  lines.push_back( Form("RF Above Threshold = %zu", badRF) );
  lines.push_back( Form("RF Below Threshold = %zu", goodRF) );
  lines.push_back( Form("#Protons Inhibited = %.2E", inhibitedProtons ) );
  lines.push_back( Form("#Protons Accepted  = %.2E", liveProtons ) );
  lines.push_back( Form("Inhibit Output Delay = 0x%02x", m_qieSettings->thresholdDelay ) );
  lines.push_back( Form("QIE Inhibit Delay    = 0x%02x", m_qieSettings->dataSumDelay ) );
  lines.push_back( Form("Min. Inhibit Width   = 0x%02x", m_qieSettings->minInhibitWidth ) );
  lines.push_back( Form("Inhibit Threshold    = 0x%02x", m_qieSettings->threshold ) );
  lines.push_back( Form("QIE Busy Delay       = 0x%02x", m_qieSettings->qieDelay ) );
  lines.push_back( Form("Marker Delay         = 0x%02x", m_qieSettings->markerDelay ) );
  lines.push_back( Form("QIE Phase Adjust     = 0x%02x", m_qieSettings->qieClockPhase ) );
  lines.push_back( Form("QIE Linearized Sum = %.2E", qieSum ) );
  lines.push_back( Form("QIE Inhibit Sum    = %.2E",     inhibitSum ) );
  lines.push_back( Form("QIE Busy Sum       = %.2E",        busySum ) );
  
  //write to new TPaveText
  //get canvases if they exist, create them if not
  TCanvas *cText = DAQUtil::Get().GetOrCreateCanvas( c_beamTextCanvasName.c_str() );
  cText->Clear();
  cText->cd();
  if( 0 == textPave )
  {
    textPave = new TPaveText( .05, .05, .95, .95 );
    textPave->SetName("textPave");
  }
  textPave->Clear();
  
  for( vector<string>::iterator i = lines.begin(); i != lines.end(); ++i )
  {
    textPave->AddText( i->c_str() );
    cout << *i << endl;
  }
  textPave->Draw();
  cText->Update();
  
  //------------------------
  //make the frame plots
  // 12-turns in a row
  //------------------------
  //plot intensity for each RF for a few turns
  //do 3 turns per plot
  const size_t nHists = 4;
  const size_t turnsPerHist = 3;
  
  //split canvas to make enough pads for histograms + label
  TCanvas *cFrames = DAQUtil::Get().GetOrCreateCanvas( c_beamFramesCanvasName.c_str() );
  cFrames->Clear();
  cFrames->cd();
  cFrames->Divide( 1, nHists+1 );

  //top pad is for text
  cFrames->cd(1);
  if( 0 == framePave )
  {
    framePave = new TPaveText( .05, 0.05, 0.95, 0.95 );
    framePave->SetName("framePave");
  }
  framePave->Clear();

  framePave->AddText( dt.AsString() );
  framePave->AddText( Form( "SeaQuest Spill Number: %d", m_spillNumber) );
  
  if( NoBeam() )
    framePave->AddText( "Received No Beam!" );
  
  framePave->AddText( Form("Duty Factor @53MHz = %.2f%%", dutyFactorNoMI) );
  framePave->AddText( Form("Turn13 = %.1f, Bunch13 = %d, NBSYD = %.1f", m_turn13, m_bunch13, m_nbsyd ) );
  framePave->AddText( Form("G2SEM  = %.2E,  G2SEM/QIESum  = %.2f", m_g2sem, chargeNorm) );
  framePave->Draw();
  cFrames->Update();
  
  //will draw a line to indicate the threshold
  const double tresholdVal = m_qieSettings->threshold * chargeNorm;
  if( NULL != thresholdLine )
    delete thresholdLine;
  
  thresholdLine = new TLine( 0, tresholdVal, turnsPerHist*c_BucketsPerTurn, tresholdVal);
  thresholdLine->SetLineColor(kRed);
  thresholdLine->SetLineWidth(2);
  
  //if true, do 12 frames in a row starting at 2s
  //if false, do 3 frames in a row at each of 2s, 2.5s, 3s, 3.5s 
  bool phaseLockFrames = false;
  const float turnsPerS = c_MIFrequency/c_BucketsPerTurn;
  
  vector<TH1D> hists;
  for( size_t iHist = 0; iHist != nHists; ++iHist )
  {
    
    //first turn is (2 + 0.5*iHist) seconds into spill for not phase locked
    //first turn is  2s + iHist*turnsPerHist for phase locked
    const size_t firstTurnOfHist = 
      phaseLockFrames ? 
      2*turnsPerS + iHist*turnsPerHist : 
      (2 + .5*iHist) * turnsPerS;
    
    const TString title = 
      phaseLockFrames ? 
      Form( "Turns %zu-%zu", firstTurnOfHist, firstTurnOfHist+turnsPerHist) :
      Form( "%zu Turns starting at %.1fs", turnsPerHist, 2 + .5*iHist ) ;
    
    //create the histogram
    TH1D hist( 
        Form("frameHist_%zu", iHist), title,
        turnsPerHist*c_BucketsPerTurn, 0, turnsPerHist*c_BucketsPerTurn-1
        );
    hists.push_back(hist);
    hist.GetXaxis()->SetTitle( "RF Bucket Index" );
    hist.GetYaxis()->SetTitle( "Protons per RF" );
    hist.SetMaximum( 2 * tresholdVal );
    
    hist.GetXaxis()->SetTitleSize( .1 );
    hist.GetXaxis()->SetTitleOffset(.6);
    hist.GetYaxis()->SetTitleSize( .08 );
    hist.GetYaxis()->SetTitleOffset(.28);
    
    hist.GetXaxis()->SetLabelSize( .08 );
    hist.GetYaxis()->SetLabelSize( .08 );
    
    size_t iHistBin = 0;
    for( size_t turnThisHist = 0; turnThisHist != turnsPerHist; ++turnThisHist )
    {
      const size_t turnIdx = firstTurnOfHist + turnThisHist;
      if( m_qieTurnVec->size() <= turnIdx )
        break;
      
      QIETurn *turn = m_qieTurnVec->at(turnIdx);
      for( size_t iBucket = 0; iBucket != c_BucketsPerTurn; ++iBucket )
      {
        unsigned char qie = turn->rfIntensity[iBucket];
        unsigned int  linearizedQIE = QIE_Conversion_Map[qie];
        hist.SetBinContent( iHistBin, linearizedQIE*chargeNorm );
        ++iHistBin;
      }//end loop over buckets in this turn
    }//end loop over turns for this histogram
    
    //draw a copy so it stays in the pad after we move on
    cFrames->cd( iHist + 2 );
    hist.DrawCopy();
    
    //draw a line to indicate threshold
    thresholdLine->Draw();
  }//end loop over histogtrams
  
  cFrames->Update();
  //-- END of frames plot
  
  //save the plots
  const string dayOfWeekAbbr[] = { "NONE", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
  const string datestamp = Form("%d-%02d-%02d-%s", dt.GetYear(), dt.GetMonth(), dt.GetDay(), dayOfWeekAbbr[dt.GetDayOfWeek()].c_str()  );
  const string timestamp = Form("%04d%02d%02d%02d%02d%02d", dt.GetYear(), dt.GetMonth(), dt.GetDay(), dt.GetHour(), dt.GetMinute(), dt.GetSecond() );
  
  //save frames canvas
  {
    //use user's dirname if given, otherwise put in official location
    string archiveDir = dirname.empty() ? Form( "%s/pics/pics/%s", c_BeamDAQ_OUTDIR.c_str(), datestamp.c_str() ) : dirname;
    DAQUtil::Get().PrepareDir( archiveDir );
    string imgFile = Form( "%s/cerenkov_%s.png", archiveDir.c_str(), timestamp.c_str() );
    cFrames->SaveAs( imgFile.c_str() );
    system( Form("chmod 777 %s", imgFile.c_str()) );
    
    //copy to the live version, todo: can use soft link
    if( dirname.empty() )
    {
      string latestImgFile = Form( "%s/pics/cerenkov_last.png", c_BeamDAQ_OUTDIR.c_str() );
      system( Form("cp -u -f %s %s", imgFile.c_str(), latestImgFile.c_str() ) );
      cout << "See latest cerenkov plots at: " << latestImgFile << endl;
    }
  }
  
  
  //save stats canvas
  {
    //use user's dirname if given, otherwise put in official location
    string archiveDir = dirname.empty() ? Form( "%s/pics/stats/%s", c_BeamDAQ_OUTDIR.c_str(), datestamp.c_str() ) : dirname;
    DAQUtil::Get().PrepareDir( archiveDir );
    string imgFile = Form( "%s/cerenkov_stats_%d.png", archiveDir.c_str(), m_spillNumber );
    cText->SaveAs( imgFile.c_str() );
    system( Form("chmod 777 %s", imgFile.c_str()) );
    
    //copy to the live version, todo: can use soft link
    if( dirname.empty() )
    {
      string latestImgFile = Form( "%s/pics/cerenkov_stats.png", c_BeamDAQ_OUTDIR.c_str() );
      system( Form("cp -u -f %s %s", imgFile.c_str(), latestImgFile.c_str() ) );
      cout << "See latest cerenkov stats at: " << latestImgFile << endl;
    }
  }

  //restore old style
  gStyle->SetTitleH( oldTitleH );
  gStyle->SetTitleW( oldTitleW );
  gStyle->SetOptStat( oldOptStat );
  gStyle->SetTitleFontSize( oldTitleSize );

  return true;
}

bool BeamDAQSpill::SetRunNumber( int runNumber /* = 0 */ )
{
  cout << "BeamDAQSpill::SetRunNumber" << endl;
  //if 0 is given, then get the run number from coda
  if( 0 == runNumber )
    runNumber = DAQUtil::Get().GetRunNumber();

  //if we get 0, something went wrong
  bool isOK = ( 0 != runNumber );

  m_runNumber = runNumber;

  return isOK;
}

bool BeamDAQSpill::SetSpillNumber( int spillNumber /* = 0 */ )
{
  cout << "BeamDAQSpill::SetSpillNumber" << endl;
  //if 0 is given, then get the spill number with DAQUtil
  if( 0 == spillNumber )
    spillNumber = DAQUtil::Get().GetSpillNumber();

  //if we get 0, something went wrong
  bool isOK = ( 0 != spillNumber );

  m_spillNumber = spillNumber;

  return isOK;
} 

bool BeamDAQSpill::OutputSpillNumber( ) const
{
  cout << "BeamDAQSpill::OutputSpillNumber" << endl;
  ofstream fout;
  fout.open( c_BeamDAQSpillFile.c_str() );

  fout << m_spillNumber << endl;
  fout.close();
  return true;
}

bool BeamDAQSpill::UpdateACNETVariables( )
{
  cout << "BeamDAQSpill::UpdateACNETVariables" << endl;
  //store each value in member variables
  //todo - check for errors
  DAQUtil::Get().ReadACNET( "G:TURN13", m_turn13 );
  DAQUtil::Get().ReadACNET( "G:BNCH13", m_bunch13 );
  DAQUtil::Get().ReadACNET( "G:NBSYD",  m_nbsyd );
  DAQUtil::Get().ReadACNET( "F:NM3SEM", m_nm3sem );
  DAQUtil::Get().ReadACNET( "F:NM2ION", m_nm2ion );
  DAQUtil::Get().ReadACNET( "S:G2SEM",  m_g2sem );
  DAQUtil::Get().ReadACNET( "F:E906BM", m_e906bm );
  return true;
}

double BeamDAQSpill::GetTurn13()  const { return m_turn13; }
int BeamDAQSpill::GetBunch13() const { return m_bunch13; }
double BeamDAQSpill::GetNBSYD()   const { return m_nbsyd; }
double BeamDAQSpill::GetNM3SEM() const { return m_nm3sem; }
double BeamDAQSpill::GetNM2ION() const { return m_nm2ion; }
double BeamDAQSpill::GetG2SEM()  const { return m_g2sem; }
double BeamDAQSpill::GetE906BM() const { return m_e906bm; }

double BeamDAQSpill::GetChargeNormalization() const
{
  cout << "BeamDAQSpill::GetChargeNormalization" << endl;

  double norm = 1.;
  //  const double qieSum = m_qieSettings->qieBoardSum;
  const double qieSum = m_qieHeader->intensitySum;

  //use g2sem to normalize to number of protons
  //  i.e. normalization factor = g2sem / qiesum
  //only use the normaliation if both numerator and denominator are legitimate
  if( NoBeam() )
    cout << "BeamDAQSpill::GetChargeNormalization -  Warning - g2sem is " << m_g2sem << ", using normalization of 1." << endl;
  else if( qieSum < 1E-4 )
    cout << "BeamDAQSpill::GetChargeNormalization -  Warning - qieSum is " << qieSum << ", using normalization of 1." << endl;
  else
    norm = m_g2sem / qieSum;

  cout << Form(" Normalization = %.2f = %.2f/%.2f = g2sem/qiesum", norm, m_g2sem, qieSum) << endl;

  return norm;
}


double BeamDAQSpill::GetInhibitBlockSum( bool forceCalc /* = false */ ) const
{
  //if sum has aleady been calculated then return it, unless we are forcing recalculation
  //note: m_inhibitSum initialized to -1
  if( forceCalc || m_inhibitSum < 0.)
  {
    //loop over InhibitData and add to the sum
    m_inhibitSum = 0.;
    for( InhibitDataVec::const_iterator i = m_inhibitDataVec->begin(); i != m_inhibitDataVec->end(); ++i )
      m_inhibitSum += (*i)->inhibitSum;
  }

  return m_inhibitSum;
}

double BeamDAQSpill::GetInhibitBucket( bool forceCalc /* = false */ ) const
{
    //if sum has aleady been calculated then return it, unless we are forcing recalculation
    //note: m_inhibitBucket initialized to -1

    //const int dturn = 0;

    if( forceCalc || m_inhibitBucket < 0.)
    {
      //loop over InhibitData and add to the sum
      m_inhibitBucket = 0.;
      for( InhibitDataVec::const_iterator i = m_inhibitDataVec->begin(); i != m_inhibitDataVec->end(); ++i ){
        int dturn = (*i)->releaseTurn - (*i)->onsetTurn;
        dturn = (dturn>0? dturn: dturn +369000);
        m_inhibitBucket += dturn*c_BucketsPerTurn + (*i)->releaseRF - (*i)->onsetRF;
      }
    }
    
    return m_inhibitBucket;
}
    

double BeamDAQSpill::GetBusyBlockSum( bool forceCalc /* = false */ ) const
{
  //if sum has aleady been calculated then return it, unless we are forcing recalculation
  //note: m_busySum initialized to -1
  if( forceCalc || m_busySum < 0.)
  {
    //loop over TriggerData and add to the sum
    m_busySum = 0.;
    for( TriggerDataVec::const_iterator i = m_triggerDataVec->begin(); i != m_triggerDataVec->end(); ++i ){
      // memo (debug); data exists
      m_busySum += (*i)->busyNotInhibit;
    }
  }

  return m_busySum;
}

double BeamDAQSpill::GetBusyBucket( bool forceCalc /* = false */ ) const
{
    //if sum has aleady been calculated then return it, unless we are forcing recalculation
    //note: m_busyBucket initialized to -1

    //const int dturn = 0;

    if( forceCalc || m_busyBucket < 0.)
    {
    //loop over InhibitData and add to the sum
    m_busyBucket = 0.;
    for( TriggerDataVec::const_iterator i = m_triggerDataVec->begin(); i != m_triggerDataVec->end(); ++i ){
     int dturn = (*i)->releaseTurn - (*i)->onsetTurn;
     dturn = (dturn>0? dturn: dturn +369000);
     m_busyBucket += dturn*c_BucketsPerTurn + (*i)->releaseRF - (*i)->onsetRF;
    }
  }
    
  return m_busyBucket;
}
    

double BeamDAQSpill::GetIntensityBlockSum( bool forceCalc /* = false */ ) const
{
  //Do turn sums if necessary.
  if( forceCalc || m_intensitySum < 0.)
    CalculateTurnSums();

  return m_intensitySum;
}

double BeamDAQSpill::GetIntensitySqBlockSum( bool forceCalc /* = false */ ) const
{
  //Do turn sums if necessary.
  if( forceCalc || m_intensitySqSum < 0.)
    CalculateTurnSums();

  return m_intensitySqSum;
}

unsigned long BeamDAQSpill::GetGoodRF( bool forceCalc /* = false */ ) const
{
  //Do turn sums if necessary.
  if( forceCalc || 0 == m_goodRF+m_badRF )
    CalculateTurnSums();

  return m_goodRF;
}

unsigned long BeamDAQSpill::GetBadRF( bool forceCalc /* = false */ ) const
{
  //Do turn sums if necessary.
  if( forceCalc || 0 == m_goodRF+m_badRF )
    CalculateTurnSums();

  return m_badRF;
}




bool BeamDAQSpill::CalculateTurnSums() const
{
  m_intensitySum = m_intensitySqSum = 0.;
  m_goodRF = m_badRF = 0;

  //add sum for each turn (sum over buckets done in construction of turns)
  for( QIETurnVec::const_iterator iTurn = m_qieTurnVec->begin(); iTurn != m_qieTurnVec->end(); ++iTurn )
  {
    m_intensitySum += (*iTurn)->intensitySum;
    m_intensitySqSum += (*iTurn)->intensitySqSum;
    for( int i = 0; i != c_BucketsPerTurn; ++i )
    {
      const unsigned char rfBin = (*iTurn)->rfIntensity[i];
      const unsigned int  rfIntensity = QIE_Conversion_Map[rfBin];
      if( m_qieSettings->threshold < rfIntensity )
        ++m_badRF;
      else
        ++m_goodRF;
    } 
  }

  return true;
}


double BeamDAQSpill::GetDutyFactor( bool forceCalc /* = false */ ) const
{

  //if we didn't get beam then return 0
  if( NoBeam() )
    return 0.;

  if( forceCalc )
  {

    //old method was to calculate this on our own
    //by adding up QIE intensities

    const double intensitySum = GetIntensityBlockSum( forceCalc );
    const double intensitySumSq = GetIntensitySqBlockSum( forceCalc );

    //if intensitySumSq is 0, then set DF to 0.  Otherwise calculate as a %
    const double dutyFactor = ( 1E-6 < intensitySumSq ) ? 100 * pow( intensitySum, 2. ) / ( c_BucketsPerSpill * intensitySumSq ) : 0.;

    return dutyFactor;
  }
  else
    return m_qieHeader->dutyFactor / 10.;
}

bool BeamDAQSpill::NoBeam() const
{
  return 0;
  //return m_g2sem < c_G2SEM_PEDESTAL;
}

//===========================
// ROOT utlities
//===========================
void BeamDAQSpill::SetROOTStyle()
{
  //---------------------------------
  //create a style type called seaquest
  //---------------------------------
  //styles borrowed from MINERvA
  TStyle *style = new TStyle("seaquest", "SeaQuest default style");

  //start from plain style 
  style->SetCanvasBorderMode(0);
  style->SetPadBorderMode(0);
  style->SetCanvasColor(0);
  style->SetPadColor(0);
  style->SetTitleColor(1);
  style->SetStatColor(0);
  style->SetFrameBorderMode(0);

  // Set the size of the default canvas: 600x500 looks almost square.
  style->SetCanvasDefH(500);
  style->SetCanvasDefW(600);
  style->SetCanvasDefX(10);
  style->SetCanvasDefY(10);

  // Color Scheme - Rainbow
  style->SetPalette(1);

  // Line Widths
  style->SetFrameLineWidth(2);
  style->SetLineWidth(2);
  style->SetHistLineWidth(2);

  // Marker Styles
  style->SetMarkerStyle(20);

  // Stats
  //style->SetOptStat(1111111); //use this if you want stats
  //style->SetOptStat(0000);
  style->SetOptFit(0000);

  // Margins
  style->SetPadTopMargin(0.09);
  style->SetPadBottomMargin(0.15);
  style->SetPadLeftMargin(0.15);
  style->SetPadRightMargin(0.15);

  // Titles
  style->SetTitleBorderSize(0);
  style->SetTitleFillColor(kWhite);
  style->SetTitleAlign(23);    // centered
  style->SetTitleX(0.5);       // the center should be in the middle
  style->SetTitleW(0.6);       // don't use the full width so that the centering is visible
  //style->SetOptTitle(0); //sometimes we don't want titles at all

  // Axis Styles
  {
    double axis_title_offset_x = 1.15;
    double axis_title_offset_y = 1.2;
    double axis_title_offset_z = .75;
    int axis_title_font = 62;
    double axis_title_size = 0.04;
    int axis_label_font = 42;
    double axis_label_size = 0.035;
    style->SetTitleOffset( axis_title_offset_x, "X" );
    style->SetTitleOffset( axis_title_offset_y, "Y" );
    style->SetTitleOffset( axis_title_offset_z, "Z" );
    style->SetTitleSize( axis_title_size, "XYZ" );
    style->SetTitleFont( axis_title_font, "XYZ" );
    style->SetLabelFont( axis_label_font, "XYZ" );
    style->SetLabelSize( axis_label_size, "XYZ" );
  }


  // Errors
  style->SetEndErrorSize(3);
  style->SetErrorX(0.5);

  // Now use the style
  // Do these 2 commands anytime to revert to SeaQuest style
  gROOT->SetStyle("seaquest");
  gROOT->ForceStyle();
}
