project(sample)

find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)

find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)

find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
find_library(SWRESAMPLE_LIBRARY swresample)

find_path(FOUNDATION_INCLUDE_DIR Foundation/Foundation.h)
find_library(FOUNDATION_FRAMEWORK Foundation)

find_path(AVFOUNDATION_INCLUDE_DIR AVFoundation/AVFoundation.h)
find_library(AVFOUNDATION_FRAMEWORK AVFoundation)

find_path(AVKIT_INCLUDE_DIR AvKit/AvKit.h)
find_library(AVKIT_FRAMEWORK AvKit)

file(GLOB SRCS "*.c")

add_executable(sample ${SRCS})

target_include_directories(sample PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${AVCODEC_INCLUDE_DIR}
        ${AVFORMAT_INCLUDE_DIR}
        ${AVUTIL_INCLUDE_DIR}
        ${AVDEVICE_INCLUDE_DIR}
        ${SWSCALE_INCLUDE_DIR}
        ${SWRESAMPLE_INCLUDE_DIR}
        ${FOUNDATION_INCLUDE_DIR}
        ${AVFOUNDATION_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})


target_link_libraries(sample peer pthread
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${AVDEVICE_LIBRARY}
        ${SWRESAMPLE_LIBRARY}
        ${SWSCALE_LIBRARY}
        ${FOUNDATION_FRAMEWORK}
        ${AVFOUNDATION_FRAMEWORK})
