cmake_minimum_required(VERSION 3.6)
project(bgbootload)

set(CMAKE_C_STANDARD 99)
add_definitions(-D__NO_SYSTEM_INIT)
add_definitions(-DEFR32BG1B232F256GM48)
add_definitions(-D__FPU_PRESENT)

include_directories(inc)
include_directories(gatt)
include_directories(libs/SEGGER_RTT_V612j/RTT)
include_directories(platform/emlib/inc)
include_directories(platform/Device/SiliconLabs/EFR32BG1B/Include)
include_directories(platform/CMSIS/Include)

set(GATT_DB gatt_db)
set(GATT_DIR ${CMAKE_SOURCE_DIR}/gatt)
set(BLE_STACK_DIR ${CMAKE_SOURCE_DIR}/libs/ble.v2.1.1.0)
set(GATT_SRC ${GATT_DIR}/gatt.bgproj)
set(GATT_XML ${GATT_DIR}/gatt.xml)
set(GATT_OUTPUTS ${GATT_DIR}/${GATT_DB}.c ${GATT_DIR}/${GATT_DB}.h)
set(BGBUILD_CMD "/Users/clyde/dev/tools/ble/v2.1.1.0/protocol/bluetooth_2.1/bin/bgbuild")
set(EBL_CMD "/Applications/Simplicity Studio.app/Contents/Eclipse/developer/adapter_packs/commander/Commander.app/Contents/MacOS/commander")

file(GLOB_RECURSE USER_SOURCES "src/*.c")
file(GLOB_RECURSE USER_HEADERS "inc/*.h")

IF (CMAKE_BUILD_TYPE MATCHES Debug)
    file(GLOB_RECURSE RTT_LIBS "libs/SEGGER_RTT_V612j/RTT/*.c")
    add_definitions(-DDEBUG)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

set(LIBRARIES ${BLE_STACK_DIR}/stack.a ${BLE_STACK_DIR}/binstack.o )
SET(LINKER_SCRIPT bgbootload.ld)
set(STARTUP "src/startup.c")
#set_property(SOURCE ${STARTUP} PROPERTY LANGUAGE C)


# add library sources needed

SET(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.ld)
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-gc-sections -T ${LINKER_SCRIPT} --specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_NAME}.map")

add_custom_command(
        OUTPUT ${GATT_OUTPUTS}
        DEPENDS ${GATT_SRC} ${GATT_XML}
        COMMAND ${BGBUILD_CMD} -gn ${GATT_SRC})
set_source_files_properties(${GATT_OUTPUTS} PROPERTIES GENERATED TRUE)

add_executable(${PROJECT_NAME}.elf ${USER_SOURCES} ${USER_HEADERS} ${LINKER_SCRIPT} gatt/${GATT_DB}.c ${RTT_LIBS})
target_link_libraries(${PROJECT_NAME}.elf ${LIBRARIES})
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Obinary --gap-fill 255 $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${BIN_FILE}"
        COMMAND ${OBJSIZE} ${PROJECT_NAME}.elf)

IF (CMAKE_BUILD_TYPE MATCHES Release)
    add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Osrec -j .text_stack\* $<TARGET_FILE:${PROJECT_NAME}.elf> file.srec
        COMMAND ${CMAKE_OBJCOPY} -Osrec -j .text_app\* $<TARGET_FILE:${PROJECT_NAME}.elf> file.srec
        #COMMAND rm file.srec
)
ENDIF (CMAKE_BUILD_TYPE MATCHES Release)

