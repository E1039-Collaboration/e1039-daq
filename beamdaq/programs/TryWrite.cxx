#include "BeamDAQ/BeamDAQGlobals.h"
#include "BeamDAQ/BeamDAQSpill.h"
#include "BeamDAQ/QIEBoardUtil.h"
#include "BeamDAQ/QIEDataTypes.h"

using namespace std;

int main(int argc, char *argv[] )
{
  BeamDAQSpill *spill = new BeamDAQSpill;

  QIEBoardUtil::Get().FillQIESettings(spill);
  QIEBoardUtil::Get().FillQIEHeader(spill);

  InhibitData *inhibitData = new InhibitData;
  spill->m_inhibitDataVec->push_back(inhibitData);
  inhibitData->onsetTurn = 234;
  inhibitData->onsetRF = 100;
  inhibitData->inhibitSum = 60000;

  TriggerData *triggerData = new TriggerData;
  spill->m_triggerDataVec->push_back(triggerData);
  triggerData->onsetTurn = 456;
  triggerData->releaseRF = 50;

  QIETurn *turn = new QIETurn;
  spill->m_qieTurnVec->push_back(turn);
  turn->capID = 0x1b1b;
  turn->rfIntensity[1] = 82;
  turn->intensitySum = 12345.6789;

  cout << spill->ToString() << endl;
  spill->OutputToBinary("test.bin");

  delete spill;

  return 0;
}
