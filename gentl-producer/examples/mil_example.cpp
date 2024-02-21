/*******************************************************************************
 * Copyright (c) 2024 Allied Vision Technologies GmbH
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
* Example for using Nerian devices with the Aurora Imaging Library by Zebra
* (formerly Matrox MIL). For more information about the library please see:
*
* https://www.zebra.com/us/en/software/machine-vision-and-fixed-industrial-scanning-software/aurora-imaging-library.html
********************************************************************************/

// Please link with the mil, mil3d, mil3dmap and milim libraries.

#include <mil.h>
#include <map>
#include <limits>

#define BUFFERING_SIZE  20

// Data structure for storing 2D display related objects
struct Display2d {
    MIL_ID milDisplay;
    MIL_ID buffer;
    MIL_ID lut;
    double maxValue;

    Display2d(): milDisplay(0), buffer(0), lut(0), maxValue(0) {
    }

    ~Display2d() {
        if(lut != 0) {
            MbufFree(lut);
            lut = 0;
        }
        if(buffer != 0) {
            MbufFree(buffer);
            buffer = 0;
        }
        if(milDisplay != 0) {
            MdispFree(milDisplay);
            milDisplay = 0;
        }
    }
};

// Data structure for storing 3D display related objects
struct Display3d {
    MIL_ID container;
    MIL_UNIQUE_3DDISP_ID display3d;

    Display3d(): container(0), display3d(0) {
    }

    ~Display3d() {
        if(container != 0) {
            M3dmapFree(container);
            container = 0;
        }
        if(display3d != 0) {
            display3d = 0;
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
    // Allocate displayable point cloud container
    MbufAllocContainer(milSystem, M_DISP, M_DEFAULT, &display.container);

    // Allocate a 3D view
    display.display3d = M3ddispAlloc(M_DEFAULT_HOST, M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, M_UNIQUE_ID);
    if(!display.display3d) {
        MosPrintf(MIL_TEXT("Failed to initialize 3D view, <Enter> to terminate ... \n"));
        MosGetch();
        exit(1);
    }

    // Show the display.
    MIL_ID MilGraphicList3d;
    M3ddispInquire(display.display3d, M_3D_GRAPHIC_LIST_ID, &MilGraphicList3d);

    M3ddispControl(display.display3d, M_AUTO_ROTATE, M_DISABLE);

    // Configure a view of the scene
    M3ddispSetView(display.display3d, M_VIEWPOINT, 1.0, -1.0, -3.0, M_DEFAULT);
    M3ddispSetView(display.display3d, M_INTEREST_POINT, 0.0, 0.0, 2.0, M_DEFAULT);
    M3ddispSetView(display.display3d, M_UP_VECTOR, 0.0, -1.0, 0.0, M_DEFAULT);

    // Empty at first
    M3ddispSelect(display.display3d, M_NULL, M_OPEN, M_DEFAULT);

    MosPrintf(MIL_TEXT("Rotate with left mouse button drag; pan with right mouse button drag.\n"));
}

// Displays a buffer in a 2d window
void display2dBufferComponent(MIL_ID componentId, Display2d& display) {
    if (display.buffer != 0) {
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
void display3dBufferComponent(MIL_ID componentId, Display3d& display, double maxZ) {
    if (display.container != 0 && display.display3d != 0) {

        // Get image size
        MIL_INT width, height;
        MbufInquire(componentId, M_SIZE_X, &width);
        MbufInquire(componentId, M_SIZE_Y, &height);

        // Get the raw point cloud data (from the GenTL 'Range' component)
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

        // Convert range component buffer to displayable point cloud object
        MbufConvert3d(componentId, display.container, M_NULL, M_DEFAULT, M_DEFAULT);

        // Show point cloud in 3D view
        MIL_INT64 gfx = M3ddispSelect(display.display3d, display.container, M_DEFAULT, M_DEFAULT);
    }
}

// Opens the GenTL device
void openGenTLDevice(MIL_ID &milSystem, MIL_ID& digitizer) {
    // Allocate a GenTL system
    MsysAlloc(M_SYSTEM_GENTL, M_DEV0 + M_GENTL_PRODUCER(0), M_DEFAULT, &milSystem);

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

    // Process each component contained in the container and display it
    std::vector<MIL_ID> components;
    MbufInquireContainer(modifiedBufferId, M_CONTAINER, M_COMPONENT_LIST, components);

    for (size_t i = 0; i < components.size(); i++) {
        MIL_INT64 componentType = 0;
        MIL_STRING componentName;
        MbufInquire(components[i], M_COMPONENT_TYPE, &componentType);
        MbufInquire(components[i], M_COMPONENT_TYPE_NAME, componentName);

        switch(componentType) {
            case 1: // Intensity
                display2dBufferComponent(components[i], userHookDataPtr->leftDisplay);
                break;
            case 8: // Disparity
                display2dBufferComponent(components[i], userHookDataPtr->disparityDisplay);
                break;
            case 4: // Range
                display3dBufferComponent(components[i], userHookDataPtr->pointCloudDisplay, 3.0);
                break;
            default:
                // M_COMPONENT_METADATA (or otherwise unhandled for this example)
                break;
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    // Initialize MIL
    MIL_ID milApplication = 0;
    MappAlloc (M_DEFAULT, &milApplication);

    // Open GenTL device
    MIL_ID milSystem = 0;
    MIL_ID digitizer = 0;
    openGenTLDevice(milSystem, digitizer);
    if(digitizer == 0) {
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
        char txt_left[] = "Left";
        char txt_disp[] = "Disparity";
        alloc2dDisplay(milSystem, userHookData.leftDisplay, MIL_TEXT(txt_left), width, height, false);
        alloc2dDisplay(milSystem, userHookData.disparityDisplay, MIL_TEXT(txt_disp), width, height, true);
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

