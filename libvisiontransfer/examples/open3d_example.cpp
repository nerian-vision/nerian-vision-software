/*******************************************************************************
 * Copyright (c) 2022 Nerian Vision GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *******************************************************************************/


#define _USE_MATH_DEFINES
#include <cmath>

// Open3D headers must be included before visiontransfer!
#include <open3d/Open3D.h>
#include <visiontransfer/deviceenumeration.h>
#include <visiontransfer/imagetransfer.h>
#include <visiontransfer/imageset.h>
#include <visiontransfer/reconstruct3d.h>
#include <iostream>
#include <exception>
#include <memory>
#include <stdio.h>

#ifdef _MSC_VER
// Visual studio does not come with snprintf
#define snprintf _snprintf_s
#endif

using namespace visiontransfer;

int main(int argc, char** argv) {
    try {
        if(argc != 2) {
            std::cout
                << "Usage: " << argv[0] << " MODE" << std::endl
                << std::endl
                << "Available modes:" << std::endl
                << "0: Display RGBD image" << std::endl
                << "1: Display 3d pointcloud" << std::endl;
            return 1;
        }
        bool displayPointcloud = (bool)atoi(argv[1]);

        // Search for Nerian stereo devices
        DeviceEnumeration deviceEnum;
        DeviceEnumeration::DeviceList devices = deviceEnum.discoverDevices();
        if(devices.size() == 0) {
            std::cout << "No devices discovered!" << std::endl;
            return -1;
        }

        // Create an image transfer object that receives data from
        // the first detected device
        ImageTransfer imageTransfer(devices[0]);

        // Initialize objects that will be needed later
        ImageSet imageSet;
        Reconstruct3D recon3d;
        open3d::geometry::AxisAlignedBoundingBox bbox(Eigen::Vector3d(-10, -10, 0),
            Eigen::Vector3d(10, 10, 5));
        std::shared_ptr<open3d::geometry::PointCloud> cloud(new open3d::geometry::PointCloud);
        std::shared_ptr<open3d::geometry::RGBDImage> rgbdImage(new open3d::geometry::RGBDImage);

        // Start viewing
        open3d::visualization::Visualizer vis;
        vis.CreateVisualizerWindow("Nerian 3D viewer", 1280, 768);
        vis.PrintVisualizerHelp();
        vis.GetRenderOption().SetPointSize(2);

        // Keep receiving new frames in a loop
        while(true) {
            // Grab frame
            while(!imageTransfer.receiveImageSet(imageSet)) {
                if(!vis.PollEvents()) {
                    return 0;
                }
            }

            if(displayPointcloud) {
                // Crop pointcloud
                (*cloud) = *recon3d.createOpen3DCloud(imageSet)->Crop(bbox);

                // Rotate for visualization
                cloud->Transform(Eigen::Affine3d(Eigen::AngleAxis<double>(M_PI, Eigen::Vector3d(1.0, 0.0, 0.0))).matrix());
            } else {
                // Get RGBD Frame
                (*rgbdImage) = *recon3d.createOpen3DImageRGBD(imageSet);
            }

            // Visualize
            if(!vis.HasGeometry()) {
                if(displayPointcloud) {
                    vis.AddGeometry(cloud);
                } else {
                    vis.AddGeometry(rgbdImage);
                }
            }

            vis.UpdateGeometry();
            if(!vis.PollEvents()) {
                return 0;
            }

            vis.UpdateRender();
        }

    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}
