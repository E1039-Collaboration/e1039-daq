#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/QIEDataTypes.h"

/*
   This file defines the data types that hold information retrieved from the QIE board.
   The types are:
   - QIEHeader
   - QIESettings
   - IhibitData
   - TriggerData
   - QIETurn
   */

QIEHeader::QIEHeader() :
  totalWordCount(0),
  year(0),
  month(0),
  day(0),
  hour(0),
  min(0),
  sec(0),
  spillCounter(0),
  spillStatus(0),
  intensitySum(0.),
  inhibitSum(0.),
  busySum(0.),
  dutyFactor(0.)
{}

std::string QIEHeader::ToString() const
{
  std::string rval = "QIEHeader : { \n";
  rval += Form( " totalWordCount = %d, ", totalWordCount );
  rval += Form( " year = %d, ", year );
  rval += Form( " month = %d, ", month );
  rval += Form( " day = %d, ", day );
  rval += Form( " hour = %d, ", hour );
  rval += Form( " min = %d, ", min );
  rval += Form( " sec = %d, ", sec );
  rval += Form( " spillCounter = %d, ", spillCounter );
  rval += Form( " spillStatus = %d, ", spillStatus );
  rval += Form( " intensitySum = %E, ", intensitySum );
  rval += Form( " inhibitSum = %E, ", inhibitSum );
  rval += Form( " busySum = %E, ", busySum );
  rval += Form( " dutyFactor = %.2f%%, ", dutyFactor/10.);
  rval += "\n} : QIEHeader";
  return rval;
}

///OK if no spill status error bits are set
bool QIEHeader::IsOK() const
{
  return 0==spillStatus;
}

///this is bit 3 (2^3=8)
bool QIEHeader::ArithmeticOverflow() const
{
  return spillStatus&8;
}

///this is bit 2 (2^2=4)
bool QIEHeader::CapIDError() const
{
  return spillStatus&4;
}

///this is bit 1 (2^1=2)
bool QIEHeader::DataOutsideSpill() const
{
  return spillStatus&2;
}

///this is bit 0 (2^0=1)
bool QIEHeader::QIEBufferOverflow() const
{
  return spillStatus&1;
}

//==============================
// End QIEHeader
//==============================

//==============================
// Begin QIESettings
//==============================
QIESettings::QIESettings() :
  thresholdDelay(0),
  dataSumDelay(0),
  minInhibitWidth(0),
  threshold(0),
  qieDelay(0),
  orbitDelay(0),
  markerDelay(0),
  qieClockPhase(0),
  triggerBlockLength(0),
  nTriggers(0),
  nInhibits(0),
  triggerOrbit(0),
  qieBoardSum(0),
  inhibitBoardSum(0),
  triggerBoardSum(0),
  qieBoardSumSq(0),
  activeBunchList( c_BucketsPerTurn, false )
{ }

std::string QIESettings::ToString() const
{
  std::string rval = "QIESettings : { \n";
  rval += Form( " thresholdDelay = 0x%02x, ", thresholdDelay);
  rval += Form( " dataSumDelay = 0x%02x, ", dataSumDelay);
  rval += Form( " minInhibitWidth = 0x%02x, ", minInhibitWidth);
  rval += Form( " threshold = 0x%02x, ", threshold);
  rval += Form( " qieDelay = 0x%02x, ", qieDelay);
  rval += Form( " orbitDelay = 0x%02x, ", orbitDelay);
  rval += Form( " markerDelay = 0x%02x, ", markerDelay);
  rval += Form( " triggerBlockLength = 0x%02x, ", triggerBlockLength);
  rval += Form( " nTriggers = %d, ", nTriggers);
  rval += Form( " nInhibits = %d, ", nInhibits);
  rval += Form( " triggerOrbit = %d, ", triggerOrbit);
  rval += Form( " qieBoardSum = %llu, ", qieBoardSum );
  rval += Form( " inhibitBoardSum = %llu, ", inhibitBoardSum );
  rval += Form( " triggerBoardSum = %llu, ", triggerBoardSum );
  rval += Form( " qieBoardSumSq = %llu, ", qieBoardSumSq );
  rval += " Active Bunch = ";
  for( int i = 0; i != c_BucketsPerTurn; ++i )
    rval += Form("[%d]=%d|", i, (int)activeBunchList[i] );
  rval += "\n} : QIESettings";
  return rval;
}

//======================
// InhibitData
//======================
InhibitData::InhibitData():
  count(0),
  onsetTurn(0),
  onsetRF(0),
  inhibitSum(0),
  releaseTurn(0),
  releaseRF(0)
{}

InhibitData::InhibitData( unsigned short inhibitWords[c_InhibitData_WordSize] )
{
  //reach each word into object
  //QIE starts rf and turn counts at 1, so subtract one 
  count       = (inhibitWords[0]<<16) + inhibitWords[1];
  onsetTurn   = (inhibitWords[2]<<16) + inhibitWords[3] - 1;
  onsetRF     = inhibitWords[4] - 1;
  inhibitSum  = (inhibitWords[5]<<16) + inhibitWords[6];
  releaseTurn = (inhibitWords[7]<<16) + inhibitWords[8] - 1;
  releaseRF   = inhibitWords[9] - 1;
}

std::string InhibitData::ToString() const
{
  std::string rval = "InhibitData : { \n";
  rval += Form( " count = %d, ", count);
  rval += Form( " onsetTurn = %d, ", onsetTurn);
  rval += Form( " onsetRF = %d, ", onsetRF);
  rval += Form( " inhibitSum = %d, ", inhibitSum);
  rval += Form( " releaseTurn = %d, ", releaseTurn);
  rval += Form( " releaseRF = %d, ", releaseRF);
  rval += "\n} : InhibitData";
  return rval;
}

//===================================
// TriggerData
//===================================
TriggerData::TriggerData():
  
  nWords(0),
  triggerCount(0),
  
  onsetTurn(0),
  onsetRF(0),
  busyNotInhibit(0),
  releaseTurn(0),
  releaseRF(0),
  rfIntensity(0),
  
  // moved to here
  sumQIE1(0),
  sumQIE2(0),
  sumQIE3(0),
  sumQIE4(0),
  
  RF00(0)
  
{}

TriggerData::TriggerData( unsigned short* triggerWords, const size_t nBucketsRead )
{
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"<< std::endl;
  //std::cout << "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"<< std::endl;
  //std::cout << "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"<< std::endl;
  //std::cout << " nBucketsRead " << nBucketsRead << std::endl;     ;
  //std::cout << "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"<< std::endl;
  //std::cout << "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"<< std::endl;
  //std::cout << "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"<< std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  
  
  
  
  //qie board starts counting rf and turn at 1 and we want to start at 0, so subtract 1
  nWords       = triggerWords[0];
  //std::cout << "triggerWords[0]=" << triggerWords[0] << std::endl;
  /* --- orig ---
  triggerCount = (triggerWords[1]<<16) + triggerWords[2];
  onsetTurn    = (triggerWords[3]<<16) + triggerWords[4] - 1;
  onsetRF      = triggerWords[5] - 1;
  */
  
  // --- modified on 2016.12.07
  triggerCount = (triggerWords[9] <<16) + triggerWords[10]    ;
  onsetTurn    = (triggerWords[11]<<16) + triggerWords[12] - 1;
  onsetRF      =  triggerWords[13] - 1;
  
  // --- newly added on 2016.12.07
  sumQIE1      =  ( (triggerWords[1]<<16) + triggerWords[2] ) / 2.;
  sumQIE2      =  ( (triggerWords[3]<<16) + triggerWords[4] ) / 2.;
  sumQIE3      =  ( (triggerWords[5]<<16) + triggerWords[6] ) / 2.;
  sumQIE4      =  ( (triggerWords[7]<<16) + triggerWords[8] ) / 2.;
  
  RF00 = triggerWords[26];
  
  // triggerWords array
  // index
  //  0          ? nWords ? 
  //  1  presum
  //  2  presum
  //  3  presum
  //  4  presum
  //  5  presum
  //  6  presum
  //  7  presum
  //  8  presum
  //  9  trigger count
  // 10  trigger count
  // 11  main injector
  // 12  main injector
  // 13  not assigned + main injector
  // 14  RF-12
  // 15  RF-11
  // 16  RF-10
  // 17  RF-09
  // 18  RF-08
  // 19  RF-07
  // 20  RF-06
  // 21  RF-05
  // 22  RF-04
  // 23  RF-03
  // 24  RF-02
  // 25  RF-01
  // 26  RF+00 //<------
  // 27  RF+01
  // 28  RF+02
  // 29  RF+03
  // 30  RF+04
  // 31  RF+05
  // 32  RF+06
  // 33  RF+07
  // 34  RF+08
  // 35  RF+09
  // 36  RF+10
  // 37  RF+11
  // 38  RF+12
  
  
  
  //if there aren't enough words to at least cover the header and footer then completely bail the program
  const size_t nExpectedWords = nBucketsRead + c_TriggerHeader_WordSize + c_TriggerFooter_WordSize;
  if( nWords !=  nExpectedWords )
  {
    std::cout << "TriggerData::TriggerData - WARNING - TriggerData claims to have " << nWords << " but it should have " << nExpectedWords << ".  If this happens for multiple spills, then reset the QIE board." << std::endl;
  }

  //these words come after the rf intensity block, so the index is from the end
  busyNotInhibit = (triggerWords[nExpectedWords - 5]<<16) + triggerWords[nExpectedWords - 4]; 
  releaseTurn    = (triggerWords[nExpectedWords - 3]<<16) + triggerWords[nExpectedWords - 2] - 1;
  releaseRF      = triggerWords[nExpectedWords - 1] - 1;
  
  //each bucket gets a word between header and footer
  unsigned short *firstBucketPtr = triggerWords + c_TriggerHeader_WordSize;
  unsigned short *lastBucketPtr  = triggerWords + c_TriggerHeader_WordSize + nBucketsRead;
  rfIntensity.assign( firstBucketPtr, lastBucketPtr);
  
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //
  //std::cout << " ----------------------------------- "       << std::endl; 
  //
  //std::cout << nExpectedWords           << std::endl; //36 --> 44
  //
  //std::cout << nBucketsRead             << std::endl; //25
  //std::cout << c_TriggerHeader_WordSize << std::endl;// 6 --> 14
  //std::cout << c_TriggerFooter_WordSize << std::endl;// 5
  //
  //std::cout << " ------------------------------------ "       << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  //std::cout << std::endl;
  
  
}

std::string TriggerData::ToString() const
{
  std::string rval = "TriggerData : { \n";
  rval += Form( " nWords = %d, ", nWords);
  rval += Form( " triggerCount = %d, ", triggerCount);
  
  rval += Form( " sumQIE1 = %ld, ", sumQIE1);
  rval += Form( " sumQIE2 = %ld, ", sumQIE2);
  rval += Form( " sumQIE3 = %ld, ", sumQIE3);
  rval += Form( " sumQIE4 = %ld, ", sumQIE4);
  
  rval += Form( " RF00 = %ld, "   , RF00   );
  
  
  rval += Form( " onsetTurn = %d, ", onsetTurn);
  rval += Form( " onsetRF = %d, ", onsetRF);
  rval += Form( " busyNotInhibit = %d, ", busyNotInhibit);
  rval += Form( " releaseTurn = %d, ", releaseTurn);
  rval += Form( " releaseRF = %d, ", releaseRF);
  rval += " rfIntensity = ";
  const unsigned int nBuckets= rfIntensity.size();
  for( unsigned int i = 0; i != nBuckets; ++i )
    rval += Form("[%d]=%d|", i, rfIntensity[i] );
  rval += "\n} : TriggerData";
  return rval;
}

//===================================
// QIETurn
//===================================
QIETurn::QIETurn() :
  turnWord1(0),
  turnWord2(0),
  capID(0),
  intensitySum(0.),
  intensitySqSum(0.),
  maxIntensity(0.),  
  rfIntensity(c_BucketsPerTurn, 0)
{ }

QIETurn::QIETurn( unsigned short turnWords[c_Turn_WordSize] ) :
  turnWord1( turnWords[0] ),
  turnWord2( turnWords[1] ),
  capID(     turnWords[2] ),
  intensitySum(0.),
  intensitySqSum(0.),
  maxIntensity(0.),  
  rfIntensity(c_BucketsPerTurn, 0)
{
  //allocate a chunk for the data
  //each intensity is one byte (0-255)

  //remaining words are for bucket data
  //note that we used 3 turn words in initialization above
  size_t iBucket = 0;
  for( size_t iWord = 3; iWord != c_Turn_WordSize; ++iWord )
  {
    //first bucket is lower 8 bits - mask lower 8 bits
    {
      unsigned char tmpIntensity = (unsigned char)(turnWords[iWord]&0x00ff);
      rfIntensity[iBucket++] = tmpIntensity;
      const float tmpIntensityLin = (float)QIE_Conversion_Map[tmpIntensity];
      intensitySum   += tmpIntensityLin;
      intensitySqSum += tmpIntensityLin*tmpIntensityLin;
      maxIntensity = std::max( maxIntensity, tmpIntensityLin );
    }

    //second bucket is higher 8 bits - mask higher 8 bits and shift them to the right by 8 bits
    {
      unsigned char tmpIntensity = (unsigned char)( (turnWords[iWord]&0xff00) >> 8 );
      rfIntensity[iBucket++] = tmpIntensity;
      const float tmpIntensityLin = (float)QIE_Conversion_Map[tmpIntensity];
      intensitySum   += tmpIntensityLin;
      intensitySqSum += tmpIntensityLin*tmpIntensityLin;
      maxIntensity = std::max( maxIntensity, tmpIntensityLin );
    }
  }

}

unsigned int QIETurn::GetTurnID() const
{
  //the -1 is because we count from 0 and the board counts from 1
  const unsigned int boardTurnID = static_cast<unsigned int>( ( static_cast<unsigned int>(turnWord1&0x00FF)<<24) |  (static_cast<unsigned int>(turnWord1&0xFF00)<<8) | ((turnWord2&0x00FF)<<8) | ((turnWord2&0xFF00)>>8) );
  return boardTurnID - 1;
}

bool QIETurn::ValidCapID() const
{
  return ( capID==c_CapID1 || capID==c_CapID2 || capID==c_CapID3 || capID==c_CapID4 );
}

float QIETurn::GetDutyFactor() const
{
  return intensitySqSum < 1E-4 ? 0. : 100. * (intensitySum*intensitySum) / (c_BucketsPerTurn*intensitySqSum);
}

std::string QIETurn::ToString() const
{
  std::string rval = "QIETurn : { \n";
  rval += Form( " turnWord1 = %d, ", turnWord1);
  rval += Form( " turnWord2 = %d, ", turnWord2);
  rval += Form( " capID = %04x (isOK=%d), ", capID, ValidCapID());
  rval += Form( " turnID = %d, ", GetTurnID() );
  rval += Form( " DutyFactor = %.2f%%, ", GetDutyFactor() );
  rval += " rfIntensity = ";
  for( int i = 0; i != c_BucketsPerTurn; ++i )
    rval += Form("[%d]=%d|", i, rfIntensity[i] );
  rval += "\n} : QIETurn";
  return rval;
}
