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
# Minimal console acquisition example, using OpenCV to save images
#

# Number of image sets to acquire and save
MAX_IMAGES = 1

try:
    import cv2
except:
    print("\n---\nThis examples requires cv2 (python3-opencv)!\n---\n")
    raise

import sys
import time

import visiontransfer

if __name__=='__main__':
    device_enum = visiontransfer.DeviceEnumeration()
    devices = device_enum.discover_devices()
    if len(devices) < 1:
        print('No devices found')
        sys.exit(1)

    print('Found these devices:')
    for i, info in enumerate(devices):
        print(f'  {i+1}: {info}')
    selected_device = 0 if len(devices)==1 else (int(input('Device to open: ') or '1')-1)
    print(f'Selected device #{selected_device+1}')
    device = devices[selected_device]

    print('Ask parameter server to set stereo mode ...')
    params = visiontransfer.DeviceParameters(device)
    params.set_operation_mode(visiontransfer.OperationMode.STEREO_MATCHING)

    print('Starting acquisition ...')
    transfer = visiontransfer.AsyncTransfer(device)

    i = 0
    while True:
        if i>=MAX_IMAGES: break
        image_set = transfer.collect_received_image_set()
        print('Received an image set, saving as channel?_' + ('%02d'%i) + '.png')
        for ch in range(image_set.get_number_of_images()):
            imgdata = image_set.get_pixel_data(ch, force8bit=True)
            if len(imgdata.shape)==3:
                # Color image: our API uses RGB, OpenCV uses BGR
                cv2.imwrite(f'channel{ch}_'+('%02d'%i)+'.png', cv2.cvtColor(imgdata, cv2.COLOR_RGB2BGR))
            else:
                cv2.imwrite(f'channel{ch}_'+('%02d'%i)+'.png', imgdata)
        i += 1

