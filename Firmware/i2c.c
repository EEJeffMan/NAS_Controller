/*
 * i2c.c
 *
 *  Created on: Feb 16, 2019
 *      Author: Jeff
 */

#include "DSP2803x_Device.h"     // DSP2803x Headerfile Include File
#include "DSP2803x_Examples.h"   // DSP2803x Examples Include File
#include "i2c.h"
#include "main.h"

void I2CA_Init(void);
Uint16 I2CA_WriteData(struct I2CMSG *msg);
Uint16 I2CA_ReadData(struct I2CMSG *msg, Uint16 command);
__interrupt void i2c_int1a_isr(void);

volatile Uint16 i2c_output_p0;
volatile Uint16 i2c_output_p1;

/*
 * PCA6416A data write:
 *  1. Address & R/nW bit (0 for write)
 *  2. Command byte
 *  3. Data byte 1
 *  4. Data byte 2
*/

/*
struct I2CMSG I2cMsgIn1=
{
    I2C_MSGSTAT_SEND_NOSTOP,
    I2C_SLAVE_ADDR,
    I2C_NUMBYTES,
    I2C_EEPROM_HIGH_ADDR,
    I2C_EEPROM_LOW_ADDR
};*/

//struct I2CMSG_NAS *CurrentMsgPtr;       // Used in interrupts



struct I2CMSG_NAS *CurrentMsgPtr_NAS;       // Used in interrupts

// Global variables
// Two bytes will be used for the outgoing address,
// thus only setup 14 bytes maximum
struct I2CMSG I2cMsgOut1={I2C_MSGSTAT_SEND_WITHSTOP,
                          I2C_SLAVE_ADDR,
                          I2C_NUMBYTES,
                          I2C_EEPROM_HIGH_ADDR,
                          I2C_EEPROM_LOW_ADDR,
                          0x12,                   // Msg Byte 1
                          0x34};                  // Msg Byte 2

struct I2CMSG I2cMsgIn1={ I2C_MSGSTAT_SEND_NOSTOP,
                          I2C_SLAVE_ADDR,
                          I2C_NUMBYTES,
                          I2C_EEPROM_HIGH_ADDR,
                          I2C_EEPROM_LOW_ADDR};

struct I2CMSG *CurrentMsgPtr;       // Used in interrupts

//
// I2CA_Init - Initialize I2C
//
void
I2CA_Init(void)
{
    Uint16 i;

    // Interrupts that are used in this example are re-mapped to
    // ISR functions found within this file.
       EALLOW;  // This is needed to write to EALLOW protected registers
       PieVectTable.I2CINT1A = &i2c_int1a_isr;
       EDIS;   // This is needed to disable write to EALLOW protected registers

    I2caRegs.I2CSAR = I2C_SLAVE_ADDR;       // Slave address - EEPROM control code

    I2caRegs.I2CPSC.all = 6;        // Prescaler - need 7-12 Mhz on module clk
    I2caRegs.I2CCLKL = 10;          // NOTE: must be non zero
    I2caRegs.I2CCLKH = 5;           // NOTE: must be non zero
    I2caRegs.I2CIER.all = 0x24;     // Enable SCD & ARDY interrupts

    I2caRegs.I2CMDR.all = 0x0020;   // Take I2C out of reset
                                    // Stop I2C when suspended

    I2caRegs.I2CFFTX.all = 0x6000;  // Enable FIFO mode and TXFIFO
    I2caRegs.I2CFFRX.all = 0x2040;  // Enable RXFIFO, clear RXFFINT,

    //
    // Clear incoming message buffer
    //
    for (i = 0; i < I2C_MAX_BUFFER_SIZE; i++)
    {
        I2cMsgIn1.MsgBuffer[i] = 0x0000;
    }

    //
    // Enable I2C interrupt 1 in the PIE: Group 8 interrupt 1
    //
    PieCtrlRegs.PIEIER8.bit.INTx1 = 1;

    //
    // Enable CPU INT8 which is connected to PIE group 8
    //
    IER |= M_INT8;

    //return;
}

//
// I2CA_WriteData -
//
Uint16 I2CA_WriteData(struct I2CMSG *msg)
{
    Uint16 i;

    //
    // Wait until STP bit is cleared from any previous master communication.
    // Clearing of this bit by the module is delayed until after the SCD bit is
    // set. If this bit is not checked prior to initiating a new message, the
    // I2C could get confused.
    //
    if (I2caRegs.I2CMDR.bit.STP == 1)
    {
        return I2C_STP_NOT_READY_ERROR;
    }

    //
    // Setup slave address
    //
    I2caRegs.I2CSAR = msg->SlaveAddress;

    //
    // Check if bus busy
    //
    if (I2caRegs.I2CSTR.bit.BB == 1)
    {
        return I2C_BUS_BUSY_ERROR;
    }

    //
    // Setup number of bytes to send
    // MsgBuffer
    //
    I2caRegs.I2CCNT = msg->NumOfBytes;

    //
    // Setup data to send
    //

    //I2caRegs.I2CDXR = msg->MemoryHighAddr;
    //I2caRegs.I2CDXR = msg->MemoryLowAddr;

    //
    // for (i=0; i<msg->NumOfBytes-2; i++)
    //
    for (i=0; i<msg->NumOfBytes; i++)
    {
        I2caRegs.I2CDXR = *(msg->MsgBuffer+i);
    }

    //
    // Send start as master transmitter
    //
    I2caRegs.I2CMDR.all = 0x6E20;

    return I2C_SUCCESS;
}

//
// I2CA_ReadData - Reads I2CA data
//
Uint16 I2CA_ReadData(struct I2CMSG *msg, Uint16 command)
{
    //
    // Wait until STP bit is cleared from any previous master communication.
    // Clearing of this bit by the module is delayed until after the SCD bit is
    // set. If this bit is not checked prior to initiating a new message, the
    // I2C could get confused.
    //
    if (I2caRegs.I2CMDR.bit.STP == 1)
    {
      return I2C_STP_NOT_READY_ERROR;
    }

    I2caRegs.I2CSAR = msg->SlaveAddress;

    if(msg->MsgStatus == I2C_MSGSTAT_SEND_NOSTOP)
    {
        //
        // Check if bus busy
        //
        if (I2caRegs.I2CSTR.bit.BB == 1)
        {
            return I2C_BUS_BUSY_ERROR;
        }

        I2caRegs.I2CCNT = 1;
        I2caRegs.I2CDXR = command;
        //I2caRegs.I2CDXR = msg->MemoryHighAddr;
        //I2caRegs.I2CDXR = msg->MemoryLowAddr;
        I2caRegs.I2CMDR.all = 0x2620;   // Send data to setup EEPROM address
    }

    else if(msg->MsgStatus == I2C_MSGSTAT_RESTART)
    {
        I2caRegs.I2CCNT = msg->NumOfBytes;  // Setup how many bytes to expect
        I2caRegs.I2CMDR.all = 0x2C20;       // Send restart as master receiver
    }
    return I2C_SUCCESS;
}

//
// i2c_int1a_isr -I2CA ISR
//
__interrupt void
i2c_int1a_isr(void)
{
    Uint16 IntSource, i;

    //
    // Read interrupt source
    //
    IntSource = I2caRegs.I2CISRC.all;

    //
    // Interrupt source = stop condition detected
    //
    if(IntSource == I2C_SCD_ISRC)
    {
        //
        // If completed message was writing data, reset msg to inactive state
        //
        if (CurrentMsgPtr->MsgStatus == I2C_MSGSTAT_WRITE_BUSY)
        {
            CurrentMsgPtr->MsgStatus = I2C_MSGSTAT_INACTIVE;
        }
        else
        {
            //
            // If a message receives a NACK during the address setup portion
            // of the EEPROM read, the code further below included in the
            // register access ready interrupt source code will generate a stop
            // condition. After the stop condition is received (here), set the
            // message status to try again. User may want to limit the number
            // of retries before generating an error.
            //
            if(CurrentMsgPtr->MsgStatus == I2C_MSGSTAT_SEND_NOSTOP_BUSY)
            {
                CurrentMsgPtr->MsgStatus = I2C_MSGSTAT_SEND_NOSTOP;
            }

            //
            // If completed message was reading EEPROM data, reset msg to
            // inactive state and read data from FIFO.
            //
            else if (CurrentMsgPtr->MsgStatus == I2C_MSGSTAT_READ_BUSY)
            {
                CurrentMsgPtr->MsgStatus = I2C_MSGSTAT_INACTIVE;

                for(i=0; i < I2C_NUMBYTES_READ; i++)
                {
                    CurrentMsgPtr->MsgBuffer[i] = I2caRegs.I2CDRR;
                }
            }
        }
    }

    //
    // Interrupt source = Register Access Ready
    // This interrupt is used to determine when the EEPROM address setup
    // portion of the read data communication is complete. Since no stop bit is
    // commanded, this flag tells us when the message has been sent instead of
    // the SCD flag. If a NACK is received, clear the NACK bit and command a
    // stop. Otherwise, move on to the read data portion of the communication.
    //
    else if(IntSource == I2C_ARDY_ISRC)
    {
        if(I2caRegs.I2CSTR.bit.NACK == 1)
        {
            I2caRegs.I2CMDR.bit.STP = 1;
            I2caRegs.I2CSTR.all = I2C_CLR_NACK_BIT;
        }
        else if(CurrentMsgPtr->MsgStatus == I2C_MSGSTAT_SEND_NOSTOP_BUSY)
        {
            CurrentMsgPtr->MsgStatus = I2C_MSGSTAT_RESTART;
        }
    }

    else
    {
        //
        // Generate some error due to invalid interrupt source
        //
        __asm("   ESTOP0");
    }

    //
    // Enable future I2C (PIE Group 8) interrupts
    //
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;
}


//end of file.
