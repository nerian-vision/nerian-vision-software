cmake_minimum_required(VERSION 3.0.0)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0020 NEW)
endif(COMMAND cmake_policy)

#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# All auxiliary source files are in the helpers directory
set(HELPER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/colorcoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/colorcoder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/ratelimit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/ratelimit.cpp
)

# Also add the helpers directory to the include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/helpers)

if(NOT CMAKE_SUBMODULE)
    if(NOT WIN32 OR MINGW)
        include(CheckCXXCompilerFlag)

        # Some useful flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")
        CHECK_CXX_COMPILER_FLAG("-march=native" NATIVE_ARCH_SUPPORT)
        if(NATIVE_ARCH_SUPPORT)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
        endif()

        # Activate c++11 support
        CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
        CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
        CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
        if(COMPILER_SUPPORTS_CXX14)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
        elseif(COMPILER_SUPPORTS_CXX11)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        elseif(COMPILER_SUPPORTS_CXX0X)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        else()
            message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
        endif()
    endif()
endif()

# Search for Open3D for example compilation
set(DISABLE_OPEN3D 0 CACHE BOOL "Disables Open3D example")
if(NOT DISABLE_OPEN3D)
    message(STATUS "looking for Open3D")
    find_package(Open3D)

    if(Open3D_FOUND)
        # Check system C++ ABI
        set(SYSTEM_CXX11_ABI 0)
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles("\
            #include <string>\n\
            #if _GLIBCXX_USE_CXX11_ABI == 0\n\
            #error\n\
            #endif\n\
            int main(int, char**) {return 0;}"
            SYSTEM_CXX11_ABI)

        # Check Open3D C++ ABI
        get_property(def TARGET Open3D::Open3D PROPERTY INTERFACE_COMPILE_DEFINITIONS)
        if(def MATCHES "GLIBCXX_USE_CXX11_ABI=0")
            set(OPEN3D_CXX11_ABI 0)
        else()
            set(OPEN3D_CXX11_ABI 1)
        endif()

        if((NOT MSVC) AND ${SYSTEM_CXX11_ABI} AND (NOT ${OPEN3D_CXX11_ABI}))
            message(WARNING "Open3D was built with old C++ ABI and cannot be used "
                "in combination with Qt. 3D visualization will not be available in NVCom")
            set(Open3D_LIBRARIES "")
        else()
            include_directories(${Open3D_INCLUDE_DIRS})
            add_compile_definitions(WITH_OPEN3D)

            if(MSVC)
                # Open3D requires static linking on windows
                if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                    string(REPLACE "/MDd" "/MTd" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
                else()
                    string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
                endif()
            endif()
        endif()
    else()
        message(WARNING "Not building Open3D example!")
        set(Open3D_LIBRARIES "")
    endif()
endif()

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Find the QtWidgets library
find_package(Qt5Widgets REQUIRED)

# Find required libraries
find_package(Threads)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Possible locations for the vision transfer library
MATH(EXPR BITS ${CMAKE_SIZEOF_VOID_P}*8)
if(MSVC)
    set(VISIONTRANSFER_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../libvisiontransfer/lib${BITS}/${CMAKE_BUILD_TYPE} CACHE STRING "Lib directory for libvisiontransfer")
else()
    set(VISIONTRANSFER_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../libvisiontransfer/lib CACHE STRING "Lib directory for libvisiontransfer")
endif()
set(VISIONTRANSFER_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../libvisiontransfer CACHE STRING "Include directory for libvisiontransfer")
link_directories(${VISIONTRANSFER_LIB})
include_directories(${VISIONTRANSFER_INCLUDE})

qt5_add_resources(RES_NVCOM resources.qrc)

if(WIN32)
    set(WINDOWS_RES windows-resource.rc)
endif()

set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH};${CMAKE_INSTALL_PREFIX}/lib)

# Optional demo mode
#add_definitions(-DDEMO_MODE)

add_executable(nvcom
    nvcom.h
    nvcom.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    connectiondialog.h
    connectiondialog.cpp
    connectiondialog.ui
    displaywidget.h
    displaywidget.cpp
    imagereader.cpp
    imagereader.h
    settingsdialog.h
    settingsdialog.cpp
    settingsdialog.ui
    qtopen3dvisualizer.h
    qtopen3dvisualizer.cpp
    ${HELPER_FILES}
    ${RES_NVCOM}
    ${WINDOWS_RES}
)

target_link_libraries(nvcom ${CMAKE_THREAD_LIBS_INIT} visiontransfer${LIB_SUFFIX} ${OpenCV_LIBS} Qt5::Widgets ${Open3D_LIBRARIES})

# Installation settings
install(TARGETS nvcom DESTINATION bin)
