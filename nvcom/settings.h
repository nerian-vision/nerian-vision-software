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

#ifndef SETTINGS_H
#define SETTINGS_H

/*
 *  A collection of all available program settings
 */
struct Settings {
    enum ColorScheme {
        COLOR_SCHEME_NONE = 0,
        COLOR_SCHEME_RED_BLUE = 1,
        COLOR_SCHEME_RAINBOW = 2
    };

    std::string readDir;
    std::string writeDir;
    bool writeDirAlwaysAsk;
    bool writeDirSelected;

    bool readImages;
    double maxFrameRate;
    bool nonGraphical;
    std::string remoteHost;
    bool tcp;
    ColorScheme colorScheme;
    int remotePort;
    bool writeLeft;
    bool writeRight;
    bool writeThirdColor;
    bool writeDisparityColor;
    bool writeDisparityRaw;
    bool writePointCloud;
    float pointCloudMaxDist;
    bool binaryPointCloud;
    bool disableReception;
    bool printTimestamps;
    int zoomPercent;
    bool displayCoordinate;
    bool writePgm; // TODO
    bool adaptiveColorScale;
    bool view3D;
    bool convert12Bit;
    bool fileNameDateTime;
    double disparityOffset;

    Settings():
        readDir(""), writeDir(""), writeDirAlwaysAsk(false), writeDirSelected(false), readImages(false),
        maxFrameRate(0.0), nonGraphical(false), remoteHost(""),
        colorScheme(COLOR_SCHEME_NONE), remotePort(0),
        writeLeft(false), writeRight(false), writeThirdColor(false), writeDisparityColor(false),
        writeDisparityRaw(false), writePointCloud(false),
        pointCloudMaxDist(0), binaryPointCloud(false),
        disableReception(false), printTimestamps(false),
        zoomPercent(100), displayCoordinate(false), writePgm(false),
        adaptiveColorScale(false), view3D(false), convert12Bit(false), fileNameDateTime(false),
        disparityOffset(0.0) {
    }
};

#endif
