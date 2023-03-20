Changelog for Nerian Vision Software Release
============================================

10.4.2 (2023-03-20)
-------------------

* API: fixed issue with remote parameter write lock

10.4.1 (2023-03-16)
-------------------

* API: fixed potential issue obtaining remote parameter enumeration

10.4.0 (2023-03-10)
-------------------
* GenTL producer
    * Exposure, Gain and Balance (if applicable) modifiable via features
    * Point cloud generation can be toggled via ComponentEnable[Range]
    * New multi-part stream layout, capable of extra channel (Ruby)
    * Custom IntensitySource feature to select primary camera (Ruby)
    * FeatureInvalidate events for remotely changed features
    * Stability fixes and updated Halcon example

10.3.0 (2022-12-19)
-------------------
* Fixed parameter server reconnection (Windows clients)
* Improved PCL example
* Added version number to libvisiontransfer headers
* Enabled SSE2 for Windows builds
* NVCom: reduced 3D visualization jerkiness in cases of slow rendering
* NVCom: added capture options for Ruby color images

10.2.0 (2022-08-25)
-------------------
* Added new software trigger functionality
* Added automatic re-connection to parameter API
* Added missing VCOMP dll file to Windows installer
* Added standard deviation display to NVCom
* Updated C++ parameter example program

10.1.0 (2022-08-01)
-------------------
* NVCom
    * Improved legibility for 3D coordinate tooltip when zoomed out
* API (Python)
    * Extended device parameter access, with new example script
    * Improved documentation
* Improved compatibility with Ruby

10.0.0 (2022-06-30)
-------------------
* New parameter protocol
    * Significant extension of changeable parameters
    * Parameter changes are immediately reflected in SceneScan / Scarlet
      web-interface
    * No API changes required
    * Requires SceneScan firmware > 9.0.0 or Scarlet firmware >= 3.0.0
* Windows: binary packages for all current Python versions
* Build fix for Ubuntu 22.04
* Stability improvements

9.0.5 (2022-03-07)
------------------
* GenTL Producer
    * Compatibility fixes for Matrox software

9.0.4 (2022-03-02)
------------------
* NVCom
    * Fixed send order for input images for windows version

* GenTL Producer
    * Minor fix to buffer purpose IDs

9.0.3 (2022-02-23)
------------------
* NVCom
    * Support for sending color input images
    * Flow control for sending input images

* API
    * Fixed wrong timestamps in IMU data
    * Stability improvements for TCP
    * Example program for sending input images

* GenTL Producer
    * Support for GenTL 1.6
    * Fixes to Scan3D parameters

9.0.2 (2022-01-05)
------------------
* NVCom
    * Fixed sending of input images
* API
    * Stability fix to network protocol
    * Support for copy and pickle in python API
    * Build fix for some GCC versions
    * Support for larger maximum disparity in 3D reconstruction

9.0.1 (2021-08-13)
------------------
* API
    * Fixed potential crash on image resolution change
    * Better handling of protocol exceptions

* NVcom
    * Fixed point cloud being displayed upside-down
    * Fixed point-cloud display error if left image is not available
    * Several minor bugfixes

9.0.0 (2021-08-10)
------------------
* API
    * Support for Open3D
    * New function for converting disparity to depth map

* NVCom
    * Live 3D display
    * Automatic color range adjustment
    * More versatile capture options
    * 12 bit to 16 bit conversion for capturing

8.3.0 (2020-12-03)
------------------
* Improved support for Scarlet
* API
    * Renamed SceneScanParameters to DeviceParameters
    * Improved data transmission for unreliable networks
    * Fixed possible crash with std::bad_alloc exception

8.2.1 (2020-09-16)
------------------
* API
    * Added new trigger mode parameters: constant on/off
    * Bugfixes to c++ examples

8.2.0 (2020-08-19)
------------------
* API
    * Transferring of exposure time with each image set
    * Transferring of last sync-pulse timestamp with each image set

8.1.1 (2020-08-07)
------------------
* Fixed bug in NVCom that prevented capturing of frames in some
  configurations

8.1.0 (2020-07-29)
------------------
* Support for 1 to 3 images in result set
* ImagePair renamed to ImageSet
* Improved error handling for GenTL producer

8.0.0 (2020-07-01)
------------------
* New python API
* Major network protocol changes (not backwards compatible)

7.2.1 (2020-04-14)
------------------
* Increased number of configurable pulse widths to 8

7.2.0 (2020-02-20)
------------------
* API
    * Support for new trigger cycling and inversion parameters
    * Support for 3D reprojection of disparity maps with odd image sizes

7.1.0 (2019-11-20)
------------------

* NVCom
    * Live device status and health field in device list
    * "Configure" button to open selected device in browser

* GenTL Producer
    * Better compliance; reporting of more Scan3d parameters

* API
    * Added subpixel ROI settings to parameter API
    * New DataChannel API for miscellaneous sensors;
      integration of Inertial Motion Unit [not on current SceneScan]
    * Modified function names 'getSaveAutoRecalibration'
      and 'getSaveAutoRecalibration' to remove typos
    * Clarifications in API documentation / example code
    * All SceneScan network port definitions centralized into
      libvisiontransfer/internalinformation.h
    * Fix for processes with many open file descriptors (Linux)
    * Fixed device enumeration (Windows)
    * Fixed Debug profile build (Windows)
    * Fixed forward compatibility of parameter API (whitelisting and
      warnings, but no exceptions for unknown parameters)

7.0.0 (2019-08-14)
------------------
* Support for image resolutions up to 6 megapixels (requires SceneScan
  firmware >= 4.0.0)
* GenTL Producer improvements:
    * Fixed ComponentSelector influence on PixelFormat, ComponentEnable/ID
    * Added Scan3d features including camera baseline and focal length
    * Clamping non-finite values to a large value (Scan3dInvalidDataValue)
      as recommended by GenICam SFNC
* Added edge dependent SGM penalty parameters to parameter protocol (
  requires SceneScan firmware >= 4.0.0)
* Added support for upcoming Karmin3 camera
* HALCON example: RGB images correctly shown

6.5.0 (2019-06-19)
------------------
* Network protocol changes (not backward compatible to older versions)
* New API for parameter enumeration
* Fixed potential buffer overflow in network protocol
* Fixed interface for sending raw transfers
* Parameter API: Added support for multiple speckle filter iterations

6.4.0 (2019-02-04)
------------------
* Added new example for Matrox MIL
* Code signing for all Windows executables
* Minor build fixes

6.3.0 (2019-01-15)
------------------
* New protocol for reading and writing parameters
* Moved libvisiontransfer to its own namespace
* Fixed binary compatibility for Windows debug builds

6.2.2 (2018-11-13)
------------------
* Added debug libraries to Windows binary release

6.2.1 (2018-11-30)
------------------
* Added support for colored PCL point clouds
* Fixed reception of large RGB images

6.2.0 (2018-11-15)
------------------
* Added support for writing PLY files with 12 bit input images
* Added support for writing binary PLY files to NVCom
* Fixed possible error message regarding buffer sizes when establishing
  a new connection.
* Fixed possible projection of invalid points when writing PLY files

6.1.1 (2018-08-17)
------------------
* Fixed crash in NVCom when writing images in non-graphical mode

6.1.0 (2018-08-08)
------------------
* Added conversion to PCL point clouds to Reconstruct3D
* Added conversion to OpenCV images to ImagePair
* Added examples for PCL and OpenCV conversion
* Minor user interface improvements to NVCom

6.0.2 (2018-07-30)
------------------
* Fixed erroneous projection of single points in Reconstruct3D and for
  NVCom's coordinate display

6.0.1 (2018-07-15)
------------------
* Compatibility fixes to GenTL producer for Halcon 18.05

6.0.0 (2018-06-07)
------------------
* New network protocol (not backwards compatible; requires firmware >= 2.0.0)
* Dropped support for SP1
* Discovery of devices on the local network
* Re-transmission of UPD packets in case of packet loss
* Added example for server application with libvisiontransfer
* Added dropped frames display to NVCom
* Compatibility improvements to GenTL producer

5.2.0 (2018-03-29)
------------------
* NVCom improvements:
    * New tool for displaying 3D coordinates
    * Added support for sending 12-bit image data
    * Fixed possible crash in full screen mode
* libvisiontransfer improvements:
    * Fixed infinite timeouts in ImageTransfer
    * Added deep copy functionality for ImagePair
* GenTL producer
    * Fixed path installation on windows to be compatible
      with other GentL producers

5.1.0 (2018-02-01)
------------------
* Added functionality for time stamp recording in NVCom
* Improved reliability of input image transmission in NVCom
* Added support for binary PLY files
* Fixed destruction of ImagePair while writing PLY files
* TCP streams no longer require a client port

5.0.1 (2017-10-18)
------------------
* Fixed synchronization issue in AsyncTransfer
* Fixed PGM output on Windows for ImagePair
* Fixed image data modifications during PGM output

5.0.0 (2017-09-29)
------------------
* Added support for SceneScan
* Added support for Jumbo Frames
* Support for 12-bit image data
* Renamed to Vision Software Release
* Renamed SpCom to NVCom
* Fixed possible display stall bug in NVCom

4.1.5 (2017-05-07)
------------------
* Added application icon to SpCom
* Compatibility fixes to GenTL producer
* New sample data for GenTL producer test builds

4.1.4 (2017-04-12)
------------------
* Added example program for MATLAB
* Fixed reading / writing of paths with non-ASCII characters with SpCom
  on Windows
* Allow recording of more than 10,000 frames in SpCom

4.1.3 (2017-03-30)
------------------
* Compatibility fixes to GenTL producer. Now compatible with MATLAB

4.1.2 (2017-03-27)
------------------
* Fixed possible image reception stall when reconfiguring SP1
* Removed restrictions to image size when compiling with SSE or AVX2

4.1.1 (2017-03-09)
------------------
* Fixed critical bug that causes image reception problems on Windows 10

4.1.0 (2017-02-15)
------------------
* Fixed erroneous subpixel decoding of disparity maps
* GenTL producer now supports multi-part data streams
* GenTL producer now has a dedicated virtual device for right camera images
* Added an example for HALCON 13
* Both HALCON examples now visualize the 3D point cloud
* SpCom build fix for older CMake versions

4.0.0 (2016-01-17)
------------------
* New network protocol for SP1 firmware 3.0.0
* Support for variable disparity range
* Added zoom and fullscreen support to SpCom
* Fixed display stall bug in SpCom
* Build fixes for ARM

3.0.1 (2016-10-13)
------------------
* Fixed build problem for some Linux systems

3.0.0 (2016-10-07)
------------------
* New protocol with lower performance overhead
* Support for larger image resolutions
* Qt GUI for SpCom
* SpCom supports saving of 3D point clouds
* New color coding scheme for SpCom
* Zoom functionality for SpCom

2.1.7 (2016-07-08)
------------------
* Compatibility fix for non-AVX CPUs for binary windows builds

2.1.6 (2016-05-18)
------------------
* Fixed possible receive buffer overflow in libvisiontransfer
* Build-fix for cross compilation of libvisiontransfer

2.1.5 (2016-03-17)
------------------
* Improved parallelization for AsyncTransfer in libvisiontransfer
* Fixed possible duplicate write of captured frames in spcom
* Fixed possible problem with captured file numbering in spcom

2.1.4 (2016-03-11)
------------------
* Fixed network exceptions on Windows 10
* Lazy initialization of receive thread in libvisiontransfer

2.1.3 (2016-03-07)
------------------
* Improved installation procedure for Windows and Linux

2.1.2 (2016-02-12)
------------------
* Improved parallelism for libvisiontransfer
* Fixed inaccurate frame rate display of spcom for Windows builds

2.1.1 (2016-01-12)
------------------
* Fix for image widths that are not a multiple of 64

2.1.0 (2015-12-08)
-----
* Added a GenICam GenTL producer that encapsulates libvisiontransfer
* Minor compilation fixes

2.0.0 (2015-11-23)
------------------
* Major clean-up and simplification of all interfaces
* Image pairs are now stored in the new class ImagePair
* Transmission / reception of q-Matrix, timestamp and sequence number
* Right image can now have 8-bit color depth
* Support for image reception and transmission on the same connection
* Several network optimizations
* Internal refactoring
