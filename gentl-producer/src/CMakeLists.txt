add_library(nerian-gentl SHARED
    gentl-adapter.cpp
    misc/common.h
    misc/handle.h
    misc/infoquery.h
    misc/infoquery.cpp
    misc/port.h
    misc/port.cpp
    misc/portimplementation.h
    misc/xmlfiles.h
    misc/testdata.h
    misc/disparity_testdata.cpp
    misc/left_testdata.cpp
    ../xml/gen/xmlfiles.cpp
    system/library.h
    system/library.cpp
    system/system.h
    system/system.cpp
    system/systemportimpl.h
    system/systemportimpl.cpp
    interface/interface.h
    interface/interface.cpp
    interface/interfaceportimpl.h
    interface/interfaceportimpl.cpp
    device/logicaldevice.h
    device/logicaldevice.cpp
    device/physicaldevice.h
    device/physicaldevice.cpp
    device/deviceportimpl.h
    device/deviceportimpl.cpp
    stream/buffermapping.h
    stream/buffermapping.cpp
    stream/datastream.h
    stream/datastream.cpp
    stream/buffer.h
    stream/buffer.cpp
    stream/streamportimpl.h
    stream/streamportimpl.cpp
    event/event.h
    event/event.cpp
    event/eventqueue.h
)

# Statically link against libvisiontransfer
target_link_libraries(nerian-gentl visiontransfer-static${LIB_SUFFIX} ${EXTRA_LIBS})

# Modify the filename to match the GenTL naming convention
set_target_properties(nerian-gentl PROPERTIES PREFIX "")
set_target_properties(nerian-gentl PROPERTIES SUFFIX ".cti")

if(WIN32 AND NOT CMAKE_CL_64)
    # We need a .def file to avoid _ prefixes
    set_target_properties(nerian-gentl PROPERTIES LINK_FLAGS "/def:${CMAKE_CURRENT_SOURCE_DIR}/exports.def")
endif()

# Add platform specific libraries
if(WIN32)
    target_link_libraries(nerian-gentl ws2_32 Iphlpapi)
else()
    target_link_libraries(nerian-gentl dl pthread)
endif()

install(TARGETS nerian-gentl DESTINATION lib/gentl)

string(ASCII 27 Esc)
install(CODE "message(\"\n${Esc}[33m\")")
install(CODE "message(\"The GenTL producer has been installed to ${CMAKE_INSTALL_PREFIX}/lib/gentl\")")
if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	install(CODE "message(\"Please add this directory to your GENICAM_GENTL64_PATH environment variable.\")")
else()
	install(CODE "message(\"Please add this directory to your GENICAM_GENTL32_PATH environment variable.\")")
endif()
install(CODE "message(\"\n${Esc}[m\")")
