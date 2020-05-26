#!/usr/bin/env bash
#########################
#Sets up to run BeamDAQ
#Sets environmental variables - near the bottom
#########################

export BEAMDAQ_ROOT=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)
export BEAMDAQ_BIN=$BEAMDAQ_ROOT/bin
export BEAMDAQ_LIB=$BEAMDAQ_ROOT/lib

#todo remove old instances of BeamDAQ from PATHs
export PATH=$BEAMDAQ_BIN:$PATH
export LD_LIBRARY_PATH=$BEAMDAQ_LIB:$LD_LIBRARY_PATH

#execute a caget to initialize our EPICS connection
#caget E906BOS  >> /dev/null
caget BOS  >> /dev/null
