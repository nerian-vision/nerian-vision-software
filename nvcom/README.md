NVCom Client Application
========================

The NVCom application can be compiled for Windows or Linux. For Windows,
precompiled packages are also available. Before compiling NVCom, make
sure that you have the libraries OpenCV and Qt installed. If you want
to enable the live 3D display, you will also require Open3D.

NVCom provides the following features:

* Receive and display images and disparity maps from Nerian's devices
* Perform color-coding of disparity maps
* Interactive 3D display of pointclouds (requires Open3D)
* Write received data to files as images or 3D point clouds
* Transmit image pairs to SceneScan

NVCom comes with a GUI that provides access to all important functions.
More advanced features are available through command line options. The
command line options can also be used for automating data recording or
playback.

Unless NVCom is run in non-graphical mode, it opens a GUI window that
displays the received depth data. The currently displayed image pair can
be written to disk by pressing the `space` key or by clicking the camera
icon in the toolbar. When pressing the `enter` key or clicking the
recording icon, all subsequent images will be saved. When closing NVCom,
it will save its current settings, which will be automatically re-loaded
when NVCom is launched the next time.

For a list of available command line options, please run
```
nvcom --help
```

NVCom contains icons from the Tango icon theme. For more details please
see: http://tango.freedesktop.org/
