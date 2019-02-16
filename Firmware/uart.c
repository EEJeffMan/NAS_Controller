/*
 * uart.c
 *
 *  Created on: Feb 16, 2019
 *      Author: Jeff
 */

#include "DSP2803x_Device.h"     // DSP2803x Headerfile Include File
#include "DSP2803x_Examples.h"   // DSP2803x Examples Include File
#include "uart.h"

#include "sci.h"

SCI_Handle mySci;

void init_uart()
{
    char *uart_msg;

    mySci = SCI_init((void *)SCIA_BASE_ADDR, sizeof(SCI_Obj));
    //
    // 1 stop bit,  No loopback, No parity,8 char bits, async mode,
    // idle-line protocol
    //
    SCI_disableParity(mySci);
    SCI_setNumStopBits(mySci, SCI_NumStopBits_One);
    SCI_setCharLength(mySci, SCI_CharLength_8_Bits);

    SCI_enableTx(mySci);
    SCI_enableRx(mySci);
    SCI_enableTxInt(mySci);
    SCI_enableRxInt(mySci);

    SCI_setBaudRate(mySci, (SCI_BaudRate_e)194);
    SCI_enable(mySci);

    //init fifo
    SCI_enableFifoEnh(mySci);
    SCI_resetTxFifo(mySci);
    SCI_clearTxFifoInt(mySci);
    SCI_resetChannels(mySci);
    SCI_setTxFifoIntLevel(mySci, SCI_FifoLevel_Empty);

    SCI_resetRxFifo(mySci);
    SCI_clearRxFifoInt(mySci);
    SCI_setRxFifoIntLevel(mySci, SCI_FifoLevel_4_Words);

    uart_msg = "\r\nHello World !\0";
    scia_msg(uart_msg);
}

void scia_xmit(int a)
{
    /**if (!SCI_putDataNonBlocking(mySci, a))
    {
        //error - TX busy
    }*/
    SCI_putDataBlocking(mySci, a);
}

void scia_msg(char * msg)
{
    int i;
    i = 0;
    while(msg[i] != '\0')
    {
        scia_xmit(msg[i]);
        i++;
    }
}

//end of file.
