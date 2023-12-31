cmake_minimum_required(VERSION 3.1)
# note: cxx-17 requires cmake 3.8, cxx-20 requires cmake 3.12
project(tempsensor CXX)

file(GLOB SRC_FILES
        "*.cpp"
        "CommandLineUtils.cpp"
        "CommandLineUtils.h"
        )

add_executable(${PROJECT_NAME} ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 14)

#set warnings
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd4068)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wno-long-long -pedantic -Werror)
endif ()

find_package(aws-crt-cpp REQUIRED)

target_link_libraries(${PROJECT_NAME} AWS::aws-crt-cpp)

install(TARGETS tempsensor DESTINATION bin)
