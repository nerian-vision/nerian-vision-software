cmake_minimum_required(VERSION 2.4.6)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if(MINGW)
    message(FATAL_ERROR "Cannot be compiled with mingw")
endif()

enable_testing()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

if(NOT CMAKE_SUBMODULE)
    if(NOT WIN32)
        include(CheckCXXCompilerFlag)

        # Some useful flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")
        CHECK_CXX_COMPILER_FLAG("-march=native" NATIVE_ARCH_SUPPORT)
        if(NATIVE_ARCH_SUPPORT)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
        endif()

        # Activate c++11 support
        CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
        CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
        if(COMPILER_SUPPORTS_CXX11)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        elseif(COMPILER_SUPPORTS_CXX0X)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        else()
            message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
        endif()
    else()
        # Visual studio settings
        foreach(flag
                CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
                CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG_INIT
                CMAKE_CXX_FLAGS_RELEASE  CMAKE_CXX_FLAGS_RELWITHDEBINFO
                CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
            string(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
            set("${flag}" "${${flag}} /EHsc /arch:SSE2 /D__SSE__ /D__SSE2__")
        endforeach()
    endif()
endif()

#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

if(NOT CMAKE_SUBMODULE)
    #set the default path for built libraries to the "lib" directory
    if(NOT WIN32 OR MINGW)
        set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
    endif()
endif()
if(MSVC)
    MATH(EXPR BITS ${CMAKE_SIZEOF_VOID_P}*8)
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib${BITS})
endif()

set(BUILD_TESTS OFF CACHE BOOL "Builds the unit tests")

# Possible locations for the vision transfer library
set(VISIONTRANSFER_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../libvisiontransfer/lib CACHE STRING "Lib directory for libvisiontransfer")
set(VISIONTRANSFER_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libvisiontransfer CACHE STRING "Include directory for libvisiontransfer")
link_directories(${VISIONTRANSFER_LIB})
include_directories(${VISIONTRANSFER_INCLUDE})

add_subdirectory(src)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()
