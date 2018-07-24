/*
 * leds.c
 *
 *  Created on: Jul 16, 2018
 *      Author: Jeff Aymond
 */

#include "DSP2803x_Device.h"     // DSP2803x Headerfile Include File
#include "DSP2803x_Examples.h"   // DSP2803x Examples Include File
#include "leds.h"

void init_LEDs()
{
    /*
     * Green LED:       GPIO20
     * Yellow LED:      GPIO21
     * Red LED:         GPIO24
     *
     * 1. Configure as GPIO
     * 2. Set as outputs
     * 3. Set green LED on, others off
     */
    EALLOW;
    // green LED: gpio20
    GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 0;    // 0=GPIO,  1=EQEPA-1,  2=Resv,  3=COMP1OUT
    GpioCtrlRegs.GPADIR.bit.GPIO20 = 1;     // 1=OUTput,  0=INput
    GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;   // uncomment if --> Set Low initially

    // green LED: gpio21
    GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 0;    // 0=GPIO,  1=EQEPA-1,  2=Resv,  3=COMP1OUT
    GpioCtrlRegs.GPADIR.bit.GPIO21 = 1;     // 1=OUTput,  0=INput
    GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;   // uncomment if --> Set Low initially

    // green LED: gpio24
    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 0;    // 0=GPIO,  1=EQEPA-1,  2=Resv,  3=COMP1OUT
    GpioCtrlRegs.GPADIR.bit.GPIO24 = 1;     // 1=OUTput,  0=INput
    GpioDataRegs.GPACLEAR.bit.GPIO24 = 1;   // uncomment if --> Set Low initially
    EDIS;
}

void set_LED(led_t LED, led_state_t state)
{
    switch(LED)
    {
    case LED_GREEN:
        set_green_led(state);
        break;
    case LED_YELLOW:
        set_yellow_led(state);
        break;
    case LED_RED:
        set_red_led(state);
        break;

    default:

        break;
    }
}

void set_green_led (led_state_t state)
{
    //gpio20

    switch(state)
    {
    case LED_ON:
        GpioDataRegs.GPASET.bit.GPIO20 = 1;
        break;
    case LED_OFF:
        GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;
        break;
    case LED_TOGGLE:
        GpioDataRegs.GPATOGGLE.bit.GPIO20 = 1;
        break;
    default:
        break;
    }
}

void set_yellow_led (led_state_t state)
{
    //gpio21

    switch(state)
    {
    case LED_ON:
        GpioDataRegs.GPASET.bit.GPIO21 = 1;
        break;
    case LED_OFF:
        GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;
        break;
    case LED_TOGGLE:
        GpioDataRegs.GPATOGGLE.bit.GPIO21 = 1;
        break;
    default:
        break;
    }
}

void set_red_led (led_state_t state)
{
    //gpio24

    switch(state)
    {
    case LED_ON:
        GpioDataRegs.GPASET.bit.GPIO24 = 1;
        break;
    case LED_OFF:
        GpioDataRegs.GPACLEAR.bit.GPIO24 = 1;
        break;
    case LED_TOGGLE:
        GpioDataRegs.GPATOGGLE.bit.GPIO24 = 1;
        break;
    default:
        break;
    }
}


