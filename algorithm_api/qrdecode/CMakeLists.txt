cmake_minimum_required(VERSION 2.8.4)

STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
MESSAGE("current project: " ${CURRENT_FOLDER})

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CROSSCOMPILING TRUE)

cmake_host_system_information(RESULT arch_value QUERY OS_PLATFORM)
message(STATUS "architecture: " ${arch_value})

## 编译工具链
if(NOT "${arch_value}" STREQUAL "armv7l")
	include ($ENV{HOME}/configs/cross.cmake)
endif()
project(${CURRENT_FOLDER})

## 编译配置
# use c++ 11
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")

# decryption路径
set(decryption_inc ../../encryption/include/decryption)
file(GLOB decryption_lib ../../encryption/lib/decryption/*.c)

# hardware verify
set(hardware_verify_inc ../../encryption/include/hardware_verify)
file(GLOB hardware_verify_lib ../../encryption/lib/hardware_verify/*.c)

set(commonApi_root ${CMAKE_CURRENT_SOURCE_DIR}/../../common_api)
## 依赖的头文件
find_package(OpenCV REQUIRED)
set(api_inc 
	include/
	${decryption_inc}/
	${hardware_verify_inc}/
	${OpenCV_INCLUDE_DIRS}/
)
include_directories(${PROJECT_NAME} PUBLIC ${api_inc})

## 依赖的动态库文件
set(sysLib_list 
	pthread 
	zbar 
)
## 依赖的静态库文件
file(GLOB file_obj ../build/*.o)
set(staticLib_objs ${file_obj})

## 依赖的源文件
file(GLOB file_source src/*.c src/*.cpp)
set(source ${file_source})

## 目标静态库
add_library(${PROJECT_NAME} STATIC ${source} ${decryption_lib} ${hardware_verify_lib} ${staticLib_objs})
target_link_libraries(${PROJECT_NAME} ${sysLib_list})
