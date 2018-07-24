/*
 * leds.h
 *
 *  Created on: Jul 16, 2018
 *      Author: Jeff
 */

#ifndef LEDS_H_
#define LEDS_H_

typedef enum
{
    LED_GREEN,
    LED_YELLOW,
    LED_RED
} led_t;

typedef enum
{
    LED_ON,
    LED_OFF,
    LED_TOGGLE
} led_state_t;

void init_LEDs();
void set_LED(led_t LED, led_state_t state);
void set_green_led (led_state_t state);
void set_yellow_led (led_state_t state);
void set_red_led (led_state_t state);

#endif /* LEDS_H_ */
