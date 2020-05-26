#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/BeamDAQSpill.h"
#include "BeamDAQ/QIEDataTypes.h"

#include <boost/filesystem.hpp>


/* This is based on a Bryan Ramson program
 * that I found in his workspace.
 * It converts a BeamDAQ binary file to
 * an ascii file that is human readible.
 * Currently David Christian uses this format
 * as the input to a fortran program that
 * checks that QIE board and readout.
 */

using namespace std;

int main(int argc, char *argv[] )
{

	if( 2 != argc )
	{
		cout << "Usage: " << argv[0] << " <input filename>" << endl;
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

	//name of output file is basename of input and .dat instead of .bin
	boost::filesystem::path filepath( inputFilename );
	//TString outputFilename = filepath.filename().string(); //for newer boost
	TString outputFilename = filepath.filename(); //for older boost
	outputFilename.ReplaceAll( ".bin", ".dat" );
	cout << "OutputFileName = " << outputFilename << endl;
	ofstream fout( outputFilename.Data(),  ofstream::out );

	//output some header info
	{
		cout << "Printing header info..." << endl;

		fout
			<< "Spill Header" << endl
			<< "Inhibit Sum: " << spill->m_qieSettings->inhibitBoardSum << endl
			<< "Trigger Sum: " << spill->m_qieSettings->triggerBoardSum << endl
			<< "Intensity Sum: " << spill->m_qieSettings->qieBoardSum << endl
			<< "Intensity Sq Sum: " << spill->m_qieSettings->qieBoardSumSq << endl
			<< "Duty Factor: " << spill->GetDutyFactor() << endl
			<< "Threshold Delay: " << spill->m_qieSettings->thresholdDelay << endl
			<< "dataSumDelay: " << spill->m_qieSettings->dataSumDelay << endl
			<< "minInhibitWidth: " << spill->m_qieSettings->minInhibitWidth << endl
			<< "inhibit threshold: " << spill->m_qieSettings->threshold << endl
			<< "qieDelay: " << spill->m_qieSettings->qieDelay << endl
			<< "orbitDelay: " << spill->m_qieSettings->orbitDelay << endl
			<< "markerDelay: " << spill->m_qieSettings->markerDelay << endl
			<< "triggerBlockLength: " << spill->m_qieSettings->triggerBlockLength << endl;



		cout << "...done with header info." << endl;
	}

	//print trigger block
	{
		cout << "Printing trigger block..." << endl;

		TriggerDataVecPtr triggers = spill->m_triggerDataVec;
		fout << "Number of Triggers: " << triggers->size() << endl;

		for( TriggerDataVec::iterator i = triggers->begin(); i != triggers->end(); ++i )
		{
			TriggerDataPtr trigger = *i;
			fout 
				<< trigger->nWords << endl
				<< trigger->triggerCount << endl
				<< trigger->onsetTurn << endl
				<< trigger->onsetRF << endl
				<< trigger->releaseTurn << endl
				<< trigger->releaseRF << endl
				<< trigger->busyNotInhibit << endl;
			//todo: this is a variable sized array, but it should always be of this length
			//      if we want to catch problems then we should not assume this length.
			for( int rfIndex = 0; rfIndex != 33; ++rfIndex )
				fout << trigger->rfIntensity[rfIndex] << endl;
		}

		cout << "...done with trigger block." << endl;
	}

	//print inhibit block
	{
		cout << "Printing inhibit block..." << endl;

		InhibitDataVecPtr inhibits = spill->m_inhibitDataVec;
		fout << "Number of Inhibits: " << inhibits->size() << endl;

		for( InhibitDataVec::iterator i = inhibits->begin(); i != inhibits->end(); ++i )
		{
			InhibitDataPtr inhibit = *i;
			fout
				<< inhibit->count << endl
				<< inhibit->onsetTurn << endl
				<< inhibit->onsetRF << endl
				<< inhibit->inhibitSum << endl
				<< inhibit->releaseTurn << endl
				<< inhibit->releaseRF << endl;
		}

		cout << "...done with inhibit block." << endl;
	}

	//print QIE block
	{
		cout << "Printing QIE block..." << endl;

		QIETurnVecPtr turns = spill->m_qieTurnVec;
		fout << "Number of QIE Turns: " << turns->size() << endl;

		unsigned int turnIdx = 0;
		for( QIETurnVec::iterator i = turns->begin(); i != turns->end(); ++i )
		{
			QIETurnPtr turn = *i;
			fout 
				<< "Turn: " << turnIdx << endl
				<< "Turn ID: " << turn->GetTurnID() << endl
				<< "Cap ID: " << hex << turn->capID << dec << endl;

			//now print linearized intensity for all buckets
			for( size_t rfIndex = 0 ; rfIndex != c_BucketsPerTurn; ++rfIndex )
				fout << QIE_Conversion_Map[ turn->rfIntensity[rfIndex] ] << endl;
			++turnIdx;
		}
		cout << "...done with QIE block." << endl;
	}


	delete spill;

	return 0;
}
