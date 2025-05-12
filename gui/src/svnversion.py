'''
Main program for generating version header file
-----------------------------------------------

Contains the main configuration setup
Normal function:
  - write file
  - Run c:\\Program Files\\TortoiseSVN\\bin\\SubWCRev.exe
  
  change for git-repositories:

 git show --pretty=format:"commit %h timestamp %at" -s
 gives:
commit e8e0101 timestamp 1534414148

'''

import subprocess
import datetime

#----- configuration -----
# SubWCRevFileName = 'c:\\Program Files\\TortoiseSVN\\bin\\SubWCRev.exe'
HeaderFileName = '.\\version.py'

def __capture(cmdLine):
    """Returns the output of a command line on success, None on failure."""
    p = subprocess.Popen(cmdLine, shell=True, stdout=subprocess.PIPE)
    if p.wait() == 0:
        return p.communicate()[0]
    return ''

#----- write the file -----
def GenerateVersionHeader():
    ''' Create a header file which shows version information 
        Revised version for git repositories: returns the last abbreviated commit hash
        and last commit time.

    '''
    version_heading = \
"""
'''This file contains version information. This information is
automatically generated -- do not edit.
(c) GHJ Morsink 2010 - 2025
'''
# pylint: disable=C0301,C0111


"""
    f = open(HeaderFileName, 'w')
    f.write(version_heading)

    ver = __capture('git show --pretty=format:"commit %h timestamp %at" -s').decode('utf-8')
    externals = __capture('git submodule summary').decode('utf-8')
    url = __capture('git config --get remote.origin.url').decode('utf-8')
    if ver is None:
        ver = 'commit UNKNOWN time 0'
    posat = url.find('@')
    if posat > 0:
        if url.startswith('https'):
            url = 'https://' + url[posat+1:-1]
        else:
            url = 'http://' + url[posat+1:-1]
    else:
        url = url[:-1]
    ver = ver.split(' ')
    while not ver[3][-1].isdigit():
        ver[3] = ver[3][:-1]
    while not ver[3][0].isdigit():
        ver[3] = ver[3][1:]
    committimestr = datetime.datetime.fromtimestamp(float(ver[3]), datetime.UTC).isoformat()

    f.write('__build_version__ = "%s"\n' % ver[1])
    f.write('__revision_time__ = "%s"\n' % committimestr)
    f.write('__git_location__ = "%s"\n' % url)
    f.write('__externals__ = "%s"\n\n' % externals)


if __name__ == '__main__':
    GenerateVersionHeader()
