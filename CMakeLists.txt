#--------------------------------------
# CMake configuration
#--------------------------------------
cmake_minimum_required (VERSION 2.8)


#--------------------------------------
# Build configuration
#--------------------------------------
project (Psychopath)
set (Psychopath_VERSION_MAJOR 0)
set (Psychopath_VERSION_MINOR 0)
set (Psychopath_VERSION_PATCH 0)

# Build type (Release, Debug)
set (CMAKE_BUILD_TYPE Release)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file ("${PROJECT_SOURCE_DIR}/config.h.in"
                "${PROJECT_BINARY_DIR}/config.h")

# Warnings
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Profiling
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")

# Define "DEBUG" in debug builds
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")


#--------------------------------------
# Boost
#--------------------------------------

set (Boost_MULTITHREADED TRUE)
set (Boost_USE_STATIC_LIBS FALSE)

find_package (Boost 1.48 COMPONENTS program_options thread unit_test_framework REQUIRED)

include_directories (${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})


#--------------------------------------
# Open Image IO
#--------------------------------------
set(OIIO_DIR ${PROJECT_SOURCE_DIR}/../libs/oiio/dist/linux64)
set(OIIO_LIBRARY ${OIIO_DIR}/lib/libOpenImageIO.so)
set(OIIO_INCLUDE ${OIIO_DIR}/include)

include_directories (${OIIO_INCLUDE})


#--------------------------------------
# Open Shading Language
#--------------------------------------
set(OSL_DIR ${PROJECT_SOURCE_DIR}/../libs/osl/dist/linux64)
set(OSL_LIBRARYS ${OSL_DIR}/lib/liboslcomp.so
                   ${OSL_DIR}/lib/liboslexec.so
                   ${OSL_DIR}/lib/liboslquery.so)
set(OSL_INCLUDE ${OSL_DIR}/include)

include_directories (${OSL_INCLUDE})


#--------------------------------------
# Project include directories
#--------------------------------------
include_directories ("${PROJECT_BINARY_DIR}",
                     "${PROJECT_SOURCE_DIR}",
                     "${PROJECT_SOURCE_DIR}/acceleration",
                     "${PROJECT_SOURCE_DIR}/basics",
                     "${PROJECT_SOURCE_DIR}/grid",
                     "${PROJECT_SOURCE_DIR}/math",
                     "${PROJECT_SOURCE_DIR}/primitives",
                     "${PROJECT_SOURCE_DIR}/raster",
                     "${PROJECT_SOURCE_DIR}/renderer",
                     "${PROJECT_SOURCE_DIR}/sampling",
                     "${PROJECT_SOURCE_DIR}/scene",
                     "${PROJECT_SOURCE_DIR}/test")


#--------------------------------------
# Build
#--------------------------------------
add_subdirectory(acceleration)
add_subdirectory(basics)
add_subdirectory(grid)
add_subdirectory(math)
add_subdirectory(primitives)
add_subdirectory(raster)
add_subdirectory(renderer)
add_subdirectory(sampling)

add_library(config
	config)

# Main library
set (PSYCHO_LIB
	renderer
	raster
	sampling
	primitives
	basics
	grid
	acceleration
	config)

# Main executable
add_executable(psychopath
    main)
target_link_libraries(psychopath
	${PSYCHO_LIB}
    ${Boost_LIBRARIES}
    ${OIIO_LIBRARY}
    ${OSL_LIBRARYS})

# Unit-test executable
file(GLOB_RECURSE TEST_FILES *_test.cpp) # Find all tests

add_executable(unit_tests
	"test/test_main" ${TEST_FILES})
target_link_libraries(unit_tests
	${PSYCHO_LIB}
	${Boost_LIBRARIES})

