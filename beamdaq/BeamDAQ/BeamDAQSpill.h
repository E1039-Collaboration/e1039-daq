#ifndef BeamDAQSpill_H
#define BeamDAQSpill_H

#include <ostream>

//todo:
// - delete copy and assignment constructor.  we store bare pointers so copies aren't what you want
//

#include"BeamDAQForwards.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>

class TDatime;

class BeamDAQSpill
{
  friend class boost::serialization::access;

  public:
  ///Constructor
  BeamDAQSpill();

  ///Destructor
  virtual ~BeamDAQSpill();

  //! Describe this object as a string
  std::string ToString() const;


  /*! Set the run number
    @param[in] runNumber The run number.  If 0, the get the runnumber from coda via plask.
    @return true if everything is OK
    */
  bool SetRunNumber( int runNumber = 0 );

  /*! Set the spill number
    @param[in] spillNumber The spill number.  If 0, the get the runnumber from spillcounter file.
    @return true if everything is OK
    */
  bool SetSpillNumber( int spillNumber = 0 );

  /*! Output spill number to file
    */
  bool OutputSpillNumber( ) const;

  //========================
  // ACNET
  //========================
  ///Fetch all relevant ACNET variables
  bool UpdateACNETVariables( );
  ///Read TURN13 from ACNET
  double GetTurn13()  const;
  ///Read BUNCH13 from ACNET
  int GetBunch13() const;
  ///Read NBSYD from ACNET
  double GetNBSYD()   const;
  ///Read NM3SEM from ACNET
  double GetNM3SEM() const;
  ///Read NM2ION from ACNET
  double GetNM2ION() const;
  ///Read G2SEM from ACNET
  double GetG2SEM()  const;
  ///Read E906BM from ACNET
  double GetE906BM() const;
  
  //===========================
  // Output functions
  //===========================
  //! Write contents to file
  bool OutputToBinary( const std::string& filename ) const;

  //! Write entry for the spill-level db ascii file
  bool OutputToSpillAscii( const std::string& filename ) const;

  //! Write entry for the spill-level ascii file for Kenichi's data-summary page
  bool OutputToSpillAsciiDataSummary( const std::string& filename ) const;

  //! Write entry for the slowcontrol_data tsv file
  bool OutputToSpillSlowcontrolData( const std::string& filename ) const;

	//! Write TTrees for Trigger, Inhibit, Spill to a file
	bool OutputToRootTrees( const std::string& filename ) const;

  //! Write SQDUTY and SQSPIL to ACNET
  bool OutputToACNET( ) const;
  bool OutputToACNET2( double hoge ) const;
  
  //! Output BeamDAQ calculations to EPICS
  bool OutputToEPICS( ) const;

	/*! Output information useful to shifters to ROOT canvases
    @param[in] dirname Print the canvases in this directory, or by default put them in official location
    */
	bool OutputToVisualROOT( const std::string& dirname = "" ) const;

  //! Get data from QIE board
  bool RetriveQIEData();

  //! Get the normalization factor \f$norm=\frac{g2sem}{qieBoardSum}\f$
  double GetChargeNormalization() const;

  //! This returns true is g2sem was below pedestal
  bool NoBeam() const;

  //! Get the run number
  unsigned int GetRunNumber() const;
  //! Get the spill number
  unsigned int GetSpillNumber() const;
  //! Get date and time as TDatime object
  TDatime GetTDatime() const;


  /*! Get the total number if inhibited linearized qie for the spill
    @param[in] forceCalc Force the loop over InhibitData.  Otherwise, it will recturn the stored calculation after the first time.
    @return sum of inhibitSum over all InhibitData
    */
  double GetInhibitBucket( bool forceCalc = false ) const;
  double GetInhibitBlockSum( bool forceCalc = false ) const;
  double GetBusyBlockSum( bool forceCalc = false ) const;
  double GetBusyBucket( bool forceCalc = false ) const;
  double GetIntensityBlockSum( bool forceCalc = false ) const;
  double GetIntensitySqBlockSum( bool forceCalc = false ) const;
  /*! Get the duty factor as a percent
    @param[in] forceCalc Use sums over QIE block if true, use QIE header if false
    @return 53MHz duty factor as percent
    */
  double GetDutyFactor( bool forceCalc = false ) const;
  unsigned long GetGoodRF( bool forceCalc = false ) const;
  unsigned long GetBadRF(  bool forceCalc = false ) const;

  bool CalculateTurnSums( ) const;


  //todo: make these private and provide const and nonconst accessors
  QIEHeaderPtr   m_qieHeader;
  QIESettingsPtr m_qieSettings;
  InhibitDataVecPtr  m_inhibitDataVec;
  TriggerDataVecPtr  m_triggerDataVec;
  QIETurnVecPtr      m_qieTurnVec;


  private:

  unsigned int m_runNumber;  ///< Run number from coda
  unsigned int m_spillNumber; ///< Spill number from spillcounter

  //lazy but effective way to store time
  //using fancy objects would make serialization hard
  unsigned short m_year;    ///< Year at construction
  unsigned short m_month;   ///< Month at construction
  unsigned short m_day;     ///< Day at construction
  unsigned short m_hour;    ///< Hour at construction
  unsigned short m_minute;  ///< Minute at construction
  unsigned short m_second;  ///< Second at construction

  mutable unsigned long m_goodRF; ///< Number of RF buckets under inhibit threshold
  mutable unsigned long m_badRF;  ///< Number of RF buckets above inhibit threshold
  mutable double m_inhibitSum; ///< Sum of linearized intensity of all inhibited buckets
  mutable double m_inhibitBucket;
  mutable double m_busySum;    ///< Sum of linearized intensity of all busied buckets that aren't inhibited
  mutable double m_busyBucket;
  mutable double m_intensitySum;   ///< Sum of linearized intensity of all buckets
  mutable double m_intensitySqSum; ///< Sum of linearized intensity squared of all buckets

  //ACNET variables.
  //  could be stored in an ACNET struct
  //  could be stored as a map from string to val if we need flexibilty
  double m_turn13;  ///< ACNET variable turn13
  int m_bunch13; ///< ACNET variable bunch13
  double m_nbsyd;   ///< ACNET variable nbsyd
  double m_nm3sem; ///< ACNET variable NM3SEM
  double m_nm2ion; ///< ACNET variable NM2ION
  double m_g2sem;  ///< ACNET variable G2SEM
  double m_e906bm; ///< ACNET variable E906BM

  template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      ar & m_runNumber;
      ar & m_spillNumber;
      ar & m_year;
      ar & m_month;
      ar & m_day;
      ar & m_hour;
      ar & m_minute;
      ar & m_second;
      ar & m_turn13;
      ar & m_bunch13;
      ar & m_nbsyd;
      ar & m_nm3sem;
      ar & m_nm2ion;
      ar & m_g2sem;
      ar & m_e906bm;
      ar & m_qieHeader;
      ar & m_qieHeader;
      ar & m_qieSettings;
      ar & m_inhibitDataVec;
      ar & m_triggerDataVec; 
      ar & m_qieTurnVec;
    }

  //! Fill Spill tree (current root TFile stores and owns this TTree)
  bool AddSpillTree() const;
  //! Fill Inhibit tree (current root TFile stores and owns this TTree)
  bool AddInhibitTree() const;
  //! Fill Trigger tree (current root TFile stores and owns this TTree)
  bool AddTriggerTree() const;

  //! Set the style for plots
  void SetROOTStyle();
  //struct stat date_buf;

  //FILE *fp2;
};

std::ostream& operator<<(std::ostream& os, const BeamDAQSpill& spill );

#endif
