#ifndef BeamDAQGlobals_H
#define BeamDAQGlobals_H

//This file is intended to be included in all source files

//very common header files
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cassert>

//doesn't play nice with serialization #include <boost/shared_ptr.hpp>
#include <TString.h>   //I want Form available all over the place


//constants
//note: a word is 2 shorts (short = 2 bytes)
const unsigned short c_QIEHeader_WordSize      = 15; ///< number of words in the QIEHeader (hdr)
const unsigned short c_TriggerHeader_WordSize  = 6  /* */  + 8  /* */ ;  ///< number of header words for each TriggerData
const unsigned short c_TriggerFooter_WordSize  = 5;  ///< number of words that come after the intensity per bucket section in a TriggerData
const unsigned short c_TriggerGarbage_WordSize = 56; ///< the header to the trigger buffer.  this does not repeat for each trigger and we don't know how to decode it.
const unsigned short c_InhibitData_WordSize    = 10; ///< number of words in each InhibitData
const unsigned short c_Turn_WordSize = 297;          ///< 2 MI turn words, 1 capID, 294 rf bukcet pairs
const unsigned short c_BucketsPerTurn  = 588;        ///< rf buckets per turn
const unsigned int   c_TurnsPerSpill = 123000;       ///< Number of turns that should be read from each ethernet port (total turns in spill is this times 3)
const float          c_BucketsPerSpill = 2.07e8;     ///< A dcc estimate for number of buckets per spill (needs validation)
//2.07e8 ~ 369000 turns * 7 batches/turn * 82 buckets/batch   (there are 84 buckets/batch but 2 are empty between batches)
const float          c_G2SEM_PEDESTAL = 1.0E9;       ///< Anything less than this from g2sem indicates no beam
const float          c_MIFrequency = 53000000.;      ///< Frequency of main injector is 53MHz

const unsigned short c_CapID1 = 0xc6c6;
const unsigned short c_CapID2 = 0x6c6c;
const unsigned short c_CapID3 = 0xb1b1;
const unsigned short c_CapID4 = 0x1b1b;

//disk locations
const std::string c_testingDir     = "";
const std::string c_BeamDAQ_OUTDIR = c_testingDir + "/data2/data/beamDAQ";
//const std::string c_BeamDAQ_OUTDIR = c_testingDir + "/data2/e1039_data/slowcontrol_data/beamDAQ";
//const std::string c_SpillCounterFile = "/data2/data/slowcontrol/spillcounter/local_spillcount.dat";
const std::string c_SpillCounterFile = "/data2/e1039_data/slowcontrol_data/spillcounter/local_spillcount.dat";
//const std::string c_BeamDAQSpillFile = "/data2/data/slowcontrol/spillcounter/beamDAQ_lastspill.dat";
const std::string c_BeamDAQSpillFile = "/data2/e1039_data/slowcontrol_data/spillcounter/beamDAQ_lastspill.dat";
const std::string c_QIESettingsFile  = "/data2/data/beamDAQ/QIE_settings/QIE_register_reset.dat";
const std::string c_ActiveBunchListFile = "/data2/data/beamDAQ/QIE_settings/active_bunchlist.dat";

const std::string c_beamTextCanvasName = "cBeamText";
const std::string c_beamFramesCanvasName = "cBeamFrames";
const std::string c_beamFrames2CanvasName = "cBeamFrames2";// new frame
const std::string c_beamAnalysisCanvasName = "cBeamAnalysis";

//QIE conversion map from boards unsigned char to float
//kept in separate file because it is 256 lines long and may change from time to time
#include "QIE_Conversion_Map.cc"
#include "QIE_Conversion_Map_fC.cc"

#endif
