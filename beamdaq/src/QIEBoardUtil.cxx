#include "BeamDAQ/QIEBoardUtil.h"

#include "BeamDAQ/BeamDAQGlobals.h"

#include "BeamDAQ/BeamDAQSpill.h"
#include "BeamDAQ/QIEDataTypes.h"
#include "BeamDAQ/DAQUtil.h"

#include <sys/types.h>   //for tcp socket
#include <sys/socket.h>  //for tcp socket
#include <netinet/in.h>  //for tcp socket
#include <arpa/inet.h>   //for tcp socket 
#include <fcntl.h>       //for tcp socket

#include <boost/lexical_cast.hpp> //to convert thread_id to string
#include <boost/algorithm/string.hpp>    //for string trim
#include <boost/thread.hpp>

using namespace std;

namespace
{
  /// Set this to false if you do not want to communicate with QIE board
  ///  Do this if you are testing stuff while a normal BeamDAQ is running
  const bool CONNECT_TO_QIE_BOARD = true;

  /// Set this to true to print a lot about what's going on
  const bool verbose = false;

  /// Set this to true if you want to write debug info to a file for trigger block problems
  const bool doTriggerBlockDebugFile = false;
}

//===========================
// Constructor
// not thread safe, so it is private
// access to this class comes from using the singleton gettor Get()
//===========================
QIEBoardUtil::QIEBoardUtil() :
  m_sock1(0),
  m_sock2(0),
  m_sock3(0)
{ }



unsigned int ext_nTriggers = 0;



bool QIEBoardUtil::ConnectSock1(const int port, const std::string&  addr )
{
  if( verbose )
    cout << "QIEBoardUtil::ConnectSock1( port = " << port << ", address = " << addr << " )" << endl;

  bool isOK = false; //return value

  if( CONNECT_TO_QIE_BOARD )
  {
    m_sock1 = socket(AF_INET, SOCK_STREAM,0);
    if( m_sock1 > 0 )
    {
      m_server1.sin_addr.s_addr = inet_addr( addr.c_str() );
      m_server1.sin_family = AF_INET;
      m_server1.sin_port = htons( port );
      if(connect(m_sock1, (struct sockaddr *)&m_server1, sizeof(m_server1))<0)
        cout << Form("QIEBoardUtil::ConnectSock1 - ERROR - Could not connect to QIE at %s:%d", addr.c_str(), port) << endl;
      else
        isOK = true;
    }
    else
      cout << "QIEBoardUtil::ConnectSock1 - ERROR - Could not create socket for connection 1" << endl;
  }
  else
    cout << "QIEBoardUtil::ConnectSock1 - Debug flag set to not connect to QIE Board ." << endl;

  if( verbose )
    cout << Form("QIEBoardUtil::ConnectSock1 - isOK = %d for socket %d to %s:%d", isOK, m_sock1, addr.c_str(), port ) << endl;

  return isOK;
}

bool QIEBoardUtil::ConnectSock2(const int port, const std::string&  addr )
{
  if( verbose )
    cout << "QIEBoardUtil::ConnectSock2( port = " << port << ", address = " << addr << " )" << endl;

  bool isOK = false; //return value

  if( CONNECT_TO_QIE_BOARD )
  {
    m_sock2 = socket(AF_INET, SOCK_STREAM,0);
    if( m_sock2 > 0 )
    {
      m_server2.sin_addr.s_addr = inet_addr( addr.c_str() );
      m_server2.sin_family = AF_INET;
      m_server2.sin_port = htons( port );
      if(connect(m_sock2, (struct sockaddr *)&m_server2, sizeof(m_server2))<0)
        cout << Form("QIEBoardUtil::ConnectSock2 - ERROR - Could not connect to QIE at %s:%d", addr.c_str(), port) << endl;
      else
        isOK = true;
    }
    else
      cout << "QIEBoardUtil::ConnectSock2 - ERROR - Could not create socket for connection 2" << endl;
  }
  else
    cout << "QIEBoardUtil::ConnectSock2 - Debug flag set to not connect to QIE Board ." << endl;

  if( verbose )
    cout << Form("QIEBoardUtil::ConnectSock2 - isOK = %d for socket %d to %s:%d", isOK, m_sock2, addr.c_str(), port ) << endl;

  return isOK;
}

bool QIEBoardUtil::ConnectSock3(const int port, const std::string&  addr )
{
  if( verbose )
    cout << "QIEBoardUtil::ConnectSock3( port = " << port << ", address = " << addr << " )" << endl;

  bool isOK = false; //return value

  if( CONNECT_TO_QIE_BOARD )
  {
    m_sock3 = socket(AF_INET, SOCK_STREAM,0);
    if( m_sock3 > 0 )
    {
      m_server3.sin_addr.s_addr = inet_addr( addr.c_str() );
      m_server3.sin_family = AF_INET;
      m_server3.sin_port = htons( port );
      if(connect(m_sock3, (struct sockaddr *)&m_server3, sizeof(m_server3))<0)
        cout << Form("QIEBoardUtil::ConnectSock3 - ERROR - Could not connect to QIE at %s:%d", addr.c_str(), port) << endl;
      else
        isOK = true;
    }
    else
      cout << "QIEBoardUtil::ConnectSock3 - ERROR - Could not create socket for connection 3" << endl;
  }
  else
    cout << "QIEBoardUtil::ConnectSock3 - Debug flag set to not connect to QIE Board ." << endl;

  if( verbose )
    cout << Form("QIEBoardUtil::ConnectSock3 - isOK = %d for socket %d to %s:%d", isOK, m_sock3, addr.c_str(), port ) << endl;

  return isOK;
}

bool QIEBoardUtil::CloseSock( int wiznet, int port )
{
  if( verbose )
    cout << "QIEBoardUtil::CloseSock(wiznet = " << wiznet << ", port = " << port << " )" << endl;

  //wiznet numbers 1-3
  if( wiznet < 1 || 3 < wiznet )
  {
    cout << "QIEBoardUtil::CloseSock - ERROR - Wiznet " << wiznet << " invalid.  Only use 1-3" << endl;
    return false;
  }

  //port can only be 5000-5002 (also accept 0-2)
  if( !( ( 0 <= port && port <= 2 ) || ( 5000 <= port && port <= 5002 ) ) )
  {
    cout << "QIEBoardUtil::CloseSock - ERROR - Port " << port << " invalid.  Only use 0-2 or 5000-5002" << endl;
    return false;
  }
  if( 10 < port )
    port -= 5000;

  //we will kill connection using wiznet 1, so make sure we don't kill self
  /* TODO: needs to be tested
     if( 1 == wiznet && m_server1.sin_port == port+5000 )
     {
     cout << "QIEBoardUtil::CloseSock - WARNING - You cannot close the socket connection that this instance of QIEBoardUtil connected to."
     return false
     ]
     */
  bool closeOK = SendMessage_Sock1( Form("Q%d %01d", wiznet, port) );

  return closeOK;
}


bool QIEBoardUtil::ConnectAllSockets()
{
  bool isOK = ConnectSock1();
  isOK = ConnectSock2() && isOK;
  isOK = ConnectSock3() && isOK;
  return isOK;
}

QIEBoardUtil& QIEBoardUtil::Get()
{
  static QIEBoardUtil singleton;
  return singleton;
}

bool QIEBoardUtil::Retrieve( BeamDAQSpill* spill )
{
  if(verbose)
    cout << "QIEBoardUtil::Retrieve" << endl;

  bool isOK = true;
  isOK = FillQIEHeader( spill ) && isOK;
  isOK = FillQIESettings( spill ) && isOK;
  isOK = FillInhibitBlock( spill ) && isOK;
  FillTriggerBlock( spill ); //note, ignore return value because it fails often but is not essential
  isOK = FillQIEBlock( spill ) && isOK;

  return isOK;
}


//========================
// QIE Header
//========================
bool QIEBoardUtil::FillQIEHeader( BeamDAQSpill* spill )
{
  if(verbose)
    cout << "QIEBoardUtil::FillQIEHeader" << endl;
  bool isOK = true;

  //there are c_QIEHeader_WordSize( currently 15 ) words in the header, each word is a short (2bytes)
  //use 40 short buffer to allow for extra room
  unsigned short bufferShort[40];

  //we want the header!
  bool sendOK  = SendMessage_Sock1( "RDBH" );
  if( sendOK )
  {
    const size_t nBytesExpected = 2*c_QIEHeader_WordSize;
    const size_t nRead = GetResponse_Sock1( bufferShort, nBytesExpected );
    if( nBytesExpected != nRead )
      cout << "QIEBoardUtil::FillQIEHeader - Warning - Expected " << nBytesExpected << " bytes but read " << nRead << endl;

    //must unpack the words - shorts must be byte swapped and some pieces of header are made of more than 1 word
    spill->m_qieHeader->totalWordCount = UnpackShort( bufferShort[0], bufferShort[1] );
    spill->m_qieHeader->year           = bufferShort[2]&0x00FF;  //upper byte
    spill->m_qieHeader->month          = (bufferShort[2]&0xFF00)>>8;   //lower byte
    spill->m_qieHeader->day            = bufferShort[3]&0x00FF;  //upper byte
    spill->m_qieHeader->hour           = (bufferShort[3]&0xFF00)>>8;   //lower byte
    spill->m_qieHeader->min            = bufferShort[4]&0x00FF;  //upper byte
    spill->m_qieHeader->sec            = (bufferShort[4]&0xFF00)>>8;   //lower byte
    union
    {
      unsigned int i;
      float f;
    } u;
    spill->m_qieHeader->spillCounter   = UnpackShort( bufferShort[5], bufferShort[6] );
    //theses sums are stored on the board as 32 bit floats
    u.i = UnpackShort( bufferShort[7], bufferShort[8] );
    spill->m_qieHeader->intensitySum = u.f;
    u.i = UnpackShort( bufferShort[9], bufferShort[10] );
    spill->m_qieHeader->inhibitSum   = u.f;
    u.i = UnpackShort( bufferShort[11], bufferShort[12] );
    spill->m_qieHeader->busySum      = u.f;
    spill->m_qieHeader->dutyFactor     = UnpackShort( bufferShort[13] );
    spill->m_qieHeader->spillStatus    = (bufferShort[14]&0xFF00)>>8; //only the last 4 bits of the upper word
    if( verbose )
    {
      cout << "Got QIE Header" << endl;
      cout << spill->m_qieHeader->ToString() << endl;
    }
  }
  else
  {
    isOK = false;
    cout << "QIEBoardUtil::FillQIEHeader - ERROR - Failed to send message requesting header." << endl;
  }

  //make sure there is nothing left in the buffer
  RemoveRemainingBytes_Sock1();

  return isOK;
}

//========================
// QIE Settings
//========================
bool QIEBoardUtil::FillQIESettings( BeamDAQSpill* spill )
{ 
  if(verbose)
    cout << "QIEBoardUtil::FillQIESettings" << endl;
  bool isOK = true;

  //settings are read from the QIE board using the RD <register> command
  //The register must be specified in hex
  ReadRegister( spill->m_qieSettings->thresholdDelay, 0x3 );
  ReadRegister( spill->m_qieSettings->dataSumDelay, 0x4 );
  ReadRegister( spill->m_qieSettings->minInhibitWidth, 0x5 );
  ReadRegister( spill->m_qieSettings->threshold, 0x6 );
  ReadRegister( spill->m_qieSettings->qieDelay, 0x7 );
  ReadRegister( spill->m_qieSettings->orbitDelay, 0x8 );
  ReadRegister( spill->m_qieSettings->markerDelay, 0xb );
  ReadRegister( spill->m_qieSettings->qieClockPhase, 0xc );
  ReadRegister( spill->m_qieSettings->triggerBlockLength, 0xd );
  
  
  //////////
  ReadRegister( spill->m_qieSettings->nTriggers, 0x24, 0x25 );
  //////////
  
  ReadRegister( spill->m_qieSettings->nInhibits, 0x26, 0x27 );
  ReadRegister( spill->m_qieSettings->triggerOrbit, 0x50, 0x51 );
  ReadRegister( spill->m_qieSettings->qieBoardSum, 0x56, 0x57, 0x58);
  ReadRegister( spill->m_qieSettings->inhibitBoardSum, 0x5a, 0x5b, 0x5c);
  ReadRegister( spill->m_qieSettings->triggerBoardSum , 0x5e, 0x5f, 0x60);
  ReadRegister( spill->m_qieSettings->qieBoardSumSq , 0x62, 0x63, 0x64);
  //these registers store the sumSq shifted right 6 bits, so unshift now
  spill->m_qieSettings->qieBoardSumSq = spill->m_qieSettings->qieBoardSumSq << 6;

  {
    //tell QIE board we intend to read active bunch list
    SendMessage_Sock1( "WR 65 0" );
    int activeBunchCount = 0;
    for( int i = 0; i < 42; ++i)
    {
      unsigned short activeInt = 0;
      ReadRegister( activeInt, 0x66 );
      //each int encodes 14 bools, unpack them now
      for( int j = 0; j != 14; ++j )
      {
        // shift j bits to the right, and then grab the last value by masking it with 1.  
        // Little endian, so 0xabcd where a is the first batch of RF buckets and d is the last.  
        // 14 bits, but use 13, since we're counting from 0.
        // A result of 1 means active.
        int activeBitInt = (activeInt>>(13-j))&1;
        bool isActive = (1 == activeBitInt);
        spill->m_qieSettings->activeBunchList[activeBunchCount] = isActive;
        ++activeBunchCount;
      }
    }
  }

  return isOK;
} 

//========================
// Inhibit Block
//========================
bool QIEBoardUtil::FillInhibitBlock( BeamDAQSpill* spill )
{ 
  if(verbose)
    cout << "QIEBoardUtil::FillInhibitBlock" << endl;
  bool isOK = true;

  //read inhibit upper then lower bytes to get number of inhibits
  unsigned int nInhibits = spill->m_qieSettings->nInhibits;
  cout << "QIEBoardUtil::FillInhibitBlock - There were " << nInhibits << " inhibits" << endl;


  //not totally sure what these 2 writes do but they prepare to read inhibits
  SendMessage_Sock1( "WR 36 e00" );
  SendMessage_Sock1( "WR 37 0" );
  //read the SDRam buffer memory
  SendMessage_Sock1( Form("RDBM 3e %x", c_InhibitData_WordSize * (unsigned int)nInhibits) );

  //get each inhibit
  unsigned short inhibitWords[c_InhibitData_WordSize];
  const size_t bytesPerInhibit = 2*c_InhibitData_WordSize;
  size_t inhibitsRetrieved = 0;
  size_t nBad = 0; //number of times we get an error
  //continue getting response until all inhibits are found
  while( inhibitsRetrieved < nInhibits )
  {
    //read out one inhibit of data
    const size_t nRecv = GetResponse_Sock1( inhibitWords, bytesPerInhibit );

    if( 5 < nBad )
    {
      cout << "QIEBoardUtil::FillInhibitBlock - ERROR - Had 5 bad InhibitData fetching attempts.  Give up." << endl;
      break;
    }

    //make sure the number of bytes is right
    if( bytesPerInhibit != nRecv )
    {
      cout << "QIEBoardUtil::FillInhibitBlock - ERROR - Got " << nRecv << " bytes for InhibitData but expected " << bytesPerInhibit << ".  Try for the next one..." << endl;
      ++nBad;
      continue;
    }

    //create the InhibitData from the words and add it to the spill
    InhibitDataPtr dat( new InhibitData(inhibitWords) );
    spill->m_inhibitDataVec->push_back( dat );
    ++inhibitsRetrieved;

  }//end while loop asking for all inhibits the board says exist

  //make sure there is nothing left in the buffer
  RemoveRemainingBytes_Sock1();

  return isOK;
} 

//========================
// Trigger Block
//========================
bool QIEBoardUtil::FillTriggerBlock( BeamDAQSpill* spill )
{ 
  if(verbose)
    cout << "QIEBoardUtil::FillTriggerBlock" << endl;
  bool isOK = true;
  
  
  
  //////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //read inhibit upper then lower bytes to get number of triggers
  unsigned int nTriggers = spill->m_qieSettings->nTriggers;
  cout << "QIEBoardUtil::FillTriggerBlock - There were " << nTriggers << " triggers" << endl;
  
  ext_nTriggers = nTriggers ; //  store ---> readout in BeamDAQSpill.cxx
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  //////////////////////////////////////////////////////////////////////////////////////////////
  
  
  
  //not totally sure what these 2 writes do but they prepare to read triggers
  SendMessage_Sock1( "WR 36 a00" );
  SendMessage_Sock1( "WR 37 0" );
  
  if( nTriggers > 0 )
  {
    // QIE settings store number of buckets surrounding the triggering bucket.
    // The +1 is for the triggering bucket
    size_t nBucketsRead   = spill->m_qieSettings->triggerBlockLength + 1;

    //Each triggers carries one word, each word is a short.
    // the +1 at the end is for the triggering bucket
    size_t wordsPerTrigger = c_TriggerHeader_WordSize + nBucketsRead + c_TriggerFooter_WordSize;
    size_t bytesPerTrigger = 2 * wordsPerTrigger;

    //read the SDRam buffer memory
    SendMessage_Sock1( Form("RDBM 3e %x", (unsigned int)( nTriggers*wordsPerTrigger + c_TriggerGarbage_WordSize ) ) );


    //store the output of raeding into triggerWords
    unsigned short triggerWords[wordsPerTrigger];
    for( size_t i = 0; i != wordsPerTrigger; ++i )
         triggerWords[i] = 0;

    //sometimes there is garbage information that preceeds the first trigger block
    //it is always the same size: c_TriggerGarbage_WordSize
    //sometimes it starts with words that look like a TriggerData but not always

    //seach for the first complete TriggerData by reading out 1 word at a time until we get a whole TriggerData
    const size_t nWordsToSearchForStart = wordsPerTrigger + c_TriggerGarbage_WordSize;
    bool foundFirstTrigger = false, foundFirstTriggerFirstWord = false;
    size_t wordsThisTrigger = 0, wordsUntilFirstTrigger = 0;

    for( size_t iWord = 0; iWord != nWordsToSearchForStart; ++iWord )
    {
      const size_t nRecv = GetResponse_Sock1( triggerWords + wordsThisTrigger, 2 );
      if( 2 != nRecv )
        cout << Form("QIEBoardUtil::FillTriggerBlock - Warning - Expected 2 bytes but got %zu when hunting for first TriggerData.", nRecv ) << endl;

      //if the word we just got is equal to the number of words per trigger, then we may have started the first real TriggerData
      if( wordsPerTrigger == triggerWords[wordsThisTrigger] )
      {
        if( iWord != 0 && iWord != c_TriggerGarbage_WordSize )
          cout << Form("QIEBoardUtil::FillTriggerBlock - Warning - Expected this to happen at wordIdx = 0 (start) or %d (end of garbage)", c_TriggerGarbage_WordSize ) << endl;
        //start filling buffer from the front again
        triggerWords[0] = triggerWords[wordsThisTrigger];
        foundFirstTriggerFirstWord = true;
        wordsThisTrigger = 0;//is incremented to 1 below
        wordsUntilFirstTrigger = 0;
      }

      ++wordsThisTrigger;
      ++wordsUntilFirstTrigger;

      //if we have enough to form a complete trigger, then do it and leave the loop
      if( wordsPerTrigger == wordsThisTrigger )
      {
        cout << Form("QIEBoardUtil::FillTriggerBlock - Found complete TriggerData after word %zu from the start of the block.", iWord ) << endl;

        if( ! foundFirstTriggerFirstWord )
          cout << Form("QIEBoardUtil::FillTriggerBlock - Warning - Never found a word that specified the length of TriggerData in words, which is %zu.  Data is messed up.", wordsPerTrigger ) << endl;

         //cout << "!!!!!!!!!!!!!!!!!!!!!!!" << endl;
         //cout << "!!!!!!!!!!!!!!!!!!!!!!!" << endl;
         //cout << "!!!!!!!!!!!!!!!!!!!!!!!" << endl;
         //cout << " nBucketsRead " << nBucketsRead << endl;
         //cout << "!!!!!!!!!!!!!!!!!!!!!!!" << endl;
         //cout << "!!!!!!!!!!!!!!!!!!!!!!!" << endl;
         //cout << "!!!!!!!!!!!!!!!!!!!!!!!" << endl;
         
        //create new object for trigger data and add to spill
        TriggerDataPtr dat( new TriggerData(triggerWords, nBucketsRead) );
        spill->m_triggerDataVec->push_back(dat);
        foundFirstTrigger = true;
        break;
      }
    }

    if(!foundFirstTrigger)
    {
      //print an error and write a debug file
      cout << "QIEBoardUtil::FillTriggerBlock - ERROR - Did not find the first TriggerData.  Something is wrong.  Can't get trigger block." << endl;

      if( doTriggerBlockDebugFile )
      {
        string debugFilename = Form( "triggerBlock_debug_run%06d_spill%06d.txt", spill->GetRunNumber(), spill->GetSpillNumber() );
        cout << "                               - Printing the first " << nWordsToSearchForStart << " words to file: " << debugFilename << endl;
        ofstream ofs;
        ofs.open( debugFilename.c_str(), ofstream::out );
        ofs << "wordIdx, word(hex), word(dec)" << endl;
        for( size_t i = 0; i != nWordsToSearchForStart; ++i )
          ofs << Form( "%zu, %x, %d", i, triggerWords[i], triggerWords[i] )<< endl;
        ofs.close();
      }

      //clear the buffer and skip the trigger block
      RemoveRemainingBytes_Sock1();
      return false;
    }

    size_t triggersReceived = 1; //assume we read enough bytes to get a trigger
    size_t nBad = 0;

    //now get a while TriggerData for each remaining
    while( triggersReceived < nTriggers )
    {
      const size_t nRecv = GetResponse_Sock1( triggerWords, bytesPerTrigger );

      if( 5 < nBad )
      {
        cout << "QIEBoardUtil::FillTriggerBlock - ERROR - Had 5 bad TriggerData fetching attempts.  Give up." << endl;
        break;
      } 


      if( nRecv != bytesPerTrigger )
      {
        cout << "QIEBoardUtil::FillTriggerBlock - ERROR - Got " << nRecv << " bytes for TriggerData but expected " << bytesPerTrigger << ".  Try for the next one..." << endl;
        ++nBad;
        continue;
      }

      //create new object for trigger data and add to spill
      TriggerDataPtr dat( new TriggerData(triggerWords, nBucketsRead) );
      spill->m_triggerDataVec->push_back(dat);

      //get ready to do it again
      ++triggersReceived;
    }//end while loop for reading number of inhibits that exist

    //make sure there is nothing left in the buffer
    //we expect there may be some because garbage isn't always present
    RemoveRemainingBytes_Sock1();


  } //end if there are triggers at all
  else
    cout << "QIEBoardUtil::FillTriggerBlock - Found no trigger data.  This is fine if there was no 'Event Wow!'" << endl;

  return isOK;
} 


//========================
// QIE Block
//========================
bool QIEBoardUtil::FillQIEBlock( BeamDAQSpill* spill )
{
  if(verbose)
    cout << "QIEBoardUtil::FillQIEBlock" << endl;
  bool isOK = true;

  unsigned int nWords1 = 0, nWords2 = 0, nWords3 = 0;
  isOK = ReadRegister( nWords1, 0x12, 0x13 );
  isOK = ReadRegister( nWords2, 0x14, 0x15 );
  isOK = ReadRegister( nWords3, 0x16, 0x17 );

  cout << " Nwords1 = " << nWords1 << ", NWords2 = " << nWords2 << ", NWords3 = " << nWords3 << endl;

  // if there is nothing to read, then just return now
  if( 0 == nWords1 )
  {
    cout << "QIEBoardUtil::FillQIEBlock - There are now QIEBlock words, so just return.  May be OK if there is no beam." << endl;
    return true;
  }

  //some qie blocks are split across ports
  //use this offset when fetching and decoding turns
  const size_t wordsUntilFirstTurn2 = nWords1 % c_Turn_WordSize;
  const size_t wordsUntilFirstTurn3 = (nWords1+nWords2) % c_Turn_WordSize;

  //todo: check that nTurns is really an int before conversion
  const unsigned int nTurns1 = nWords1 / c_Turn_WordSize;
  const unsigned int nTurns2 = nWords2 / c_Turn_WordSize;
  const unsigned int nTurns3 = nWords3 / c_Turn_WordSize;

  //are we getting the right amount of data?
  if( nTurns1 != c_TurnsPerSpill || nTurns2 != c_TurnsPerSpill || nTurns3 != c_TurnsPerSpill )
  {
    //cout << "QIEBoardUtil::FillQIEBlock - ERROR - Turns on each socket are %zu, %zu, %zu - they should all be %zu.  Cannot get QIE Block." << endl;
    //cout << "QIEBoardUtil::FillQIEBlock - ERROR - Turns on each socket are "<< nTurns1 << ", " << nTurns2 << ", " << nTurns3 << " - they should all be" << c_TurnsPerSpill << ".  Cannot get QIE Block." << endl;
        cout << "QIEBoardUtil::FillQIEBlock - ERROR - Turns on each socket are "<< nTurns1 << ", " << nTurns2 << ", " << nTurns3 << " - they should all be " << c_TurnsPerSpill << ".  Cannot get QIE Block." << endl;
    return false;
  }

  //prepare buffer to be read
  SendMessage_Sock1( "WR 36 0" );
  SendMessage_Sock1( "WR 37 0" );
  SendMessage_Sock1( Form("RDB 3f %02x", (unsigned int)nWords1 ) );

  //data comes from 3 ports
  //use a thread for each port

  QIETurnVecPtr turns1( new QIETurnVec );
  QIETurnVecPtr turns2( new QIETurnVec );
  QIETurnVecPtr turns3( new QIETurnVec );


  //reserve enough space now, rounding up
  turns1->reserve( nTurns1 );
  turns2->reserve( nTurns2 );
  turns3->reserve( nTurns3 );

  //create the threads
  //constructor for creating thread to call member function is:
  //    boost::thread( reference to function, class instance, args ... )
  //todo: joins should happen after all threads created
  boost::thread fill1( &QIEBoardUtil::FillQIEBlockSingle, this, boost::ref(turns1), m_sock1, nTurns1, 0 );
  boost::thread fill2( &QIEBoardUtil::FillQIEBlockSingle, this, boost::ref(turns2), m_sock2, nTurns2, wordsUntilFirstTurn2 );
  boost::thread fill3( &QIEBoardUtil::FillQIEBlockSingle, this, boost::ref(turns3), m_sock3, nTurns3, wordsUntilFirstTurn3 );

  fill1.join();
  fill2.join();
  fill3.join();

  cout << "QIEBoardUtil::FillTriggerBlock - Done getting words from all sockets." << endl;

  //turns may be split across network cards, somove broken turns to the previous vector

  //merge start of turns2 into end of turns1
  QIETurnVec::iterator startCopy2 = turns2->begin();
  if( 0 < wordsUntilFirstTurn2 )
  {
    QIETurnPtr last1  = turns1->back();
    QIETurnPtr first2 = turns2->front();

    //fill remaining buckets in last turn on previous vector
    size_t nMissing = std::max( (size_t)c_BucketsPerTurn,  (size_t)2*(c_Turn_WordSize-wordsUntilFirstTurn2) );
    size_t firstBucket1 = c_BucketsPerTurn - nMissing;
    for( size_t iBucket = 0; iBucket != nMissing; ++iBucket )
      last1->rfIntensity[iBucket + firstBucket1] = first2->rfIntensity[iBucket];

    //update capid and turn words if needed
    if( wordsUntilFirstTurn2 < 3 )
      last1->capID = first2->capID;
    if( wordsUntilFirstTurn2 < 2 )
      last1->turnWord2 = first2->turnWord2;

    //do not save this rump turn, by failing to copy it into the main QIETurnVec
    //don't bother removing it from the array since it causes expensive reshuffling
    //no need to delete that QIETurn, since it is a smart pointer
    ++startCopy2;
  }
   
  //merge start of turns3 into end of turns2
  QIETurnVec::iterator startCopy3 = turns3->begin();
  if( 0 < wordsUntilFirstTurn3 )
  {
    QIETurnPtr last2  = turns2->back();
    QIETurnPtr first3 = turns3->front();

    //fill remaining buckets in last turn on previous vector
    size_t nMissing = std::max( (size_t)c_BucketsPerTurn,  (size_t)2*(c_Turn_WordSize-wordsUntilFirstTurn3) );
    size_t firstBucket2 = c_BucketsPerTurn - nMissing;
    for( size_t iBucket = 0; iBucket != nMissing; ++iBucket )
      last2->rfIntensity[iBucket + firstBucket2] = first3->rfIntensity[iBucket];

    //update capid and turn words if needed
    if( wordsUntilFirstTurn3 < 3 )
      last2->capID = first3->capID;
    if( wordsUntilFirstTurn3 < 2 )
      last2->turnWord2 = first3->turnWord2;

    //do not save this rump turn, by failing to copy it into the main QIETurnVec
    //don't bother removing it from the array since it causes expensive reshuffling
    //no need to delete that QIETurn, since it is a smart pointer
    ++startCopy3;
  }


  //after data from all ports is fetched merge turns into a single vector
  //make enough space for all turns
  const size_t totalTurns = turns1->size() + turns2->size() + turns3->size();
  spill->m_qieTurnVec->clear();
  spill->m_qieTurnVec->reserve( totalTurns );

  //now add the turns from each port
  spill->m_qieTurnVec->insert( spill->m_qieTurnVec->end(), turns1->begin(), turns1->end());
  spill->m_qieTurnVec->insert( spill->m_qieTurnVec->end(), startCopy2, turns2->end());
  spill->m_qieTurnVec->insert( spill->m_qieTurnVec->end(), startCopy3, turns3->end());

  //and cleanup.  do not delete the QIETurn objects,since they are returned
  turns1->clear();
  delete turns1;
  turns2->clear();
  delete turns2;
  turns3->clear();
  delete turns3;

  return isOK;
}

bool QIEBoardUtil::FillQIEBlockSingle( QIETurnVecPtr turns, int sock, unsigned int nTurns, size_t wordsUntilFirstTurn )
{
  //note: wordsUntilFirstTurn is an offset.  So if it is 0, no words are skipped.  if it is 1, then 1 word from the first turn's data is missing.
  //
  cout << "QIEBoardUtil::FillQIEBlockSingle for socket " << sock << " in thread " << boost::this_thread::get_id() << " getting  " << nTurns << " turns\n" << endl;
  bool isOK = true;

  if( 0 < wordsUntilFirstTurn )
    cout << "BRIAN BRIAN BRIAN - Data on this port doesn't start with a fullturn.  There are " << wordsUntilFirstTurn << " until the first turn.  This should not happen." << endl;

  unsigned short bufferShort[c_Turn_WordSize];
  for( size_t i = 0; i != c_Turn_WordSize-wordsUntilFirstTurn; ++i )
    bufferShort[i] = 0;

  //first time only, fill in values with 0 if there won't be data there
  size_t bufferOffset = 0;
  size_t bytesExpected = 2 * c_Turn_WordSize;
  if( 0 < wordsUntilFirstTurn )
  {
    bufferOffset  = c_Turn_WordSize - wordsUntilFirstTurn;
    bytesExpected =  2 * wordsUntilFirstTurn;
  }

  size_t nBadCapID = 0;

  if(verbose)
    cout << "  words from first full turn = " << wordsUntilFirstTurn << endl;
  //we only need the exact number of bytes for the first recv
  size_t nBytesRecv = 0;
  while( (nBytesRecv = GetResponse(sock, bufferShort + bufferOffset, bytesExpected ) ) )
  {
    //let the QIETurn constructor do the decoding
    QIETurnPtr turn( new QIETurn( bufferShort ) );

    //check the capid
    //note that if a turn is broken across ports it won't have a capid (but this should never happen)
    if( ! turn->ValidCapID() )
      ++nBadCapID;

    turns->push_back(turn);

    if( 0 == turns->size() % 10000 )
      cout << Form("Sock %d retrieved %dk turns (%.0f%% complete).", sock, int(turns->size()/1000), 100.*turns->size()/nTurns ) << endl;;

    //if number of bytes received is less than expected, there are no more words
    if( nBytesRecv < bytesExpected )
    {
      cout << Form("  Eding fetching of turns for sock %d.  Expected %zu bytes but got %zu.", sock, bytesExpected, nBytesRecv) << endl;
      break;
    }

    //if we retrieved all the turns there are, then exit
    if( nTurns == turns->size() )
    {
      cout << Form("  Fetched all %d turns from sock %d, so exit.", nTurns, sock ) << endl;
      break;
    }

    //from now on we should try to get whole turns
    bufferOffset = 0;
    bytesExpected = 2*c_Turn_WordSize;
  }

  string threadStr = boost::lexical_cast<string>( boost::this_thread::get_id() );
  cout << Form( "Exiting QIEBoardUtil::FillQIEBlockSingle for socket %d in thread %s after adding %zu turns, %zu had bad capID", sock, threadStr.c_str(), turns->size(),  nBadCapID ) << endl;

  //make sure there is nothing left in the buffer
  //note: this operation isn't protected by mutex
  //      however the only place this function is calls is followed immediately by a join
  //      so there is no risk of a deadlock on a socket.
  //      Also the RemoveRemainingBytes should be non-blocking
  RemoveRemainingBytes( sock );

  return isOK;
}


//============================================
// InitializeRegistry
//============================================
bool QIEBoardUtil::InitializeRegistry( bool reloadFlash /*= false*/ )
{
  if(verbose)
    cout << "QIEBoardUtil::InitializeRegistry - reloadFlash = " << reloadFlash << endl;

  //innocent until proven guilty
  bool allOK = true;

  bool writeOK = true;

  //wipe the flash memory if desired
  if( reloadFlash )
  {
    writeOK = SendMessage_Sock1("FF");
    if(!writeOK)
    {
      allOK = false;
      cout << "QIEBoardUtil::InitializeRegistry - ERROR wiping flash memory." << endl;
    }

    int sleepTime = 10;
    cout << Form( "QIEBoardUtil::InitializeRegistry - Flash memory being reset.  Sleep %d seconds now.", sleepTime) << endl;
    sleep(sleepTime);//QIE board takes a few seconds to reset itself.  
  }

  //set the control bits that are settable
  writeOK = SendMessage_Sock1("WR 0 321");
  if(!writeOK)
  { 
    allOK = false;
    cout << "QIEBoardUtil::InitializeRegistry - ERROR writing to register 0 (Control and Status Register)" << endl;
  }

  //registers 9 and a set the number of turns to readout per spill
  writeOK = SendMessage_Sock1( "WR 9 5" ) && SendMessage_Sock1( "WR A A168" );
  if(!writeOK)
  {
    allOK = false;
    cout << "QIEBoardUtil::InitializeRegistry - ERROR writing to registers 9 and A (number of turns to readout)" << endl;
  }

  //register 2 tells QIE board how to find MI beam synch, we are the 75-event
  writeOK = SendMessage_Sock1( "WR 2 75" );
  if(!writeOK)
  {
    allOK = false;
    cout << "QIEBoardUtil::InitializeRegistry - ERROR writing to register 2 (MIBS signal)" << endl;
  }

  //register 8 tells QIE board how many orbits to skip after MIBS before counting
  writeOK = SendMessage_Sock1( "WR 8 100" );
  if(!writeOK)
  {
    allOK = false;
    cout << "QIEBoardUtil::InitializeRegistry - ERROR writing to register 8 (orbit/turn delay count)" << endl;
  }

  //set active bunch list
  ifstream bunchListFile;
  bunchListFile.open( c_ActiveBunchListFile.c_str() );
  //read the file line by line, storing each line in s
  string s;
  string desc, regStr, valStr;
  while( getline(bunchListFile, s) )
  {
    //trim surrounding whitespace (helps identify comment lines that may have leading whitespace)
    boost::algorithm::trim(s);

    //skip blank lines or comment lines
    if( s.empty() || '#' == s[0])
      continue;

    //read the pieces of data <description> <registry(hex)> <value(hex)>
    std::istringstream ss(s);
    ss >> desc >> regStr >> valStr;

    //write the value to that registry location
    bool writeOK = SendMessage_Sock1( Form( "WR %s %s", regStr.c_str(), valStr.c_str() ) );

    //if there was an error then we are not OK and notify stdout
    if( !writeOK )
    {
      cout << "QIEBoardUtil::InitializeRegistry - ERROR - Could not set registry using line: " << s << endl;
      allOK = false;
    }
  }

  return allOK;
}

bool QIEBoardUtil::ResetRegistryFromFile( std::string filename /* = c_QIESettingsFile */, bool verify /* = false */ )
{
  if( filename.empty() )
    filename = c_QIESettingsFile;


  
  if(verbose)
    cout << "QIEBoardUtil::ResetRegistryFromFile - file = " << filename << ", verify = " << verify << endl;

  //open the file and loop over lines
  string expandedFilename = DAQUtil::Get().ExpandEnvironmentals( filename );
  ifstream setFile( expandedFilename.c_str() );

  bool allOK = true;
  //only try reading the file if we could open it
  if( setFile )
  {
    //read the file line by line, storing each line in s
    string s;
    string desc, regStr, valStr;
    while( getline(setFile, s) )
    {
      //trim surrounding whitespace (helps identify comment lines that may have leading whitespace)
      boost::algorithm::trim(s);

      //skip blank lines or comment lines
      if( s.empty() || '#' == s[0])
        continue;

      //read the pieces of data <description> <registry(hex)> <value(hex)>
      std::istringstream ss(s);
      ss >> desc >> regStr >> valStr;
      //write the value to that registry location
      bool writeOK = SendMessage_Sock1( Form( "WR %s %s", regStr.c_str(), valStr.c_str() ) );
      
      //=-=-=-=-=-=----
      cout << Form( "---->   WR %s %s", regStr.c_str(), valStr.c_str() ) << endl;
      //=-=-=-=-=-=----
      
      //if there was an error then we are not OK and notify stdout
      if( !writeOK )
      {
        cout << "QIEBoardUtil::ResetRegistryFromFile - ERROR - Could not set registry using line: " << s << endl;
        allOK = false;
      }

      //if we want to verify that the registry setting has changed, read it and check
      if( verify )
      {
        int reg = 0;
        unsigned short intendedVal = 0; //in decimal

        //extract intended decimal values from hex
        unsigned int tmpInt = 0;
        sscanf( regStr.c_str(), "%x", &tmpInt );
        reg = static_cast<unsigned short>(tmpInt);

        sscanf( valStr.c_str(), "%x", &tmpInt );
        intendedVal = static_cast<unsigned short>(tmpInt);

        unsigned short actualVal = 0;
        bool readOK = ReadRegister( actualVal, reg);
        if( ! readOK )
        {
          cout << Form("QIEBoardUtil::ResetRegistryFromFile - ERROR - Unable to verify settings because I couldn't read from register %02x", reg ) << endl;
          allOK = false;
        }

        if( actualVal != intendedVal )
        {
          cout << Form("QIEBoardUtil::ResetRegistryFromFile - ERROR - Registry %02x not set to %x (dec=%d) correctly.  It reads %x (dec=%d).", reg, intendedVal, intendedVal, actualVal, actualVal) << endl;
          allOK = false;
        }
      }//end if(verify)
    }//end loop over lines in registry settings file
  }
  else
  {
    cout << "QIEBoardUtil::ResetRegistryFromFile - ERROR - Could not open QIE registry settings file : " << expandedFilename << endl;
    allOK = false;
  }

  return allOK;
}



//============================================
// Communication Utilities
//============================================
std::string QIEBoardUtil::ReadRegister( int reg )
{
  //todo: error handling
  if(verbose)
    cout << "QIEBoardUtil::ReadRegister( " << reg << " )" << endl;

  //send the request to read this register
  TString message = Form( "RD %x", reg );

  if(verbose)
    cout << "Read register message = " << message << endl;

  SendMessage_Sock1( message.Data() );

  //get the response and require exactly 4 bytes, then NULL terminate the string
  char buff[5] = { 0,0,0,0,0};
  GetResponse_Sock1( buff, 4 );
  buff[4] = '\0';

  RemoveRemainingBytes_Sock1();

  //put this char* into a more stable memory structure and pass it back
  string rval(buff);
  if(verbose)
    cout << Form("QIEBoardUtil::ReadRegister (0x%02x)- response was '%s'.", reg, rval.c_str() ) << endl;

  return rval;
}

bool QIEBoardUtil::ReadRegister( unsigned short& rval, int reg )
{
  if(verbose)
    cout << "QIEBoardUtil::ReadRegister( " << reg << ", unsigned short& rval )" << endl;
  rval = 0;

  string response = ReadRegister( reg );

  //convert to long and return by value
  unsigned int tmpVal = 0;
  sscanf( response.c_str(), "%x", &tmpVal );
  rval = static_cast<unsigned short>(tmpVal);
  return true;
}

bool QIEBoardUtil::ReadRegister( unsigned int& rval, int regHi, int regLow)
{
  rval = 0;

  unsigned short hi = 0, low = 0;
  bool isOK = ReadRegister( hi, regHi );
  isOK = ReadRegister( low, regLow ) && isOK;

  rval = static_cast<unsigned int>( static_cast<unsigned int>(hi)<<16 | static_cast<unsigned int>(low) );
  return isOK;
}

bool QIEBoardUtil::ReadRegister( ULong64_t& rval, int regHi, int regMid, int regLow)
{
  rval = 0;

  //these store the response of each
  unsigned short hi = 0, mid = 0, low = 0;
  bool isOK = ReadRegister( hi, regHi );
  isOK = ReadRegister( mid, regMid ) && isOK;
  isOK = ReadRegister( low, regLow ) && isOK;

  rval = static_cast<ULong64_t>( static_cast<Long64_t>(hi)<<32 | static_cast<Long64_t>(mid)<<16 | static_cast<Long64_t>(low) );
  return isOK;
}

bool QIEBoardUtil::SendMessage_Sock1( std::string message )
{
  if( verbose )
    cout << "QIEBoardUtil::SendMessage_Sock1: " << message << endl;
  if(!CONNECT_TO_QIE_BOARD)
  {
    if( verbose )
      cout << "QIEBoardUtil::SendMessage_Sock1 - sending nothing in nonconnect mdoe" << endl;
    return false;
  }

  //lock the socket1 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock1 );

  //all messages must end with return 
  if( "\r" != message.substr(message.size()-1) )
    message += "\r";

  int rval = send(m_sock1, message.c_str(), message.size(), 0);
  if( rval <= 0 )
    cout << "QIEBoardUtil::SendMessage_Sock1 - ERROR - Return value " << rval << " when sending message " << message << endl;

  //anything greater than 0 is OK
  return (rval > 0);
}

bool QIEBoardUtil::SendMessage_Sock2( std::string message )
{
  if( verbose )
    cout << "QIEBoardUtil::SendMessage_Sock2: " << message << endl;

  if(!CONNECT_TO_QIE_BOARD)
  {
    if( verbose )
      cout << "QIEBoardUtil::SendMessage_Sock2 - sending nothing in nonconnect mdoe" << endl;
    return false;
  } 

  //lock the socket2 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock2 );

  //all messages must end with return 
  if( "\r" != message.substr(message.size()-1) )
    message += "\r";

  int rval = send(m_sock2, message.c_str(), message.size(), 0);
  if( rval <= 0 )
    cout << "QIEBoardUtil::SendMessage_Sock2 - ERROR - Return value " << rval << " when sending message " << message << endl;

  //anything greater than 0 is OK
  return (rval > 0);
}

bool QIEBoardUtil::SendMessage_Sock3( std::string message )
{
  if( verbose )
    cout << "QIEBoardUtil::SendMessage_Sock3: " << message << endl;

  if(!CONNECT_TO_QIE_BOARD)
  {
    if( verbose )
      cout << "QIEBoardUtil::SendMessage_Sock3 - sending nothing in nonconnect mdoe" << endl;
    return false;
  }

  //lock the socket3 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock3 );

  //all messages must end with return 
  if( "\r" != message.substr(message.size()-1) )
    message += "\r";

  int rval = send(m_sock3, message.c_str(), message.size(), 0);
  if( rval <= 0 )
    cout << "QIEBoardUtil::SendMessage_Sock3 - ERROR - Return value " << rval << " when sending message " << message << endl;

  //anything greater than 0 is OK
  return (rval > 0);
}


size_t QIEBoardUtil::GetResponse_Sock1( void* buff, size_t nBytesToRead )
{
  //lock the socket1 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock1 );
  return GetResponse( m_sock1, buff, nBytesToRead );
}

size_t QIEBoardUtil::GetResponse_Sock2( void* buff, size_t nBytesToRead )
{
  //lock the socket2 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock2 );
  return GetResponse( m_sock2, buff, nBytesToRead );
}

size_t QIEBoardUtil::GetResponse_Sock3( void* buff, size_t nBytesToRead )
{
  //lock the socket3 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock3 );
  return GetResponse( m_sock3, buff, nBytesToRead );
}

size_t QIEBoardUtil::GetResponse( int sock, void* buff, size_t nBytesToRead )
{
  if( verbose )
    cout << "QIEBoardUtil::GetResponse - sock = " << sock << ", nBytes = " << nBytesToRead << endl;

  if(!CONNECT_TO_QIE_BOARD)
  {
    if( verbose )
      cout << "QIEBoardUtil::GetResponse - getting nothing in nonconnect mdoe" << endl;
    return false;
  }


  char *buffNow = (char *)buff;
  ssize_t bytesRead = 0;
  size_t totalRead = 0;

  //keep reading until socket gives no response
  while( ( bytesRead = recv(sock, buffNow, nBytesToRead-totalRead, 0) ) > 0 ) 
  {
    //increment where to write output by how many bytes were received
    buffNow += bytesRead;

    //increment total bytes read
    totalRead += bytesRead;

    if( verbose )
      cout << "   Got " << bytesRead << " bytes, totalRead = " << totalRead << ",  requested " << nBytesToRead << endl;

    //break the loop if user supplied buffer is exhaused
    if( totalRead >= nBytesToRead )
      break;
  }

  if( bytesRead < 0 )
    cout << "QIEBoardUtil::GetResponse - ERROR - Socket " << sock << " returned error from recv." << endl;

  if( nBytesToRead != totalRead )
    cout << "QIEBoardUtil::GetResponse - ERROR - Socket " << sock << " read " << totalRead << ", was asked to read " << nBytesToRead << endl;

  if( verbose )
    cout << "QIEBoardUtil::GetResponse - Read " << totalRead << ", buff = " << buff << endl;

  return totalRead;
}


size_t QIEBoardUtil::RemoveRemainingBytes_Sock1()
{ 
  //lock the socket1 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock1 );
  return RemoveRemainingBytes( m_sock1 );
}

size_t QIEBoardUtil::RemoveRemainingBytes_Sock2()
{ 
  //lock the socket2 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock2 );
  return RemoveRemainingBytes( m_sock2 );
}

size_t QIEBoardUtil::RemoveRemainingBytes_Sock3()
{ 
  //lock the socket3 mutex to avoid multiple writes
  boost::mutex::scoped_lock scoped_lock( m_mutexSock3 );
  return RemoveRemainingBytes( m_sock3 );
}


size_t QIEBoardUtil::RemoveRemainingBytes( int sock )
{
  if(verbose)
    cout << Form("QIEBoardUtil::RemoveRemainingBytes( sock = %d )", sock) << endl;

  size_t nBytes = 0;

  //this function is tough.  see here for some info: http://stackoverflow.com/questions/6715736/using-select-for-non-blocking-sockets

  //set socket to non blocking
  int opts = fcntl(sock, F_GETFL);
  if( opts < 0 ) 
  {
    cout << "QIEBoardUtil::RemoveRemainingBytes - ERROR - Could not get socket opts" << endl;
    return 0;
  }

  opts = (opts | O_NONBLOCK);
  if( fcntl(sock, F_SETFL, opts) < 0 )
  {
    cout << "QIEBoardUtil::RemoveRemainingBytes - ERROR - Could set socket to non-blocking" << endl;
    return 0;
  }

  //loop until there is nothing to be read
  char tmpBuff[256];
  size_t readsDone = 0, maxReads = 1024;
  bool useMaxReads = false;
  fd_set read_flags;    // the flag sets to be used
  FD_ZERO(&read_flags); // initialize flags to 0
  FD_SET(sock, &read_flags); //add socket to the flags to check for reading
  struct timeval waitd = {0, 10}; // the max wait time for an event ( 1st number is s, 2nd is ns )
  while(1)
  {
    //check for file descriptors ready to read
    //the first arg is the max socket number to not check
    const int sel = select( sock + 1, &read_flags, (fd_set*)0, (fd_set*)0, &waitd);
    if(verbose)
      cout << "QIEBoardUtil::RemoveRemainingBytes - select() return value was " << sel << ", expect 3 I think." << endl;
    if( -1 == sel )
    {
      cout << "QIEBoardUtil::RemoveRemainingBytes - select() end in error." << endl;
      return nBytes;
    }

    //this is true if there is stuff to read in this socket
    if( FD_ISSET(sock, &read_flags) )
    {
      size_t bytesHere = recv( sock, tmpBuff, 2, 0 );
      if(verbose)
      {
        cout << "QIEBoardUtil::RemoveRemainingBytes - removed " << bytesHere << " bytes" << endl;
        if( 2 == bytesHere && string("da") == Form("%x%x", tmpBuff[0], tmpBuff[1]) )
          cout << "QIEBoardUtil::RemoveRemainingBytes - found a newline pair." << endl;
      }
      if( bytesHere <= 0 )
        break;
      nBytes += bytesHere;
    }
    else
    {
      if(verbose)
        cout << "QIEBoardUtil::RemoveRemainingBytes - socket not ready for reading anymore.  break and return." << endl;
      break;
    }
    ++readsDone;
    if( useMaxReads && maxReads < readsDone )
    {
      cout << "QIEBoardUtil::RemoveRemainingBytes - Warning - max reads reached. break and return." << endl;
      break;
    }
  }


  //set socket back to blocking
  opts = opts & (~O_NONBLOCK);
  if( fcntl(sock, F_SETFL, opts) < 0 ) 
    cout << "QIEBoardUtil::RemoveRemainingBytes - Warning - Could not set socket back to blocking.  This may not be a problem if the network is speedy." << endl;

  if(verbose)
    cout << "QIEBoardUtil::RemoveRemainingBytes - " << nBytes << " were removed using " << readsDone << " recv commands." << endl;

  return nBytes;
}

unsigned short QIEBoardUtil::UnpackShort( unsigned short a )
{
  //swap upper and lower bytes.
  return  ((a&0x00FF)<<8) |  ((a&0xFF00)>>8);
}

unsigned int QIEBoardUtil::UnpackShort( unsigned short upper, unsigned short lower )
{
  //swap upper and lower bytes in each short.  move upper short up 2 byes.
  return static_cast<unsigned int>( ( static_cast<unsigned int>(upper&0x00FF)<<24) |  (static_cast<unsigned int>(upper&0xFF00)<<8) | ((lower&0x00FF)<<8) | ((lower&0xFF00)>>8) );
}

