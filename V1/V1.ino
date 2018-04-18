/*********************************************************************
Sketch for Arduino-powered Smart Stryfe V1

I don't care about 80 char

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

uint8_t magSizeArr[] = {5, 6, 10, 12, 15, 18, 20, 22, 25, 36, 0};  		//keep track of the magazine sizes
uint8_t currentMagSize = 0;  											//keep track of the current magazine size
uint8_t currentAmmo = magSizeArr[currentMagSize];    					//keep track of how much ammo there currently is
uint8_t maxAmmo = magSizeArr[currentMagSize];    						//keep track of what the max ammo is, for use when reloading 

float chronoReading = 123;												//keep track of chrono readings

float voltage = 0;														//keep track of voltage from voltmeter
float lastVoltage = 0;													//keep track of last voltage reading

int motorVel = 0;														//keep track of motor velocity via PWM

uint8_t toUpdateDisplay = true;											//flag to update display. Don't want to update display every loop()	 


void setup() {
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);							//begin display with correct I2C address
	display.clearDisplay();												//clear display of any jumk that might be on it
	updateDisplay();													//update display to print default values
}

void loop() {
	changeMagSizes();														//change magazine sizes, if needed
	reload();																//reload, if needed
	updateDisplay();														//update display, if needed
}

void updateDisplay() {														//function to deal with updating display
	if (toUpdateDisplay) {													//make sure need to update display before update it
		display.clearDisplay();												//clear display of any stuff from last display print
		display.setTextColor(WHITE);										//set color to print stuff

		//display ammo counter values
		display.setTextSize(6);												//set text size to print ammo
		display.setCursor(30, 8);											//set cursor position to print ammo
		if (currentAmmo < 10) {												//if current ammo less than 10
			display.print("0" + (String)currentAmmo);						//print current ammo with preceding 0
		} else {															//if current ammo more than 10
			display.print((String)currentAmmo);								//just print current ammo
		}

		display.setTextSize(1);												//set smaller text size for chrono and voltage readings

		//display chrono reading
		display.setCursor(10, 56);											//set cursor position to print chrono vals
		//validation of chrono vals
		if (chronoReading > 0) {											//if chronoReading greater than 0, it's valid
			display.print((String)chronoReading + " fps");					//print chrono reading as fps
		} else {															//if chronoReading invalid
			display.print("ERR");											//display error message for chrono
		}

		//display voltage reading
		display.setCursor(78, 56);											//set cursor position to print voltage vals
		display.print((String)voltage + " fps");							//print chrono reading

		//display motor PWM bar 
		uint8_t lineLength = 64 - motorVel * 4;								//calculate length of line to draw based on pot reading
	    display.drawLine(0, 63, 0, lineLength, WHITE);						//draw 1 line with desired length
	    display.drawLine(1, 63, 1, lineLength, WHITE);						//draw other line with desired length. 2 lines drew to make bar more visible


		display.display();													//actually show all the stuff printed onto the display

		toUpdateDisplay = false;											//display just updated, next update when data change
	}

}

void changeMagSizes () {													//function to deal with changing magazine sizes 
	magSzTogBtn.read();														//read the magazine size toggle button's value
	if (magSzTogBtn.wasPressed()) {											//if mag size toggle btn pressed
	    currentMagSize = (currentMagSize < 									//cycle through mag sizes based on array
	    	(sizeof(magSizeArr)/sizeof(magSizeArr[0]) - 1)) ? 				//make sure array doens't overflow
	    	currentMagSize + 1 : 0;    

	    maxAmmo = magSizeArr[currentMagSize];								//there's a new max ammo, because there's a new magazine size
	    currentAmmo = maxAmmo;												//also reload

	    toUpdateDisplay = true;												//data has been changed, update display to show data
	}	
}

void reload () {															//function to deal with reloading 
	reloadBtn.read();														//read the value of the switch within the magwell
	if (reloadBtn.wasPressed()) {											//if switch in magwell pressed
		currentAmmo = maxAmmo;												//reset magazine size to max

	    toUpdateDisplay = true;												//data has been changed, update display to show data
	}
}

