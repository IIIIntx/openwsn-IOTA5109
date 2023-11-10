/**
\brief This program shows the use of the "radio" bsp module.

Since the bsp modules for different platforms have the same declaration, you
can use this project with any platform.

After loading this program, your board will switch on its radio on frequency
CHANNEL.

While receiving a packet (i.e. from the start of frame event to the end of
frame event), it will turn on its sync LED.

Every TIMER_PERIOD, it will also send a packet containing LENGTH_PACKET bytes
set to ID. While sending a packet (i.e. from the start of frame event to the
end of frame event), it will turn on its error LED.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2014.
*/

#include "board.h"
#include "radio.h"
#include "leds.h"
#include "sctimer.h"
#include "uart.h"
#include "i2c.h"
#include "stdint.h"
#include "string.h"
#include "bmx160.h"
#include "stdio.h"
#include "math.h"

//=========================== defines =========================================

#define LENGTH_PACKET   125+LENGTH_CRC  ///< maximum length is 127 bytes
#define LEN_PKT_TO_SEND 20+LENGTH_CRC
#define CHANNEL         11             ///< 11=2.405GHz
#define TIMER_PERIOD    (0xffff>>4)    ///< 0xffff = 2s@32kHz
#define ID              0x55           ///< byte sent in the packets
#define track_flag      0x01           ///< 0x01 for host 0x00 for slave
#define BUFFER_SIZE     0x08   //2B*3 axises value + 2B ending with '\r\n'

uint8_t stringToSend[]  = "+002 Ptest.24\n";

//=========================== variables =======================================

enum {
    APP_FLAG_START_FRAME = 0x01,
    APP_FLAG_END_FRAME   = 0x02,
    APP_FLAG_TIMER       = 0x04,
};

typedef enum {
    APP_STATE_TX         = 0x01,
    APP_STATE_RX         = 0x02,
} app_state_t;

typedef struct {
    uint8_t              num_startFrame;
    uint8_t              num_endFrame;
    uint8_t              num_timer;
    
    uint8_t              num_rx_startFrame;
    uint8_t              num_rx_endFrame;
} app_dbg_t;

app_dbg_t app_dbg;

typedef struct {
                uint8_t         uart_lastTxByteIndex;
    volatile    uint8_t         uartDone;
    volatile    uint8_t         uartSendNow;
    volatile   uint8_t uartToSend[BUFFER_SIZE];

                uint8_t         flags;
                app_state_t     state;
                uint8_t         packet[LENGTH_PACKET];
                uint8_t         packet_len;
                int8_t          rxpk_rssi;
                uint8_t         rxpk_lqi;
                bool            rxpk_crc;

                uint8_t who_am_i;
                float   temp_f;
                char    temp_data[6];

                uint16_t num_compare;
                bool sampling_now;
                uint8_t axes[6];
                float axis_x;
                float axis_y;
                float axis_z;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_startFrame(PORT_TIMER_WIDTH timestamp);
void     cb_endFrame(PORT_TIMER_WIDTH timestamp);
void     cb_timer(void);

void     cb_uart_tx_done(void);
uint8_t  cb_uart_rx(void);

//=========================== main ============================================

/**
\brief The program starts executing here.
*/
int mote_main(void) {
    uint8_t i;
    uint8_t j;

    uint8_t freq_offset;
    uint8_t sign;
    uint8_t read;

    // clear local variables
    memset(&app_vars,0,sizeof(app_vars_t));

    // initialize board
    board_init();

    // setup UART
    uart_setCallbacks(cb_uart_tx_done,cb_uart_rx);
    uart_enableInterrupts();

    app_vars.uartDone = 1;

    //// set bmx388
    //if(track_flag){
    //  // alway set address first
    //  i2c_set_addr(BMX388_ADDR);
    //  bmx160_power_on();

    //  // should be 0x50 for bmx388
    //  app_vars.who_am_i = bmx388_who_am_i();
    //}

    // set bmx160
    // alway set address first
    i2c_set_addr(BMX160_ADDR);

    // should be 0x50 for bmx388
    app_vars.who_am_i = bmx160_who_am_i();


    // add callback functions radio
    radio_setStartFrameCb(cb_startFrame);
    radio_setEndFrameCb(cb_endFrame);

    // prepare packet
    app_vars.packet_len = sizeof(app_vars.packet);
    for (i=0;i<app_vars.packet_len;i++) {
        app_vars.packet[i] = ID;
    }

    // start bsp timer
    sctimer_set_callback(cb_timer);
    sctimer_setCompare(sctimer_readCounter()+TIMER_PERIOD);
    sctimer_enable();

    // prepare radio
    radio_rfOn();
    // freq type only effects on scum port
    radio_setFrequency(CHANNEL, FREQ_RX);

    // switch in RX by default
    radio_rxEnable();
    app_vars.state = APP_STATE_RX;

    // start by a transmit
    app_vars.flags |= APP_FLAG_TIMER;

    while (1) {

        // sleep while waiting for at least one of the flags to be set
        while (app_vars.flags==0x00) {
            board_sleep();
        }

        //if(track_flag){
        //  //read bmx388 data
        //  bmx160_read_9dof_data();
        //  bmp388_get_compensation();
        //  bmp388_compensation_temp();
        //  app_vars.temp_f = bmx388_read_t_fine();
        //  float_to_char(app_vars.temp_f,app_vars.temp_data,4);
        //}
        //if(track_flag){
          
        //  //int16_t tmp;
        //  //i=0;
        //  //tmp = bmx160_read_gyr_x();
        //  //app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
        //  //app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);
        
        //  //tmp = bmx160_read_gyr_y();
        //  //app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
        //  //app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);

        //  //tmp = bmx160_read_gyr_z();
        //  //app_vars.uartToSend[i++] = (uint8_t)((tmp>>8) & 0x00ff);
        //  //app_vars.uartToSend[i++] = (uint8_t)((tmp>>0) & 0x00ff);

        //  //app_vars.uartToSend[i++] = '\r';
        //  //app_vars.uartToSend[i++] = '\n';
        //  }



        // handle and clear every flag
        while (app_vars.flags) {


            //==== APP_FLAG_START_FRAME (TX or RX)

            if (app_vars.flags & APP_FLAG_START_FRAME) {
                // start of frame

                switch (app_vars.state) {
                    case APP_STATE_RX:
                        // started receiving a packet

                        // led
                        leds_error_on();
                        break;
                    case APP_STATE_TX:
                        // started sending a packet

                        // led
                        leds_sync_on();
                    break;
                }

                // clear flag
                app_vars.flags &= ~APP_FLAG_START_FRAME;
            }

            //==== APP_FLAG_END_FRAME (TX or RX)

            if (app_vars.flags & APP_FLAG_END_FRAME) {
                // end of frame

                switch (app_vars.state) {

                    case APP_STATE_RX:

                        // done receiving a packet
                        app_vars.packet_len = sizeof(app_vars.packet);

                        // get packet from radio
                        radio_getReceivedFrame(
                            app_vars.packet,
                            &app_vars.packet_len,
                            sizeof(app_vars.packet),
                            &app_vars.rxpk_rssi,
                            &app_vars.rxpk_lqi,
                            &app_vars.rxpk_crc
                        );

                        freq_offset = radio_getFrequencyOffset();
                        sign = (freq_offset & 0x80) >> 7;
                        if (sign){
                            read = 0xff - (uint8_t)(freq_offset) + 1;
                        } else {
                            read = freq_offset;
                        }

                        //i = 0;
                        //if (sign) {
                        //    stringToSend[i++] = '-';
                        //} else {
                        //    stringToSend[i++] = '+';
                        //}
                        //stringToSend[i++] = '0'+read/100;
                        //stringToSend[i++] = '0'+read/10;
                        //stringToSend[i++] = '0'+read%10;
                        //stringToSend[i++] = ' ';

                        //stringToSend[i++] = 'P';
                        ////memcpy(&stringToSend[i],&app_vars.packet[0],14);
                        ////i += 14;

                        //memcpy(&stringToSend[i],&app_vars.packet[5],6);
                        //i += 6;
                        //stringToSend[i++] = ' ';

                        //stringToSend[i++] = '0'+j/100;
                        //stringToSend[i++] = '0'+j%100-j%10;
                        //stringToSend[i++] = '0'+j%10;

                        //j ++;
                        //if(j>=255)  { j = 0;}
                        

                        //sign = (app_vars.rxpk_rssi & 0x80) >> 7;
                        //if (sign){
                        //    read = 0xff - (uint8_t)(app_vars.rxpk_rssi) + 1;
                        //} else {
                        //    read = app_vars.rxpk_rssi;
                        //}

                        //if (sign) {
                        //    stringToSend[i++] = '-';
                        //} else {
                        //    stringToSend[i++] = '+';
                        //}
                        //stringToSend[i++] = '0'+read/100;
                        //stringToSend[i++] = '0'+read/10;
                        //stringToSend[i++] = '0'+read%10;

                        //// send ssri
                        //stringToSend[i++] = '0'+((uint8_t)LEN_PKT_TO_SEND)/10;
                        //stringToSend[i++] = '0'+((uint8_t)LEN_PKT_TO_SEND)%10;
                        //stringToSend[i++] = ' ';
                        //stringToSend[i++] = '0'+j/100;
                        //stringToSend[i++] = '0'+(j%100-j%10)/10;
                        //stringToSend[i++] = '0'+j%10;
                        //stringToSend[i++] = ' ';
                        //stringToSend[i++] = '0'+app_vars.rxpk_crc%10;
                        //stringToSend[i++] = ' ';

                        //sign = (app_vars.rxpk_rssi & 0x80) >> 7;
                        //if (sign){
                        //    read = 0xff - (uint8_t)(app_vars.rxpk_rssi) + 1;
                        //} else {
                        //    read = app_vars.rxpk_rssi;
                        //}

                        //if (sign) {
                        //    stringToSend[i++] = '-';
                        //} else {
                        //    stringToSend[i++] = '+';
                        //}
                        //stringToSend[i++] = '0'+read/100;
                        //stringToSend[i++] = '0'+read/10;
                        //stringToSend[i++] = '0'+read%10;

                        i = 0;
                        if(track_flag==0x00){
                        memcpy(&stringToSend[i],&app_vars.packet[5],8);
                        }

                        stringToSend[sizeof(stringToSend)-2] = '\r';
                        stringToSend[sizeof(stringToSend)-1] = '\n';

                        //j ++;
                        //if(j>=255)  { j = 0;}

                        // send string over UART
                        if (app_vars.uartDone == 1) {
                            app_vars.uartDone              = 0;
                            app_vars.uart_lastTxByteIndex  = 0;
                            uart_writeByte(stringToSend[app_vars.uart_lastTxByteIndex]);
                        }

                        // led
                        leds_error_off();
                        break;
                    case APP_STATE_TX:
                        // done sending a packet

                        // switch to RX mode
                        radio_rxEnable();
                        radio_rxNow();
                        app_vars.state = APP_STATE_RX;

                        // led
                        leds_sync_off();
                        break;
                }
                // clear flag
                app_vars.flags &= ~APP_FLAG_END_FRAME;
            }

            //==== APP_FLAG_TIMER

            if (app_vars.flags & APP_FLAG_TIMER) {
                // timer fired

                if (app_vars.state==APP_STATE_RX) {
                    // stop listening
                    radio_rfOff();

                    // prepare packet
                    app_vars.packet_len = sizeof(app_vars.packet);
                    i = 0;
                    app_vars.packet[i++] = 't';
                    app_vars.packet[i++] = 'e';
                    app_vars.packet[i++] = 's';
                    app_vars.packet[i++] = 't';
                    app_vars.packet[i++] = CHANNEL;
                    if(track_flag){
                      uint8_t n = 0;
                      int16_t tmp;
                      bmx160_read_9dof_data();
                      tmp = bmx160_read_gyr_x();
                      app_vars.uartToSend[n++] = (uint8_t)((tmp>>8) & 0x00ff);
                      app_vars.uartToSend[n++] = (uint8_t)((tmp>>0) & 0x00ff);
        
                      tmp = bmx160_read_gyr_y();
                      app_vars.uartToSend[n++] = (uint8_t)((tmp>>8) & 0x00ff);
                      app_vars.uartToSend[n++] = (uint8_t)((tmp>>0) & 0x00ff);

                      tmp = bmx160_read_gyr_z();
                      app_vars.uartToSend[n++] = (uint8_t)((tmp>>8) & 0x00ff);
                      app_vars.uartToSend[n++] = (uint8_t)((tmp>>0) & 0x00ff);
                      memcpy(&app_vars.packet[i],&app_vars.uartToSend[0],8);
                      i=i+8;
                    }

                    while (i<app_vars.packet_len) {
                        app_vars.packet[i++] = ID;
                    }

                    // start transmitting packet
                    radio_loadPacket(app_vars.packet,LEN_PKT_TO_SEND);
                    radio_txEnable();
                    radio_txNow();

                    app_vars.state = APP_STATE_TX;
                }

                // clear flag
                app_vars.flags &= ~APP_FLAG_TIMER;
            }
        }
    }
}

//=========================== callbacks =======================================

void cb_startFrame(PORT_TIMER_WIDTH timestamp) {
    // set flag
    app_vars.flags |= APP_FLAG_START_FRAME;

    // update debug stats
    app_dbg.num_startFrame++;

    if (app_vars.state == APP_STATE_RX) {
        app_dbg.num_rx_startFrame++;
    }
}

void cb_endFrame(PORT_TIMER_WIDTH timestamp) {
    // set flag
    app_vars.flags |= APP_FLAG_END_FRAME;

    // update debug stats
    app_dbg.num_endFrame++;

    if (app_vars.state == APP_STATE_RX) {
        app_dbg.num_rx_endFrame++;
    }
}

void cb_timer(void) {
    // set flag
    app_vars.flags |= APP_FLAG_TIMER;

    // update debug stats
    app_dbg.num_timer++;

    sctimer_setCompare(sctimer_readCounter()+TIMER_PERIOD);
}

void cb_uart_tx_done(void) {
    app_vars.uart_lastTxByteIndex++;
    if (app_vars.uart_lastTxByteIndex<sizeof(stringToSend)) {
        uart_writeByte(stringToSend[app_vars.uart_lastTxByteIndex]);
    } else {
        app_vars.uartDone = 1;
    }
}

uint8_t cb_uart_rx(void) {
    uint8_t byte;

    // toggle LED
    leds_error_toggle();

    // read received byte
    byte = uart_readByte();

    // echo that byte over serial
    uart_writeByte(byte);

    return 0;
}
