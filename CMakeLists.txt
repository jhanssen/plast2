cmake_minimum_required(VERSION 2.8)

set(RCT_STATIC 1)

include(rct/rct.cmake)
include(common/common.cmake)

add_subdirectory(scheduler)
add_subdirectory(daemon)
add_subdirectory(client)

add_dependencies(scheduler rct common)
add_dependencies(daemon rct common)
add_dependencies(plastc rct common)

if (EXISTS "rules.ninja")
  add_custom_target(release COMMAND cmake -GNinja -DCMAKE_BUILD_TYPE=Release . WORKING_DIRECTORY .)
  add_custom_target(debug COMMAND cmake -GNinja -DCMAKE_BUILD_TYPE=Debug . WORKING_DIRECTORY .)
else()
  add_custom_target(release COMMAND cmake -DCMAKE_BUILD_TYPE=Release . WORKING_DIRECTORY)
  add_custom_target(debug COMMAND cmake -DCMAKE_BUILD_TYPE=Debug . WORKING_DIRECTORY)
endif()
