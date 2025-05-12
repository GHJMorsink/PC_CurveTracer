# -*- coding: utf-8 -*-
'''
Main program startup for the CurveTracer
----------------------------------------

Contains the main configuration startup and starts all elements
'''


import sys
import time
import ctypes
import traceback

from gui import GuiBuilder
from measuring import Measure
from bootld import Bootload


class Application(object):
    ''' Contains the LRC application '''
    def __init__(self):
        ''' Application (root) object constructor '''
        self.__guiBuilder = None
        self.statusfunction = None
        self.Meas = None

    def getDevice(self):
        ''' try to find the device '''
        if self.Meas:
            self.Meas.getDevice()

    def doReset(self, fileName):
        ''' request for a reset and boot loading '''
        if fileName == '':
            return
        if self.Meas:
            self.Meas.sendResetCmd()
        time.sleep(0.5)
        flasher = Bootload(fileName)
        flasher.start_cl(self.statusfunction)


    def calibrate(self, value):
        ''' translate the user input to a calibration
        '''
        try:
            rawvalue = float(value)
            #print 'Read value:', rawvalue
            if rawvalue > 0:
                self.Meas.writeCalibration(0, rawvalue)
                self.Meas.writeCalibration(1, rawvalue)
        except:
            traceback.print_exc()


    def once(self, statusfunction, bias=[5], points=64, traces=1, side=0):
        ''' Do a one-shot measurement for a set of traces with basecurrent/gatevoltage "bias" '''
        self.statusfunction = statusfunction
        self.Meas.setDsp(statusfunction)
        self.Meas.getDevice()
        if self.Meas.connected:
            statusfunction('once', Version = self.Meas.version)
            if side & 0x02:
                side &= 0x01
                for trace in range(traces):
                    self.Meas.doInitNew0Measurement(bias[trace], points, trace, side)
                    self.Meas.dostart()
                # and for the other side do a single trace without bias
                side ^= 0x01    # flip the side
                self.Meas.doInitNew0Measurement(0, points, 5, side)
                self.Meas.dostart()
            else:
                for trace in range(traces):
                    self.Meas.doInitNew0Measurement(bias[trace], points, trace, side)
                    self.Meas.dostart()

    def setDisplay(self, statusfunction):
        ''' set the statusfunction '''
        self.statusfunction = statusfunction

    def getversion(self):
        ''' Get the USB device version '''
        return self.Meas.version


    def start_gui(self):
        '''Start main GUI thread'''
        #self.__app = QtGui.QApplication([])
        self.Meas = Measure()
        #At this point other subthreads already started so we only start the GUI main thread
        self.__guiBuilder = GuiBuilder( self )
        self.__guiBuilder.start()

    def stop(self):
        ''' Stop all internal stuff '''
        self.Meas.stop()


def main():
    ''' Starts the application '''

    application = Application()

    try:
        application.start_gui()
    finally:
        sys.stdout.flush()
        sys.stderr.flush()
        application.stop()
        ctypes.cdll.msvcrt.exit()


if __name__ == '__main__':
    main()
