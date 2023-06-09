cmake_minimum_required(VERSION 3.16)

project(rtspserver) #Project name

set(CMAKE_CXX_STANDARD 14) #setting C++ 14 standard
find_package(PkgConfig) #finding pkg-config is a helper tool
find_package(OpenCV REQUIRED COMPONENTS core imgproc aruco)
if (NOT OpenCV_FOUND)
  message(STATUS "Did not find OpenCV 4")
endif()

#using pkg-config to getting Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTRTSP REQUIRED gstreamer-rtsp-server-1.0)

#including GStreamer header files directory
include_directories(
  ${GLIB_INCLUDE_DIRS}
  ${GSTREAMER_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)

#linking GStreamer library directory
link_directories(
        ${GLIB_LIBRARY_DIRS}
        ${GSTREAMER_LIBRARY_DIRS}
        ${OpenCV_LIB_DIR}
)

#building target executable
add_executable(${PROJECT_NAME} main.cpp)

#linking Gstreamer library with target executable

message(STATUS "OpenCV libs: ${OpenCV_LIBS}")

target_link_libraries(${PROJECT_NAME} ${GSTREAMER_LIBRARIES} ${GSTRTSP_LIBRARIES} ${OpenCV_LIBS} opencv_core)
