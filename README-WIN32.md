Vision Software Binary Release 1.0.0 for Windows
=============================================

Included components
--------------------

This software release comprises the following components:

* `libvisiontransfer`: A C++ API for communicating with Allied Vision's
  stereo vision sensor systems.

* `API documentation`: Reference documentation for the libvisiontransfer
  API.

* `NVCom`: A client application that provides a live display of the
  depth data that is delivered by Allied Vision's devices. This application
  depends on `libvisiontransfer`.

* `GenTL producer`: A GenICam GenTL compatible producer module. Use this
  module for integrating Allied Vision's devices into existing applications that
  act as GenTL consumers, such as HALCON or MATLAB.

* `Examples`: Programming examples for using the libvisiontransfer API
  in C++ and for using the GenTL producer with H-Develop or MATLAB.

* `Python`: Precompiled libvisiontransfer Python wrapper (egg and wheel)
  for 64-bit Python 3; use pip install to install the .whl file.
  Python examples (included) may require additional external modules.

Source code for all applications and libraries can be obtained from the
Allied Vision website.

Further Information
-------------------

For further information on using the API, the NVCom application or the
GenICam GenTL producer, please refer to the appropriate sections in the
user manual.

Contact
-------

For further information please contact Allied Vision Technologies GmbH at:

Email: info (at) alliedvision.com  
Website: https://www.alliedvision.com

Address:  
Allied Vision Technologies GmbH  
Taschenweg 2a  
07646 Stadtroda  
Germany
 