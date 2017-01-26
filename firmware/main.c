/* 
 * HOUZ AUTOMAÇÃO
 *
 * Author: David M. Krepsky
 * Version: 1.0
 * Copyright (c) 2017 Houz Automação.
 */

#pragma config FOSC = HS
#pragma config WDTE = ON
#pragma config PWRTE = OFF
#pragma config BOREN = OFF
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
#include <htc.h>        /* HiTech General Include File */
#endif

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

/* Pinos dos LEDS */
#define LED_CONNECTED 0x02;
#define LED_SHORT 0x04;
#define LED_OPEN 0x08;
#define LED_CROSS 0x10;

#define LED_TRIS TRISB
#define LED_PORT PORTB

/* Buzzer */
#define BUZZER 0x02
#define BUZZER_TRIS TRISE
#define BUZZER_PORT PORTE

/* Connectors */
#define J1_TRIS TRISC
#define J1_PORT PORTC

#define J2_TRIS TRISD
#define J2_PORT PORTD

/* Oscillator */
#define _XTAL_FREQ 20000000L
#define SYS_FREQ   _XTAL_FREQ
#define FOSC       SYS_FREQ/4

/* Indicates if the cable is connected or not */
static bool connected = false;

/* Beep Beep Beep motherfucker*/
void beep() {
    for (int i = 0; i < 500; i++) {
        BUZZER_PORT = BUZZER;
        __delay_us(100);
        BUZZER_PORT &= ~BUZZER;
        __delay_us(100);
        CLRWDT();
    }
}

/* Detect cable presence */
bool check() {
    /* If any J2 input is high, that means there is a cable connected */
    J1_PORT = 0xFF;

    if (J2_PORT == 0)
        return false;
    else
        return true;
}

/* Count the number of high J2 inputs */
uint8_t one_count() {
    uint8_t cnt = 0;
    uint8_t tmp = J2_PORT;

    for (uint8_t i = 0; i < 8; i++) {
        if (tmp & 0x01)
            cnt++;

        tmp >>= 1;
    }

    return cnt;
}

/* Stupid soldering error!!11!
 * Swapped J2 bits 0, 1, 2 and 3 to 3, 2, 1 and 0.
 * Made a kick firmware fix to avoid unsoldering.
 * This part must be remove if the hardware is right.
 */
uint8_t hardware_fix() {
    uint8_t tmp = J2_PORT;

    if (J2_PORT & 0x01) {
        tmp |= 0x08;
    } else {
        tmp &= ~0x08;
    }

    if (J2_PORT & 0x02) {
        tmp |= 0x04;
    } else {
        tmp &= ~0x04;
    }

    if (J2_PORT & 0x04) {
        tmp |= 0x02;
    } else {
        tmp &= ~0x02;
    }

    if (J2_PORT & 0x08) {
        tmp |= 0x01;
    } else {
        tmp &= ~0x01;
    }

    return tmp;
}

/* Checks cable integrity */
void validate() {

    uint8_t pin = 0x01;

    LED_PORT = LED_CONNECTED;

    /* check connections */
    for (uint8_t i = 0; i < 8; i++) {
        J1_PORT = pin;

        __delay_ms(1);

        uint8_t tmp = hardware_fix();

        if (tmp != pin) {

            /* If the number of high inputs at J2 is greater than 1, that means
             * two wires are shorted
             */
            uint8_t cnt = one_count();

            if (cnt > 1) {
                PORTB |= LED_SHORT;
                break;
            }

            /* If there is no input high, the cable is open */
            if (cnt == 0) {
                PORTB |= LED_OPEN;
                break;
            }

            /* If the high input does have an one bit offset, this means the 
             * cable is cross over
             */
            if ((tmp == (pin >> 1)) || (tmp == (pin << 1))) {
                PORTB |= LED_CROSS;
                break;
            }
        }

        pin <<= 1;
    }


}

void main(void) {

    LED_TRIS = 0b11100001;
    LED_PORT = 0b00000000;

    J1_TRIS = 0b00000000;
    J1_PORT = 0b11111111;

    J2_TRIS = 0b11111111;

    /* Care must be taken with the "microprocessor mode" of this port */
    BUZZER_TRIS = 0b00000011;
    BUZZER_PORT = 0;

    /* Adjust WDT prescaler */
    OPTION_REG &= ~(0x04);

    while (1) {

        bool check_con = check();

        if ((check_con == false) && (connected == false)) {
            /* No cable detected, blink the led */
            PORTB ^= LED_CONNECTED;

        } else if ((check_con == true) && (connected == true)) {
            /* Cable is connected */
            validate();

        } else if ((check_con == true) && (connected == false)) {
            /* Cable was plugged */
            connected = true;

            /* A little delay to avoid connection erros */
            CLRWDT();
            for (uint16_t i = 0; i < 300; i++) {
                __delay_ms(1);
                CLRWDT();
            }

            validate();

            beep();
        } else if ((check_con == false) && (connected == true)) {
            /* Cable disconnected */
            connected = false;
            PORTB = 0b00000000;
        }

        SLEEP();
    }
}
