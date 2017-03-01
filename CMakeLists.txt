cmake_minimum_required(VERSION 2.8.12)
project(mobile-radio-monitor)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "MOBILE-RADIO-MONITOR: please DO NOT BUILD in-tree.")
endif()

###
# CMake Module usage
###
list(APPEND CMAKE_MODULE_PATH
  # https://github.com/Makman2/GCR_CMake
  ${CMAKE_CURRENT_SOURCE_DIR}/CMake_modules/GCR_CMake/macros)

cmake_policy(SET CMP0054 NEW)

include(GlibCompileResourcesSupport)

# L10N and I18N handling
#include(FindIntl)
#include(FindGettext)

###
# CMake external Libraries
###

# external lib definitions
find_library(M m)

# Use the package PkgConfig to detect external headers/library definitions
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0>=3.9.8)
pkg_check_modules(QMI REQUIRED qmi-glib>=1.4)
pkg_check_modules(GUDEV REQUIRED gudev-1.0>=147)

# external executables (provided by glib-2.0)
find_file(glib-mkenums_EXECUTABLE glib-mkenums PATH)
find_file(glib-compile-resources_EXECUTABLE glib-compile-resources PATH)

# external properties
add_library(gtk+-3.0 INTERFACE IMPORTED)
set_property(TARGET gtk+-3.0 PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES ${GTK3_INCLUDE_DIR})
add_library(qmi-glib INTERFACE IMPORTED)
set_property(TARGET qmi-glib PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES ${QMI_INCLUDE_DIR})
add_library(gudev-1.0 INTERFACE IMPORTED)
set_property(TARGET gudev-1.0 PROPERTY
  INTERFACE_INCLUDE_DIRECTORIES ${GUDEV_INCLUDE_DIR})
message(STATUS "External Libaries:")
message(STATUS " gtk+-3.0: ${GTK3_LIBRARIES}")
message(STATUS " qmi-glib: ${QMI_LIBRARIES}")
message(STATUS " gudev-1.0: ${GUDEV_LIBRARIES}")
message(STATUS "External Include-Direcotries:")
message(STATUS " gtk+-3.0: ${GTK3_INCLUDE_DIRS}")
message(STATUS " qmi-glib: ${QMI_INCLUDE_DIRS}")
message(STATUS " gudev-1.0: ${GUDEV_INCLUDE_DIRS}")

###
# System-Name specific settings
###
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

# Treat MSYS as Cygwin
if (MSYS)
    set(CYGWIN 1)
    set(CMAKE_SYSTEM_NAME "CYGWIN")
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "CYGWIN" OR WIN32)
  add_definitions(-DEXEEXT="\".EXE\"")
else ()
  add_definitions(-DEXEEXT="")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    add_definitions(-D__LONG_LONG_SUPPORTED)
endif ()

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(START_GROUP "")
    set(END_GROUP "")
else ()
    set(START_GROUP "-Wl,--start-group")
    set(END_GROUP "-Wl,--end-group")
endif ()

if (APPLE)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ggdb")
endif ()

if (ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

###
# Compiler specific settings
###
# this are cmake defaults
#message(STATUS "CMAKE_C_FLAGS_DEBUG is ${CMAKE_C_FLAGS_DEBUG}")
#message(STATUS "CMAKE_C_FLAGS_RELEASE is ${CMAKE_C_FLAGS_RELEASE}")
#message(STATUS "CMAKE_C_FLAGS_RELWITHDEBINFO is ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
#message(STATUS "CMAKE_C_FLAGS_MINSIZEREL is ${CMAKE_C_FLAGS_MINSIZEREL}")

#message(STATUS "Setup C++ Compiler specific attributes for ${CMAKE_CXX_COMPILER_ID}")
set(CMAKE_CXX_STANDARD 11)  # enable C++11 standard
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANGCXX TRUE)
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
  set(CMAKE_COMPILER_IS_GNUCXX TRUE)
endif()

# message(STATUS "Setup C Compiler specific attributes for ${CMAKE_C_COMPILER_ID}")
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR
    "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  string(APPEND CMAKE_C_COMPILER_WARNINGS " -Wall")
  #string(APPEND CMAKE_C_COMPILER_WARNINGS " -Wextra -Wno-cast-qual -Wno-shadow -Wno-sign-compare")
  string(APPEND CMAKE_C_COMPILER_WARNINGS " -Wno-unused-function -Wno-implicit-function-declaration")
  string(APPEND CMAKE_C_COMPILER_WARNINGS " -Wno-unused-parameter -Wno-unused-variable")
  string(APPEND CMAKE_C_COMPILER_WARNINGS " -Wno-deprecated-declarations -Wno-missing-field-initializers")
  if (NOT CYGWIN)
    string(APPEND CMAKE_C_COMPILER_WARNINGS " -Wstack-protector")
  endif ()
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    string(APPEND CMAKE_C_COMPILER_WARNINGS "/W4 /WX /EHsc")
endif()
if (NOT CONFIGURED_ONCE)
  set(CMAKE_CXX_FLAGS_WARNINGS ${CMAKE_C_COMPILER_WARNINGS}
    CACHE STRING "CXX: Flags used by the compiler during all build types.")
  set(CMAKE_C_FLAGS_WARNINGS "${CMAKE_C_COMPILER_WARNINGS}"
    CACHE STRING "C: Flags used by the compiler during all build types.")
  #string(APPEND CMAKE_CXX_FLAGS ${CMAKE_C_COMPILER_WARNINGS})
  string(APPEND CMAKE_C_FLAGS ${CMAKE_C_COMPILER_WARNINGS})
endif()

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANGC TRUE)
  string(APPEND CMAKE_C_FLAGS " -fstack-protector-strong")
  string(APPEND CMAKE_C_FLAGS " -pthread")
else ("${CMAKE_C_COMPILER_ID}" MATCHES "GNU")
  set(CMAKE_COMPILER_IS_GNUC TRUE)
  #string(APPEND CMAKE_C_FLAGS " -fvisibility=hidden -fstack-protector-strong -fPIC")
  string(APPEND CMAKE_C_FLAGS " -fstack-protector-strong")
  string(APPEND CMAKE_C_FLAGS " -pthread")
  string(APPEND CMAKE_C_FLAGS " -pipe")
  if (NOT CYGWIN)
    string(APPEND CMAKE_C_FLAGS " -fstack-protector-all")
  endif ()
endif()

message(STATUS "Using C++ Compiler specific attributes for ${CMAKE_CXX_COMPILER_ID}: ${CMAKE_CXX_FLAGS}")
#message(STATUS "Using C++ Compiler specific warnings for ${CMAKE_C_COMPILER_ID}: ${CMAKE_CXX_FLAGS_WARNINGS}")
message(STATUS "Using C Compiler specific attributes for ${CMAKE_C_COMPILER_ID}: ${CMAKE_C_FLAGS}")
#message(STATUS "Using C Compiler specific warnings for ${CMAKE_C_COMPILER_ID}: ${CMAKE_C_FLAGS_WARNINGS}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

include(FeatureSummary)

####
# Makefile Rules
###
if (EXISTS "rules.ninja")
    add_custom_target(release COMMAND ${CMAKE_COMMAND} -GNinja -DCMAKE_BUILD_TYPE=Release . WORKING_DIRECTORY .)
    add_custom_target(debug COMMAND ${CMAKE_COMMAND} -GNinja -DCMAKE_BUILD_TYPE=Debug . WORKING_DIRECTORY .)
else()
    add_custom_target(release COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release . WORKING_DIRECTORY .)
    add_custom_target(debug COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug . WORKING_DIRECTORY .)
endif()

###
# Project Settings: Global
###
message(STATUS "Mobile-Radio-Monitor: global definitions:")

# configure.ac: Extract the MOBILE-RADIO-MONITOR Values
set(MRM_VERSION_MAJOR 1)
set(MRM_VERSION_MINOR 0)
set(MRM_VERSION_BUILD 0)
set(MRM_VERSION_PATCHLEVEL 0)
set(MRM_VERSION_TAG 1)

set(MRM_VERSION_FULL_STRING ${MRM_VERSION_MAJOR}.${MRM_VERSION_MINOR}.${MRM_VERSION_PATCHLEVEL}.${MRM_VERSION_BUILD}${MRM_VERSION_TAG})
set(MRM_VERSION_STRING ${MRM_VERSION_MAJOR}.${MRM_VERSION_MINOR}.${MRM_VERSION_PATCHLEVEL}.-${MRM_VERSION_TAG})
message(STATUS " MRM_VERSION_MAJOR='${MRM_VERSION_MAJOR}'")
message(STATUS " MRM_VERSION_MINOR='${MRM_VERSION_MINOR}'")
message(STATUS " MRM_VERSION_PATCHLEVEL='${MRM_VERSION_PATCHLEVEL}'")
message(STATUS " MRM_VERSION_FULL_STRING='${MRM_VERSION_FULL_STRING}'")
message(STATUS " MRM_VERSION_STRING='${MRM_VERSION_STRING}'")

#if(NOT COMPILE_DATETIME)
#  string(TIMESTAMP COMPILE_DATETIME "%Y%m%d%H%M%S")
#endif()
#add_definitions(-DCOMPILE_DATETIME="${COMPILE_DATETIME}")

if (MRM_ENABLE_DEV_OPTIONS)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "mobile-radio-monitor")
  set(CPACK_PACKAGE_VENDOR "Aleksander Morgado")
  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A monitor for mobile radio environment parameters")
  set(CPACK_PACKAGE_CONTACT "https://bitbucket.org/aleksander0m/mobile-radio-monitor")
  SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
  set(CPACK_PACKAGE_VERSION_MAJOR ${MRM_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${MRM_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${MRM_VERSION_PATCHLEVEL})
  set(CPACK_PACKAGE_VERSION "${MRM_VERSION_FULL_STRING}")
  SET(CPACK_GENERATOR "DEB")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ralf Zerres")
  #SET(CPACK_GENERATOR "RPM")
  #SET(CPACK_RPM_PACKAGE_MAINTAINER "Ralf Zerres")
  include(CPack)
endif ()

set(MRM_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")
set(MRM_BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(MRM_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share")
set(MRM_LOCALE_DIR "${CMAKE_INSTALL_PREFIX}/share/locale")
set(MRM_HEADER_DIR "${CMAKE_INSTALL_PREFIX}/include/mobile-radio-monitor5")
set(MRM_POLICYDATA_DIR "${MRM_DATA_DIR}/polkit-1/actions")
set(MRM_DESKTOPDATA_DIR "${MRM_DATA_DIR}/applications")
set(MRM_ICONDATA_DIR "${MRM_DATA_DIR}/icons/hicolor/256x256/apps")

add_definitions(
  "-DCMAKE_BUILD"
  "-DOS_${CMAKE_SYSTEM_NAME}"
  "-DPACKAGE=\"mobile-radio-monitor\""
  "-DPACKAGE_VERSION=\"${MRM_VERSION_STRING}\""
  "-DLOCALEDIR=\"${MRM_LOCALE_DIR}\"")
  #"-DMRM_DESKTOPDATA_DIR=\"${MRM_DESKTOPDATA_DIR}\""
  #"-DMRM_ICONDATA_DIR=\"${MRM_ICONDATA_DIR}\"")

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions("-DMRM_DEBUG")
else()
  string(APPEND CMAKE_C_FLAGS " -D_FORTIFY_SOURCE=2")
endif ()

if (MRM_WITH_TESTS)
  message(STATUS "Mobile-Radio-Monitor: enable tests")
  set(MRM_WITH_TESTS true)
  add_definitions("-DMRM_TESTS")
endif ()

###
# Project Settings: pre-processor
###
get_directory_property(DirDefs COMPILE_DEFINITIONS)
message(STATUS "Mobile-Radio-Monitor: pre-processor definitions:")
foreach(d ${DirDefs} )
    message( STATUS " " ${d} )
endforeach()

feature_summary(INCLUDE_QUIET_PACKAGES WHAT ALL)

###
# Project Settings: License
###
set(mrm_license_DATA
  COPYING)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(tests)

install(CODE "message(\"Installing ...\")")
install(FILES ${mrm_license_DATA} DESTINATION ${MRM_DATA_DIR}/share/license/${project} COMPONENT mrm
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
