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


set(commonApi_root ${CMAKE_CURRENT_SOURCE_DIR}/../)
## 依赖的头文件
set(api_inc
	${commonApi_root}/
	${commonApi_root}/log_manager/ ${commonApi_root}/log_manager/include
	${commonApi_root}/system_opt/ ${commonApi_root}/system_opt/include
	include/
	include_internal/
	include_internal/iconv
)
include_directories(${PROJECT_NAME} PUBLIC ${api_inc})

## 依赖的动态库文件

## 依赖的静态库文件
#execute_process(COMMAND ar x ${commonApi_root}/system_opt/libsystem_opt.a)
execute_process(COMMAND ar x ${CMAKE_CURRENT_SOURCE_DIR}/libs/libfreetype.a)
execute_process(COMMAND ar x ${CMAKE_CURRENT_SOURCE_DIR}/libs/libiconv.a)
file(GLOB file_obj ../build/*.o)
set(staticLib_objs ${file_obj})

## 依赖的源文件
file(GLOB file_source src/*.c src/*.cpp)
set(source ${file_source})

## 目标静态库
add_library(${PROJECT_NAME} STATIC ${source} ${staticLib_objs})
