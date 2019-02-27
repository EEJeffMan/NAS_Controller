/*
 * task_scheduler.c
 *
 *  Created on: Jul 21, 2018
 *      Author: Jeff
 */

#include "task_scheduler.h"
#include "DSP28x_Project.h"
#include "leds.h"
#include "smps.h"

int VTimer0[4];                   // Virtual Timers slaved off CPU Timer 0
int VTimer1[4];                   // Virtual Timers slaved off CPU Timer 1

// Variable declarations
void (*Alpha_State_Ptr)(void);  // Base States pointer
void (*A_Task_Ptr)(void);       // State pointer A branch
void (*B_Task_Ptr)(void);       // State pointer B branch

//=================================================================================
//  STATE-MACHINE SEQUENCING AND SYNCRONIZATION
//=================================================================================

//--------------------------------- FRAMEWORK -------------------------------------
void A0(void) {
    // loop rate synchronizer for A-tasks
    if (CpuTimer0Regs.TCR.bit.TIF == 1) {
        CpuTimer0Regs.TCR.bit.TIF = 1;  // clear flag

        //-----------------------------------------------------------
        (*A_Task_Ptr)();        // jump to an A Task (A1,A2,A3,...)
        //-----------------------------------------------------------

        VTimer0[0]++;           // virtual timer 0, instance 0 (spare)
    }

    Alpha_State_Ptr = &B0;      // Comment out to allow only A tasks
}

void B0(void) {
    // loop rate synchronizer for B-tasks
    if (CpuTimer1Regs.TCR.bit.TIF == 1) {
        CpuTimer1Regs.TCR.bit.TIF = 1;              // clear flag

        //-----------------------------------------------------------
        (*B_Task_Ptr)();        // jump to a B Task (B1,B2,B3,...)
        //-----------------------------------------------------------
        VTimer1[0]++;           // virtual timer 1, instance 0 (spare)
    }

    Alpha_State_Ptr = &A0;      // Allow C state tasks
}

//=================================================================================
//  A - TASKS
//=================================================================================
//--------------------------------------------------------
void A1(void)
//--------------------------------------------------------
{

    //-------------------
    //the next time CpuTimer0 'counter' reaches Period value go to A2
    A_Task_Ptr = &A2;
    //-------------------
}

//--------------------------------------------------------
void A2(void)  // Connect Disconnect
//-----------------------------------------------------------------
{

    //-------------------
    //the next time CpuTimer0 'counter' reaches Period value go to A1
    A_Task_Ptr = &A3;
    //-------------------
}

//--------------------------------------------------------
void A3(void)
//-----------------------------------------
{

    //-----------------
    //the next time CpuTimer0 'counter' reaches Period value go to A1
    A_Task_Ptr = &A4;
    //-----------------
}

//--------------------------------------------------------
void A4(void)  // Spare
//--------------------------------------------------------
{
    //-----------------
    //the next time CpuTimer0 'counter' reaches Period value go to A1
    A_Task_Ptr = &A1;
    //-----------------
}

//=================================================================================
//  B - TASKS
//=================================================================================
//----------------------------------------
void B1(void)
//----------------------------------------
{
    EALLOW;
    AdcRegs.ADCSOCFRC1.bit.SOC0 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC1 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC2 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC3 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC4 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC5 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC6 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC7 = 1;
    /*AdcRegs.ADCSOCFRC1.bit.SOC8 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC9 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC10 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC11 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC12 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC13 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC14 = 1;*/
    EDIS;
    //-----------------
    //the next time CpuTimer1 'counter' reaches Period value go to B2
    B_Task_Ptr = &B2;
    //-----------------
}

//----------------------------------------
void B2(void) // B2
//----------------------------------------
{

   /* EALLOW;
    AdcRegs.ADCSOCFRC1.bit.SOC8 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC9 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC10 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC11 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC12 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC13 = 1;
    AdcRegs.ADCSOCFRC1.bit.SOC14 = 1;
    EDIS;*/

    set_LED(LED_GREEN, LED_TOGGLE);

    //-----------------
    //the next time CpuTimer1 'counter' reaches Period value go to B3
    B_Task_Ptr = &B3;
    //-----------------
}

//----------------------------------------
void B3(void) // B3
//----------------------------------------
{
    // after 150ms, set eload to constant current mode, and start PWM.
    //GpioDataRegs.GPACLEAR.bit.GPIO30 = 1;

    //-----------------
    //the next time CpuTimer1 'counter' reaches Period value go to B4
    B_Task_Ptr = &B1;
    //-----------------
}

