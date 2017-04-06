INCLUDE(CMakeForceCompiler)
SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)

# The gcc tools start with this prefix
set(GCC_PREFIX arm-none-eabi)

# locate the c compiler, provide a list of paths for Cmake to look in.
find_path(GCC_DIR bin/${GCC_PREFIX}-gcc
        $ENV{HOME}/dev/tools/4.9_2015q3)

set(OBJSIZE ${GCC_DIR}/bin/${GCC_PREFIX}-size)
# specify the cross compiler
CMAKE_FORCE_C_COMPILER(${GCC_DIR}/bin/${GCC_PREFIX}-gcc GNU)
CMAKE_FORCE_CXX_COMPILER(${GCC_DIR}/bin/${GCC_PREFIX}-g++ GNU)

# These flags are appropriate for the SiLabs EFR32BG series chips
SET(COMMON_FLAGS "-mcpu=cortex-m4 -mthumb -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -g -fno-common -fmessage-length=0")
SET(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=c++11" CACHE STRING "" FORCE)
SET(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu99" CACHE STRING "" FORCE)
