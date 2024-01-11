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
#include "packet_gen.h"
/* Standard demo includes. */
#include "partest.h"

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
bool txbit;
bool recvd;
int tx_buffer[TXLEN] = { 0 };
int tx_counter = 0;
bool txflag = 0;

bool enableflag = 0;

#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer_a_bitbuf() {
    puts("Hello, world14\n");
    xQueueReceive(queue, &recvd, pdMS_TO_TICKS(10));
    if(tx_counter < TXLEN){
        if (recvd == 1){
            puts("Hello, world12\n");
            TA0CCR0 = (CPU_FREQ/400000) -2 ;
            P1OUT ^= BIT2;  // Toggle P1.6 to generate PWM signal
        }
        else if(recvd == 0){
            puts("Hello, world13\n");
            TA0CCR0 = (CPU_FREQ/200000) -2 ;
            P1OUT ^= BIT2;  // Toggle P1.6 to generate PWM signal
        }

        tx_counter++;
    }
    else{
         txflag = 1;
         tx_counter = 0;
         TA0CCR0 = (CPU_FREQ/100000) -2 ;
         xQueueReset(queue);
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
    uint32_t mask = 1 << 31;

    // Iterate through each bit
    int i = 0;
    for (i = 0; i < 32; i++) {
        // Check if the bit is set or not
        txbit = num & mask;
        xQueueSend(queue, &txbit, 0);
        // Shift the mask to the right for the next bit
        mask >>= 1;
    }
}

void delay_ms(int ms) {

    while (ms) {
        __delay_cycles(CYCLES_PER_MS);
        ms--;
    }
}

int main_blinky(void) {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer


//    LogInfo("hello");
    // Configure GPIO
    P1OUT &= ~BIT2;                 // Clear P1.0 output latch for a defined power-on state

    P1DIR |= BIT2;                  // Set P1.0 to output direction

    PM5CTL0 &= ~LOCKLPM5;           // Disable the GPIO power-on default high-impedance mode
                                    // to activate previously configured port settings

    set_cpu_freq();

    queue = xQueueCreate(TXLEN, ITEM_SIZE);


    static uint8_t message[buffer_size(PAYLOADSIZE+2, HEADER_LEN)*4] = {0};  // include 10 header bytes
    static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // initialize the buffer
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];



    while (1) {
        txflag = 0;
//        puts("Hello, world1\n");
        /* generate new data */
        generate_data(tx_payload_buffer, PAYLOADSIZE, true);
//        puts("Hello, world2\n");
        /* add header (10 byte) to packet */
        add_header(&message[0], seq, header_tmplate);
//        puts("Hello, world4\n");
        /* add payload to packet */
        memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);
//        puts("Hello, world5\n");
        xQueueReset(queue);
        puts("Hello, world6\n");
        // printf("Packet start \n");
        /* casting for 32-bit fifo */
        uint8_t i=0;
        for (i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
            puts("Hello, world7\n");
            buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
            // printf("%x \n",buffer[i]);
            uint32_to_binary(buffer[i]);
        }
        puts("Hello, world8\n");
        // printf("Packet end \n");

        /* put the data to FIFO */
        // ESP_LOGI("debug", "counter %d\n", counter);
        // for (int i = 0; i<TXLEN; i++){
        //     printf("%x", tx_buffer[i]);
        // }
        // printf("\n");
        seq++;
        tx_counter = 0;
        // xTaskCreate(&timer, "task", 8192,(void *)queue, 1, &task_handle_1);

        while(txflag == 0){
            puts("Hello, world9\n");
            if (enableflag == 0){
                puts("Hello, world10\n");
                __enable_interrupt();       // Enable global interrupts
                TA0CCTL0 = CCIE;
                TA0CTL = TASSEL_2 | TACLR | MC_1;
                enableflag = 1;
            }
            if(txflag == 1){
                puts("txdone");
                break;
            }
        }
//        while (tx_counter < TXLEN)
//        {
//            puts("Hello, world9\n");
//            __enable_interrupt();       // Enable global interrupts
//            TA0CCTL0 = CCIE;
//            TA0CTL = TASSEL_2 | TACLR | MC_1;
//
////            tx_counter++;
//        }
//        if (tx_counter == TXLEN)
//        {
//            tx_counter = 0;
//            xQueueReset(queue);
            // esp_timer_stop(timer_handler);
            // esp_timer_delete(timer_handler);
//            puts("tx completed\n");
//            TA0CTL &= ~(TASSEL_2 | TACLR | MC_1);  // Stop the timer
//            TA0CCTL0 &= ~CCIE; // Disable interrupts
//            __disable_interrupt();

            // gpio_set_level(GPIO_NUM_2, 0);
//        }

        // vTaskDelay(TX_DURATION/portTICK_PERIOD_MS);
        // delay_ms(TX_DURATION);
//        vTaskDelete(task_handle_1);
    }

    while(1);
    return 0;
}

