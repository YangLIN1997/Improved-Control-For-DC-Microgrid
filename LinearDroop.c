#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include "math.h"
#include "IQmathLib.h"

unsigned int c = 0;
Uint16 Voltage[2];
Uint16 Current[2];
Uint16 period = 500;
Uint16 duty1 = 0, duty2 = 0;
float32 volt[2], curr[2];
float32 Vpre1 = 0, Vpre2 = 0;
float32 Vref1 = 0, Vref2 = 0;
float32 u1[2], e1[2];
float32 u2[2], e2[2];
float32 x;
float32 y;
float32 result;

interrupt void adc_isr(void);
void EPwmSet(void);
void ADCSet(void);

void main(void) {
  // Step 1. Initialize System Control:
  // PLL, WatchDog, enable Peripheral Clocks
  // This example function is found in the DSP2833x_SysCtrl.c file.
  InitSysCtrl();
  EALLOW;#
  if (CPU_FRQ_150MHZ) // Default - 150 MHz SYSCLKOUT
    # define ADC_MODCLK 0x3 // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 150/(2*3)   = 25.0 MHz
  # endif#
  if (CPU_FRQ_100MHZ)# define ADC_MODCLK 0x2 // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 100/(2*2)   = 25.0 MHz
  # endif
  EDIS;

  // Define ADCCLK clock frequency ( less than or equal to 25 MHz )
  // Assuming InitSysCtrl() has set SYSCLKOUT to 150 MHz
  EALLOW;
  SysCtrlRegs.HISPCP.all = ADC_MODCLK;
  EDIS;

  // Step 2. Initialize GPIO:
  // This example function is found in the DSP2833x_Gpio.c file and
  // illustrates how to set the GPIO to it's default state.
  // InitGpio();  // Skipped for this example

  // For this case just init GPIO pins for ePWM1, ePWM2, ePWM3
  // These functions are in the DSP2833x_EPwm.c file
  InitEPwm1Gpio();
  InitEPwm2Gpio();
  InitEPwm3Gpio();
  //  InitEPwm4Gpio();

  // Step 3. Clear all interrupts and initialize PIE vector table:
  // Disable CPU interrupts
  DINT;

  // Initialize the PIE control registers to their default state.
  // The default state is all PIE interrupts disabled and flags
  // are cleared.
  // This function is found in the DSP2833x_PieCtrl.c file.
  InitPieCtrl();

  // Disable CPU interrupts and clear all CPU interrupt flags:
  IER = 0x0000;
  IFR = 0x0000;

  // Initialize the PIE vector table with pointers to the shell Interrupt
  // Service Routines (ISR).
  // This will populate the entire table, even if the interrupt
  // is not used in this example.  This is useful for debug purposes.
  // The shell ISR routines are found in DSP2833x_DefaultIsr.c.
  // This function is found in DSP2833x_PieVect.c.
  InitPieVectTable();

  EALLOW; // This is needed to write to EALLOW protected register
  PieVectTable.ADCINT = & adc_isr;
  EDIS; // This is needed to disable write to EALLOW protected registers

  EALLOW;
  SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 0;
  EDIS;

  // Enable ADCINT in PIE
  PieCtrlRegs.PIEIER1.bit.INTx6 = 1;
  IER |= M_INT1; // Enable CPU Interrupt 1
  EINT; // Enable Global interrupt INTM
  ERTM; // Enable Global realtime interrupt DBGM

  InitAdc(); // For this example, init the ADC

  ADCSet(); //setup adc
  EPwmSet(); //setup pwm

  EALLOW;
  SysCtrlRegs.PCLKCR0.bit.TBCLKSYNC = 1;
  EDIS;

  // Step 5. User specific code, enable interrupts:

  // Enable CPU INT3 which is connected to EPWM1-3 INT:
  IER |= M_INT3;

  // Enable EPWM INTn in the PIE: Group 3 interrupt 1-3
  PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
  PieCtrlRegs.PIEIER3.bit.INTx2 = 1;
  PieCtrlRegs.PIEIER3.bit.INTx3 = 1;

  // Enable global Interrupts and higher priority real-time debug events:
  EINT; // Enable Global interrupt INTM
  ERTM; // Enable Global realtime interrupt DBGM

  // Step 6. IDLE loop. Just sit and loop forever (optional):

  while (1) {

  }
}

interrupt void adc_isr(void) {

  Voltage[0] = AdcRegs.ADCRESULT0 >> 4;
  Current[0] = AdcRegs.ADCRESULT1 >> 4;
  Voltage[1] = AdcRegs.ADCRESULT2 >> 4;
  Current[1] = AdcRegs.ADCRESULT3 >> 4;

  volt[0] = Voltage[0] * 0.00791;
  volt[1] = Voltage[1] * 0.00805;
  curr[0] = Current[1] * 0.00188 - 1.2;
  curr[1] = Current[1] * 0.0020 - 1.2;

  Vref1 = 10 - curr[0] * 0.5;
  if ((Vref1 - Vpre1) > 0.01 || (Vref1 - Vpre1) < 0.01) {
    Vref1 = (Vref1 - Vpre1) * 0.1 + Vpre1;
  }
  Vpre1 = Vref1;
  e1[0] = Vref1 - volt[0];
  //u1[0]=1.983*u1[1]-0.9835*u1[2]+0.7109*e1[0]-1.418*e1[1]+0.7073*e1[2];
  if (u1[1] < 1 && u1[1] > 0) {
    u1[0] = 1.9385 * u1[1] + 0.9233 * e1[0] - 0.9163 * e1[1];
  } else {
    u1[0] = 0.9385 * u1[1] + 0.9233 * e1[0] - 0.9163 * e1[1];
  }
  e1[1] = e1[0];
  u1[1] = u1[0];
  if (u1[0] > 1) {
    u1[0] = 1;
  } else if (u1[0] < 0) {
    u1[0] = 0;
  }
  EPwm1Regs.CMPA.half.CMPA = period * u1[0];

  Vref2 = 10 - curr[1] * 0.5;
  if ((Vref2 - Vpre2) > 0.01 || (Vref2 - Vpre2) < 0.01) {
    Vref2 = (Vref2 - Vpre2) * 0.1 + Vpre2;
  }
  Vpre2 = Vref2;
  e2[0] = Vref2 - volt[1];
  if (u2[1] < 1 && u2[1] > 0) {
    u2[0] = 1.9385 * u2[1] + 0.9233 * e2[0] - 0.9163 * e2[1];
  } else {
    u2[0] = 0.9385 * u2[1] + 0.9233 * e2[0] - 0.9163 * e2[1];
  }
  e2[1] = e2[0];
  u2[1] = u2[0];
  if (u2[0] > 1) {
    u2[0] = 1;
  } else if (u2[0] < 0) {
    u2[0] = 0;
  }

  EPwm2Regs.CMPA.half.CMPA = period * u2[0];

  // Reinitialize for next ADC sequence
  AdcRegs.ADCTRL2.bit.RST_SEQ1 = 1; // Reset SEQ1
  AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1; // Clear INT SEQ1 bit
  AdcRegs.ADCTRL2.bit.RST_SEQ2 = 1; // Reset SEQ1
  AdcRegs.ADCST.bit.INT_SEQ2_CLR = 1; // Clear INT SEQ1 bit
  PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; // Acknowledge interrupt to PIE

  return;
}

void ADCSet(void) {

  AdcRegs.ADCMAXCONV.all = 0x00F; // Setup 2 conv's on SEQ1
  AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0x0; // Setup ADCINA3 as 1st SEQ1 conv.
  AdcRegs.ADCCHSELSEQ1.bit.CONV01 = 0x1; // Setup ADCINA2 as 2nd SEQ1 conv.
  AdcRegs.ADCCHSELSEQ1.bit.CONV02 = 0x2; // Setup ADCINA1 as 3rd SEQ1 conv.
  AdcRegs.ADCCHSELSEQ1.bit.CONV03 = 0x3; // Setup ADCINA4 as 4th SEQ1 conv.
  AdcRegs.ADCCHSELSEQ2.bit.CONV04 = 0x4; // Setup ADCINA0 as 5th SEQ2 conv.
  AdcRegs.ADCCHSELSEQ2.bit.CONV05 = 0x5; // Setup ADCINA5 as 6th SEQ2 conv.
  AdcRegs.ADCCHSELSEQ2.bit.CONV06 = 0x6; // Setup ADCINA6 as 6th SEQ2 conv.
  AdcRegs.ADCCHSELSEQ2.bit.CONV07 = 0x7; // Setup ADCINA7 as 7th SEQ2 conv.

  AdcRegs.ADCTRL2.bit.EPWM_SOCA_SEQ1 = 1; // Enable SOCA from ePWM to start SEQ1
  AdcRegs.ADCTRL2.bit.INT_ENA_SEQ1 = 1; // Enable SEQ1 interrupt (every EOS)
  AdcRegs.ADCTRL2.bit.EPWM_SOCB_SEQ2 = 1; // Enable SOCB from ePWM to start SEQ2
  AdcRegs.ADCTRL2.bit.INT_ENA_SEQ2 = 1; // Enable SEQ2 interrupt (every EOS)

  // Assumes ePWM1 clock is already enabled in InitSysCtrl();
  EPwm2Regs.ETSEL.bit.SOCAEN = 1; // Enable SOC on A group
  EPwm2Regs.ETSEL.bit.SOCASEL = 1; //4;  	     // Select SOC from from CPMA on upcount
  EPwm2Regs.ETPS.bit.SOCAPRD = 1; // Generate pulse on 1st event

  //SOCB
  EPwm2Regs.ETSEL.bit.SOCBEN = 1; // Enable SOC on B group
  EPwm2Regs.ETSEL.bit.SOCBSEL = 1; //4;      	 // Select SOC from from CPMB on upcount
  EPwm2Regs.ETPS.bit.SOCBPRD = 1; // Generate pulse on 1st event

  EPwm2Regs.CMPA.half.CMPA = 0x0080; // Set compare A value
  EPwm2Regs.TBPRD = 0xFFFF; // Set period for ePWM1
  EPwm2Regs.TBCTL.bit.CTRMODE = 0; // count up and start
}

void EPwmSet(void) {

  EPwm1Regs.TBPRD = period; // Period = 1600 TBCLK counts
  EPwm1Regs.TBPHS.half.TBPHS = 0; // Set Phase register to zero
  EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN; // Symmetrical mode
  /*
	   EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP; // Count-up mode: used for asymmetric PWM
	*/
  EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Master module
  EPwm1Regs.TBCTL.bit.PRDLD = TB_SHADOW;
  EPwm1Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO; // Sync down-stream module
  EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
  EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
  EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO; // load on CTR=Zero
  EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // load on CTR=Zero
  EPwm1Regs.AQCTLA.bit.CAU = AQ_SET; // set actions for EPWM1A
  EPwm1Regs.AQCTLA.bit.CAD = AQ_CLEAR;
  EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE; // enable Dead-band module
  EPwm1Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC; // Active Hi complementary
  EPwm1Regs.DBFED = 50; // FED = 50 TBCLKs
  EPwm1Regs.DBRED = 50; // RED = 50 TBCLKs
  EPwm1Regs.CMPA.half.CMPA = duty1; // adjust duty for output EPWM1A

  EPwm2Regs.TBPRD = period; // Period = 1600 TBCLK counts
  EPwm2Regs.TBPHS.half.TBPHS = 0; // Set Phase register to zero
  EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN; // Symmetrical mode
  EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE; // Master module
  EPwm2Regs.TBCTL.bit.PRDLD = TB_SHADOW;
  EPwm2Regs.TBCTL.bit.SYNCOSEL = TB_CTR_ZERO; // Sync down-stream module
  EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
  EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
  EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO; // load on CTR=Zero
  EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // load on CTR=Zero
  EPwm2Regs.AQCTLA.bit.CAU = AQ_SET; // set actions for EPWM1A
  EPwm2Regs.AQCTLA.bit.CAD = AQ_CLEAR;
  EPwm2Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE; // enable Dead-band module
  EPwm2Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC; // Active Hi complementary
  EPwm2Regs.DBFED = 50; // FED = 50 TBCLKs
  EPwm2Regs.DBRED = 50; // RED = 50 TBCLKs
  EPwm2Regs.CMPA.half.CMPA = duty2; // adjust duty for output EPWM1A

}
