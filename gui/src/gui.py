# pylint: disable-msg=C0302
'''Startup gui'''

#pylint: disable-msg=E0611,E1101,E1103,F0401, E1002, W0105
#if the designer block isn't compiled these errors will occur
import os
import time
from PyQt5 import QtGui, QtCore, QtWidgets
import pyqtgraph as pg
from pyqtgraph.graphicsItems.ViewBox.ViewBox import ViewBox
from pyqtgraph.widgets.PlotWidget import PlotWidget

from designer.CurveTracer import Ui_MainWindow
from version import __build_version__


home = os.path.dirname(__file__)

MAXCURVES = 5
_BACKGROUND = '#FFFFFF'
_YAXISWIDTH = 35
_BLACK = '#000000'

defaultV = [4.99, 4.0, 3.0, 2.0, 1.0]
RESISTOR = 100.0        #BASE RESISTOR IN KOHM
RWAIT = 5   # wait time when repeating

class MyPlotWidget(PlotWidget):
    '''Derived class to intercept keyboard events'''
    def __init__(self, parent=None):
        PlotWidget.__init__(self, parent=parent)
        PlotWidget.setBackground(self, _BACKGROUND)
        self.plotItem.hideButtons()
        self.plotItem.setMouseEnabled(x=False, y=False)
        self.setMouseEnabled(False)
        #defaults for this graph
        self.showGrid(x=True, y=True, alpha=0.2)
        self.getAxis("bottom").setPen(color=_BLACK)
        self.getAxis("left").setWidth(_YAXISWIDTH)

    def onShiftSpace(self):
        '''event handler to be overridden'''
        pass

    def keyPressEvent(self, ev): #pylint: disable=R0201
        ''' intervention/overload on events (no keyboard actions)'''
        if type(ev) == QtGui.QKeyEvent: #pylint: disable=C0123
            if ev.key() == QtCore.Qt.Key_Space and (ev.modifiers() & QtCore.Qt.ShiftModifier):
                self.onShiftSpace()
                ev.ignore()
            else:
                super().keyPressEvent(ev)

    def scale(self, sx, sy, center=None):
        ''' Override scaling '''
        pass

    def wheelEvent(self, ev):
        ev.ignore()


class MyMainWindow(QtWidgets.QMainWindow):
    ''' classdoc MyMainWindow '''
    def __init__(self, app, methods, parent=None):
        '''  constructor '''
        super().__init__(parent)
        self.stopped = True
        self._app = app
        self.methods = methods
        self.splitting = [1, 1]
        self.upgradefile = ''
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.ui.versionLabel.setText('USB Curve Tracer  Version 0.1_%s.\n' \
                                     '\251 2025, GHJ Morsink'
                                      % __build_version__)
        # Calibration
        self.ui.SetCorrFpushButton.clicked.connect(self._setCorrFactors)
        # about block ...
        self.ui.browsePushButton.clicked.connect(self._browse)
        self.ui.FlashButton.clicked.connect(self._flash)
        # changing graph
        self.ui.TopLeftNegative.toggled.connect(self.setGraphDim)
        self.ui.TopLeftPositive.toggled.connect(self.setGraphDim)
        self.ui.Fullgrapghbox.toggled.connect(self.setGraphDim)
        self.ui.TopLeftbox.toggled.connect(self.setGraphDim)

        self.ui.SplitterButton.clicked.connect(self.changeSplit)
        self.ui.pushButton_Single.clicked.connect(self.oneShot)
        self.ui.pushButton_Continuous.clicked.connect(self.continuous)
        self.ui.voltageY0v.setText('%0.2f' %  defaultV[0])
        self.ui.currentY0ma.setText(self.getC(defaultV[0]))
        self.ui.voltageY1v.setText('%0.2f' %  defaultV[1])
        self.ui.currentY1ma.setText(self.getC(defaultV[1]))
        self.ui.voltageY2v.setText('%0.2f' %  defaultV[2])
        self.ui.currentY2ma.setText(self.getC(defaultV[2]))
        self.ui.voltageY3v.setText('%0.2f' %  defaultV[3])
        self.ui.currentY3ma.setText(self.getC(defaultV[3]))
        self.ui.voltageY4v.setText('%0.2f' %  defaultV[4])
        self.ui.currentY4ma.setText(self.getC(defaultV[4]))

        self.ui.voltageY0v.editingFinished.connect(self._checkinputV0)
        self.ui.voltageY1v.editingFinished.connect(self._checkinputV1)
        self.ui.voltageY2v.editingFinished.connect(self._checkinputV2)
        self.ui.voltageY3v.editingFinished.connect(self._checkinputV3)
        self.ui.voltageY4v.editingFinished.connect(self._checkinputV4)

        # Create the plot and curves
        self.plot, self.curve = self.create_plot()
        self.pens = [pg.mkPen("#00C000", width=2),
                     pg.mkPen("#B06000", width=2),
                     pg.mkPen("#0000FF", width=2),
                     pg.mkPen("#FF0000", width=2),
                     pg.mkPen("#FF00FF", width=2),
                     pg.mkPen("#808080", width=2)]
        self.horaxisdata = [[0]*1, [0]*1, [0]*1, [0]*1, [0]*1, [0]*1]
        self.vertaxisdata = [[0]*1, [0]*1, [0]*1, [0]*1, [0]*1, [0]*1]
        self.show()

    def getC(self, val):
        ''' calculate current from voltage '''
        voltage = val - 0.6
        voltage = max(voltage, 0)
        current = 1000.0*voltage/RESISTOR
        return '%3.01f' % current


    def _browse(self):
        '''Browse the file system, show a file browser popup'''
        fileName = QtWidgets.QFileDialog.getOpenFileName(None,
                                                     "Select Hex file", "", "*.hex")
        if fileName[0] != "":
            self.ui.fileNameLineEdit.setText(fileName[0])

    def _flash(self):
        ''' run the flasher '''
        print('flash')
        self.methods.setDisplay(self.UpdateScrStatus)
        self.methods.doReset(str(self.ui.fileNameLineEdit.text()))

    def _setCorrFactors(self):
        ''' set the calibration data '''
        self.methods.calibrate(self.ui.CorrFactorEdit.text())

    def _checkinputV0(self):
        ''' changed input '''
        try:
            raw = float(self.ui.voltageY0v.text())
        except:
            raw = 1.0
        raw = min(raw, 4.99)
        raw = max(raw, 0.2)
        self.ui.voltageY0v.setText('%0.02f' % raw)
        self.ui.currentY0ma.setText(self.getC(raw))

    def _checkinputV1(self):
        ''' changed input '''
        try:
            raw = float(self.ui.voltageY1v.text())
        except:
            raw = 1.0
        raw = min(raw, 4.99)
        raw = max(raw, 0.2)
        self.ui.voltageY1v.setText('%0.02f' % raw)
        self.ui.currentY1ma.setText(self.getC(raw))

    def _checkinputV2(self):
        ''' changed input '''
        try:
            raw = float(self.ui.voltageY2v.text())
        except:
            raw = 1.0
        raw = min(raw, 4.99)
        raw = max(raw, 0.2)
        self.ui.voltageY2v.setText('%0.02f' % raw)
        self.ui.currentY2ma.setText(self.getC(raw))

    def _checkinputV3(self):
        ''' changed input '''
        try:
            raw = float(self.ui.voltageY3v.text())
        except:
            raw = 1.0
        raw = min(raw, 4.99)
        raw = max(raw, 0.2)
        self.ui.voltageY3v.setText('%0.02f' % raw)
        self.ui.currentY3ma.setText(self.getC(raw))

    def _checkinputV4(self):
        ''' changed input '''
        try:
            raw = float(self.ui.voltageY4v.text())
        except:
            raw = 1.0
        raw = min(raw, 4.99)
        raw = max(raw, 0.2)
        self.ui.voltageY4v.setText('%0.02f' % raw)
        self.ui.currentY4ma.setText(self.getC(raw))

    def setGraphDim(self):
        ''' Set other dimensions '''
        if self.ui.Fullgrapghbox.isChecked():
            self.plot.plotItem.getViewBox().setRange(xRange=(-5.0, +5.0))
            self.plot.plotItem.getViewBox().setRange(yRange=(-10.0, 10.0))
        else:
            if self.ui.TopLeftNegative.isChecked():
                self.plot.plotItem.getViewBox().setRange(xRange=(-5.0, 0))
                self.plot.plotItem.getViewBox().setRange(yRange=(-10.0, 0))
            else:
                self.plot.plotItem.getViewBox().setRange(xRange=(0, 5.0))
                self.plot.plotItem.getViewBox().setRange(yRange=(0, 10.0))

    def create_plot(self):
        """ 
        Purpose:   create the pyqtgraph plot on the frame
        Return:    return a list containing the plot and the list of the curves
        """
        plot = MyPlotWidget() # the graph for 2D representation
        plot.setObjectName("graphWidget")
        self.ui.GraphLayout.addWidget(plot, 2)

        # plot.getAxis("left").setWidth(10)
        plot.showGrid(x=True, y=True)
        plot.plotItem.getViewBox().setRange(xRange=(0, 5.0))
        plot.plotItem.getViewBox().disableAutoRange(ViewBox.YAxis)
        plot.plotItem.getViewBox().setRange(yRange=(0, 10.0))
        plot.setLabel('left', 'mA', color='black')
        plot.setLabel('bottom', 'Volt')
        curve = [plot.plotItem.plot(connect="finite", name="test1"),
                 plot.plotItem.plot(connect="finite", name="test2"),
                 plot.plotItem.plot(connect="finite", name="test3"),
                 plot.plotItem.plot(connect="finite", name="test4"),
                 plot.plotItem.plot(connect="finite", name="test5"),
                 plot.plotItem.plot(connect="finite", name="testpos")]

        return plot, curve

    def update_plot(self):
        ''' update the shown plot '''
        for i in range(6):
            self.curve[i].setData(self.horaxisdata[i], self.vertaxisdata[i], pen=self.pens[i])
        self.plot.replot()


    def getBiasValue(self, item):
        ''' get a value derived from the text written in item between 0 and 255 as bias-value'''
        text = item.text()
        try:
            val = float(text)
            intval = int(val * 51.0)
        except:
            intval = 1
        return intval & 0xFF


    def continuous(self):
        ''' set automatic repeat method '''
        if self.ui.pushButton_Continuous.isChecked():
            self.stopped = False
            while not self.stopped:
                self.singleShot()
                if not self.stopped:    # could be switched off already!
                    time.sleep(RWAIT)
        else:
            self.stopped = True


    def oneShot(self):
        ''' only once if not in repeated mode '''
        if self.ui.pushButton_Continuous.isChecked():
            self.ui.pushButton_Continuous.setChecked(False)
            self.stopped = True
        else:
            self.singleShot()


    def singleShot(self):
        ''' set the measuring once data '''
        traces = self.ui.StepSpinBox.value()
        bias = [self.getBiasValue(self.ui.voltageY4v),
                self.getBiasValue(self.ui.voltageY3v),
                self.getBiasValue(self.ui.voltageY2v),
                self.getBiasValue(self.ui.voltageY1v),
                self.getBiasValue(self.ui.voltageY0v)
                ]
        try:
            points = int(self.ui.HPointsEdit.text())
        except:
            points = 1      # this will default to 16, and set the text!
        points = max(points, 16)
        points = min(points, 255)
        self.ui.HPointsEdit.setText("%d" % points)
        side = 0
        if self.ui.Fullgrapghbox.isChecked():
            side |= 0x02
        if self.ui.TopLeftNegative.isChecked():
            side |= 0x01
        self.clrPlot()
        self.methods.once(self.UpdateScrStatus, bias, points, traces, side)


    def clrPlot(self):
        ''' Clear the plotscreen and the measurements '''
        self.horaxisdata = [[0]*1, [0]*1, [0]*1, [0]*1, [0]*1, [0]*1]
        self.vertaxisdata = [[0]*1, [0]*1, [0]*1, [0]*1, [0]*1, [0]*1]
        #self.plot.clear()
        for i in range(6):
            self.curve[i].clear()
        #self.plot.replot()


    #pylint: disable=R0913
    def UpdateScrStatus(self, text,
                        AValue = None, CorrFactor = None,
                        Version = None):
        ''' Change values on screen in status and progress '''
        self.ui.statusbar.showMessage(text)
        if Version:
            self.ui.VersionFirmwText.setText('%d.%02d' % (Version/256, Version & 0xFF))
        if AValue:
            self.horaxisdata[AValue[0]].append(AValue[2])
            self.vertaxisdata[AValue[0]].append(AValue[1])
            self.update_plot()
        if CorrFactor:
            self.ui.CorrFactorEdit.setText('%0.3f' % (CorrFactor/1000.0))

        self._app.processEvents()
        return True


    def show(self):
        ''' Start the constant timer '''
        QtWidgets.QMainWindow.show(self)

    def changeSplit(self):
        ''' change the splitting panels from collapse/open the settings/about panel widget '''
        self.splitting = [0,1] if self.splitting == [1,1] else [1,1]
        #get the splitter widget to resize
        self.ui.tracerSplitter.setSizes(self.splitting)


class GuiBuilder(object):
    ''' GuiBuilder '''

    def __init__(self, methods ):
        '''Constructor'''
        self.app = QtWidgets.QApplication([])
        self.mw = MyMainWindow(self.app, methods)
        self.app.aboutToQuit.connect(self.exitHandler)
        self.app.lastWindowClosed.connect(self.exitHandler)


    def exitHandler(self):
        ''' system stops '''
        self.mw.methods.stop()


    def start(self):
        '''Run the gui event loop'''
        self.mw.setWindowTitle("USB I-V Curve Tracer v0.1")
        self.mw.show()
        self.app.setStyle(QtWidgets.QStyleFactory.create('Windows'))
        self.app.exec_()
