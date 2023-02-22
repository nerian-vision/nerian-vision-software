find_package(GTest)
if(GTEST_FOUND)
    find_package(Threads REQUIRED)
   include_directories(${GTEST_INCLUDE_DIRS})

    add_executable(test-all
        test-all.cpp
        test-library.cpp
        test-system.cpp
        test-interface.cpp
        test-device.cpp
        test-stream.cpp
        test-port.cpp
        test-event.cpp
    )

    target_link_libraries(test-all ${GTEST_BOTH_LIBRARIES} pthread nerian-gentl)

    add_test(NAME test-all COMMAND "${EXECUTABLE_OUTPUT_PATH}/test-all")
else()
    message(WARNING "!!! Not building tests as Google Test library was not found!!!")
endif()
