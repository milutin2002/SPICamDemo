#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"

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
static uint8_t ardu_read(uint8_t reg){
    cs_low();
    uint8_t tx[2]={(uint8_t)reg&0x7Fu,0x00},rx[2];
    spi_write_read_blocking(SPI_PORT,tx,rx,2);
    cs_high();
    return rx[1];
}
static void ardu_write(uint8_t reg,uint8_t val){
    cs_low();
    uint8_t tx[2]={(uint8_t)reg|0x80u,val},rx[2];
    spi_write_read_blocking(SPI_PORT,tx,rx,2);
    cs_high();
}

static uint32_t fifo_len(){
    uint32_t L=0;
    L|=(uint32_t)ardu_read(ARDUCHIP_FIFO_SIZE1);
    L|=(uint32_t)ardu_read(ARDUCHIP_FIFO_SIZE2)<<16;
    L|=(uint32_t)ardu_read(ARDUCHIP_FIFO_SIZE3)<<24;
    return L;
}

static void fifo_reset(){
    ardu_write(ARDUCHIP_FIFO,FIFO_RDPTR_RST_MASK | FIFO_WRPTR_RST_MASK);
}

static void start_cap(){
    ardu_write(ARDUCHIP_TRIG,START_CAP);
}

static void fifo_burst_read_blocking(uint8_t *buf,size_t len){
    cs_low();
    uint8_t cmd=(uint8_t)(BURST_FIFO_READ | 0x7F);
    spi_write_blocking(SPI_PORT,&cmd,1);
    spi_read_blocking(SPI_PORT,0x00,buf,len);
    cs_high();
}

static bool s_w(uint8_t r, uint8_t v){
    uint8_t b[2]={r,v}; return i2c_write_blocking(I2C_PORT, OV2640_ADDR, b, 2, false) >= 0;
}
static void ov2640_init_qvga_jpeg(){
    s_w(0xFF,0x01); s_w(0x12,0x80); sleep_ms(100); 
    s_w(0xFF,0x00); s_w(0x2C,0xFF); s_w(0x2E,0xDF);
    s_w(0xFF,0x01); s_w(0x15,0x00); s_w(0x12,0x40); 
    s_w(0xFF,0x00); s_w(0xE0,0x04);
    s_w(0xC0,0x64); s_w(0xC1,0x4B); s_w(0x86,0x3D); s_w(0x50,0x00);
    s_w(0x51,0xC8); s_w(0x52,0x96); s_w(0x53,0x00); s_w(0x54,0x00);
    s_w(0x55,0x00); s_w(0x57,0x00); s_w(0x5A,0xC8); s_w(0x5B,0x96); s_w(0x5C,0x00);
    s_w(0xE0,0x00); s_w(0xFF,0x01); s_w(0x11,0x01); s_w(0x0C,0x00);
}

static inline void send_via_uart(uint8_t *b,int n){
    for (size_t i = 0; i < n; i++)
    {
        uart_putc_raw(UART_ID,b[i]);
    }
}

static inline void init_spi(){
    spi_init(SPI_PORT,8*1000*1000);
    gpio_set_function(PIN_SCK,GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO,GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI,GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS,GPIO_OUT);

}

static inline void init_i2c(){
    i2c_init(I2C_PORT,100*1000);

    gpio_set_function(PIN_SDA,GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL,GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);
}

static inline void init_uart(){
    uart_init(UART_ID,UART_BAUD);
    gpio_set_function(UART_TX,GPIO_FUNC_UART);
}
static void sendAll(msg_t b){
    for(int i=0;i<b.size;i++){
        uart_putc_raw(UART_ID,b.buffer[i]);
    }
}
static void core1Send(){
    msg_t m;
    printf("Core 1 activate");
    while(true){
        queue_remove_blocking(&q,&m);
        printf("Sending buffer");
        sendAll(m);
    }
}
int main()
{
    stdio_init_all();
    init_spi();
    init_i2c();
    init_uart();
    ardu_write(ARDUCHIP_TEST1,0x55);
    uint8_t t=ardu_read(ARDUCHIP_TEST1);
    if(t!=0x55){
        printf("The value is %d\n",t);
        while(true){
            printf("Not able to connect spi\n");
        }
    }
    ov2640_init_qvga_jpeg();
    
    queue_init(&q,sizeof(msg_t),4);
    multicore_launch_core1(core1Send);

    fifo_reset();
    start_cap();

    while(!(ardu_read(ARDUCHIP_TRIG) & CAP_DONE_MASK)){
        tight_loop_contents();
    }

    uint32_t len=fifo_len();
    msg_t m;
    if(len && len<8*1024u*1024u){
        m.size=8;
        m.buffer[0]='F';
        m.buffer[1]='R';
        m.buffer[2]='A';
        m.buffer[3]='M';
        m.buffer[4]=(uint8_t)(0xFF&len);
        m.buffer[5]=(uint8_t)(0xFF&(len>>8));
        m.buffer[6]=(uint8_t)(0xFF&(len>>16));
        m.buffer[7]=(uint8_t)(0xFF&(len>>24));
        queue_add_blocking(&q,&m);
        printf("Adding message");
        while(len){
            uint32_t take=len>BUFSIZ ? BUFSIZ:len;
            m.size=take;
            fifo_burst_read_blocking(m.buffer,take);
            queue_add_blocking(&q,&m);
            len-=take;
        }
    }
    while(true){
    }
}
