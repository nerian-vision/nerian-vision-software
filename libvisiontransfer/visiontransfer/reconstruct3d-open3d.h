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

#ifndef VISIONTRANSFER_RECONSTRUCT3D_OPEN3D_H
#define VISIONTRANSFER_RECONSTRUCT3D_OPEN3D_H

#ifdef OPEN3D_VERSION

namespace visiontransfer {

/*
 * Open3d-specific implementations that need to be inlined in order to avoid
 * dependencies for projects that do not make use of Open3D
 */

inline std::shared_ptr<open3d::geometry::PointCloud> Reconstruct3D::createOpen3DCloud(
        const ImageSet& imageSet, ColorSource colSource, unsigned short minDisparity, unsigned short maxDisparity) {

    int numPoints = imageSet.getWidth() * imageSet.getHeight();
    std::shared_ptr<open3d::geometry::PointCloud> ret(new open3d::geometry::PointCloud());

    // Convert the 3D point cloud
    ret->points_.resize(numPoints);

    float* points = createPointMap(imageSet, minDisparity, maxDisparity);
    float* end = &points[4*numPoints];
    Eigen::Vector3d* dest = &ret->points_[0];

    while(points != end) {
        float x = *(points++);
        float y = *(points++);
        float z = *(points++);
        points++;

        *dest = Eigen::Vector3d(x, y, z);
        dest++;
    }

    ImageSet::ImageType colImg = getColorImage(imageSet, colSource);

    // Convert the color information if enabled
    if(colSource != COLOR_NONE && imageSet.hasImageType(colImg)) {
        ret->colors_.resize(numPoints);
        unsigned char* pixel = imageSet.getPixelData(colImg);
        Eigen::Vector3d* color = &ret->colors_[0];
        Eigen::Vector3d* colorEnd = &ret->colors_[numPoints];

        switch(imageSet.getPixelFormat(colImg)) {
            case ImageSet::FORMAT_8_BIT_MONO:
                while(color != colorEnd) {
                    double col = double(*(pixel++))/0xFF;
                    *(color++) = Eigen::Vector3d(col, col, col);
                }
                break;
            case ImageSet::FORMAT_12_BIT_MONO:
                while(color != colorEnd) {
                    double col = double(*reinterpret_cast<unsigned short*>(pixel))/0xFFF;
                    pixel+=2;
                    *(color++) = Eigen::Vector3d(col, col, col);
                }
                break;
            case ImageSet::FORMAT_8_BIT_RGB:
                while(color != colorEnd) {
                    double r = double(*(pixel++))/0xFF;
                    double g = double(*(pixel++))/0xFF;
                    double b = double(*(pixel++))/0xFF;
                    *(color++) = Eigen::Vector3d(r, g, b);
                }
                break;
            default: throw std::runtime_error("Illegal pixel format");
        }
    }

    return ret;
}

inline std::shared_ptr<open3d::geometry::RGBDImage> Reconstruct3D::createOpen3DImageRGBD(const ImageSet& imageSet,
    ColorSource colSource, unsigned short minDisparity) {

    std::shared_ptr<open3d::geometry::RGBDImage> ret(new open3d::geometry::RGBDImage);

    // Convert depth map
    ret->depth_.width_ = imageSet.getWidth();
    ret->depth_.height_ = imageSet.getHeight();
    ret->depth_.num_of_channels_ = 1;
    ret->depth_.bytes_per_channel_ = sizeof(float);
    ret->depth_.data_.resize(ret->depth_.width_*ret->depth_.height_*ret->depth_.bytes_per_channel_);

    float* zMap = createZMap(imageSet, minDisparity);
    memcpy(&ret->depth_.data_[0], zMap, ret->depth_.data_.size());

    // Convert color
    ret->color_.width_ = imageSet.getWidth();
    ret->color_.height_ = imageSet.getHeight();
    ret->color_.num_of_channels_ = 3;
    ret->color_.bytes_per_channel_ = 1;
    ret->color_.data_.resize(ret->color_.width_ * ret->color_.height_ *
        ret->color_.num_of_channels_ * ret->color_.bytes_per_channel_);

    ImageSet::ImageType colImg = getColorImage(imageSet, colSource);

    unsigned char* srcPixel = imageSet.getPixelData(colImg);
    unsigned char* dstPixel = &ret->color_.data_[0];
    unsigned char* dstEnd = &ret->color_.data_[ret->color_.data_.size()];

    switch(imageSet.getPixelFormat(colImg)) {
        case ImageSet::FORMAT_8_BIT_MONO:
            while(dstPixel != dstEnd) {
                *(dstPixel++) = *srcPixel;
                *(dstPixel++) = *srcPixel;
                *(dstPixel++) = *(srcPixel++);
            }
            break;
        case ImageSet::FORMAT_12_BIT_MONO:
            while(dstPixel != dstEnd) {
                unsigned short pixel16Bit = *reinterpret_cast<unsigned short*>(srcPixel);
                unsigned char pixel8Bit = pixel16Bit / 0xF;
                srcPixel += 2;

                *(dstPixel++) = pixel8Bit;
                *(dstPixel++) = pixel8Bit;
                *(dstPixel++) = pixel8Bit;
            }
            break;
        case ImageSet::FORMAT_8_BIT_RGB:
            memcpy(&ret->color_.data_[0], srcPixel, ret->color_.data_.size());
            break;
        default: throw std::runtime_error("Illegal pixel format");
    }

    return ret;
}

} // namespace

#endif
#endif
