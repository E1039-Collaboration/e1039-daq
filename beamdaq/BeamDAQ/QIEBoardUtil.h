#ifndef QIEBoardUtil_H
#define QIEBoardUtil_H

#include <boost/thread/mutex.hpp>
#include <sys/types.h>   //for tcp socket
#include <sys/socket.h>  //for tcp socket
#include <netinet/in.h>  //for tcp socket
#include <vector>

#include <boost/thread/mutex.hpp>

#include "BeamDAQForwards.h"
#include <Rtypes.h> //for Long64_t

/*! Singlelton class to help with command QIE Board tasks.

  Retrieves QIE data from board.
  */
class QIEBoardUtil
{
  public:
    ///Singleton gettor
    static QIEBoardUtil& Get();

    /*! Establish Connection to QIE Board through socket 1.
      @param[in] port1 Port on wiznet 1 to connect to
      @param[in] addr1 IP Address of wiznet 1
      @return true if connection was established.  false otherwise.
      */
    bool ConnectSock1( const int port1 = 5000, const std::string&  addr1 = "192.168.24.78" );
    /*! Establish Connection to QIE Board through socket 2.
      @param[in] port2 Port on wiznet 2 to connect to
      @param[in] addr2 IP Address of wiznet 2
      @return true if connection was established.  false otherwise.
      */
    bool ConnectSock2( const int port2 = 5000, const std::string&  addr2 = "192.168.24.79" );
    /*! Establish Connection to QIE Board through socket 3.
      @param[in] port3 Port on wiznet 3 to connect to
      @param[in] addr3 IP Address of wiznet 3
      @return true if connection was established done
      */
    bool ConnectSock3( const int port3 = 5000, const std::string&  addr3 = "192.168.24.80" );

    ///Connect with all 3 sockets using defaults
    bool ConnectAllSockets();

    /*! Close the connection to a socket using socket on wiznet 1
      @param[in] wiznet Wiznet that has the socket connection you want to close (must be 1-3)
      @param[in] port Port of the socket conection you want to close (must be 0-2 or 5000-5002)
      @return true if the message to close socket was sent correctly.
      */
    bool CloseSock( int wiznet, int port );

      ///Do all steps of data retrieval
      bool Retrieve( BeamDAQSpill* spill );

    /*! Read QIE Header information and store it in local memory
      @param[in,out] spill Object that stores the QIEHeader
      @return True if everything worked
      */
    bool FillQIEHeader( BeamDAQSpill* spill );

    /*! Read QIE Settings (registry information) and store it in local memory
      @param[in,out] spill Object that stores the QIESettings
      @return True if everything worked
      */
    bool FillQIESettings( BeamDAQSpill* spill );

    /*! Read InhibitBlock information and store it in local memory
      @param[in,out] spill Object that stores a vector of InhibitData from the InhibitBlock
      @return True if everything worked
      */
    bool FillInhibitBlock( BeamDAQSpill* spill );

    /*! Read TriggerBlock information and store it in local memory
      @param[in,out] spill Object that stores a vector of TriggerData from the TriggerBlock
      @return True if everything worked
      */
    bool FillTriggerBlock( BeamDAQSpill* spill );

    /*! Read QIEDataBlock information and store it in local memory
      @param[in,out] spill Object that stores a vector of QIETurn data from the QIEDataBlock
      @return True if everything worked
      */
    bool FillQIEBlock( BeamDAQSpill* spill );

    /*! Write essential registers for operation
      @param[in] reloadFlash Reload the FPGA flash memory
      @return True if everything worked
      */
    bool InitializeRegistry( bool reloadFlash = false );

    /*! Parse a QIE registry settings file and write to QIE board.

      The registry settings file format is "<human_name> <regitry_loc (hex)> <value (hex)>".
      Blank lines and lines starting with "#" are ignored
      @param[in] filename Filename of the registry settings file (may contain environmental variables).  Default of c_QIESettingsFile enforced in implementation of function.
      @param[in] verify If true, then read all the settings back after setting for verification
      @return True if everything worked
      */
    bool ResetRegistryFromFile(std::string filename = "", bool verify = false );


    //! Read QIE board register and return result as string
    std::string ReadRegister( int reg );
    //! Read QIE board register and interpret result as short
    bool ReadRegister( unsigned short& rval, int reg );
    //! Read 2 QIE board registers and interpret result as unsigned int
    bool ReadRegister( unsigned int& rval, int regHi, int regLow );
    //! Read 3 QIE board registers and interpret result as long long
    bool ReadRegister( ULong64_t& rval, int regHi, int regMid, int regLow );

    //! Send a message to socket 1, return true if all was OK
    bool SendMessage_Sock1( std::string message );
    //! Send a message to socket 2, return true if all was OK
    bool SendMessage_Sock2( std::string message );
    //! Send a message to socket 3, return true if all was OK
    bool SendMessage_Sock3( std::string message );

    //! Get response from socket 1, return number of bytes read
    size_t GetResponse_Sock1( void* buff, size_t nBytesToRead );
    //! Get response from socket 2, return number of bytes read
    size_t GetResponse_Sock2( void* buff, size_t nBytesToRead );
    //! Get response from socket 3, return number of bytes read
    size_t GetResponse_Sock3( void* buff, size_t nBytesToRead );

    //! Remove remaining socket 1 bytes, if any, and return number of bytes removed
    size_t RemoveRemainingBytes_Sock1();
    //! Remove remaining socket 2 bytes, if any, and return number of bytes removed
    size_t RemoveRemainingBytes_Sock2();
    //! Remove remaining socket 3 bytes, if any, and return number of bytes removed
    size_t RemoveRemainingBytes_Sock3();

  private:
    /*! Private Constructor to force access through singleton gettor Get().

      It must be private because initialization steps occur which are not thread safe.
      We could make initialization thread safe using c++11 stuff but most of our machines have an older
      version of gcc.  It is easier/better to enforce that the singleton gettor is used (which is almost nearly thread safe).
      */
    QIEBoardUtil();


    /*! Read the QIE data words from one ethernet card and form QIETurns
      @param[out] turns Push each new QIETurn onto this vector
      @param[in] sock Socket to read from
      @param[in] nTurns Number of turns of data to fetch
      @param[in] wordsUntilFirstTurn Number of words in a turn fragment that starts the data from this port
      @return True if everything worked
      */
    bool FillQIEBlockSingle( QIETurnVecPtr turns, int sock, unsigned int nTurns, size_t wordsUntilFirstTurn );

    /*! Get response from this socket.
      This is is private so that this class can control the sockets being used.
      The public interface is to specify which of the 3 known sockets to use.
      @param[in] sock Socket to read from
      @param[in,out] buff Address to store bytes that are read
      @param[in] nBytesToRead Number of bytes to request
      @return Number of bytes actually read
      */
    size_t GetResponse( int sock, void* buff, size_t nBytesToRead );

    //! Remove remaining bytes, if any, and return number of bytes removed
    size_t RemoveRemainingBytes( int sock);


    /*! Unpack a word by swapping bytes
      @param[in] a The word
      @return The word byte-swapped
      */
    unsigned short UnpackShort( unsigned short a );
    /*! Unpack two words by swapping bytes then combine them into a int using all 4 bytes
      @param[in] upper Word that holds the upper 2 byes (15..8)
      @param[in] lower Word that holds the lower 2 byes (7..0)
      @return Words byte-swapped
      */
    unsigned int UnpackShort( unsigned short upper, unsigned short lower );

    //---------------------------------------------//
    // Member Variables
    //---------------------------------------------//


    //todo: not sure if these are necessary or if the tcp system calls handle this already
    mutable boost::mutex m_mutexSock1; ///< Mutex prevents simultaneous read/writes to sock1
    mutable boost::mutex m_mutexSock2; ///< Mutex prevents simultaneous read/writes to sock2
    mutable boost::mutex m_mutexSock3; ///< Mutex prevents simultaneous read/writes to sock3

    //sockets
    int m_sock1;  ///< Socket connection to QIE board ethernet card 1
    int m_sock2;  ///< Socket connection to QIE board ethernet card 2
    int m_sock3;  ///< Socket connection to QIE board ethernet card 3
    struct sockaddr_in m_server1;  ///< Server address for socket1
    struct sockaddr_in m_server2;  ///< Server address for socket2
    struct sockaddr_in m_server3;  ///< Server address for socket3
};

#endif
