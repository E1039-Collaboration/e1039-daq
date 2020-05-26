#ifndef BeamDAQForwards_H
#define BeamDAQForwards_H

struct QIEHeader;
struct QIESettings;
struct TriggerData;
struct InhibitData;
struct QIETurn;

class BeamDAQSpill;

//typedefs for safer pointers
typedef QIEHeader*   QIEHeaderPtr;
typedef QIESettings* QIESettingsPtr;
typedef InhibitData* InhibitDataPtr;
typedef TriggerData* TriggerDataPtr;
typedef QIETurn*     QIETurnPtr;

//vectors for those typedefs that we expect to be in vectors
typedef std::vector<InhibitDataPtr>    InhibitDataVec;
typedef std::vector<TriggerDataPtr>    TriggerDataVec;
typedef std::vector<QIETurnPtr>        QIETurnVec;

//safe pointer to those vectors
//yeah, it gets a bit silly but it's better than memory leaks
typedef InhibitDataVec* InhibitDataVecPtr;
typedef TriggerDataVec* TriggerDataVecPtr;
typedef QIETurnVec*     QIETurnVecPtr;

#endif

