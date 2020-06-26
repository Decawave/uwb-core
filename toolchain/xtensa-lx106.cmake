# Name of the target
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR xtensa)

set(LD_FLAGS "-nostartfiles -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static")

include(${CMAKE_CURRENT_LIST_DIR}/common/xtensa-lx106-gcc.cmake)
