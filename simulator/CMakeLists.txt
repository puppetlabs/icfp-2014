cmake_minimum_required(VERSION 2.8)
project(ICFP-2014)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Defaulting to a release build.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package(Boost REQUIRED COMPONENTS regex)

set(LAMBDA_MAN_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/world.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/aiproc.cpp
    )

include_directories(
    ${Boost_INCLUDE_DIRS}
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(lambda-man ${LAMBDA_MAN_SOURCES})
target_link_libraries(lambda-man ${Boost_LIBRARIES})

add_test("run_world" "${PROJECT_BINARY_DIR}/lamdba-man")

