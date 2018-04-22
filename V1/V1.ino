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

#include <SoftwareSerial.h>

//libraries to include
#include <Adafruit_SSD1306.h>												//import library for display driver
#include <Adafruit_GFX.h>													//import library for display graphics
#include <Button.h>															//import library for buttons

//analog input pins
#define IR_REC_PIN 0														//pin for IR receiver                  	
#define VOLTMETER_PIN 1														//pin for voltmeter
#define POT_PIN 2															//pin for potentiometer

//button output pins
#define TRIGGER_BTN_PIN 4													//pin for trigger button
#define RELOAD_BTN_PIN 7													//pin for reload switch
#define MAG_SZ_TOG_BTN_PIN 8												//pin for magazine size toggle button

//PWM output pins
#define MOTOR_OUT_PIN 11													//pin for PWM out to control motor

//macros for display functionality
#define SCREEN_WIDTH 128													//screen width of display
#define SCREEN_HEIGHT 64													//screen height of display
#define OLED_RESET 4														//OLED Reset pin

//macros for IR gate and chrono functionality
#define IR_GATE_TRIP_VAL 80               									//value at which the IR gate is considered "blocked", or "tripped"
#define IR_GATE_UNTRIP_VAL 30												//value at which IR gate no blocked, or "un-tripped"
#define DART_LENGTH 0.23622													//length of dart, in feet

//macros for voltmeter functionality
#define R1 100000.0															//reistance of R1 to calculate voltage division
#define R2 10000.0															//reistance of R2 to calculate voltage division

//macros for button functionality
#define PU_ENABLE false														//don't enable internal pullup resistors for buttons
#define INVERT false														//don't enable invert button readings
#define DB_TIME 25															//debouce time for button

//macors for PWM functionality
#define MOTOR_ACCEL_DELAY 50												//time (in milliseconds) to allow motors to rev to max speed


Adafruit_SSD1306 display(OLED_RESET);										//display object instantiated from SSD1306 driver library

Button triggerBtn (TRIGGER_BTN_PIN, PU_ENABLE, true, DB_TIME);				//Trigger button, from Button lib
Button reloadBtn (RELOAD_BTN_PIN, PU_ENABLE, INVERT, DB_TIME);				//Reload button, from Button lib
Button magSzTogBtn (MAG_SZ_TOG_BTN_PIN, PU_ENABLE, INVERT, DB_TIME);		//Magazine size toggle button, from Button lib

uint8_t magSizeArr[] = {5, 6, 10, 12, 15, 18, 20, 22, 25, 36, 0};  			//keep track of the magazine sizes
uint8_t currentMagSize = 0;  												//keep track of the current magazine size
uint8_t currentAmmo = magSizeArr[currentMagSize];    						//keep track of how much ammo there currently is
uint8_t maxAmmo = magSizeArr[currentMagSize];    							//keep track of what the max ammo is, for use when reloading 

float voltage = 0;															//keep track of voltage from voltmeter
double lastVoltageCheckTime = 0;

uint8_t motorVel = 0;														//keep track of motor velocity via PWM
uint8_t lastMotorVel = 0;													//keep track of last motor velocity
double lastMotorVelCheckTime = 0;											//keep track of time for debouncing
uint8_t hasMotorsAccel = false;												//flag to check if motors have already acceletated
double motorsAccelStartTime = 0;											//keep track of when motors started accel

float chronoReading = 0;													//keep track of chrono readings
double enterTime = -10;														//time when dart enters IR gate, in microsecodns. Set to -1 to indicate no chrono val
double exitTime = -10;														//time when dart enters IR gate, in microseconds. Set to -1 to indicate no chrono val

uint8_t toUpdateDisplay = true;												//flag to update display. Don't want to update display every loop()	 


void setup() {
	Serial.begin(9600);
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);								//begin display with correct I2C address
	display.clearDisplay();													//clear display of any jumk that might be on it
	updateDisplay();														//update display to print default values

	pinMode(MOTOR_OUT_PIN, OUTPUT);											//set MOTOR_OUT_PIN as an output pin 
}

void loop() {
	changeMagSizes();														//change magazine sizes, if needed
	reload();																//reload, if needed
	potInput();																//deal with potentieometer as input
	PWMFlywheels();															//PWM out to motors
	voltmeter();															//deal with all voltmeter stuff
	chrono();																//count ammo and do chrono stuff, if needed
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
		display.print((String)voltage + "V");								//print chrono reading

		//display motor PWM bar 
		uint8_t lineLength = 64 - motorVel * 4;								//calculate length of line to draw based on pot reading
	    display.drawLine(0, 63, 0, lineLength, WHITE);						//draw 1 line with desired length
	    display.drawLine(1, 63, 1, lineLength, WHITE);						//draw other line with desired length. 2 lines drew to make bar more visible


		display.display();													//actually show all the stuff printed onto the display

		toUpdateDisplay = false;											//display just updated, next update when data change
	}

}

void changeMagSizes() {														//function to deal with changing magazine sizes 
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

void reload() {																//function to deal with reloading 
	reloadBtn.read();														//read the value of the switch within the magwell
	if (reloadBtn.wasPressed()) {											//if switch in magwell pressed
		currentAmmo = maxAmmo;												//reset magazine size to max

	    toUpdateDisplay = true;												//data has been changed, update display to show data
	}
}

void potInput() {															//function to deal with pot input changes			
	if ( ((millis() + DB_TIME) > lastMotorVelCheckTime) && 					//make sure to check at certian intervals instead of every loop()
		(motorVel != lastMotorVel) ) {										//make sure motor velocity changed

		lastMotorVel = motorVel = map(analogRead(POT_PIN), 					//set new motor velocity
			0, 1010, 0, 16);				
		lastMotorVelCheckTime = millis();									//reset time for checking motor vel

	    toUpdateDisplay = true;												//data has been changed, update display to show data
	} else {																//if motor velocity didn't change or not time to check
		lastMotorVel = map(analogRead(POT_PIN),0, 1010, 0, 16);				//set new last motor velocity to compare changes
	}
}

void PWMFlywheels() {														//function to deal with PWMing flywheel motors
	triggerBtn.read();														//read trigger
	if (triggerBtn.isPressed() && 											//if trigger pressed
		analogRead(POT_PIN) > 50) {											//and if pot high enough value

		if (!hasMotorsAccel) {												//if motors haven't accelerated
			accelMotors();													//accel motors
		} else {															//if motors have accelerated
			analogWrite(MOTOR_OUT_PIN, analogRead(POT_PIN)/4);				//PWM motor
		}
	} else {																//if trigger not pressed
		digitalWrite(MOTOR_OUT_PIN, false);									//turn off motor
		hasMotorsAccel = false;												//reset hasMotorAccel for next trigger pull
		motorsAccelStartTime = 0;											//reset motorsAccelStartTime for next trigger pull
	}
}

void accelMotors () {														//function to deal with accelerating motors
	if (motorsAccelStartTime == 0) {										//if motorAccelStartTime not set
		motorsAccelStartTime = millis();									//set it to now, since it's going to start accelerating now
	}

	if (millis() < (motorsAccelStartTime + MOTOR_ACCEL_DELAY)) {			//if still need to accelerate motors
		digitalWrite(MOTOR_OUT_PIN, HIGH);									//spin motors at full speed so they can accel
	} else {																//if motors already done accel 
		hasMotorsAccel = true;												//motors alread accelled
	}
}

void voltmeter() {															//function to deal with voltmeter
	if (lastVoltageCheckTime + 1000 < millis()) {							//only check once per second to keep Arduino from lagging
		voltage = ((analogRead(VOLTMETER_PIN) * 5.0) 						//magial math/physics calculations to find voltage
			/ 1024.0)/(R2/(R1+R2));
		lastVoltageCheckTime = millis();									//set time so will check voltage again 1 second fron now

    	toUpdateDisplay = true;												//data has been changed, update display to show data
	}
}

void chrono() {																//function to deal with chronograph and ammo counter
	uint8_t mappedIRRecReading = map(analogRead(IR_REC_PIN),				//value of IR receiver reading
		0, 1023, 0, 100);

	if (mappedIRRecReading > IR_GATE_TRIP_VAL) { 							//if IR gate tripped
		Serial.println("entered");
		if (enterTime == -10 && exitTime == -10) {							//if values indicate no chrono readings, so should be looking for one
			enterTime = micros();											//store current time as time at which dart enters
		}
	} else if (mappedIRRecReading < IR_GATE_UNTRIP_VAL						//if IR gate un tripped, dart leaves IR gate
		&& enterTime > 0) {													//make sure dart actually entered		
		Serial.println("exit");
		exitTime = micros();												//store current time as time at which dart exits
		calculateChronoReadings();											//calculate chrono readings with recorded times
		
		ammoCounter();														//dart left IR gate, so a dart has been fired

    	toUpdateDisplay = true;												//data has been changed, update display to show data
	} else if (enterTime > -10 && 											//if dart entered gate
		exitTime == -10 && 													//and dart hasn't exited gate
		enterTime + 1000000 < micros()) {									//and 1 second passed but dart still hasn't exit, something went wrong	
		
		chronoReading = -1;													//set to -1 so display knows to show error
		resetChronoReadings();												//reset chrono readings so can chrono next shot

    	toUpdateDisplay = true;												//data has been changed, update display to show data
	}
}

void calculateChronoReadings() {											//function to deal with calculating chrono readings
	chronoReading = (DART_LENGTH) / ((exitTime-enterTime)/1000000.0);		//calculate how fast dart going
	resetChronoReadings();													//reset values to get ready for next shot
}

void resetChronoReadings() {												//function to reset chrono readings so can calculate next chrono readings
	enterTime = exitTime = -10;												//reset exit and enter times to get ready for next shot
}

void ammoCounter() {														//function to deal with ammo counting
	if (maxAmmo != 0 && currentAmmo < 99 && currentAmmo > 0) {  			//make sure ammo is less than 99 so doesnt overflow the display when not in count-up mode
      currentAmmo--;    													//decrement ammo
    } else if (maxAmmo == 0 && currentAmmo < 99) { 							//make sure ammo more than 0 so no negative numbers are displayed in count-up mode
      currentAmmo++;    													//increment ammo
    }

    Serial.println("shot fired");

	toUpdateDisplay = true;													//data has been changed, update display to show data
}

