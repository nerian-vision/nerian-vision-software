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
#   (live view with newer Open3D renderer, developed on open3d 0.13)
#

try:
    import open3d
    import open3d.visualization as vis
except:
    print("\n---\nThis examples requires open3d!\n---\n")
    raise

import numpy as np
import sys
import threading

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

    rec3d = visiontransfer.Reconstruct3D()
    pcd = open3d.geometry.PointCloud()

    vis.gui.Application.instance.initialize()
    window = vis.gui.Application.instance.create_window('img', width=640, height=480)
    widget = vis.gui.SceneWidget()
    widget.scene = vis.rendering.Open3DScene(window.renderer)
    widget.scene.set_lighting(vis.rendering.Open3DScene.LightingProfile.NO_SHADOWS, (0,0,0))
    window.add_child(widget)
    mat = vis.rendering.MaterialRecord()
    mat.shader = 'defaultUnlit'
    widget.scene.camera.look_at([0,0,1], [0,0,-1], [0,-1,0])

    def update_fn():
        widget.scene.clear_geometry()
        widget.scene.add_geometry('cloud', pcd, mat)

    running = True
    def acquisition_thread():
        while running:
            image_set = transfer.collect_received_image_set(1.0)
            if image_set is not None:
                newpcd = rec3d.create_open3d_pointcloud(image_set, max_z=10)
                pcd.points = newpcd.points
                pcd.colors = newpcd.colors
                vis.gui.Application.instance.post_to_main_thread(window, update_fn)

    thr = threading.Thread(target=acquisition_thread)
    thr.start()

    vis.gui.Application.instance.run()
    running = False


