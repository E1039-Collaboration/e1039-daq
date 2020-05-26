#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/BeamDAQSpill.h"
#include "BeamDAQ/QIEDataTypes.h"
#include "BeamDAQ/BeamAnalysis.h"
#include <boost/filesystem.hpp>

#include <TApplication.h>

using namespace std;

int main(int argc, char *argv[] )
{

  if( 2 != argc )
  {
    cout << "Usage: " << argv[0] << " <filename>" << endl;
    return 0;
  }


  const string inputFilename( argv[1] );
  BeamDAQSpill *spill = new BeamDAQSpill;
  {
    std::ifstream ifs( inputFilename.c_str() );
    boost::archive::binary_iarchive ia(ifs);
    ia >> *spill;
  }

  if(0==spill)
  {
    cout << "ERROR: Could not create BeamDAQ object from input file." << endl;
    return 1;
  }


  cout << spill->ToString() << endl;

  //start application so the TCanvases are shown
  TApplication theApp("BeamDAQ",&argc,argv);

  const bool doFFT = true;
  
  
  //AnalyzeSpill( spill, "hists.root", doFFT );
  double hoge = -999.;
  AnalyzeSpill( &hoge , 
                spill, "hists.root", doFFT );
  
  spill->OutputToVisualROOT( "visual_root" );


  delete spill;

  theApp.Run();

  return 0;
}
