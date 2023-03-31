cmake_minimum_required(VERSION 2.8.4)

STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} )
MESSAGE("current project: " ${CURRENT_FOLDER})
project(${CURRENT_FOLDER})



set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CROSSCOMPILING TRUE)

cmake_host_system_information(RESULT arch_value QUERY OS_PLATFORM)
message(STATUS "architecture: " ${arch_value})

if(NOT "${arch_value}" STREQUAL "armv7l")
	include ($ENV{HOME}/configs/cross.cmake)
endif()

# use c++ 11
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -fPIC ")

# decryption路径
set(decryption_inc ../../encryption/include/decryption)
file(GLOB decryption_lib ../../encryption/lib/decryption/*.c)

# hardware verify
set(hardware_verify_inc ../../encryption/include/hardware_verify)
file(GLOB hardware_verify_lib ../../encryption/lib/hardware_verify/*.c)

## 头文件
set(api_inc include/ include_internal)

file(GLOB file_source src/*.cpp)
set(source ${file_source})

add_library(${PROJECT_NAME} STATIC ${source} ${decryption_lib} ${hardware_verify_lib})
include_directories(${PROJECT_NAME} PUBLIC ${api_inc} ${decryption_inc} ${hardware_verify_inc})
target_link_libraries(${PROJECT_NAME} -lpthread )
