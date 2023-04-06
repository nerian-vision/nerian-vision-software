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

#ifndef VISIONTRANSFER_RECONSTRUCT3D_H
#define VISIONTRANSFER_RECONSTRUCT3D_H

#include <limits>
#include <stdexcept>
#include "visiontransfer/common.h"
#include "visiontransfer/imageset.h"

#ifdef OPEN3D_VERSION
#    include <memory>
#endif

namespace visiontransfer {

/**
 * \brief Transforms a disparity map into a set of 3D points.
 *
 * Use this class for reconstructing the 3D location for each valid
 * point in a disparity map.
 */
class VT_EXPORT Reconstruct3D {
public:

    /**
     * \brief Source channel selection for color information.
     */
    enum ColorSource {
        /// Omit color information
        COLOR_NONE,
        /// Automatically choose the best color channel
        COLOR_AUTO,
        /// Copy color data from left camera
        COLOR_LEFT,
        /// Copy color data from 3rd color camera
        COLOR_THIRD_COLOR,
    };

    /**
     * \brief Constructs a new object for 3D reconstructing.
     */
    Reconstruct3D();

    ~Reconstruct3D();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    /**
     * \brief Reconstructs the 3D location of each pixel in the given
     * disparity map.
     *
     * \param dispMap Pointer to the data of the disparity map. The disparity map is
     *        assumed to have a N-bit subpixel resolution. This means that each
     *        value needs to be divided by the subpixel factor to receive the true disparity.
     * \param width Width of the disparity map.
     * \param height Height of the disparity map.
     * \param rowStride Row stride (i.e. distance between two rows in bytes) of the
     *        disparity map.
     * \param q Disparity-to-depth mapping matrix of size 4x4. The matrix is
     *        stored in a row-wise alignment. Obtain this matrix from your
     *        camera calibration data.
     * \param minDisparity The minimum disparity, again with N-bit subpixel
     *        resolution. Lower disparities will be clamped to this value before
     *        computing the 3D location.
     * \param subpixelFactor Subpixel division factor for disparity value.
     * \param maxDisparity The maximum value that occurs in the disparity map. Any value
     *        greater or equal will be marked as invalid.
     *
     * This method is deprecated in favor of createPointMap(const ImageSet&, unsigned short).
     */

    DEPRECATED("Use createPointMap(const ImageSet&, ...) instead.")
    float* createPointMap(const unsigned short* dispMap, int width, int height,
        int rowStride, const float* q, unsigned short minDisparity = 1,
        int subpixelFactor = 16, unsigned short maxDisparity = 0xFFF);
#endif

    /**
     * \brief Reconstructs the 3D location of each pixel in the given
     * disparity map.
     *
     * \param imageSet Image set containing the disparity map.
     * \param minDisparity The minimum disparity with N-bit subpixel resolution.
     * \param maxDisparity The maximum value that occurs in the disparity map. Any value
     *        greater or equal will be marked as invalid.
     *
     * The output map will have a size of exactly 4*width*height float values. For each
     * point the x, y and z coordinates are stored consecutively, plus one additional
     * float (four bytes) as padding. Invalid disparities will be set to the given minimum disparity.
     *
     * If the minimum disparity is set to 0, points with a disparity of 0 or an invalid
     * disparity will receive a z coordinate of +inf. If a larger minimum disparity is given,
     * points with a lower disparity will be at a fix depth that corresponds to this
     * disparity.
     *
     * The returned point map is valid until the next call of createPointMap(), createZMap(), or
     * writePlyFile().
     */
    float* createPointMap(const ImageSet& imageSet, unsigned short minDisparity = 1,
        unsigned short maxDisparity = 0xFFF);

    /**
     * \brief Converts the disparity in an image set to a depth map
     *
     * \param imageSet Image set containing the disparity map.
     * \param minDisparity The minimum disparity with N-bit subpixel resolution.
     * \param maxDisparity The maximum value that occurs in the disparity map. Any value
     *        greater or equal will be marked as invalid.
     *
     * The output map will have a size of exactly width*height float values. Each
     * value represents the depth at the given pixel coordinate in meters.
     *
     * This method is closely related to createPointMap(). It only computes the
     * Z coordinates, whereas createPointMap() also computes X and Y coordinates
     * for each image point.
     *
     * If the minimum disparity is set to 0, points with a disparity of 0 or an invalid
     * disparity will receive a z coordinate of +inf. If a larger minimum disparity is given,
     * points with a lower disparity will be at a fix depth that corresponds to this
     * disparity.
     *
     * The returned map is valid until the next call of createZMap(), createPointMap() or
     * writePlyFile().
     */
    float* createZMap(const ImageSet& imageSet, unsigned short minDisparity = 1,
        unsigned short maxDisparity = 0xFFF);

    /**
     * \brief Reconstructs the 3D location of one individual point.
     *
     * \param imageX X component of the image location.
     * \param imageY Y component of the image location.
     * \param disparity Value of the disparity map at the image location.
     *        It is assumed that the lower N bits are the fractional component.
     *        This means that each value needs to be divided by a subpixel factor to
     *        receive the true disparity.
     * \param q Disparity-to-depth mapping matrix of size 4x4. The matrix is
     *        stored in a row-wise alignment. Obtain this matrix from your
     *        camera calibration data.
     * \param pointX Destination variable for the 3D point X component.
     * \param pointY Destination variable for the 3D point Y component.
     * \param pointZ Destination variable for the 3D point Z component.
     * \param subpixelFactor Subpixel division factor for disparity value.
     *
     *
     * This method projects a single point from a disparity map to a
     * 3D location. If the 3D coordinates of multiple points are of interest,
     * createPointMap() should be used for best performance.
     */
    void projectSinglePoint(int imageX, int imageY, unsigned short disparity, const float* q,
        float& pointX, float& pointY, float& pointZ, int subpixelFactor = 16);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    /**
     * \brief Projects the given disparity map to 3D points and exports the result to
     * a PLY file.
     *
     * \param file The output file
     * \param dispMap Pointer to the data of the disparity map. The disparity map is
     *        assumed to have a N-bit subpixel resolution. This means that each
     *        value needs to be divided by a subpixel factor to receive the true disparity.
     * \param image Rectified left input image in 8-bit gray scale format.
     * \param width Width of the disparity map and left image.
     * \param height Height of the disparity map and left image.
     * \param format Pixel format for the left image.
     * \param dispRowStride Row stride (i.e. distance between two rows in bytes) of the
     *        disparity map.
     * \param imageRowStride Row stride (i.e. distance between two rows in bytes) of the
     *        left image.
     * \param q Disparity-to-depth mapping matrix of size 4x4. The matrix is
     *        stored in a row-wise alignment. Obtain this matrix from your
     *        camera calibration data.
     * \param maxZ Maximum allowed z-coordinate. Points with a higher z-coordinate
     *        are not exported.
     * \param binary Specifies whether the ASCII or binary PLY-format should be used.
     * \param subpixelFactor Subpixel division factor for disparity value.
     * \param maxDisparity The maximum value that occurs in the disparity map. Any value
     *        greater or equal will be marked as invalid.
     *
     * This method is deprecated in favor of
     * writePlyFile(const char*, const ImageSet&, double maxZ, bool);
     */
    DEPRECATED("Use writePlyFile(const char*, const ImageSet&, ...) instead.")
    void writePlyFile(const char* file, const unsigned short* dispMap,
        const unsigned char* image,  int width, int height, ImageSet::ImageFormat format,
        int dispRowStride, int imageRowStride, const float* q,
        double maxZ = (std::numeric_limits<double>::max)(),
        bool binary = false, int subpixelFactor = 16, unsigned short maxDisparity = 0xFFF);
#endif

    /**
     * \brief Projects the given disparity map to 3D points and exports the result to
     * a PLY file.
     *
     * \param file The name for the output file.
     * \param imageSet Image set containing camera image and disparity map.
     * \param maxZ Maximum allowed z-coordinate.
     * \param binary Specifies whether the ASCII or binary PLY-format should be used.
     * \param colSource Source channel of the color information
     * \param maxDisparity The maximum value that occurs in the disparity map. Any value
     *        greater or equal will be marked as invalid.
     *
     */
    void writePlyFile(const char* file, const ImageSet& imageSet,
        double maxZ = (std::numeric_limits<double>::max)(),
        bool binary = false,
        ColorSource colSource = COLOR_AUTO,
        unsigned short maxDisparity = 0xFFF);

#ifdef PCL_MAJOR_VERSION
    /**
     * \brief Projects the given disparity map to a PCL point cloud without pixel intensities
     *
     * \param imageSet Image set containing the disparity map.
     * \param frameId Frame ID that will be assigned to the created point cloud.
     * \param minDisparity The minimum disparity with N-bit subpixel resolution.
     *
     * For this method to be available, the PCL headers must be included before
     * the libvisiontransfer headers!
     *
     * If the minimum disparity is set to 0, points with a disparity of 0 or an invalid
     * disparity will receive a z coordinate of +inf. If a larger minimum disparity is given,
     * points with a lower disparity will be at a fix depth that corresponds to this
     * disparity.
     */
    inline pcl::PointCloud<pcl::PointXYZ>::Ptr createXYZCloud(const ImageSet& imageSet,
            const char* frameId, unsigned short minDisparity = 0);

    /**
     * \brief Projects the given disparity map to a PCL point cloud, including pixel intensities.
     *
     * See createXYZCloud() for details.
     */
    inline pcl::PointCloud<pcl::PointXYZI>::Ptr createXYZICloud(const ImageSet& imageSet,
            const char* frameId, unsigned short minDisparity = 0);

    /**
     * \brief Projects the given disparity map to a PCL point cloud, including pixel RGB data.
     *
     * See createXYZCloud() for details.
     */
    inline pcl::PointCloud<pcl::PointXYZRGB>::Ptr createXYZRGBCloud(const ImageSet& imageSet,
        const char* frameId, unsigned short minDisparity = 0);
#endif

#ifdef OPEN3D_VERSION
    /**
     * \brief Projects the given disparity map to a Open3D point cloud
     *
     * \param imageSet Image set containing the disparity map.
     * \param colSource Source channel of the color information
     * \param minDisparity The minimum disparity with N-bit subpixel resolution.
     * \param maxDisparity The maximum value that occurs in the disparity map. Any value
     *        greater or equal will be marked as invalid.
     *
     * For this method to be available, the Open3d headers must be included before
     * the libvisiontransfer headers!
     *
     * If the minimum disparity is set to 0, points with a disparity of 0 or an invalid
     * disparity will receive a z coordinate of +inf. If a larger minimum disparity is given,
     * points with a lower disparity will be at a fix depth that corresponds to this
     * disparity.
     */
    inline std::shared_ptr<open3d::geometry::PointCloud> createOpen3DCloud(const ImageSet& imageSet,
        ColorSource colSource = COLOR_AUTO, unsigned short minDisparity = 0, unsigned short maxDisparity = 0xFFF);
    /**
     * \brief Converts the given disparity map to a Open3D RGBDn image
     *
     * \param imageSet Image set containing the disparity map.
     * \param minDisparity The minimum disparity with N-bit subpixel resolution.
     *
     * For this method to be available, the Open3d headers must be included before
     * the libvisiontransfer headers!
     *
     * If the minimum disparity is set to 0, points with a disparity of 0 or an invalid
     * disparity will receive a z coordinate of +inf. If a larger minimum disparity is given,
     * points with a lower disparity will be at a fix depth that corresponds to this
     * disparity.
     */
    inline std::shared_ptr<open3d::geometry::RGBDImage> createOpen3DImageRGBD(const ImageSet& imageSet,
        ColorSource colSource = COLOR_AUTO, unsigned short minDisparity = 0);
#endif

private:
    // We follow the pimpl idiom
    class Pimpl;
    Pimpl* pimpl;

    // This class cannot be copied
    Reconstruct3D(const Reconstruct3D& other);
    Reconstruct3D& operator=(const Reconstruct3D&);

#ifdef PCL_MAJOR_VERSION
    // Initializes a PCL point cloud
    template <typename T>
    typename pcl::PointCloud<T>::Ptr initPointCloud(const ImageSet& imageSet, const char* frameId);
#endif

    // Inlined code, as it is needed by PCL and Open3D bindings
    static ImageSet::ImageType getColorImage(const ImageSet& imageSet, ColorSource colSource) {
        switch(colSource) {
            case COLOR_AUTO:
                if(imageSet.hasImageType(ImageSet::IMAGE_COLOR)) {
                    return ImageSet::IMAGE_COLOR;
                } else {
                    return ImageSet::IMAGE_LEFT;
                }
            case COLOR_LEFT:
                return ImageSet::IMAGE_LEFT;
            case COLOR_THIRD_COLOR:
                return ImageSet::IMAGE_COLOR;
                break;
            default:
                return ImageSet::IMAGE_UNDEFINED;
        }
    }
};

} // namespace

#include "visiontransfer/reconstruct3d-pcl.h"
#include "visiontransfer/reconstruct3d-open3d.h"

#endif
