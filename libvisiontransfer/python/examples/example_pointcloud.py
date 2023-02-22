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
# Highlights usage of Reconstruct3D to calculate a point cloud.
# Grabs one single image set and shows a scatter plot of the data.
#

# matplotlib's scatter() isn't fast; show only every n-th point
THINNING = 30

import sys

import visiontransfer

try:
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D
except:
    print("\n---\nThis examples requires matplotlib (python3-matplotlib)!\n---\n")
    raise

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
    image_set = transfer.collect_received_image_set()

    print('Received an image set, reconstructing point cloud ...')
    # Generator for point cloud data from images
    rec3d = visiontransfer.Reconstruct3D()
    pointcloud = rec3d.create_point_map(image_set, max_z=10)

    print('Saving CSV and PLY ...')
    # Also save the point cloud data as CSV ...
    with open('pointcloud_00.csv', 'w') as f:
        for point in pointcloud:
            f.write(f"{point[0]:.5f},{point[1]:.5f},{point[2]:.5f}\n")

    # ... and directly as PLY, via the API
    rec3d.write_ply_file('pointcloud_00.ply', image_set, max_z=10.0, binary=False)

    print('Preprocessing ...')
    # Thinning and minimal z reordering
    reduced_data = sorted(pointcloud[::THINNING], key=lambda e: -e[2])
    # Convert to three separate x, y, z lists, reordering and flipping axes
    data = [list(flip * p[idx] for p in reduced_data) for idx, flip in [[0,1], [2,1], [1,-1]]]

    # Show interactive plot
    print('Showing point cloud ...')
    my_palette = plt.cm.gist_ncar_r
    fig = plt.figure()
    fig.canvas.set_window_title("Point cloud (drag to rotate)")
    ax = fig.add_subplot(111, projection='3d')
    ax.scatter(*data, c=data[1], marker='.', cmap=my_palette)
    plt.show()


