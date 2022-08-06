cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)

set(SERVER_SRC_FILES
    rkbtsource_server.cpp
)

set(CLIENT_SRC_FILES
    rkbtsource.cpp
)

add_definitions("-fpermissive")

add_executable(rkbtsource_server ${SERVER_SRC_FILES})
target_include_directories(rkbtsource_server PUBLIC
        "${deviceio_test_SOURCE_DIR}/DeviceIO/include" )
target_link_libraries(rkbtsource_server DeviceIo asound)

add_executable(rkbtsource ${CLIENT_SRC_FILES})
target_include_directories(rkbtsource PUBLIC
        "${deviceio_test_SOURCE_DIR}/DeviceIO/include" )
target_link_libraries(rkbtsource DeviceIo asound)

install(TARGETS rkbtsource_server DESTINATION bin)
install(TARGETS rkbtsource DESTINATION bin)