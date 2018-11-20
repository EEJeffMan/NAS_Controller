//###########################################################################
//
// FILE:   Example_2803xClaAdc.c
//
// TITLE:  CLA ADC example
//
//!  \addtogroup f2803x_example_list
//!  <h1>CLA ADC (cla_adc)</h1>
//!
//! In this example ePWM1 is setup to generate a periodic ADC SOC.
//! Channel ADCINA2 is converted. When the ADC begins conversion,
//! it will assert ADCINT2 which will start CLA task 2.
//!
//! Cla Task2 logs 20 ADCRESULT1 values in a circular buffer.
//! When Task2 completes an interrupt to the CPU clears the ADCINT2 flag.
//!
//! \b Watch \b Variables \n
//! - VoltageCLA       - Last 20 ADCRESULT1 values
//! - ConversionCount  - Current result number
//! - LoopCount        - Idle loop counter
//
//
//###########################################################################
// $TI Release: F2803x Support Library v2.01.00.00 $
// $Release Date: Tue Jun 26 03:13:23 CDT 2018 $
// $Copyright:
// Copyright (C) 2009-2018 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//###########################################################################

//
// Included Files
//
#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include "CLAShared.h"
#include <string.h>
#include <stdint.h>
#include "sci.h"
#include "leds.h"
#include "smps.h"
#include "task_scheduler.h"
//#include "main.h"

//#define CLA_ENABLE
#define ADC_ENABLE
#define PWM_ENABLE
#define LED_ENABLE
#define UART_ENABLE
#define TASK_SCHEDULER_ENABLE
//#define I2C_ENABLE

#ifdef ADC_ENABLE
#define ADC_ARRAY_SIZE  20
#endif

//
// Prototype statements
//
#ifdef ADC_ENABLE
void configure_adc();
__interrupt void adcint1_isr(void);
__interrupt void adcint2_isr(void);
#endif
void init_gpio();

#ifdef I2C_ENABLE
void I2CA_Init(void);
uint16_t I2CA_WriteData(struct I2CMSG *msg);
Uint16 I2CA_ReadData(struct I2CMSG *msg);
__interrupt void i2c_int1a_isr(void);

#define I2C_SLAVE_ADDR        0x50
#define I2C_NUMBYTES          2
#define I2C_EEPROM_HIGH_ADDR  0x00
#define I2C_EEPROM_LOW_ADDR   0x30

//
// Two bytes will be used for the outgoing address,
// thus only setup 14 bytes maximum
//
struct I2CMSG I2cMsgOut1=
{
    I2C_MSGSTAT_SEND_WITHSTOP,
    I2C_SLAVE_ADDR,
    I2C_NUMBYTES,
    I2C_EEPROM_HIGH_ADDR,
    I2C_EEPROM_LOW_ADDR,
    0x12,     // Msg Byte 1
    0x34      // Msg Byte 2
};

struct I2CMSG I2cMsgIn1=
{
    I2C_MSGSTAT_SEND_NOSTOP,
    I2C_SLAVE_ADDR,
    I2C_NUMBYTES,
    I2C_EEPROM_HIGH_ADDR,
    I2C_EEPROM_LOW_ADDR
};

struct I2CMSG *CurrentMsgPtr;       // Used in interrupts

#endif

#ifdef CLA_ENABLE
__interrupt void cla1_isr2(void);

//
// Globals
//
#pragma DATA_SECTION(ConversionCount, "Cla1ToCpuMsgRAM");
#pragma DATA_SECTION(VoltageCLA,      "Cla1ToCpuMsgRAM");
#endif

Uint16 ConversionCount;
Uint16 VoltageCLA[NUM_DATA_POINTS];

#ifdef ADC_ENABLE
volatile Uint16 adc_array_A0[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_B0[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_A1[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_B1[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_A2[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_B2[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_A3[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_B3[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_A4[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_B4[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_A7[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_B7[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_A6[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_B6[ADC_ARRAY_SIZE];
volatile Uint16 adc_array_A5[ADC_ARRAY_SIZE];
volatile Uint16 adc_index = 0;
#endif

//
// These are defined by the linker file
//

// Used for running BackGround in flash, and ISR in RAM
//extern Uint16 *IQfuncsLoadStart, *IQfuncsLoadEnd, *IQfuncsRunStart;

extern Uint16 Cla1funcsLoadStart;
extern Uint16 Cla1funcsLoadSize;
extern Uint16 Cla1funcsRunStart;

#ifdef UART_ENABLE
SCI_Handle mySci;

void init_uart();
void scia_xmit(int a);
void scia_msg(char * msg);
#endif

//
// Main
//
void main(void)
{
#ifdef UART_ENABLE
    char *uart_msg;
#endif

    unsigned int i;

    //
    // Step 1. Initialize System Control:
    // PLL, WatchDog, enable Peripheral Clocks
    // This example function is found in the DSP2803x_SysCtrl.c file.
    //
    InitSysCtrl();

    //
    // Step 2. Initialize GPIO:
    // This example function is found in the DSP2803x_Gpio.c file and
    // illustrates how to set the GPIO to it's default state.
    //
    //InitGpio();  // Skipped for this example
    init_gpio();

    //
    // Step 3. Clear all interrupts and initialize PIE vector table:
    // Disable CPU interrupts
    //
    DINT;

    //
    // Initialize the PIE control registers to their default state.
    // The default state is all PIE interrupts disabled and flags
    // are cleared.
    // This function is found in the DSP2803x_PieCtrl.c file.
    //
    InitPieCtrl();

    //
    // Disable CPU interrupts and clear all CPU interrupt flags:
    //
    IER = 0x0000;
    IFR = 0x0000;

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    // This will populate the entire table, even if the interrupt
    // is not used in this example.  This is useful for debug purposes.
    // The shell ISR routines are found in DSP2803x_DefaultIsr.c.
    // This function is found in DSP2803x_PieVect.c.
    //
    InitPieVectTable();

#ifdef ADC_ENABLE
    EALLOW;
    PieVectTable.ADCINT1 = &adcint1_isr;
    PieVectTable.ADCINT2 = &adcint2_isr;
    EDIS;
#endif

#ifdef I2C_ENABLE
    I2CA_Init();

    //
    // Clear incoming message buffer
    //
    for (i = 0; i < I2C_MAX_BUFFER_SIZE; i++)
    {
        I2cMsgIn1.MsgBuffer[i] = 0x0000;
    }
#endif

//#ifdef FLASH
// Copy time critical code and Flash setup code to RAM
// The  RamfuncsLoadStart, RamfuncsLoadEnd, and RamfuncsRunStart
// symbols are created by the linker. Refer to the linker files.

    MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
//    MemCopy(&IQfuncsLoadStart, &IQfuncsLoadEnd, &IQfuncsRunStart);

// Call Flash Initialization to setup flash waitstates
// This function must reside in RAM

    InitFlash();    // Call the flash wrapper init function

//#endif //(FLASH)

    //
    // Interrupts that are used in this example are re-mapped to
    // ISR functions found within this file.
    //
#ifdef CLA_ENABLE
    EALLOW;             // This is needed to write to EALLOW protected register
    PieVectTable.CLA1_INT2 = &cla1_isr2;
    EDIS;      // This is needed to disable write to EALLOW protected registers
#endif
    //
    // Step 4. Initialize all the Device Peripherals:
    //
#ifdef ADC_ENABLE
    InitAdc();         // For this example, init the ADC
    InitAdcAio();
    AdcOffsetSelfCal();
#endif

    //
    // Step 5. User specific code, enable interrupts:
    //

    //
    // Enable ADCINT1 in PIE
    //
#ifdef CLA
    PieCtrlRegs.PIEIER11.bit.INTx2 = 1; // Enable INT 11.2 in PIE (CLA Task2)
#endif
    //IER |= M_INT11;                     // Enable CPU Interrupt 11
#ifdef ADC_ENABLE
    // ADCINT1: INT1.1
    // ADCINT2: INT1.2
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;  // Enable INT 1.1 in the PIE
    PieCtrlRegs.PIEIER1.bit.INTx2 = 1;  // Enable INT 1.2 in the PIE

#ifdef I2C_ENABLE
    //
    // Enable I2C interrupt 1 in the PIE: Group 8 interrupt 1
    //
    PieCtrlRegs.PIEIER8.bit.INTx1 = 1;

    //
    // Enable CPU INT8 which is connected to PIE group 8
    //
    IER |= M_INT8;
#endif

    IER |= M_INT1;      // Enable CPU INT1 for ADCINT1,ADCINT2,ADCINT9,TripZone
#endif
    EINT;                               // Enable Global interrupt INTM
    ERTM;                              // Enable Global realtime interrupt DBGM

#ifdef ADC_ENABLE
    configure_adc();
#endif

    //
    // Copy CLA code from its load address to CLA program RAM
    //
    // Note: during debug the load and run addresses can be
    // the same as Code Composer Studio can load the CLA program
    // RAM directly.
    //
    // The ClafuncsLoadStart, ClafuncsLoadEnd, and ClafuncsRunStart
    // symbols are created by the linker.
    //
#ifdef CLA_ENABLE
    memcpy((uint16_t *)&Cla1funcsRunStart,(uint16_t *)&Cla1funcsLoadStart,
            (unsigned long)&Cla1funcsLoadSize);
#endif
    //
    // Initialize the CLA registers
    //

    EALLOW;
#ifdef CLA_ENABLE
    Cla1Regs.MVECT2 = (Uint16) (&Cla1Task2 - &Cla1Prog_Start)*sizeof(Uint32);
    Cla1Regs.MVECT8 = (Uint16) (&Cla1Task8 - &Cla1Prog_Start)*sizeof(Uint32);
    
    //
    // ADCINT2 starts Task 2
    //
    Cla1Regs.MPISRCSEL1.bit.PERINT2SEL = CLA_INT2_ADCINT2; 
    
    Cla1Regs.MMEMCFG.bit.PROGE = 1;  // Map CLA program memory to the CLA
    Cla1Regs.MCTL.bit.IACKE = 1;     // Enable IACK to start tasks via software
    Cla1Regs.MIER.all = (M_INT8 | M_INT2);   // Enable Task 8 and Task 2
    Cla1ForceTask8andWait();    // Force CLA task 8.
                                // This will initialize ConversionCount to zero
#endif

    EDIS;

    //
    // Assumes ePWM1 clock is already enabled in InitSysCtrl();
    //
#ifdef PWM_ENABLE
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
/*    EDIS;
    EPwm1Regs.ETSEL.bit.SOCAEN  = 1;    // Enable SOC on A group
    EPwm1Regs.ETSEL.bit.SOCASEL = 4;    // Select SOC from from CPMA on upcount
    EPwm1Regs.ETPS.bit.SOCAPRD  = 1;    // Generate pulse on 1st event
    EPwm1Regs.CMPA.half.CMPA = 0x2EE;   // Set compare A value
    
    //
    // Set period for ePWM1 - this will determine the sampling frequency(20KHz)
    //
    EPwm1Regs.TBPRD = 0x5DC;

    EPwm1Regs.TBCTL.bit.CTRMODE     = 0;        // count up and start
    EALLOW;
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;*/

    init_smps();
    SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;

    //initialize duty cycles to test supplies
    // Vaux: 5.5Vout; duty cycle = 5.5/12 = 45.8%... period = 240, CMPA = 110
    // 5V2 A & B: 5.2Vout, duty cycle = 5.2/12 = 43.3%... period = 240, CMPA = 104
    // SEPIC: 6Vout, duty cycle = (vo/vi) / (1+vo/vi) = (6/12)/(1+6/12) = 33.3%... period = 240, CMPA = 80

    EPwm1Regs.CMPA.half.CMPA = 667;//267;//133;//80;//110;     80/240 = 33.3%... 12*0.333 ~= 4V
    EPwm2Regs.CMPA.half.CMPA = 133;//104;
    EPwm3Regs.CMPA.half.CMPA = 133;//104;
    EPwm4Regs.CMPA.half.CMPA = 80;

    EDIS;
#endif

#ifdef TASK_SCHEDULER_ENABLE
    EALLOW;
    // Timing sync for background loops
    // Timer period definitions found in PeripheralHeaderIncludes.h
    CpuTimer0Regs.PRD.all = mSec5;      // A tasks
    CpuTimer1Regs.PRD.all = mSec50; // B tasks

    // Tasks State-machine init
    Alpha_State_Ptr = &A0;
    A_Task_Ptr = &A1;
    B_Task_Ptr = &B1;

    VTimer0[0] = 0;
    VTimer1[0] = 0;
    EDIS;
#endif

#ifdef LED_ENABLE
    //init_LEDs();  // configured in init_gpio()
    set_LED(LED_GREEN, LED_ON);
#endif

#ifdef UART_ENABLE
    //uart
    mySci = SCI_init((void *)SCIA_BASE_ADDR, sizeof(SCI_Obj));
    init_uart();

    uart_msg = "\r\nHello World !\0";
    scia_msg(uart_msg);
#endif
    //
    // Wait for ADC interrupt
    //

    // initialize adc arrays
    for (i = 0; i < ADC_ARRAY_SIZE; i++)
    {
        adc_array_A0[i] = 0;
        adc_array_A1[i] = 0;
        adc_array_A2[i] = 0;
        adc_array_A3[i] = 0;
        adc_array_A4[i] = 0;
        adc_array_A5[i] = 0;
        adc_array_A6[i] = 0;
        adc_array_A7[i] = 0;
        adc_array_B0[i] = 0;
        adc_array_B1[i] = 0;
        adc_array_B2[i] = 0;
        adc_array_B3[i] = 0;
        adc_array_B4[i] = 0;
        adc_array_B6[i] = 0;
        adc_array_B7[i] = 0;
    }


    for(;;)
    {
#ifdef TASK_SCHEDULER_ENABLE
        // State machine entry & exit point
        //===========================================================
        (*Alpha_State_Ptr)();   // jump to an Alpha state (A0,B0,...)
        //===========================================================
#endif
    }
}

void MemCopy(Uint16 *SourceAddr, Uint16* SourceEndAddr, Uint16* DestAddr)
{
    while(SourceAddr < SourceEndAddr)
    {
       *DestAddr++ = *SourceAddr++;
    }
    return;
}

void init_gpio()
{
    //--------------------------------------------------------------------------------------
    // GPIO (GENERAL PURPOSE I/O) CONFIG
    //--------------------------------------------------------------------------------------
    //-----------------------
    // QUICK NOTES on USAGE:
    //-----------------------
    // If GpioCtrlRegs.GP?MUX?bit.GPIO?= 1, 2 or 3 (i.e. Non GPIO func), then leave
    //  rest of lines commented
    // If GpioCtrlRegs.GP?MUX?bit.GPIO?= 0 (i.e. GPIO func), then:
    //  1) uncomment GpioCtrlRegs.GP?DIR.bit.GPIO? = ? and choose pin to be IN or OUT
    //  2) If IN, can leave next to lines commented
    //  3) If OUT, uncomment line with ..GPACLEAR.. to force pin LOW or
    //             uncomment line with ..GPASET.. to force pin HIGH or
    //--------------------------------------------------------------------------------------
    EALLOW;
#ifdef PWM_ENABLE
    //--------------------------------------------------------------------------------------
    //  GPIO-00 - PIN FUNCTION = PWM1A (Vaux)
        GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;     // 0=GPIO,  1=EPWM1A,  2=Resv,  3=Resv
    //  GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO0 = 1;      // uncomment if --> Set High initially
#else
    //--------------------------------------------------------------------------------------
    //  GPIO-00 - PIN FUNCTION = PWM1A (Vaux)
        GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;     // 0=GPIO,  1=EPWM1A,  2=Resv,  3=Resv
        GpioCtrlRegs.GPADIR.bit.GPIO0 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO0 = 1;      // uncomment if --> Set High initially
#endif
    //--------------------------------------------------------------------------------------
    //  GPIO-01 - PIN FUNCTION = TP6
        GpioCtrlRegs.GPAMUX1.bit.GPIO1 = 0;     // 0=GPIO,  1=EPWM1B,  2=Resv,  3=COMP1OUT
        GpioCtrlRegs.GPADIR.bit.GPIO1 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO1 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO1 = 1;      // uncomment if --> Set High initially
#ifdef PWM_ENABLE
    //--------------------------------------------------------------------------------------
    //  GPIO-02 - PIN FUNCTION = PWM2A (5V2_A)
        GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;     // 0=GPIO,  1=EPWM2A,  2=Resv,  3=Resv
    //  GpioCtrlRegs.GPADIR.bit.GPIO2 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO2 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO2 = 1;      // uncomment if --> Set High initially
#else
    //--------------------------------------------------------------------------------------
    //  GPIO-02 - PIN FUNCTION = PWM2A (5V2_A)
        GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 0;     // 0=GPIO,  1=EPWM2A,  2=Resv,  3=Resv
        GpioCtrlRegs.GPADIR.bit.GPIO2 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO2 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO2 = 1;      // uncomment if --> Set High initially
#endif
    //--------------------------------------------------------------------------------------
    //  GPIO-03 - PIN FUNCTION = Digital out (PI_RST_OUT)
        GpioCtrlRegs.GPAMUX1.bit.GPIO3 = 0;     // 0=GPIO,  1=EPWM2B,  2=SPISOMI-A,  3=COMP2OUT
        GpioCtrlRegs.GPADIR.bit.GPIO3 = 1;      // 1=OUTput,  0=INput
        GpioDataRegs.GPACLEAR.bit.GPIO3 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO3 = 1;      // uncomment if --> Set High initially
#ifdef PWM_ENABLE
    //--------------------------------------------------------------------------------------
    //  GPIO-04 - PIN FUNCTION = PWM3A (5V2_B)
        GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;     // 0=GPIO,  1=EPWM3A,  2=Resv,  3=Resv
    //  GpioCtrlRegs.GPADIR.bit.GPIO4 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO4 = 1;      // uncomment if --> Set High initially
#else
    //--------------------------------------------------------------------------------------
    //  GPIO-04 - PIN FUNCTION = PWM3A (5V2_B)
        GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 0;     // 0=GPIO,  1=EPWM3A,  2=Resv,  3=Resv
        GpioCtrlRegs.GPADIR.bit.GPIO4 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO4 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO4 = 1;      // uncomment if --> Set High initially
#endif
    //--------------------------------------------------------------------------------------
    //  GPIO-05 - PIN FUNCTION = Fan feedback (Ecap?)
        GpioCtrlRegs.GPAMUX1.bit.GPIO5 = 3;     // 0=GPIO,  1=EPWM3B,  2=SPISIMO-A,  3=ECAP1
    //  GpioCtrlRegs.GPADIR.bit.GPIO5 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO5 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO5 = 1;      // uncomment if --> Set High initially
#ifdef PWM_ENABLE
    //--------------------------------------------------------------------------------------
    //  GPIO-06 - PIN FUNCTION = PWM4A (SEPIC)
        GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 1;     // 0=GPIO,  1=EPWM4A,  2=SYNCI,  3=SYNCO
    //  GpioCtrlRegs.GPADIR.bit.GPIO6 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO6 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO6 = 1;      // uncomment if --> Set High initially
#else
    //--------------------------------------------------------------------------------------
    //  GPIO-06 - PIN FUNCTION = PWM4A (SEPIC)
        GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 0;     // 0=GPIO,  1=EPWM4A,  2=SYNCI,  3=SYNCO
        GpioCtrlRegs.GPADIR.bit.GPIO6 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO6 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO6 = 1;      // uncomment if --> Set High initially
#endif
    //--------------------------------------------------------------------------------------
    //  GPIO-07 - PIN FUNCTION = DEBUG_UART_RX
        GpioCtrlRegs.GPAMUX1.bit.GPIO7 = 2;     // 0=GPIO,  1=EPWM4B,  2=SCIRX-A,  3=Resv
    //  GpioCtrlRegs.GPADIR.bit.GPIO7 = 1;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO7 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO7 = 1;      // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-08 - PIN FUNCTION = PWM5A (MIKROE)
        GpioCtrlRegs.GPAMUX1.bit.GPIO8 = 1;     // 0=GPIO,  1=EPWM5A,  2=Resv,  3=ADCSOC-A
    //  GpioCtrlRegs.GPADIR.bit.GPIO8 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO8 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO8 = 1;      // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-09 - PIN FUNCTION = Digital in (MIKROE AN)
        GpioCtrlRegs.GPAMUX1.bit.GPIO9 = 0;     // 0=GPIO,  1=EPWM5B,  2=LINTX-A,  3=Resv
        GpioCtrlRegs.GPADIR.bit.GPIO9 = 0;      // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO9 = 1;    // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO9 = 1;      // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-10 - PIN FUNCTION = Digital out (PI_PWR_OUT)
        GpioCtrlRegs.GPAMUX1.bit.GPIO10 = 0;    // 0=GPIO,  1=EPWM6A,  2=Resv,  3=ADCSOC-B
        GpioCtrlRegs.GPADIR.bit.GPIO10 = 1;     // 1=OUTput,  0=INput
        GpioDataRegs.GPACLEAR.bit.GPIO10 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO10 = 1;     // uncomment if --> Set High initially
#ifdef PWM_ENABLE
    //--------------------------------------------------------------------------------------
    //  GPIO-11 - PIN FUNCTION = PWM6B (Fan)
        GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 1;    // 0=GPIO,  1=EPWM6B,  2=LINRX-A,  3=Resv
    //  GpioCtrlRegs.GPADIR.bit.GPIO11 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO11 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO11 = 1;     // uncomment if --> Set High initially
#else
    //--------------------------------------------------------------------------------------
    //  GPIO-11 - PIN FUNCTION = PWM6B (Fan)
        GpioCtrlRegs.GPAMUX1.bit.GPIO11 = 0;    // 0=GPIO,  1=EPWM6B,  2=LINRX-A,  3=Resv
        GpioCtrlRegs.GPADIR.bit.GPIO11 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO11 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO11 = 1;     // uncomment if --> Set High initially
#endif
    //--------------------------------------------------------------------------------------
    //  GPIO-12 - PIN FUNCTION = DEBUG_UART_TX
        GpioCtrlRegs.GPAMUX1.bit.GPIO12 = 2;    // 0=GPIO,  1=TZ1,  2=SCITX-A,  3=SPISIMO-B
    //  GpioCtrlRegs.GPADIR.bit.GPIO12 = 1;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO12 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO12 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-13 - PIN FUNCTION = NA
        GpioCtrlRegs.GPAMUX1.bit.GPIO13 = 0;    // 0=GPIO,  1=TZ2,  2=Resv,  3=SPISOMI-B
        GpioCtrlRegs.GPADIR.bit.GPIO13 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO13 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO13 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-14 - PIN FUNCTION = NA
        GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 0;    // 0=GPIO,  1=TZ3,  2=LINTX-A,  3=SPICLK-B
        GpioCtrlRegs.GPADIR.bit.GPIO14 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO14 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO14 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-15 - PIN FUNCTION = NA
        GpioCtrlRegs.GPAMUX1.bit.GPIO15 = 0;    // 0=GPIO,  1=TZ1,  2=LINRX-A,  3=SPISTE-B
        GpioCtrlRegs.GPADIR.bit.GPIO15 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO15 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO15 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------
    //  GPIO-16 - PIN FUNCTION = SPI_MOSI
        GpioCtrlRegs.GPAMUX2.bit.GPIO16 = 1;    // 0=GPIO,  1=SPISIMO-A,  2=Resv,  3=TZ2
    //  GpioCtrlRegs.GPADIR.bit.GPIO16 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO16 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO16 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-17 - PIN FUNCTION = SPI_MISO
        GpioCtrlRegs.GPAMUX2.bit.GPIO17 = 1;    // 0=GPIO,  1=SPISOMI-A,  2=Resv,  3=TZ3
    //  GpioCtrlRegs.GPADIR.bit.GPIO17 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO17 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO17 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-18 - PIN FUNCTION = SPI_SCK
        GpioCtrlRegs.GPAMUX2.bit.GPIO18 = 1;    // 0=GPIO,  1=SPICLK-A,  2=LINTX-A,  3=XCLKOUT
    //  GpioCtrlRegs.GPADIR.bit.GPIO18 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO18 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO18 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-19 - PIN FUNCTION = TP1
        GpioCtrlRegs.GPAMUX2.bit.GPIO19 = 0;    // 0=GPIO,  1=SPISTE-A,  2=LINRX-A,  3=ECAP1
        GpioCtrlRegs.GPADIR.bit.GPIO19 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO19 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO19 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-20 - PIN FUNCTION = Green LED
        GpioCtrlRegs.GPAMUX2.bit.GPIO20 = 0;    // 0=GPIO,  1=EQEPA-1,  2=Resv,  3=COMP1OUT
        GpioCtrlRegs.GPADIR.bit.GPIO20 = 1;     // 1=OUTput,  0=INput
        GpioDataRegs.GPACLEAR.bit.GPIO20 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO20 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-21 - PIN FUNCTION = Yellow LED
        GpioCtrlRegs.GPAMUX2.bit.GPIO21 = 0;    // 0=GPIO,  1=EQEPB-1,  2=Resv,  3=COMP2OUT
        GpioCtrlRegs.GPADIR.bit.GPIO21 = 1;     // 1=OUTput,  0=INput
        GpioDataRegs.GPACLEAR.bit.GPIO21 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO21 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-22 - PIN FUNCTION = IO expander interrupt input
        GpioCtrlRegs.GPAMUX2.bit.GPIO22 = 0;    // 0=GPIO,  1=EQEPS-1,  2=Resv,  3=LINTX-A
        GpioCtrlRegs.GPADIR.bit.GPIO22 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO22 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO22 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-23 - PIN FUNCTION = IO expander reset output
        GpioCtrlRegs.GPAMUX2.bit.GPIO23 = 0;    // 0=GPIO,  1=EQEPI-1,  2=Resv,  3=LINRX-A
        GpioCtrlRegs.GPADIR.bit.GPIO23 = 1;     // 1=OUTput,  0=INput
        GpioDataRegs.GPACLEAR.bit.GPIO23 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO23 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-24 - PIN FUNCTION = Red LED
        GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 0;    // 0=GPIO,  1=ECAP1,  2=Resv,  3=SPISIMO-B
        GpioCtrlRegs.GPADIR.bit.GPIO24 = 1;     // 1=OUTput,  0=INput
        GpioDataRegs.GPACLEAR.bit.GPIO24 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO24 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-25 - PIN FUNCTION = NA
        GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 3;    // 0=GPIO,  1=Resv,  2=Resv,  3=SPISOMI-B
    //  GpioCtrlRegs.GPADIR.bit.GPIO25 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO25 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO25 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-26 - PIN FUNCTION = NA
        GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 3;    // 0=GPIO,  1=Resv,  2=Resv,  3=SPICLK-B
    //  GpioCtrlRegs.GPADIR.bit.GPIO26 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO26 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO26 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-27 - PIN FUNCTION = NA
        GpioCtrlRegs.GPAMUX2.bit.GPIO27 = 3;    // 0=GPIO,  1=Resv,  2=Resv,  3=SPISTE-B
    //  GpioCtrlRegs.GPADIR.bit.GPIO27 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO27 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO27 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-28 - PIN FUNCTION = UART_COM_RX
        GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;    // 0=GPIO,  1=SCIRX-A,  2=I2CSDA-A,  3=TZ2
    //  GpioCtrlRegs.GPADIR.bit.GPIO28 = 1;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO28 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO28 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-29 - PIN FUNCTION = UART_COM_TX
        GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;    // 0=GPIO,  1=SCITXD-A,  2=I2CSCL-A,  3=TZ3
    //  GpioCtrlRegs.GPADIR.bit.GPIO29 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO29 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO29 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-30 - PIN FUNCTION = TP5
        GpioCtrlRegs.GPAMUX2.bit.GPIO30 = 0;    // 0=GPIO,  1=CANRX-A,  2=Resv,  3=Resv
        GpioCtrlRegs.GPADIR.bit.GPIO30 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO30 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO30 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-31 - PIN FUNCTION = TP4
        GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0;    // 0=GPIO,  1=CANTX-A,  2=Resv,  3=Resv
        GpioCtrlRegs.GPADIR.bit.GPIO31 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPACLEAR.bit.GPIO31 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPASET.bit.GPIO31 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------
    //  GPIO-32 - PIN FUNCTION = I2C_SDA
        GpioCtrlRegs.GPBMUX1.bit.GPIO32 = 1;    // 0=GPIO,  1=I2CSDA-A,  2=SYNCI,  3=ADCSOCA
    //  GpioCtrlRegs.GPBDIR.bit.GPIO32 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO32 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO32 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-33 - PIN FUNCTION = I2C_SCL
        GpioCtrlRegs.GPBMUX1.bit.GPIO33 = 1;    // 0=GPIO,  1=I2CSCL-A,  2=SYNCO,  3=ADCSOCB
    //  GpioCtrlRegs.GPBDIR.bit.GPIO33 = 1;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO33 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO33 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-34 - PIN FUNCTION = BOOT_MODE
        GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0;    // 0=GPIO,  1=Resv,  2=Resv,  3=Resv
        GpioCtrlRegs.GPBDIR.bit.GPIO34 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO34 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO34 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------
    // GPIO 35-38 are defaulted to JTAG usage, and are not shown here to enforce JTAG debug
    // usage.
    //--------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------

    // GPIO 39-44 are not on the 28035 device used.
    //  GPIO-39 - PIN FUNCTION = NA
        GpioCtrlRegs.GPBMUX1.bit.GPIO39 = 0;    // 0=GPIO,  1=Resv,  2=Resv,  3=Resv
        GpioCtrlRegs.GPBDIR.bit.GPIO39 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO39 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO39 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-40 - PIN FUNCTION = NA
        GpioCtrlRegs.GPBMUX1.bit.GPIO40 = 0;    // 0=GPIO,  1=EPWM7A,  2=Resv,  3=Resv
        GpioCtrlRegs.GPBDIR.bit.GPIO40 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO40 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO40 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-41 - PIN FUNCTION = NA
        GpioCtrlRegs.GPBMUX1.bit.GPIO41 = 0;    // 0=GPIO,  1=EPWM7B,  2=Resv,  3=Resv
        GpioCtrlRegs.GPBDIR.bit.GPIO41 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO41 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO41 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-42 - PIN FUNCTION = NA
        GpioCtrlRegs.GPBMUX1.bit.GPIO42 = 0;    // 0=GPIO,  1=Resv,  2=Resv,  3=COMP1OUT
        GpioCtrlRegs.GPBDIR.bit.GPIO42 = 0; // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO42 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO42 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-43 - PIN FUNCTION = NA
        GpioCtrlRegs.GPBMUX1.bit.GPIO43 = 0;    // 0=GPIO,  1=Resv,  2=Resv,  3=COMP2OUT
        GpioCtrlRegs.GPBDIR.bit.GPIO43 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO43 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO43 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //  GPIO-44 - PIN FUNCTION = NA
        GpioCtrlRegs.GPBMUX1.bit.GPIO44 = 0;    // 0=GPIO,  1=Resv,  2=Resv,  3=Resv
        GpioCtrlRegs.GPBDIR.bit.GPIO44 = 0;     // 1=OUTput,  0=INput
    //  GpioDataRegs.GPBCLEAR.bit.GPIO44 = 1;   // uncomment if --> Set Low initially
    //  GpioDataRegs.GPBSET.bit.GPIO44 = 1;     // uncomment if --> Set High initially
    //--------------------------------------------------------------------------------------
    //--------------------------------------------------------------------------------------
        EDIS;   // Disable register access
}

#ifdef UART_ENABLE
void init_uart()
{
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
#endif

#ifdef ADC_ENABLE

void configure_adc()
{
    EALLOW;

    /*
     * ADC channels:
     *      A0/B0: Vaux V/I, SOC0/1
     *      A1/B1: 5V2_A V/I, SOC2/3
     *      A2/B2: 5V2_B V/I, SOC4/5
     *      A3/B3: SEPIC V/I, SOC6/7
     *      A4/B4: Batt V/I, SOC8/9
     *      A7/B7: VCC_V / Batt Temp, SOC10/11
     *      A6/B6: Vin V/I, SOC12/13
     *      A5: Internal temp sensor, SOC14
     *
     *      High priority: A/B 0-3, SOC0-7
     *      Low priority: A/B 4-7, SOC8-14
     *
     *      SOCPRICTL.SOCPRIORITY = 0x08: SOC0-7 are high priority, SOC8+ are round robin
     *
     *      Simultaneous samples (ADCSAMPLEMODE):
     *      A0/B0, SOC0/1 -> SIMULEN0
     *      A1/B1, SOC2/3 -> SIMULEN2
     *      A2/B2, SOC4/5 -> SIMULEN4
     *      A3/B3, SOC6/7 -> SIMULEN6
     *      A4/B4, SOC8/9 -> SIMULEN8
     *      A6/B6, SOC12/13 -> SIMULEN12
     *      A7/B7, SOC10/11 -> SIMULEN10
     *
     *  SOC's:
     *      A0/B0: EPWM1 trig, SOC0/1
     *      A1/B1: EPWM1 trig, SOC2/3
     *      A2/B2: EPWM1 trig, SOC4/5
     *      A3/B3: EPWM1 trig, SOC6/7
     *
     *      A4/B4: EOC7
     *      A5: EOC7
     *      A6/B6: EOC7
     *      A7/B7: EOC6
     *
     *      ADCINTSOCSEL1 (SOC0-7) = 0x00 - all SOC0-7 triggers are determined by TRIGSEL (below)
     *      ADCINTSOCSEL2 (SOC8-15):
     *      0x01 for SOC8-14... ADCINT1 will trigger SOC8-14, and TRIGSEL (below) will be ignored.
     *      SOC15 is N/A, not used.
     *
     *      ADCSOCxCTL.TRIGSEL:
     *      0: 0x05 - EPWM1, SOCA
     *      1: 0x05 - EPWM1, SOCA
     *      2: 0x05 - EPWM1, SOCA
     *      3: 0x05 - EPWM1, SOCA
     *      4: 0x05 - EPWM1, SOCA
     *      5: 0x05 - EPWM1, SOCA
     *      6: 0x05 - EPWM1, SOCA
     *      7: 0x05 - EPWM1, SOCA
     *      8: 0 - SW trig only (overridden by ADCINTSOCSEL2 register)
     *      9: 0 - SW trig only (overridden by ADCINTSOCSEL2 register)
     *      10: 0 - SW trig only (overridden by ADCINTSOCSEL2 register)
     *      11: 0 - SW trig only (overridden by ADCINTSOCSEL2 register)
     *      12: 0 - SW trig only (overridden by ADCINTSOCSEL2 register)
     *      13: 0 - SW trig only (overridden by ADCINTSOCSEL2 register)
     *      14: 0 - SW trig only (overridden by ADCINTSOCSEL2 register)
     *
     *      ADCSOCxCTL.CHSEL:
     *      SOC0: A0/B0 pair... 0x0
     *      SOC2: A1/B1 pair... 0x01
     *      SOC4: A2/B2 pair... 0x02
     *      SOC6: A3/B3 pair... 0x03
     *      SOC8: A4/B4 pair... 0x04
     *      SOC10: A7/B7 pair... 0x07
     *      SOC12: A6/B6 pair... 0x06
     *      SOC14: A5... 0x05
     *
     *      ADCSOCxCTL.ACQPS = 0x06... fastest/smallest sample window: 7 clock cycles
     *
     *  ADC interrupt ADCIN1: EOC6, ISR to update PWM's for all 4 control loops
     *
     *  ADC interrupt ADCINT2: EOC14, transfer low priority ADC data to RAM array, NOT continuous
     *
     *      ADCINT1N2:
     *          INT1CONT = 1, new interrupt pulse will begin new INT
     *          INT1E = 1, enable
     *          INT1SEL = 0x06, EOC6
     *          INT2CONT = 0, no further pulses until cleared
     *          INT2E = 1, enable
     *          INT2SEL = 0x0E, EOC14
     *
     *  ADC settings:
     *      Internal BG ref
     *      INT pulse at end of conversion (INTPULSEPOS = 1)
     *      Enable temp sensor
     *      Allow overlap of sample & conversion... NOTE on page 41 of ADC user guide suggests there is a 1st sample issue...?
     *
     *  ADCCTL1
     *      xADCENABLE = 1
     *      xADCPWDN = 1
     *      xADCBGPWD = 1
     *      xADCREFPWD = 1
     *      xADCREFSEL = 0
     *      xINTPULSEPOS = 1
     *      xTEMPCONV = 1
     *
     *  Sequence of events: EPWM1 SOCA -> SOC0-6 (V/I of SMPS's) -> EOC6 -> ADCINT1 -> control loop
     *                                                                              -> SOC8-14 -> EOC14 -> ADCINT2
     *
     *  ADC testing prior to PWMs configured:
     *  -SW trigger for SOC0-6
     *  -ADCINT1... toggle yellow LED and save ADC data to array
     *  -ADCINT2... toggle red LED and save ADC data to array
     *
     *
     *
     */

    AdcRegs.ADCCTL1.bit.ADCBGPWD  = 1;          // Power ADC BG
    AdcRegs.ADCCTL1.bit.ADCREFPWD = 1;          // Power reference
    AdcRegs.ADCCTL1.bit.ADCPWDN   = 1;          // Power ADC
    AdcRegs.ADCCTL1.bit.ADCENABLE = 1;          // Enable ADC
    AdcRegs.ADCCTL1.bit.ADCREFSEL = 0;          // Select interal BG
    AdcRegs.ADCCTL1.bit.TEMPCONV = 1;           // Enable temp sensor
    AdcRegs.ADCCTL1.bit.INTPULSEPOS = 1;        // INT pulse at end of conversion

    AdcRegs.INTSEL1N2.bit.INT1CONT = 1;         // New interrupt pulse will begin new INT
    AdcRegs.INTSEL1N2.bit.INT1E = 1;            // INT enable
    //AdcRegs.INTSEL1N2.bit.INT1SEL = 0x06;       // EOC6 when in simultaneous mode
    AdcRegs.INTSEL1N2.bit.INT1SEL = 0x07;       // EOC7 when not in simultaneous mode
    //AdcRegs.INTSEL1N2.bit.INT1SEL = 0x0E;       // EOC14, the last round robin conversion
    AdcRegs.INTSEL1N2.bit.INT2CONT = 1;         // No further INT pulses until cleared
    AdcRegs.INTSEL1N2.bit.INT2E = 1;            // INT enable
    AdcRegs.INTSEL1N2.bit.INT2SEL = 0x0E;       // EOC14, the last round robin conversion
    //AdcRegs.INTSEL1N2.bit.INT2SEL = 0x08;       // EOC8

    AdcRegs.SOCPRICTL.bit.SOCPRIORITY = 0x08;   // SOC0-7 are high priority, SOC8+ are round robin
    //AdcRegs.SOCPRICTL.bit.SOCPRIORITY = 0;      // All SOCs are round robin

    /*AdcRegs.ADCSAMPLEMODE.bit.SIMULEN0 = 1;     // SOC0/1 for A0/B0
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN2 = 1;     // SOC2/3 for A1/B1
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN4 = 1;     // SOC4/5 for A2/B2
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN6 = 1;     // SOC6/7 for A3/B3
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN8 = 1;     // SOC8/9 for A4/B4
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN10 = 1;    // SOC10/11 for A7/B7
    AdcRegs.ADCSAMPLEMODE.bit.SIMULEN12 = 1;*/    // SOC12/13 for A6/B6

    AdcRegs.ADCSAMPLEMODE.all = 0;              // No simultaneous samples

    AdcRegs.ADCINTSOCSEL1.all = 0;              // No ADCINT trigger, use TRIGSEL to define trigger
    //AdcRegs.ADCINTSOCSEL2.all = 0;              // No ADCINT trigger, use TRIGSeL to define trigger

    AdcRegs.ADCINTSOCSEL2.bit.SOC8 = 1;        // ADCINT1
    AdcRegs.ADCINTSOCSEL2.bit.SOC9 = 1;        // ADCINT1
    AdcRegs.ADCINTSOCSEL2.bit.SOC10 = 1;       // ADCINT1
    AdcRegs.ADCINTSOCSEL2.bit.SOC11 = 1;       // ADCINT1
    AdcRegs.ADCINTSOCSEL2.bit.SOC12 = 1;       // ADCINT1
    AdcRegs.ADCINTSOCSEL2.bit.SOC13 = 1;       // ADCINT1
    AdcRegs.ADCINTSOCSEL2.bit.SOC14 = 1;       // ADCINT1

    /*AdcRegs.ADCINTSOCSEL2.bit.SOC8 = 0;
    AdcRegs.ADCINTSOCSEL2.bit.SOC9 = 0;
    AdcRegs.ADCINTSOCSEL2.bit.SOC10 = 0;
    AdcRegs.ADCINTSOCSEL2.bit.SOC11 = 0;
    AdcRegs.ADCINTSOCSEL2.bit.SOC12 = 0;
    AdcRegs.ADCINTSOCSEL2.bit.SOC13 = 0;
    AdcRegs.ADCINTSOCSEL2.bit.SOC14 = 0;*/

    AdcRegs.ADCINTSOCSEL2.bit.SOC15 = 0;       // No ADCINT trigger

    /*AdcRegs.ADCSOC0CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0;          // A0/B0
    //AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0x05;     // ePWM1 ADCSOCA
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC2CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC2CTL.bit.CHSEL = 1;          // A1/B1
    //AdcRegs.ADCSOC2CTL.bit.TRIGSEL = 0x05;     // ePWM1 ADCSOCA
    AdcRegs.ADCSOC2CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC4CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC4CTL.bit.CHSEL = 2;          // A2/B2
    //AdcRegs.ADCSOC4CTL.bit.TRIGSEL = 0x05;     // ePWM1 ADCSOCA
    AdcRegs.ADCSOC4CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC6CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC6CTL.bit.CHSEL = 3;          // A3/B3
    //AdcRegs.ADCSOC6CTL.bit.TRIGSEL = 0x05;     // ePWM1 ADCSOCA
    AdcRegs.ADCSOC6CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC8CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC8CTL.bit.CHSEL = 4;          // A4/B4
    AdcRegs.ADCSOC8CTL.bit.TRIGSEL = 0;        // SW trig only (overridden by ADCINTSOCSEL2 register)

    AdcRegs.ADCSOC10CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC10CTL.bit.CHSEL = 7;          // A7/B7
    AdcRegs.ADCSOC10CTL.bit.TRIGSEL = 0;        // SW trig only (overridden by ADCINTSOCSEL2 register)

    AdcRegs.ADCSOC12CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC12CTL.bit.CHSEL = 6;          // A6/B6
    AdcRegs.ADCSOC12CTL.bit.TRIGSEL = 0;        // SW trig only (overridden by ADCINTSOCSEL2 register)

    AdcRegs.ADCSOC14CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC14CTL.bit.CHSEL = 5;          // A5
    AdcRegs.ADCSOC14CTL.bit.TRIGSEL = 0;*/        // SW trig only (overridden by ADCINTSOCSEL2 register)

    AdcRegs.ADCSOC0CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC0CTL.bit.CHSEL = 0;          // A0
    AdcRegs.ADCSOC0CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC1CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC1CTL.bit.CHSEL = 0x08;       // B0
    AdcRegs.ADCSOC1CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC2CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC2CTL.bit.CHSEL = 0x01;       // A1
    AdcRegs.ADCSOC2CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC3CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC3CTL.bit.CHSEL = 0x09;       // B1
    AdcRegs.ADCSOC3CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC4CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC4CTL.bit.CHSEL = 0x02;       // A2
    AdcRegs.ADCSOC4CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC5CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC5CTL.bit.CHSEL = 0x0A;       // B2
    AdcRegs.ADCSOC5CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC6CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC6CTL.bit.CHSEL = 0x03;       // A3
    AdcRegs.ADCSOC6CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC7CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC7CTL.bit.CHSEL = 0x0B;       // B3
    AdcRegs.ADCSOC7CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC8CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC8CTL.bit.CHSEL = 0x04;       // A4
    AdcRegs.ADCSOC8CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC9CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC9CTL.bit.CHSEL = 0x0C;       // B4
    AdcRegs.ADCSOC9CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC10CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC10CTL.bit.CHSEL = 0x0F;       // B7
    AdcRegs.ADCSOC10CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC11CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC11CTL.bit.CHSEL = 0x06;       // A6
    AdcRegs.ADCSOC11CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC12CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC12CTL.bit.CHSEL = 0x0E;       // B6
    AdcRegs.ADCSOC12CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC13CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC13CTL.bit.CHSEL = 0x07;       // A7
    AdcRegs.ADCSOC13CTL.bit.TRIGSEL = 0;        // SW trigger

    AdcRegs.ADCSOC14CTL.bit.ACQPS = 0x06;       // 7 ADCCLKs
    AdcRegs.ADCSOC14CTL.bit.CHSEL = 0x05;       // A5
    AdcRegs.ADCSOC14CTL.bit.TRIGSEL = 0;        // SW trigger

    EDIS;
}

#ifdef I2C_ENABLE
//
// I2CA_Init - Initialize I2C
//
void
I2CA_Init(void)
{
    I2caRegs.I2CSAR = 0x0050;       // Slave address - EEPROM control code

    I2caRegs.I2CPSC.all = 6;        // Prescaler - need 7-12 Mhz on module clk
    I2caRegs.I2CCLKL = 10;          // NOTE: must be non zero
    I2caRegs.I2CCLKH = 5;           // NOTE: must be non zero
    I2caRegs.I2CIER.all = 0x24;     // Enable SCD & ARDY interrupts

    I2caRegs.I2CMDR.all = 0x0020;   // Take I2C out of reset
                                    // Stop I2C when suspended

    I2caRegs.I2CFFTX.all = 0x6000;  // Enable FIFO mode and TXFIFO
    I2caRegs.I2CFFRX.all = 0x2040;  // Enable RXFIFO, clear RXFFINT,

    return;
}

//
// I2CA_WriteData -
//
Uint16
I2CA_WriteData(struct I2CMSG *msg)
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
    // MsgBuffer + Address
    //
    I2caRegs.I2CCNT = msg->NumOfBytes+2;

    //
    // Setup data to send
    //
    I2caRegs.I2CDXR = msg->MemoryHighAddr;
    I2caRegs.I2CDXR = msg->MemoryLowAddr;

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
Uint16
I2CA_ReadData(struct I2CMSG *msg)
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

        I2caRegs.I2CCNT = 2;
        I2caRegs.I2CDXR = msg->MemoryHighAddr;
        I2caRegs.I2CDXR = msg->MemoryLowAddr;
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

                for(i=0; i < I2C_NUMBYTES; i++)
                {
                    CurrentMsgPtr->MsgBuffer[i] = I2caRegs.I2CDRR;
                }

                {
                    //
                    // Check received data
                    //
                    for(i=0; i < I2C_NUMBYTES; i++)
                    {
                        if(I2cMsgIn1.MsgBuffer[i] == I2cMsgOut1.MsgBuffer[i])
                        {
                            PassCount++;
                        }
                        else
                        {
                            FailCount++;
                        }
                    }

                    if(PassCount == I2C_NUMBYTES)
                    {
                        pass();
                    }

                    else
                    {
                        fail();
                    }
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

#endif

__interrupt void
adcint1_isr(void)
{
    // save ADCs to arrays: A0/B0, A1/B1, A2/B2, A3/B3

    adc_array_A0[adc_index] = AdcResult.ADCRESULT0;
    adc_array_B0[adc_index] = AdcResult.ADCRESULT1;
    adc_array_A1[adc_index] = AdcResult.ADCRESULT2;
    adc_array_B1[adc_index] = AdcResult.ADCRESULT3;
    adc_array_A2[adc_index] = AdcResult.ADCRESULT4;
    adc_array_B2[adc_index] = AdcResult.ADCRESULT5;
    adc_array_A3[adc_index] = AdcResult.ADCRESULT6;
    adc_array_B3[adc_index] = AdcResult.ADCRESULT7;

    /*adc_array_A0[adc_index] = AdcResult.ADCRESULT0;
    adc_array_A1[adc_index] = AdcResult.ADCRESULT1;
    adc_array_A2[adc_index] = AdcResult.ADCRESULT2;
    adc_array_A3[adc_index] = AdcResult.ADCRESULT3;
    adc_array_A4[adc_index] = AdcResult.ADCRESULT4;
    adc_array_A5[adc_index] = AdcResult.ADCRESULT5;
    adc_array_A6[adc_index] = AdcResult.ADCRESULT6;
    adc_array_A7[adc_index] = AdcResult.ADCRESULT7;
    adc_array_B0[adc_index] = AdcResult.ADCRESULT8;
    adc_array_B1[adc_index] = AdcResult.ADCRESULT9;
    adc_array_B2[adc_index] = AdcResult.ADCRESULT10;
    adc_array_B3[adc_index] = AdcResult.ADCRESULT11;
    adc_array_B4[adc_index] = AdcResult.ADCRESULT12;
    adc_array_B6[adc_index] = AdcResult.ADCRESULT13;
    adc_array_B7[adc_index] = AdcResult.ADCRESULT14;*/

    /*if (adc_index >= (ADC_ARRAY_SIZE-1))
    {
        adc_index = 0;
    }
    else
    {
        adc_index++;
    }*/

    set_LED(LED_YELLOW, LED_TOGGLE);

    AdcRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

__interrupt void
adcint2_isr(void)
{
    // save ADCs to arrays: A4/B4, A5, A6/B6, A7/B7

    adc_array_A4[adc_index] = AdcResult.ADCRESULT8;
    adc_array_B4[adc_index] = AdcResult.ADCRESULT9;
    adc_array_A7[adc_index] = AdcResult.ADCRESULT10;
    adc_array_B7[adc_index] = AdcResult.ADCRESULT11;
    adc_array_A6[adc_index] = AdcResult.ADCRESULT12;
    adc_array_B6[adc_index] = AdcResult.ADCRESULT13;
    adc_array_A5[adc_index] = AdcResult.ADCRESULT14;

    if (adc_index >= (ADC_ARRAY_SIZE-1))
    {
        adc_index = 0;
    }
    else
    {
        adc_index++;
    }

    set_LED(LED_RED, LED_TOGGLE);

    AdcRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
#endif

//
// cla1_isr2 - This interrupt occurs when CLA Task 2 completes
//
#ifdef CLA_ENABLE
__interrupt void 
cla1_isr2()
{
    //
    // Clear ADCINT2 flag reinitialize for next SOC
    //
    AdcRegs.ADCINTFLGCLR.bit.ADCINT2 = 1;    
    PieCtrlRegs.PIEACK.all = 0xFFFF;
}
#endif
//
// End of File
//

