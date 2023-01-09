cmake_minimum_required(VERSION 3.10.2)

STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
MESSAGE("current project: " ${CURRENT_FOLDER})
project(${CURRENT_FOLDER})

set(CMAKE_SYSTEM_NAME Linux)

cmake_host_system_information(RESULT arch_value QUERY OS_PLATFORM)
message(STATUS "architecture: " ${arch_value})

if("${arch_value}" STREQUAL "armv7l")
	set(CMAKE_C_COMPILER "gcc")
	set(CMAKE_CXX_COMPILER "g++")
else()
	set(CMAKE_CROSSCOMPILING TRUE)
	set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
	set(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")

	# find host dir and set CMAKE_TOOLCHAIN_FILE before call project
	execute_process(COMMAND which ${CMAKE_C_COMPILER} OUTPUT_VARIABLE gcc_full_path)
	get_filename_component(gcc_full_dir ${gcc_full_path} DIRECTORY)
	set(HOST_DIR "${gcc_full_dir}/..")
	set(CMAKE_TOOLCHAIN_FILE "${HOST_DIR}/share/buildroot/toolchainfile.cmake")
endif()



# use c++ 11
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -DCPPHTTPLIB_OPENSSL_SUPPORT")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -DCPPHTTPLIB_OPENSSL_SUPPORT")

## 头文件
set(api_inc include/)

file(GLOB file_source src/*.cpp)
set(source ${file_source})

add_library(${PROJECT_NAME} STATIC ${source} )
include_directories(${PROJECT_NAME} PUBLIC ${api_inc} )
