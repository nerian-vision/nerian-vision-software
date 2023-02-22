Vision Software Release
==============================

Overview
--------

This software release comprises the following subprojects:

* `libvisiontransfer`: A C++ API for communicating with Nerian's
  stereo vision sensor systems.

* `libvisiontransfer/python`: Cython-based Python wrapper of
  libvisiontransfer for Python 3; contains Python examples.

* `nvcom`: A client application that provides a live display of the
  depth data that is delivered by Nerian's devices. This application
  depends on `libvisiontransfer`.

* `gentl-producer`: A GenICam GenTL compatible producer module. Use this
  module for integrating Nerian's devices into existing applications that
  act as GenTL consumers, such as HALCON or MATLAB.

Please see the respective subdirectories for further documentation.


Installation
------------

Please use CMake to compile the software for Linux. If you are using
Windows, then we recommend to download a pre-compiled binary release.
If you are compiling NVCom, please make sure that you have the
libraries OpenCV and Qt installed.

You can build each submodule separately from their respective
subdirectories. To build all software at once, please enter the
following commands in the top-level directory:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

After compilation you can use the applications and libraries directly
from the created subdirectories `bin/` and `lib/`. Alternatively you
can perform a system wide installation. For installation please use the
following command:

    $ sudo make install

Optional Python module:
Note that the extension will only be built if the dependencies are met:
Python 3 and "pip install Cython numpy wheel" may be required first.
Use easy-install then to install the egg inside python3-egg, or use
pip/pip3 on modern installations to install the extension to your
user directory.

    $ cd python3-wheel
    $ pip install *.whl

Optional Open3D support for NVCom:
If Open3D is not installed in the default search paths, the internal
3D point cloud display mode in NVCom is disabled.
For Open3D >=0.15 you must include the following options in its CMake
config to build a working library (either shared or static) for NVCom:

    -DGLIBCXX_USE_CXX11_ABI=ON -DBUILD_GUI=OFF
    -DBUILD_WEBRTC=OFF -DENABLE_HEADLESS_RENDERING=ON

Contact
-------

For further information please contact Nerian Vision GmbH at:

Email: service (at) nerian.com  
Website: https://nerian.com

Address:  
Nerian Vision GmbH  
Zettachring 2  
70567 Stuttgart  
Germany
