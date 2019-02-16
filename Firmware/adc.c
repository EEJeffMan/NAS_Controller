/*
 * adc.c
 *
 *  Created on: Feb 16, 2019
 *      Author: Jeff
 */

#include "DSP2803x_Device.h"     // DSP2803x Headerfile Include File
#include "DSP2803x_Examples.h"   // DSP2803x Examples Include File
#include "adc.h"
#include "main.h"
#include "leds.h"

#define ADC_ARRAY_SIZE  20

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

void init_adc()
{
    unsigned int i;

    EALLOW;
    PieVectTable.ADCINT1 = &adcint1_isr;
    PieVectTable.ADCINT2 = &adcint2_isr;
    EDIS;

    InitAdc();         // For this example, init the ADC
    InitAdcAio();
    AdcOffsetSelfCal();

    // ADCINT1: INT1.1
    // ADCINT2: INT1.2
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;  // Enable INT 1.1 in the PIE
    PieCtrlRegs.PIEIER1.bit.INTx2 = 1;  // Enable INT 1.2 in the PIE

    IER |= M_INT1;      // Enable CPU INT1 for ADCINT1,ADCINT2,ADCINT9,TripZone

    configure_adc();

    //
    // Wait for ADC interrupt?
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
}


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

//end of file.
