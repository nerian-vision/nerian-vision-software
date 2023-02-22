set(HEADERS
    parametervalue.h
    parameter.h
    parameterset.h
    parameterserialization.h
    tokenizer.h
    asynctransfer.h
    imageprotocol.h
    imagetransfer.h
    common.h
    reconstruct3d.h
    reconstruct3d-pcl.h
    reconstruct3d-open3d.h
    alignedallocator.h
    datablockprotocol.h
    imageset.h
    imageset-opencv.h
    imagepair.h
    bitconversions.h
    datachannelservicebase.h
    datachannelservice.h
    datachannel-imu-bno080.h
    protocol-sh2-imu-bno080.h
    deviceinfo.h
    deviceenumeration.h
    internalinformation.h
    parameterinfo.h
    parametertransfer.h
    parametertransferdata.h
    deviceparameters.h
    scenescanparameters.h
    standardparameterids.h
    networking.h
    sensorringbuffer.h
    sensordata.h
    conversionhelpers.h
)

set(SOURCES
    parametervalue.cpp
    parameter.cpp
    parameterset.cpp
    parameterserialization.cpp
    asynctransfer.cpp
    imageprotocol.cpp
    imagetransfer.cpp
    reconstruct3d.cpp
    datablockprotocol.cpp
    imageset.cpp
    bitconversions.cpp
    datachannelservicebase.cpp
    datachannelservice.cpp
    datachannel-imu-bno080.cpp
    deviceenumeration.cpp
    internalinformation.cpp
    parameterinfo.cpp
    parametertransfer.cpp
    deviceparameters.cpp
    standardparameterids.cpp
    networking.cpp
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
