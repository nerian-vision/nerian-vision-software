include_directories(${PROJECT_SOURCE_DIR})

if(NOT WIN32 OR MINGW)
    include(CheckCXXCompilerFlag)

    # Some useful flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")
    CHECK_CXX_COMPILER_FLAG("-march=native" NATIVE_ARCH_SUPPORT)
    if(NATIVE_ARCH_SUPPORT AND NOT DISABLE_NATIVE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()

    # Test most examples with legacy C++ (some are overridden below)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")

else()
    # Visual studio settings
    set(CMAKE_DEBUG_POSTFIX "-debug")
endif()

if(UNIX OR MINGW)
    set(EXTRA_LIBS pthread)
endif()

if(WIN32)
    set(EXTRA_LIBS ws2_32)
endif()

# All examples except PCL and OpenCV
set(EXAMPLES
	imagetransfer_example
	asynctransfer_example
	reconstruct3d_example
	server_example
	parameter_example
	temperature_example
	parameter_enumeration_example
	parameter_set_batch_write_example
	imu_data_channel_example
	input_transfer_example
)

foreach(example IN LISTS EXAMPLES)
    add_executable(${example}
        ${example}.cpp
    )

    target_link_libraries(${example} visiontransfer${LIB_SUFFIX} ${EXTRA_LIBS})
endforeach()


# Enforce C++11/C++14 for examples that require it
if(NOT MSVC)
    # The new (full) parameter API defines C++11 or up as a requirement
    string(REPLACE "-std=c++98" "-std=c++11" COMPILE_FLAGS_CXX11 "${CMAKE_CXX_FLAGS}")
    set_target_properties(parameter_enumeration_example PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS_CXX11}")
    set_target_properties(parameter_set_batch_write_example PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS_CXX11}")
    set_target_properties(temperature_example PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS_CXX11}")

    string(REPLACE "-std=c++98" "-std=c++14" COMPILE_FLAGS_CXX14 "${CMAKE_CXX_FLAGS}")
    set_target_properties(input_transfer_example PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS_CXX14}")
endif()


# PCL example
if(PCL_FOUND)
    add_executable(pcl_example
        pcl_example.cpp
    )

    if(NOT MSVC)
        # Latest PCL needs C++14, so we will not build this specific one with C++98
        set_target_properties(pcl_example PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS_CXX14}")
    endif()

    target_link_libraries(pcl_example visiontransfer${LIB_SUFFIX} ${PCL_LIBRARIES} ${EXTRA_LIBS})
endif()

# OpenCV example
if(OpenCV_FOUND)
    add_executable(opencv_example
        opencv_example.cpp
    )

    if(NOT MSVC)
        # OpenCV >=4.0 needs C++11, so we will not build this specific one with C++98
        set_target_properties(opencv_example PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS_CXX11}")
    endif()

    target_link_libraries(opencv_example visiontransfer${LIB_SUFFIX} ${OpenCV_LIBS} ${EXTRA_LIBS})
endif()

# Open3D example
if(Open3D_FOUND)
    add_executable(open3d_example
        open3d_example.cpp
    )
    if(MSVC)
        # Open3D requires static linking on windows
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            string(REPLACE "/MDd" "/MTd" COMPILE_FLAGS_STATIC "${CMAKE_CXX_FLAGS_DEBUG}")
        else()
            string(REPLACE "/MD" "/MT" COMPILE_FLAGS_STATIC "${CMAKE_CXX_FLAGS_RELEASE}")
        endif()
        set_target_properties(open3d_example PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS_STATIC}")
    else()
        # Open3D needs C++14, so we will not build this specific one with C++98
        set_target_properties(open3d_example PROPERTIES COMPILE_FLAGS "${COMPILE_FLAGS_CXX14}")
    endif()

    target_link_libraries(open3d_example visiontransfer${OPEN3D_LIB_SUFFIX} ${Open3D_LIBRARIES} ${EXTRA_LIBS})
endif()
