Vision Transfer Library 10.3.0
-----------------------------

The given library provides functionally for receiving and transmitting
image pairs over a computer network. The intended use for this library
is to receive output data from Nerian's 3D camera systems. However, the
library also provides functionality for transmitting image data. It can
thus be used for emulating a camera system when performing systems
development, or for transmitting image data to Nerian's SceneScan
system when using network image input.

Images can be transferred with a bit depth of either 8 or 12 bits. When
receiving 12-bit images, the library inflates the images internally to
16 bits, in order to allow for more efficient processing. Monochrome
and RGB images are supported.

When receiving data from a camera system, the first image of an image
set is typically the rectified image of the left camera, with a bit
depth of 8 or 12 bits. The second image is typically a disparity map
with subpixel resolution, which is transmitted as a 12-bit image. Each
value in the disparity map has to be divided by 16 in order to receive
disparities at the correct scale.

There exist three possible ways for receiving and transmitting image
pairs:

* `visiontransfer::AsyncTransfer` allows for the asynchronous reception
   or transmission of image pairs. This class creates one or more
   threads that handle all network communication.

* `visiontransfer::ImageTransfer` opens up a network socket for sending
   and receiving image pairs. This class is single-threaded and will
   thus block when receiving or transmitting data.

* `visiontransfer::ImageProtocol` is the most low-level interface. This
   class allows for the encoding and decoding of image pairs to / from
   network messages. You will have to handle all network communication
   yourself.


In order to discover connected devices on the network, the class
`visiontransfer::DeviceEnumeration` can be used, which scans for
available devices and returns a list of `visiontransfer::DeviceInfo`
objects. Such a `visiontransfer::DeviceInfo` object can be used for
instantiating `visiontransfer::ImageTransfer` or
`visiontransfer::AsyncTransfer`.

A separate network protocol is used for reading and writing device
parameters. This protocol is implemented by
`visiontransfer::DeviceParameters`. Any parameters that are changed
through this protocol will be reset if the device is rebooted or if the
user makes a parameter change through the web interface.

The library further includes the class `visiontransfer::Reconstruct3D`,
which can be used for transforming a received disparity map into a set
of 3D points.

Available Examples
------------------

| File name                              | Description                                                                    |
|----------------------------------------|--------------------------------------------------------------------------------|
| `asynctransfer_example.cpp`            | Demonstration of asynchroneous transfers with `visiontransfer::AsyncTransfer`. |
| `imagetransfer_example.cpp`            | Demonstration of synchroneous transfers with `visiontransfer::ImageTransfer`.  |
| `imu_data_channel_example.cpp`         | Shows how to receive IMU data.                                                 |
| `imagetransfer_example.cpp`            | Shows how to transfer input image data to a device that supports this mode.    |
| `open3d_example.cpp`                   | Shows how to convert a disparity map to a Open3D pointcloud.                   |
| `opencv_example.cpp`                   | Shows how to convert an ImagePair to OpenCV images.                            |
| `parameter_enumeration_example.cpp`    | Shows how to enumerate available device parameters.                            |
| `parameter_example.cpp`                | Shows how to read and write device parameters.                                 |
| `pcl_example.cpp`                      | Shows how to convert a disparity map to a PCL point cloud                      |
| `server_example.cpp`                   | Shows how to create a server that acts like a SceneScan device.                |
| `reconstruct3d_example.cpp`            | Shows how to generate pointclouds from a disparity map                         |
| `temperature_example.cpp`              | Shows how to read device temperatures.                                         |


[Changelog](CHANGELOG.md)
