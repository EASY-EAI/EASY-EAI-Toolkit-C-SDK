cmake_minimum_required(VERSION 2.8.4)

STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
MESSAGE("current project: " ${CURRENT_FOLDER})
project(${CURRENT_FOLDER})

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_CROSSCOMPILING TRUE)

set(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")

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
set(api_inc 
	${decryption_inc}/ 
	${hardware_verify_inc}/ 
	include/ 
	include_internal/ 
)
include_directories(${PROJECT_NAME} PUBLIC ${api_inc})

## 依赖的动态库文件
set(sysLib_list 
	pthread 
	zbar 
)
## 依赖的静态库文件
execute_process(COMMAND ar x ${CMAKE_CURRENT_SOURCE_DIR}/libs/libzbar.a)
file(GLOB file_obj ../build/*.o)
set(staticLib_objs ${file_obj})

## 依赖的源文件
file(GLOB file_source src/*.c src/*.cpp)
set(source ${file_source})

## 目标静态库
add_library(${PROJECT_NAME} STATIC ${source} ${decryption_lib} ${hardware_verify_lib} ${staticLib_objs})
target_link_libraries(${PROJECT_NAME} ${sysLib_list})
