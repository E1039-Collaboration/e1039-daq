#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/BeamDAQSpill.h"
#include "BeamDAQ/QIEBoardUtil.h"
#include "BeamDAQ/QIEDataTypes.h"

#include "BeamDAQ/BeamAnalysis.h"

using namespace std;

int main(int argc, char *argv[] )
{

  if( 2 != argc )
  {
    cout << "Usage: " << argv[0] << " <filename>" << endl;
    return 0;
  }


  BeamDAQSpill *spill = new BeamDAQSpill;

  {
    std::ifstream ifs( argv[1] );
    boost::archive::binary_iarchive ia(ifs);
    ia >> *spill;
  }

  cout << spill->ToString() << endl;

  const bool doFFT = true;
  
  double hoge = -999.;
  AnalyzeSpill(  &hoge,
                 spill, "hists.root", doFFT );

  delete spill;

  return 0;
}
