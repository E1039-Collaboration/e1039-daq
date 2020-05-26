#!/bin/tcsh
#########################
#Sets up to run BeamDAQ
#Sets environmental variables - near the bottom
#########################

set sourced=($_)
if ("$sourced" != "") then
	set script_name = "$sourced[2]"
else
	set script_name = $0
endif

#if the script is a link, follow the link
if ( -l "$script_name" ) then
  set script_name = `readlink $script_name`
endif

set script_dir = `dirname $script_name`
setenv BEAMDAQ_ROOT `cd $script_dir && pwd`
setenv BEAMDAQ_BIN "$BEAMDAQ_ROOT/bin"
setenv BEAMDAQ_LIB "$BEAMDAQ_ROOT/lib"

#todo remove old instances of BeamDAQ from PATHs
setenv PATH "$BEAMDAQ_BIN":"$PATH"
setenv LD_LIBRARY_PATH "$BEAMDAQ_LIB":"$LD_LIBRARY_PATH"

#execute a caget to initialize our EPICS connection
caget E906BOS  >> /dev/null
