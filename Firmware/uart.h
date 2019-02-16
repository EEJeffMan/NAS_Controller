/*
 * uart.h
 *
 *  Created on: Feb 16, 2019
 *      Author: Jeff
 */

#ifndef UART_H_
#define UART_H_

void init_uart();
void scia_xmit(int a);
void scia_msg(char * msg);

#endif /* UART_H_ */
