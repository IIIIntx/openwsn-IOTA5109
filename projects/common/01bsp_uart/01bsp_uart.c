/**
\brief This is a program which shows how to use the bsp modules for the board
       and UART.

\note: Since the bsp modules for different platforms have the same declaration,
       you can use this project with any platform.

Load this program on your board. Open a serial terminal client (e.g. PuTTY or
TeraTerm):
- You will read "Hello World!" printed over and over on your terminal client.
- when you enter a character on the client, the board echoes it back (i.e. you
  see the character on the terminal client) and the "ERROR" led blinks.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, February 2012
*/

#include "stdint.h"
#include "stdio.h"
#include "string.h"
// bsp modules required
#include "board.h"
#include "uart.h"
#include "sctimer.h"
#include "leds.h"

//=========================== defines =========================================

#define SCTIMER_PERIOD     0x4fff // 0xffff@32kHz = 2s
uint8_t stringToSend[]       = "Hello, World!\r\n";

//=========================== variables =======================================

typedef struct {
              uint8_t uart_lastTxByteIndex;
              uint8_t uart_lastRxByteIndex;
              uint8_t echoStartFlag;
              uint8_t echoFinishFlag;
   volatile   uint8_t uartDone;
   volatile   uint8_t uartSendNow;
} app_vars_t;

app_vars_t app_vars;
uint8_t stringToReceive[32];

//=========================== prototypes ======================================

void cb_compare(void);
void cb_uartTxDone(void);
uint8_t cb_uartRxCb(void);
void some_delay(void);

//=========================== main ============================================

/**
\brief The program starts executing here.
*/
int mote_main(void) {
  
   // clear local variable
   memset(&app_vars,0,sizeof(app_vars_t));
    
   app_vars.uartSendNow = 1;
   app_vars.uart_lastRxByteIndex = 0;
   app_vars.echoStartFlag = 0;
   app_vars.echoFinishFlag = 0;
   
   // initialize the board
   board_init();
   
   // setup UART
   uart_setCallbacks(cb_uartTxDone,cb_uartRxCb);
   uart_enableInterrupts();
   
   // setup sctimer
   sctimer_set_callback(cb_compare);
   sctimer_setCompare(sctimer_readCounter()+SCTIMER_PERIOD);
   
   while(1) {
      
      // wait for timer to elapse
      while (app_vars.uartSendNow==0);
      app_vars.uartSendNow = 0;
      uart_writeByte(0x66);
      // send string over UART
      app_vars.uartDone              = 0;
      app_vars.uart_lastTxByteIndex  = 0;
      //uart_writeByte(stringToSend[app_vars.uart_lastTxByteIndex]);
      uart_writeByte(0x70);
      while(app_vars.uartDone==0);
   }
}

//=========================== callbacks =======================================

void cb_compare(void) {
   
   // have main "task" send over UART
   app_vars.uartSendNow = 1;
   
   // schedule again
   sctimer_setCompare(sctimer_readCounter()+SCTIMER_PERIOD);
}

void cb_uartTxDone(void) {
   if (app_vars.echoStartFlag == 1)
   {
   app_vars.echoStartFlag == 0;
   //for(uint8_t i=0;i<=app_vars.uart_lastRxByteIndex;i=i+1){
   //   uart_writeByte(stringToReceive[i]);
   //   }
   uart_writeByte(stringToReceive[app_vars.uart_lastRxByteIndex]);
   uart_writeByte('\n');
   if(stringToReceive[app_vars.uart_lastRxByteIndex]==0x0d) app_vars.uart_lastRxByteIndex = 0;
   app_vars.echoFinishFlag = 1;
   }
   app_vars.uartDone = 1;
   //else {
   //  app_vars.uart_lastTxByteIndex++;
   //  if (app_vars.uart_lastTxByteIndex<sizeof(app_vars.uart_lastRxByteIndex)) {
   //     uart_writeByte(stringToSend[app_vars.uart_lastTxByteIndex]);
   //     //uart_writeByte(stringToReceive);
   //  } else {
   //     app_vars.uartDone = 1;
   //  }
   // }
}

uint8_t cb_uartRxCb(void) {
   uint8_t byte;
   // toggle LED
   leds_error_on();
   // read received byte
   byte = uart_readByte();
   stringToReceive[app_vars.uart_lastRxByteIndex] = byte;
   if(stringToReceive[app_vars.uart_lastRxByteIndex]==0x0d) 
     {app_vars.echoStartFlag=1;}  
   app_vars.uart_lastRxByteIndex ++;

   // echo that byte over serial
   return 0;
}

void some_delay(void) {
   volatile uint32_t delay;
   for (delay=0x00fffff;delay>0;delay--);
}