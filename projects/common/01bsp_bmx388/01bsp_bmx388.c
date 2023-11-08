/**
\brief This program shows the use of the "bmx160" bsp module.

Since the bsp modules for different platforms have the same declaration, you
can use this project with any platform.

This project configure bmx160 and read the gyroscope values in 3 axises.
Then it sends out the gyro data through uart at interval of SAMPLE_PERIOD.

\author Tengfei Chang <tengfei.chang@gmail.com>, Nov 2021.
*/

#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "board.h"
#include "leds.h"
#include "sctimer.h"
#include "i2c.h"
#include "uart.h"
#include "bmx388.h"

//=========================== defines =========================================

#define SAMPLE_PERIOD   (32768>>6)  // @32kHz = 1s
#define BUFFER_SIZE     0x08   //2B*3 axises value + 2B ending with '\r\n'

//=========================== variables =======================================

typedef struct {
   uint16_t num_compare;
   bool sampling_now;
   uint8_t axes[6];
   float axis_x;
   float axis_y;
   float axis_z;

   uint8_t who_am_i;
   uint8_t sensorTime;
   int16_t temp;
   float   temp_f;

   // uart specific
              uint8_t uart_lastTxByteIndex;
   volatile   uint8_t uartDone;
   volatile   uint8_t uartSendNow;
   volatile   uint8_t uartToSend[BUFFER_SIZE];
    uint8_t   pwr_status;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void cb_compare(void);
void cb_uartTxDone(void);
uint8_t cb_uartRxCb(void);

//=========================== main ============================================

/**
\brief The program starts executing here.
*/
int mote_main(void) {

    int16_t tmp;
    uint8_t i;

    // initialize board. 
    board_init();

    // setup UART
    uart_setCallbacks(cb_uartTxDone,cb_uartRxCb);
    uart_enableInterrupts();
   
    sctimer_set_callback(cb_compare);
    sctimer_setCompare(sctimer_readCounter()+SAMPLE_PERIOD);
         
    // alway set address first
    i2c_set_addr(BMX388_ADDR);
    bmx160_power_on();
    app_vars.pwr_status = bmx160_get_pwr_status();

    // should be 0x50 for bmx388
    app_vars.who_am_i = bmx388_who_am_i();
        

    while (1) {

        // wait for timer to elapse
        while (app_vars.uartSendNow==0);
        app_vars.uartSendNow = 0;
        //bmx160_set_cmd(BMX160_CMD_PMU_GYR_NORMAL);

        bmx160_read_9dof_data();
        bmp388_get_sensortime();
        uint8_t stasus;
        stasus = bmx160_get_pmu_status();

        bmp388_get_compensation();
        bmp388_compensation_temp();
        app_vars.temp_f = bmx388_read_t_fine();


        //i=0;
        //tmp = bmx160_read_gyr_x();
        //app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
        //app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);
        
        //tmp = bmx160_read_gyr_y();
        //app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
        //app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);

        //tmp = bmx160_read_gyr_z();
        //app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
        //app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);

        //app_vars.uartToSend[i++] = '\r';
        //app_vars.uartToSend[i++] = '\n';

        // send string over UART
        app_vars.uartDone              = 0;
        app_vars.uart_lastTxByteIndex  = 0;
        //uart_writeByte(app_vars.uartToSend[app_vars.uart_lastTxByteIndex]);
        //uart_writeFloatByte(app_vars.temp_f);
        //while(app_vars.uartDone==0);
        //app_vars.sensorTime = bmp388_get_sensortime();
    }
}

//=========================== callbacks =======================================

void cb_compare(void) {
   
    // have main "task" send over UART
    app_vars.uartSendNow = 1;

    // schedule again
    sctimer_setCompare(sctimer_readCounter()+SAMPLE_PERIOD);
}

void cb_uartTxDone(void) {

    app_vars.uart_lastTxByteIndex++;
    if (app_vars.uart_lastTxByteIndex<sizeof(app_vars.uartToSend)) {
        uart_writeByte(app_vars.uartToSend[app_vars.uart_lastTxByteIndex]);
    } else {
        app_vars.uartDone = 1;
    }
}

uint8_t cb_uartRxCb(void) {
   
   uint8_t byte;
   
   // toggle LED
   leds_error_toggle();
   
   // read received byte
   byte = uart_readByte();
   
   // echo that byte over serial
   uart_writeByte(byte);
   
   return 0;
}