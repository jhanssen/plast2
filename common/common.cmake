set(MESSAGES_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/HttpServer.cpp
  ${CMAKE_CURRENT_LIST_DIR}/Messages.cpp
  ${CMAKE_CURRENT_LIST_DIR}/Plast.cpp
  ${CMAKE_CURRENT_LIST_DIR}/WebSocket.cpp)

add_library(common ${MESSAGES_SOURCES})

if (APPLE)
  find_library(SECURITY_LIBRARY Security)
  target_link_libraries(common ${SECURITY_LIBRARY})
endif()
target_link_libraries(common -L${CMAKE_CURRENT_LIST_DIR}/../lib wslay)

include_directories(${CMAKE_CURRENT_LIST_DIR})
