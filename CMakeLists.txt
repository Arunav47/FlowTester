cmake_minimum_required(VERSION 3.20)

project(FlowTester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(yaml-cpp CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

add_library(flowtester_parser
    src/yaml_parser/parser.cpp
)

add_library(flowtester_validator
    src/validator.cpp
)

target_include_directories(flowtester_validator
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(flowtester_parser
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/yaml_parser
)

target_link_libraries(flowtester_parser
    PUBLIC
        yaml-cpp
)

add_executable(flowtester
    src/main.cpp
)

target_link_libraries(flowtester
    PRIVATE
        flowtester_parser
        flowtester_validator
)

enable_testing()

add_executable(parser_tests tests/parser_tests.cpp)
target_link_libraries(parser_tests PRIVATE flowtester_parser GTest::gtest_main)
target_compile_definitions(parser_tests PRIVATE
    FLOWTESTER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

include(GoogleTest)
gtest_discover_tests(parser_tests)
