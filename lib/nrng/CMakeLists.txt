project(nrng VERSION ${VERSION} LANGUAGES C)

file(GLOB ${PROJECT_NAME}_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.c
)

file(GLOB ${PROJECT_NAME}_HEADERS
    ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/*.h
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include/
    ${PROJECT_SOURCE_DIR}/../../bin/targets/syscfg/generated/include/
)

source_group(include/${PROJECT_NAME} FILES ${${PROJECT_NAME}_HEADERS})
source_group(lib FILES ${${PROJECT_NAME}_SOURCES})

add_library(${PROJECT_NAME}
    STATIC
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
)

include(GNUInstallDirs)
target_include_directories(${PROJECT_NAME}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    ${PROJECT_NAME}
    uwb
    cir
    dpl_os
    dpl_hal
    json
    euclid
    uwb_rng
    uwb_wcs
    uwb_ccp
    twr_ss_nrng
    dsp
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