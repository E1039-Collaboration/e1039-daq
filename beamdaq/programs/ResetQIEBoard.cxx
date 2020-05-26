#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/QIEBoardUtil.h"

#include <sys/wait.h>

using namespace std;

void usage(int argc, char *argv[] );

int main(int argc, char *argv[] )
{
  if( argc > 2 )
  {
    usage(argc, argv);
    return 1;
  }
  string file =  (argc<2) ? "" : argv[1];

  //look for a help string
  if( file == "-h" || file == "--help" )
  {
    usage( argc, argv);
    return 1;
  }

  bool verify = true;
  bool reloadFlash = true;

  cout << "This code will kill all BeamDAQ processes, reset the QIE board, and reload the QIE registry parameters." << endl;
  cout << "You should wait until after the End Of Spill has passed." << endl;
  cout << "Press the Enter key to continue (or hit crtl-c to cancel)...";
  getchar();

  cout << "Killing RunBeamDAQ..." << endl;
  system("pkill RunBeamDAQ");

  //sleep a bit
  const int sleepyTime = 2;
  cout << Form("Sleep for %ds...", sleepyTime) << endl;
  sleep( sleepyTime );


  //use a different port than usual for server 1, in case there is a problem on the normal one
  // connect only to socket 1 and use a non-default port
  cout << "Establishing connection to QIE Board..." << endl;
  bool isOK = false;
  int port = -1;
  vector<int> ports;
  ports.push_back(5001);
  ports.push_back(5002);
  ports.push_back(5000);
  for( size_t i = 0; i != ports.size(); ++i )
  {
    port = ports[i];
    cout <<  Form("... try port %d ...", port) << endl;
    isOK = QIEBoardUtil::Get().ConnectSock1( port );
    if( isOK )
    {
      cout << Form("...connected to port %d!", port) << endl;
      break;
    }
    else
      cout << Form("... connection to port %d failed!",port) << endl;
  }

  if( !isOK )
  {
    cout << "Could not connect to QIE board.  Quitting." << endl;
    return 1;
  }


  //=====================
  //Initialize registry
  //=====================
  isOK = QIEBoardUtil::Get().InitializeRegistry( reloadFlash );
  if( !isOK )
  {
    cout << "===> There were problems Intializing QIE Board.  Trying again...." << endl;
    isOK = QIEBoardUtil::Get().InitializeRegistry( );
    if( !isOK )
      cout << "===> There were problems Intializing QIE Board AGAIN.  Uh-oh?" << endl;
    else
      cout << "===> 2nd attempt to initialize QIE board worked." << endl;
  }


  //=====================
  //Load registry values from file
  //=====================
  isOK = QIEBoardUtil::Get().ResetRegistryFromFile( file, verify );
  if( !isOK )
  { 
    cout << "===> There were problems reset QIE Board registry values.  Trying again...." << endl;
    isOK = QIEBoardUtil::Get().ResetRegistryFromFile( file, verify );
    if( !isOK )
      cout << "===> There were problems resetting QIE Board registry values AGAIN.  Uh-oh?" << endl;
    else
      cout << "===> 2nd attempt to reset QIE board registry values worked." << endl;
  }

  //=====================
  // Make sure nobody is connected to ports 5000
  //=====================
  //only close wiznet 1 port 5000 if we are not connected to it
  cout << "===> Make sure nobody is connected to any port 5000 on QIE" << endl;
  if( 5000 != port )
    isOK = QIEBoardUtil::Get().CloseSock( 1, 5000 );
  QIEBoardUtil::Get().CloseSock( 2, 5000 );
  QIEBoardUtil::Get().CloseSock( 3, 5000 );

  return 0;
}


void usage(int argc, char *argv[] )
{
  cout << "Usage: " << argv[0] << " [qie_register_file]" << endl;
  cout << "      If no file given, use the default." << endl;
}
