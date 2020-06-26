cmake_minimum_required(VERSION 3.7)
project(twr_tag_tdma VERSION ${VERSION} LANGUAGES C CXX)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLINUX_BUILD")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX_BUILD")

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)

set(SYSCFG_NAME ${PROJECT_NAME}_syscfg)
set(SYSCFG_NAME_OUTPUTS "${PROJECT_BINARY_DIR}/../../../bin/targets/${SYSCFG_NAME}/generated/include/syscfg/syscfg.h" "${PROJECT_SOURCE_DIR}/../../bin/targets/${SYSCFG_NAME}/generated/src/${SYSCFG_NAME}-sysinit-app.c")

add_custom_command(OUTPUT ${SYSCFG_NAME_OUTPUTS}
  COMMAND newt build ${SYSCFG_NAME} >/dev/null 2>&1|| echo "done"
  DEPENDS "${PROJECT_BINARY_DIR}/../../../targets/${SYSCFG_NAME}" ${PROJECT_SOURCE_DIR}/syscfg.yml
  COMMENT "-- Building newt dependencies (${SYSCFG_NAME})"
  )

add_custom_command(OUTPUT "${PROJECT_BINARY_DIR}/../../../targets/${SYSCFG_NAME}"
  COMMAND newt target create ${SYSCFG_NAME}
  COMMAND newt target set ${SYSCFG_NAME} app=apps/${PROJECT_NAME}
  COMMAND newt target set ${SYSCFG_NAME} bsp=@decawave-uwb-core/hw/bsp/hikey960_dwmX000
  COMMENT "-- Building newt target (${SYSCFG_NAME})"
  )


file(GLOB ${PROJECT_NAME}_SOURCES
    src/*.c
    "${PROJECT_SOURCE_DIR}/../../bin/targets/${SYSCFG_NAME}/generated/src/${SYSCFG_NAME}-sysinit-app.c"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/uwb_dw1000/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../hw/bsp/hikey960_dwmX000/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_os/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl/linux/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl/linux/src/*.cc"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_hal/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_lib/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/json/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_rng/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/euclid/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/dsp/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss_ext/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss_ack/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ds/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ds_ext/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_ccp/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../lib/tdma/src/*.c"
    "${PROJECT_SOURCE_DIR}/../../sys/uwbcfg/src/*.c"
)

file(GLOB ${PROJECT_NAME}_HEADERS
    "${PROJECT_SOURCE_DIR}/../../bin/targets/${SYSCFG_NAME}/generated/include/syscfg/*.h"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_lib/include/dpl/*.h"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_lib/include/json/*.h"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl/linux/include/dpl/*.h"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_hal/include/hal/*.h"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/include/uwb/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/json/include/json/*.h"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/uwb_dw1000/include/dw1000/*.h"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/uwb_dw3000/include/dw3000/*.h"
    "${PROJECT_SOURCE_DIR}/../../hw/bsp/hikey960_dwmX000/include/bsp/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_ccp/include/uwb_ccp/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/timescale/include/timescale/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_rng/include/uwb_rng/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/euclid/include/euclid/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/dsp/include/dsp/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ds/include/twr_ds/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ds_ext/include/twr_ds/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss/include/twr_ss/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss_ext/include/twr_ss_ext/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss_ack/include/twr_ss_ack/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_wcs/include/uwb_wcs/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_ccp/include/uwb_ccp/*.h"
    "${PROJECT_SOURCE_DIR}/../../lib/tdma/include/tdma/*.h"
    "${PROJECT_SOURCE_DIR}/../../sys/uwbcfg/include/uwbcfg/*.h"
)

include_directories(

    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/src/"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/uwb_dw1000/src/"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/uwb_dw3000/src/"
    "${PROJECT_SOURCE_DIR}/../../hw/bsp/hikey960_dwmX000/include/"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_os/src/"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl/linux/src/"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_hal/src/"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_lib/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/json/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_rng/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/euclid/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/dsp/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss_ack/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss_ext/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ds/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ds_ext/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_wcs/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_ccp/src/"
    "${PROJECT_SOURCE_DIR}/../../lib/tdma/src/"
    "${PROJECT_SOURCE_DIR}/../../sys/uwbcfg/src/"

    "${PROJECT_SOURCE_DIR}/include/"
    "${PROJECT_SOURCE_DIR}/../../bin/targets/${SYSCFG_NAME}/generated/include"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_os/include"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl/linux/include"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_hal/include"
    "${PROJECT_SOURCE_DIR}/../../porting/dpl_lib/include/"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/include/"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/uwb_dw1000/include/"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/uwb_dw3000/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/json/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_rng/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/euclid/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/dsp/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss_ack/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ss_ext/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ds/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/twr_ds_ext/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_wcs/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/timescale/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/uwb_ccp/include/"
    "${PROJECT_SOURCE_DIR}/../../lib/tdma/include/"
    "${PROJECT_SOURCE_DIR}/../../sys/uwbcfg/include/"
    "${PROJECT_SOURCE_DIR}/../../hw/drivers/uwb/include/"
)

install(
  FILES
  "${PROJECT_BINARY_DIR}/../../../bin/targets/${SYSCFG_NAME}/generated/src/syscfg-sysinit-app.c"
  DESTINATION syscfg/src
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
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install library
install(DIRECTORY include/ DESTINATION include/
        FILES_MATCHING PATTERN "*.h"
)

if(ANDROID_APK_BUILD)
  target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE ANDROID_APK_BUILD=1
    )

  ## Also build 32-bit version of this library
  add_library(32_${PROJECT_NAME}
    STATIC
    ${SYSCFG_NAME_OUTPUTS}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
    )

  target_compile_definitions(
    32_${PROJECT_NAME}
    PRIVATE ANDROID_APK_BUILD=1
    )

  target_include_directories(32_${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

  target_compile_options(
    32_${PROJECT_NAME}
    PUBLIC
    -m32
    )

else()

  add_executable(
    TWR_TAG_TDMA
    src/main.c
    )

  if (AOSP)
    target_link_libraries(
      TWR_TAG_TDMA
      twr_tag_tdma
      uwb_dw1000
      uwb_dw3000
      dpl_os
      dpl_hal
      dpl_linux
      -static
      -lm
      )
  else()
    target_link_libraries(
      TWR_TAG_TDMA
      twr_tag_tdma
      uwb_dw1000
      uwb_dw3000
      dpl_os
      dpl_hal
      dpl_linux
      util
      -static
      -lrt
      -lm
      -lpthread
      )
  endif()
endif()



include(../../CMakeCommon.cmake)
