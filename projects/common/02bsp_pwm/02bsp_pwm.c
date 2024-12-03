/**
\brief This is a program which shows how to use the bsp modules for the board
       and leds.

\note: Since the bsp modules for different platforms have the same declaration,
       you can use this project with any platform.

Load this program on your boards. The LEDs should start blinking furiously.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2014.
*/

#include "stdint.h"
#include "stdio.h"
// bsp modules required
#include "board.h"
#include "leds.h"
#include "nrfx_pwm.h"
#include "boards.h"
#include "nrf_gpio.h"

#define PIN_NUM_P0_08  NRF_GPIO_PIN_MAP(0,04)

void some_delay(void);

nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);

void init_gpio(void)
{
  nrf_gpio_cfg_output(PIN_NUM_P0_08);
}


// Function for initializing the PWM
void init_pwm(void)
{
    uint32_t err_code;
    // Declare a configuration structure and use a macro to instantiate it with default parameters.
    nrfx_pwm_config_t pwm_config = NRFX_PWM_DEFAULT_CONFIG;

    // We must override some of the parameters:
    pwm_config.output_pins[0] = LED_1; // Connect LED_1 on the nRF52840 DK to PWM Channel 0
    //pwm_config.output_pins[1] = LED_2; // Connect LED_2 on the nRF52840 DK to PWM Channel 1
    pwm_config.output_pins[2] = 3; // Connect LED_3 on the nRF52840 DK to PWM Channel 2
    //pwm_config.output_pins[3] = LED_4; // Connect LED_4 on the nRF52840 DK to PWM Channel 3
    pwm_config.top_value    = 20000; // Make PWM count from 0 - 100
    pwm_config.load_mode    = NRF_PWM_LOAD_INDIVIDUAL; // Use individual duty cycle for each PWM channel
    
    // Pass config structure into driver init() function 
    err_code = nrfx_pwm_init(&m_pwm0, &pwm_config, NULL);
    APP_ERROR_CHECK(err_code);
}

static nrf_pwm_values_individual_t pwm_duty_cycle_values = 
{
    .channel_0 = 19000, //< Duty cycle value for channel 0.
    .channel_1 = 3000, //< Duty cycle value for channel 1.
    .channel_2 = 20000, //< Duty cycle value for channel 2.
    .channel_3 = 20000  //< Duty cycle value for channel 3.
};

static nrf_pwm_sequence_t pwm_sequence =
{
    .values.p_individual = &pwm_duty_cycle_values,
    .length          = (sizeof(pwm_duty_cycle_values) / sizeof(uint16_t)),
    .repeats         = 0,
    .end_delay       = 0
};

/**
\brief The program starts executing here.
*/
int mote_main(void) {uint8_t i;
   
   board_init();
   init_pwm();
   nrfx_pwm_simple_playback(&m_pwm0, &pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);
   //init_gpio();
   //for(;;){
   //  nrf_gpio_pin_toggle(PIN_NUM_P0_08);
   //  leds_error_toggle();
   //  some_delay();
   //  }

   return 0;
}

void some_delay(void) {
   volatile uint32_t delay;
   for (delay=0x000fffff;delay>0;delay--);
}