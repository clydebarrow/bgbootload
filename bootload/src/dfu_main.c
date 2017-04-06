#include <stdint.h>
#include <stdio.h>
#include <gatt_db.h>
#include <SEGGER_RTT.h>
#include <native_gecko.h>
#include <io.h>
#include <dfu.h>
#include <em_device.h>
#include <bg_types.h>
#include <aat_def.h>
#include <em_crypto.h>
#include "gecko_configuration.h"
#include "native_gecko.h"

#define MAX_CONNECTIONS        1   // we only talk to one device at a time
// to allow for AES decryption and flash programming

#define AAT_VALUE   ((uint32_t)&__dfu_AAT)          // word value of AAT address
#define RESET_REQUEST   0x05FA0004      // value to request system reset

uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];
extern uint32_t __dfu_AAT;                          // our AAT address
unsigned char deKey[KEY_LEN];
uint8 currentConnection;

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
    uint8 response;

    writeStatus = &evt->data.evt_gatt_server_user_write_request;
    /*
    unsigned i;
    printf("Write value: attr=%d, opcode=%d, offset=%d, value:\n",
           writeStatus->characteristic, writeStatus->att_opcode, writeStatus->offset);
    for (i = 0; i != writeStatus->value.len; i++)
        printf("%02X ", writeStatus->value.data[i]);
    printf("\n");
        */
    switch (writeStatus->characteristic) {
        case GATTDB_ota_control:
            response = (uint8) (processCtrlPacket(writeStatus->value.data) ? 0 : 1);
            gecko_cmd_gatt_server_send_user_write_response(writeStatus->connection, writeStatus->characteristic,
                                                           response);
            break;

        case GATTDB_ota_data:
            // only respond if the command fails
            if (!processDataPacket(writeStatus->value.data, writeStatus->value.len))
                gecko_cmd_gatt_server_send_user_write_response(writeStatus->connection, writeStatus->characteristic, 1);
            break;

        default:
            printf("Bad write - handle %d\n", writeStatus->characteristic);
            gecko_cmd_gatt_server_send_user_write_response(writeStatus->connection, writeStatus->characteristic, 1);
            break;

    }

}

// this is the entry point for the firmware upgrader
void dfu_main(void) {
    //EMU_init();
    //CMU_init();

    printf("Started V2\n");
    if (USER_BLAT->type != APP_APP_ADDRESS_TYPE)
        enterDfu = true;

    if (!enterDfu) {
        SCB->VTOR = (uint32_t) USER_BLAT->vectorTable;
        //Use stack from aat
        asm("mov sp,%0"::"r" (USER_BLAT->topOfStack));
        USER_BLAT->resetVector();
    }
    CRYPTO_AES_DecryptKey256(CRYPTO, deKey, ota_key);
    gecko_init(&config);
    printf("Stack initialised\n");
    gecko_cmd_gatt_set_max_mtu(MAX_MTU);


    for (;;) {
        /* Event pointer for handling events */
        struct gecko_cmd_packet *evt;
        struct gecko_msg_le_connection_parameters_evt_t *pp;
        uint16 i;

        /* Check for stack event. */
        evt = gecko_wait_event();

        /* Handle events */
        unsigned id = BGLIB_MSG_ID(evt->header) & ~gecko_dev_type_gecko;
#if defined(DEBUG) && false
        if (id != (gecko_evt_hardware_soft_timer_id & ~gecko_dev_type_gecko)) {
        }
#endif
        switch (BGLIB_MSG_ID(evt->header)) {
            struct gecko_msg_gatt_server_characteristic_status_evt_t *status;
            struct gecko_msg_gatt_server_user_read_request_evt_t *readStatus;

            /* This boot event is generated when the system boots up after reset.
             * Here the system is set to start advertising immediately after boot procedure. */
            case gecko_evt_system_boot_id:
                printf("Bootloader: system_boot\n");

                /* Set advertising parameters. 100ms advertisement interval. All channels used.
             * The first two parameters are minimum and maximum advertising interval, both in
             * units of (milliseconds * 1.6). The third parameter '7' sets advertising on all channels. */
                gecko_cmd_le_gap_set_adv_parameters(100, 100, 7);

                /* Start general advertising and enable connections. */
                gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
                break;

            case gecko_evt_le_connection_opened_id:
                printf("Connection opened\n");
                gecko_cmd_gatt_set_max_mtu(MAX_MTU);
                currentConnection = evt->data.evt_le_connection_opened.connection;
                break;

            case gecko_evt_le_connection_closed_id:
                printf("Connection closed\n");
                if (doReset) {
                    printf("Resetting....\n");
                    SCB->AIRCR = RESET_REQUEST;
                } else
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

            case gecko_evt_le_connection_parameters_id:
                pp = &evt->data.evt_le_connection_parameters;
                printf("Connection parameters: interval %d, latency %d, timeout %d\n",
                       pp->interval, pp->latency, pp->timeout);
                break;

            case gecko_evt_gatt_mtu_exchanged_id:
                printf("MTU exchanged: %d\n", evt->data.evt_gatt_mtu_exchanged.mtu);
                break;

            case gecko_evt_endpoint_status_id:
                //printf("Endpoint %d status: flags %X, type %d\n",
                       //evt->data.evt_endpoint_status.endpoint, evt->data.evt_endpoint_status.flags, evt->data.evt_endpoint_status.type);
                break;

            default:
                if (id & gecko_msg_type_evt) {
                    id &= ~gecko_msg_type_evt;
                    printf("unhandled event = %X\n", id);
                } else if (id & gecko_msg_type_rsp) {
                    id &= ~gecko_msg_type_rsp;
                    printf("unhandled response = %X\n", id);
                } else
                    printf("undeciphered message = %X\n", id);
                break;
        }
    }
}