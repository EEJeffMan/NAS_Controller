/*
 * i2c.h
 *
 *  Created on: Feb 16, 2019
 *      Author: Jeff
 */

#ifndef I2C_H_
#define I2C_H_

#define I2C_SLAVE_ADDR          0x20
#define I2C_NUMBYTES_WRITE      3
#define I2C_NUMBYTES_READ       2
#define I2C_PCA6416A_READ_P0    0x00
#define I2C_PCA6416A_READ_P1    0x01
#define I2C_PCA6416A_OUTPUT_P0  0x02
#define I2C_PCA6416A_OUTPUT_P1  0x03
#define I2C_PCA6416A_POLINV_P0  0x04
#define I2C_PCA6416A_POLINV_P1  0x05
#define I2C_PCA6416A_CONFIG_P0  0x06
#define I2C_PCA6416A_CONFIG_P1  0x07

//TODO: Remove these next 3 when driver is updated.
#define I2C_EEPROM_HIGH_ADDR    0
#define I2C_EEPROM_LOW_ADDR     1
#define I2C_NUMBYTES            2

//
// I2C Message Structure
//
struct I2CMSG_NAS {
    Uint16 MsgStatus;   // Word stating what state msg is in:
                        //   I2C_MSGCMD_INACTIVE = do not send msg
                        //   I2C_MSGCMD_BUSY = msg start has been sent,
                        //                     awaiting stop
                        //   I2C_MSGCMD_SEND_WITHSTOP = command to send
                        //       master trans msg complete with a stop bit
                        //   I2C_MSGCMD_SEND_NOSTOP = command to send
                        //       master trans msg without the stop bit
                        //   I2C_MSGCMD_RESTART = command to send a restart
                        //       as a master receiver with a stop bit
    Uint16 SlaveAddress;    // I2C address of slave msg is intended for
    Uint16 NumOfBytes;      // Num of valid bytes in/or to be put in MsgBuffer

    Uint16 MsgBuffer[I2C_MAX_BUFFER_SIZE]; // Array holding msg data - max that
                                           // MAX_BUFFER_SIZE can be is 16                                          // due to the FIFO's
};

#endif /* I2C_H_ */
