/*
 * Copyright (c) 2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 * Copyright (c) 2025 Jakub Kramarz <jakub@hackerspace-krk.pl>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>
#include <osdp.hpp>

OSDP::PeripheralDevice pd;

int serial1_send_func(void *data, uint8_t *buf, int len);
int serial1_recv_func(void *data, uint8_t *buf, int len);
int pd_command_handler(void *data, struct osdp_cmd *cmd);

int serial1_send_func(void *data, uint8_t *buf, int len)
{
    (void)(data);

    int sent = 0;
    for (int i = 0; i < len; i++) {
        if (SERIAL_PORT_HARDWARE.write(buf[i])) {
            sent++;
        } else {
            break;
        }
    }

    return sent;
}

int serial1_recv_func(void *data, uint8_t *buf, int len)
{
    (void)(data);

    int read = 0;
    while (SERIAL_PORT_HARDWARE.available() && read < len) {
        buf[read] = SERIAL_PORT_HARDWARE.read();
        read++;
    }
    return read;
}

osdp_pd_info_t info_pd = {
    .name = "pd[101]",
    .baud_rate = 115200,
    .address = 101,
    .flags = 0,
    .id = {
        .version = 1,
        .model = 153,
        .vendor_code = 31337,
        .serial_number = 0x01020304,
        .firmware_version = 0x0A0B0C0D,
    },
    .cap = (struct osdp_pd_cap []) {
        {
            .function_code = OSDP_PD_CAP_READER_LED_CONTROL,
            .compliance_level = 1,
            .num_items = 1
        },
        {
            .function_code = OSDP_PD_CAP_READER_AUDIBLE_OUTPUT,
            .compliance_level = 1,
            .num_items = 1
        },
        { static_cast<uint8_t>(-1), 0, 0 } /* Sentinel */
    },
    .channel = {
        .data = nullptr,
        .id = 0,
        .recv = serial1_recv_func,
        .send = serial1_send_func,
        .flush = nullptr
    },
    .scbk = nullptr,
};

int pd_command_handler(void *data, struct osdp_cmd *cmd)
{
    (void)(data);

    Serial.println("Received a command!");
    return 0;
}

int logger_print(const char *msg)
{
    Serial.println(msg);
    return 0;
}

void setup()
{
    delay(1000);
    Serial.begin(115200);

    Serial.println("OSDP Peripheral Device");

    // disable Wiegand interface
    pinMode(0, OUTPUT);
    pinMode(1, OUTPUT);
    digitalWrite(0, HIGH);
    digitalWrite(1, HIGH);

    // configure RS485 transceiver
    pinMode(PIN_OSDP_DE, OUTPUT); 
    digitalWrite(PIN_OSDP_DE, LOW);

    pinMode(PIN_OSDP_RE, OUTPUT);
    digitalWrite(PIN_OSDP_RE, LOW);

    pinMode(PIN_OSDP_TX, OUTPUT);
    digitalWrite(PIN_OSDP_TX, LOW);

    pinMode(PIN_OSDP_RX, INPUT);
    digitalWrite(PIN_OSDP_RX, LOW);
    
    pinMode(PIN_OSDP_TERM, OUTPUT);
    digitalWrite(PIN_OSDP_TERM, HIGH);

    // This configuration at the first glance is a bit counterintuitive:
    // - receiver is permanently enabled, as:
    //    - it is used for collision detection
    //    - echo is suppresed by UART hardware
    // - hardware flow in the UART controller is DISABLED, as it doesn't support half-duplex communication
    // - RTS signal is controlled the software UART driver
    // - CTS signal is not used
    // https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32c3/api-reference/peripherals/uart.html#overview-of-rs485-specific-communication-options
    
    SERIAL_PORT_HARDWARE.begin(
        115200,
        SERIAL_8N1,
        PIN_OSDP_RX,
        PIN_OSDP_TX
    );
    SERIAL_PORT_HARDWARE.setMode(UART_MODE_RS485_HALF_DUPLEX);
    SERIAL_PORT_HARDWARE.setPins(PIN_OSDP_RX, PIN_OSDP_TX, -1, PIN_OSDP_DE);
    SERIAL_PORT_HARDWARE.setHwFlowCtrlMode(UART_HW_FLOWCTRL_DISABLE, 122);

    Serial.println("Starting OSDP Peripheral Device");


    pd.logger_init("osdp::pd", OSDP_LOG_DEBUG, logger_print);

    pd.setup(&info_pd);

    pd.set_command_callback(pd_command_handler, nullptr);

    Serial.println("OSDP Peripheral Device started");
}

void loop()
{
    pd.refresh();
}