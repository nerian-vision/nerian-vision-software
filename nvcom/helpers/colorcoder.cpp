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

#include <cstdio>
#include <string>
#include "colorcoder.h"

using namespace cv;
using namespace std;

const std::vector<cv::Vec3b>  ColorCoder::redBlueTable = {
    // See Moreland, K.: Diverging Color Maps for Scientific Visualization
    cv::Vec3b(59, 76, 192),
    cv::Vec3b(68, 90, 204),
    cv::Vec3b(77, 104, 215),
    cv::Vec3b(87, 117, 225),
    cv::Vec3b(98, 130, 234),
    cv::Vec3b(108, 142, 241),
    cv::Vec3b(119, 154, 247),
    cv::Vec3b(130, 165, 251),
    cv::Vec3b(141, 176, 254),
    cv::Vec3b(152, 185, 255),
    cv::Vec3b(163, 194, 255),
    cv::Vec3b(174, 201, 253),
    cv::Vec3b(184, 208, 249),
    cv::Vec3b(194, 213, 244),
    cv::Vec3b(204, 217, 238),
    cv::Vec3b(213, 219, 230),
    cv::Vec3b(221, 221, 221),
    cv::Vec3b(229, 216, 209),
    cv::Vec3b(236, 211, 197),
    cv::Vec3b(241, 204, 185),
    cv::Vec3b(245, 196, 173),
    cv::Vec3b(247, 187, 160),
    cv::Vec3b(247, 177, 148),
    cv::Vec3b(247, 166, 135),
    cv::Vec3b(244, 154, 123),
    cv::Vec3b(241, 141, 111),
    cv::Vec3b(236, 127, 99),
    cv::Vec3b(229, 112, 88),
    cv::Vec3b(222, 96, 77),
    cv::Vec3b(213, 80, 66),
    cv::Vec3b(203, 62, 56),
    cv::Vec3b(192, 40, 47),
    cv::Vec3b(180, 4, 38)
};

const cv::Vec3b ColorCoder::redBlueInvalid = cv::Vec3b(0, 0, 0);

const std::vector<cv::Vec3b>  ColorCoder::rainbowTable = {
cv::Vec3b(63, 0, 63),
    cv::Vec3b(63, 0, 191),
    cv::Vec3b(0, 63, 255),
    cv::Vec3b(0, 128, 255),
    cv::Vec3b(0, 160, 255),
    cv::Vec3b(63, 191, 255),
    cv::Vec3b(63, 223, 255),
    cv::Vec3b(63, 255, 255),
    cv::Vec3b(63, 255, 191),
    cv::Vec3b(63, 255, 63),
    cv::Vec3b(128, 255, 63),
    cv::Vec3b(191, 255, 63),
    cv::Vec3b(255, 255, 63),
    cv::Vec3b(255, 223, 63),
    cv::Vec3b(255, 160, 63),
    cv::Vec3b(255, 96, 63),
    cv::Vec3b(255, 31, 63),
    cv::Vec3b(255, 96, 191),
    cv::Vec3b(255, 160, 255)
};

const std::vector<cv::Vec3b>  ColorCoder::rainbowCircTable = {
    cv::Vec3b(255, 255, 0)  + cv::Vec3b(32, 32, 32),
    cv::Vec3b(0, 255, 0)    + cv::Vec3b(32, 32, 32),
    cv::Vec3b(0, 255, 255)  + cv::Vec3b(32, 32, 32),
    cv::Vec3b(0, 0, 255)    + cv::Vec3b(32, 32, 32),
    cv::Vec3b(255, 0, 255)  + cv::Vec3b(32, 32, 32),
    cv::Vec3b(255, 0, 0)    + cv::Vec3b(32, 32, 32),
    cv::Vec3b(255, 255, 0)  + cv::Vec3b(32, 32, 32),
};

const cv::Vec3b ColorCoder::rainbowInvalid = cv::Vec3b(128, 128, 128);


ColorCoder::ColorCoder(ColorScale colorScale, float min, float max, bool shadowLess, bool shadowGreater,
        std::pair<unsigned short, unsigned short> invalidRange)
    :minVal(min), maxVal(max), shadowLess(shadowLess), shadowGreater(shadowGreater), invalidRange(invalidRange)
{
    switch(colorScale) {
        case COLOR_RED_BLUE_BGR:
            colorTable = redBlueTable;
            invalidColor = redBlueInvalid;
            break;
        case COLOR_RED_BLUE_RGB:
            bgrToRgbTable(redBlueTable, colorTable);
            invalidColor = cv::Vec3b(redBlueInvalid(2), redBlueInvalid(1), redBlueInvalid(0));
            break;
        case COLOR_RAINBOW_BGR:
            colorTable = rainbowTable;
            invalidColor = rainbowInvalid;
            break;
        case COLOR_RAINBOW_RGB:
            bgrToRgbTable(rainbowTable, colorTable);
            invalidColor = cv::Vec3b(rainbowInvalid(2), rainbowInvalid(1), rainbowInvalid(0));
            break;
        case COLOR_RAINBOW_CIRC_BGR:
            colorTable = rainbowCircTable;
            invalidColor = rainbowInvalid;
            break;
        case COLOR_RAINBOW_CIRC_RGB:
            bgrToRgbTable(rainbowCircTable, colorTable);
            invalidColor = cv::Vec3b(rainbowInvalid(2), rainbowInvalid(1), rainbowInvalid(0));
            break;
    }

    recalculateLookups();
}

void ColorCoder::recalculateLookups() {
    for(int i=0; i<0x100; i++)
        byteLookup[i] = getColor((float)i);

    for(int i=0; i<0x10000; i++)
        shortLookup[i] = getColor((float)i);

    if(!shadowGreater) {
        // Make sure that at least the invalid disparity range is
        // shadowed
        for(int i=invalidRange.first; i<= invalidRange.second; i++) {
            shortLookup[i] = invalidColor;
        }
    } else {
        shortLookup[0xFFFF] = invalidColor;
    }

    if(shadowLess && minVal == 0)
        byteLookup[255] = invalidColor;
}

void ColorCoder::bgrToRgbTable(const std::vector<cv::Vec3b>& src, std::vector<cv::Vec3b>& dst) {
    dst.clear();
    for(cv::Vec3b col: src) {
        dst.push_back(cv::Vec3b(col(2), col(1), col(0)));
    }
}

template <>
void ColorCoder::codeImage(const cv::Mat_<unsigned char>& input, cv::Mat_<cv::Vec3b> &output) {
    for(int y = 0; y<input.rows; y++)
        for(int x = 0; x<input.cols; x++)
            output(y,x) = byteLookup[input(y,x)];
}

template <>
void ColorCoder::codeImage(const cv::Mat_<unsigned short>& input, cv::Mat_<cv::Vec3b> &output) {
    for(int y = 0; y<input.rows; y++)
        for(int x = 0; x<input.cols; x++)
            output(y,x) =  shortLookup[input(y,x)];
}

Mat_<Vec3b> ColorCoder::createLegendBorder(unsigned int srcWidth, unsigned int srcHeight, double legendScale) {
    int gap = 2;

    // Find the number of decimal digits to print
    int maxDecimals = 0, minDecimals = 0;
    float minBuf = minVal, maxBuf = maxVal;
    for(int i=0; i<4; i++)
    {
        if(minBuf - int(minBuf) != 0)
            minDecimals++;
        if(maxBuf - int(maxBuf) != 0)
            maxDecimals++;
        minBuf*=10.0;
        maxBuf*=10.0;
    }

    // Create label strings
    char maxStr[10], minStr[10];
    snprintf(minStr, sizeof(minStr), "%.*f", minDecimals, minVal*legendScale);
    snprintf(maxStr, sizeof(maxStr), "%.*f", maxDecimals, maxVal*legendScale);
    const char* minWidthString = "000";

    constexpr double fontScale = 0.7;

    // Find label size
    int baseline;
    Size fontSize = getTextSize(strlen(minWidthString) > strlen(maxStr) ? minWidthString : maxStr,
        FONT_HERSHEY_TRIPLEX, fontScale, 4.0, &baseline);

    // Draw color gradient
    Mat_<Vec3b> dst(srcHeight, srcWidth + gap + fontSize.width, Vec3b(0, 0, 0));
    for(int y = 0; y< dst.rows; y++)
        for(int x = srcWidth + gap; x < dst.cols; x++) {
            dst(y, x) = codeRelativeValue(y/double(srcHeight));
    }

    // Print labels
    putText(dst, maxStr,
        cv::Point(srcWidth + gap, dst.rows - baseline),
        FONT_HERSHEY_TRIPLEX, fontScale,
        (Scalar)Vec3b(255, 255, 255), 3 /*thickness*/, cv::LINE_AA);
    putText(dst, maxStr,
        cv::Point(srcWidth + gap, dst.rows - baseline),
        FONT_HERSHEY_TRIPLEX, fontScale,
        (Scalar)Vec3b(0, 0, 0), 1 /*thickness*/, cv::LINE_AA);
    putText(dst, minStr,
        cv::Point(srcWidth + gap, fontSize.height + baseline),
        FONT_HERSHEY_TRIPLEX, fontScale,
        (Scalar)Vec3b(255, 255, 255), 3 /*thickness*/, cv::LINE_AA);
    putText(dst, minStr,
        cv::Point(srcWidth + gap, fontSize.height + baseline),
        FONT_HERSHEY_TRIPLEX, fontScale,
        (Scalar)Vec3b(0, 0, 0), 1 /*thickness*/, cv::LINE_AA);

    return dst;
}
