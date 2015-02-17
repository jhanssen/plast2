cmake_minimum_required(VERSION 2.8)

set(RCT_STATIC 1)

include(rct/rct.cmake)
add_subdirectory(scheduler)

add_dependencies(scheduler rct)
