#include <stdint.h>
#include <stdio.h>
#include <gatt_db.h>
#include <SEGGER_RTT.h>
#include <native_gecko.h>
#include "io.h"
#include "dfu.h"
#include <em_device.h>
#include <bg_types.h>
#include <aat_def.h>
#include "gecko_configuration.h"
#include "native_gecko.h"

#define MAX_CONNECTIONS 1
#define AAT_VALUE   ((uint32_t)&__dfu_AAT)          // word value of AAT address
#define USER_BASE   (AAT_VALUE+FLASH_PAGE_SIZE)
#define USER_AAT    ((aat_t *)USER_BASE)

uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];
extern uint32_t __dfu_AAT;                          // our AAT address

/* Gecko configuration parameters (see gecko_configuration.h) */

static const gecko_configuration_t config = {
        .config_flags=0,
        .sleep.flags=0,
        .bluetooth.max_connections=MAX_CONNECTIONS,
        .bluetooth.heap=bluetooth_stack_heap,
        .bluetooth.heap_size=sizeof(bluetooth_stack_heap),
        .ota.flags=0,
        .ota.device_name_len=3,
        .ota.device_name_ptr="OTA",
        .gattdb=&bg_gattdb_data,
};

static void user_write(struct gecko_cmd_packet *evt) {
    struct gecko_msg_gatt_server_user_write_request_evt_t *writeStatus;
    unsigned i;

    writeStatus = &evt->data.evt_gatt_server_user_write_request;
    printf("Write value: attr=%d, opcode=%d, offset=%d, value:\n",
           writeStatus->characteristic, writeStatus->att_opcode, writeStatus->offset);
    for (i = 0; i != writeStatus->value.len; i++)
        printf("%02X ", writeStatus->value.data[i]);
    printf("\n");
    switch (writeStatus->characteristic) {
        uint8 response;
        case GATTDB_ota_control:
            response = (uint8) (processCtrlPacket(writeStatus->value.data) ? 1 : 0);
            gecko_cmd_gatt_server_send_user_write_response(writeStatus->connection, writeStatus->characteristic,
                                                           response);
            break;

        case GATTDB_ota_data:
            response = (uint8) (processDataPacket(writeStatus->value.data, writeStatus->value.len) ? 1 : 0);
            gecko_cmd_gatt_server_send_user_write_response(writeStatus->connection, writeStatus->characteristic,
                                                           response);
            break;


        default:
            gecko_cmd_gatt_server_send_user_write_response(writeStatus->connection, writeStatus->characteristic, 1);
            break;

    }

}

// this is the entry point for the firmware upgrader
void dfu_main(void) {
    //EMU_init();
    //CMU_init();
    printf("Started V2\n");
    if(USER_AAT->type != APP_ADDRESS_TABLE_TYPE)
        enterDfu = true;

    if(!enterDfu) {
        SCB->VTOR=(uint32_t)USER_AAT->vectorTable;
        //Use stack from aat
        asm("mov sp,%0" :: "r" (USER_AAT->topOfStack));
        USER_AAT->resetVector();
    }
    gecko_init(&config);
    printf("Stack initialised\n");


    for (;;) {
        /* Event pointer for handling events */
        struct gecko_cmd_packet *evt;

        /* Check for stack event. */
        evt = gecko_wait_event();

        /* Handle events */
#if DEBUG
        unsigned id = BGLIB_MSG_ID(evt->header) & ~gecko_dev_type_gecko;
        if (id != (gecko_evt_hardware_soft_timer_id & ~gecko_dev_type_gecko)) {
            if (id & gecko_msg_type_evt) {
                id &= ~gecko_msg_type_evt;
                printf("event = %X\n", id);
            } else if (id & gecko_msg_type_rsp) {
                id &= ~gecko_msg_type_rsp;
                printf("response = %X\n", id);
            }
        }
#endif
        switch (BGLIB_MSG_ID(evt->header)) {
            struct gecko_msg_gatt_server_characteristic_status_evt_t *status;
            struct gecko_msg_gatt_server_user_read_request_evt_t *readStatus;

            /* This boot event is generated when the system boots up after reset.
             * Here the system is set to start advertising immediately after boot procedure. */
            case gecko_evt_system_boot_id:
                printf("system_boot\n");

                /* Set advertising parameters. 100ms advertisement interval. All channels used.
             * The first two parameters are minimum and maximum advertising interval, both in
             * units of (milliseconds * 1.6). The third parameter '7' sets advertising on all channels. */
                gecko_cmd_le_gap_set_adv_parameters(100, 100, 7);

                /* Start general advertising and enable connections. */
                gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

                break;

            case gecko_evt_le_connection_opened_id:
                printf("Connection opened\n");
                break;

            case gecko_evt_le_connection_closed_id:
                printf("Connection closed\n");
                /* Restart advertising after client has disconnected */
                gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
                break;

            case gecko_evt_gatt_server_characteristic_status_id:
                status = &evt->data.evt_gatt_server_characteristic_status;
                printf("Char. status: connection=%X, characteristic=%d, status_flags=%X, client_config_flags=%X\n",
                       status->connection, status->characteristic, status->status_flags, status->client_config_flags);
                break;

            case gecko_evt_gatt_server_user_read_request_id:
                readStatus = &evt->data.evt_gatt_server_user_read_request;
                printf("Read request: connection=%X, characteristic=%d, status_flags=%X, offset=%d\n",
                       readStatus->connection, readStatus->characteristic, readStatus->att_opcode, readStatus->offset);
                break;

            case gecko_evt_gatt_server_user_write_request_id:
                user_write(evt);
                break;

            default:
                break;
        }
    }
}