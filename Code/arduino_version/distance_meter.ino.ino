#include <VL53L1X.h>
#include <LiquidCrystal_PCF8574.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "PinChangeInterrupt.h"
#include <string.h>
#include <EEPROM.h>

#define MENU 0
#define DISTANCE 1
#define AREA 2
#define HISTORY 3
#define RESULT 4

LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display
volatile boolean pinchange = true;

//Set pin interrupt
const byte PIN_UP = 2;
const byte PIN_DOWN = 3;
const byte PIN_SELECT = 8;
const byte PIN_MODE = 9;

//Set flag
int up_flag = false;
int down_flag = false;
int select_flag = false;
int mode_flag = false;
int ok_flag = false;
int length_flag = false;
int width_flag = false;
int count = 0;
int show_area_flag = false;

//Declare line the cursor point
int current_line = 0;

//Declare necessry variable for multi-menu
int current_mode;
int next_mode;

//Declare variable to store data from sensor
int32_t data_from_sensor = 0;
int32_t length_data = 0;
int32_t width_data = 0;
int current_eeprom_index = 0;

//Declare sensor
VL53L1X sensor;

void setup()
{
    //DDRD = DDRD & ~(1<<2); //set pin to input
    //interrupt request enabled
    pinMode(PIN_UP, INPUT_PULLUP);
    pinMode(PIN_DOWN, INPUT_PULLUP);
    pinMode(PIN_SELECT, INPUT_PULLUP);
    pinMode(PIN_MODE, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_UP), pin_up, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_DOWN), pin_down, RISING);
    attachPCINT(digitalPinToPCINT(PIN_SELECT), pin_select, RISING);
    attachPCINT(digitalPinToPCINT(PIN_MODE), pin_mode, RISING);

    //Initial flag
    up_flag = false;
    down_flag = false;
    select_flag = false;
    mode_flag = false;
    ok_flag = false;
    length_flag = false;
    width_flag = false;
    show_area_flag = false;

    //Intitial Line
    current_line = 0;

    //Initial LCD
    Serial.begin(115200); // initialize the serial monitor
    lcd.begin(16, 2);     // initialize the lcd
    lcd.home();
    lcd.setBacklight(255); // open backlight
    lcd.cursor();
    lcd.clear();

    //Initial display
    showmenu();

    //Initial EEPROM
    //  EEPROM[0] = 0;

    //Initial sensor
    sensor.setTimeout(500);
    if (!sensor.init())
    {
        lcd.print("Sensor Failed");
        while (1)
            ;
    }
    sensor.setDistanceMode(VL53L1X::Long);
    sensor.setMeasurementTimingBudget(50000);
    sensor.startContinuous(50);
}

void loop()
{
    if (up_flag == true)
    {
        scrollUp();
        up_flag = false;
    }
    if (down_flag == true)
    {
        scrollDown();
        down_flag = false;
    }
    if (select_flag == true)
    {
        //When select flag work next mode start working
        processNextMode();
        select_flag = false;
    }
    if (mode_flag == true)
    {
        showmenu();
        mode_flag = false;
    }

    if (ok_flag == false && current_mode == DISTANCE)
    {
        sensor.read();
        //int distance_cm = sensor.ranging_data.range_mm/10;
        lcd.setCursor(0, current_line);
        lcd.print("= ");
        lcd.print(sensor.ranging_data.range_mm);
        data_from_sensor = sensor.ranging_data.range_mm;
        lcd.print(" mm.");
        delay(300);
    }

    if (current_mode == AREA)
    {
        //lcd.print("AREA");
        if (count == 0)
        {
            lcd.setCursor(0, 0);
            lcd.print("Length :");
            sensor.read();
            //int distance_cm = sensor.ranging_data.range_mm/10;
            length_data = sensor.ranging_data.range_mm;
            String buff = String(length_data);
            lcd.print(buff);
            lcd.print(" mm.");
            delay(300);
        }
        else if (count == 1)
        { //first click
            lcd.setCursor(0, 1);
            lcd.print("Width :");
            sensor.read();
            //int distance_cm = sensor.ranging_data.range_mm/10;
            width_data = sensor.ranging_data.range_mm;
            String buff = String(width_data);
            lcd.print(buff);
            lcd.print(" mm.");
            delay(300);
        }
        else if (count == 2)
        {
            current_mode = RESULT;
            next_mode = AREA;
            lcd.clear();
            int32_t area = (width_data) * (length_data);
            String buff;
            lcd.setCursor(0, 0);
            lcd.print("L:");
            buff = String(length_data);
            lcd.print(buff);
            lcd.print(" W :");
            buff = String(width_data);
            lcd.print(buff);
            lcd.setCursor(0, 1);
            buff = String(area);
            lcd.print(buff);
            lcd.print(" sq.mm.");
            count = 0;
        }
    }
}

void processNextMode()
{
    if (next_mode == DISTANCE)
    {
        current_mode = DISTANCE;
        next_mode = HISTORY;
        lcd.clear();
        current_line = 0;
        lcd.setCursor(0, current_line);
        lcd.print("Distance mode");
        current_line = 1;
    }
    else if (next_mode == MENU)
    {
        showmenu();
    }
    else if (next_mode == HISTORY)
    {
        current_mode = HISTORY;
        next_mode = DISTANCE;
        delay(1000);
        lcd.clear();
        int32_t val;

        //if it is a first time.
        if (EEPROM[0] == 255)
        {
            lcd.setCursor(0, 0);
            lcd.print("No record");
            //        current_eeprom_index++;
            current_eeprom_index = current_eeprom_index % sizeof(int32_t);
            //        EEPROM[current_eeprom_index] = data_from_sensor;

            EEPROM.put(current_eeprom_index, data_from_sensor);
        }
        else
        {
            lcd.setCursor(0, 0);
            lcd.print("Old rec.: ");
            EEPROM.get(current_eeprom_index, val);
            lcd.print(val);

            current_eeprom_index += sizeof(int32_t);
            current_eeprom_index = current_eeprom_index % sizeof(int32_t);
            EEPROM.put(current_eeprom_index, data_from_sensor);
        }
        //Print new record
        current_line = 1;
        lcd.setCursor(0, current_line);
        EEPROM.get(current_eeprom_index, val);
        lcd.print("New rec.: ");
        lcd.print(val);
    }
    else if (next_mode == AREA)
    {
        current_mode = AREA;
        //      next_mode = RESULT;
        ok_flag = false;
        if (count == 0) //never click
            lcd.clear();
        current_line = 0;
        lcd.setCursor(0, current_line);
    }
}

void showmenu()
{
    lcd.clear();
    current_line = 0;
    lcd.setCursor(0, current_line);
    lcd.print(">Distance");
    current_line++;
    lcd.setCursor(1, current_line);
    lcd.print("Area");

    current_line = 0;
    lcd.setCursor(0, current_line);

    current_mode = MENU;
    next_mode = DISTANCE;
}

void scrollUp()
{
    if ((current_line % 2) != 0 && current_mode != DISTANCE && current_mode != AREA)
    {
        if (current_mode == MENU)
        {
            lcd.setCursor(0, current_line);
            lcd.print(" ");
        }
        current_line--;
        current_line = current_line % 2;
        lcd.setCursor(0, current_line);
        if (current_mode == MENU)
        {
            lcd.print(">");
            next_mode = DISTANCE;
        }
    }
}

void scrollDown()
{
    if ((current_line % 2) != 1 && current_mode != DISTANCE && current_mode != AREA)
    {
        if (current_mode == MENU)
        {
            lcd.setCursor(0, current_line);
            lcd.print(" ");
        }
        current_line++;
        current_line = current_line % 2;
        lcd.setCursor(0, current_line);
        if (current_mode == MENU)
        {
            lcd.print(">");
            next_mode = current_line + 1;
        }
    }
}

void pin_up()
{
    up_flag = !up_flag;
    delay(1000);
}

void pin_down()
{
    down_flag = !down_flag;
    delay(1000);
}

void pin_select()
{
    if (current_mode != DISTANCE or current_mode != AREA or next_mode == HISTORY)
        select_flag = !select_flag;
    else if (current_mode == DISTANCE)
        ok_flag = !ok_flag;

    if (current_mode == AREA && count <= 2)
    {
        count++;
    }

    delay(1000);
}

void pin_mode()
{
    mode_flag = !mode_flag;
    delay(1000);
}

int multiply(int a, int b)
{
    // flag to store if result is positive or negative
    bool isNegative = false;

    // if both numbers are negative, make both numbers
    // positive as result will be positive anyway
    if (a < 0 && b < 0)
        a = -a, b = -b;

    // if only a is negative, make it positive
    // and mark result as negative
    if (a < 0)
        a = -a, isNegative = true;

    // if only b is negative, make it positive
    // and mark result as negative
    if (b < 0)
        b = -b, isNegative = true;

    // initialize result
    int res = 0;

    // run till b becomes 0
    while (b)
    {
        // if b is odd, add b to the result
        if (b & 1)
            res += a;

        // multiply a by 2
        a = a << 1;

        // divide b by 2
        b = b >> 1;
    }

    return (isNegative) ? -res : res;
}
