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

#ifndef VISIONTRANSFER_IMAGESET_H
#define VISIONTRANSFER_IMAGESET_H

#include <cassert>
#include <cstddef>
#include "visiontransfer/common.h"

namespace visiontransfer {

/**
 * \brief A set of one to three images, but usually two (the left camera image
 *  and the disparity map). One- and three-image modes can be enabled
 *  in the device configuration interface.
 *
 *  For backwards compatibility, for sets of at least two images the image at
 *  index 0 is always the left camera image, while the one at index 1 is either the
 *  disparity map if in disparity processing mode, or the right image otherwise.
 *
 * All images must be of equal width and height, but are allowed to have
 * different pixel formats. Please note that the class does not manage the
 * pixel data but only keeps pointers. You thus need to ensure that the pixel
 * data remains valid for as long as this object persists.
 */
class VT_EXPORT ImageSet {
public:
    static const int MAX_SUPPORTED_IMAGES = 4;
    /**
     * \brief Image formats that can be transferred.
     */
    enum ImageFormat {
        /// 8-bit greyscale format
        FORMAT_8_BIT_MONO,

        /// 8-bit RGB format
        FORMAT_8_BIT_RGB,

        /// 12-bit greyscale format plus 4 bits of padding
        /// (hence a total of 16 bits).
        FORMAT_12_BIT_MONO
    };

    /**
     * \deprecated Please use the new format constants in \c ImageFormat.
     */
    enum ImageFormat_Deprecated {
        FORMAT_8_BIT =  FORMAT_8_BIT_MONO,
        FORMAT_12_BIT = FORMAT_12_BIT_MONO
    };

    /**
     * \brief Supported image types
     */
    enum ImageType {
        IMAGE_UNDEFINED,
        IMAGE_LEFT,
        IMAGE_DISPARITY,
        IMAGE_RIGHT,

        /// 3rd color camera for devices where this is supported
        IMAGE_COLOR
    };

    /**
     * \brief Default constructor creating an image set with no pixel data.
     */
    ImageSet();

    /**
     * \brief Copy constructor creating a shallow copy of the image set.
     */
    ImageSet(const ImageSet& other);

    ~ImageSet();
    ImageSet& operator= (ImageSet const& other);

    /**
     * \brief Sets a new width for both images.
     */
    void setWidth(int w) {width = w;}

    /**
     * \brief Sets a new width for both images.
     */
    void setHeight(int h) {height = h;}

    /**
     * \brief Sets a new row stride for the pixel data of one image.
     *
     * \param imageNumber Number of the image for which to set the
     *        row stride (0 ... getNumberOfImages()-1).
     * \param stride The row stride that shall be set.
     */
    void setRowStride(int imageNumber, int stride) {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        rowStride[imageNumber] = stride;
    }

    /**
     * \brief Sets the pixel format for the given image.
     *
     * \param imageNumber Number of the image for which to set the
     *        pixel format (0 ... getNumberOfImages()-1).
     * \param format The pixel format that shall be set.
     */
    void setPixelFormat(int imageNumber, ImageFormat format) {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        formats[imageNumber] = format;
    }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    DEPRECATED("Use setPixelFormat(int, ImageFormat) instead") void setPixelFormat(int imageNumber, ImageFormat_Deprecated format) {
        setPixelFormat(imageNumber, static_cast<ImageFormat>(format));
    }
#endif

    /**
     * \brief Sets the pixel data for the given image.
     *
     * \param imageNumber Number of the image for which to set the
     *        pixel data (0 ... getNumberOfImages()-1).
     * \param pixelData The pixel data that shall be set.
     */
    void setPixelData(int imageNumber, unsigned char* pixelData) {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        data[imageNumber] = pixelData;
    }

    /**
     * \brief Sets the pointer to the disparity-to-depth mapping matrix q.
     *
     * No data is copied. The data which q is pointing to has to remain valid
     * for as long as this object exists.
     */
    void setQMatrix(const float* q) {
        qMatrix = q;
    }

    /**
     * \brief Sets the sequence number for this image set.
     */
    void setSequenceNumber(unsigned int num) {
        seqNum = num;
    }

    /**
     * \brief Sets the time at which this image set has been captured.
     *
     * \param seconds The time stamp with a resolution of one second.
     * \param microsec The fractional seconds part of the time stamp with
     *        a resolution of 1 microsecond.
     */
    void setTimestamp(int seconds, int microsec) {
        timeSec = seconds;
        timeMicrosec = microsec;
    }

    /**
     * \brief Sets the value range for the disparity map contained in this
     *        image set.
     *
     * \param minimum Minimum disparity value.
     * \param maximum Maximum disparity value.
     */
    void setDisparityRange(int minimum, int maximum) {
        minDisparity = minimum;
        maxDisparity = maximum;
    }

    /**
     * \brief Sets the subpixel factor for this image set.
     */
    void setSubpixelFactor(int subpixFact) {
        subpixelFactor = subpixFact;
    }

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    /**
     * \brief Sets whether this is a left camera image and disparity
     * map set, or two raw camera images.
     *
     * DEPRECATION NOTICE: Please use setNumberOfImages() and setIndexOf()
     *  for more comprehensive control of the images in the image set.
     */
    DEPRECATED("Only compatible with two-image sets: use setNumberOfImages() and setIndexOf() instead")
    void setImageDisparityPair(bool dispPair);
#endif

    /**
     * \brief Returns the width of each image.
     */
    int getWidth() const {return width;}

    /**
     * \brief Returns the height of each image.
     */
    int getHeight() const {return height;}

    /**
     * \brief Returns the row stride for the pixel data of one image.
     *
     * \param imageNumber Number of the image for which to obtain the
     *        row stride (0 ... getNumberOfImages()-1).
     *
     * Please use getRowStride(ImageSet::ImageType) to access the
     * data by their abstract role in lieu of their index in the set.
     */
    int getRowStride(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return rowStride[imageNumber];
    }

    /**
     * \brief Returns the row stride for the pixel data of one image.
     *
     * \param what ImageType of the desired channel.
     *
     * This function will throw an exception when the ImageType
     * is not present in this set (use hasImageType(what) to check).
     */
    int getRowStride(ImageType what) const {
        int idx = getIndexOf(what, true);
        return getRowStride(idx);
    }

    /**
     * \brief Returns the pixel format for the given image.
     *
     * \param imageNumber Number of the image for which to receive the
     *        pixel format (0 ... getNumberOfImages()-1).
     *
     * Please use getPixelFormat(ImageSet::ImageType) to access the
     * data by their abstract role in lieu of their index in the set.
     */
    ImageFormat getPixelFormat(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return formats[imageNumber];
    }

    /**
     * \brief Returns the pixel format for the given image.
     *
     * \param what ImageType of the desired channel.
     *
     * This function will throw an exception when the ImageType
     * is not present in this set (use hasImageType(what) to check).
     */
    ImageFormat getPixelFormat(ImageType what) const {
        int idx = getIndexOf(what, true);
        return getPixelFormat(idx);
    }

    /**
     * \brief Returns the pixel data for the given image.
     *
     * \param imageNumber Number of the image for which to receive the
     *        pixel data (0 ... getNumberOfImages()-1).
     *
     * Please use getPixelData(ImageSet::ImageType) to access the
     * data by their abstract role in lieu of their index in the set.
     */
    unsigned char* getPixelData(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return data[imageNumber];
    }

    /**
     * \brief Returns the pixel data for the given image.
     *
     * \param what ImageType of the desired channel.
     *
     * This function will throw an exception when the ImageType
     * is not present in this set (use hasImageType(what) to check).
     */
    unsigned char* getPixelData(ImageType what) const {
        int idx = getIndexOf(what, true);
        return getPixelData(idx);
    }

    /**
     * \brief Returns a pointer to the disparity-to-depth mapping matrix q.
     */
    const float* getQMatrix() const {
        return qMatrix;
    }

    /**
     * \brief Returns the sequence number for this image set.
     */
    unsigned int getSequenceNumber() const {return seqNum;}

    /**
     * \brief Returns the time at which this image set has been captured.
     *
     * \param seconds The time stamp with a resolution of one second.
     * \param microsec The fractional seconds part of the time stamp with
     *        a resolution of 1 microsecond.
     */
    void getTimestamp(int& seconds, int& microsec) const {
        seconds = timeSec;
        microsec = timeMicrosec;
    }

    /**
     * \brief Gets the value range for the disparity map contained in this
     *        image set. If the image set does not contain any disparity data
     *        then the disparity range is undefined.
     *
     * \param minimum Minimum disparity value.
     * \param maximum Maximum disparity value.
     */
    void getDisparityRange(int& minimum, int& maximum) const {
        minimum = minDisparity;
        maximum = maxDisparity;
    }

    /**
     * \brief Gets the subpixel factor for this image set.
     */
    int getSubpixelFactor() const {
        return subpixelFactor;
    }

    /**
     * \brief Writes one image of the set to a PGM or PPM file.
     *
     * \param imageNumber The number of the image that shall be written.
     * \param File name of the PGM or PPM file that shall be created.
     */
    void writePgmFile(int imageNumber, const char* fileName) const;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    /**
     * \brief Returns true if this is a left camera image and disparity
     * map pair.
     *
     * DEPRECATION NOTICE: this is a legacy function; in case of image sets
     * with one or three images, the result may not be as expected
     * (this functions returns true only for the case of exactly two images:
     * left image plus disparity image).
     *
     * The new function hasImageType(ImageSet::ImageType) provides better
     * granularity of what data are enabled and present.
     */
    DEPRECATED("Only compatible with two-image sets: use hasImageType(ImageSet::IMAGE_DISPARITY) instead")
    bool isImageDisparityPair() const {
        return (getNumberOfImages()==2) && hasImageType(IMAGE_DISPARITY);
    }
#endif

    /**
     * \brief Makes a deep copy of this image set.
     */
    void copyTo(ImageSet& dest);

    /**
     * \brief Returns the number of bytes that are required to store one
     * image pixel.
     *
     * \param imageNumber The number of the image (0 ... getNumberOfImages()-1).
     */
    int getBytesPerPixel(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return getBytesPerPixel(formats[imageNumber]);
    }

    /**
     * \brief Returns the number of bits that are required to store one
     * image pixel.
     *
     * \param imageNumber The number of the image (0 ... getNumberOfImages()-1).
     */
    int getBitsPerPixel(int imageNumber) const {
        assert(imageNumber >= 0 && imageNumber < getNumberOfImages());
        return getBitsPerPixel(formats[imageNumber]);
    }

    int getBitsPerPixel(ImageType what) const {
        int idx = getIndexOf(what, true);
        return getBitsPerPixel(idx);
    }

    static int getBitsPerPixel(ImageFormat format);

    /**
     * \brief Returns the number of bytes that are required to store one
     * image pixel with the given pixel format.
     */
    static int getBytesPerPixel(ImageFormat format);

    /**
     * \brief Returns the number of images in this set
     */
    int getNumberOfImages() const {
        return numberOfImages;
    }

    /**
     * \brief Sets the number of valid images in this set
     */
    void setNumberOfImages(int number) {
        assert(number >= 1 && number <= MAX_SUPPORTED_IMAGES);
        numberOfImages = number;
    }

    /**
     * \brief Returns the ImageType of the specified channel
     */
    ImageType getImageType(int imageNumber) const;

    /**
     * \brief Returns the index of a specific image type
     *
     * \param what ImageType of the desired channel.
     * \param throwIfNotFound Throw a runtime error instead of returning -1.
     *
     * \return Returns the index of the specified type, or -1 if not found.
     */
    int getIndexOf(ImageType what, bool throwIfNotFound=false) const;

    /**
     * \brief Returns whether a left camera image is included in the enabled data
     */
    bool hasImageType(ImageType what) const {
        return getIndexOf(what) >= 0;
    }


    /**
     * \brief Assign an image index to a specified ImageType, -1 to disable
     *
     * \param what The ImageType to assign a new image index to.
     * \param idx The index of the specified ImageType inside the data of
     *            this ImageSet (-1 to disable).
     */
    void setIndexOf(ImageType what, int idx);


#ifdef CV_MAJOR_VERSION
    /**
     * \brief Converts one image of the set to an OpenCV image.
     *
     * \param imageNumber The number of the image that shall be converted
     *        (0 ... getNumberOfImages()-1).
     * \param convertRgbToBgr If true, then color images will converted from
     *        RGB to BGR in order to comply to OpenCV's convention.
     *
     * For this method to be available, the OpenCV headers need to be
     * included before including headers for libvisiontransfer.
     *
     * Please note that only a shallow copy is performed. The ImageSet object
     * must be kept alive for as long as the OpenCV image is in use.
     */
    inline void toOpenCVImage(int imageNumber, cv::Mat& dest, bool convertRgbToBgr = true);
#endif

    /**
     * \brief Sets the exposure time that was used for capturing the image set
     *
     * \param timeMicrosec Exposure time measured in microseconds
     */
    void setExposureTime(int timeMicrosec) {
        exposureTime = timeMicrosec;
    }

    /**
     * \brief Gets the exposure time in microseconds that was used for
     * capturing the image set.
     *
     * \return Exposure time in microseconds
     */
    int getExposureTime() const {
        return exposureTime;
    }

    /**
     * \brief Sets the timestamp of the last received sync pulse

     * \param seconds The time stamp with a resolution of one second.
     * \param microsec The fractional seconds part of the time stamp with
     *        a resolution of 1 microsecond.
     */
    void setLastSyncPulse(int seconds, int microsec) {
        lastSyncPulseSec = seconds;
        lastSyncPulseMicrosec = microsec;
    }

    /**
     * \brief Gets the timestamp of the last received sync pulse
     *
     * \param seconds The time stamp with a resolution of one second.
     * \param microsec The fractional seconds part of the time stamp with
     *        a resolution of 1 microsecond.
     */
    void getLastSyncPulse(int& seconds, int& microsec) const {
        seconds = lastSyncPulseSec;
        microsec = lastSyncPulseMicrosec;
    }

private:
    // No pimpl idiom here as almost everything is inlined.
    int width;
    int height;
    int rowStride[MAX_SUPPORTED_IMAGES];
    ImageFormat formats[MAX_SUPPORTED_IMAGES];
    unsigned char* data[MAX_SUPPORTED_IMAGES];
    const float* qMatrix;
    int timeSec;
    int timeMicrosec;
    unsigned int seqNum;
    int minDisparity;
    int maxDisparity;
    int subpixelFactor;
    int* referenceCounter;
    int numberOfImages;

    int indexLeftImage;
    int indexRightImage;
    int indexDisparityImage;
    int indexColorImage;

    int exposureTime;
    int lastSyncPulseSec;
    int lastSyncPulseMicrosec;

    void copyData(ImageSet& dest, const ImageSet& src, bool countRef);
    void decrementReference();
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// For source compatibility
class DEPRECATED("Use ImageSet instead.") ImagePair: public ImageSet {
};
#endif

} // namespace

#include "visiontransfer/imageset-opencv.h"
#endif
