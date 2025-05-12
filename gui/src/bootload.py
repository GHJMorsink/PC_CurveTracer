'''
Main program startup for the programmer
--------------------------------------

Contains the main configuration startup and starts all elements
Normal function:
  - Read the hex file
  - Reset connected meter
  - ping every 0.03 s for reaction
  - if within 5 sec reaction:
    - Send Clear/Erase for needed sectors
    - Repeat:
       - Transfer datablock 64 bytes, and 
       - Flash
    - Until ready
    - Send reset 
'''

import sys
import time
import optparse
#import ctypes

from loadhex import HexFile
from bootusb import BootUSBDriver


BLOCKSIZE = 64


class Application(object):
    ''' Contains the  application '''
    def __init__(self, options):
        ''' Application (root) object constructor '''
        self.hexfile = options.hexfile + '.hex'
        self.dtsstop = options.dtsstop
        self.pagesize = 0
        self.flashsize = 0
        self.datafile = HexFile(self.hexfile )
        self.driver = None

    def _emitReset(self):
        '''Emit a command to reset the Device (not yet implemented)'''
        pass

    def _pingpresence(self):
        ''' Check for presence of the Device in programming mode
            If it reacts on the message: the Device is in boot programming mode
        '''
        counter = 0
        while counter < 50: # maximal 5 seconds pinging
            connected = self.driver.getDevice()
            if connected:
                (page, flash, _) = self.driver.getStatusReport()
                if page != 0:
                    self.pagesize = page
                    self.flashsize = flash
                    return True
            time.sleep(0.005)  # wait 10 ms
            counter += 1
        return False  # nothing found


    def _sendblock(self, blk):
        ''' Send a block of maximal BLOCKSIZE bytes to the device (and flash) '''
        flashstartaddress = blk * BLOCKSIZE
        internalbuf = self.datafile.getmemoryportion(flashstartaddress, BLOCKSIZE)
        flashlength = len(internalbuf)
        counter = 0
        while counter < 3:
            counter += 1
            if not self.driver.writeDatablock(flashstartaddress, internalbuf, flashlength):
                print('Error sending block %d' % blk)
                return False
            time.sleep(0.2)
            return True
        return False


    def start_cl(self):
        ''' Commandline interface '''
        print("Loading %s" % self.hexfile)
        length = self.datafile.readfile()
        blocknumber = length / BLOCKSIZE
        block = 0
        self.driver = BootUSBDriver()
        if not self.driver.getDevice():
            print("Connection to device not present")
            sys.exit(1)

        print('do flashing')
        if self.dtsstop:
            self._emitReset()
            time.sleep(0.1)
        if not self._pingpresence():
            print("No device found (not responding)")
            return
        #connected
        print("Flashing")
        while block <= blocknumber:
            print('\r%3d%%' % ((100*block)/blocknumber),)
            print("Flashaddress %04X" % (block * BLOCKSIZE),)
            if not self._sendblock(block):
                print("\r\nFlashing not succeeded")
                return
            block += 1
        #self.can.sendCANmsg(SENDID, chr(RESTARTCOMMAND) + chr(0)) # Restart the system
        print("\r\nSUCCESS")
        return

    def stop(self):
        ''' ending '''
        pass

def main():
    ''' Starts the application '''
    parser = optparse.OptionParser(
        usage = "%prog [options] [hexfile]",
        description = "AVR-USBflasher for python-supported systems (console app)" )

    parser.add_option("-f", "--file", dest = "hexfile",
                        help = "The Intel-hex file to be flashed (without the "'".hex"'")",
                        default = '')

    parser.add_option("-s", "--stop", dest = "dtsstop",
                        action = "store_true",
                        help = "Request to stop the running application software",
                        default = False)
    orgargs = sys.argv
    (options, args) = parser.parse_args()

    if len(orgargs) == 1:
        parser.print_help()
        sys.exit(1)

    if len(orgargs) > 1:
        if (args == []) and (options.hexfile == ''):
            parser.print_help()
            sys.exit(1)
        if options.hexfile == '':
            options.hexfile = args[0]


    application = Application(options)

    application.start_cl()
    application.stop()


if __name__ == '__main__':
    main()
