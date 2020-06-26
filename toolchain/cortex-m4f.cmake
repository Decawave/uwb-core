# Name of the target
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR armv7-m-hard)

set(MCPU_FLAGS "-mthumb -mcpu=cortex-m4")
set(VFP_FLAGS "-mfloat-abi=hard -mfpu=fpv4-sp-d16")
set(LD_FLAGS "-nostartfiles")

include(${CMAKE_CURRENT_LIST_DIR}/common/arm-none-eabi.cmake)
