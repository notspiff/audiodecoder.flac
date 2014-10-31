project(audiodecoder.flac)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)
find_package(FLAC REQUIRED)

include_directories(${KODI_INCLUDE_DIR}
                    ${FLAC_INCLUDE_DIRS})

set(FLAC_SOURCES src/FLACCodec.cpp)

set(DEPLIBS ${FLAC_LIBRARIES})

add_definitions(-D__STDC_FORMAT_MACROS)

build_addon(audiodecoder.flac FLAC DEPLIBS)
