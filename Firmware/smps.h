/*
 * smps.h
 *
 *  Created on: Jul 16, 2018
 *      Author: Jeff
 */

#ifndef SMPS_H_
#define SMPS_H_

// TBCLK period = 60000 (60MHz)
#define ELOAD_PERIOD        600     // period = TBCLK period * VAUX_PERIOD... 600 = 100kHz
#define PERIOD_5V2_A        60      // period = TBCLK period * VAUX_PERIOD... 60 = 1MHz
#define PERIOD_5V2_B        60      // period = TBCLK period * VAUX_PERIOD... 60 = 1MHz
#define SEPIC_PERIOD        240     // period = TBCLK period * VAUX_PERIOD... 240 = 250kHz

#define DUTY_ELOAD          60      // 60/600 = 10% duty cycle @ 100kHz
#define DUTY_5V2A           30      // 30/60 = 50% duty cycle @ 1MHz
#define DUTY_5V2B           30      //
#define DUTY_SEPIC          80      // 80/240 = 33.3% @ 250kHz

void init_smps();

#endif /* SMPS_H_ */
