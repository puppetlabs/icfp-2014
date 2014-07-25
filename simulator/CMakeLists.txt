cmake_minimum_required(VERSION 2.8)
project(ICFP-2014)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Defaulting to a release build.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

find_package(Boost REQUIRED)

set(LAMBDA_MAN_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/world.cpp
    )

include_directories(
    ${Boost_INCLUDE_DIRS}
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(lamdba-man ${LAMBDA_MAN_SOURCES})

add_test("run_world" "${PROJECT_BINARY_DIR}/lamdba-man")

