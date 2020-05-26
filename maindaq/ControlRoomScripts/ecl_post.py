#!/usr/local/bin/python3
#
import os
proxy ="e906-gat6.fnal.gov:3128/"
os.environ['HTTP_PROXY'] = "http://"+proxy
os.environ['http_proxy'] = "http://"+proxy
os.environ['HTTPS_PROXY'] = "https://"+proxy
os.environ['https_proxy'] = "https://"+proxy



from __future__ import print_function
from ECLAPI import ECLConnection, ECLEntry

#URL = "http://dbweb6.fnal.gov:8080/ECL/spin_quest"
URL = "http://dbweb6.fnal.gov:8080/ECL/spin_quest"
user = "SQ_DAQ"
password = "nuc-dep"


if __name__ == '__main__':
    '''
    '''
    # For testing only!
    import ssl
    ssl._create_default_https_context = ssl._create_unverified_context

    import getopt, sys

    opts, args = getopt.getopt(sys.argv[1:], 'np:u:U:f:r:s:k:c:')

    print_only = False

    for opt, val in opts:
        if opt=='-n':           # Print the entry to screen, not to post it
            print_only = True
        if opt=='-p':           # User password
            password = val
        if opt=='-u':           # User name
            user = val
        if opt == '-U':         # ECL instance URL to use for the posts
            URL = val
        if opt == '-f':         # ECL instance URL to use for the posts
            RunFile = val
        if opt == '-r':         # run number
            runN = val
        if opt == '-s':         # run number
            usrs = val
            print(usrs)
        if opt == '-k':         # run number
            keywds = val
            print(keywds)
        if opt == '-c':         # run number
            cmts = val
            print(cmts)

##########################################################################
#   Create test entry
#                 tags=['Muon'],
#    e = ECLEntry(category='XML_test',

    textStr = """Current run number is """+runN+"\n"

    fileToSend = '/data2/e906daq/coda/run_descriptor/'+RunFile
    print(fileToSend)

    with open(fileToSend) as f2send:
       for line in f2send.readlines():
          if 'user' in line:
#            print(line)
             textStr=textStr+line
          if 'keyword' in line:
#            print(line)
             textStr=textStr+line
          if 'comment' in line:
#            print(line)
             textStr=textStr+line

    print(textStr)


    e = ECLEntry(category='Runs',
                 formname='default',
                 text=textStr,
                 preformatted=False)

    if True:
        # Optional. Set the entry comment
        e.addSubject('New run started: '+runN)

    if False:
        # Assume the form has the fields named 'firstname', 'lastname', 'email'
        # Fill some fields in the form
        e.setValue(name="firstname", value='John')
        e.setValue(name="lastname", value='Doe')
        e.setValue(name="email", value='johndoe@domain.net')

    if True:
        # Attach some file; tried and successful      Andrew Chen, 2019/07/25
        #e.addAttachment(name='attached-file', filename='/data2/e906daq/coda/run_descriptor/run_descript_505.dat')
        e.addAttachment(name='attached-file', filename=fileToSend)
#       data='Data may come here as an argument. The file will not be read in this case')

    if False:
        # Attach some image; tried and successful     Andrew Chen, 2019/07/25
        # Image data also can be passed as a parameter using 'image' argument.
        e.addImage(name='aurora light', filename='/data2/users/chenyc/Pictures/img_lights.jpg',image=None)

    if not print_only:
        # Define connection
        elconn = ECLConnection(url=URL, username=user, password=password)
        #
        # The user should be a special user created with the "raw password" by administrator.
        # This user cannot login via GUI

        # Post test entry created above
        response = elconn.post(e)

        # Print what we have got back from the server
        print(response)

        # Close the connection
        elconn.close()
    else:
        # Just print prepared XML
        print(e.xshow())

