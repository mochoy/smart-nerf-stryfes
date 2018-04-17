/*********************************************************************
Sketch for Arduino-powered Smart Stryfe V1

Description and part links also in README:
	- Features: IR Gate ammo counter, chronograph, reload detection, 
		togglable magazine sizes, and PWM variable motor control
	- Parts: Suild MOSFET board ($10), Artifact Red cage ($34), 
		Worker High-Crush Wheels ($13), Fang ReVamped motors ($15)

By Monty C, 04/15/18
*********************************************************************/


//libraries to include
#include <Adafruit_SSD1306.h>											//import library for display driver
#include <Adafruit_GFX.h>												//import library for display graphics
#include <Button.h>														//import library for buttons

//analog input pins
#define IR_REC_PIN 0													//pin for IR receiver                  	
#define VOLTMETER_PIN 1													//pin for voltmeter
#define POT_PIN 3														//pin for potentiometer

//button output pins
#define TRIGGER_BTN_PIN 4												//pin for trigger button
#define RELOAD_BTN_PIN 7												//pin for reload switch
#define MAG_SZ_TOG_BTN_PIN 8											//pin for magazine size toggle button

//PWM output pins
#define MOTOR_OUT_PIN 11												//pin for PWM out to control motor

//macros for display functionality
#define SCREEN_WIDTH 128												//screen width of display
#define SCREEN_HEIGHT 64												//screen height of display
#define OLED_RESET 4													//OLED Reset pin

//macros for IR gate and chrono functionality
#define IR_GATE_TRIP_VAL 60               								//value at which the IR gate is considered "blocked", or "tripped"
#define DART_LEGTH 0.23622												//length of dart, in feet

//macros for voltmeter functionality
#define R1 100000f														//reistance of R1 to calculate voltage division
#define R2 10000f														//reistance of R2 to calculate voltage division

//macros for button functionality
#define PU_ENABLE false													//don't enable internal pullup resistors for buttons
#define INVERT false													//don't enable invert button readings
#define DB_TIME 25														//debouce time for button

Adafruit_SSD1306 display(OLED_RESET);									//display object instantiated from SSD1306 driver library

Button triggerBtn (TRIGGER_BTN_PIN, PU_ENABLE, INVERT, DB_TIME);		//Trigger button, from Button lib
Button reloadBtn (RELOAD_BTN_PIN, PU_ENABLE, INVERT, DB_TIME);			//Reload button, from Button lib
Button magSzTogBtn (MAG_SZ_TOG_BTN_PIN, PU_ENABLE, INVERT, DB_TIME);	//Magazine size toggle button, from Button lib

byte magSizeArr[] = {5, 6, 10, 12, 15, 18, 20, 22, 25, 36, 0};  		//keep track of the magazine sizes
byte currentMagSize = 0;  												//keep track of the current magazine size
byte currentAmmo = magSizeArr[currentMagSize];    						//keep track of how much ammo there currently is
byte maxAmmo = magSizeArr[currentMagSize];    							//keep track of what the max ammo is, for use when reloading 

float chronoReading = 123;												//keep track of chrono readings


void setup() {
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);							//begin display with correct I2C address
	display.clearDisplay();												//clear display of any jumk that might be on it
}

void loop() {
	updateDisplay();
}

void updateDisplay() {
	display.clearDisplay();												//clear display of any stuff from last display print
	display.setTextColor(WHITE);										//set color to print stuff

	//display ammo counter values
	display.setTextSize(6);												//set text size to print ammo
	display.setCursor(30, 10);											//set cursor position to print ammo
	if (currentAmmo < 10) {												//if current ammo less than 10
		display.print("0" + (String)currentAmmo);						//print current ammo with preceding 0
	} else {															//if current ammo more than 10
		display.print((String)currentAmmo);								//just print current ammo
	}

	display.setTextSize(1);												//set smaller text size for chrono and voltage readings

	//display chrono reading
	display.setCursor(5, 56);											//set cursor position to print chrono vals
	display.print((String)chronoReading + " fps");						//print chrono reading

	display.display();													//actually show all the stuff printed onto the display


}