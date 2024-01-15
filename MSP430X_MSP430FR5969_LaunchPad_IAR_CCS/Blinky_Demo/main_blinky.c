/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <msp430fr5969.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Standard demo includes. */
#include "partest.h"
#include "stdarg.h"

/* Priorities at which the tasks are created. */
#define BLINK_TASK_PRIORITY     ( tskIDLE_PRIORITY + 2 )
/* The LED toggled by the Rx task. */
#define mainTASK_LED                        ( 0 )
#define CPU_FREQ_24MHZ  24000000
#define CPU_FREQ_8MHZ    8000000
#define CPU_FREQ    CPU_FREQ_24MHZ
#define BAUDRATE    2938
#define CYCLES_PER_MS (CPU_FREQ/1000)
#define TX_DURATION 200 // send a packet every 200ms (when changing baud-rate, ensure that the TX delay is larger than the transmission time)
#define RECEIVER 1352 // define the receiver board either 2500 or 1352
#define ITEM_SIZE sizeof(int)
#define TXLEN                   (buffer_size(PAYLOADSIZE, HEADER_LEN) * 8 * 4)

QueueHandle_t queue;
TaskHandle_t task_handle_1;
bool txbit;
int tx_buffer[TXLEN] = { 0 };
int tx_counter = 0;
bool txflag = 0;
bool recvd;
bool enableflag = 0;

void delay_ms(int ms) {

    while (ms) {
        __delay_cycles(CYCLES_PER_MS);
        ms--;
    }
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0 (void)
{
//  P1OUT ^= BIT3; // Toggle P1.0
    P1OUT ^= BIT2;

//  TA1CCR0 += 50000;                          // Add Offset to TACCR0
}


void freqhigh(){
    TA1CCR0 = (CPU_FREQ/(2*240000)) -1;                      // Set compare value for 200 kHz
}

void freqlow(){
    TA1CCR0 = (CPU_FREQ/(2*200000)) -1;                      // Set compare value for 200 kHz
}

void disable_signal(){
    P1OUT = 0; // Set all P1 pins low
    __disable_interrupt();
    TA1CTL &= ~(TASSEL_2 | TACLR | MC_1);
    TA1CCTL0 = ~CCIE;
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

void callback_function(void) {
//    P1OUT ^= BIT2;
    // Perform desired actions here, e.g., toggle an LED, read a sensor, etc.
    if (tx_counter < TXLEN){
        xQueueReceiveFromISR(queue,&recvd, NULL);

//        printf("%d",recvd);
        if (recvd == 1){
            TA1CCR0 = (CPU_FREQ/(2*240000)) -1;
//            P1OUT ^= BIT2;  // Toggle an LED as an example
        }
        else{
            TA1CCR0 = (CPU_FREQ/(2*200000)) -1;
//            P1OUT ^= BIT2;  // Toggle an LED as an example
        }
        tx_counter++;
    }
    else{
        disable_signal();
        tx_counter = 0;
        xQueueReset(queue);
        msp430_timer_stop();
    }
}

void msp430_timer_start_periodic(unsigned int period_us, void (*callback)(void)) {
    // Configure Timer_A0 for periodic operation (adjust as needed)
    TA0CTL = TASSEL_2 | MC_1 | ID_0| TACLR;  // SMCLK, Up mode, no divider
    TA0CCR0 = (CPU_FREQ * period_us) / 1000000 - 1;  // Set period based on SMCLK frequency
    TA0CCTL0 = CCIE;  // Enable interrupts
//    TA0CTL |= MC_1;   // Start timer

    TA1CTL = TASSEL_2 | TACLR | MC_1 ;  // SMCLK, up mode, /8 divider
    TA1CCTL0 = CCIE;                  // Enable CCR0 interrupt

    // Store callback function pointer safely
    __disable_interrupt();  // Disable interrupts temporarily
    static void (*callback_function)(void) = NULL;
    callback_function = callback;
    __enable_interrupt();  // Re-enable interrupts
}

// Interrupt service routine for Timer_A0 (adjust for other timers)
__attribute__((interrupt(TIMER0_A0_VECTOR)))
void timer_a0_isr(void) {
    if (callback_function) {
        __disable_interrupt();  // Ensure exclusive access to callback
        callback_function();    // Call user-defined callback
        __enable_interrupt();  // Restore interrupts
    }
}

void set_cpu_freq() {
//#if CPU_FREQ == CPU_FREQ_8MHZ
    // 8MHz
//    CSCTL0_H = 0xA5;
//    CSCTL1 = DCOFSEL_6;
//    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
//    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;
//    CSCTL0_H = 0;

    // 24 MHz

#if CPU_FREQ == CPU_FREQ_24MHZ
    FRCTL0 = 0xA500 | ((1) << 4);
    CSCTL0_H = 0xA5;
    CSCTL1 = DCOFSEL_6 | DCORSEL;
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;
    CSCTL0_H = 0;
#endif
}

void uint32_to_binary(uint32_t num) {


    // Start from the leftmost bit (31st bit)
    uint32_t mask = 1u << 15;
    uint16_t MSB = num >> 16;
    uint16_t LSB = num & 0xFFFF;

    int i = 0;
    for (i = 0; i < 16; i++) {
        txbit = MSB & mask;             // Check if the bit is set or not
        xQueueSend(queue, &txbit, 0);
        mask >>= 1;                     // Shift the mask to the right for the next bit
    }
    mask = 1u<<15;
    for (i = 0; i < 16; i++) {
        txbit = LSB & mask;
        xQueueSend(queue, &txbit, 0);
        mask >>= 1;                     // Shift the mask to the right for the next bit
    }

}

//void timer(void* param){
//     msp430_timer_start_periodic(20, callback_function);
//     while (1);
//}

int main_blinky(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    queue = xQueueCreate(TXLEN, ITEM_SIZE);

    // Configure GPIO
    P1OUT &= ~BIT2;                 // Clear P1.0 output latch for a defined power-on state
    P1OUT &= ~BIT3;
    P1DIR |= BIT2;                  // Set P1.0 to output direction
    P1DIR |= BIT3;

    PM5CTL0 &= ~LOCKLPM5;           // Disable the GPIO power-on default high-impedance mode
                                    // to activate previously configured port settings

    set_cpu_freq();
    static uint8_t message[buffer_size(PAYLOADSIZE+2, HEADER_LEN)*4] = {0};  // include 10 header bytes
    static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // initialize the buffer
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];

    while (1) {
        txflag = 0;
        /* generate new data */
        generate_data(tx_payload_buffer, PAYLOADSIZE, true);
        /* add header (10 byte) to packet */
        add_header(&message[0], seq, header_tmplate);
        /* add payload to packet */
        memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);
        xQueueReset(queue);

        /* casting for 32-bit fifo */
        uint8_t i=0;
        for (i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
            buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            uint32_to_binary(buffer[i]);
        }

        seq++;
        tx_counter = 0;
        msp430_timer_start_periodic(100, callback_function);
        delay_ms(50);

    }

    while(1);
    return 0;
}
