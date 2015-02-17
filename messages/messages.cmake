set(MESSAGES_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/Messages.cpp)

add_library(messages ${MESSAGES_SOURCES})

include_directories(${CMAKE_CURRENT_LIST_DIR})
