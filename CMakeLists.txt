cmake_minimum_required(VERSION 3.0.0)

project(bbs-server)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS system thread serialization)

add_executable(${PROJECT_NAME} main.cxx http_helper.hxx )

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

