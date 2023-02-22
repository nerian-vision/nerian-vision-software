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

/*******************************************************************************
* Example for using Nerian devices with Matrox MIL. For more information about MIL
* please see:
* https://www.matrox.com/imaging/en/products/software/mil/
/*******************************************************************************/

#include <mil.h>
#include <MdispD3D.h>
#include <windows.h>
#include <map>

#define BUFFERING_SIZE  20

// Data structure for storing 2D display related objects
struct Display2d {
    MIL_ID milDisplay;
    MIL_ID buffer;
    MIL_ID lut;
    double maxValue;

    Display2d(): milDisplay(NULL), buffer(NULL), lut(NULL), maxValue(0) {
    }

    ~Display2d() {
        if(lut != NULL) {
            MbufFree(lut);
            lut = NULL;
        }
        if(buffer != NULL) {
            MbufFree(buffer);
            buffer = NULL;
        }
        if(milDisplay != NULL) {
            MdispFree(milDisplay);
            milDisplay = NULL;
        }
    }
};

// Data structure for storing 3D display related objects
struct Display3d {
    MIL_ID container;
    MIL_DISP_D3D_HANDLE display3d;

    Display3d(): container(NULL), display3d(NULL) {
    }

    ~Display3d() {
        if(container != NULL) {
            M3dmapFree(container);
            container = NULL;
        }
        if(display3d != NULL) {
            MdispD3DFree(display3d);
            display3d = NULL;
        }
    }
};

// User's processing function hook data structure.
struct HookDataStruct {
    Display2d leftDisplay;
    Display2d disparityDisplay;
    Display3d pointCloudDisplay;
} ;

// Allocate a 2D display for the left camera image or disparity map
void alloc2dDisplay(MIL_ID milSystem, Display2d& display, MIL_TEXT_PTR title, int width, int height, bool disparity) {
    // For disparity maps we only need one color channel
    int colorChannels = (disparity ? 1 : 3);

    // Create display
    MdispAlloc(milSystem, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, &display.milDisplay);
    MdispControl(display.milDisplay, M_TITLE, title);

    // Create buffer
    MbufAllocColor(milSystem, colorChannels, width, height, 16, M_IMAGE + M_DISP + M_PROC, &display.buffer);
    MbufClear(display.buffer, M_COLOR_BLACK);

    // For disparity windows create an LUT
    if(disparity) {
        MbufAllocColor(milSystem, 3, 4095, 1, 8 + M_UNSIGNED, M_LUT, &display.lut);
        MbufClear(display.lut, M_COLOR_BLACK);

        MIL_ID MilDispLutChild = MbufChild1d(display.lut, 0, 4095, M_NULL);
        MgenLutFunction(MilDispLutChild, M_COLORMAP_JET , M_DEFAULT, M_DEFAULT, M_DEFAULT, M_DEFAULT, M_DEFAULT, M_DEFAULT);
        MbufFree(MilDispLutChild);

        MdispLut(display.milDisplay, display.lut);

        display.maxValue = 4095;
    } else {
        display.maxValue = -1; // Will be set later
    }

    // Associate buffer with the display
    MdispSelect(display.milDisplay, display.buffer);
}

// Allocates a 3D display for the point cloud
void alloc3dDisplay(MIL_ID milSystem, Display3d& display, int width, int height) {
    // Allocate point cloud container
    M3dmapAllocResult(milSystem, M_POINT_CLOUD_CONTAINER, M_DEFAULT, &display.container);

    // Allocate a 3D view
    display.display3d = MPtCldD3DAlloc(display.container, M_DEFAULT, true, width, height, 0);

    // Set default viewing position
    MdispD3DControl(display.display3d, MD3D_ROTATE, MD3D_FALSE);
    MdispD3DControl(display.display3d, MD3D_EYE_DIST, 3.0);
    MdispD3DControl(display.display3d, MD3D_EYE_THETA, 90.0);
    MdispD3DControl(display.display3d, MD3D_EYE_PHI, -10.0);
    MdispD3DControl(display.display3d, MD3D_LOOK_AT_X, 0.2);
    MdispD3DControl(display.display3d, MD3D_LOOK_AT_Y, 0.2);
    MdispD3DControl(display.display3d, MD3D_LOOK_AT_Z, 3.0);

    // Print usage help
    MdispD3DPrintHelp(display.display3d);
}

// Displays a buffer in a 2d window
void display2dBufferComponent(MIL_ID modifiedBufferId, Display2d& display, int component) {
    MIL_ID componentId = NULL;
    MbufInquire(modifiedBufferId, M_COMPONENT_ID_BY_INDEX(component), &componentId);

    if (componentId && display.buffer != NULL) {
        if(display.maxValue <= 0) {
            double maxVal = 0.0;
            MbufInquire(componentId, M_MAX, &maxVal);

            if (maxVal != 0.0 && maxVal != display.maxValue) {
                MbufControl(display.buffer, M_MAX, maxVal);
                display.maxValue = maxVal;
            }
        }
        MbufCopy(componentId, display.buffer);
    }
}

// Displays a 3d buffer in a 3d window
void display3dBufferComponent(MIL_ID modifiedBufferId, Display3d& display, int component, double maxZ) {
    MIL_ID componentId = NULL;
    MbufInquire(modifiedBufferId, M_COMPONENT_ID_BY_INDEX(component), &componentId);

    if (componentId && display.container != NULL && display.display3d != NULL) {
        // Get image size
        MIL_INT width, height;
        MbufInquire(componentId, M_SIZE_X, &width);
        MbufInquire(componentId, M_SIZE_Y, &height);

        // Get the raw point cloud data
        float* cloudData = (float*)MbufInquire(componentId, M_HOST_ADDRESS, M_NULL);

        // Remove all points with z > maxZ by setting them to NaN
        if(maxZ > 0) {
            float* endPtr = &cloudData[width*height*3];
            for(float* cloudPtr = &cloudData[2]; cloudPtr < endPtr; cloudPtr += 3) {
                if(*cloudPtr > maxZ) {
                    cloudPtr[0] = std::numeric_limits<float>::quiet_NaN();
                    cloudPtr[-1] = std::numeric_limits<float>::quiet_NaN();
                    cloudPtr[-2] = std::numeric_limits<float>::quiet_NaN();
                }
            }
        }

        // Put the point cloud in the container.
        M3dmapPut(display.container, M_POINT_CLOUD_LABEL(1), M_XYZ, 32 + M_FLOAT, width*height*3,
            cloudData, M_NULL, M_NULL, M_NULL, M_DEFAULT);

        // Show 3D view
        MPtCldD3DSetPointCloud(display.display3d, display.container, M_POINT_CLOUD_LABEL(1), true);
        MdispD3DShow(display.display3d);
    }
}

// Opens the GenTL device
void openGenTLDevice(MIL_ID &milSystem, MIL_ID& digitizer) {
    // Allocate a GenTL system
    MsysAlloc(M_SYSTEM_GENTL, M_DEV0 + M_GENTL_PRODUCER(2), M_DEFAULT, &milSystem);

    // Get the GenTL Producer info via the GenTL System module XML.
    MIL_STRING Vendor, Model, Version, Type;
    MsysInquireFeature(milSystem, M_GENTL_SYSTEM + M_FEATURE_VALUE, MIL_TEXT("TLVendorName"), M_TYPE_STRING, Vendor);
    MsysInquireFeature(milSystem, M_GENTL_SYSTEM + M_FEATURE_VALUE, MIL_TEXT("TLModelName"), M_TYPE_STRING, Model);
    MsysInquireFeature(milSystem, M_GENTL_SYSTEM + M_FEATURE_VALUE, MIL_TEXT("TLVersion"), M_TYPE_STRING, Version);
    MsysInquireFeature(milSystem, M_GENTL_SYSTEM + M_FEATURE_VALUE, MIL_TEXT("TLType"), M_TYPE_STRING, Type);

    MosPrintf(MIL_TEXT("\n-------------------------- GenTL producer information --------------------------\n"));
    MosPrintf(MIL_TEXT("Vendor:               %s.\n"), Vendor.c_str());
    MosPrintf(MIL_TEXT("Model:                %s.\n"), Model.c_str());
    MosPrintf(MIL_TEXT("Version:              %s.\n"), Version.c_str());
    MosPrintf(MIL_TEXT("Transport layer type: %s.\n"), Type.c_str());

    // Allocate a digitizer, using the first device which is the multipart device
    MdigAlloc(milSystem, M_DEV0, MIL_TEXT("M_DEFAULT"), M_DEFAULT, &digitizer);

    // Display feature browser
    MdigControl(digitizer, M_GC_FEATURE_BROWSER, M_OPEN + M_ASYNCHRONOUS);
}

// Allocates the buffer containers
void allocateContainers(MIL_ID milSystem, MIL_ID digitizer, MIL_ID* containers, int count) {
    for (int i = 0; i < count; i++) {
        MbufAllocDefault(milSystem, digitizer,
            M_CONTAINER + M_3D_SCENE + M_GRAB + M_PROC,
            M_DEFAULT,
            M_DEFAULT,
            &containers[i]);
    }
}

// User's processing function called every time a grab buffer is ready.
MIL_INT MFTYPE processingFunction(MIL_INT hookType, MIL_ID hookId, void* hookDataPtr) {
    HookDataStruct *userHookDataPtr = (HookDataStruct *)hookDataPtr;
    MIL_ID modifiedBufferId = M_NULL;
    MIL_INT64 attribute = 0;

    // Retrieve the MIL_ID of the grabbed buffer; a M_3D_SCENE in this case.
    MdigGetHookInfo(hookId, M_MODIFIED_BUFFER + M_BUFFER_ID, &modifiedBufferId);
    MbufInquire(modifiedBufferId, M_EXTENDED_ATTRIBUTE, &attribute);

    if (M_IS_3D_SCENE(attribute)) {
        display2dBufferComponent(modifiedBufferId, userHookDataPtr->leftDisplay, 0);
        display2dBufferComponent(modifiedBufferId, userHookDataPtr->disparityDisplay, 1);
        display3dBufferComponent(modifiedBufferId, userHookDataPtr->pointCloudDisplay, 2, 3.0);
    }
    else {
        MosPrintf(MIL_TEXT("Error in ProcessingFunction: Expected a M_3D_SCENE.\n"));
    }

    return 0;
}

int main(int argc, char** argv) {
    // Initialize MIL
    MIL_ID milApplication = NULL;
    MappAlloc (M_DEFAULT, &milApplication);

    // Open GenTL device
    MIL_ID milSystem = NULL;
    MIL_ID digitizer = NULL;
    openGenTLDevice(milSystem, digitizer);
    if(digitizer == NULL) {
        return -1;
    }

    // Allocate containers
    MIL_ID milContainers[BUFFERING_SIZE] = { 0 };
    allocateContainers(milSystem, digitizer, milContainers, BUFFERING_SIZE);

    // Inquire the image size
    MIL_INT width, height;
    MdigInquireFeature(digitizer, M_FEATURE_VALUE, MIL_TEXT("Width"), M_TYPE_MIL_INT, &width);
    MdigInquireFeature(digitizer, M_FEATURE_VALUE, MIL_TEXT("Height"), M_TYPE_MIL_INT, &height);

    // Make sure userHookData will be freed before milApplication
    {
        // Initialize the user's processing function data structure.
        HookDataStruct userHookData;
        alloc2dDisplay(milSystem, userHookData.leftDisplay, MIL_TEXT("Left"), width, height, false);
        alloc2dDisplay(milSystem, userHookData.disparityDisplay, MIL_TEXT("Disparity"), width, height, true);
        alloc3dDisplay(milSystem, userHookData.pointCloudDisplay, 800, 600);

        // Start the processing. The processing function is called with every frame grabbed.
        MdigProcess(digitizer, milContainers, BUFFERING_SIZE,
            M_START, M_DEFAULT, processingFunction, &userHookData);

        // Here the main() is free to perform other tasks while the processing is executing.
        // ---------------------------------------------------------------------------------
        // Print a message and wait for a key press after a minimum number of frames.
        MosPrintf(MIL_TEXT("Press <Enter> to stop.                    \n\n"));
        MosGetch();

        // Stop the processing.
        MdigProcess(digitizer, milContainers, BUFFERING_SIZE,
            M_STOP, M_DEFAULT, processingFunction, &userHookData);
    }

    // Print statistics.
    MIL_INT processFrameCount = 0;
    MIL_DOUBLE processFrameRate = 0;
    MdigInquire(digitizer, M_PROCESS_FRAME_COUNT, &processFrameCount);
    MdigInquire(digitizer, M_PROCESS_FRAME_RATE, &processFrameRate);
    MosPrintf(MIL_TEXT("\n\n%d frames grabbed at %.1f frames/sec (%.1f ms/frame).\n"),
        (int)processFrameCount, processFrameRate, 1000.0 / processFrameRate);
    MosPrintf(MIL_TEXT("Press <Enter> to end.\n\n"));
    MosGetch();

    // Free the containers.
    for (MIL_INT i = 0; i < BUFFERING_SIZE; i++) {
        MbufFree(milContainers[i]);
    }

    MdigFree(digitizer);
    MsysFree(milSystem);
    MappFree(milApplication);
}
