/* 
Program for the CasuallyLoaded® Unipolar Stepper Motor Driver kit, Created 2022.
Compiled for use on the Attiny44 with 16Mhz external clock and 4.3v BOD. Each kit has 
this code preprogrammed into the included IC so its not necessary to do the upload yourself. 
For those looking to modify the code it has been annotated. 
*/

//create variables
volatile int StepCount = 0;
volatile int rotationDirct = 0;
volatile int takeHalfSteps = 0;
volatile int SetrevLEDFlash = 0;
volatile int stepDivideLEDFlash = 0;
volatile int MotorStepping = 0;
volatile int motionCheck = 0;
unsigned long revLEDtoggleTime = 0;
unsigned long stpDivLEDtoggleTime = 0;
unsigned long millisRightnow = 0;
unsigned long mtrMotionChk = 0;
int revLEDstate = 0;
int StpDivLEDstate = 0;
int IndicatorsActive = 0;

void setup() {

  DDRA = B00111111; //Sets the MOSFET and led pins to be outputs
  PORTA = B00000000; //Sets all MOSFETS and LEDs to be low.

  GIMSK = B00110000; //Sets PORTA & PORTB to have to have interupts
  PCMSK0 = B11000000; //Sets the pins PORTA6 & PORTA7 to trigger the half step and reverse interupt function
  PCMSK1 = B00000100; //Sets PORTB2 to trigger the step interupt function

  PORTA |= B00110000; //Small function to blink the leds, indicating successful startup
  delay(250);
  PORTA &= ~B00110000; //NOTE: |= B1 will set the bit to a 1 and &= ~B1 will set the bit to a 0 without impacting any other bits
  delay(250);
  PORTA |= B00110000;
  delay(250);
  PORTA &= ~B00110000;
  delay(250);
  
}

void loop() { //since most of the step setting is done using interupts the loop mainly keeps track of the indicator LEDs and if the motor is still spinning

  millisRightnow = millis(); //saving millis to test if the driver is still receiving step inputs
  if ((MotorStepping == 1) && (millisRightnow - mtrMotionChk > 20) && (motionCheck == 1)){ //if the most recent step input is within the limit 
    MotorStepping = 0; //set a test variable that should be back to 1 by the next timeout
    IndicatorsActive = 1; //confirm that LEDs are still blinking
    mtrMotionChk = millisRightnow; //record the most recent timeout 
  }
  else if ((MotorStepping == 0) && (millisRightnow - mtrMotionChk > 20) && (motionCheck == 1)){ //test variable hasnt been reset by the ISR so the motor isnt spinning
    IndicatorsActive = 0; //set that indicators are now off
    motionCheck = 0; //no longer need to check if the motor is in motion
    PORTA &= ~B00110000; //turn off the LEDs
    revLEDstate = 0; //record the states
    StpDivLEDstate = 0;
    mtrMotionChk = millisRightnow; //record the most recent timeout 
  }

  millisRightnow = millis(); //saves millis to control the reverse indicator LED flash
  if ((IndicatorsActive == 1) && (SetrevLEDFlash == 1) && (millisRightnow - revLEDtoggleTime >= 500)){ //runs if the LED should be active and flashing

    if (revLEDstate == 0){ //if the LED is currently off
      PORTA |= B00100000; //turn it on
      revLEDstate = 1; //record the state 
      revLEDtoggleTime = millisRightnow; //record the state change time
    }
    else if (revLEDstate == 1){ //if the LED is currently on
      PORTA &= ~B00100000; //turn it off
      revLEDstate = 0; //record the state 
      revLEDtoggleTime = millisRightnow; //record the state change time
    }
  }
  else if ((IndicatorsActive == 1) && (SetrevLEDFlash == 0)) { //runs if the LED should be active and solid state
    PORTA |= B00100000; //turn it on
    revLEDstate = 1; //record the state 
  }
  
  millisRightnow = millis(); //saves millis to control the step size indicator LED flash
  if ((IndicatorsActive == 1) && (stepDivideLEDFlash == 1) && (millisRightnow - stpDivLEDtoggleTime >= 1000)){ //runs if the LED should be active and flashing

    if (StpDivLEDstate == 0){ //if the LED is currently off
      PORTA |= B00010000; //turn it on
      StpDivLEDstate = 1; //record the state 
      stpDivLEDtoggleTime = millisRightnow; //record the state change time
    }
    else if (StpDivLEDstate == 1){ //if the LED is currently on
      PORTA &= ~B00010000; //turn it off
      StpDivLEDstate = 0; //record the state 
      stpDivLEDtoggleTime = millisRightnow; //record the state change time
    }
  }
  else if ((IndicatorsActive == 1) && (stepDivideLEDFlash == 0)){ //runs if the LED should be active and solid state
    PORTA |= B00010000; //turn it on
    StpDivLEDstate = 1; //record the state 
  }
}

ISR (PCINT0_vect) { //pin change interupt vector for direction and step size inputs 

  if ((PINA & B01000000)){ //checks if pin PORTA6 went high rev 
    rotationDirct = 1; //set the variable used in the step function
    SetrevLEDFlash = 1; //set the variable used in LED indicator function
  }
  else if (!(PINA & B01000000)){ //checks if pin PORTA6 went low fwrd
    rotationDirct = 0; //set the variable used in the step function
    SetrevLEDFlash = 0; //set the variable used in LED indicator function
  }

  if ((PINA & B10000000)){ //checks if pin PORTA7 went high half steps active
    takeHalfSteps = 1; //set the variable used in the step function
    stepDivideLEDFlash = 1; //set the variable used in LED indicator function
  }
  else if (!(PINA & B10000000)){ //checks if pin PORTA7 went low full steps active
    takeHalfSteps = 0; //set the variable used in the step function
    stepDivideLEDFlash = 0; //set the variable used in LED indicator function
  }

}

ISR (PCINT1_vect) { //pin change interupt vector for the step input

  if ((PINB & B00000100)){ //checks if the pin PORTA2 is high 
    MotorStepping = 1; //record that the motor is stepping
    motionCheck = 1; //make sure the loop checks if the motor stays stepping
    StepCount++; //increase the step count
    if (StepCount > 8) { StepCount = 1; } //if the count goes over the max of 8 set it back to 1
    TakeStep(StepCount, takeHalfSteps, rotationDirct); //take a step with its size and the spin direction included
    
  }
}

void TakeStep(int sequenceStep, int stepSize, int StepDirection) { //single function for setting all steps in all modes

  if ((stepSize == 0) && (StepDirection == 0)) { //full steps going forward

    switch(sequenceStep){ //selects the correct step, there are only 4 full steps but for simplicity this section repeats once after step 4 to cover the whole 8 steps
      
      case 1:
      PORTA &= ~B00001100; //turns off only the prevous steps pins
      PORTA |= B00001000; //turns on only the current steps pins
      break;
      
      case 2:
      PORTA &= ~B00001001; //turns off only the prevous steps pins
      PORTA |= B00000001; //turns on only the current steps pins
      break;
      
      case 3:
      PORTA &= ~B00000011; //turns off only the prevous steps pins
      PORTA |= B00000010; //turns on only the current steps pins
      break;
      
      case 4:
      PORTA &= ~B00000110; //turns off only the prevous steps pins
      PORTA |= B00000100; //turns on only the current steps pins
      break;

      case 5:
      PORTA &= ~B00001100; //turns off only the prevous steps pins
      PORTA |= B00001000; //turns on only the current steps pins
      break;
      
      case 6:
      PORTA &= ~B00001001; //turns off only the prevous steps pins
      PORTA |= B00000001; //turns on only the current steps pins
      break;
      
      case 7:
      PORTA &= ~B00000011; //turns off only the prevous steps pins
      PORTA |= B00000010; //turns on only the current steps pins
      break;
      
      case 8:
      PORTA &= ~B00000110; //turns off only the prevous steps pins
      PORTA |= B00000100; //turns on only the current steps pins
      break;
      
    }   
  }

  if ((stepSize == 0) && (StepDirection == 1)) { //full steps going backward
  
    switch(sequenceStep){ //selects the correct step, there are only 4 full steps but for simplicity this section repeats once after step 4 to cover the whole 8 steps

      case 1: 
      PORTA &= ~B00000110; //turns off only the prevous steps pins
      PORTA |= B00000100; //turns on only the current steps pins
      break;

      case 2:
      PORTA &= ~B00000011; //turns off only the prevous steps pins
      PORTA |= B00000010; //turns on only the current steps pins
      break;

      case 3:
      PORTA &= ~B00001001; //turns off only the prevous steps pins
      PORTA |= B00000001; //turns on only the current steps pins
      break;

      case 4:
      PORTA &= ~B00001100; //turns off only the prevous steps pins
      PORTA |= B00001000; //turns on only the current steps pins
      break;

      case 5:
      PORTA &= ~B00000110; //turns off only the prevous steps pins
      PORTA |= B00000100; //turns on only the current steps pins
      break;

      case 6:
      PORTA &= ~B00000011; //turns off only the prevous steps pins
      PORTA |= B00000010; //turns on only the current steps pins
      break;

      case 7:
      PORTA &= ~B00001001; //turns off only the prevous steps pins
      PORTA |= B00000001; //turns on only the current steps pins
      break;

      case 8:
      PORTA &= ~B00001100; //turns off only the prevous steps pins
      PORTA |= B00001000; //turns on only the current steps pins
      break;
 
    }
  }

  if ((stepSize == 1) && (StepDirection == 0)) { //half steps going forward
    
    switch(sequenceStep){ //selects the correct step
      
      case 1:
      PORTA &= ~B00000100; //turns off only the prevous steps pins
      PORTA |= B00001100; //turns on only the current steps pins
      break;
      
      case 2:
      PORTA &= ~B00001100; //turns off only the prevous steps pins
      PORTA |= B00001000; //turns on only the current steps pins
      break;
      
      case 3:
      PORTA &= ~B00001000; //turns off only the prevous steps pins
      PORTA |= B00001001; //turns on only the current steps pins
      break;
      
      case 4:
      PORTA &= ~B00001001; //turns off only the prevous steps pins
      PORTA |= B00000001; //turns on only the current steps pins
      break;
      
      case 5:
      PORTA &= ~B00000001; //turns off only the prevous steps pins
      PORTA |= B00000011; //turns on only the current steps pins
      break;
      
      case 6:
      PORTA &= ~B00000011; //turns off only the prevous steps pins
      PORTA |= B00000010; //turns on only the current steps pins
      break;
      
      case 7:
      PORTA &= ~B00000010; //turns off only the prevous steps pins
      PORTA |= B00000110; //turns on only the current steps pins
      break;
      
      case 8:
      PORTA &= ~B00000110; //turns off only the prevous steps pins
      PORTA |= B00000100; //turns on only the current steps pins
      break;
    }  
  }

  if ((stepSize == 1) && (StepDirection == 1)) { //half steps going backward
    
    switch(sequenceStep){ //selects the correct step

      case 1:
      PORTA &= ~B00001100; //turns off only the prevous steps pins
      PORTA |= B00000100; //turns on only the current steps pins
      break;

      case 2:
      PORTA &= ~B00000100; //turns off only the prevous steps pins
      PORTA |= B00000110; //turns on only the current steps pins
      break;

      case 3:
      PORTA &= ~B00000110; //turns off only the prevous steps pins
      PORTA |= B00000010; //turns on only the current steps pins
      break;

      case 4:
      PORTA &= ~B00000010; //turns off only the prevous steps pins
      PORTA |= B00000011; //turns on only the current steps pins
      break;

      case 5:
      PORTA &= ~B00000011; //turns off only the prevous steps pins
      PORTA |= B00000001; //turns on only the current steps pins
      break;

      case 6:
      PORTA &= ~B00000001; //turns off only the prevous steps pins
      PORTA |= B00001001; //turns on only the current steps pins
      break;

      case 7:
      PORTA &= ~B00001001; //turns off only the prevous steps pins
      PORTA |= B00001000; //turns on only the current steps pins
      break;

      case 8:
      PORTA &= ~B00001000; //turns off only the prevous steps pins
      PORTA |= B00001100; //turns on only the current steps pins
      break;
 
    }
  }
}
