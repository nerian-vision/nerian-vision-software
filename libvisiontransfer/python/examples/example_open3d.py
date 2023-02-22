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
# Example for using visiontransfer data to populate an Open3D point cloud
#

try:
    import open3d
    import open3d.visualization as vis
except:
    print("\n---\nThis examples requires open3d!\n---\n")
    raise

import numpy as np
import sys

import visiontransfer

if __name__=='__main__':
    device_enum = visiontransfer.DeviceEnumeration()
    devices = device_enum.discover_devices()
    if len(devices) < 1:
        print('No devices found')
        sys.exit(1)

    # Generator for point cloud data from images
    rec3d = visiontransfer.Reconstruct3D()

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
    image_set = transfer.collect_received_image_set()
    print('Received an image set')

    # One-liner to convert to Open3D point cloud
    print('Calculating point cloud ...')
    pcd = rec3d.create_open3d_pointcloud(image_set, max_z=10)

    # RGBD generation for completeness sake; not used below
    print('Calculating RGBD image ...')
    rgbd = rec3d.create_open3d_rgbd_image(image_set, depth_trunc=10)

    # Open3D point cloud viewer
    vis = open3d.visualization.Visualizer()
    vis.create_window(window_name='Nerian visiontransfer and Open3D Python', width=800, height=600)
    vis.add_geometry(pcd)
    ctl = vis.get_view_control()
    ctl.set_zoom(0.1)
    ctl.set_front([0, 0, -1])
    ctl.set_lookat([0, 0, 1])
    ctl.set_up([0, -1, 0])
    vis.run()

 
