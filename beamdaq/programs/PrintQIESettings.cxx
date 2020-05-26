#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/BeamDAQSpill.h"
#include "BeamDAQ/QIEBoardUtil.h"
#include "BeamDAQ/QIEDataTypes.h"

using namespace std;

int main(int argc, char *argv[] )
{
  //connect to nonstandard port on socket 1
	QIEBoardUtil::Get().ConnectSock1(5001);

  BeamDAQSpill *spill = new BeamDAQSpill;
  bool isOK = QIEBoardUtil::Get().FillQIESettings( spill );

  cout << spill->m_qieSettings->ToString() << endl;

  cout << " isOK = " << isOK << endl;

  delete spill;

  return 0;
}
