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
#include "leds.h"
#include "smps.h"
#include "task_scheduler.h"
#include "main.h"
#include "gpio.h"
#include "uart.h"
#include "i2c.h"
#include "adc.h"

//
// Prototype statements
//
void init();

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

//
// These are defined by the linker file
//

// Used for running BackGround in flash, and ISR in RAM
//extern Uint16 *IQfuncsLoadStart, *IQfuncsLoadEnd, *IQfuncsRunStart;

extern Uint16 Cla1funcsLoadStart;
extern Uint16 Cla1funcsLoadSize;
extern Uint16 Cla1funcsRunStart;


//
// Main
//
void main(void)
{
    init();

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

void init()
{
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

#ifdef I2C_ENABLE
    I2CA_Init();
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
    init_adc();
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

    EINT;                               // Enable Global interrupt INTM
    ERTM;                              // Enable Global realtime interrupt DBGM

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
    init_smps();
#endif

#ifdef TASK_SCHEDULER_ENABLE
    EALLOW;
    // Timing sync for background loops
    // Timer period definitions found in PeripheralHeaderIncludes.h
    CpuTimer0Regs.PRD.all = mSec5;      // A tasks
    CpuTimer1Regs.PRD.all = mSec50;     // B tasks

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

    init_uart();
    //uart
#endif

}

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

