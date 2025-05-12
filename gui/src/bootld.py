'''
Main program startup for the programmer part
--------------------------------------

Contains the main configuration startup and starts all elements
Normal function:
  - Read the hex file
  - Reset connected meter
  - ping every 0.03 s for reaction
  - if within 5 sec reaction:
    - Repeat:
       - Transfer datablock 64 bytes, and 
       - Flash
    - Until ready
    - Send reset 
'''

import sys
import time
import optparse

from loadhex import HexFile
from bootusb import BootUSBDriver


BLOCKSIZE = 64


class Bootload(object):
    ''' Contains the  application '''
    def __init__(self, hexfile):
        ''' Application (root) object constructor '''
        self.hexfile = hexfile
        self.pagesize = 0
        self.flashsize = 0
        self.datafile = HexFile(self.hexfile)
        self.driver = None
        self.dsp = None

    def _pingpresence(self):
        ''' Check for presence of the Device in programming mode
            If it reacts on the message: the Device is in boot programming mode
        '''
        counter = 0
        while counter < 50: # maximal 0.5 seconds pinging
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


    def start_cl(self, displayfunction = None):
        ''' Commandline interface '''
        self.dsp = displayfunction
        if self.dsp:
            self.dsp( "Loading %s" % self.hexfile )
        length = self.datafile.readfile()
        maxblocknumber = length / BLOCKSIZE
        block = 0
        self.driver = BootUSBDriver()
        if not self.driver.getDevice():
            if self.dsp:
                self.dsp("Connection to device not present")
            return 1

        time.sleep(0.1)  # wait 100 ms
        if not self._pingpresence():
            if self.dsp:
                self.dsp("No device found (not responding)")
            return 1
        #connected
        #print "Flashing"
        while block <= maxblocknumber:
            if self.dsp:
                self.dsp("%3d%%,  Flashaddress %04X" % ((100*block)/maxblocknumber, (block * BLOCKSIZE)))
            if not self._sendblock(block):
                if self.dsp:
                    self.dsp("Flashing not succeeded")
                return 1
            block += 1
        if self.dsp:
            self.dsp("Flashed SUCCESSFUL")
        print("Successful")
        self._sendblock(0xFFFF/BLOCKSIZE) # This is a reset command
        self.driver.closedevice()         # release the USB connection (A must!)
        return 0

# Test and independent actions
def main():
    ''' Starts the application '''
    parser = optparse.OptionParser(
        usage = "%prog [options] [hexfile]",
        description = "AVR-USBflasher for python-supported systems (console app)" )

    parser.add_option("-f", "--file", dest = "hexfile",
                    help = "The Intel-hex file to be flashed (without the "'".hex"'")",
                    default = '')

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

    application = Bootload(options.hexfile)
    if application.start_cl() != 0:
        sys.exit(1)


if __name__ == '__main__':
    main()
