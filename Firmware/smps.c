/*
 * smps.c
 *
 *  Created on: Jul 16, 2018
 *      Author: Jeff Aymond
 */
#include "DSP2803x_Device.h"     // DSP2803x Headerfile Include File
#include "DSP2803x_Examples.h"   // DSP2803x Examples Include File
#include "smps.h"

void init_smps()
{
    //EALLOW must be asserted prior to function call

    /*
     * HWPWM pins:
     * GPIO0, 1A: PWM_VAUX
     * GPIO2, 2A: PWM_5V2_A
     * GPIO4, 3A: PWM_5V2_B
     * GPIO6, 4A: PWM_SEPIC
     */

    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;    // Disable pull-up on GPIO0 (EPWM1A)
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 1;   // Configure GPIO0 as EPWM1A
    GpioCtrlRegs.GPAPUD.bit.GPIO2 = 1;    // Disable pull-up on GPIO2 (EPWM2A)
    GpioCtrlRegs.GPAMUX1.bit.GPIO2 = 1;   // Configure GPIO2 as EPWM2A
    GpioCtrlRegs.GPAPUD.bit.GPIO4 = 1;    // Disable pull-up on GPIO4 (EPWM3A)
    GpioCtrlRegs.GPAMUX1.bit.GPIO4 = 1;   // Configure GPIO4 as EPWM3A
    GpioCtrlRegs.GPAPUD.bit.GPIO6 = 1;    // Disable pull-up on GPIO6 (EPWM4A)
    GpioCtrlRegs.GPAMUX1.bit.GPIO6 = 1;   // Configure GPIO6 as EPWM4A

    /*
     * ePWM registers:
     *
     * xTBPRD: 250kHz or 4us period
     * xTBPRDHR: Hi res
     * xTBPHS & TBPHSHR: phase register, ignore for now
     * xTBCTR: current count of timer
     * xTBCTL:
     *      Emulation: Free run
     *      PHSDIR: N/A, only applicable to up-down mode
     *      CLKDIV: keep default 0, /1 div
     *      ... use defaults for most...
     *      CTRMODE = 0, up count mode
     *  HRPCTL: ??
     *  CMPA / CMPB: Comparator value
     *  xCMPCTL:
     *      SHDWAMODE / SHDWBMODE: 0 = shadow mode, CMPx is updated based on LOADxMODE; 1 = immediate mode, CMPx is updated immediately
     *  CMPAHR: Hi res
     *  xAQCTLA: for EPWMxA
     *      CBD: TBCTR = CMPB while counting down
     *      CBU: TBCTR = CMPB while counting up
     *      CAD: TBCTR = CMPA while counting down
     *      CAU: TBCTR = CMPA while couting up
     *      PRD: TBCTR = TBPRD
     *      ZRO: TBCTR = Zero
     *      all above:
     *          00 = do nothing
     *          01 = clear (output low)
     *          10 = set (output high)
     *          11 = toggle
     *  AQCTLB: for EPWMxB, all same fields as above
     *  AQSFRC: action qual SW force register... not used
     *  DBCTL: N/A
     *  PCCTL: N/A
     *  TZSEL / TZCTL: N/A
     *  DCTRIPSEL: N/A
     *  DCACTL / DCBCTL: N/A
     *  xETSEL:
     *      SOCBEN: EPWMxSOCB, 0 = disabled
     *      SOCBSEL: EPWMxSOCB
     *      SOCAEN: EPWMxSOCA, 1 = enable
     *      SOCASEL: EPWMxSOCA, 0b001 = TBCTR = 0
     *      INTEN: 0 to disable INT
     *      INTSEL
     *  xETPS:
     *      SOCBPRD: 0, disabled
     *      SOCAPRD: 0b01, generate SOCA pulse on first event
     *      INTPRD: 0, disabled
     *
     *  xHRCNFG:
     *       HRLOAD: Load on CTR = 0, PRD, or either
     *       CTLMODE: Edge position controlled by CMPAHR(8) or TBPRDHR(8) (bit = 0), or TBPHSHR(8) (bit = 1)
     *       EDGMODE: MEP control of rising, falling, or both edges
     *
     */

    // VAUX

    EPwm1Regs.TBCTL.bit.FREE_SOFT = 1;      // allow free run when CPU halted
    EPwm1Regs.TBPRD = VAUX_PERIOD;
    //EPwm1Regs.TBPRDHR =
    EPwm1Regs.CMPA.half.CMPA = 0;            //set to 0 initially
    //EPwm1Regs.CMPA.half.CMPAHR = (1 << 8);  // initialize HRPWM extension
    EPwm1Regs.TBPHS.all = 0;                // not used for now
    EPwm1Regs.TBCTR = 0;

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;         // ePWM1 is the Master
    EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    EPwm1Regs.AQCTLA.bit.ZRO = AQ_SET;          // PWM toggle high/low
    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;

    // ADC SOCA
    EPwm1Regs.ETSEL.bit.SOCAEN = 1;                 // enable SOCA
    EPwm1Regs.ETSEL.bit.SOCASEL = ET_CTR_ZERO;      // TBCTR = 0

    EPwm1Regs.ETPS.bit.SOCAPRD = ET_1ST;            // SOCA on first event

    EPwm1Regs.HRCNFG.all = 0x0;
    EPwm1Regs.HRCNFG.bit.EDGMODE = HR_FEP;        // MEP control on falling edge
    EPwm1Regs.HRCNFG.bit.CTLMODE = HR_CMP;        // Duty control mode
    EPwm1Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO;   // Load on CTR = 0

    // 5V2_A

    EPwm2Regs.TBCTL.bit.FREE_SOFT = 1;      // allow free run when CPU halted
    EPwm2Regs.TBPRD = PERIOD_5V2_A;
    //EPwm2Regs.TBPRDHR =
    EPwm2Regs.CMPA.half.CMPA = 0;            //set to 0 initially
    //EPwm2Regs.CMPA.half.CMPAHR = (1 << 8);  // initialize HRPWM extension
    EPwm2Regs.TBPHS.all = 0;                // not used for now
    EPwm2Regs.TBCTR = 0;

    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE;         // ePWM1 is the Master
    EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    EPwm2Regs.AQCTLA.bit.ZRO = AQ_SET;          // PWM toggle high/low
    EPwm2Regs.AQCTLA.bit.CAU = AQ_CLEAR;

    EPwm2Regs.HRCNFG.all = 0x0;
    EPwm2Regs.HRCNFG.bit.EDGMODE = HR_FEP;        // MEP control on falling edge
    EPwm2Regs.HRCNFG.bit.CTLMODE = HR_CMP;        // Duty control mode
    EPwm2Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO;   // Load on CTR = 0

    // 5V2_B

    EPwm3Regs.TBCTL.bit.FREE_SOFT = 1;      // allow free run when CPU halted
    EPwm3Regs.TBPRD = PERIOD_5V2_B;
    //EPwm3Regs.TBPRDHR =
    EPwm3Regs.CMPA.half.CMPA = 0;            //set to 0 initially
    //EPwm3Regs.CMPA.half.CMPAHR = (1 << 8);  // initialize HRPWM extension
    EPwm3Regs.TBPHS.all = 0;                // not used for now
    EPwm3Regs.TBCTR = 0;

    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    EPwm3Regs.TBCTL.bit.PHSEN = TB_DISABLE;         // ePWM1 is the Master
    EPwm3Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    EPwm3Regs.AQCTLA.bit.ZRO = AQ_SET;          // PWM toggle high/low
    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;

    EPwm3Regs.HRCNFG.all = 0x0;
    EPwm3Regs.HRCNFG.bit.EDGMODE = HR_FEP;        // MEP control on falling edge
    EPwm3Regs.HRCNFG.bit.CTLMODE = HR_CMP;        // Duty control mode
    EPwm3Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO;   // Load on CTR = 0

    // SEPIC

    EPwm4Regs.TBCTL.bit.FREE_SOFT = 1;      // allow free run when CPU halted
    EPwm4Regs.TBPRD = SEPIC_PERIOD;
    //EPwm4Regs.TBPRDHR =
    EPwm4Regs.CMPA.half.CMPA = 0;            //set to 0 initially
    //EPwm4Regs.CMPA.half.CMPAHR = (1 << 8);  // initialize HRPWM extension
    EPwm4Regs.TBPHS.all = 0;                // not used for now
    EPwm4Regs.TBCTR = 0;

    EPwm4Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    EPwm4Regs.TBCTL.bit.PHSEN = TB_DISABLE;         // ePWM1 is the Master
    EPwm4Regs.TBCTL.bit.SYNCOSEL = TB_SYNC_DISABLE;
    EPwm4Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm4Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm4Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm4Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm4Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm4Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    EPwm4Regs.AQCTLA.bit.ZRO = AQ_SET;          // PWM toggle high/low
    EPwm4Regs.AQCTLA.bit.CAU = AQ_CLEAR;

    EPwm4Regs.HRCNFG.all = 0x0;
    EPwm4Regs.HRCNFG.bit.EDGMODE = HR_FEP;        // MEP control on falling edge
    EPwm4Regs.HRCNFG.bit.CTLMODE = HR_CMP;        // Duty control mode
    EPwm4Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO;   // Load on CTR = 0

    //EDIS should be asserted after function call
}



