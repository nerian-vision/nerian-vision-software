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
# Minimal console pickle / unpickle example
#

MAX_IMAGES = 1

import sys
import time

import pickle
import numpy as np

import visiontransfer

if __name__=='__main__':

    if len(sys.argv) > 1:
        device = sys.argv[1]
        print('Manually specified device address:', device)
    else:
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

    idx = 0
    while True:
        if idx>=MAX_IMAGES: break
        image_set_orig = transfer.collect_received_image_set()

        print('Received an image set')

        # Use case 1: Preserve an ImageSet locally by cloning it (unaffected by C++ API)
        image_set = image_set_orig.copy()  # (just for reference, not used below; but you could)

        # Use case 2: Serialize / deserialize an ImageSet using pickle
        filename = 'pickled_imageset.p'
        with open(filename, 'wb') as f:
            pickle.dump(image_set_orig, f)
            print('Pickled an ImageSet as', filename)

        # (Load and verify)
        with open(filename, 'rb') as f:
            loaded = pickle.load(f)
            print('Unpickled an ImageSet with', loaded.get_number_of_images(), 'images')
            print(' Resolution', loaded.get_width(), 'x' , loaded.get_height())
            for i in range(loaded.get_number_of_images()):
                print(' Image', i, ' identical to original? ', np.all(image_set_orig.get_pixel_data(i) == loaded.get_pixel_data(i)))

        idx += 1

