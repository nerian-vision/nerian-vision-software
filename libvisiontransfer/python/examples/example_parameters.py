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
# Device parameter access
#
# (Connects immediately if exactly one device is found)
#

import sys
import time
import traceback

import visiontransfer
from visiontransfer import OperationMode, ImageFormat, ImageType

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

    #
    # Obtain parameter server connection
    #

    deviceParams = visiontransfer.DeviceParameters(device)

    #
    #  Accessing / reading parameters by UID
    #

    # Obtain the set of all current parameters
    #  NOTE: this set is a disconnected copy. Parameters are not updated
    #  on-the-fly inside this set. Setting parameters here does not work
    #  either; it should be done with set_parameter (see further down)
    paramset = deviceParams.get_parameter_set()

    # Iteration over (UID, Parameter) tuples; list available parameters:
    for uid, param in paramset:
        print(uid)
    print('-> total number of parameters:', len(paramset))

    # Print some selected parameters with different configurations; the final one does not exist
    for uid in ['operation_mode', 'number_of_disparities', 'manual_gain', 'model_name', 'calib_M_1', 'NONEXISTENT_PARAM']:
        print()
        # Check for existence (otherwise an exception would be raised)
        if uid in paramset:
            param = paramset[uid]
        else:
            print('Not getting unknown parameter:', uid)
            continue
        # Print different fields
        access = '[read/write]' if param.is_writable() else '[read-only]'
        print(param.get_uid(), access)
        print('  Name         ', param.get_name())
        print('  Type         ', param.get_type())
        mod = '  [MODIFIED]' if (param.is_modified() and param.is_writable()) else ''
        if param.is_tensor():
            print('  Tensor shape ', str(param.get_tensor_shape()))
            print('  Tensor data  ', str(param.get_tensor_data()), mod)
        else:
            print('  Current value', repr(param.get_current()), mod)
        if param.get_unit() != '':
            print('  Unit         ', param.get_unit())
        if param.get_description() != '':
            print('  Description  ', param.get_description()[:60]+('...' if len(param.get_description())>60 else ''))
        if param.has_range():
            print('  Range        ', param.get_min(), '—', param.get_max())
        if param.has_increment():
            print('  Increment    ', param.get_increment())
        if param.has_options():
            # This is an enum-style type, we will obtain the valid settings
            opt = param.get_options()
            des = param.get_option_descriptions()
            # Format and pretty print enum options
            optstr = ''
            for i in range(len(opt)):
                if i:
                    optstr += ' | '
                optstr += repr(opt[i])
                if des[i] != '':
                    optstr += ' (' + des[i] + ')'
            print('  Options      ', optstr)


    #
    #  Modifying parameters by UID
    #

    # Exception tests
    try:
        deviceParams.set_parameter('NONEXISTENT_PARAM', ':-(')
    except Exception as e:
        print('Expected failure to set parameter:', str(e))
    try:
        deviceParams.set_parameter('model_name', 'myOverride') # (read-only)
    except Exception as e:
        print('Expected failure to set parameter:', str(e))

    # Modify trigger frequency
    # Store old value so we can restore it. get_parameter is an alternative
    #  to get_parameter_set, for getting single parameters.
    fps = deviceParams.get_parameter('trigger_frequency').get_current()
    print()
    print('Reducing trigger frequency by half for 8 sec')
    # Setting parameters is requested directly on the DeviceParameters instance
    deviceParams.set_parameter('trigger_frequency', max(1, fps//2))
    time.sleep(8)
    print('Restoring trigger frequency')
    deviceParams.set_parameter('trigger_frequency', fps)

    # Parameter-specific getters and setters also exist for many parameters, see pydoc
    # deviceParams.set_operation_mode(OperationMode.STEREO_MATCHING)

