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

\author Tengfei Chang <tengfei.chang@inria.fr>, August 2020.
*/

#include "board.h"
#include "radio.h"
#include "leds.h"
#include "sctimer.h"
#include "radio_ble.h"
#include "uart.h"

//=========================== defines =========================================

#define LENGTH_PACKET   125+LENGTH_CRC  ///< maximum length is 127 bytes
#define CHANNEL         0              ///< 0~39
#define TIMER_PERIOD    (0xffff>>2)     ///< 0xffff = 2s@32kHz
#define TXPOWER         0xD5            ///< 2's complement format, 0xD8 = -40dbm
#define LENGTH_SERIAL_FRAME  127              // length of the serial frame


#define LEN_PKT_TO_SEND 20+LENGTH_CRC
char stringToSend[]  = "YYMYYMYYM\n";
uint16_t length = 0;

const static uint8_t ble_device_addr[6] = { 
    0xaa, 0xbb, 0xcc, 0xcc, 0xbb, 0xaa
};

// get from https://openuuid.net/signin/:  a24e7112-a03f-4623-bb56-ae67bd653c73
const static uint8_t ble_uuid[16]       = {

    0xa2, 0x4e, 0x71, 0x12, 0xa0, 0x3f, 
    0x46, 0x23, 0xbb, 0x56, 0xae, 0x67,
    0xbd, 0x65, 0x3c, 0x73
};

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
} app_dbg_t;

app_dbg_t app_dbg;

typedef struct {
     // uart
                uint8_t    uart_txFrame[LENGTH_SERIAL_FRAME];
                uint8_t    uart_lastTxByte;
    volatile    uint8_t    uart_done;

                uint8_t         flags;
                app_state_t     state;
                uint8_t         packet[LENGTH_PACKET];
                uint8_t         packet_len;
                int8_t          rxpk_rssi;
                uint8_t         rxpk_lqi;
                bool            rxpk_crc;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_startFrame(PORT_TIMER_WIDTH timestamp);
void     cb_endFrame(PORT_TIMER_WIDTH timestamp);
void     cb_timer(void);

// uart
void cb_uartTxDone(void);
uint8_t cb_uartRxCb(void);
void send_string(const char* str);


void     assemble_ibeacon_packet(void);
void     assemble_test_packet();

//=========================== main ============================================

/**
\brief The program starts executing here.
*/
int mote_main(void) {
    uint8_t i,j,m;

    uint8_t freq_offset;
    uint8_t sign;
    uint8_t read;

    // clear local variables
    memset(&app_vars,0,sizeof(app_vars_t));

    // initialize board
    board_init();
    radio_ble_init();

    // add callback functions radio
    radio_setStartFrameCb(cb_startFrame);
    radio_setEndFrameCb(cb_endFrame);

    // prepare packet
    app_vars.packet_len = sizeof(app_vars.packet);

    // setup UART
    uart_setCallbacks(cb_uartTxDone,cb_uartRxCb);
    uart_enableInterrupts();
    length = strlen("uart is ok!");
    send_string("uart is ok!");


    // start bsp timer
    sctimer_set_callback(cb_timer);
    sctimer_setCompare(sctimer_readCounter()+TIMER_PERIOD);
    sctimer_enable();

    // prepare radio
    radio_rfOn();
    // freq type only effects on scum port
    radio_ble_setFrequency(CHANNEL);

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
                        radio_ble_getReceivedFrame(
                            app_vars.packet,
                            &app_vars.packet_len,
                            sizeof(app_vars.packet),
                            &app_vars.rxpk_rssi,
                            &app_vars.rxpk_lqi,
                            &app_vars.rxpk_crc
                        );

                        if (app_vars.packet[0] == 0xAA) {
                            leds_debug_toggle();
                        }

                        if (app_vars.packet[2] == 0xCC
                        ) {
                            leds_debug_toggle();
                            //send_string("PACK1: ");
                            m = 0;
                            j = 5;
                            stringToSend[m++] = '0' + app_vars.packet[j] / 10;
                            stringToSend[m++] = '0' + app_vars.packet[j++] % 10;
                            stringToSend[m++] = '.';
                            stringToSend[m++] = '0' + app_vars.packet[j] / 10;
                            stringToSend[m++] = '0' + app_vars.packet[j++] % 10;
                            stringToSend[m++] = '.';
                            stringToSend[m++] = '0' + app_vars.packet[j] / 10;
                            stringToSend[m++] = '0' + app_vars.packet[j++] % 10;
                            stringToSend[m++] = '\r';
                            stringToSend[m++] = '\n';
                            send_string(stringToSend);
                        }
                        if (app_vars.packet_len <10) {
                            leds_debug_toggle();
                            m = 0;
                            //stringToSend[m++] = 'P';
                            //stringToSend[m++] = 'A';
                            //stringToSend[m++] = 'C';
                            //stringToSend[m++] = 'K';
                            //stringToSend[m++] = ':';
                            length = 5;
                            send_string("PACK: ");
                            //for(j = 0; j < app_vars.packet_len; ++j){
                            //  //stringToSend[m++] = app_vars.packet[j];
                            j = 2;
                            stringToSend[m++] = '0' + app_vars.packet[j] / 10;
                            stringToSend[m++] = '0' + app_vars.packet[j++] % 10;
                            stringToSend[m++] = '.';
                            stringToSend[m++] = '0' + app_vars.packet[j] / 10;
                            stringToSend[m++] = '0' + app_vars.packet[j++] % 10;
                            stringToSend[m++] = '.';
                            stringToSend[m++] = '0' + app_vars.packet[j] / 10;
                            stringToSend[m++] = '0' + app_vars.packet[j++] % 10;
                            stringToSend[m++] = '\r';
                            stringToSend[m++] = '\n';
                              //send_string(app_vars.packet[j]);
                            //}
                            length = 10;
                            send_string(stringToSend);
                            //stringToSend[m++] = '\r';
                            //stringToSend[m++] = '\n';
                            //send_string("\r\n");
                        }

                        //// send string over UART
                        //if (app_vars.uartDone == 1) {
                        //    app_vars.uartDone              = 0;
                        //    app_vars.uart_lastTxByteIndex  = 0;
                        //    uart_writeByte(stringToSend[app_vars.uart_lastTxByteIndex]);
                        //}


                        // led
                        leds_error_off();

                        // continue to listen
                        radio_rxNow();
                        break;
                    case APP_STATE_TX:
                        // done sending a packet

                        memset( app_vars.packet, 0x00, sizeof(app_vars.packet) );

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
                    
                    assemble_ibeacon_packet();
                    //assemble_test_packet();


                    // start transmitting packet
                    radio_ble_loadPacket(app_vars.packet,LENGTH_PACKET);
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
//=========================== private =========================================

void assemble_ibeacon_packet(void) {

    uint8_t i;
    i=0;

    memset( app_vars.packet, 0x00, sizeof(app_vars.packet) );

    app_vars.packet[i++]  = 0x42;               // BLE ADV_NONCONN_IND (this is a must)
    app_vars.packet[i++]  = 0x21;               // Payload length
    app_vars.packet[i++]  = ble_device_addr[0]; // BLE adv address byte 0
    app_vars.packet[i++]  = ble_device_addr[1]; // BLE adv address byte 1
    app_vars.packet[i++]  = ble_device_addr[2]; // BLE adv address byte 2
    app_vars.packet[i++]  = ble_device_addr[3]; // BLE adv address byte 3
    app_vars.packet[i++]  = ble_device_addr[4]; // BLE adv address byte 4
    app_vars.packet[i++]  = ble_device_addr[5]; // BLE adv address byte 5

    app_vars.packet[i++]  = 0x1a;
    app_vars.packet[i++]  = 0xff;
    app_vars.packet[i++]  = 0x4c;
    app_vars.packet[i++]  = 0x00;

    app_vars.packet[i++]  = 0x02;
    app_vars.packet[i++]  = 0x15;
    memcpy(&app_vars.packet[i], &ble_uuid[0], 16);
    i                    += 16;
    app_vars.packet[i++]  = 0x00;               // major
    app_vars.packet[i++]  = 0xff;
    app_vars.packet[i++]  = 0x00;               // minor
    app_vars.packet[i++]  = 0x0f;
    app_vars.packet[i++]  = TXPOWER;            // power level
}

void assemble_test_packet(void) {

    uint8_t i;
    i=0;

    memset( app_vars.packet, 0x00, sizeof(app_vars.packet) );

    app_vars.packet[i++]  = 0x20;               // BLE ADV_NONCONN_IND (this is a must)
    app_vars.packet[i++]  = 0x03;               // Payload length
    app_vars.packet[i++]  = 24; // BLE adv address byte 0
    app_vars.packet[i++]  = 24; // BLE adv address byte 1
    app_vars.packet[i++]  = 24; // BLE adv address byte 2
}

//=========================== callbacks =======================================

void cb_startFrame(PORT_TIMER_WIDTH timestamp) {
    // set flag
    app_vars.flags |= APP_FLAG_START_FRAME;

    // update debug stats
    app_dbg.num_startFrame++;
}

void cb_endFrame(PORT_TIMER_WIDTH timestamp) {
    // set flag
    app_vars.flags |= APP_FLAG_END_FRAME;

    // update debug stats
    app_dbg.num_endFrame++;
}

void cb_timer(void) {
    // set flag
    app_vars.flags |= APP_FLAG_TIMER;

    // update debug stats
    app_dbg.num_timer++;

    sctimer_setCompare(sctimer_readCounter()+TIMER_PERIOD);
}

//===== uart

void cb_uartTxDone(void) {

    uart_clearTxInterrupts();

    // prepare to send the next byte
    app_vars.uart_lastTxByte++;

    if (app_vars.uart_lastTxByte<length) {
        uart_writeByte(app_vars.uart_txFrame[app_vars.uart_lastTxByte]);
    } else {
        app_vars.uart_done=1;
    }
}

//void cb_uartTxDone(void) {
//   app_vars.uart_lastTxByte++;
//   if (app_vars.uart_lastTxByte<length) {
//      uart_writeByte(stringToSend[app_vars.uart_lastTxByte]);
//   } else {
//      app_vars.uart_done = 1;
//   }
//}

uint8_t cb_uartRxCb(void) {

    //  uint8_t byte;
    uart_clearRxInterrupts();
    return 1;
}


void send_string(const char* str)
{
    strcpy(app_vars.uart_txFrame, str);
    for (uint16_t i = length; i < sizeof(app_vars.uart_txFrame); i++)
    app_vars.uart_txFrame[i] = 0;
    app_vars.uart_done = 0;
    app_vars.uart_lastTxByte = 0;
    uart_writeByte(app_vars.uart_txFrame[app_vars.uart_lastTxByte]);
    while(app_vars.uart_done==0);
}