cmake_minimum_required(VERSION 3.0.0)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

MATH(EXPR BITS ${CMAKE_SIZEOF_VOID_P}*8)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Search for PCL for example compilation
set(DISABLE_PCL 0 CACHE BOOL "Disables OpenCL example")
if(NOT DISABLE_PCL)
    message(STATUS "looking for PCL")
    find_package(PCL 1.3 COMPONENTS common io filters)

    if(PCL_FOUND)
        include_directories(${PCL_INCLUDE_DIRS})
        link_directories(${PCL_LIBRARY_DIRS})
        add_definitions(${PCL_DEFINITIONS})
    else()
        message(WARNING "Not building PCL example!")
    endif()
endif()

# Search for Open3D for example compilation
set(DISABLE_OPEN3D 0 CACHE BOOL "Disables Open3D example")
if(NOT DISABLE_OPEN3D)
    # Search for Open3D
    message(STATUS "looking for Open3D")
    find_package(Open3D)
    if(Open3D_FOUND)
        include_directories(${Open3D_INCLUDE_DIRS})

        # Check Open3D C++ ABI
        get_property(def TARGET Open3D::Open3D PROPERTY INTERFACE_COMPILE_DEFINITIONS)
        if(def MATCHES "GLIBCXX_USE_CXX11_ABI=0")
            set(OPEN3D_CXX11_ABI 0)
        else()
            set(OPEN3D_CXX11_ABI 1)
        endif()

        # Check system C++ ABI
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles("\
            #include <string>\n\
            #if _GLIBCXX_USE_CXX11_ABI == 0\n\
            #error\n\
            #endif\n\
            int main(int, char**) {return 0;}"
            SYSTEM_CXX11_ABI)

        # Check if ABIs match
        set(OPEN3D_LIB_SUFFIX "")
        if(NOT MSVC)
            if(${SYSTEM_CXX11_ABI} AND (NOT ${OPEN3D_CXX11_ABI}))
                message(WARNING
                    "Open3D was built with old C++ ABI (_GLIBCXX_USE_CXX11_ABI=0). "
                    "A separate version of libvisiontransfer will be built for linking "
                    "against Open3D. Using Open3D in combination with other libraries "
                    "that are built with the more recent C++ ABI will not be possible.")
                set(BUILD_WITHOUT_CXX11_ABI 1)
                set(OPEN3D_LIB_SUFFIX "-without-cxx11-abi${LIB_SUFFIX}")
            endif()
        endif()
    else()
        message(WARNING "Not building Open3D example!")
    endif()
endif()

# Search for OpenCV for example compilation
set(DISABLE_OPENCV 0 CACHE BOOL "Disables OpenCV example")
if(NOT DISABLE_OPENCV)
    message(STATUS "looking for OpenCV")
    find_package(OpenCV)
    if(OpenCV_FOUND)
        include_directories(${OpenCV_INCLUDE_DIRS})
    else()
        message(WARNING "Not building OpenCV example!")
    endif()
endif()

set(DISABLE_NATIVE 0 CACHE BOOL "Disables native architecture compile flag")
if(NOT WIN32 OR MINGW)
    include(CheckCXXCompilerFlag)

    # Some useful flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")
    CHECK_CXX_COMPILER_FLAG("-march=native" NATIVE_ARCH_SUPPORT)
    if(NATIVE_ARCH_SUPPORT AND NOT DISABLE_NATIVE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()

    # Activate c++11 or newer support
    CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
    CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    if(COMPILER_SUPPORTS_CXX17)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    elseif(COMPILER_SUPPORTS_CXX14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    elseif(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
        message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
else()
    set(CMAKE_DEBUG_POSTFIX "-debug")
endif()
if (MSVC)
    # Enable feature for correct reporting of __cplusplus
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
endif()


# Search for python3
set(DISABLE_PYTHON 0 CACHE BOOL "Disables python library")
if(NOT DISABLE_PYTHON)
    message(STATUS "looking for python3")

    if(WIN32 AND NOT MINGW)
        # Make sure we use the Windows python, not a matching one from msys!
        # If pyenv-win is also installed, we prefer its current setting.
        set(USERPROFILE $ENV{USERPROFILE})
        # Note - shell redirection into CMake only works with the .bat wrapper
        file(GLOB pyenv_EXECUTABLE "${USERPROFILE}/.pyenv/pyenv-win/bin/pyenv.bat")
        if ("${pyenv_EXECUTABLE}" STREQUAL "")
            message(STATUS "pyenv not installed, assuming default Python directories")
            if(${BITS} EQUAL 32)
                file(GLOB Python3_EXECUTABLE
                    "${USERPROFILE}/AppData/Local/Programs/Python/Python3?-32/python.exe"
                    "${USERPROFILE}/AppData/Local/Programs/Python/Python3??-32/python.exe")
            else()
                file(GLOB Python3_EXECUTABLE
                    "${USERPROFILE}/AppData/Local/Programs/Python/Python3?/python.exe"
                    "${USERPROFILE}/AppData/Local/Programs/Python/Python3??/python.exe")
            endif()
            message(WARNING "Windows build - assuming Python 3 is \"${Python3_EXECUTABLE}\".")
        else()
            message(STATUS "pyenv - using wrapper found at: ${pyenv_EXECUTABLE}")
            execute_process(COMMAND "${pyenv_EXECUTABLE}" "which" "python"
                OUTPUT_VARIABLE pyenv_which_stdout
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            message(STATUS "Using this Python 3 via 'pyenv which python': ${pyenv_which_stdout}")
            set(Python3_EXECUTABLE "${pyenv_which_stdout}")
        endif()
    else()
        set(Python3_EXECUTABLE "python3")
        # This is for CMake 3.12 and up; making sure we get python3
        find_package (Python3 COMPONENTS Interpreter)
        if(NOT Python3_FOUND)
            find_package (Python3 COMPONENTS Interpreter HINTS "/mingw64")
        endif()
        if(NOT Python3_FOUND)
            # We don't give up just yet
            message(WARNING "Failed finding python3 with FindPython3. Assuming python3 is \"${Python3_EXECUTABLE}\"")
        endif()
    endif()

    # Search for cython
    message(STATUS "looking for Cython")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "-c"
"\
from distutils.core import setup\n\
from distutils.extension import Extension\n\
from Cython.Build import cythonize\n\
print('OK')\
"
        OUTPUT_VARIABLE cython_output
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if("${cython_output}" STREQUAL "OK")
        set(BUILD_CYTHON 1)
    else()
        message(WARNING "${cython_output}")
        message(WARNING "Cython not found! Not building python library!")
    endif()

    # Search for python-wheel
    message(STATUS "looking for Wheel")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "-c"
"\
import wheel\n\
print('OK')\
"
        OUTPUT_VARIABLE wheel_output
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if("${wheel_output}" STREQUAL "OK")
        set(BUILD_WHEEL 1)
    else()
        message(WARNING "${wheel_output}")
        message(WARNING "Wheel not found! Not building python wheel packages!")
    endif()
endif()

if(NOT CMAKE_SUBMODULE)
    #set the default path for built libraries to the "lib" directory
    if(NOT WIN32 OR MINGW)
        set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
    else()
        set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib${BITS})
    endif()
endif()

message(STATUS "CXX FLAGS: ${CMAKE_CXX_FLAGS}")

add_subdirectory(visiontransfer)
add_subdirectory(examples)

if(BUILD_CYTHON)
    add_subdirectory(python)
endif()

