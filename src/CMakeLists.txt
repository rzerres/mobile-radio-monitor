cmake_minimum_required(VERSION 2.8.12)

set(EXTRA_DIST
  mrm-window.ui
  mrm-signal-tab.ui
  mrm-power-tab.ui
  mrm.gresource.xml)

###
# handle error type creation
###
set(mrm_enum_error_HEADERS
  mrm-error.h)

add_custom_target(mrm-error-types.h DEPENDS ${mrm_enum_error_HEADERS})
add_custom_command(TARGET mrm-error-types.h PRE_BUILD
  COMMAND ${glib-mkenums_EXECUTABLE}
  --fhead \"\#ifndef __MRM_ERROR_TYPES_H__\\n\#define __MRM_ERROR_TYPES_H__\\n\"
  --template ${CMAKE_SOURCE_DIR}/build-aux/mrm-error-types-template.h
  --ftail \"\#endif /* __MRM_ERROR_TYPES_H__ */\\n\"
  ${mrm_enum_error_HEADERS} > ${CMAKE_CURRENT_BINARY_DIR}/mrm-error-types.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  #VERBATIM
  COMMENT "Create type-header: mrm-error-types.h")

add_custom_target(mrm-error-types.c DEPENDS ${mrm_enum_error_HEADERS})
add_custom_command(TARGET mrm-error-types.c PRE_BUILD
  COMMAND ${glib-mkenums_EXECUTABLE}
  --fhead \"\#include \\"mrm-error.h\\"\\n\#include \\"mrm-error-types.h\\"\\n\"
  --template ${CMAKE_SOURCE_DIR}/build-aux/mrm-error-types-template.c
  ${mrm_enum_error_HEADERS} > ${CMAKE_CURRENT_BINARY_DIR}/mrm-error-types.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  #VERBATIM
  COMMENT "Create type-source: mrm-error-types.c")

###
# handle enum type creation
###
set(mrm_enum_types_HEADERS
  mrm-device.h
  mrm-graph.h)

add_custom_target(mrm-enum-types.h DEPENDS ${mrm_enum_types_HEADERS})
add_custom_command(TARGET mrm-enum-types.h PRE_BUILD
  COMMAND ${glib-mkenums_EXECUTABLE}
  --fhead \"\#include \\"mrm-device.h\\"\\n\#include \\"mrm-graph.h\\"\\n\#ifndef __MRM_ENUM_TYPES_H__\\n\#define __MRM_ENUM_TYPES_H__\\n\"
  --template ${CMAKE_SOURCE_DIR}/build-aux/mrm-enum-types-template.h
  --ftail \"\#endif /* __MRM_ENUM_TYPES_H__ */\\n\"
  ${mrm_enum_types_HEADERS} > ${CMAKE_CURRENT_BINARY_DIR}/mrm-enum-types.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  #VERBATIM
  COMMENT "Create type-header: mrm-enum-types.h")

add_custom_target(mrm-enum-types.c DEPENDS ${mrm_enum_types_HEADERS})
add_custom_command(TARGET mrm-enum-types.c PRE_BUILD
  COMMAND ${glib-mkenums_EXECUTABLE}
  --fhead \"\#include \\"mrm-enum-types.h\\"\\n\"
  --template ${CMAKE_SOURCE_DIR}/build-aux/mrm-enum-types-template.c
  ${mrm_enum_types_HEADERS} > ${CMAKE_CURRENT_BINARY_DIR}/mrm-enum-types.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  #VERBATIM
  COMMENT "Create type-source: mrm-enum-types.c")

###
# handle types targets
###
set(mrm_types_HEADERS
  mrm-error-types.h
  mrm-enum-types.h)

set(mrm_types_SOURCES
  mrm-enum-types.c
  mrm-error-types.c)

add_custom_target(mrm_types_generated DEPENDS
  ${mrm_types_HEADERS}
  ${mrm_types_SOURCES}
  COMMENT "Create mrm-types: header, sources")

# Tell CMake the source won't be available until build time.
set_source_files_properties(
  ${mrm_types_HEADERS}
  ${mrm_types_SOURCES}
  PROPERTIES GENERATED TRUE)

##
# handle gnome gresources creation
###
set(mrm_gresources_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/mrm.gresource.xml
  STRIPBLANKS mrm-window.ui
  STRIPBLANKS mrm-signal-tab.ui
  STRIPBLANKS mrm-power-tab.ui
  )

#compile_gresources(mrm.gresource.xml
#compile_gresources(mrm_gresources_HEADERS
#  XML_OUT
#  TYPE EMBED_H
#  CNAME mrm-h
#  TARGET mrm-resources.h
#  RESOURCES ${mrm_gresources_SOURCES})

compile_gresources(mrm_gresources_SOURCES
  XML_OUT
  TYPE EMBED_C
  TARGET mrm-resources.c
  PREFIX "/es/aleksander/mrm"
  CNAME mrm
  RESOURCES ${mrm_gresources_SOURCES})

set(mrm_resources_HEADERS
  mrm-resource.h)

set(mrm_resources_SOURCES
  mrm-resources.c)

add_custom_target(mrm_resources_generated DEPENDS
  mrm_types_generated
  #${mrm_gresources_HEADERS}
  ${mrm_gresources_SOURCES}
  COMMENT "Create mrm-resources.c")

# Tell CMake the source won't be available until build time.
SET_SOURCE_FILES_PROPERTIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/mrm.gresource.xml
  ${CMAKE_CURRENT_BINARY_DIR}/${mrm_resources_SOURCES}
  ${CMAKE_CURRENT_BINARY_DIR}/${mrm_resources_HEADERS}
  PROPERTIES GENERATED TRUE)

###
# Mobile-Radio-Monitor: graph
###
set(mrm_graph_HEADERS
  mrm-graph.h)

set(mrm_graph_SOURCES
  mrm-color-icon.c
  mrm-graph.c)

add_library(mrm_graph_objects OBJECT
  ${mrm_types_SOURCES}
  ${mrm_graph_SOURCES})

add_dependencies(mrm_graph_objects
  mrm_types_generated
  mrm_resources_generated)

target_include_directories(mrm_graph_objects PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR};${QMI_INCLUDE_DIRS};${GTK3_INCLUDE_DIRS};${GUDEV_INCLUDE_DIRS};${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}>")

###
# Mobile-Radio-Monitor: base
###
set(mrm_HEADERS
  ${mrm-enum_error_HEADERS}
  ${mrm-enum_types_HEADERS}
  ${mrm-types_HEADERS}
  ${mrm_resources_HEADERS}
  mrm-color-icon.h
  mrm-signal-tab.h
  mrm-power-tab.h
  mrm-window.h
  mrm-app.h)

set(mrm_SOURCES
  mrm-app.c
  mrm-main.c
  mrm-device.c
  mrm-power-tab.c
  mrm-signal-tab.c
  mrm-window.c)

set(mobile-radio-monitor_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/mrm-resources.c
  ${mrm_resources_SOURCES}
  ${mrm_SOURCES})

add_executable(mobile-radio-monitor
  $<TARGET_OBJECTS:mrm_graph_objects>
  ${mobile-radio-monitor_SOURCES})

add_dependencies(mobile-radio-monitor
  mrm_types_generated
  mrm_resources_generated)

target_include_directories(mobile-radio-monitor PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR};${QMI_INCLUDE_DIRS};${GTK3_INCLUDE_DIRS};${GUDEV_INCLUDE_DIRS};${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}>")

target_link_libraries(mobile-radio-monitor LINK_PUBLIC
  "${GTK3_LIBRARIES}"
  -lqmi-glib
  "${QMI_LIBRARIES}"
  -lgudev-1.0
  "${GUDEV_LIBRARIES}"
  "${M}")

# Install
install(CODE "message(\"Installing src...\")")
install(TARGETS mobile-radio-monitor COMPONENT mrm
  RUNTIME DESTINATION  ${MRM_BIN_DIR}
  LIBRARY DESTINATION ${MRM_LIB_DIR}
  ARCHIVE DESTINATION ${MRM_LIB_DIR} )
#install(FILES ${mrm_HEADERS} DESTINATION $(MRM_HEADER_DIR) COMPONENT mrm
#    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ  WORLD_READ)
