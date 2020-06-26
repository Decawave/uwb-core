project(syscfg VERSION ${VERSION} LANGUAGES C)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)

set(SYSCFG_NAME ${PROJECT_NAME})
set(SYSCFG_NAME_OUTPUTS "${PROJECT_BINARY_DIR}/../../../bin/targets/${SYSCFG_NAME}/generated/include/syscfg/syscfg.h" "${PROJECT_SOURCE_DIR}/../../bin/targets/${SYSCFG_NAME}/generated/src/${SYSCFG_NAME}-sysinit-app.c")

add_custom_command(OUTPUT ${SYSCFG_NAME_OUTPUTS}
  COMMAND newt build ${SYSCFG_NAME} || echo "done"
  DEPENDS "${PROJECT_BINARY_DIR}/../../../targets/${SYSCFG_NAME}" ${PROJECT_SOURCE_DIR}/syscfg.yml
  COMMENT "-- Building newt dependencies (${SYSCFG_NAME})"
  )

add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/../../../targets/${SYSCFG_NAME}"
  COMMAND newt target create ${SYSCFG_NAME}
  COMMAND newt target set ${SYSCFG_NAME} app=apps/${PROJECT_NAME}
  COMMAND newt target set ${SYSCFG_NAME} bsp=@decawave-uwb-core/hw/bsp/hikey960_dwmX000
  COMMENT "-- Building newt target"
  )

file(GLOB ${PROJECT_NAME}_SOURCES
    src/*.c
    "${PROJECT_BINARY_DIR}/../../../bin/targets/${PROJECT_NAME}/generated/src/syscfg-sysinit-app.c"
)
file(GLOB ${PROJECT_NAME}_HEADERS
    include/*.h
    "${PROJECT_BINARY_DIR}/../../../bin/targets/${PROJECT_NAME}/generated/include/syscfg/*.h"
    "${PROJECT_BINARY_DIR}/../../../bin/targets/${PROJECT_NAME}/generated/include/sysflash/*.h"
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include/
    ${PROJECT_SOURCE_DIR}/../../../bin/targets/syscfg/generated/include/
)

install(
  FILES
  ${PROJECT_BINARY_DIR}/../../../bin/targets/${PROJECT_NAME}/generated/src/syscfg-sysinit-app.c
  DESTINATION syscfg/src
)

install(
  FILES
  ${PROJECT_BINARY_DIR}/../../../bin/targets/${PROJECT_NAME}/generated/include/syscfg/syscfg.h
  DESTINATION syscfg/include
)

install(
  FILES
  ${${PROJECT_NAME}_HEADERS}
  DESTINATION syscfg/include
)

add_library(${PROJECT_NAME}
    STATIC
    ${SYSCFG_NAME_OUTPUTS}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)

include(GNUInstallDirs)
target_include_directories(${PROJECT_NAME}
    PUBLIC
#      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../bin/targets/${PROJECT_NAME}/generated/include/>
      PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install library
install(DIRECTORY include/ DESTINATION include/
        FILES_MATCHING PATTERN "*.h"
)


# Install library
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
# Create a FILE that may be included by outside projects to import this targets
install(
    EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-config.cmake
    NAMESPACE uwb-core::
    DESTINATION ${CMAKE_LIBRARY_PATH}/${PROJECT_NAME}
)

install(
  FILES ${PROJECT_NAME}-config.cmake
  DESTINATION ${CMAKE_LIBRARY_PATH}/${PROJECT_NAME}
  CONFIGURATIONS Debug
)

export(
    TARGETS ${PROJECT_NAME}
    FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    NAMESPACE uwb-core::
    EXPORT_LINK_INTERFACE_LIBRARIES
)

# Store the current build directory in the CMake user package registry for package
export(
    PACKAGE ${PROJECT_NAME}
)




