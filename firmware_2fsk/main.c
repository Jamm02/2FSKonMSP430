/*
 * Author: Moteen Shah
 * Email:  mashah_b20@et.vjti.ac.in
 */

#include <msp430fr5969.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "packet_gen.h"
#include "stdarg.h"

#define CPU_FREQ_16MHZ  16000000
#define CPU_FREQ_8MHZ    8000000
#define CPU_FREQ_4MHZ    6670000
#define CPU_FREQ    CPU_FREQ_16MHZ
#define BAUDRATE    2938
#define CYCLES_PER_US (CPU_FREQ/1000000)
#define RECEIVER 1352                                // define the receiver board either 2500 or 1352
#define ITEM_SIZE sizeof(int)
#define TXLEN                   (buffer_size(PAYLOADSIZE, HEADER_LEN) * 8 * 4)

bool txbit;
uint16_t tx_counter = 0;
int arr_dump[TXLEN] = {0};
int arrdump_counter = 0;

#define MAX_SIZE TXLEN

void delay_us(int us) {

    while (us) {
        __delay_cycles(CYCLES_PER_US);
        us--;
    }
}

void disable_signal(){

    P1OUT = 0; // Set all P1 pins low
    TB0CTL &= ~(TBSSEL_2 | TBCLR | MC_1);

}

void msp430_timer_stop(void) {
    P1OUT = 0; // Set all P1 pins low
    // Disable interrupts temporarily for safe access
    __disable_interrupt();

    // Clear callback function pointer to prevent further calls
    static void (*callback_function)(void) = NULL;

    // Stop timer hardware
    TA0CTL &= ~MC_1;  // Halt timer operation

    // Clear interrupt flag to prevent unexpected interrupts
    TA0CCTL0 &= ~CCIFG;

    // Enable interrupts again
    __enable_interrupt();
}


// Interrupt service routine for Timer_A0 (adjust for other timers)
__attribute__((interrupt(TIMER0_A0_VECTOR)))
void timer_a0_isr(void) {

    if (tx_counter < TXLEN){

            if (arr_dump[tx_counter] == 0){
//                TB0CCR0 = (CPU_FREQ/(2*180000)) -1;
                  TB0CCR0 = 5;  // 1.33 Mhz
            }
            else if(arr_dump[tx_counter] == 1){
                TB0CCR0 = (CPU_FREQ/(2*220000)) -1;
                  TB0CCR0 = 4;  // 1.60 Mhz
            }
            tx_counter++;
        }
        else{
            tx_counter = 0;
            disable_signal();
            msp430_timer_stop();
        }

}

void set_cpu_freq() {

    // Clock System Setup
//     CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
//     CSCTL1 = DCOFSEL_6;                       // Set DCO to 8MHz
//     CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;  // Set SMCLK = MCLK = DCO
//                                               // ACLK = VLOCLK
//     CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers to 1
//     CSCTL0_H = 0;                             // Lock CS registers
//
////        // Clock System Setup
    FRCTL0 = FRCTLPW | NWAITS_1;
     CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
     CSCTL1 = DCORSEL | DCOFSEL_4;             // Set DCO to 16MHz
     CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
     CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers to 1
//     CSCTL4 |= LFXTDRIVE_3 | HFXTDRIVE_1 | HFFREQ_1;
//     CSCTL4 &= ~(LFXTOFF | HFXTOFF);
//     do
//     {
//       CSCTL5 &= ~(LFXTOFFG | HFXTOFFG);       // Clear XT1 and XT2 fault flag
//       SFRIFG1 &= ~OFIFG;
//     }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag
//     CSCTL0_H = 0;                             // Lock CS registers


//

//    CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
//    CSCTL1 = DCOFSEL_5;
//    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK; // Set SMCLK = MCLK = DCO,
//                                              // ACLK = VLOCLK
//    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;     // Set all dividers
//    CSCTL0_H = 0;                             // Lock CS registers

}

void uint32_to_binary(uint32_t num) {


    // Start from the leftmost bit (31st bit)
    uint32_t mask = 1u << 15;
    uint16_t MSB = num >> 16;
    uint16_t LSB = num & 0xFFFF;

    int i = 0;
    for (i = 0; i < 16; i++) {
        txbit = MSB & mask;             // Check if the bit is set or not
        arr_dump[arrdump_counter] = txbit;
        arrdump_counter++;
        mask >>= 1;                     // Shift the mask to the right for the next bit
    }
    mask = 1u<<15;
    for (i = 0; i < 16; i++) {
        txbit = LSB & mask;
        arr_dump[arrdump_counter] = txbit;
        arrdump_counter++;
        mask >>= 1;                     // Shift the mask to the right for the next bit
    }

}


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    P1SEL0 |= BIT4;             //sets pin 1.4 to special function output
    P1DIR |= BIT4;              //sets pin 1.4 for output for channel 1 PWM

    PM5CTL0 &= ~LOCKLPM5;       // Disable the GPIO power-on default high-impedance mode

    set_cpu_freq();
    static uint8_t message[buffer_size(PAYLOADSIZE+2, HEADER_LEN)*4] = {0};  // include 10 header bytes
    static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // initialize the buffer
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];


    while (1) {
        /* generate new data */
        generate_data(tx_payload_buffer, PAYLOADSIZE, true);
        /* add header (10 byte) to packet */
        add_header(&message[0], seq, header_tmplate);
        /* add payload to packet */
        memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);

        arrdump_counter = 0;
        /* casting for 32-bit fifo */
        uint8_t i=0;
        for (i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
            buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            uint32_to_binary(buffer[i]);
        }

        seq++;
        tx_counter = 0;


        __enable_interrupt();
        TA0CTL = TASSEL__SMCLK | MC_1 | TACLR;  // SMCLK, Up mode, no divider
        TA0CCR0 = (16000000/100000) - 1;  // Set period based on SMCLK frequency
        TA0CCTL0 = CCIE;  // Enable interrupts
        TA0CCTL1 = OUTMOD_7;
        TA0CTL |= MC_1;   // Start timer

        TB0CTL = TBSSEL__SMCLK | TBCLR | MC_1 ;  // SMCLK, up mode, /8 divider
        TB0CCTL1 = OUTMOD_4;

        delay_us(500);

    }
    return 0;
}

