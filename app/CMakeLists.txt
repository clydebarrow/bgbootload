cmake_minimum_required(VERSION 3.6)
project(app
        VERSION 1.0)

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

# find the bgbuild command used to translate xml into C
find_program(BGBUILD_CMD bin/bgbuild
        $ENV{HOME}/dev/tools/ble/v2.1.1.0/protocol/bluetooth_2.1)

# find the bgfirmware utility. Firstly look in the parent directory/utils. Customise this for your project layout
find_program(FIRMWARE_CMD bgfirmware
        PATHS ${CMAKE_SOURCE_DIR}/../utils/build-debug)

set(GATT_DB gatt_db)
set(GATT_DIR ${CMAKE_SOURCE_DIR}/gatt)
set(LOCALDIR ${CMAKE_SOURCE_DIR}/localdefs)
find_file(OTA_SERVICE_UUID_FILE ota_service.txt
        ${LOCALDIR}
        ${CMAKE_SOURCE_DIR}/../bootload/localdefs)
find_file(OTA_KEY_FILE ota_key.txt
        ${LOCALDIR}
        ${CMAKE_SOURCE_DIR}/../bootload/localdefs)

set(BLE_STACK_DIR ${CMAKE_SOURCE_DIR}/libs/ble.v2.1.1.0)
set(GATT_SRC ${GATT_DIR}/gatt.bgproj)
set(GATT_XML ${GATT_DIR}/gatt.xml)
set(GATT_I ${GATT_DIR}/gatt.i)
set(GATT_OUTPUTS ${GATT_DIR}/${GATT_DB}.c ${GATT_DIR}/${GATT_DB}.h)
set(BIN_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin)
set(FMW_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.fmw)

file(GLOB_RECURSE USER_SOURCES "src/*.c")
file(GLOB_RECURSE USER_HEADERS "inc/*.h")
file(STRINGS ${OTA_SERVICE_UUID_FILE} OTA_SERVICE)
file(STRINGS ${OTA_KEY_FILE} OTA_KEY)

IF (CMAKE_BUILD_TYPE MATCHES Debug)
    file(GLOB_RECURSE RTT_LIBS "libs/SEGGER_RTT_V612j/RTT/*.c")
    add_definitions(-DDEBUG)
ENDIF (CMAKE_BUILD_TYPE MATCHES Debug)

set(LIBRARIES ${BLE_STACK_DIR}/stack.a)
SET(LINKER_SCRIPT app.ld)
set(STARTUP "src/startup.c")
#set_property(SOURCE ${STARTUP} PROPERTY LANGUAGE C)

set(CMAKE_COM)
# add library sources needed

SET(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.ld)
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,-gc-sections -T ${LINKER_SCRIPT} --specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_NAME}.map,--no-check-sections")

add_custom_command(
        OUTPUT ${GATT_OUTPUTS}
        DEPENDS ${GATT_SRC} ${GATT_XML}
        COMMAND ${BGBUILD_CMD} -gn ${GATT_SRC})
set_source_files_properties(${GATT_OUTPUTS} PROPERTIES GENERATED TRUE)

add_executable(${PROJECT_NAME}.elf ${USER_SOURCES} ${USER_HEADERS} ${LINKER_SCRIPT} gatt/${GATT_DB}.c ${RTT_LIBS})
target_link_libraries(${PROJECT_NAME}.elf ${LIBRARIES})
set_target_properties(${PROJECT_NAME}.elf PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMENT "Building ${BIN_FILE}"
        COMMAND ${CMAKE_OBJCOPY} -Obinary --gap-fill 255 $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${FMW_FILE}"
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
        COMMAND ${FIRMWARE_CMD} -o${FMW_FILE} -n${PROJECT_VERSION} -k${OTA_KEY} -s${OTA_SERVICE} ${PROJECT_NAME}.hex
        COMMAND ${OBJSIZE} ${PROJECT_NAME}.elf)


