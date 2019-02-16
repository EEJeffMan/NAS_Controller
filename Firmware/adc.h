/*
 * adc.h
 *
 *  Created on: Feb 16, 2019
 *      Author: Jeff
 */

#ifndef ADC_H_
#define ADC_H_

void init_adc();
void configure_adc();
__interrupt void adcint1_isr(void);
__interrupt void adcint2_isr(void);

#endif /* ADC_H_ */
