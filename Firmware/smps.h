/*
 * smps.h
 *
 *  Created on: Jul 16, 2018
 *      Author: Jeff
 */

#ifndef SMPS_H_
#define SMPS_H_

// TBCLK period = 60000 (60MHz)
#define ELOAD_PERIOD        6000    // period = TBCLK period * VAUX_PERIOD... 6000 = 10kHz
#define PERIOD_5V2_A        60      // period = TBCLK period * VAUX_PERIOD... 60 = 1MHz
#define PERIOD_5V2_B        60      // period = TBCLK period * VAUX_PERIOD... 60 = 1MHz
#define SEPIC_PERIOD        240     // period = TBCLK period * VAUX_PERIOD... 240 = 250kHz

void init_smps();

#endif /* SMPS_H_ */
