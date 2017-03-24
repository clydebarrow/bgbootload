INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)

set(GCC_DIR /Users/clyde/dev/tools/4.9_2015q3)
set(OBJSIZE ${GCC_DIR}/bin/arm-none-eabi-size)
# specify the cross compiler
CMAKE_FORCE_C_COMPILER(${GCC_DIR}/bin/arm-none-eabi-gcc GNU)
CMAKE_FORCE_CXX_COMPILER(${GCC_DIR}/bin/arm-none-eabi-g++ GNU)

#SET(COMMON_FLAGS "-mcpu=cortex-m4 -mthumb -mthumb-interwork -mfloat-abi=soft -ffunction-sections -fdata-sections -g -fno-common -fmessage-length=0")
SET(COMMON_FLAGS "-mcpu=cortex-m4 -mthumb -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -g -fno-common -fmessage-length=0")
SET(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -std=c++11")
SET(CMAKE_C_FLAGS "${COMMON_FLAGS} -std=gnu99")
