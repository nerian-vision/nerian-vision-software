Vision Software Binary Release 1.0.0 for Windows
=============================================

Included components
--------------------

This software release comprises the following components:

* `libvisiontransfer`: A C++ API for communicating with Nerian's
  stereo vision sensor systems.

* `API documentation`: Reference documentation for the libvisiontransfer
  API.

* `NVCom`: A client application that provides a live display of the
  depth data that is delivered by Nerian's devices. This application
  depends on `libvisiontransfer`.

* `GenTL producer`: A GenICam GenTL compatible producer module. Use this
  module for integrating Nerian's devices into existing applications that
  act as GenTL consumers, such as HALCON or MATLAB.

* `Examples`: Programming examples for using the libvisiontransfer API
  in C++ and for using the GenTL producer with H-Develop or MATLAB.

* `Python`: Precompiled libvisiontransfer Python wrapper (egg and wheel)
  for 64-bit Python 3; use pip install to install the .whl file.
  Python examples (included) may require additional external modules.

Source code for all applications and libraries can be obtained from the
Nerian website.

Further Information
-------------------

For further information on using the API, the NVCom application or the
GenICam GenTL producer, please refer to the appropriate sections in the
user manual.

Contact
-------

For further information please contact Nerian Vision GmbH at:

Email: service (at) nerian.com  
Website: http://nerian.com

Address:  
Nerian Vision GmbH  
Zettachring 2  
70567 Stuttgart  
Germany  

