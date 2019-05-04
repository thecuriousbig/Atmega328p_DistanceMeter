/*
lcdpcf8574 lib sample

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/

#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "vl53l1x/vl53l1_api.h"
#include "lcdpcf8574.h"
#include "uart.h"

#define PIN(x) (*(&x - 2))    /* address of input register of port x */
#define UART_BAUD_RATE 19200

VL53L1_Dev_t dev;
VL53L1_DEV   Dev = &dev;
int status;
int line = 0;
int sensor_ready = 0;
ISR(INT0_vect)
{
	
}

ISR(INT1_vect)
{
	if(line%2==0)
	{
		lcd_gotoxy(0,line);
		lcd_puts(" ");
		line++;
		line %=2;
		PORTC ^= (1<<PORTC0);
		lcd_gotoxy(0,line);
		lcd_puts(">");
	
	}
}

void vl53l1x_init()
{
	Dev->I2cDevAddr = 0x52;
	VL53L1_software_reset(Dev);
	status = VL53L1_WaitDeviceBooted(Dev);
	status = VL53L1_DataInit(Dev);
	status = VL53L1_StaticInit(Dev);
	status = VL53L1_SetDistanceMode(Dev, VL53L1_DISTANCEMODE_LONG);
	status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(Dev, 50000);
	status = VL53L1_SetInterMeasurementPeriodMilliSeconds(Dev, 50);
	status = VL53L1_StartMeasurement(Dev);
	lcd_clrscr();
	lcd_gotoxy(0,0);
	if (status)
	{
		lcd_puts("SENSOR FAILED");
		while(1);
	}
	lcd_puts("SENSORREADY");
	_delay_ms(10);
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
	vl53l1x_init();
	
	lcd_clrscr();
	lcd_gotoxy(0,0);
	while(1)
	{
		static VL53L1_RangingMeasurementData_t rangingData;
		status = VL53L1_WaitMeasurementDataReady(Dev);
		lcd_puts("NAHEE");
		//if (!status)
		//{
			status = VL53L1_GetRangingMeasurementData(Dev, &rangingData);
			if (status == 0)
			{
				char distance_out[16];
				sprintf(distance_out, "%d", rangingData.RangeMilliMeter);
				lcd_puts(distance_out);
				_delay_ms(1);
				
			}
		//}
		
	}

}


