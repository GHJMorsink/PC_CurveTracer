# -*- coding: utf-8 -*-
'''
USB booting Driver 
-------------------------

The driver will check connections;
it will do initializing
it will do status info calling
it will do transport of blocks of 64 bytes + address

'''

from pywinusb import hid

MYVENDORID = 0x16c0     # VOTI, ob-dev usb free VID/PID for hid class
MYPRODUCTID = 0x05df

STATUSREPORT = 0
DATAREPORT = 1

STATUSLEN = 7
DATABLOCKLEN = 4+64


class BootUSBDriver(object):  #pylint: disable=E1101
    ''' classdocs usb driver '''

    def __init__(self):
        ''' Constructor '''
        self.dsp = None
        self.connected = False
        self.devices = None
        self.device = None
        self.report = None
        self.feat_report = None
        self._stop = False


    def stop(self):
        ''' Stopping this thread '''
        self._stop = True


    def getDevice(self):
        ''' try to find the device '''
        if self.device is None:
            devfilter = hid.HidDeviceFilter(vendor_id = MYVENDORID, product_id = MYPRODUCTID)
            self.devices = devfilter.get_devices()
            if self.devices:
                self.device = self.devices[0]
                #print self.device
                print('Versionnr: %d.%d' % (int(self.device.version_number/256),
                                            int(self.device.version_number & 0xFF)))
                self.device.open()
                self.report = self.device.find_feature_reports()
                print('Feature-reports:', self.report)
                self.connected = True
        return self.connected

    def getStatusReport(self):
        ''' Get the current status as
            - size of flash memory
            - flag 'previous datablock flashed'
        '''
        if not self.connected:
            return (0, 0, 0)
        sbuffer = [0xFF]*68
        sbuffer[0] = STATUSREPORT + 1
        self.report = self.device.find_feature_reports()
        count = 0
        pagesize = 0
        totalflash = 0
        readyflag = 0
        rlen = 0
        while count < 3 and rlen == 0:
            count += 1
            self.report[STATUSREPORT].set_raw_data(sbuffer)
            self.report[STATUSREPORT].send()
            rbuffer = self.report[STATUSREPORT].get()
            rlen = len(rbuffer)
            if rlen > 6:
                print('buffer:', rbuffer[0:7])
                pagesize = rbuffer[1]+256*rbuffer[2]
                totalflash = rbuffer[3]+256*rbuffer[4]
                readyflag = rbuffer[6]
        return (pagesize, totalflash, readyflag)


    def writeDatablock(self, address, dbytes, length):
        ''' write to address, a block of length data-bytes '''
        if not self.connected:
            return False
        if length > DATABLOCKLEN-4:
            return False
        sbuffer = [0xFF]*DATABLOCKLEN
        sbuffer[0] = DATAREPORT + 1
        sbuffer[1] = int(address) & 0xFF
        sbuffer[2] = (int(address/256)) & 0xFF
        sbuffer[3] = 0
        sbuffer[4] = 0
        for i in range(length):
            sbuffer[i+4] = dbytes[i]

        self.report = self.device.find_feature_reports()
        self.report[DATAREPORT].set_raw_data(sbuffer)
        self.report[DATAREPORT].send()
        return True

    def closedevice(self):
        ''' colse the connection '''
        self.device.close()
