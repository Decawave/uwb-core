# Name of the target
set(CMAKE_SYSTEM_PROCESSOR generic)

# Toolchain settings
set(CMAKE_C_COMPILER    gcc)
set(CMAKE_CXX_COMPILER  g++)
set(AS                  as)
set(AR                  ar)
set(OBJCOPY             objcopy)
set(OBJDUMP             objdump)
set(SIZE                size)

set(CMAKE_C_FLAGS   "-D_GNU_SOURCE -D_POSIX_C_SOURCE=200809 -std=c11 -Werror -Wformat=0 -fms-extensions -fdata-sections -ffunction-sections -Wno-missing-declarations -Wno-microsoft-anon-tag" CACHE INTERNAL "c compiler flags")
set(CMAKE_CXX_FLAGS "-D_GNU_SOURCE -D_POSIX_C_SOURCE=200809 -std=c++11 -fms-extensions -fdata-sections -ffunction-sections" CACHE INTERNAL "cxx compiler flags")
set(CMAKE_ASM_FLAGS "" CACHE INTERNAL "asm compiler flags")
if (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-dead_strip" CACHE INTERNAL "exe link flags")
else (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-lrt -Wl,--gc-sections" CACHE INTERNAL "exe link flags")
endif (APPLE)

SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -ggdb3" CACHE INTERNAL "c debug compiler flags")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb3" CACHE INTERNAL "cxx debug compiler flags")
SET(CMAKE_ASM_FLAGS_DEBUG "-g -ggdb3" CACHE INTERNAL "asm debug compiler flags")

SET(CMAKE_C_FLAGS_RELEASE "-O2 -g -ggdb3" CACHE INTERNAL "c release compiler flags")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -ggdb3" CACHE INTERNAL "cxx release compiler flags")
SET(CMAKE_ASM_FLAGS_RELEASE "" CACHE INTERNAL "asm release compiler flags")
