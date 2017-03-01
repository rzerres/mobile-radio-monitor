cmake_minimum_required(VERSION 2.8.12)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/es.aleksander.MobileRadioMonitor.pkexec.policy.in"
  "${CMAKE_CURRENT_BINARY_DIR}/es.aleksander.MobileRadioMonitor.pkexec.policy")

set(mrm_policy_DATA
  ${CMAKE_CURRENT_BINARY_DIR}/es.aleksander.MobileRadioMonitor.pkexec.policy)

set(mrm_desktop_DATA
 mobile-radio-monitor.desktop)

set(mrm_image_DATA
 mobile-radio-monitor.png)

# Install
install(CODE "message(\"Installing data...\")")
install(FILES ${mrm_policy_DATA} DESTINATION ${MRM_POLICYDATA_DIR} COMPONENT mrm
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
install(FILES ${mrm_desktop_DATA} DESTINATION ${MRM_DESKTOPDATA_DIR} COMPONENT mrm
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
install(FILES ${mrm_image_DATA} DESTINATION ${MRM_ICONDATA_DIR} COMPONENT mrm
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
