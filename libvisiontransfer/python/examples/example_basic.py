#!/usr/bin/env python3

###############################################################################/
# Copyright (c) 2024 Allied Vision Technologies GmbH
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
# Minimal console image set reception example
#

import sys
import time

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

    print('Starting acquisition ...')
    transfer = visiontransfer.AsyncTransfer(device)

    while True:
        image_set = transfer.collect_received_image_set()
        # Pretty-print the image types here for a quick glance
        image_types = ', '.join([str(image_set.get_pixel_format(i)).split('_', 1)[1] for i in range(image_set.get_number_of_images())])
        print('Received ImageSet, size', image_set.get_width(), 'x', image_set.get_height(),
              '- with', image_set.get_number_of_images(), 'images:', image_types)




