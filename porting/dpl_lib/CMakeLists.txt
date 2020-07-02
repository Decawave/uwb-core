project(dpl_lib VERSION ${VERSION} LANGUAGES C)

file(GLOB ${PROJECT_NAME}_SOURCES
    src/*.c
)
file(GLOB ${PROJECT_NAME}_HEADERS
#    include/dpl/*.h
#    include/json/*.h
    include/
)

include_directories(
    include
    "${PROJECT_SOURCE_DIR}/../dpl_os/include"
    "${PROJECT_SOURCE_DIR}/../../bin/targets/syscfg/generated/include/"
)

source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES})

add_library(${PROJECT_NAME}
    STATIC
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)

add_library(libdpl_linux ALIAS dpl_linux)
get_target_property(libdpl_linux_INCLUDE_DIRECTORIES libdpl_linux INCLUDE_DIRECTORIES)
add_library(libdpl_hal ALIAS dpl_hal)
get_target_property(libdpl_hal_INCLUDE_DIRECTORIES libdpl_hal INCLUDE_DIRECTORIES)

include(GNUInstallDirs)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${libdpl_linux_INCLUDE_DIRECTORIES}
        ${libdpl_hal_INCLUDE_DIRECTORIES}

)

install(DIRECTORY include/ DESTINATION include/
        FILES_MATCHING PATTERN *.h
)

# Install library
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
# Export library
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

export(
    PACKAGE ${PROJECT_NAME}
)

