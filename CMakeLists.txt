#--------------------------------------
# CMake configuration
#--------------------------------------
cmake_minimum_required (VERSION 2.8)

# Show build commands
#set (CMAKE_VERBOSE_MAKEFILE true)

# Custom modules
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake")



#--------------------------------------
# Build configuration
#--------------------------------------
project (Psychopath)
set (Psychopath_VERSION_MAJOR 0)
set (Psychopath_VERSION_MINOR 0)
set (Psychopath_VERSION_PATCH 0)

# Set dummy compiler for clang static analysis
# Run 'scan-build make' to peform the analysis
#set (CMAKE_C_COMPILER /usr/share/clang/scan-build/ccc-analyzer)
#set (CMAKE_CXX_COMPILER /usr/share/clang/scan-build/c++-analyzer)

# Build type (Release, Debug)
set (CMAKE_BUILD_TYPE Release)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file ("${PROJECT_SOURCE_DIR}/config.h.in"
                "${PROJECT_BINARY_DIR}/config.h")

# C++11 for gcc and clang
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -g")

# Disable deprecation warnings (for some Imath includes)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")

# Fast math
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")

# SSE3 and SSSE3 support
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3 -mssse3")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse3 -mssse3")

# Warnings
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-function")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function")

# Define "DEBUG" in debug builds
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -g")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -g")


#--------------------------------------
# Boost
#--------------------------------------

set (Boost_MULTITHREADED TRUE)
set (Boost_USE_STATIC_LIBS FALSE)

find_package (Boost 1.48 COMPONENTS program_options REQUIRED)

include_directories (${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})


#--------------------------------------
# IlmBase
#--------------------------------------
find_package(IlmBase REQUIRED)

include_directories ("${ILMBASE_INCLUDE_DIR}/OpenEXR")


#--------------------------------------
# Open Image IO
#--------------------------------------
set(OIIO_DIR ${PROJECT_SOURCE_DIR}/../libs/oiio/dist/linux64)
set(OIIO_LIBRARY ${OIIO_DIR}/lib/libOpenImageIO.so)
set(OIIO_INCLUDE ${OIIO_DIR}/include)

include_directories (${OIIO_INCLUDE})


##--------------------------------------
## Open Shading Language
##--------------------------------------
#set(OSL_DIR ${PROJECT_SOURCE_DIR}/../libs/osl/dist/linux64)
#set(OSL_LIBRARYS ${OSL_DIR}/lib/liboslcomp.so
#                   ${OSL_DIR}/lib/liboslexec.so
#                   ${OSL_DIR}/lib/liboslquery.so)
#set(OSL_INCLUDE ${OSL_DIR}/include)
#
#include_directories (${OSL_INCLUDE})


#--------------------------------------
# Project include directories
#--------------------------------------
include_directories ("${PROJECT_BINARY_DIR}",
                     "${PROJECT_SOURCE_DIR}",
                     "${PROJECT_SOURCE_DIR}/accel",
                     "${PROJECT_SOURCE_DIR}/basics",
                     "${PROJECT_SOURCE_DIR}/integrator",
                     "${PROJECT_SOURCE_DIR}/lights",
                     "${PROJECT_SOURCE_DIR}/math",
                     "${PROJECT_SOURCE_DIR}/parser",
                     "${PROJECT_SOURCE_DIR}/object",
                     "${PROJECT_SOURCE_DIR}/film",
                     "${PROJECT_SOURCE_DIR}/renderer",
                     "${PROJECT_SOURCE_DIR}/sampling",
                     "${PROJECT_SOURCE_DIR}/scene",
                     "${PROJECT_SOURCE_DIR}/shading",
                     "${PROJECT_SOURCE_DIR}/test",
                     "${PROJECT_SOURCE_DIR}/tracer",
                     "${PROJECT_SOURCE_DIR}/utils",)


#--------------------------------------
# Build
#--------------------------------------
add_subdirectory(accel)
#add_subdirectory(basics)
add_subdirectory(integrator)
add_subdirectory(math)
add_subdirectory(object)
add_subdirectory(parser)
add_subdirectory(renderer)
add_subdirectory(sampling)
add_subdirectory(tracer)

add_library(config
    config)
add_library(global
    global)

# Main library
set (PSYCHO_LIB
    renderer
    integrator
    tracer
    sampling
    parser
    object
#    basics
    accel
    config
    global)

# Main executable
add_executable(psychopath
    main)
target_link_libraries(psychopath
    ${PSYCHO_LIB}
    ${Boost_LIBRARIES}
    ${ILMBASE_LIBRARIES}
    ${OIIO_LIBRARY}
#    ${OSL_LIBRARYS}
    )

# Unit-test executable
file(GLOB_RECURSE TEST_FILES *_test.cpp) # Find all tests

add_executable(unit_tests
    "test/test_main" "test/test_float.cpp" ${TEST_FILES})
target_link_libraries(unit_tests
    ${PSYCHO_LIB}
    ${Boost_LIBRARIES}
    ${ILMBASE_LIBRARIES}
    ${OIIO_LIBRARY}
#    ${OSL_LIBRARYS}
    )
