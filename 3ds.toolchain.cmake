# 3ds.toolchain.cmake
# Toolchain file for building Nintendo 3DS homebrew with devkitARM via CMake

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR armv6k)

# Path to devkitPro
set(DEVKITPRO /opt/devkitpro)
set(DEVKITARM ${DEVKITPRO}/devkitARM)

if(NOT EXISTS ${DEVKITARM})
    message(FATAL_ERROR "Please install devkitARM and make sure /opt/devkitpro/devkitARM exists.")
endif()

# Specify the cross compiler
set(CMAKE_C_COMPILER ${DEVKITARM}/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${DEVKITARM}/bin/arm-none-eabi-g++)
set(CMAKE_AR ${DEVKITARM}/bin/arm-none-eabi-ar CACHE FILEPATH "")
set(CMAKE_RANLIB ${DEVKITARM}/bin/arm-none-eabi-ranlib CACHE FILEPATH "")

# Bypass compiler checks as the bare compiler without -lctru will fail to link
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Search paths for libraries and headers
# We MUST tell CMake to only look here so it doesn't try finding macOS host libraries!
set(CMAKE_FIND_ROOT_PATH ${DEVKITPRO}/portlibs/3ds ${DEVKITPRO}/libctru)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# General compiler flags for 3DS
set(ARCH_FLAGS "-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_FLAGS} -O3 -mword-relocations -ffunction-sections -fdata-sections" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS} -O3 -mword-relocations -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -std=gnu++17" CACHE STRING "CXX flags")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${ARCH_FLAGS} -specs=3dsx.specs -g" CACHE STRING "Linker flags")

# Include paths for 3DS devkitPro standard libs
include_directories(SYSTEM ${DEVKITPRO}/libctru/include)
include_directories(SYSTEM ${DEVKITPRO}/portlibs/3ds/include)

# Tell the linker where to find the libraries
link_directories(${DEVKITPRO}/libctru/lib ${DEVKITPRO}/portlibs/3ds/lib)

# Let CMake know we are building for 3DS so we can branch in CMakeLists
set(N3DS TRUE CACHE BOOL "True if building for Nintendo 3DS" FORCE)

# Macro to generate a .3dsx file from an elf
macro(add_3dsx_target target)
    target_link_libraries(${target} PRIVATE -lcitro2d -lcitro3d -lctru -lm)
    
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${DEVKITPRO}/tools/bin/3dsxtool $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target}.3dsx
        COMMENT "Generating .3dsx file"
    )
endmacro()
