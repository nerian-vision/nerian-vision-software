#!/usr/bin/env python3

###############################################################################/
# Copyright (c) 2021 Nerian Vision GmbH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
###############################################################################/

#
# Show a Qt5 window with polled real-time image data
# and parameter access (set operation mode)
#
# Just connects to the first of any found devices
#

try:
    from PyQt5 import QtCore, QtGui, QtWidgets
except:
    print("\n---\nThis examples requires PyQt5 (python3-pyqt5)!\n---\n")
    raise

import sys
import time
import traceback

import visiontransfer
from visiontransfer import OperationMode, ImageFormat, ImageType

class App(QtWidgets.QMainWindow):

    def __init__(self):
        super().__init__()
        self.title = 'Nerian Stereo for Python with PyQt5'
        self.left, self.top = 200, 200
        self.width, self.height = 800, 400
        self.transfer = None
        self.timer = None
        self.last_number_of_images = 0

        self.downscale_factor = 2
        self.exaggerate_depth = 1
        self.mask_undefined_depth = True
        self.resized = False
        self.statustext = ''

        self.initUI()
        self.reset_fps()

    def reset_fps(self):
        # fps
        self.frames = 0
        self.last_frames = 0
        self.fps_time = 0.0

    def initUI(self):
        self.setWindowTitle(self.title)
        self.setGeometry(self.left, self.top, self.width, self.height)
        #
        self.main_frame = QtWidgets.QFrame(self)
        self.main_frame.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.main_frame.setStyleSheet('background-color:rgb(222,222,222);')
        self.setCentralWidget(self.main_frame)
        self.main_vbox = QtWidgets.QVBoxLayout(self.main_frame)

        self.optionsframe = QtWidgets.QFrame(self.main_frame)
        self.optionsframe.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.main_vbox.addWidget(self.optionsframe)
        self.optionsbox =  QtWidgets.QHBoxLayout(self.optionsframe)
        self.optionsbox.setAlignment(QtCore.Qt.AlignLeft)

        self.zoomcombo = QtWidgets.QComboBox()
        self.zoomcombo.activated.connect(self.activated_zoomcombo)
        self.zoomcombo.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        self.zoomcombo.addItem('Zoom 100%', 1)
        self.zoomcombo.addItem('Zoom 50%',  2)
        self.zoomcombo.addItem('Zoom 25%',  4)
        self.zoomcombo.setCurrentIndex(1 if self.downscale_factor==2 else (2 if self.downscale_factor==4 else 0))
        self.optionsbox.addWidget(self.zoomcombo)

        self.checkbox_mask = QtWidgets.QCheckBox('Remask undefined depths black')
        self.checkbox_mask.toggled.connect(self.toggled_mask)
        self.checkbox_mask.setChecked(self.mask_undefined_depth)
        self.checkbox_mask.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        self.optionsbox.addWidget(QtWidgets.QLabel('     '))
        self.optionsbox.addWidget(self.checkbox_mask)

        self.exaggeratecombo = QtWidgets.QComboBox()
        self.exaggeratecombo.activated.connect(self.activated_exaggeratecombo)
        self.exaggeratecombo.setSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        self.exaggeratecombo.addItem('None', 1)
        self.exaggeratecombo.addItem('x2',  2)
        self.exaggeratecombo.addItem('x4',  4)
        self.exaggeratecombo.setCurrentIndex(1 if self.exaggerate_depth==2 else (2 if self.exaggerate_depth==4 else 0))
        self.optionsbox.addWidget(QtWidgets.QLabel('    Depth exaggeration'))
        self.optionsbox.addWidget(self.exaggeratecombo)

        self.imageframe = QtWidgets.QFrame(self.main_frame)
        self.main_vbox.addWidget(self.imageframe)
        self.imagebox =  QtWidgets.QHBoxLayout(self.imageframe)

        self.images = []
        self.images.append(QtWidgets.QLabel())
        self.images.append(QtWidgets.QLabel())
        self.images.append(QtWidgets.QLabel())
        self.images[0].setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.images[1].setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.images[2].setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.imagebox.addWidget(self.images[0])
        self.imagebox.addWidget(self.images[1])
        self.imagebox.addWidget(self.images[2])

        self.formatlabel = QtWidgets.QLabel('  No images received yet')
        self.main_vbox.addWidget(self.formatlabel, 0, QtCore.Qt.AlignLeft)

        self.buttonframe = QtWidgets.QFrame(self.main_frame)
        self.buttonframe.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Minimum)
        self.main_vbox.addWidget(self.buttonframe)
        self.buttonbox =  QtWidgets.QHBoxLayout(self.buttonframe)



        # Buttons

        bold_font = QtGui.QFont()
        bold_font.setBold(True)

        self.start_button = QtWidgets.QPushButton('Connect / acquire') #, self)
        self.start_button.setToolTip('Connect to first available device and start image acquisition')
        self.start_button.setFont(bold_font)
        self.start_button.clicked.connect(self.on_click_start)
        self.buttonbox.addWidget(self.start_button)

        self.stop_button = QtWidgets.QPushButton('Stop') #, self)
        self.stop_button.setToolTip('Suspend image acquisition')
        self.stop_button.setFont(bold_font)
        self.stop_button.setEnabled(False)
        self.stop_button.clicked.connect(self.on_click_stop)
        self.buttonbox.addWidget(self.stop_button)

        self.mode_pass_button = QtWidgets.QPushButton('Mode: passthrough') #, self)
        self.mode_pass_button.setToolTip('Set pass-through mode')
        self.mode_pass_button.clicked.connect(self.on_click_mode_pass)
        self.buttonbox.addWidget(self.mode_pass_button)

        self.mode_rect_button = QtWidgets.QPushButton('Mode: rectify') #, self)
        self.mode_rect_button.setToolTip('Set rectification mode')
        self.mode_rect_button.clicked.connect(self.on_click_mode_rect)
        self.buttonbox.addWidget(self.mode_rect_button)

        self.mode_stereo_button = QtWidgets.QPushButton('Mode: stereo') #, self)
        self.mode_stereo_button.setToolTip('Set stereo mode')
        self.mode_stereo_button.clicked.connect(self.on_click_mode_stereo)
        self.buttonbox.addWidget(self.mode_stereo_button)

        self.statusBar().showMessage('No device opened.')
        self.show()

    @QtCore.pyqtSlot()
    def activated_zoomcombo(self):
        self.downscale_factor = self.zoomcombo.itemData(self.zoomcombo.currentIndex())
        self.resized = True

    @QtCore.pyqtSlot()
    def activated_exaggeratecombo(self):
        self.exaggerate_depth = self.exaggeratecombo.itemData(self.exaggeratecombo.currentIndex())

    @QtCore.pyqtSlot()
    def toggled_mask(self):
        self.mask_undefined_depth = self.checkbox_mask.isChecked()

    @QtCore.pyqtSlot()
    def toggled_exaggerate(self):
        self.exaggerate_depth = self.checkbox_exaggerate.isChecked()

    @QtCore.pyqtSlot()
    def on_click_mode_pass(self):
        self.init_transfer_if_needed()
        self.parameters.set_operation_mode(OperationMode.PASS_THROUGH)

    @QtCore.pyqtSlot()
    def on_click_mode_rect(self):
        self.init_transfer_if_needed()
        self.parameters.set_operation_mode(OperationMode.RECTIFY)

    @QtCore.pyqtSlot()
    def on_click_mode_stereo(self):
        self.init_transfer_if_needed()
        self.parameters.set_operation_mode(OperationMode.STEREO_MATCHING)

    @QtCore.pyqtSlot()
    def on_click_stop(self):
        if self.timer is not None:
            self.start_button.setEnabled(True)
            self.stop_button.setEnabled(False)
            self.timer.stop()
            self.timer = None

    @QtCore.pyqtSlot()
    def on_click_start(self):
        if self.timer is None:
            self.start_button.setEnabled(False)
            self.stop_button.setEnabled(True)
            self.fps_time = time.time()
            self.frames = 0
            self.last_frames = 0
            self.timer = QtCore.QTimer(self)
            self.timer.timeout.connect(self.get_and_update_image_set)
            self.timer.start(10)

    def init_transfer_if_needed(self):
        if self.transfer is not None:
            return
        de = visiontransfer.DeviceEnumeration()
        devices = de.discover_devices()
        if len(devices) < 1:
            print('No devices found')
            sys.exit(1)

        # Just connect to the first one in this example
        device = devices[0]

        # Obtain parameter server connection
        self.parameters = visiontransfer.DeviceParameters(device)

        self.statusBar().showMessage('Connecting ip=%s'%(device.get_ip_address()))
        # Static status text (shown later with current fps appended)
        self.statustext = "Connected to %s (%s, %s, %s)"%(
            device.get_ip_address(),
            device.get_model().name,
            device.get_network_protocol().name[9:],
            ("jumbo frames" if device.get_status().get_jumbo_frames_enabled() else "standard MTU"),
            )

        # Obtain image server connection
        self.transfer = visiontransfer.AsyncTransfer(device)

    def poll_image_set(self):
        try:
            self.init_transfer_if_needed()
            image_set = self.transfer.collect_received_image_set()
        except:
            print(traceback.format_exc())
            sys.exit(1)
        # update fps
        self.frames += 1
        t = time.time()
        elapsed = t - self.fps_time
        if elapsed >= 1.0:
            f = self.frames - self.last_frames
            self.last_frames = self.frames
            self.fps_time = t
            fps = 1.0 * f / elapsed
            self.statusBar().showMessage(self.statustext+'    FPS: %0.1f'%fps)

        # get image data, forcing any 12-bit channels to be resampled to 8bit for displaying here
        images = [image_set.get_pixel_data(idx, force8bit=True) for idx in range(image_set.get_number_of_images())]
        return image_set, images

    def get_and_update_image_set(self):
        imgset, imgs = self.poll_image_set()

        width, height = imgset.get_width(), imgset.get_height()
        imgformat = []
        for i in range(imgset.get_number_of_images()):
            imgformat.append(imgset.get_pixel_format(i))
        format_description = ' / '.join([f.name[7:] for f in imgformat])
        self.formatlabel.setText(f'  {width}x{height}    {format_description}')

        # Show all received images
        for i in range(imgset.get_number_of_images()):
            # Perform any remasking and depth exaggeration in case of disparity channel
            if i == imgset.get_index_of(ImageType.IMAGE_DISPARITY):
                # NB: original was 12 bit, we already have 8 bit data due to force8bit (above)
                invalid_mask_value = 0  # black
                msk = imgs[i].copy()
                if self.exaggerate_depth > 1:
                    imgs[i][msk > (255//self.exaggerate_depth)] = 254
                    imgs[i][msk < 128] *= self.exaggerate_depth
                if self.mask_undefined_depth:
                    imgs[i][msk > 254] = invalid_mask_value
            qt_format = QtGui.QImage.Format_Grayscale8 if len(imgs[i].shape)==2 else QtGui.QImage.Format_RGB888
            imgw, imgh = imgs[i].shape[1] // self.downscale_factor, imgs[i].shape[0] // self.downscale_factor
            imgdata = imgs[i].data if self.downscale_factor<2 else \
                    imgs[i].__getitem__((slice(0,-1,self.downscale_factor), slice(0,-1,self.downscale_factor))).copy().data
            qImg = QtGui.QPixmap(QtGui.QImage(imgdata, imgw, imgh, qt_format))
            self.images[i].setPixmap(qImg)

        # Hide the other frames
        for i in range(imgset.get_number_of_images(), len(self.images)):
            qImg = QtGui.QPixmap(QtGui.QImage(bytes([127]*8), 1, 1, QtGui.QImage.Format_Grayscale8))
            self.images[i].setPixmap(qImg)

        if self.resized or self.last_number_of_images != imgset.get_number_of_images():
            self.setFixedSize(self.sizeHint())
            self.resized = False

        self.last_number_of_images = imgset.get_number_of_images()

if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    ex = App()
    sys.exit(app.exec_())

