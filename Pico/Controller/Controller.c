#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

// ---- ArduCAM mini FIFO regs ----
#define ARDUCHIP_TEST1      0x00
#define ARDUCHIP_FIFO       0x04
#define ARDUCHIP_TRIG       0x41
#define ARDUCHIP_FIFO_SIZE1 0x42
#define ARDUCHIP_FIFO_SIZE2 0x43
#define ARDUCHIP_FIFO_SIZE3 0x44
#define FIFO_RDPTR_RST_MASK 0x10
#define FIFO_WRPTR_RST_MASK 0x20
#define CAP_DONE_MASK       0x08
#define START_CAP           0x02
#define BURST_FIFO_READ     0x3C

#define SPI_PORT spi0
#define PIN_SCK 2
#define PIN_MOSI 3
#define PIN_MISO 4
#define PIN_CS 5

#define I2C_PORT i2c0
#define PIN_SDA 8
#define PIN_SCL 9

#define UART_ID     uart0
#define UART_TX     0
#define UART_BAUD   115200

#define OV2640_ADDR 0x30

#define CHUNK 1024
typedef struct msg_t{
    uint16_t size;
    uint8_t buffer[1024];

}msg_t;
queue_t q;

static inline void cs_low(){
    gpio_put(PIN_CS,0);
}
static void cs_high(){
    gpio_put(PIN_CS,1);
}


int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
