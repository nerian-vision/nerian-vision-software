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

#ifndef VISIONTRANSFER_RECONSTRUCT3D_PCL_H
#define VISIONTRANSFER_RECONSTRUCT3D_PCL_H

#ifdef PCL_MAJOR_VERSION

namespace visiontransfer {

/*
 * PCL-specific implementations that need to be inlined in order to avoid
 * dependencies for projects that do not make use of PCL
 */

template <typename T>
typename pcl::PointCloud<T>::Ptr Reconstruct3D::initPointCloud(const ImageSet& imageSet, const char* frameId) {
    int sec, microsec;
    imageSet.getTimestamp(sec, microsec);

    typename pcl::PointCloud<T>::Ptr ret(
        new pcl::PointCloud<T>(imageSet.getWidth(), imageSet.getHeight()));

    ret->header.frame_id = frameId;
    ret->header.seq = imageSet.getSequenceNumber();
    ret->header.stamp = sec * 1000000LL + microsec;
    ret->width = imageSet.getWidth();
    ret->height = imageSet.getHeight();
    ret->is_dense = true;
    return ret;
}

inline pcl::PointCloud<pcl::PointXYZ>::Ptr Reconstruct3D::createXYZCloud(const ImageSet& imageSet,
        const char* frameId, unsigned short minDisparity) {
    float* pointMap = createPointMap(imageSet, minDisparity);
    pcl::PointCloud<pcl::PointXYZ>::Ptr ret = initPointCloud<pcl::PointXYZ>(imageSet, frameId);
    memcpy(&ret->points[0].x, pointMap, ret->width*ret->height*sizeof(float)*4);
    return ret;
}

inline pcl::PointCloud<pcl::PointXYZI>::Ptr Reconstruct3D::createXYZICloud(const ImageSet& imageSet,
        const char* frameId, unsigned short minDisparity) {
    float* pointMap = createPointMap(imageSet, minDisparity);
    pcl::PointCloud<pcl::PointXYZI>::Ptr ret = initPointCloud<pcl::PointXYZI>(imageSet, frameId);

    ImageSet::ImageType colImg = ImageSet::IMAGE_LEFT;

    pcl::PointXYZI* dstPtr = &ret->points[0];
    if(imageSet.getPixelFormat(colImg) == ImageSet::FORMAT_8_BIT_MONO) {
        for(int y = 0; y < imageSet.getHeight(); y++) {
            unsigned char* rowPtr = imageSet.getPixelData(colImg) + y*imageSet.getRowStride(colImg);
            unsigned char* endPtr = rowPtr + imageSet.getWidth();
            for(; rowPtr < endPtr; rowPtr++) {
                dstPtr->intensity = static_cast<float>(*rowPtr)/255.0F;
                dstPtr->x = *pointMap++;
                dstPtr->y = *pointMap++;
                dstPtr->z = *pointMap++;

                pointMap++;
                dstPtr++;
            }
        }
    } else if(imageSet.getPixelFormat(colImg) == ImageSet::FORMAT_12_BIT_MONO) {
        for(int y = 0; y < imageSet.getHeight(); y++) {
            unsigned short* rowPtr = reinterpret_cast<unsigned short*>(imageSet.getPixelData(colImg) + y*imageSet.getRowStride(colImg));
            unsigned short* endPtr = rowPtr + imageSet.getWidth();
            for(; rowPtr < endPtr; rowPtr++) {
                dstPtr->intensity = static_cast<float>(*rowPtr)/4095.0F;
                dstPtr->x = *pointMap++;
                dstPtr->y = *pointMap++;
                dstPtr->z = *pointMap++;

                pointMap++;
                dstPtr++;
            }
        }
    } else {
        throw std::runtime_error("Left image does not have a valid greyscale format");
    }

    return ret;
}

inline pcl::PointCloud<pcl::PointXYZRGB>::Ptr Reconstruct3D::createXYZRGBCloud(const ImageSet& imageSet,
        const char* frameId, unsigned short minDisparity) {
    float* pointMap = createPointMap(imageSet, minDisparity);
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr ret = initPointCloud<pcl::PointXYZRGB>(imageSet, frameId);

    ImageSet::ImageType colImg = getColorImage(imageSet, COLOR_AUTO);

    pcl::PointXYZRGB* dstPtr = &ret->points[0];
    if(imageSet.getPixelFormat(colImg) != ImageSet::FORMAT_8_BIT_RGB) {
        throw std::runtime_error("Left image is not an RGB image");
    }

    for(int y = 0; y < imageSet.getHeight(); y++) {
        unsigned char* rowPtr = imageSet.getPixelData(colImg) + y*imageSet.getRowStride(colImg);
        unsigned char* endPtr = rowPtr + 3*imageSet.getWidth();
        for(; rowPtr < endPtr;rowPtr +=3) {
            dstPtr->r = rowPtr[0];
            dstPtr->g = rowPtr[1];
            dstPtr->b = rowPtr[2];
            dstPtr->x = *pointMap++;
            dstPtr->y = *pointMap++;
            dstPtr->z = *pointMap++;

            pointMap++;
            dstPtr++;
        }
    }

    return ret;
}

} // namespace

#endif
#endif
