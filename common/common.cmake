set(MESSAGES_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/HttpServer.cpp
  ${CMAKE_CURRENT_LIST_DIR}/Messages.cpp
  ${CMAKE_CURRENT_LIST_DIR}/Plast.cpp
  ${CMAKE_CURRENT_LIST_DIR}/WebSocket.cpp)

add_library(common ${MESSAGES_SOURCES})

include_directories(${CMAKE_CURRENT_LIST_DIR})
