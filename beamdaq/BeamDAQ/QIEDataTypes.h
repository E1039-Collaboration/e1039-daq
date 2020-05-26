#ifndef QIEDataTypes_H
#define QIEDataTypes_H

#include "BeamDAQGlobals.h"
#include <Rtypes.h> //for Long64_t

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>


/*
   This file defines the data types that hold information retrieved from the QIE board.
   The types are:
   - QIEHeader
   - QIESettings
   - IhibitData
   - TriggerData
   - QIETurn
   */

struct QIEHeader
{
  friend class boost::serialization::access;

  unsigned int totalWordCount;
  unsigned short year;  ///< 2-digit year of spill
  unsigned short month; ///< month of spill (1-12)
  unsigned short day;   ///< day of month of spill
	unsigned short hour;  ///< hour of day of spill (0-23)
	unsigned short min;   ///< minute of hour of spill
	unsigned short sec;   ///< second of minute of spill
	unsigned int spillCounter;  ///< on-board spill counter
	unsigned short spillStatus; ///< Spill satus bitfied 
	float intensitySum; ///< Sum of linearized intensity in all buckets
	float inhibitSum;   ///< Sum of linearized intensity in all inhibited buckets
	float busySum;      ///< Sum of linearized intensity in buckets where DAQ is busy and triggers are not inhibited by beam
	unsigned short dutyFactor; ///< This short stores \f$\frac{<I^{2}>}{<I>^{2}}*1000\f$, see ToString for how to interpret

	///Default constructor
	QIEHeader();

	/// How to pack/unpack archive file
	template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & totalWordCount;
			ar & year;
			ar & month;
			ar & day;
			ar & hour;
			ar & min;
			ar & sec;
			ar & spillCounter;
			ar & spillStatus;
			ar & intensitySum;
			ar & inhibitSum;
			ar & busySum;
			ar & dutyFactor;
		}


	/// String description of this object
	std::string ToString() const;

	///OK if no spill status error bits are set
	bool IsOK() const;
	///this is bit 3 (2^3=8) of spillStatus
	bool ArithmeticOverflow() const;
	///this is bit 2 (2^2=4) of spillStatus
	bool CapIDError() const;
	///this is bit 1 (2^1=2) of spillSatus
	bool DataOutsideSpill() const;
	///this is bit 0 (2^0=1) of spillStatus
	bool QIEBufferOverflow() const;

};

struct QIESettings
{
	friend class boost::serialization::access;

	//for documentation on these, see https://seaquest-docdb.fnal.gov:440/cgi-bin/ShowDocument?docid=537
	unsigned short thresholdDelay; ///< Register 0x3
	unsigned short dataSumDelay;   ///< Register 0x4
	unsigned short minInhibitWidth; ///< Register 0x5
	unsigned short threshold;       ///< Register 0x6
	unsigned short qieDelay;        ///< Register 0x7
	unsigned short orbitDelay;      ///< Register 0x8
	unsigned short markerDelay;     ///< Register 0xb
	unsigned short qieClockPhase;   ///< Register 0xc
	unsigned short triggerBlockLength;     ///< Register 0xd
	unsigned int nTriggers;       ///< Registers 0x24-25
	unsigned int nInhibits;       ///< Registers 0x26-27
	unsigned int triggerOrbit;    ///< Registers 0x50-0x51
	ULong64_t qieBoardSum;        ///< Registers 0x56-58
	ULong64_t inhibitBoardSum;    ///< Registers 0x5a-5c
	ULong64_t triggerBoardSum;    ///< Registers 0x5e-60
	ULong64_t qieBoardSumSq;      ///< Registers 0x62-64
	std::vector<bool> activeBunchList;   ///< Registers 0x65-66 (this will be of constant size c_BucketsPerTurn)

	///Default Constructor
	QIESettings();

	/// How to pack/unpack archive file
	template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & thresholdDelay;
			ar & dataSumDelay;
			ar & minInhibitWidth;
			ar & threshold;
			ar & qieDelay;
			ar & orbitDelay;
			ar & markerDelay;
			ar & qieClockPhase;
			ar & triggerBlockLength;
      ar & nTriggers;
      ar & nInhibits;
			ar & triggerOrbit;
			ar & qieBoardSum;
			ar & inhibitBoardSum;
			ar & triggerBoardSum;
			ar & qieBoardSumSq;
			ar & activeBunchList;
		}


	/// String description of this object
	std::string ToString() const;
};

struct InhibitData
{
	friend class boost::serialization::access;

	unsigned int count;  //words 0-1
	unsigned int onsetTurn;   ///< turnID of first bucket inhibited (starts at 0)
	unsigned short onsetRF;   ///< RF bucket within turn of the first bucket inhibited (starts at 0)
	unsigned int inhibitSum;  ///< Linearized intenity sum of inhibited buckets
	unsigned int releaseTurn; ///< turnID of last bucket inhibited (starts at 0)
	unsigned short releaseRF; ///< RF bucket within turn of the last bucket inhibited (starts at 0)

	///Default Constructor
	InhibitData();

	///Construct using one InihibitData's worth of words
	explicit InhibitData( unsigned short inhibitWords[c_InhibitData_WordSize] );

	/// How to pack/unpack archive file
	template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & count;
			ar & onsetTurn;
			ar & onsetRF;
			ar & inhibitSum;
			ar & releaseTurn;
			ar & releaseRF;
		}

	/// String description of this object
	std::string ToString() const;
};

struct TriggerData
{
	friend class boost::serialization::access;

	unsigned short nWords; ///< Number of words in this TriggerData
	unsigned int triggerCount; ///< This is the iTh trigger so far (probably starts at 1?)


	unsigned int onsetTurn;    ///< turnID of first bucket included (starts at 0)
	unsigned short onsetRF;    ///< RF bucket within turn of the first bucket included (starts at 0)
	unsigned int busyNotInhibit; ///< Sum of linearized intensity of buckets which are busy and not inhibited (?)
	unsigned int releaseTurn; ///< turnID of last bucket included (starts at 0)
	unsigned short releaseRF; ///< RF bucket within turn of the last bucket included (starts at 0)

	//number of recorded bucket intensity surrounding triggering bucket 
	//  is a board setting that may change and is not too large
	//  therefore we use a variable sized array.
	std::vector<unsigned short> rfIntensity; ///<Intensity in buckets surrounding triggering bucket, indices run from 0..2N meaning buckets -N..active..N



   // added on 2016.12.07 
   unsigned long sumQIE1;
   unsigned long sumQIE2;
   unsigned long sumQIE3;
   unsigned long sumQIE4;

   unsigned long RF00;


	///Default Constructor
	TriggerData();

	///Constructor out of one TriggerData's worth of words, and you must tell it how many buckets it reads out
	TriggerData( unsigned short* triggerWords, const size_t nBucketsRead );

	/// How to pack/unpack archive file
	template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & nWords;
			ar & triggerCount;
			ar & onsetTurn;
			ar & onsetRF;
			ar & busyNotInhibit;
			ar & releaseTurn;
			ar & releaseRF;
			ar & rfIntensity;
      
      
      
      
      
      
      ar &  sumQIE1;
      ar &  sumQIE2;
      ar &  sumQIE3;
      ar &  sumQIE4;
      
      ar &  RF00;
      
      
		}


	/// String description of this object
	std::string ToString() const;
};


struct QIETurn
{
	friend class boost::serialization::access;

	unsigned short turnWord1; ///< Upper bits of the turnID
	unsigned short turnWord2; ///< Lower bits of the turnID
	unsigned short capID;     ///< capID
  float intensitySum;       ///< Manual sum of linearized intensity
  float intensitySqSum;     ///< Manual sum of linearized intensity squared
  float maxIntensity;     ///< Max linearized intensity seen in a bucket
	std::vector<unsigned char> rfIntensity; ///< intensity in each bucket

	///Default Constructor
	QIETurn();

	///Construct using one QIETurn's chunk of words
	explicit QIETurn( unsigned short turnWords[c_Turn_WordSize] );

	/// How to pack/unpack archive file
	template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{ 
			ar & turnWord1;
			ar & turnWord2;
			ar & capID;
      ar & intensitySum;
      ar & intensitySqSum;
      ar & maxIntensity;
			ar & rfIntensity;
		}


	///Combine upper and lower words into turnID
	unsigned int GetTurnID() const;

	///Check that the capID is valid
	bool ValidCapID() const;

	///Get duty factor for this turn
	float GetDutyFactor() const;

	/// String description of this object
	std::string ToString() const;
};

#endif
