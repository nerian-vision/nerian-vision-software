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

// Activate DLL export for GenTL
#define GCTLIDLL

#include "misc/common.h"
#include "system/library.h"
#include "system/system.h"
#include "interface/interface.h"
#include "device/logicaldevice.h"
#include "device/physicaldevice.h"
#include "stream/datastream.h"
#include "event/event.h"

#include <genicam/gentl.h>
#include <iostream>
#include <fstream>
#include <exception>

#ifdef ENABLE_DEBUGGING
#include "misc/backward.hpp"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * This is the adapter layer that maps calls to the GenTL C interface to
 * method calls of the internal objects.
 */

namespace GenTL {

/*
* Debugging macros
*/
#ifdef ENABLE_DEBUGGING

#ifdef _WIN32
    std::fstream debugStream("C:\\debug\\gentl-debug-" + std::to_string(time(nullptr)) + ".txt", std::ios::out);
#define DEBUG_HIGHLIGHT_PREFIX ""
#define DEBUG_HIGHLIGHT_SUFFIX ""
#else
    std::ostream& debugStream = std::cout;
    // If desired, set escape sequence for highlighting
#define DEBUG_HIGHLIGHT_PREFIX ""
#define DEBUG_HIGHLIGHT_SUFFIX ""
#endif

#define DEBUG_VAL(val)      #val << " = " << (std::string(#val) == "iAddress" ? std::hex : std::dec) << val
#define DEBUG               {debugStream << DEBUG_HIGHLIGHT_PREFIX << __FUNCTION__ << DEBUG_HIGHLIGHT_SUFFIX << std::endl;}
#define DEBUG1(a)           {debugStream << DEBUG_HIGHLIGHT_PREFIX << __FUNCTION__ << ": " << DEBUG_VAL(a) << DEBUG_HIGHLIGHT_SUFFIX << std::endl;}
#define DEBUG2(a, b)        {debugStream << DEBUG_HIGHLIGHT_PREFIX << __FUNCTION__ << ": " << DEBUG_VAL(a) << "; " << DEBUG_VAL(b) << DEBUG_HIGHLIGHT_SUFFIX << std::endl;}
#define DEBUG3(a, b, c)     {debugStream << DEBUG_HIGHLIGHT_PREFIX << __FUNCTION__ << ": " << DEBUG_VAL(a) << "; " << DEBUG_VAL(b) \
    << "; " << DEBUG_VAL(c) << DEBUG_HIGHLIGHT_SUFFIX << std::endl;}
#define DEBUG4(a, b, c, d)  {debugStream << DEBUG_HIGHLIGHT_PREFIX << __FUNCTION__ << ": " << DEBUG_VAL(a) << "; " << DEBUG_VAL(b) \
    << "; " << DEBUG_VAL(c) << "; " << DEBUG_VAL(d) << DEBUG_HIGHLIGHT_SUFFIX << std::endl;}
#define DEBUG5(a, b, c, d, e)  {debugStream << DEBUG_HIGHLIGHT_PREFIX << __FUNCTION__ << ": " << DEBUG_VAL(a) << "; " << DEBUG_VAL(b) \
    << "; " << DEBUG_VAL(c) << "; " << DEBUG_VAL(d) << "; " << DEBUG_VAL(e) << DEBUG_HIGHLIGHT_SUFFIX << std::endl;}
#define DEBUG6(a, b, c, d, e, f)  {debugStream << DEBUG_HIGHLIGHT_PREFIX << __FUNCTION__ << ": " << DEBUG_VAL(a) << "; " << DEBUG_VAL(b) \
    << "; " << DEBUG_VAL(c) << "; " << DEBUG_VAL(d) << "; " << DEBUG_VAL(e) << "; " << DEBUG_VAL(f) << DEBUG_HIGHLIGHT_SUFFIX << std::endl;}
#define DEBUG7(a, b, c, d, e, f, g)  {debugStream << DEBUG_HIGHLIGHT_PREFIX << __FUNCTION__ << ": " << DEBUG_VAL(a) << "; " << DEBUG_VAL(b) \
    << "; " << DEBUG_VAL(c) << "; " << DEBUG_VAL(d) << "; " << DEBUG_VAL(e) << "; " << DEBUG_VAL(f) \
    << "; " << DEBUG_VAL(g) << DEBUG_HIGHLIGHT_SUFFIX << std::endl;}


#else

#define DEBUG                          {}
#define DEBUG1(a)                      {}
#define DEBUG2(a, b)                   {}
#define DEBUG3(a, b, c)                {}
#define DEBUG4(a, b, c, d)             {}
#define DEBUG5(a, b, c, d, e)          {}
#define DEBUG6(a, b, c, d, e, f)       {}
#define DEBUG7(a, b, c, d, e, f, g)    {}


#endif

/*
* Macros for try/catch
*/

#define GENTL_TRY try {

#ifdef _WIN32
#ifdef ENABLE_DEBUGGING
// Debugging mode also logs exception and backtrace to log file
#define GENTL_CATCH } catch (const std::exception& ex) {\
    std::string msgString = "Exception occurred: " + std::string(ex.what()); \
    debugStream << msgString << std::endl; \
    backward::StackTrace st; st.load_here(32); \
    backward::Printer p; p.print(st, debugStream); \
    MessageBox(NULL, msgString.c_str(), "Nerian GenTL", MB_ICONERROR); \
    return GC_ERR_ERROR;\
}
#else
#define GENTL_CATCH } catch (const std::exception& ex) {\
    std::string msgString = "Exception occurred: " + std::string(ex.what()); \
    MessageBox(NULL, msgString.c_str(), "Nerian GenTL", MB_ICONERROR); \
    return GC_ERR_ERROR;\
}
#endif
#else
#ifdef ENABLE_DEBUGGING
#define GENTL_CATCH } catch (const std::exception& ex) {\
    std::cout << "Exception: " << ex.what(); \
    backward::StackTrace st; st.load_here(32); \
    backward::Printer p; p.print(st, std::cout); \
    return GC_ERR_ERROR;\
}
#else
#define GENTL_CATCH } catch (const std::exception& ex) {\
    std::cout << "Exception: " << ex.what(); \
    return GC_ERR_ERROR;\
}
#endif
#endif


thread_local GenTL::GC_ERROR lastError = GenTL::GC_ERR_SUCCESS;

// Verifies that the given handle is valid
inline bool verifyHandle(void* handle, Handle::HandleType type) {
    return handle != nullptr && reinterpret_cast<Handle*>(handle)->getType() == type;
}

inline GenTL::GC_ERROR setLastError(GenTL::GC_ERROR error) {
    if(error != GenTL::GC_ERR_SUCCESS) {
        lastError = error;
    }
    return error;
}

/*
 * Library functions
 */

GC_API GCInitLib(void) {
    GENTL_TRY;
    DEBUG;
    return setLastError(Library::initLib());
    GENTL_CATCH;
}

GC_API GCCloseLib(void) {
    GENTL_TRY;
    DEBUG;
    return setLastError(Library::closeLib());
    GENTL_CATCH;
}

GC_API GCGetInfo(TL_INFO_CMD iInfoCmd, INFO_DATATYPE* piType, void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG1(iInfoCmd);
    return setLastError(Library::getInfo(iInfoCmd, piType, pBuffer, piSize));
    GENTL_CATCH;
}

GC_API GCGetLastError(GC_ERROR* piErrorCode, char* sErrorText, size_t* piSize) {
    GENTL_TRY;
    DEBUG;
    return setLastError(Library::getLastError(piErrorCode, sErrorText, piSize, lastError));
    GENTL_CATCH;
}


/*
 * System functions
 */

GC_API TLOpen(TL_HANDLE* phSystem) {
    GENTL_TRY;
    DEBUG;
    return setLastError(System::open(reinterpret_cast<System**>(phSystem)));
    GENTL_CATCH;
}

GC_API TLClose(TL_HANDLE hSystem) {
    GENTL_TRY;
    DEBUG1(hSystem);

    if(!verifyHandle(hSystem, Handle::TYPE_SYSTEM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(System::close(reinterpret_cast<System*>(hSystem)));
    }

    GENTL_CATCH;
}

GC_API TLUpdateInterfaceList(TL_HANDLE hSystem, bool8_t* pbChanged, uint64_t iTimeout) {
    GENTL_TRY;
    DEBUG1(hSystem);

    if(!verifyHandle(hSystem, Handle::TYPE_SYSTEM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<System*>(hSystem)->updateInterfaceList(pbChanged, iTimeout));
    }

    GENTL_CATCH;
}

GC_API TLGetNumInterfaces(TL_HANDLE hSystem, uint32_t* piNumIfaces) {
    GENTL_TRY;
    DEBUG1(hSystem);

    if(!verifyHandle(hSystem, Handle::TYPE_SYSTEM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<System*>(hSystem)->getNumInterfaces(piNumIfaces));
    }

    GENTL_CATCH;
}

GC_API TLGetInterfaceID(TL_HANDLE hSystem, uint32_t iIndex, char* sIfaceID, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hSystem, iIndex);

    if(!verifyHandle(hSystem, Handle::TYPE_SYSTEM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<System*>(hSystem)->getInterfaceID(iIndex, sIfaceID, piSize));
    }

    GENTL_CATCH;
}

GC_API TLGetInterfaceInfo(TL_HANDLE hSystem, const char* sIfaceID, INTERFACE_INFO_CMD iInfoCmd,
        INFO_DATATYPE* piType, void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG3(hSystem, sIfaceID, iInfoCmd);

    if(!verifyHandle(hSystem, Handle::TYPE_SYSTEM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<System*>(hSystem)->getInterfaceInfo(sIfaceID, iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}

GC_API TLOpenInterface(TL_HANDLE hSystem, const char* sIfaceID, IF_HANDLE* phIface) {
    GENTL_TRY;
    DEBUG2(hSystem, sIfaceID);

    if(!verifyHandle(hSystem, Handle::TYPE_SYSTEM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<System*>(hSystem)->openInterface(sIfaceID, phIface));
    }

    GENTL_CATCH;
}

GC_API TLGetInfo(TL_HANDLE hSystem, TL_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hSystem, iInfoCmd);

    if(!verifyHandle(hSystem, Handle::TYPE_SYSTEM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<System*>(hSystem)->getInfo(iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}


/*
 * Interface functions
 */

GC_API IFClose(IF_HANDLE hIface) {
    GENTL_TRY;
    DEBUG1(hIface);

    if(!verifyHandle(hIface, Handle::TYPE_INTERFACE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Interface*>(hIface)->close());
    }

    GENTL_CATCH;
}

GC_API IFUpdateDeviceList(IF_HANDLE hIface, bool8_t* pbChanged, uint64_t iTimeout) {
    GENTL_TRY;
    DEBUG2(hIface, iTimeout);

    if(!verifyHandle(hIface, Handle::TYPE_INTERFACE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Interface*>(hIface)->updateDeviceList(pbChanged, iTimeout));
    }

    GENTL_CATCH;
}

GC_API IFGetNumDevices(IF_HANDLE hIface, uint32_t* piNumDevices) {
    GENTL_TRY;
    DEBUG1(hIface);

    if(!verifyHandle(hIface, Handle::TYPE_INTERFACE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Interface*>(hIface)->getNumDevices(piNumDevices));
    }

    GENTL_CATCH;
}

GC_API IFGetDeviceID(IF_HANDLE hIface, uint32_t iIndex, char* sDeviceID, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hIface, iIndex);

    if(!verifyHandle(hIface, Handle::TYPE_INTERFACE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Interface*>(hIface)->getDeviceID(iIndex, sDeviceID, piSize));
    }

    GENTL_CATCH;
}

GC_API IFGetDeviceInfo(IF_HANDLE hIface, const char* sDeviceID, DEVICE_INFO_CMD iInfoCmd,
        INFO_DATATYPE* piType, void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG3(hIface, sDeviceID, iInfoCmd);

    if(!verifyHandle(hIface, Handle::TYPE_INTERFACE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Interface*>(hIface)->getDeviceInfo(sDeviceID, iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}

GC_API IFGetParentTL(IF_HANDLE hIface, TL_HANDLE* phSystem) {
    GENTL_TRY;
    DEBUG2(hIface, phSystem);

    if(!verifyHandle(hIface, Handle::TYPE_INTERFACE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Interface*>(hIface)->getParentTL(phSystem));
    }

    GENTL_CATCH;
}

GC_API IFGetInfo(IF_HANDLE hIface, INTERFACE_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hIface, iInfoCmd);

    if(!verifyHandle(hIface, Handle::TYPE_INTERFACE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Interface*>(hIface)->getInfo(iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}

GC_API IFOpenDevice(IF_HANDLE hIface, const char* sDeviceID, DEVICE_ACCESS_FLAGS iOpenFlags,
        DEV_HANDLE* phDevice) {
    GENTL_TRY;
    DEBUG3(hIface, sDeviceID, iOpenFlags);

    if(!verifyHandle(hIface, Handle::TYPE_INTERFACE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Interface*>(hIface)->openDevice(sDeviceID, iOpenFlags, phDevice));
    }

    GENTL_CATCH;
}


/*
 * Device functions
 */

GC_API DevClose(DEV_HANDLE hDevice) {
    GENTL_TRY;
    DEBUG1(hDevice);

    if(!verifyHandle(hDevice, Handle::TYPE_DEVICE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        GenTL::GC_ERROR error =  reinterpret_cast<LogicalDevice*>(hDevice)->close();
        reinterpret_cast<LogicalDevice*>(hDevice)->getPhysicalDevice()->getInterface()->getSystem()->freeUnusedDevices();
        return setLastError(error);
    }

    GENTL_CATCH;
}

GC_API DevGetParentIF(DEV_HANDLE hDevice, IF_HANDLE* phIface) {
    GENTL_TRY;
    DEBUG1(hDevice);

    if(!verifyHandle(hDevice, Handle::TYPE_DEVICE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<LogicalDevice*>(hDevice)->getParentIF(phIface));
    }

    GENTL_CATCH;
}

GC_API DevGetInfo(DEV_HANDLE hDevice, DEVICE_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hDevice, iInfoCmd);

    if(!verifyHandle(hDevice, Handle::TYPE_DEVICE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<LogicalDevice*>(hDevice)->getInfo(
            iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}

GC_API DevGetNumDataStreams(DEV_HANDLE hDevice, uint32_t* piNumDataStreams) {
    GENTL_TRY;
    DEBUG1(hDevice);

    if(!verifyHandle(hDevice, Handle::TYPE_DEVICE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<LogicalDevice*>(hDevice)->getNumDataStreams(piNumDataStreams));
    }

    GENTL_CATCH;
}

GC_API DevGetPort(DEV_HANDLE hDevice, PORT_HANDLE* phRemoteDev) {
    GENTL_TRY;
    DEBUG1(hDevice);

    if(!verifyHandle(hDevice, Handle::TYPE_DEVICE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<LogicalDevice*>(hDevice)->getPort(phRemoteDev));
    }

    GENTL_CATCH;
}

GC_API DevGetDataStreamID(DEV_HANDLE hDevice, uint32_t iIndex, char* sDataStreamID,
        size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hDevice, iIndex);

    if(!verifyHandle(hDevice, Handle::TYPE_DEVICE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<LogicalDevice*>(hDevice)->getDataStreamID(
            iIndex, sDataStreamID, piSize));
    }

    GENTL_CATCH;
}

GC_API DevOpenDataStream(DEV_HANDLE hDevice, const char* sDataStreamID, DS_HANDLE* phDataStream) {
    GENTL_TRY;
    DEBUG2(hDevice, sDataStreamID);

    if(!verifyHandle(hDevice, Handle::TYPE_DEVICE)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<LogicalDevice*>(hDevice)->openDataStream(sDataStreamID, phDataStream));
    }

    GENTL_CATCH;
}


/*
 * Data stream functions
 */

GC_API DSAnnounceBuffer(DS_HANDLE hDataStream, void* pBuffer, size_t iSize, void* pPrivate, BUFFER_HANDLE* phBuffer) {
    GENTL_TRY;
    DEBUG4(hDataStream, pBuffer, iSize, pPrivate);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->announceBuffer(pBuffer, iSize, pPrivate, phBuffer));
    }

    GENTL_CATCH;
}

GC_API DSAllocAndAnnounceBuffer(DS_HANDLE hDataStream, size_t iBufferSize, void* pPrivate, BUFFER_HANDLE* phBuffer) {
    GENTL_TRY;
    DEBUG2(hDataStream, iBufferSize);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->allocAndAnnounceBuffer(iBufferSize, pPrivate, phBuffer));
    }

    GENTL_CATCH;
}

GC_API DSClose(DS_HANDLE hDataStream) {
    GENTL_TRY;
    DEBUG1(hDataStream);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->close());
    }

    GENTL_CATCH;
}

GC_API DSRevokeBuffer(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, void ** ppBuffer, void ** ppPrivate) {
    GENTL_TRY;
    DEBUG2(hDataStream, hBuffer);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->revokeBuffer(hBuffer, ppBuffer, ppPrivate));
    }

    GENTL_CATCH;
}

GC_API DSQueueBuffer(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer) {
    GENTL_TRY;
    DEBUG2(hDataStream, hBuffer);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->queueBuffer(hBuffer));
    }

    GENTL_CATCH;
}

GC_API DSGetParentDev(DS_HANDLE hDataStream, DEV_HANDLE* phDevice) {
    GENTL_TRY;
    DEBUG1(hDataStream);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getParentDev(phDevice));
    }

    GENTL_CATCH;
}

GC_API DSStartAcquisition(DS_HANDLE hDataStream, ACQ_START_FLAGS iStartFlags, uint64_t iNumToAcquire) {
    GENTL_TRY;
    DEBUG2(hDataStream, iNumToAcquire);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->startAcquisition(iStartFlags, iNumToAcquire));
    }

    GENTL_CATCH;
}

GC_API DSStopAcquisition(DS_HANDLE hDataStream, ACQ_STOP_FLAGS iStopFlags) {
    GENTL_TRY;
    DEBUG1(hDataStream);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->stopAcquisition(iStopFlags));
    }

    GENTL_CATCH;
}

GC_API DSFlushQueue(DS_HANDLE hDataStream, ACQ_QUEUE_TYPE iOperation) {
    GENTL_TRY;
    DEBUG2(hDataStream, iOperation);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->flushQueue(iOperation));
    }

    GENTL_CATCH;
}

GC_API DSGetBufferChunkData(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, SINGLE_CHUNK_DATA* pChunkData,
        size_t* piNumChunks) {
    GENTL_TRY;
    DEBUG2(hDataStream, hBuffer);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getBufferChunkData(hBuffer, pChunkData, piNumChunks));
    }

    GENTL_CATCH;
}

GC_API DSGetBufferID(DS_HANDLE hDataStream, uint32_t iIndex, BUFFER_HANDLE* phBuffer) {
    GENTL_TRY;
    DEBUG2(hDataStream, iIndex);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getBufferID(iIndex, phBuffer));
    }

    GENTL_CATCH;
}

GC_API DSGetBufferInfo(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, BUFFER_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG3(hDataStream, hBuffer, iInfoCmd);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getBufferInfo(hBuffer, iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}

GC_API DSGetInfo(DS_HANDLE hDataStream, STREAM_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hDataStream, iInfoCmd);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getInfo(iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}

GC_API DSGetNumBufferParts(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumParts ) {
    GENTL_TRY;
    DEBUG2(hDataStream, hBuffer);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getNumBufferParts(hBuffer, piNumParts));
    }

    GENTL_CATCH;
}

GC_API DSGetBufferPartInfo(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iPartIndex,
        BUFFER_PART_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize) {
    GENTL_TRY;
    DEBUG4(hDataStream, hBuffer, iPartIndex, iInfoCmd);

    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getBufferPartInfo(hBuffer, iPartIndex,
            iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}


/*
 * Port functions
 */

GC_API GCGetPortInfo(PORT_HANDLE hPort, PORT_INFO_CMD iInfoCmd, INFO_DATATYPE* piType,
        void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hPort, iInfoCmd);

    if(verifyHandle(hPort, Handle::TYPE_PORT)) {
        return setLastError(reinterpret_cast<Port*>(hPort)->getPortInfo(iInfoCmd, piType, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_SYSTEM)) {
        return setLastError(reinterpret_cast<System*>(hPort)->getPort()->getPortInfo(iInfoCmd, piType, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_INTERFACE)) {
        return setLastError(reinterpret_cast<Interface*>(hPort)->getPort()->getPortInfo(iInfoCmd, piType, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_DEVICE)) {
        return setLastError(reinterpret_cast<LogicalDevice*>(hPort)->getLocalPort()->getPortInfo(iInfoCmd, piType, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_STREAM)) {
        return setLastError(reinterpret_cast<DataStream*>(hPort)->getPort()->getPortInfo(iInfoCmd, piType, pBuffer, piSize));
    } else {
        return setLastError(GC_ERR_INVALID_HANDLE);
    }

    GENTL_CATCH;
}

GC_API GCGetPortURL(PORT_HANDLE hPort, char* sURL, size_t* piSize) {
    GENTL_TRY;
    DEBUG1(hPort);

    if(verifyHandle(hPort, Handle::TYPE_PORT)) {
        return setLastError(reinterpret_cast<Port*>(hPort)->getPortURL(sURL, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_SYSTEM)) {
        return setLastError(reinterpret_cast<System*>(hPort)->getPort()->getPortURL(sURL, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_INTERFACE)) {
        return setLastError(reinterpret_cast<Interface*>(hPort)->getPort()->getPortURL(sURL, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_DEVICE)) {
        return setLastError(reinterpret_cast<LogicalDevice*>(hPort)->getLocalPort()->getPortURL(sURL, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_STREAM)) {
        return setLastError(reinterpret_cast<DataStream*>(hPort)->getPort()->getPortURL(sURL, piSize));
    } else {
        return setLastError(GC_ERR_INVALID_HANDLE);
    }

    GENTL_CATCH;
}

GC_API GCGetNumPortURLs(PORT_HANDLE hPort, uint32_t* piNumURLs) {
    GENTL_TRY;
    DEBUG1(hPort);

    if(verifyHandle(hPort, Handle::TYPE_PORT)) {
        return setLastError(reinterpret_cast<Port*>(hPort)->getNumPortURLs(piNumURLs));
    } else if(verifyHandle(hPort, Handle::TYPE_SYSTEM)) {
        return setLastError(reinterpret_cast<System*>(hPort)->getPort()->getNumPortURLs(piNumURLs));
    } else if(verifyHandle(hPort, Handle::TYPE_INTERFACE)) {
        return setLastError(reinterpret_cast<Interface*>(hPort)->getPort()->getNumPortURLs(piNumURLs));
    } else if(verifyHandle(hPort, Handle::TYPE_DEVICE)) {
        return setLastError(reinterpret_cast<LogicalDevice*>(hPort)->getLocalPort()->getNumPortURLs(piNumURLs));
    } else if(verifyHandle(hPort, Handle::TYPE_STREAM)) {
        return setLastError(reinterpret_cast<DataStream*>(hPort)->getPort()->getNumPortURLs(piNumURLs));
    } else {
        return setLastError(GC_ERR_INVALID_HANDLE);
    }

    GENTL_CATCH;
}

GC_API GCGetPortURLInfo(PORT_HANDLE hPort, uint32_t iURLIndex, URL_INFO_CMD iInfoCmd,
        INFO_DATATYPE* piType, void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG3(hPort, iURLIndex, iInfoCmd);

    if(verifyHandle(hPort, Handle::TYPE_PORT)) {
        return setLastError(reinterpret_cast<Port*>(hPort)->getPortURLInfo(iURLIndex, iInfoCmd, piType, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_SYSTEM)) {
        return setLastError(reinterpret_cast<System*>(hPort)->getPort()->getPortURLInfo(iURLIndex, iInfoCmd, piType, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_INTERFACE)) {
        return setLastError(reinterpret_cast<Interface*>(hPort)->getPort()->getPortURLInfo(iURLIndex, iInfoCmd, piType, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_DEVICE)) {
        return setLastError(reinterpret_cast<LogicalDevice*>(hPort)->getLocalPort()->getPortURLInfo(iURLIndex, iInfoCmd, piType, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_STREAM)) {
        return setLastError(reinterpret_cast<DataStream*>(hPort)->getPort()->getPortURLInfo(iURLIndex, iInfoCmd, piType, pBuffer, piSize));
    } else {
        return setLastError(GC_ERR_INVALID_HANDLE);
    }

    GENTL_CATCH;
}

GC_API GCReadPort(PORT_HANDLE hPort, uint64_t iAddress, void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hPort, iAddress);

    if(verifyHandle(hPort, Handle::TYPE_PORT)) {
        return setLastError(reinterpret_cast<Port*>(hPort)->readPort(iAddress, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_SYSTEM)) {
        return setLastError(reinterpret_cast<System*>(hPort)->getPort()->readPort(iAddress, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_INTERFACE)) {
        return setLastError(reinterpret_cast<Interface*>(hPort)->getPort()->readPort(iAddress, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_DEVICE)) {
        return setLastError(reinterpret_cast<LogicalDevice*>(hPort)->getLocalPort()->readPort(iAddress, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_STREAM)) {
        return setLastError(reinterpret_cast<DataStream*>(hPort)->getPort()->readPort(iAddress, pBuffer, piSize));
    } else {
        return setLastError(GC_ERR_INVALID_HANDLE);
    }

    GENTL_CATCH;
}

GC_API GCWritePort(PORT_HANDLE hPort, uint64_t iAddress, const void* pBuffer, size_t* piSize) {
    GENTL_TRY;
    DEBUG2(hPort, iAddress);

    if(verifyHandle(hPort, Handle::TYPE_PORT)) {
        return setLastError(reinterpret_cast<Port*>(hPort)->writePort(iAddress, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_SYSTEM)) {
        return setLastError(reinterpret_cast<System*>(hPort)->getPort()->writePort(iAddress, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_INTERFACE)) {
        return setLastError(reinterpret_cast<Interface*>(hPort)->getPort()->writePort(iAddress, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_DEVICE)) {
        return setLastError(reinterpret_cast<LogicalDevice*>(hPort)->getLocalPort()->writePort(iAddress, pBuffer, piSize));
    } else if(verifyHandle(hPort, Handle::TYPE_STREAM)) {
        return setLastError(reinterpret_cast<DataStream*>(hPort)->getPort()->writePort(iAddress, pBuffer, piSize));
    } else {
        return setLastError(GC_ERR_INVALID_HANDLE);
    }

    GENTL_CATCH;
}

GC_API GCWritePortStacked(PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY* pEntries, size_t* piNumEntries) {
    GENTL_TRY;
    DEBUG1(hPort);

    if(verifyHandle(hPort, Handle::TYPE_PORT)) {
        return setLastError(reinterpret_cast<Port*>(hPort)->writePortStacked(pEntries, piNumEntries));
    } else if(verifyHandle(hPort, Handle::TYPE_SYSTEM)) {
        return setLastError(reinterpret_cast<System*>(hPort)->getPort()->writePortStacked(pEntries, piNumEntries));
    } else if(verifyHandle(hPort, Handle::TYPE_INTERFACE)) {
        return setLastError(reinterpret_cast<Interface*>(hPort)->getPort()->writePortStacked(pEntries, piNumEntries));
    } else if(verifyHandle(hPort, Handle::TYPE_DEVICE)) {
        return setLastError(reinterpret_cast<LogicalDevice*>(hPort)->getLocalPort()->writePortStacked(pEntries, piNumEntries));
    } else if(verifyHandle(hPort, Handle::TYPE_STREAM)) {
        return setLastError(reinterpret_cast<DataStream*>(hPort)->getPort()->writePortStacked(pEntries, piNumEntries));
    } else {
        return setLastError(GC_ERR_INVALID_HANDLE);
    }

    GENTL_CATCH;
}

GC_API GCReadPortStacked(PORT_HANDLE hPort, PORT_REGISTER_STACK_ENTRY* pEntries, size_t* piNumEntries) {
    GENTL_TRY;
    DEBUG1(hPort);

    if(verifyHandle(hPort, Handle::TYPE_PORT)) {
        return setLastError(reinterpret_cast<Port*>(hPort)->readPortStacked(pEntries, piNumEntries));
    } else if(verifyHandle(hPort, Handle::TYPE_SYSTEM)) {
        return setLastError(reinterpret_cast<System*>(hPort)->getPort()->readPortStacked(pEntries, piNumEntries));
    } else if(verifyHandle(hPort, Handle::TYPE_INTERFACE)) {
        return setLastError(reinterpret_cast<Interface*>(hPort)->getPort()->readPortStacked(pEntries, piNumEntries));
    } else if(verifyHandle(hPort, Handle::TYPE_DEVICE)) {
        return setLastError(reinterpret_cast<LogicalDevice*>(hPort)->getLocalPort()->readPortStacked(pEntries, piNumEntries));
    } else if(verifyHandle(hPort, Handle::TYPE_STREAM)) {
        return setLastError(reinterpret_cast<DataStream*>(hPort)->getPort()->readPortStacked(pEntries, piNumEntries));
    } else {
        return setLastError(GC_ERR_INVALID_HANDLE);
    }

    GENTL_CATCH;
}


/*
 * Event functions
 */

GC_API EventFlush(EVENT_HANDLE hEvent) {
    GENTL_TRY;
    DEBUG1(hEvent);

    if(!verifyHandle(hEvent, Handle::TYPE_EVENT)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Event*>(hEvent)->flush());
    }

    GENTL_CATCH;
}

GC_API EventGetData(EVENT_HANDLE hEvent, void* pBuffer, size_t* piSize, uint64_t iTimeout) {
    GENTL_TRY;
    DEBUG2(hEvent, iTimeout);

    if(!verifyHandle(hEvent, Handle::TYPE_EVENT)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Event*>(hEvent)->getData(pBuffer, piSize, iTimeout));
    }

    GENTL_CATCH;
}

GC_API EventGetDataInfo(EVENT_HANDLE hEvent, const void* pInBuffer, size_t iInSize,
        EVENT_DATA_INFO_CMD iInfoCmd, INFO_DATATYPE* piType, void* pOutBuffer, size_t* piOutSize) {
    GENTL_TRY;
    DEBUG3(hEvent, iInSize, iInfoCmd);

    if(!verifyHandle(hEvent, Handle::TYPE_EVENT)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Event*>(hEvent)->getDataInfo(pInBuffer, iInSize, iInfoCmd, piType, pOutBuffer, piOutSize));
    }

    GENTL_CATCH;
}

GC_API EventGetInfo(EVENT_HANDLE hEvent, EVENT_INFO_CMD iInfoCmd, INFO_DATATYPE * piType,
        void * pBuffer, size_t * piSize) {
    GENTL_TRY;
    DEBUG2(hEvent, iInfoCmd);

    if(!verifyHandle(hEvent, Handle::TYPE_EVENT)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Event*>(hEvent)->getInfo(iInfoCmd, piType, pBuffer, piSize));
    }

    GENTL_CATCH;
}

GC_API EventKill(EVENT_HANDLE hEvent) {
    GENTL_TRY;
    DEBUG1(hEvent);

    if(!verifyHandle(hEvent, Handle::TYPE_EVENT)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<Event*>(hEvent)->kill());
    }

    GENTL_CATCH;
}

GC_API GCRegisterEvent(EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID, EVENT_HANDLE* phEvent) {
    GENTL_TRY;
    DEBUG2(hModule, iEventID);

    if(hModule == nullptr) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(Event::registerEvent(hModule, iEventID, phEvent));
    }

    GENTL_CATCH;
}

GC_API GCUnregisterEvent(EVENTSRC_HANDLE hModule, EVENT_TYPE iEventID) {
    GENTL_TRY;
    DEBUG2(hModule, iEventID);

    if (hModule == nullptr) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(Event::unregisterEvent(hModule, iEventID));
    }

    GENTL_CATCH;
}

/**** GenTL 1.6 additions ****/

GC_API DSAnnounceCompositeBuffer(DS_HANDLE hDataStream, size_t iNumSegments, void **ppSegments,
        size_t *piSizes, void *pPrivate, BUFFER_HANDLE *phBuffer) {
    GENTL_TRY;
    DEBUG6(hDataStream,iNumSegments,ppSegments,piSizes,pPrivate,phBuffer);
    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(phBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->announceCompositeBuffer(iNumSegments, ppSegments,
            piSizes, pPrivate, phBuffer));
    }
    GENTL_CATCH;
}

GC_API DSGetBufferInfoStacked(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
        DS_BUFFER_INFO_STACKED *pInfoStacked, size_t iNumInfos) {
    GENTL_TRY;
    DEBUG4(hDataStream,hBuffer,pInfoStacked,iNumInfos);
    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getBufferInfoStacked(
            hBuffer, pInfoStacked, iNumInfos));
    }
    GENTL_CATCH;
}

GC_API DSGetBufferPartInfoStacked(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer,
        DS_BUFFER_PART_INFO_STACKED *pInfoStacked, size_t iNumInfos) {
    GENTL_TRY;
    DEBUG4(hDataStream,hBuffer,pInfoStacked,iNumInfos);
    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getBufferPartInfoStacked(
            hBuffer, pInfoStacked, iNumInfos));
    }
    GENTL_CATCH;
}

GC_API DSGetNumFlows(DS_HANDLE hDataStream, uint32_t *piNumFlows) {
    GENTL_TRY;
    DEBUG2(hDataStream,piNumFlows);
    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getNumFlows(piNumFlows));
    }
    GENTL_CATCH;
}

GC_API DSGetFlowInfo(DS_HANDLE hDataStream, uint32_t iFlowIndex, FLOW_INFO_CMD iInfoCmd,
        INFO_DATATYPE *piType, void *pBuffer, size_t *piSize) {
    GENTL_TRY;
    DEBUG6(hDataStream,iFlowIndex,iInfoCmd,piType,pBuffer,piSize);
    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getFlowInfo(
            iFlowIndex, iInfoCmd, piType, pBuffer, piSize));
    }
    GENTL_CATCH;
}

GC_API DSGetNumBufferSegments(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t *piNumSegments) {
    GENTL_TRY;
    DEBUG3(hDataStream,hBuffer,piNumSegments);
    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getNumBufferSegments(
            hBuffer, piNumSegments));
    }
    GENTL_CATCH;
}

GC_API DSGetBufferSegmentInfo(DS_HANDLE hDataStream, BUFFER_HANDLE hBuffer, uint32_t iSegmentIndex,
        SEGMENT_INFO_CMD iInfoCmd, INFO_DATATYPE *piType, void *pBuffer, size_t *piSize) {
    GENTL_TRY;
    DEBUG7(hDataStream,hBuffer,iSegmentIndex,iInfoCmd,piType,pBuffer,piSize);
    if(!verifyHandle(hDataStream, Handle::TYPE_STREAM) || !verifyHandle(hBuffer, Handle::TYPE_BUFFER)) {
        return setLastError(GC_ERR_INVALID_HANDLE);
    } else {
        return setLastError(reinterpret_cast<DataStream*>(hDataStream)->getBufferSegmentInfo(
            hBuffer, iSegmentIndex, iInfoCmd, piType, pBuffer, piSize));
    }
    GENTL_CATCH;
}


} // namespace

