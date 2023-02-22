message(STATUS "Python build uses source directory ${CMAKE_CURRENT_SOURCE_DIR}")

set(LIBVISIONTRANSFER_SRCDIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Base directory of libvisiontransfer source package")
set(LIBVISIONTRANSFER_LIBDIR "${LIBRARY_OUTPUT_PATH}" CACHE PATH "Base directory of built libvisiontransfer libraries")
set(LIBVISIONTRANSFER_EGGDIR "${LIBRARY_OUTPUT_PATH}/../python3-egg" CACHE PATH "Target directory for Python .egg packaging")
set(LIBVISIONTRANSFER_WHEELDIR "${LIBRARY_OUTPUT_PATH}/../python3-wheel" CACHE PATH "Target directory for Python .whl packaging")

if (WIN32 OR MINGW)
    # Extra libs to link in cython step
    set(LIBVISIONTRANSFER_EXTRA_LIBS "ws2_32,Iphlpapi")
else()
    set(LIBVISIONTRANSFER_EXTRA_LIBS "")
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/visiontransfer_src/__init__.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/visiontransfer_src)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/visiontransfer_src/visiontransfer_cpp.pxd.in DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/visiontransfer_src)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/visiontransfer_src/visiontransfer.pyx.in DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/visiontransfer_src)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tools/autogen_docstrings.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tools)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tools/autogen_parameters.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tools)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_sources.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/tools)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/setup.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(WIN32 AND NOT MINGW)
    # CMAKE_BUILD_TYPE did not work here, but we only build for Release anyway
    set(LIBNAME "/Release/visiontransfer-static${LIB_SUFFIX}.lib")
else()
    # Linux and msys builds
    set(LIBNAME "/libvisiontransfer-static${LIB_SUFFIX}.a")
endif()

# Target to call all required preprocessing and build steps for cython
#  (The || cd . is a Unix-compatible way to clear Windows errorlevel if directory already exists)
add_custom_target(cython ALL
    DEPENDS visiontransfer-static${LIB_SUFFIX}
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
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${CMAKE_CURRENT_BINARY_DIR}" LIBVISIONTRANSFER_SRCDIR="${LIBVISIONTRANSFER_SRCDIR}"
        LIBVISIONTRANSFER_LIBDIR=${LIBVISIONTRANSFER_LIBDIR} LIBVISIONTRANSFER_LIB=${LIBNAME}
        LIBVISIONTRANSFER_EXTRA_LIBS=${LIBVISIONTRANSFER_EXTRA_LIBS}
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/setup.py bdist_egg --dist-dir "${LIBVISIONTRANSFER_EGGDIR}"
)

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

