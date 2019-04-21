/*
lcdpcf8574 lib sample

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>

#include "lcdpcf8574.h"

#define UART_BAUD_RATE 19200
#include "uart.h"


#define PIN(x) (*(&x - 2))    /* address of input register of port x */

int line = 0;
uint8_t message[] = "welcome",readblock[16];

ISR(INT0_vect)
{
	/*if(line%2==1)
	{
		lcd_gotoxy(0,line);
		lcd_puts(" ");
		line--;
		line %=2;
		PORTC ^= (1<<PORTC0);
		lcd_gotoxy(0,line);
		lcd_puts(">");	
	}*/
	eeprom_read_block((void *)readblock, (const void *)0, 16);
	char *readstring;
	readstring = (char *)readblock;
	lcd_puts(readstring);

}

ISR(INT1_vect)
{
	/*if(line%2==0)
	{
		lcd_gotoxy(0,line);
		lcd_puts(" ");
		line++;
		line %=2;
		PORTC ^= (1<<PORTC0);
		lcd_gotoxy(0,line);
		lcd_puts(">");
	}*/
	eeprom_write_block((const void*)message, (void *)0, 16);
}

void menu()
{
	line %= 2;
	//initialize
	lcd_gotoxy(0,line);
	lcd_puts(">");
	
	lcd_gotoxy(1,line);
	lcd_puts("Measurement");
	line++;
	lcd_gotoxy(1,line);
	lcd_puts("Area");
	
	line = 0;
	lcd_gotoxy(0,line);
}

int main(void)
{
	//init switch
	DDRC |= (1<<PORTC);
	
	PORTC &= !(1<<PORTC0);
	//enable internal pull-up button up
	PORTD |= (1<<PORTD2);
	EICRA |= (1<<ISC01); //interrupt on falling edge of INT0
	EIMSK |= (1<<INT0); // enable interrupt for INT0
	
	//enable internal pull-up button up
	PORTD |= (1<<PORTD3);
	EIMSK |= (1<<INT1); // enable interrupt for INT0
	
	sei();
	
	//init uart
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );

	uart_puts("starting...");

	//init lcd
	lcd_init(LCD_DISP_ON_BLINK);

	//lcd go home
	lcd_home();

	uint8_t led = 0;
	lcd_led(led); //set led
	
	//menu();
	while(1);

}
