#include <MSP430.h>
//Chris Iapicco & Bryan Regn
//Created: October 4th, 2017
//Last Updated: October 15th, 2017
int main(void)
 {
    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

    TB0CTL|=BIT1; //Enable TimerB0 capture compare interupt
    TB0CCTL1=OUTMOD_7; //Red LED reset/set mode
    TB0CCTL2=OUTMOD_7;//Green LED reset/set mode
    TB0CCTL3=OUTMOD_7;//Blue LED reset/set mode
    TB0CTL = TASSEL_2 + MC_1 + TACLR; // SMCLK, upmode

    TB0CCR0=255; //Period of PWM
    TB0CCR1=255; //Red LED duty cycle
    TB0CCR2=255; //Green LED duty cycle
    TB0CCR3=255; //Blue LED duty cycle

    P1DIR|=(BIT5+BIT4); //P1.4, P1.5 set to output TimerB0 capture compare outputs 1 and 2
    P1SEL0|=(BIT5+BIT4);//P1.4, P1.5 set to output TimerB0 capture compare outputs 1 and 2
    P3DIR|=BIT4;//P3.4 set to output TimerB0 capture compare output 3
    P3SEL0|=BIT4;//P3.4 set to output TimerB0 capture compare output 3

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;


    // Configure UART pins
    P2SEL0 &= ~(BIT0 | BIT1);//
    P2SEL1 |= BIT0 | BIT1;                  // USCI_A0 UART operation


    // Startup clock system with max DCO setting ~8MHz
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;           // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers
    CSCTL0_H = 0;                           // Lock CS registers

    // Configure USCI_A0 for UART mode
    UCA0CTLW0 = UCSWRST;                    // Put eUSCI in reset
    UCA0CTLW0 |= UCSSEL__SMCLK;             // CLK = SMCLK
    // Baud Rate calculation
    // 8000000/(16*9600) = 52.083
    // Fractional portion = 0.083
    // User's Guide Table 21-4: UCBRSx = 0x04
    // UCBRFx = int ( (52.083-52)*16) = 1
    UCA0BRW = 52;                           // 8000000/16/9600
    UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
    UCA0CTLW0 &= ~UCSWRST;                  // Initialize eUSCI
    UCA0IE |= UCRXIE;                       // Enable USCI_A0 RX interrupt

    __bis_SR_register(LPM3_bits | GIE);     // Enter LPM3, interrupts enabled
    __no_operation();                       // For debugger

    return 0;
 }


int decrement=0;//Counting register
int total_bytes=0;//Tracks total number of bytes in each packet

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=EUSCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)//UART interrupt handler
#elif defined(__GNUC__)
void __attribute__ ((interrupt(EUSCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
         if (decrement==0) //If first byte received
     {
       total_bytes = UCA0RXBUF; //Total_bytes is updated with the character recieved
       decrement = total_bytes; //The counting register is updated with the expected # of bytes
       decrement--; //The counting register is decremented
     }
    else if (decrement== (total_bytes-1)) //If Red LED duty cycle byte
    {
      TB0CCR1=UCA0RXBUF;//Red LED duty cycle is updated with byte received
      decrement--;//Counting register is decremented
    }
  else if (decrement==(total_bytes-2)) //If Green LED duty cycle byte
    {
             TB0CCR2=UCA0RXBUF; //Green LED duty cycle updated with byte received
             decrement--;//Counting register is decremented
  }
  else if (decrement==(total_bytes-3)) //If Blue LED duty cycle byte
  {
    while (!(UCA0IFG&UCTXIFG));//If the TXBUF is ready to send move on
    TB0CCR3=UCA0RXBUF;//Blue LED duty cycle updated with byte received
    UCA0TXBUF=(total_bytes-3); //Send updated amount of bytes that will be in UART command
    decrement--;//Counting register is decremented
  }

    else //All bytes after the fourth byte
  {
    while (!(UCA0IFG&UCTXIFG));  //If the TXBUF is ready to send move on
    UCA0TXBUF=UCA0RXBUF; //Send byte received
    decrement--; //Counting register is decremented
  }
}
