/*
 * File:   main.c
 * Author: Emilio Soriano
 *
 * Created on 24 de noviembre de 2021, 03:50 PM
 */

#define F_CPU   16000000UL
#define BAUD    9600
#define MYUBRR  (F_CPU/16/BAUD-1)

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "myprintf.h"
#include "lcdpcf8574.h"

void USART_Init(unsigned int ubrr);
void USART_Transmit(unsigned char);
unsigned char USART_Receive();
void portsInit();
unsigned char keyboard();

QueueHandle_t Queue1 = 0;
QueueHandle_t Queue2 = 0;

void sender_task(void *p) {
    unsigned char kboard;
    unsigned char conf2;
    while (1) {
        kboard = 'N';
        while (kboard == 'N') {
            kboard = keyboard();
        }
        if (!xQueueSend(Queue1, &kboard, 10)) {
            myprintf("Failed to send %c\n", kboard);
        }
        while (!xQueueReceive(Queue2, &conf2, 10));
        vTaskDelay(10);
    }
}

void receiver_task(void *p) {
    unsigned char transmit;
    unsigned char conf1;
    unsigned char y;
    while (1) {
        if (xQueueReceive(Queue1, &transmit, 10)) {
            if (transmit != 'N') {
            USART_Transmit(transmit);
            lcd_clrscr();
            _delay_ms(200);
            }
        }
        
        y = USART_Receive();
            if (y != 1) {
                lcd_putc(y);
            }
        xQueueSend(Queue2, &conf1, 10);   
        /*vTaskDelay(20);*/
    }
}

int main(void) {
    uint8_t col = 0;
    uint8_t row = 0;
    uint8_t led = 0;
    portsInit();
    USART_Init(MYUBRR);        /* initialize serial communication */
    lcd_init(LCD_DISP_ON_BLINK);
    lcd_home();                //lcd go home
    lcd_led(led);              //set led
    lcd_gotoxy(0, 0);
    
    
    Queue1 = xQueueCreate(3, sizeof(int));
    Queue2 = xQueueCreate(3, sizeof(int));
    
    xTaskCreate(sender_task,   /* The function that implements the task. */
                (char*) "st",  /* The text name assigned to the task. */
                1024,          /* The size of the stack for the task. */
                NULL,          /* The parameter passed to the task  */
                1,             /* The priority assigned to the task. */
                NULL);         /* The task handle is not required. */
    xTaskCreate(receiver_task, /* The function that implements the task. */
                (char*) "rt",  /* The text name assigned to the task. */
                1024,          /* The size of the stack for the task. */
                NULL,          /* The parameter passed to the task  */
                2,             /* The priority assigned to the task. */
                NULL);         /* The task handle is not required. */
    
    vTaskStartScheduler();     /* Start the tasks and timer running. */
    
}

void portsInit() {
    PORTA = 0xF0;
    DDRA = 0x0F;
    MCUCR = MCUCR & 0b11101111;
}

void USART_Init(unsigned int ubrr) {
    /* Set baud rate */
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    /* Enable receiver and transmitter */
    UCSR0B = (1 << RXEN0)|(1 << TXEN0);
    /* Set frame format: 8data, 2stop bit */
    UCSR0C = UCSR0C |= (3 << UCSZ00);
    UCSR0C = UCSR0C & ~(1 << USBS0);
} 

void USART_Transmit(unsigned char data) {
    /* Wait for empty transmit buffer */
    while (!(UCSR0A & (1 << UDRE0)));
    /* Put data into buffer, sends the data */
    UDR0 = data;
}

unsigned char USART_Receive( void ) {
    /* Wait for data to be received */
    if ((UCSR0A & (1<<RXC0))) {
    /* Get and return received data from buffer */
        return UDR0;
    }
    return 1;
    }

unsigned char keyboard() {
    PORTA = PORTA & ~(1 << PA0);
    PORTA = PORTA | (1 << PA3) | (1 << PA2) | (1 << PA1);
    if ((PINA & (1 << PA7)) == 0) {
        _delay_ms(300);
        return '1';
    }
    if ((PINA & (1 << PA6)) == 0) {
        _delay_ms(300);
        return '2';
    }
    if ((PINA & (1 << PA5)) == 0) {
        _delay_ms(300);
        return '3';
    } else {
        return 'N';
    }
}
