message(STATUS "Python build uses source directory ${CMAKE_CURRENT_SOURCE_DIR}")

set(LIBVISIONTRANSFER_SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Base directory of libvisiontransfer source package")
set(LIBVISIONTRANSFER_LIBDIR "${LIBRARY_OUTPUT_PATH}" CACHE PATH "Base directory of built libvisiontransfer libraries")
set(LIBVISIONTRANSFER_WHEELDIR "${LIBRARY_OUTPUT_PATH}/../python3-wheel" CACHE PATH "Target directory for Python .whl packaging")

if (WIN32 OR MINGW)
    # Extra libs to link in cython step
    set(LIBVISIONTRANSFER_EXTRA_LIBS "ws2_32,Iphlpapi")
else()
    set(LIBVISIONTRANSFER_EXTRA_LIBS "")
endif()

add_custom_target(init_cython
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/visiontransfer_src/__init__.py ${CMAKE_CURRENT_BINARY_DIR}/visiontransfer_src
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/visiontransfer_src/visiontransfer_cpp.pxd.in ${CMAKE_CURRENT_BINARY_DIR}/visiontransfer_src
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/visiontransfer_src/visiontransfer.pyx.in ${CMAKE_CURRENT_BINARY_DIR}/visiontransfer_src
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tools/autogen_docstrings.py ${CMAKE_CURRENT_BINARY_DIR}/tools
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tools/autogen_parameters.py ${CMAKE_CURRENT_BINARY_DIR}/tools
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_sources.py ${CMAKE_CURRENT_BINARY_DIR}/tools
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/setup.py ${CMAKE_CURRENT_BINARY_DIR}
)

if(WIN32 AND NOT MINGW)
    # CMAKE_BUILD_TYPE did not work here, but we only build for Release anyway
    set(LIBNAME "/Release/visiontransfer-static${LIB_SUFFIX}.lib")
else()
    # Linux and msys builds
    set(LIBNAME "/libvisiontransfer-static${LIB_SUFFIX}.a")
endif()

# Target to call all required preprocessing and build steps for cython
#  (The || cd . is a Unix-compatible way to clear Windows errorlevel if directory already exists)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Makefile
    DEPENDS init_cython
    COMMENT "Will run the Cython build target"
    COMMAND mkdir visiontransfer || cd .
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${CMAKE_CURRENT_BINARY_DIR}" LIBVISIONTRANSFER_SRCDIR="${LIBVISIONTRANSFER_SRCDIR}"
        LIBVISIONTRANSFER_LIBDIR=${LIBVISIONTRANSFER_LIBDIR} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/tools/autogen_docstrings.py
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${CMAKE_CURRENT_BINARY_DIR}" LIBVISIONTRANSFER_SRCDIR="${LIBVISIONTRANSFER_SRCDIR}"
        LIBVISIONTRANSFER_LIBDIR=${LIBVISIONTRANSFER_LIBDIR} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/tools/autogen_parameters.py
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${CMAKE_CURRENT_BINARY_DIR}" LIBVISIONTRANSFER_SRCDIR="${LIBVISIONTRANSFER_SRCDIR}"
        LIBVISIONTRANSFER_LIBDIR=${LIBVISIONTRANSFER_LIBDIR} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/tools/generate_sources.py
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${CMAKE_CURRENT_BINARY_DIR}" LIBVISIONTRANSFER_SRCDIR="${LIBVISIONTRANSFER_SRCDIR}"
        LIBVISIONTRANSFER_LIBDIR=${LIBVISIONTRANSFER_LIBDIR} LIBVISIONTRANSFER_LIB=${LIBNAME}
        LIBVISIONTRANSFER_EXTRA_LIBS=${LIBVISIONTRANSFER_EXTRA_LIBS}
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/setup.py build_ext
)
add_custom_target(cython ALL
    DEPENDS visiontransfer-static${LIB_SUFFIX}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Makefile)

# Wheel is built separately as it might not be available on all systems
if(BUILD_WHEEL)
    add_custom_target(wheel ALL
        DEPENDS cython
        COMMENT "Creates python wheel package"
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${CMAKE_CURRENT_BINARY_DIR}" LIBVISIONTRANSFER_SRCDIR="${LIBVISIONTRANSFER_SRCDIR}"
            LIBVISIONTRANSFER_LIBDIR=${LIBVISIONTRANSFER_LIBDIR} LIBVISIONTRANSFER_LIB=${LIBNAME}
            LIBVISIONTRANSFER_EXTRA_LIBS=${LIBVISIONTRANSFER_EXTRA_LIBS}
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/setup.py bdist_wheel --dist-dir "${LIBVISIONTRANSFER_WHEELDIR}"
    )
endif()

install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E env LIBVISIONTRANSFER_LIBDIR=${LIBVISIONTRANSFER_LIBDIR} \
    LIBVISIONTRANSFER_EXTRA_LIBS=${LIBVISIONTRANSFER_EXTRA_LIBS} \
    ${Python3_EXECUTABLE} setup.py install WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})" )
