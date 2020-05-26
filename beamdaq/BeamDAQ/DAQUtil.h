#ifndef DAQUTIL_H
#define DAQUTIL_H
#include <string>
 
class TCanvas;

/*! Singlelton class to help with command DAQ tasks.

  Gets and sets EPICS variables.
  Gets and sets ACNET variables.
  Gets current run and spill numbers.
  Handles some utilitarian system tasks.
  */
class DAQUtil
{
  public:
    ///Singleton gettor
    static DAQUtil& Get();

    //=============================//
    // Fetch run and spill
    //=============================//
    /*! Get the run number from coda.
      @return The run number.  0 if there was a problem
      */
    int GetRunNumber() const;

    /*! Get the spill number from coda
      @return The spill number.  0 if there was a problem
      */
    int GetSpillNumber() const;

    /*! Determine if MainDAQ is between runs
      @return false if the coda is active or near active, true otherwise
      */
    bool IsDAQBetweenRuns() const;

    //=============================//
    // EPICS
    //=============================//
    /*! Read an int from EPICS
      @param[in] name Name of the EPICS variable
      @param[out] rval Value fetched from EPICS
      @return True if everything worked and the value of rval is to be trusted
      */
    bool ReadEPICS( const std::string& name, int &rval ) const;

    /*! Write a value to EPICS
      @param[in] name Name of the EPICS variable
      @param[in] val Value to put into EPICS
      @return True if everything worked
      */
    bool WriteEPICS( const std::string& name, const std::string& val ) const;


    //=============================//
    // ACNET
    //=============================//
    /*! Read an int from ACNET
      @param[in] name Name of the variable
      @param[out] rval Value fetched from ACNET
      @return True if everything worked and the value of rval is to be trusted
      */
    bool ReadACNET( const std::string& name, int &rval ) const;

    /*! Read a double from ACNET
      @param[in] name Name of the variable
      @param[out] rval Value fetched from ACNET
      @return True if everything worked and the value of rval is to be trusted
      */
    bool ReadACNET( const std::string& name, double &rval ) const;

    /*! Write a value to ACNET
      @param[in] name Name of the ACNET variable
      @param[in] val Value to put into ACNET
      @return True if everything worked
      */
    bool WriteACNET( const std::string& name, const std::string& val ) const;



    //============================
    // System Utils
    //============================
    /*! Prepare a directory on the local file system
      @param[in] dir Directory to create
      @return Return code from mkdir
      */
    int PrepareDir( const std::string& dir ) const;
    
    /*! Expand environmental variables in a string
      @param[in] s String that may have environmental variabless in it
      @return String with environmental variables epanded
      */    
    std::string ExpandEnvironmentals( const std::string& s ) const;

    //============================
    // ROOT Utils
    //============================
    /*! Get a pointer to existing canvas or create a new canvas for a specific purpose (keyed by name)
      @param[in] name Name of the canvas
      @return Pointer to the TCanvas, or NULL if name is unknown
      */
    TCanvas* GetOrCreateCanvas( const std::string& name ) const;

};

#endif
