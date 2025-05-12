'''
Reading Intel-hex file to memory
--------------------------------

.. autoclass:: HexFile
   :members:

Read the hex file to memory,
set checksum,
write to new hex file

'''
#Created on Jun 27, 2011
#author: GHJ Morsink

import sys

class HexFile(object):
    ''' Basic Intel hex file structure 
    '''

    def __init__(self, readfilename = ''):
        ''' setup the used structures
        '''
        self.localmemory = [0xFF for _ in range(0x8000)] #pylint: disable-msg=W0612
        self.endaddress = 0
        self.beginaddress = 0x8000
        self.inFile = readfilename

    def setFilenames(self, readfilename):
        ''' Set the used names for reading and conversion '''
        self.inFile = readfilename

    def readfile(self):
        '''Read the file to memory'''
        infile = open( self.inFile, "r", encoding="utf-8")
        for line in infile.readlines():
            hexlen = int(line[1:3], 16)
            hexaddress = int(line[3:7], 16)
            hextype = line[7:9]
            if hextype == '00':
                self.beginaddress = min(self.beginaddress, hexaddress)
                self.endaddress = max(self.endaddress, hexaddress + hexlen)
                for item in range(hexlen):
                    membyte = int(line[(9 + 2*item):(11 + 2*item)], 16)
                    self.localmemory[hexaddress+item] = membyte
                    # NOTE: we do not check the line's checksum
        infile.close()
        return self.endaddress - self.beginaddress


    def addchecksum(self):
        ''' Add the length/checksum at locations 0x1040 '''
        if (self.beginaddress < 0x1000) or \
           (self.endaddress > 0x7FFF):
            length = self.endaddress - self.beginaddress

            return (length, -1)
        checksum = 0
        length = self.endaddress - 0x1000
        self.localmemory[0x1040] = length
        self.localmemory[0x1041] = 0
        for i in range(length):
            checksum = (checksum + self.localmemory[i + 0x1000]) & 0xFFFF
        self.localmemory[0x1041] = (-checksum) & 0xFFFF
        return (length, checksum)


    #   def writefile(self):
    #     ''' Write the memory to the given output hex file '''
    #     currentbegin = self.beginaddress
    #     outfile = open(self.outFile, 'w')
    #     while currentbegin < self.endaddress:
    #       clen = 16
    #       if (currentbegin + clen) > self.endaddress:
    #         clen = self.endaddress - currentbegin
    #       line = (':%02X%04X' % (clen*2, currentbegin))+'00'
    #       for i in range(clen):
    #         line = line + '%04X' % (self.localmemory[currentbegin+i])
    #       chk = 0
    #       for i in range(4+2*clen):
    #         chk = (chk + int(line[(1+2*i):(3+2*i)], 16)) & 0xFF
    #       chk = (0x100 - chk) & 0xFF
    #       line = line + '%02X\n' % chk
    #       outfile.write(line)
    #       currentbegin = currentbegin + clen
    #     outfile.write(':00000001FF\n')
    #     outfile.close()


    def getmemoryportion(self, startaddress, size):
        ''' return a portion of the memory '''
        outp = []
        if (startaddress < self.beginaddress) or \
           (startaddress > self.endaddress):
            return outp
        if (startaddress + size) > self.endaddress :
            size = self.endaddress - startaddress

        for i in range(size):
            outp.append( self.localmemory[i+startaddress] )
        return outp


#------------------------------------------------------
def main():
    ''' Test the main method '''
    args = sys.argv
    if len(args) == 2:
        hexitem = HexFile(args[1])
        hexitem.readfile()
        length, checksum = hexitem.addchecksum()
        #hexitem.writefile()
        print('Written %s, with length 0x%04X words and checksum 0x%04X' % \
              (args[1], length, checksum))
    else:
        print('Calling convention: %s [Infile] [OutFile]' % args[0])

if __name__ == '__main__':
    main()
