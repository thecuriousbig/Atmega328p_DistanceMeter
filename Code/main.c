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
#include <string.h>
#include <stdbool.h>

#define F_CPU 16000000UL
#include <util/delay.h>

#include "lcdpcf8574.h"

#define UART_BAUD_RATE 19200
#include "uart.h"

#define MAX 10
//Declare mode of each page
#define MAIN_MENU 0
#define DISTANCE_MODE 1
#define AREA_MODE 2
#define HISTORY_DISTANCE_MODE 3

#define PIN(x) (*(&x - 2)) /* address of input register of port x */

// macro for easier usage
#define read_eeprom_word(address) eeprom_read_word((const uint16_t *)address)
#define write_eeprom_word(address, value) eeprom_write_word((uint16_t *)address, (uint16_t)value)
#define update_eeprom_word(address, value) eeprom_update_word((uint16_t *)address, (uint16_t)value)
#define read_eeprom_array(address, value_p, length) eeprom_read_block((void *)value_p, (const void *)address, length)
#define write_eeprom_array(address, value_p, length) eeprom_write_block((const void *)value_p, (void *)address, length)

//declare an eeprom array
unsigned int EEMEM my_eeprom_array[MAX];

//declare a ram array
unsigned int my_ram_array[MAX];
unsigned int my_another_ram_array[MAX];

int line = 0;			//a line that cursor is point
int isMenu = 0;		//Check is meno mode:0 is false , 1 is true
int nextMode = 0; //Next mode of to go
int mode = 0;			//Current mode

//Use for distance history queue
int front = 0;
int rear = -1;
int itemCount = 0;

int data_from_sensor = 0;
int current_history_index = -1; //declare current index for history mode

unsigned int peek()
{
	return my_ram_array[front];
}

bool isEmpty()
{
	return itemCount == 0;
}

bool isFull()
{
	return itemCount == MAX;
}

int size()
{
	return itemCount;
}

void insert(unsigned int data)
{

	if (!isFull())
	{

		if (rear == MAX - 1)
		{
			rear = -1;
		}

		my_ram_array[++rear] = data;
		itemCount++;
	}
}

unsigned int removeData()
{
	uint8_t data = my_ram_array[front++];

	if (front == MAX)
	{
		front = 0;
	}

	itemCount--;
	return data;
}

//this function is for show menu
void menu()
{
	isMenu = 1;
	line %= 2;
	//initialize
	lcd_gotoxy(0, line);
	lcd_puts(">");

	lcd_gotoxy(1, line);
	lcd_puts("Measure");
	line++;
	lcd_gotoxy(1, line);
	lcd_puts("Area");

	line = 0;
	lcd_gotoxy(0, line);
	mode = MAIN_MENU;
	nextMode = DISTANCE_MODE;
}

//This function is for initial history page after press select button in the Distance mode.
void distanceHistory()
{
	// restore to my_other_ram_array from eeprom
	read_eeprom_array(my_eeprom_array, my_ram_array, sizeof(my_eeprom_array));
	char buff[10];
	int index, distance;
	front = 0;
	rear = itemCount - 1;
	if (size() != 0)
	{
		index = rear;
		lcd_gotoxy(0, line);
		itoa(index, buff, 10);
		lcd_puts(buff);
		lcd_gotoxy(2, line);
		lcd_puts(".");
		distance = my_ram_array[rear - 1];
		itoa(distance, buff, 10);
		lcd_gotoxy(3, line);
		lcd_puts(buff);
		lcd_gotoxy(9, line);
		lcd_puts("CM");
		line++;
		//lastest data
		lcd_gotoxy(0, line);
		index = rear + 1;
		itoa(index, buff, 10);
		lcd_puts(buff);
		lcd_gotoxy(2, line);
		lcd_puts(".");
		distance = my_ram_array[rear];
		itoa(distance, buff, 10);
		lcd_gotoxy(3, line);
		lcd_puts(buff);
		lcd_gotoxy(9, line);
		lcd_puts("CM");

		line = 0;
		lcd_gotoxy(0, line);
		current_history_index = rear;
	}
	else
	{
		lcd_puts("NO DATA ---");
	}
}

//GO Upper button interrupt function.
ISR(INT0_vect)
{
	//Go to upper line
	if (line % 2 == 1)
	{
		if (isMenu)
		{
			lcd_gotoxy(0, line);
			lcd_puts(" ");
		}
		line--;
		line %= 2;
		lcd_gotoxy(0, line);
	}
	//If we are in main menu
	if (isMenu)
	{
		lcd_gotoxy(0, line);
		lcd_puts(">");
		lcd_gotoxy(0, line);
		nextMode = DISTANCE_MODE;
	}
	else
	{
		if (mode == HISTORY_DISTANCE_MODE)
		{
			int index, distance;
			char buff[10];
			if (size() != 0 && current_history_index > 1)
			{
				//clear screen
				lcd_clrscr();

				//Decrease index to show history
				current_history_index--;
				index = current_history_index;
				lcd_gotoxy(0, line);
				itoa(index, buff, 10);
				lcd_puts(buff);
				lcd_gotoxy(2, line);
				lcd_puts(".");
				distance = my_ram_array[current_history_index - 1];
				itoa(distance, buff, 10);
				lcd_gotoxy(3, line);
				lcd_puts(buff);
				lcd_gotoxy(9, line);
				lcd_puts("CM");
				line++;

				//lastest data
				lcd_gotoxy(0, line);
				index = current_history_index + 1;
				itoa(index, buff, 10);
				lcd_puts(buff);
				lcd_gotoxy(2, line);
				lcd_puts(".");
				distance = my_ram_array[current_history_index];
				itoa(distance, buff, 10);
				lcd_gotoxy(3, line);
				lcd_puts(buff);
				lcd_gotoxy(9, line);
				lcd_puts("CM");

				line = 0;
			}
		}
	}
}

//Go lower button interrupt function
ISR(INT1_vect)
{
	//Go to lower line
	if (line % 2 == 0)
	{
		if (isMenu)
		{
			lcd_gotoxy(0, line);
			lcd_puts(" ");
		}
		line++;
		line %= 2;
		lcd_gotoxy(0, line);
	}
	//If we are in main menu
	if (isMenu)
	{
		lcd_gotoxy(0, line);
		lcd_puts(">");
		lcd_gotoxy(0, line);
		nextMode = AREA_MODE;
	}
	else
	{
		if (mode == HISTORY_DISTANCE_MODE)
		{
			int index, distance;
			char buff[10];
			if (size() != 0 && current_history_index < 10 && current_history_index < (itemCount - 1))
			{
				//clear screen
				lcd_clrscr();

				//Increase index to show history
				current_history_index++;
				index = current_history_index;
				line = 0;
				lcd_gotoxy(0, line);
				itoa(index, buff, 10);
				lcd_puts(buff);
				lcd_gotoxy(2, line);
				lcd_puts(".");
				distance = my_ram_array[current_history_index - 1];
				itoa(distance, buff, 10);
				lcd_gotoxy(3, line);
				lcd_puts(buff);
				lcd_gotoxy(9, line);
				lcd_puts("CM");
				line++;

				//lastest data
				lcd_gotoxy(0, line);
				index = current_history_index + 1;
				itoa(index, buff, 10);
				lcd_puts(buff);
				lcd_gotoxy(2, line);
				lcd_puts(".");
				distance = my_ram_array[current_history_index];
				itoa(distance, buff, 10);
				lcd_gotoxy(3, line);
				lcd_puts(buff);
				lcd_gotoxy(9, line);
				lcd_puts("CM");

				line = 0;
			}
		}
	}
}

//SELECT BUTTON (PORTB0)
//MODE BUTTON (PORTB1)
ISR(PCINT0_vect)
{
	//Select button
	if (!(PINB & (1 << PORTB0)))
	{
		lcd_clrscr();
		lcd_home();
		line = 0;
		switch (nextMode)
		{
		case DISTANCE_MODE:
			lcd_gotoxy(0, line);
			lcd_puts("Distance");
			line++;
			lcd_gotoxy(0, line);
			lcd_puts("= ");
			lcd_gotoxy(6, line);
			lcd_puts("CM");
			isMenu = 0;
			mode = DISTANCE_MODE;
			nextMode = HISTORY_DISTANCE_MODE;
			break;
		case AREA_MODE:
			lcd_gotoxy(0, line);
			lcd_puts("Area Mode ");
			line++;
			mode = AREA_MODE;
			isMenu = 0;
			break;
		case HISTORY_DISTANCE_MODE:
			lcd_gotoxy(0, line);
			if (!isFull())
			{
				insert(data_from_sensor);
			}
			else
			{
				removeData();
				insert(data_from_sensor);
			}
			// Copy data from my_ram_array to eeprom array
			int i = 0;
			while (!isEmpty())
			{
				int n = removeData();
				//printf("%d ",n);
				my_another_ram_array[i] = n;
				i++;
			}
			itemCount = i;
			lcd_clrscr();
			write_eeprom_array(my_eeprom_array, my_another_ram_array, sizeof(my_eeprom_array));
			distanceHistory();
			mode = HISTORY_DISTANCE_MODE;
			nextMode = DISTANCE_MODE;
			isMenu = 0;
			break;
		}
	}
	else if (!(PINB & (1 << PORTB1))) //MODE BUTTON
	{
		lcd_clrscr();
		lcd_home();
		line = 0;
		menu();
	}
}

int main(void)
{
	//init switch
	//enable internal pull-up button up
	PORTD |= (1 << PORTD2) | (1 << PORTD3);
	PORTB |= (1 << PORTB0) | (1 << PORTB1);
	EICRA |= (1 << ISC01);							//interrupt on falling edge of INT0
	EIMSK |= (1 << INT0) | (1 << INT1); // enable interrupt for INT0

	PCMSK0 |= (1 << PCINT0) | (1 << PCINT1);
	PCICR |= (1 << PCIE0);
	sei();

	//init uart
	uart_init(UART_BAUD_SELECT(UART_BAUD_RATE, F_CPU));

	uart_puts("starting...");

	//init lcd
	lcd_init(LCD_DISP_ON_BLINK);

	//lcd go home
	lcd_home();

	uint8_t led = 0;
	lcd_led(led); //set led

	//MOCK UP DATA
	insert(1);
	insert(2);
	insert(3);

	menu();
	while (1)
	{
	}
	return 0;
}
