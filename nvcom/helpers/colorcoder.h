/*******************************************************************************
 * Copyright (c) 2021 Nerian Vision GmbH
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

#ifndef NVSHARED_COLORCODER_H
#define NVSHARED_COLORCODER_H

#include <opencv2/opencv.hpp>

// Performs a color coding for given values
class ColorCoder {
public:
    enum ColorScale {
        COLOR_RED_BLUE_BGR,
        COLOR_RED_BLUE_RGB,
        COLOR_RAINBOW_BGR,
        COLOR_RAINBOW_RGB,
        COLOR_RAINBOW_CIRC_BGR,
        COLOR_RAINBOW_CIRC_RGB,
    };

    ColorCoder(ColorScale colorScale, float min, float max, bool shadowLess, bool shadowGreater,
        std::pair<unsigned short, unsigned short> invalidRange = std::pair<unsigned short, unsigned short>(0xFFF, 0xFFFF));

    // Color-codes a single value
    cv::Vec3b getColor(float val) {
        if(val<minVal) {
            if(shadowLess)
                return invalidColor;
            else val = minVal;
        } else if(val > maxVal) {
            if(shadowGreater)
                return invalidColor;
            else val = maxVal;
        }

        double relVal = (val - minVal)/(maxVal-minVal);
        return codeRelativeValue(relVal);
    }

    // Optimized method for unsigned chars
    cv::Vec3b getColor(unsigned char val) {
        return byteLookup[val];
    }

    // Optimized method for unsigned shorts
    cv::Vec3b getColor(unsigned short val) {
        return shortLookup[val];
    }

    // Color codes an entire image
    template <typename T>
    void codeImage(const cv::Mat_<T>& input, cv::Mat_<cv::Vec3b>& output) {
        for(int y = 0; y<input.rows; y++)
            for(int x = 0; x<input.cols; x++)
                output(y,x) = getColor((float)input(y,x));
    }

    // Creates a new image with a legend at the right border
    cv::Mat_<cv::Vec3b> createLegendBorder(unsigned int srcWidth, unsigned int srcHeight,
        double legendScale = 1.0);

    // [Re]create the accelerated color lookup tables
    void recalculateLookups();

    // Set new min / max values (for dynamic range adaptation)
    inline void setMin(float minVal) {
        this->minVal = minVal;
    }
    inline void setMax(float maxVal) {
        this->maxVal = maxVal;
    }

    // Get min / max values
    inline float getMin() const {
        return this->minVal;
    }
    inline float getMax() const {
        return this->maxVal;
    }

private:
    float minVal;
    float maxVal;
    bool shadowLess;
    bool shadowGreater;
    std::pair<unsigned short, unsigned short> invalidRange;

    static const std::vector<cv::Vec3b> redBlueTable;
    static const cv::Vec3b redBlueInvalid;

    static const std::vector<cv::Vec3b> rainbowTable;
    static const std::vector<cv::Vec3b> rainbowCircTable;
    static const cv::Vec3b rainbowInvalid;

    // The selected color scheme
    std::vector<cv::Vec3b> colorTable;
    cv::Vec3b invalidColor;

    // Lookup table for converting byte and unsigned short values
    cv::Vec3b byteLookup[0x100];
    cv::Vec3b shortLookup[0x10000];

    cv::Vec3b codeRelativeValue(float i) {
        int min = std::min(int(colorTable.size())-1, std::max(0, int(i * colorTable.size())));
        int max = std::min(int(colorTable.size())-1, std::max(0, int(i * colorTable.size())+1));

        double f2 = (i*colorTable.size()) - min;
        double f1 = 1.0 - f2;
        return cv::Vec3b((unsigned char)(f1 * colorTable[min][2] + f2 * colorTable[max][2] + 0.5),
            (unsigned char)(f1 * colorTable[min][1] + f2 * colorTable[max][1] + 0.5),
            (unsigned char)(f1 * colorTable[min][0] + f2 * colorTable[max][0] + 0.5));
    }

    void bgrToRgbTable(const std::vector<cv::Vec3b>& src, std::vector<cv::Vec3b>& dst);
};

// Optimized implementations for codeImage
template <>
void ColorCoder::codeImage(const cv::Mat_<unsigned char>& input, cv::Mat_<cv::Vec3b>& output);

template <>
void ColorCoder::codeImage(const cv::Mat_<unsigned short>& input, cv::Mat_<cv::Vec3b>& output);

#endif
