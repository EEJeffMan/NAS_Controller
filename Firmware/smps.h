/*
 * smps.h
 *
 *  Created on: Jul 16, 2018
 *      Author: Jeff
 */

#ifndef SMPS_H_
#define SMPS_H_

#define VAUX_PERIOD     400//240     // period = TBCLK period * VAUX_PERIOD - 240 = 250kHz
#define PERIOD_5V2_A     400//240     // period = TBCLK period * VAUX_PERIOD - 240 = 250kHz
#define PERIOD_5V2_B     400//240     // period = TBCLK period * VAUX_PERIOD - 240 = 250kHz
#define SEPIC_PERIOD     240     // period = TBCLK period * VAUX_PERIOD - 240 = 250kHz

void init_smps();

#endif /* SMPS_H_ */
