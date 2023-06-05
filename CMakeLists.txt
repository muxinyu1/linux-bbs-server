cmake_minimum_required(VERSION 3.0.0)

project(bbs-server)

find_package(Boost REQUIRED COMPONENTS system thread serialization)

add_executable(${PROJECT_NAME} main.cxx )

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

