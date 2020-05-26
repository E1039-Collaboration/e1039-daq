#include "BeamDAQ/BeamAnalysis.h"
#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/BeamDAQSpill.h"
#include "BeamDAQ/DAQUtil.h"
#include "BeamDAQ/QIEDataTypes.h"

#include <TROOT.h>
#include <TStopwatch.h>
#include <TVirtualFFT.h>
#include <TGraph.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TDirectory.h>
#include <TFile.h>


#include <TDatime.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TGraph.h>

#include <time.h>
#include <locale.h>

#include <TStopwatch.h>
#include <TApplication.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <TDatime.h>

using namespace std;

//------------------
//helper classes
//------------------

//FreqHist
FreqHist::FreqHist( float freq, float start, float stop, const std::string& name, bool cleanSelf /* = false */ ) :
  m_freq(freq),
  m_startTime(start),
  m_stopTime(stop),
  m_name(name),
  m_cleanSelf(cleanSelf)
{
  const unsigned int nBins( (m_stopTime - m_startTime ) * m_freq );
  m_hist = new TH1F( m_name.c_str(), (m_name + "; Time in Spill (s); Linearized Intensity").c_str(), nBins, m_startTime, m_stopTime );

  //don't let ROOT manage this histogram, we will delete it outselves in destructor
  if( m_cleanSelf )
    m_hist->SetDirectory(0);
}

FreqHist::~FreqHist()
{
  if( m_cleanSelf && m_hist )
    delete m_hist;
}

void FreqHist::AddBucket( unsigned int idx, float intensity )
{
  //is this bucket out of bounds?
  const unsigned int startBucket = c_MIFrequency * m_startTime;
  const unsigned int stopBucket  = c_MIFrequency * m_stopTime;
  if( idx < startBucket || stopBucket < idx )
    return;

  //find bin for this bucket
  const unsigned int bucketBin = ( idx - startBucket ) * (m_freq / c_MIFrequency);
  const float oldContent = m_hist->GetBinContent( bucketBin );
  m_hist->SetBinContent( bucketBin, oldContent + intensity  );
}

float FreqHist::DutyFactor() const
{
  float sum = 0., sumSq = 0.;
  for( int bin = 1; bin <= m_hist->GetNbinsX(); ++bin )
  {
    sum += m_hist->GetBinContent(bin);
    sumSq += pow( m_hist->GetBinContent(bin), 2);
  }
  return 100 * sum*sum / ( m_hist->GetNbinsX() * sumSq );
}

//end of FreqHist functions

void GetStdFreqHists( std::vector<FreqHist*>& freqHists, const std::string& suffix /* = "" */, bool cleanSelf /* = false */ )
{
  freqHists.clear();
  freqHists.push_back( new FreqHist( 1.0E3, 0.,   4., "FreqHist_1kHz"   + suffix, cleanSelf ) );
  freqHists.push_back( new FreqHist( 7.5E3, 0.,   4., "FreqHist_7.5kHz" + suffix, cleanSelf ) );
  freqHists.push_back( new FreqHist( 1.0E4, 0.,   4., "FreqHist_10kHz"  + suffix, cleanSelf ) );
  freqHists.push_back( new FreqHist( 1.0E5, 2.,   3., "FreqHist_100kHz" + suffix, cleanSelf ) );
  freqHists.push_back( new FreqHist( 1.0E6, 1.0, 1.1, "FreqHist_1MHz"   + suffix, cleanSelf ) );
}


//----------------
// Functions
//----------------
bool AnalyzeSpill(  double *mean_deadtime,
                    const BeamDAQSpill* spill, const std::string& outfileName, bool doFFT /*=false*/ 
                  )
{
  
  cout << "BeamAnalysis::AnalyzeSpill - " << outfileName << endl;
  
  //create output dir if needed
  boost::filesystem::path filepath( outfileName );
  boost::filesystem::path dirpath = filepath.parent_path();
  DAQUtil::Get().PrepareDir( dirpath.string() );
  
  //use this to set variable sized bins
  double tmpQIEConversion[256];
  for( int i = 0; i != 256; ++i )
    tmpQIEConversion[i] = double(QIE_Conversion_Map[i]);
  
  TFile fout( outfileName.c_str(), "RECREATE" );
  
  //---------------------------------
  //-- Plots made from turn-level info
  TH1F *hTurnDF = new TH1F( "hTurnDF", "Duty Factor per Turn; Duty Factor (%); N Turns", 100, 0, 100 );
  TH2F *hTurn_vs_DF = new TH2F( "hTurn_vs_DF", "Duty Factor vs Turn; Turn Index; Duty Factor", 1000, 0, 400000, 50, 0, 100 );
  
  const unsigned int highestPossibleIntensity = QIE_Conversion_Map[255];
  TH1F *hMaxIntensity = new TH1F( "hTurnMaxIntensity", "Max Intensity of Bucket in Turn; Linearized QIE; N Turns", 256, 0, highestPossibleIntensity );
  TH1F *hAvgIntensity = new TH1F( "hTurnAvgIntensity", "Avg Intensity of Bucket in Turn; Linearized QIE; N Turns", 256, 0, highestPossibleIntensity/4. );
  TH1F *hMaxIntensity_trueBin = new TH1F( "hTurnMaxIntensity_trueBin", "Max Intensity of Bucket in Turn; Linearized QIE; N Turns", 252, tmpQIEConversion );
  cout << endl;
  //cout << endl;
  //cout << endl;
  //cout << endl;
  //cout << "hoge 1" << endl;
  //cout << endl;
  //cout << endl;
  //cout << endl;
  //cout << endl;
  //cout << endl;
  
  const float timePerBucket = 1E6 / c_MIFrequency; //in microseconds
  for( TriggerDataVec::const_iterator iTrigger = spill->m_triggerDataVec->begin(); iTrigger != spill->m_triggerDataVec->end(); ++iTrigger )
  {
    
  //cout << endl;
  //cout << endl;
  //cout << endl;
  //cout << endl;
  //  cout << "hoge 2" << endl;
  //  
  //cout << endl;
  //cout << endl;
  //cout << endl;
  //cout << endl;
  //cout << endl;
  //
    //only use complete triggetrs
    if( (*iTrigger)->rfIntensity.size() < /*33*/  25  )
    {
      cout << "Trigger only has " << (*iTrigger)->rfIntensity.size() << " rf recorded. skip..." << endl;
      continue;
    }
    
    int indexOfMaxBucket = 0;
    unsigned int maxBucketIntensity = 0;
    for( size_t index = 0; index !=  /*33*/ 25 ; ++index )
    {
      const unsigned int intensity = (*iTrigger)->rfIntensity.at(index);
      if( maxBucketIntensity < intensity )
      {
        indexOfMaxBucket = (int)index;
        maxBucketIntensity = intensity;
      }
      hTriggerRFIntensity->Fill( int(index) - /*16*/ 12 , intensity );
    }
    hMaxRFTriggerOffset->Fill( indexOfMaxBucket - /*16*/ 12 );
    
    // -------------------
    // deadtime calc.(old)
    // -------------------
    
    //note: deadtime is always greater than 1 turn
    //const int deadBuckets = 
    //  (  c_BucketsPerTurn - (*iTrigger)->onsetRF  )
    //   + (*iTrigger)->releaseRF
    //   + c_BucketsPerTurn * ( (*iTrigger)->releaseTurn - (*iTrigger)->onsetTurn - 1 );
    //const float deadTime = deadBuckets * timePerBucket;
    
    
    // -------------------
    // deadtime calc.(new)
    // -------------------
    
    double RF1 = (*iTrigger)->onsetRF  ;
    double RF2 = (*iTrigger)->releaseRF;
    
    double Turn1 = (*iTrigger)->onsetTurn   ;
    double Turn2 = (*iTrigger)->releaseTurn ;
    
    if ( RF1 > RF2 ) {
      RF2 += c_BucketsPerTurn;
      Turn2 -= 1;
    }
    
    double deltaR = RF2   - RF1  ;
    double deltaT = Turn2 - Turn1;
    
    double deadTime = ( deltaT * c_BucketsPerTurn + deltaR ) * 18.83e-9 ;
    // 1 / 53.1MHz = 18.83 nsec
    
    if (deltaT < 0 ) cout << "delta T is negative !!!" << endl;
    
    hBusyPerTrigger    ->Fill( deadTime * 1.e6);
    hBusyPerTriggerZoom->Fill( deadTime * 1.e6);
    
    /////////////////////////////////////
    //////*mean_deadtime = deadTime * 1.e6 ;
    *mean_deadtime = -11111111. ;
    /////////////////////////////////////
    
    // Memo:
    //   onsetRF,releaseTurn,onsetTurn;  in src/QIEDataTypes.cxx
    //   c_BucketsPerTurn  = 588;  in BeamDAQ/BeamDAQGlobals.h
  }
  
  
  
  //---------------------------------
  //-- Plots made for frequency analysis
  //
  //note: there are many many bins and pieces of data to add to histograms.
  //      TH1::Fill is slow because it does a binary search to find the histogram bin to fill.
  //      Since we know the bin index, it is much faster to set bin content directly.
  //      As a result, the stats are nonsense (but I don't think we care)
  //      ROOT should really make other methods available...
  
  //partially analysis spill with several threads
  vector< vector<FreqHist*> > freqHistFamilies;
  
  vector<TH2F*> rfIntensityThreadHists;
  TH2F *rfIntensityHist = new TH2F( "intensity_vs_bucket", "Intensity vs Bucket; rf Bucket; Linearized Intensity", c_BucketsPerTurn-1, 0, c_BucketsPerTurn-1, 255-1, tmpQIEConversion );
  
  boost::thread_group threads;
  unsigned int nThreads = 8;
  unsigned int turnsPerThread = spill->m_qieTurnVec->size() / nThreads;
  for( unsigned int iThread = 0; iThread != nThreads; ++iThread )
  {
    vector<FreqHist*> threadFreqHists;
    const bool cleanSelf = true;
    const string suffix  = Form( "_%u", iThread );
    GetStdFreqHists( threadFreqHists, suffix, cleanSelf );
    freqHistFamilies.push_back(threadFreqHists);
    
    TH2F *rfIntensityThreadHist = dynamic_cast<TH2F*>( rfIntensityHist->Clone( Form( "%s_%u", rfIntensityHist->GetName(), iThread) ) );
    rfIntensityThreadHist->SetDirectory(0);
    rfIntensityThreadHists.push_back(rfIntensityThreadHist);
    
    //first turn is included, lastTurn is excluded
    unsigned int firstTurn = iThread * turnsPerThread;
    unsigned int lastTurn  = std::min( (1+iThread) * turnsPerThread, (unsigned int)spill->m_qieTurnVec->size() );
    
    boost::thread *t = new boost::thread( &FillFreqHists, threadFreqHists, rfIntensityThreadHist, spill, firstTurn, lastTurn );
    threads.add_thread( t );
  }
  
  //wait for all threads to finish
  threads.join_all();
  
  //get the sum of FreqHists from threads and do FFT
  vector<FreqHist*> freqHists;
  GetStdFreqHists( freqHists );
  
  //loop over all threads' histograms for this frequency and add to the sums
  for( unsigned int iThread = 0; iThread != nThreads; ++iThread )
  {
    //add the 2D intensity vs rf bucket plot
    rfIntensityHist->Add( rfIntensityThreadHists[iThread] );
    delete rfIntensityThreadHists[iThread];
    
    //add all frequency hist families
    vector<FreqHist*>& thisHistFamily = freqHistFamilies[iThread];
    for( unsigned int iHist = 0; iHist != freqHists.size(); ++iHist )
    {
      FreqHist *threadHist = thisHistFamily.at(iHist);
      freqHists[iHist]->m_hist->Add( threadHist->m_hist );
      
      //delete the thread histogram after we have added its contents
      //clear vectors to make sure we don't try to use deleted objects
      delete threadHist;
    }
    thisHistFamily.clear();
  }
  freqHistFamilies.clear();
  rfIntensityThreadHists.clear();
  
  //plot projection of intensity vs bucket
  //todo: bins do not always increase, ROOT doesn't like this
  if(false)
  {
    TDirectory *rfDir = fout.mkdir( "RFBuckets", "Intensity in this bucket" );
    rfDir->cd();
    for( unsigned int i = 0; i != c_BucketsPerTurn; ++i )
    {
      unsigned int bin = i + 1;
      TH1D *proj = rfIntensityHist->ProjectionY( Form( "RFBucket_%u", bin ), bin, bin );
      proj->SetDirectory( rfDir );
    }
    fout.cd();
  }
  
  if( doFFT )
  {
    
    for( vector<FreqHist*>::iterator iHist = freqHists.begin(); iHist != freqHists.end(); ++iHist )
    {
      FreqHist *freqHist = *iHist;
      cout << "Doing FFT on " << freqHist->m_hist->GetName() << endl;
      
      TH1 *hm =0;
      TVirtualFFT::SetTransform(0);
      hm = freqHist->m_hist->FFT(hm, "MAG");
      hm->SetName( Form( "FFT_%s", freqHist->m_hist->GetName() ) );
      hm->SetTitle( Form("Magnitude of FFT (%s)", freqHist->m_hist->GetTitle()) );
    }
  }
  
  
  fout.Write();
  fout.Close();
  
  //did we leak.  I don't know ROOT memory confuses me.
  //ROOT should delete those histograms
  //maybe we leak the TGraph.
  
  return true;
}


//----------------------------------------------

bool AnalyzeSpill2( const bool NOBEAM, const BeamDAQSpill* spill, const std::string& imgFile , const std::string& readFileName )
{
  cout << "BeamAnalysis::AnalyzeSpill2 - " << imgFile << endl;
  if (NOBEAM == true){
    cout << "   no-beam " << endl;
    //return(false);
  }
  
  
  cout << " reading the file from - " << readFileName << endl;
  cout << endl;
  
  TFile fin( readFileName.c_str(), "READ" );
  TH1F* hist_ext = (TH1F*)fin.Get("hTurn_vs_Intensisy");
  
  ////////////////////////////////////////////////////////////////////
  TH1F* histDeadTime_ext     = (TH1F*)fin.Get("hBusyPerTrigger");
  TH1F* histDeadTime_extZoom = (TH1F*)fin.Get("hBusyPerTriggerZoom");
  ////////////////////////////////////////////////////////////////////
  
  //create output dir if needed
  boost::filesystem::path filepath( imgFile );
  boost::filesystem::path dirpath = filepath.parent_path();
  DAQUtil::Get().PrepareDir( dirpath.string() );
  
  
  {
    gStyle->SetOptTitle(0);
    gROOT->ForceStyle();
    
    //crappy hack because root doesn't do DrawClone correctly
    TPaveText *textPave = NULL;
    TPaveText *framePave = NULL;
    TLine     *thresholdLine = NULL;
    
    //split canvas to make enough pads for histograms + label
    TCanvas *cFrames2 = DAQUtil::Get().GetOrCreateCanvas( c_beamFrames2CanvasName.c_str() );
    cFrames2->Clear();
    cFrames2->cd();
    cFrames2->Divide( 1, 4 );
    
    // -----------------------------
    //top pad is for text
    cFrames2->cd(1);
    
    
    //////////////////////////////////////////////////////////
    TVirtualPad *cvsT = cFrames2->cd(1); cvsT->Divide(4,1);
    
    TVirtualPad *cvsA = cFrames2->cd(2); cvsA->Divide(4,1);
    TVirtualPad *cvsB = cFrames2->cd(3); cvsB->Divide(4,1);
    TVirtualPad *cvsC = cFrames2->cd(4); cvsC->Divide(4,1);
    
    cvsT->cd(1);
    //////////////////////////////////////////////////////////
    
    if( 0 == framePave )
    {
      framePave = new TPaveText( .05, 0.05, 0.95, 0.95 );
      framePave->SetName("framePave");
    }
    framePave->Clear();
    
    TDatime dt ;
    framePave->AddText( dt.AsString() );
    
    if( NOBEAM )
      framePave->AddText( "Received No Beam!" );
    
    ////////////////////////////////////////
    framePave->AddText( Form("Run# %d"  , spill->GetRunNumber() )   );
    framePave->AddText( Form("Spill# %d", spill->GetSpillNumber() ) );
    framePave->Draw();
    //framePave->DrawCopy();
    
    //// dead time ////////////////////////
    cvsC->cd(4);
    
    histDeadTime_ext->GetXaxis()->SetLabelSize( .06 ); 
    histDeadTime_ext->GetYaxis()->SetLabelSize( .06 ); 
    
    //gStyle->SetOptStat("nemruo");
    //gStyle->SetTitle("");
    gStyle->SetOptStat("emruo");
    gStyle->SetStatFontSize(0.11);
    gStyle->SetStatW(0.4);
    gStyle->SetStatH(0.10);
    
    histDeadTime_ext     -> DrawCopy();
    
    cvsC->cd(3);
    
    histDeadTime_extZoom->GetXaxis()->SetLabelSize( .06 ); 
    histDeadTime_extZoom->GetYaxis()->SetLabelSize( .06 ); 
    gPad->SetLogy();
    
    histDeadTime_extZoom -> DrawCopy();
    //histDeadTime_extZoom -> DrawCopy();
    
    ///////////////////////////////////////
    
    
    
    cFrames2->Update();
    
    vector<TH2D> hists2;
    vector<TH1D> hists1;
    vector<TH1D> hists3;
    
    TH2D hist2_0( Form("frameHist2_0"), "RF00 vs Sum QIE 0",  100/2, 0, 2.0E3  ,  100/2, 0, 3.0E4 );  
    TH2D hist2_1( Form("frameHist2_1"), "RF00 vs Sum QIE 1",  100/2, 0, 2.0E3  ,  100/2, 0, 1.0E5 );  
    TH2D hist2_2( Form("frameHist2_2"), "RF00 vs Sum QIE 2",  100/2, 0, 2.0E3  ,  100/2, 0, 3.0E5 );  
    TH2D hist2_3( Form("frameHist2_3"), "RF00 vs Sum QIE 3",  100/2, 0, 2.0E3  ,  100/2, 0, 1.0E6 );  
    TH1D hist1_0( Form("frameHist1_0"), "Sum QIE 0"        ,  100, 0, 3.0E4 );  
    TH1D hist1_1( Form("frameHist1_1"), "Sum QIE 1"        ,  100, 0, 1.0E5 );  
    TH1D hist1_2( Form("frameHist1_2"), "Sum QIE 2"        ,  100, 0, 3.0E5 );  
    TH1D hist1_3( Form("frameHist1_3"), "Sum QIE 3"        ,  100, 0, 1.0E6 );  
    TH1D hist3_0( Form("frameHist3_0"), "RF00"             ,  100, 0, 10.0E3 );  
    TH1D hist3_1( Form("frameHist3_1"), "RF00"             ,  100, 0,  2.0E3 );  
    
    hist2_0.GetXaxis()->SetNdivisions(505); hist2_0.GetXaxis()->SetTitle( "RF00"      ); 
    hist2_1.GetXaxis()->SetNdivisions(505); hist2_1.GetXaxis()->SetTitle( "RF00"      ); 
    hist2_2.GetXaxis()->SetNdivisions(505); hist2_2.GetXaxis()->SetTitle( "RF00"      ); 
    hist2_3.GetXaxis()->SetNdivisions(505); hist2_3.GetXaxis()->SetTitle( "RF00"      ); 
    hist1_0.GetXaxis()->SetNdivisions(505); hist1_0.GetXaxis()->SetTitle( "Sum QIE_0" ); 
    hist1_1.GetXaxis()->SetNdivisions(505); hist1_1.GetXaxis()->SetTitle( "Sum QIE_1" ); 
    hist1_2.GetXaxis()->SetNdivisions(505); hist1_2.GetXaxis()->SetTitle( "Sum QIE_2" ); 
    hist1_3.GetXaxis()->SetNdivisions(505); hist1_3.GetXaxis()->SetTitle( "Sum QIE_3" ); 
    hist3_0.GetXaxis()->SetNdivisions(505); hist3_0.GetXaxis()->SetTitle( "RF00"      ); 
    hist3_1.GetXaxis()->SetNdivisions(505); hist3_1.GetXaxis()->SetTitle( "RF00"      ); 
    
    hist2_0.GetYaxis()->SetTitle( "Sum QIE_0" ); 
    hist2_1.GetYaxis()->SetTitle( "Sum QIE_1" ); 
    hist2_2.GetYaxis()->SetTitle( "Sum QIE_2" ); 
    hist2_3.GetYaxis()->SetTitle( "Sum QIE_3" ); 
    hist1_0.GetYaxis()->SetTitle( "Events"    ); 
    hist1_1.GetYaxis()->SetTitle( "Events"    ); 
    hist1_2.GetYaxis()->SetTitle( "Events"    ); 
    hist1_3.GetYaxis()->SetTitle( "Events"    ); 
    hist3_0.GetYaxis()->SetTitle( "Events"    ); 
    hist3_1.GetYaxis()->SetTitle( "Events"    ); 
    
    hist2_0.GetXaxis()->SetTitleSize( .07 ); 
    hist2_1.GetXaxis()->SetTitleSize( .07 ); 
    hist2_2.GetXaxis()->SetTitleSize( .07 ); 
    hist2_3.GetXaxis()->SetTitleSize( .07 ); 
    hist1_0.GetXaxis()->SetTitleSize( .07 ); 
    hist1_1.GetXaxis()->SetTitleSize( .07 ); 
    hist1_2.GetXaxis()->SetTitleSize( .07 ); 
    hist1_3.GetXaxis()->SetTitleSize( .07 ); 
    hist3_0.GetXaxis()->SetTitleSize( .07 ); 
    hist3_1.GetXaxis()->SetTitleSize( .07 ); 
    
    hist2_0.GetXaxis()->SetTitleOffset(0.8); 
    hist2_1.GetXaxis()->SetTitleOffset(0.8); 
    hist2_2.GetXaxis()->SetTitleOffset(0.8); 
    hist2_3.GetXaxis()->SetTitleOffset(0.8); 
    hist1_0.GetXaxis()->SetTitleOffset(0.8); 
    hist1_1.GetXaxis()->SetTitleOffset(0.8); 
    hist1_2.GetXaxis()->SetTitleOffset(0.8); 
    hist1_3.GetXaxis()->SetTitleOffset(0.8); 
    hist3_0.GetXaxis()->SetTitleOffset(0.8); 
    hist3_1.GetXaxis()->SetTitleOffset(0.8); 
    
    hist2_0.GetYaxis()->SetTitleSize( .07 ); 
    hist2_1.GetYaxis()->SetTitleSize( .07 ); 
    hist2_2.GetYaxis()->SetTitleSize( .07 ); 
    hist2_3.GetYaxis()->SetTitleSize( .07 ); 
    hist1_0.GetYaxis()->SetTitleSize( .07 ); 
    hist1_1.GetYaxis()->SetTitleSize( .07 ); 
    hist1_2.GetYaxis()->SetTitleSize( .07 ); 
    hist1_3.GetYaxis()->SetTitleSize( .07 ); 
    hist3_0.GetYaxis()->SetTitleSize( .07 ); 
    hist3_1.GetYaxis()->SetTitleSize( .07 ); 
    
    hist2_0.GetYaxis()->SetTitleOffset(1.1); 
    hist2_1.GetYaxis()->SetTitleOffset(1.1); 
    hist2_2.GetYaxis()->SetTitleOffset(1.1); 
    hist2_3.GetYaxis()->SetTitleOffset(1.1); 
    hist1_0.GetYaxis()->SetTitleOffset(1.1); 
    hist1_1.GetYaxis()->SetTitleOffset(1.1); 
    hist1_2.GetYaxis()->SetTitleOffset(1.1); 
    hist1_3.GetYaxis()->SetTitleOffset(1.1); 
    hist3_0.GetYaxis()->SetTitleOffset(1.1); 
    hist3_1.GetYaxis()->SetTitleOffset(1.1); 
    
    hist2_0.GetXaxis()->SetLabelSize( .06 ); 
    hist2_1.GetXaxis()->SetLabelSize( .06 ); 
    hist2_2.GetXaxis()->SetLabelSize( .06 ); 
    hist2_3.GetXaxis()->SetLabelSize( .06 ); 
    hist1_0.GetXaxis()->SetLabelSize( .06 ); 
    hist1_1.GetXaxis()->SetLabelSize( .06 ); 
    hist1_2.GetXaxis()->SetLabelSize( .06 ); 
    hist1_3.GetXaxis()->SetLabelSize( .06 ); 
    hist3_0.GetXaxis()->SetLabelSize( .06 ); 
    hist3_1.GetXaxis()->SetLabelSize( .06 ); 
    
    hist2_0.GetYaxis()->SetLabelSize( .06 ); 
    hist2_1.GetYaxis()->SetLabelSize( .06 ); 
    hist2_2.GetYaxis()->SetLabelSize( .06 ); 
    hist2_3.GetYaxis()->SetLabelSize( .06 ); 
    hist1_0.GetYaxis()->SetLabelSize( .06 ); 
    hist1_1.GetYaxis()->SetLabelSize( .06 ); 
    hist1_2.GetYaxis()->SetLabelSize( .06 ); 
    hist1_3.GetYaxis()->SetLabelSize( .06 ); 
    hist3_0.GetYaxis()->SetLabelSize( .06 ); 
    hist3_1.GetYaxis()->SetLabelSize( .06 ); 
    
    hists2.push_back( hist2_0 ); 
    hists2.push_back( hist2_1 ); 
    hists2.push_back( hist2_2 ); 
    hists2.push_back( hist2_3 ); 
    hists1.push_back( hist1_0 ); 
    hists1.push_back( hist1_1 ); 
    hists1.push_back( hist1_2 ); 
    hists1.push_back( hist1_3 ); 
    hists3.push_back( hist3_0 ); 
    hists3.push_back( hist3_1 ); 
    
   // cout << " test-- 1 " << endl;
    
    for( TriggerDataVec::const_iterator iTrigger = spill->m_triggerDataVec->begin(); iTrigger != spill->m_triggerDataVec->end(); ++iTrigger ){
      
     // cout << " test-- 2 " << endl;
      
      //only use complete triggetrs
      if( (*iTrigger)->rfIntensity.size() < /*33*/25 ){
        cout << "Trigger only has " << (*iTrigger)->rfIntensity.size() << " rf recorded. skip..." << endl;
        continue;
      }
      
     // cout << " test-- 3 " << endl;
      
      Double_t sumQIE1 = (*iTrigger)->sumQIE1;
      Double_t sumQIE2 = (*iTrigger)->sumQIE2;
      Double_t sumQIE3 = (*iTrigger)->sumQIE3;
      Double_t sumQIE4 = (*iTrigger)->sumQIE4;
      Double_t RF00    = (*iTrigger)->RF00   ;
      
      //cout << "RF00  sum1234 " << RF00 << "  " << sumQIE1 << "  " << sumQIE2 << "  " << sumQIE3 << "  " << sumQIE4 << endl;
      
      hist2_0.Fill( RF00 , sumQIE1 ); 
      hist2_1.Fill( RF00 , sumQIE2 ); 
      hist2_2.Fill( RF00 , sumQIE3 ); 
      hist2_3.Fill( RF00 , sumQIE4 ); 
      
      hist1_0.Fill( sumQIE1 );
      hist1_1.Fill( sumQIE2 );
      hist1_2.Fill( sumQIE3 );
      hist1_3.Fill( sumQIE4 );
      
      hist3_0.Fill( RF00    );
      hist3_1.Fill( RF00    );
    }
    
    cvsA->cd(0+1); hist2_0.DrawCopy("colz"); //hist2_0.DrawCopy("colz");
    cvsA->cd(1+1); hist2_1.DrawCopy("colz"); //hist2_1.DrawCopy("colz");
    cvsA->cd(2+1); hist2_2.DrawCopy("colz"); //hist2_2.DrawCopy("colz");
    cvsA->cd(3+1); hist2_3.DrawCopy("colz"); //hist2_3.DrawCopy("colz");
    
    cvsB->cd(0+1); /*gPad->SetLogy();*/ hist1_0.DrawCopy();
    cvsB->cd(1+1); /*gPad->SetLogy();*/ hist1_1.DrawCopy();
    cvsB->cd(2+1); /*gPad->SetLogy();*/ hist1_2.DrawCopy();
    cvsB->cd(3+1); /*gPad->SetLogy();*/ hist1_3.DrawCopy();
    
    cvsC->cd(0+1); /*gPad->SetLogy();*/ hist3_0.DrawCopy();
    cvsC->cd(0+2); /*gPad->SetLogy();*/ hist3_1.DrawCopy();
    
    
    hist_ext->GetXaxis()->SetLabelSize( .06 ); 
    hist_ext->GetYaxis()->SetLabelSize( .06 ); 
    
    //// TurnID vs Intensisy/Turn ////
    cvsT->cd(1+1); hist_ext->GetXaxis()->SetRange(     0, 1850-4); hist_ext->DrawCopy(); // full range
    cvsT->cd(2+1); hist_ext->GetXaxis()->SetRange(     0,     60); hist_ext->DrawCopy(); // zoom up of beginning part
    cvsT->cd(3+1); hist_ext->GetXaxis()->SetRange(1790-4, 1850-4); hist_ext->DrawCopy(); // zoom up of ending part
    ///////////////////////////////////
    
    
    
    cFrames2->Update();
    
    cout << "-------------------------------------" << endl;
    cFrames2->SaveAs( imgFile.c_str() );
    system( Form("chmod 777 %s", imgFile.c_str()) );
    cout << "-------------------------------------" << endl;
    
  }
  
  
  return true;
}


void FillFreqHists( vector<FreqHist*> threadFreqHists, TH2F *rfBucketIntensity, const BeamDAQSpill *spill, unsigned int firstTurn, unsigned int lastTurn )
{
  cout << dec << "FillFreqHists - Begin filling turns " << firstTurn << " to " << lastTurn << endl;

  QIETurnVec turns = *(spill->m_qieTurnVec);
  for( unsigned int iTurn = firstTurn; iTurn != lastTurn; ++iTurn )
  {
    for( unsigned int iBucket = 0; iBucket != c_BucketsPerTurn; ++iBucket )
    {
      const unsigned int bucketIdx = iTurn*c_BucketsPerTurn + iBucket;
      const unsigned char rfQIE = turns[iTurn]->rfIntensity[iBucket];
      const unsigned int  rfIntensity = QIE_Conversion_Map[rfQIE];
      
      //save time by adding intensity by hand instead of using fill
      {
        const int globalBin = rfBucketIntensity->GetBin( iBucket+1, (unsigned int)rfQIE + 1);
        const float oldRFVal = rfBucketIntensity->GetBinContent( globalBin );
        rfBucketIntensity->SetBinContent( globalBin, oldRFVal + 1. );
      }
      
      for( vector<FreqHist*>::iterator iHist = threadFreqHists.begin(); iHist != threadFreqHists.end(); ++iHist )
        (*iHist)->AddBucket( bucketIdx, rfIntensity );
    }
  }
  
  cout << "FillFreqHists - Done filling turns " << firstTurn << " to " << lastTurn << endl;
}
