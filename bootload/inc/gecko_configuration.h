#ifndef GECKO_CONFIGURATION
#define GECKO_CONFIGURATION

#include "bg_gattdb_def.h"

/**
 * USART configuration defined by application
 */
#define USART_FLAGS_ENABLED  1
#define USART_FLAGS_BGAPI    2
#define USART_FLAGS_NORTSCTS 4//Do not enable hardware flow control in uart
#define USART_FLAGS_NO_HW_INIT 8

#define DEFAULT_BLUETOOTH_HEAP(CONNECTIONS) (2976+(CONNECTIONS)*844)

typedef struct
{    
    uint32_t    baudrate;
    uint8_t     flags;
    uint8_t     rts_loc;
    uint8_t     cts_loc;
    uint8_t     rx_loc;
    uint8_t     tx_loc;
    uint8_t     parity;
    uint8_t     databits;
    uint8_t     stopbits;
}gecko_usart_config_t;

#define I2C_FLAGS_ENABLED  1
typedef struct
{    
    uint8_t     flags;
    uint8_t     scl_loc;
    uint8_t     sda_loc;
}gecko_i2c_config_t;

#define GPIO_FLAGS_ENABLED  1

typedef struct
{
    uint32_t    CTRL;
    uint32_t    MODEL;
    uint32_t    MODEH;
    uint32_t    DOUT;
}gecko_gpio_config_t;

typedef struct
{
    uint32_t    EXTIPSELL;
    uint32_t    EXTIPSELH;
    uint32_t    EXTIRISE;
    uint32_t    EXTIFALL;
    uint32_t    IEN;
}gecko_gpio_exti_config_t;

#define ADC_FLAGS_ENABLED  1
typedef struct
{
    uint8_t     flags;
}gecko_adc_config_t;

// The first two flags are used by both gecko_configuration_t .sleep
// and .host_wakeup. the third is only used by .sleep
#define SLEEP_FLAGS_ENABLE_WAKEUP_PIN  1
#define SLEEP_FLAGS_ACTIVE_HIGH        2
#define SLEEP_FLAGS_DEEP_SLEEP_ENABLE  4

typedef struct
{
    uint8_t     flags;
    uint8_t     port;
    uint8_t     pin;
}gecko_sleep_config_t;

typedef struct
{
    uint8_t max_connections;//!< Maximum number of connections to support, if 0 defaults to 4
    uint8_t max_advertisers;//!< Maximum number of advertisers to support, if 0 defaults to 1
    //heap configuration, if NULL uses default
    void    *heap;
    uint16_t heap_size;
    uint16_t sleep_clock_accuracy; // ppm, 0 = default (250 ppm)
}gecko_bluetooth_config_t;

#define RADIO_OBS_FLAGS_ENABLED  1
typedef struct
{
    uint8_t     flags;
    int8_t      rx_pin;
    int8_t      tx_pin;
}gecko_radio_obs_config_t;

#define GECKO_RADIO_PA_INPUT_VBAT   0
#define GECKO_RADIO_PA_INPUT_DCDC   1

typedef struct
{
    uint8_t config_enable; // Non-zero value indicates this PA config is valid.
    uint8_t input;  // VBAT or DCDC
}gecko_radio_pa_config_t;

typedef struct
{
    uint8_t mode; //mode selection for output
    uint8_t location; //location to output clock
}gecko_clock_output_t;


#define GECKO_OTA_FLAGS_AUTHENTICATED_WRITE 0x200
#define GECKO_OTA_FLAGS_ENCRYPTED_WRITE  0x100
#define GECKO_OTA_FLAGS_BONDED_WRITE     0x400

typedef struct
{
    uint32_t flags;
    uint8_t  device_name_len;
    char     *device_name_ptr;
}gecko_ota_config_t;


#define GECKO_CONFIG_FLAG_STACK_DISABLE                 2
#define GECKO_CONFIG_FLAG_LFXO_DISABLE                  4
#define GECKO_CONFIG_FLAG_BGBUILD_COMPATIBILITY         8
#define GECKO_CONFIG_FLAG_DCDC_MEASURE                 16

#define GECKO_CONFIG_FLAG_USE_LAST_CTUNE               64 //Use ctune value from last_ctune ps-key
#define GECKO_CONFIG_FLAG_SUPERVISOR                   128

typedef void (*gecko_priority_schedule_callback)(void);

typedef struct
{
    uint32_t config_flags;
    gecko_usart_config_t  usarts[3];
    gecko_i2c_config_t  i2cs[1];
    gecko_gpio_config_t gpios[6];
    gecko_gpio_exti_config_t gpio_exti;
    gecko_adc_config_t  adc;
    gecko_sleep_config_t  sleep;
    gecko_bluetooth_config_t bluetooth;
    gecko_sleep_config_t host_wakeup;
    gecko_radio_obs_config_t radio_obs;
    //

    const struct bg_gattdb_def *gattdb;
    //DCDC config, if NULL uses default values
    const void * dcdc;
    //HFXO config, if NULL uses default values
    const void * hfxo;
    //LFXO config, if NULL uses default values
    const void * lfxo;
    //PTI config, if NULL PTI is disabled
    const void * pti;
    //Callback for priority scheduling, used for RTOS support. If NULL uses pendsv irq.
    gecko_priority_schedule_callback scheduler_callback;

    gecko_radio_pa_config_t pa;

    gecko_clock_output_t clock_outputs[2];

    gecko_ota_config_t ota;

}gecko_configuration_t;


#endif
