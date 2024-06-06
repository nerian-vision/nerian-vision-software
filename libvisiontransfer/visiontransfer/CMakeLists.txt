set(HEADERS
    parametervalue.h
    parameter.h
    parameterset.h
    asynctransfer.h
    imageprotocol.h
    imagetransfer.h
    common.h
    reconstruct3d.h
    reconstruct3d-pcl.h
    reconstruct3d-open3d.h
    imageset.h
    imageset-opencv.h
    imagepair.h
    datachannelservice.h
    deviceinfo.h
    deviceenumeration.h
    parameterinfo.h
    deviceparameters.h
    scenescanparameters.h
    sensordata.h
    libraryinfo.h
    types.h
    internal/alignedallocator.h
    internal/bitconversions.h
    internal/conversionhelpers.h
    internal/datablockprotocol.h
    internal/datachannel-imu-bno080.h
    internal/datachannelservicebase.h
    internal/internalinformation.h
    internal/networking.h
    internal/parameterserialization.h
    internal/parametertransfer.h
    internal/parametertransferdata.h
    internal/protocol-sh2-imu-bno080.h
    internal/sensorringbuffer.h
    internal/tokenizer.h
)

set(SOURCES
    parametervalue.cpp
    parameter.cpp
    parameterset.cpp
    asynctransfer.cpp
    imageprotocol.cpp
    imagetransfer.cpp
    reconstruct3d.cpp
    imageset.cpp
    datachannelservice.cpp
    deviceenumeration.cpp
    parameterinfo.cpp
    deviceparameters.cpp
    libraryinfo.cpp
    internal/bitconversions.cpp
    internal/datablockprotocol.cpp
    internal/datachannel-imu-bno080.cpp
    internal/datachannelservicebase.cpp
    internal/internalinformation.cpp
    internal/networking.cpp
    internal/parameterserialization.cpp
    internal/parametertransfer.cpp
)

# Build static and shared version
add_library(visiontransfer${LIB_SUFFIX} SHARED
    ${HEADERS}
    ${SOURCES}
)

add_library(visiontransfer-static${LIB_SUFFIX} STATIC
    ${HEADERS}
    ${SOURCES}
)

if(${BUILD_WITHOUT_CXX11_ABI})
    add_library(visiontransfer${OPEN3D_LIB_SUFFIX} SHARED
        ${HEADERS}
        ${SOURCES}
    )
    set_target_properties(visiontransfer${OPEN3D_LIB_SUFFIX} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0 -DVISIONTRANSFER_NO_OLD_ABI_WARNING")
endif()

if(WIN32)
    target_link_libraries(visiontransfer${LIB_SUFFIX} ws2_32 Iphlpapi)
    target_link_libraries(visiontransfer-static${LIB_SUFFIX} ws2_32 Iphlpapi)
else()
    set_target_properties(visiontransfer-static${LIB_SUFFIX}
       PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

install(TARGETS visiontransfer${LIB_SUFFIX} DESTINATION lib)
install(TARGETS visiontransfer-static${LIB_SUFFIX} DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/visiontransfer)

