# -*- coding: utf-8 -*-
'''
USB measuring Driver 
-------------------------

The driver will check connections;
it will do initializing at change of measurement-type
it will do regular measurement-update requests
Responses are read through an independent thread.

'''

import time
from threading import Thread
from pywinusb import hid



MYVENDORID = 0x16c0     # VOTI, ob-dev usb free VID/PID for hid class
MYPRODUCTID = 0x05df

REPORT_COUNT = 6+1

MODE_N = 0              # normal measurement
MODE_L = 0x8
MODE_RESET = 0x78

CMD_INIT = 0
CMD_START = 1
CMD_MEASURE = 2
CMD_CALSET = 3
CMD_CALREAD = 4

NP_INDEX = 0
NEXT_INDEX = 1
MAX_INDEX = 2

class Measure(Thread):  #pylint: disable=E1101
    ''' classdocs measurement '''
    TIMEDELAY = 0.04  # every 40ms a update polling time between measurements

    def __init__(self):
        ''' Constructor '''
        Thread.__init__( self)
        # self.setName("Measure")

        self.dsp = None
        self.connected = False
        self.currenttype = None
        self.currentvalue = None
        self.currenttrace = 0
        self.rangecalibrations = [1]*4
        self.defaultcal = [2180, 2000, 1, 1,    # correction factors (per 4 per function)
                           1, 1, 1, 1]          # default starting values, if not calibrated yet
        self.devices = None
        self.device = None
        self.report = None
        self.buffer = [0xff]*REPORT_COUNT
        self.connectioncount = 0

        self._pollingtime = Measure.TIMEDELAY
        self._stop = False
        self.started = False
        self.version = None
        self.sendrequests = 0
        self.points = 0
        self.side = 0


    def dostart(self):
        ''' begin the system '''
        if not self.started:
            self._stop = False
            self.started = True
            self.dsp('', CorrFactor = self.rangecalibrations[0])
            self.run()
            print('Stopped')
            self.started = False

    def setDsp(self, statusf):
        ''' set the connection to the display '''
        self.dsp = statusf


    def stop(self):
        ''' Stopping this thread '''
        self._stop = True


    def getDevice(self):
        ''' try to find the device '''
        if self.device is None:
            devfilter = hid.HidDeviceFilter(vendor_id = MYVENDORID, product_id = MYPRODUCTID)
            self.devices = devfilter.get_devices()
            if self.devices:
                try:
                    self.device = self.devices[0]
                    print('Serialnr: %s' % self.device.serial_number)
                    print('Versionnr:', self.device.version_number)
                    # Check for "ghj.morsink@gmail.com TRACER" should be here
                    self.version = self.device.version_number
                    self.device.open()
                    self.report = self.device.find_feature_reports()
                    self.connected = True
                    self.connectioncount = 0
                except:
                    self.device = None
                    self.connected = False
        if not self.connected and self.dsp:
            self.dsp('No connection')


    def setCommandbuffer(self, functype):
        ''' Set the sending buffer for byte 0 and 1 according to current type
            and the requested function
            Return true if connected, else false
        '''
        if self.connected:
            self.buffer[0] = 0
            if self.currenttype < MAX_INDEX:
                self.buffer[1] = (self.currenttype * 8) + functype
            else:
                self._stop = True
                self.started = False
                return False
            return True
        return False


    def doMeasuring(self):
        ''' request a measurement according to the current type 
            (currently only type NP_INDEX implemented)
        '''
        if not self.setCommandbuffer(CMD_MEASURE):
            return

        self.report = self.device.find_feature_reports()
        self.report[0].set_raw_data(self.buffer)
        self.report[0].send()
        rbuffer = self.report[0].get()
        correction = self.rangecalibrations[0]/1000.0   # 2.18
        print('buffer: %s answer: %s' % (' '.join('{:02x}'.format(x) for x in self.buffer),
                                         ' '.join('{:02x}'.format(x) for x in rbuffer)))
        if len(rbuffer) > 2:
            self.connectioncount = 0
            self.sendrequests += 1
            if rbuffer[1] < 128:  # check reading is finished
                self.dsp('%d %d' % (self.currenttype, rbuffer[1] & 0x7f))
                return
            if self.currenttype == NP_INDEX:
                if self.side == 1:
                    MValue1 = correction * ((int(rbuffer[3])/51.0) - 5.0)
                    MValue2 = (int((rbuffer[2] + rbuffer[6]) & 0xff)/51.0) - 5.0
                else:
                    MValue1 = correction * int(rbuffer[3])/51.0
                    MValue2 = int((rbuffer[2] - rbuffer[6]) & 0xff)/51.0

                self.dsp('reading', AValue = [self.currenttrace, MValue1, MValue2])
                print('point %0.3f %0.3f' % (MValue1, MValue2))
            elif self.currenttype == NEXT_INDEX:
                MValueStr1 = int(rbuffer[2])/51.0
                MValueStr2 = int(rbuffer[3])/51.0
                self.dsp('', Avalue = [4, self.sendrequests*5.0/self.points, int(rbuffer[4])/51.0])
                self.dsp('reading', AValue = [0, MValueStr1, MValueStr2])

            if self.sendrequests > self.points:
                self.sendrequests = 0
                self.dsp('ended')
                self.stop()
        else:
            self.connectioncount += 1
            if self.connectioncount > 2:
                self.connected = False
                self.device = None
                self.version = None


    def doInitNew0Measurement(self, bias, points, trace, side):
        ''' init the measurement with bias settings  cmd-type 0 '''
        self.currenttype = NP_INDEX
        self.currenttrace = trace
        if not self.setCommandbuffer(CMD_INIT):
            return
        try:
            self.buffer[2] = bias
            self.buffer[3] = points
            self.buffer[4] = side
            self.report = self.device.find_feature_reports()
            print('sending: %s ' % (' '.join('{:02x}'.format(x) for x in self.buffer)), self.buffer)
            self.report[0].set_raw_data(self.buffer)
            self.report[0].send()

            # Read the calibrations (only index 0 used yet)
            count = 0
            found = False
            while (not found) and (count < 2):
                time.sleep(0.1)
                self.rangecalibrations[0] = self.readCalibrations(0)
                found = self.rangecalibrations[0] != 0xFFFF
                count += 1
            if self.rangecalibrations[0] == 0xFFFF:
                self.rangecalibrations[0] = self.defaultcal[0 + 4*self.currenttype]

            self.sendrequests = 1
            self.points = points
            self.side = side
        except:
            self.connected = False
            self.device.close()
            self.device = None
            time.sleep(0.5)


    def readCalibrations(self, offset):
        ''' Read the calibration value according to the current mode and offset '''
        if not self.setCommandbuffer(CMD_CALREAD):
            return 0xFFFF
        rlen = 0
        count = 0
        while rlen == 0 and count < 2:
            count += 1
            self.buffer[2] = offset
            self.report[0].set_raw_data(self.buffer)
            self.report[0].send()
            rbuffer = self.report[0].get()
            print('calbuffer:', rbuffer)
            rlen = len(rbuffer)
            if rlen > 0:
                return rbuffer[3] * 256 + rbuffer[4]
        return 0xFFFF


    def writeCalibration(self, offset, rawvalue):
        ''' Write a calibration value according to current setting
            Input: the user value as given; calibration value still to be calculated
        '''
        if not self.setCommandbuffer(CMD_CALSET):
            return
        newcalvalue = int(rawvalue * 1000.0)
        self.buffer[2] = offset
        self.rangecalibrations[offset] = newcalvalue
        self.buffer[3] = int(newcalvalue / 256) & 0xFF
        self.buffer[4] = newcalvalue & 0xFF
        print('New calibration:', newcalvalue)
        self.report = self.device.find_feature_reports()
        self.report[0].set_raw_data(self.buffer)
        self.report[0].send()


    def sendResetCmd(self):
        ''' send a reset command to the device; and stop measurements '''
        if not self.connected:
            self.getDevice()
        if self.connected:
            try:
                self.buffer[0] = 0
                self.buffer[1] = MODE_RESET
                self.report = self.device.find_feature_reports()
                self.report[0].set_raw_data(self.buffer)
                self.report[0].send()
            except:
                pass
            self.connected = False #there is no connection anymore
            self.device.close()
            self.device = None


    def run(self):
        ''' 
            This is the main thread, running on a time-element of 0.04 second. 
        '''
        while not self._stop:

            if not self.connected:
                self.getDevice()
            if self.connected:
                self.doMeasuring()

            time.sleep(self._pollingtime)  # wait  for speedy stopping
