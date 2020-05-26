#ifndef BEAMANALYSIS_H
#define BEAMANALYSIS_H
#include <string>
#include <vector>

class TH1F;
class TH2F;
class BeamDAQSpill;

//------------------
//helper classes
//------------------
struct FreqHist
{
  FreqHist( float f, float start, float stop, const std::string& name, bool cleanSelf );
  virtual ~FreqHist();

  void AddBucket( unsigned int idx, float intensity );
  float DutyFactor() const;

  float m_freq;
  float m_startTime;
  float m_stopTime;
  std::string m_name;
  bool m_cleanSelf;
  TH1F *m_hist;

};  //end of FreqHist


//----------------
// Functions
//----------------
bool AnalyzeSpill( double *mean_deadtime ,
                    const BeamDAQSpill* spill, const std::string& outfileName, bool doFFT = false 
                   );

bool AnalyzeSpill2( const bool NOBEAM, 
                   const BeamDAQSpill* spill, const std::string& imgFile,
                                              const std::string& readFileName );

void GetStdFreqHists( std::vector<FreqHist*>& freqHists, const std::string& suffix = "", bool cleanSelf = false );
void FillFreqHists( std::vector<FreqHist*> threadFreqHists, TH2F *rfBucketIntensity, const BeamDAQSpill *spill, unsigned int firstTurn, unsigned int lastTurn );

#endif
