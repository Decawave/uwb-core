
# Toolchain settings
set(CMAKE_C_COMPILER    aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER  aarch64-linux-gnu-g++)
set(AS                  aarch64-linux-gnu-as)
set(AR                  aarch64-linux-gnu-ar)
set(OBJCOPY             aarch64-linux-gnu-objcopy)
set(OBJDUMP             aarch64-linux-gnu-objdump)
set(SIZE                aarch64-linux-gnu-size)


set(MCPU_FLAGS "-mcpu=cortex-a73")
set(CMAKE_C_FLAGS   "${MCPU_FLAGS} -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809 -std=gnu11 -Werror -Wformat=0 -fms-extensions -fdata-sections -ffunction-sections -Wno-missing-declarations" CACHE INTERNAL "c compiler flags")
set(CMAKE_CXX_FLAGS "${MCPU_FLAGS} -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809 -std=gnu++11 -fms-extensions -fdata-sections -ffunction-sections" CACHE INTERNAL "cxx compiler flags")
set(CMAKE_ASM_FLAGS "${MCPU_FLAGS}" CACHE INTERNAL "asm compiler flags")
if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-dead_strip" CACHE INTERNAL "exe link flags")
else (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${MCPU_FLAGS} -Wl,--gc-sections" CACHE INTERNAL "exe link flags")
endif (APPLE)

SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -ggdb3" CACHE INTERNAL "c debug compiler flags")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb3" CACHE INTERNAL "cxx debug compiler flags")
SET(CMAKE_ASM_FLAGS_DEBUG "-g -ggdb3" CACHE INTERNAL "asm debug compiler flags")

SET(CMAKE_C_FLAGS_RELEASE "-O2 -g -ggdb3" CACHE INTERNAL "c release compiler flags")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -ggdb3" CACHE INTERNAL "cxx release compiler flags")
SET(CMAKE_ASM_FLAGS_RELEASE "" CACHE INTERNAL "asm release compiler flags")
