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

// PCL headers must be included first!
#include <pcl/pcl_base.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/filters/extract_indices.h>

#include <visiontransfer/deviceenumeration.h>
#include <visiontransfer/imagetransfer.h>
#include <visiontransfer/imageset.h>
#include <visiontransfer/reconstruct3d.h>
#include <iostream>
#include <exception>
#include <stdio.h>

#ifdef _MSC_VER
// Visual studio does not come with snprintf
#define snprintf _snprintf_s
#endif

using namespace visiontransfer;

#define RGB_CLOUD // Undefine for monochrome cameras

#ifdef RGB_CLOUD
    typedef pcl::PointXYZRGB PclPointType;
#else
    typedef pcl::PointXYZI PclCloudType;
#endif

int main() {
    try {
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

        // Receive one image
        std::cout << "Receiving image set ..." << std::endl;
        ImageSet imageSet;
        while(!imageTransfer.receiveImageSet(imageSet)) {
        }

        // Project to PCL point cloud
        Reconstruct3D recon3d;
        pcl::PointCloud<PclPointType>::Ptr cloud;
#ifdef RGB_CLOUD
        cloud = recon3d.createXYZRGBCloud(imageSet, "world");
#else
        cloud = recon3d.createXYZICloud(imageSet, "world");
#endif

        // Extract all points up to 5 meters
        pcl::PointIndices::Ptr inliers(new pcl::PointIndices());
        for (unsigned int i = 0; i < cloud->size(); i++) {
            if(cloud->points[i].z < 5) {
                inliers->indices.push_back(i);
            }
        }
        pcl::ExtractIndices<PclPointType> extract;
        extract.setInputCloud(cloud);
        extract.setIndices(inliers);
        extract.filterDirectly(cloud);

        // Write point cloud
        pcl::io::savePCDFile("cloud.pcd", *cloud, false);
        std::cout << "Written point cloud to cloud.pcd" << std::endl;

    } catch(const std::exception& ex) {
        std::cerr << "Exception occurred: " << ex.what() << std::endl;
    }

    return 0;
}
