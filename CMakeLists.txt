# cmake最低版本
cmake_minimum_required(VERSION 3.15)
# 工程名
project(imgUpdate)
# c++标准
set(CMAKE_CXX_STANDARD 17)
# 开启Qt预处理的组件
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
#包含当前项目路径
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# 设置cmake模块查询目录
set(QT_PATH D:\\software\\qt\\5.15.2\\mingw81_64)
set(VCPKG_PATH D:\\software\\vcpkg-clion\\installed\\x64-mingw-dynamic)
#set(QT_PATH /usr/lib/aarch64-linux-gnu/qt5)
set(CMAKE_PREFIX_PATH ${QT_PATH} ${VCPKG_PATH})
# 查找QT模块
find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        TextToSpeech
        Quick
        REQUIRED)
# 添加外部库
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic")
find_package(libssh CONFIG REQUIRED)
find_package(JsonCpp CONFIG REQUIRED)
find_package(curl CONFIG REQUIRED)
find_package(curlcpp CONFIG REQUIRED)

# 添加源文件
aux_source_directory(. DIRS_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src DIRS_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/sftp DIRS_SRCS)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/src/sftp)

#link_directories(/usr/local/lib)
set(WINDOWS_RELEASE 1)
if (WINDOWS_RELEASE)
    add_executable(${PROJECT_NAME} WIN32 ${DIRS_SRCS})
else()
    add_executable(${PROJECT_NAME} ${DIRS_SRCS})
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::TextToSpeech
        Qt5::Quick
        ssh
        ws2_32
        crypt32
        JsonCpp::JsonCpp
        curlcpp::curlcpp
        )

