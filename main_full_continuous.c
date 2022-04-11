#include <msp430.h>

#include "display_utils.h"

// Display Library from Alex with some edit

/**
 * main.c
 */

// Other notes

// There are serval ports we dont want to use P1.0-1.3 and P1.6 should be reserved.
// If needed, we can disconnect the jumper on P1.0 and 1.6 to get 2 extra ports.

// P1.3 is reserved for the button

/* global variable declaration */
int acc;
// this allows to figure out wether temperature or pressure

int main(void) {
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
    P2SEL &= ~0xC0; // To use P2.6 and P2.7

    // We start by setting the button interupt function

    P1DIR = 0b01010001; //Set pin direction

    P1OUT = BIT3 + BIT0; // set Pins P1.3 to high.  This will be how we want everything to start and
    // we trigger the system when we press (ground P1.3)
    //P1.0 is set so that we get a red light

    P1REN = 0b00001000; //enable pull up/down resistor on P1.3 00001000
    P1IE = 0b00001000; //Enable input at P1.3 as an interrupt
    _BIS_SR(LPM4_bits + GIE); //Turn on interrupts and go into the lowest
    //power mode (the program stops here)
    //Notice the strange format of the function, it is an "intrinsic"
    //ie. not part of C; it is specific to this chipset
}

// We also want a interrupt function after we press the button.
// This is achieved through the void __attribute__

// We also need something to switch between pressure and temperature when pressing
// P1.3 button, likely achieved through a extra accumulator. (see the changing led bit)

void __attribute__((interrupt(PORT1_VECTOR))) PORT1_ISR(void) {

    P1DIR = 0b01000111; //set all P1 pins for output except P1.3

    int temp; // Define variable for temperature measurement
    float pressure; // Define variable for pressure measurement (in psi)
    int pressure_millibar; // pressure in millibars
    acc = 0;

    while (1) {
        if (acc == 0) {
            P2DIR = 0b11111111;
            acc = 1;
            P1OUT = 0b01001000;
            P1OUT = BIT3 + BIT6; // same as above, too lazy to change
            ADC10CTL0 = ADC10SHT_2 + ADC10ON;
            ADC10CTL1 = INCH_1;
            ADC10AE0 |= 2;

            int acc_continuous;
            for (acc_continuous = 0; acc_continuous < 50; acc_continuous++) {
                ADC10CTL0 |= ENC + ADC10SC;
                while (ADC10CTL1 & ADC10BUSY);
                temp = (int)(((3.2258 * ADC10MEM) - 500.0) * (1 / 10.0));
                unsigned char temp_arr[4];
                temp_arr[0] = 0;
                temp_arr[1] = 0;
                temp_arr[2] = (unsigned char)(temp / 10);
                temp_arr[3] = (unsigned char)(temp % 10);
                set_strobe(1);
                set_display_from_nums(temp_arr);
                _delay_cycles(100000);
            }

        } else if (acc == 1) {
            //continuous display, too lazy to change
            P2SEL &= ~0xC0;
            P2DIR = 0b11111111;
            P1DIR = 0b01010001;
            acc = 0;
            P1OUT = BIT6 + BIT0 + BIT3 + BIT4;
            ADC10CTL0 = ADC10SHT_2 + ADC10ON;
            ADC10CTL1 = INCH_2;
            ADC10AE0 |= 2;

            int acc_continuous;
            for (acc_continuous = 0; acc_continuous < 50; acc_continuous++) {
                ADC10CTL0 |= ENC + ADC10SC; // Sampling and conversion start
                while (ADC10CTL1 & ADC10BUSY); // ADC10BUSY?
                pressure = ((14.696 + (ADC10MEM - 102.3) * (25.0 / 4092.0)));
                pressure_millibar = (int)(68.9 * pressure);
                unsigned char pressure_arr[4];
                pressure_arr[0] = (unsigned char)(pressure_millibar / 1000);
                pressure_arr[1] = (unsigned char)((pressure_millibar % 1000) / 100);
                pressure_arr[2] = (unsigned char)((pressure_millibar % 100) / 10);
                pressure_arr[3] = (unsigned char)(pressure_millibar % 10);
                set_strobe(1);
                set_display_from_nums(pressure_arr);
                _delay_cycles(100000);

            }
        }
    }

    P1IFG &= ~8; // Done Interrupt command

}
