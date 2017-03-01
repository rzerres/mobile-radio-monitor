cmake_minimum_required(VERSION 2.8.12)


set(mrm_test-graph_SOURCES
  test-graph.c)

add_executable(test-graph
  $<TARGET_OBJECTS:mrm_graph_objects>
  ${mrm_test-graph_SOURCES})

add_dependencies(test-graph
  mrm_types_generated)

target_include_directories(test-graph PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../src;${CMAKE_CURRENT_SOURCE_DIR}/../src;${QMI_INCLUDE_DIRS};${GTK3_INCLUDE_DIRS};${GUDEV_INCLUDE_DIRS};${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}>")

target_link_libraries(test-graph LINK_PUBLIC
  "${QMI_LIBRARIES}"
  "${GTK3_LIBRARIES}"
  "${M}")

# Install
#install(CODE "message(\"Installing tests...\")")
#install(TARGETS test-graph  COMPONENT mrm
#  RUNTIME DESTINATION  ${MRM_BIN_DIR}
#  LIBRARY DESTINATION ${MRM_LIB_DIR}
#  ARCHIVE DESTINATION ${MRM_LIB_DIR})
#install(FILES ${mrm_test-graph_HEADERS} DESTINATION ${MRM_HEADER_DIR} COMPONENT mrm
#    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ  WORLD_READ)
