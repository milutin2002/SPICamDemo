#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"
#include "Lib/arducam.h"
#define CHUNK 1024
typedef struct msg_t{
    uint16_t size;
    uint8_t buffer[1024];

}msg_t;
queue_t q;

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
    uint8_t id_h,id_l,spiTestVal;
    arducam.systemInit();
    
    if(arducam.busDetect()==1){
        return 1;
    }
    if(arducam.cameraProbe()){
        while(true){
            printf("Camera not probed\n");
        }
        return 1;
    }
    arducam.cameraInit();
    arducam.setJpegSize(res_320x240);
    while(true){
        singleCapture();
    }
}
