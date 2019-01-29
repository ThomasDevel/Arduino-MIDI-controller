#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Namespace.h>
#include <midi_Settings.h>

/*
 * SN74HC165N_shift_reg
 *
 * Program to shift in the bit values from a SN74HC165N 8-bit
 * parallel-in/serial-out shift register.
 *
 * This sketch demonstrates reading in 16 digital states from a
 * pair of daisy-chained SN74HC165N shift registers while using
 * only 4 digital pins on the Arduino.
 *
 * You can daisy-chain these chips by connecting the serial-out
 * (Q7 pin) on one shift register to the serial-in (Ds pin) of
 * the other.
 * 
 * Of course you can daisy chain as many as you like while still
 * using only 4 Arduino pins (though you would have to process
 * them 4 at a time into separate unsigned long variables).
 * 
 */

/* How many shift register chips are daisy-chained */
#define NUMBER_OF_SHIFT_CHIPS   1

/* Width of data (how many ext lines) */
#define DATA_WIDTH   NUMBER_OF_SHIFT_CHIPS * 8

/* Width of pulse to trigger the shift register to read and latch */
#define PULSE_WIDTH_USEC 8

/* Optional delay between shift register reads */
#define POLL_DELAY_MSEC  0

/* You will need to change the "int" to "long" If the
 * NUMBER_OF_SHIFT_CHIPS is higher than 2.
 */
#define BYTES_VAL_T unsigned int


/* ______PINS______ */

int qh1 = 2;
int qh2 = 3;
/* clk of serial out shift registers but also paralel out shift registers */
int clk = 4;
int sh = 5;
int octMin = 6;
int octPlus = 7;

int controlButton = 8;
int controlLed = 9;

/* LEDS */
int ser = 10;
int rclk = 11;

/* ANALOG POTS */
int s0 = A0;     
int s1 = A1;    
int s2 = A2; 
int analoogIn = A3; 

/*______PINS______ */

/*______VARIABLES_____ */

unsigned int channel;

BYTES_VAL_T pinValues;
BYTES_VAL_T pinValues2;
BYTES_VAL_T oldPinValues;
//BYTES_VAL_T oldPinValues2;
BYTES_VAL_T pinControlVal;
BYTES_VAL_T pinControlVal2;
BYTES_VAL_T oldPinControlVal;
BYTES_VAL_T controlState_0;
BYTES_VAL_T oldControlState_0;

BYTES_VAL_T controlState_1;
BYTES_VAL_T oldControlState_1;

BYTES_VAL_T controlState_2;
BYTES_VAL_T oldControlState_2;



byte controlMode;
byte buttonState;
byte oldButtonState;

/* Debounce pushbuttons: */
long timeLastPush[15];
long debounceDelay = 50;

int valuePotMeterX;
int potMeterArray[7];

/*______VARIABLES_____ */

/*_______READ_SHIFT_REGS______ */

/* This function is essentially a "shift-in" routine reading the
 * serial Data from the shift register chips and representing
 * the state of those pins in an unsigned integer (or long).
 */
BYTES_VAL_T read_shift_regs(int Input)
{
    byte bitVal;
    BYTES_VAL_T bytesVal = 0;

    /* Trigger a parallel Load to latch the state of the data lines */
    digitalWrite(sh, LOW);
    //delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(sh, HIGH);

    /* Loop to read each bit value from the serial out line of the SN74HC165N */
    for(int i = 0; i < DATA_WIDTH; i++)
    {
        bitVal = digitalRead(Input);


        /* Set the corresponding bit in bytesVal */
        bytesVal |= (bitVal << ((DATA_WIDTH-1) - i));

        /* Pulse the Clock (rising edge shifts the next bit) */
        digitalWrite(clk, HIGH);
        //delayMicroseconds(PULSE_WIDTH_USEC);
        digitalWrite(clk, LOW);
    }
    return(bytesVal);
}
/* _______READ SHIFT REGS______ */

/* _______CHECK_LEDS______ */

void checkLeds(int pinValues1en2){
    digitalWrite(rclk,LOW);
    digitalWrite(clk,LOW);
    for(int i = 0; i < 16; i++){
        digitalWrite(ser,bitRead(pinValues1en2,15-i));
        digitalWrite(clk,HIGH);
        digitalWrite (clk,LOW);
    } 
    digitalWrite(rclk,HIGH);
}

/* _______CHECK_LEDS_______ */


/* _______MIDI_NOTES______ */

void playMidiNotes(int pinValuesX, int oldPinValuesX, int octaveX, int channel){

    for(int i = 0; i < 16; i++)
    {
        if((millis() - timeLastPush[i]) > debounceDelay){

            if(((~oldPinValuesX & pinValuesX) >> i) & 1){
                MIDI.sendNoteOn(20+i+(octaveX*16),127,channel);
                timeLastPush[i] = millis();
            }
            if(((oldPinValuesX & ~pinValuesX) >> i) & 1){
                MIDI.sendNoteOff(20+i+(octaveX*16),0,channel);
                timeLastPush[i] = millis();
            }
        }
    }
}

/*_______MIDI_NOTES_______ */


/* ______CONTROL_MIDI_NOTES______ */

void bedienMidiNoten(int pinControlValX, int oldPinControlValX , int controlStateX, int oldControlStateX, int octaveX){

    //Serial.print("control state  " + controlStateX);
    //Serial.println(controlStateX);
    //Serial.print("old control state  " + oldControlStateX);
    //Serial.println(oldControlStateX);

    for(int i = 0; i < 16 ; i++){

        if((millis() - timeLastPush[i]) > debounceDelay){

            /* Only if a pin value at a certain position has changed do we have to check if an effect should be turned on or off */ 
            if(bitRead(pinControlValX,i) != bitRead(oldPinControlValX,i)){
                if(((~controlStateX & pinControlValX) >> i) & 1 && (bitRead(oldControlStateX, i) == 0)){
                    //&& (oldControlStateX >> i == 0)
                    MIDI.sendControlChange(1+i+(octaveX*16),127,1);
                    // because an effect has just been turned on, switch controlstate at position i from 0 -> 1 */
                    setBedienToestand(i,octaveX);
                    timeLastPush[i] = millis();
                }  
                if(((controlStateX & pinControlValX) >> i ) & 1 && (bitRead(oldControlStateX, i)  == 1) ){ 
                    //&& (oldControlStateX >> i == 1)
                    MIDI.sendControlChange(1+i+(octaveX*16),0,1);
                    // because an effect has just been turned off, switch controlstate at position i from 1 -> 0 */
                    setBedienToestand(i,octaveX);
                    timeLastPush[i] = millis();
                }
            }
        }
    }
}

/* ______CONTROL_MIDI_NOTES______ */


/* ______SET_CONTROLSTATE_______ */

void setBedienToestand(int i, int octaveX){
    if(octaveX == 0){
        controlState_0 = controlState_0 ^ ( B00000001 << i);
    }
    if(octaveX == 1){
        controlState_1 = controlState_1 ^ ( B00000001 << i);
    }
    if(checkOctave() == 2 ){
        controlState_2 = controlState_2 ^ ( B00000001 << i);
    }
}

/* ______SET_CONTROLSTATE_______ */


/* ______CHECK_OCTAVE______ */

int checkOctave(){
    if(digitalRead(octMin) == 1 && digitalRead(octPlus) == 0 ){
        return(0);
    }
    if(digitalRead(octPlus) == 1 && digitalRead(octMin) == 0){
        return(2);
    }
    else if(digitalRead(octPlus) == 0 && digitalRead(octMin) == 0){
        return(1);
    }
}

/* ______CHECK_OCTAVE______ */


/* _____CHECK_CONTROLSTATE______ */

void checkBedienKnopToestand(){
    buttonState = digitalRead(controlButton);

    if(buttonState == 1 && !oldButtonState){
        controlMode = ~controlMode;
    }    
    digitalWrite(controlLed, controlMode);
    oldButtonState = buttonState;  
}

/* _____CHECK_CONTROLSTATE______ */

/* _____CHECK_CHANNEL______ */

int checkChannel(int pinValuesX){
    int channel = 1;
    for(int i = 0; i < 16; i++)
    {
        if( (pinValuesX >> i) & 1){
            channel = i+1;
        } 
    }

    digitalWrite(controlLed, HIGH);
    delay(80);
    digitalWrite(controlLed, LOW);
    delay(80);
    digitalWrite(controlLed, HIGH);
    delay(80);
    digitalWrite(controlLed, LOW);
    delay(80);
    digitalWrite(controlLed, HIGH);
    delay(80);
    digitalWrite(controlLed, LOW);
    delay(80);
    digitalWrite(controlLed, HIGH);
    delay(80);
    digitalWrite(controlLed, LOW);

    controlMode = ~controlMode;
    digitalWrite(controlLed, controlMode);
    return(channel);
}

/* _____CHECK_CHANNEL______ */

/* _____CHECK_ANALOG______ */

void checkAnalog(){

    for (int i=0; i<3 ;i++) { 
        digitalWrite(s0, LOW);
        digitalWrite(s1, LOW);
        digitalWrite(s2, LOW);

        if(i == 1 || i == 3 || i == 5 || i == 6 ){
            digitalWrite(s0, HIGH);
        }   
        if(i == 2 || i == 3 || i == 6  || i == 7) {
            digitalWrite(s1, HIGH);
        }
        if(i > 3) {
            digitalWrite(s2, HIGH);
        }  
        sendAnalogValues(i);
    }

}

/* _____CHECK_ANALOG______ */

void sendAnalogValues(int potMeterX){
    valuePotMeterX = analogRead(analoogIn);
    valuePotMeterX = valuePotMeterX/8;
    if(valuePotMeterX != potMeterArray[potMeterX]){
        MIDI.sendControlChange(49+potMeterX,valuePotMeterX,1);
        potMeterArray[potMeterX] = valuePotMeterX;
    }
}

/* _______SETUP_______ */

void setup(){
    Serial.begin(115200);

    /* Initialize our digital pins...*/
    pinMode(sh, OUTPUT);
    pinMode(clk, OUTPUT);
    pinMode(qh1, INPUT);
    pinMode(qh2, INPUT);

    digitalWrite(clk, LOW);
    digitalWrite(sh, HIGH);

    pinMode(controlButton, INPUT);
    pinMode(controlLed, OUTPUT);

    pinMode(octMin,INPUT);
    pinMode(octPlus,INPUT);

    pinMode(ser,OUTPUT);
    pinMode(rclk, OUTPUT);

    pinMode(A0, OUTPUT);   
    pinMode(A1, OUTPUT);   
    pinMode(A2, OUTPUT);    

    pinMode(analoogIn, INPUT);

    pinValues = read_shift_regs(qh1);
    pinValues2 = read_shift_regs(qh2);
    oldPinValues = pinValues;

    controlState_0 = 0;
    oldControlState_0 = 0;

    controlState_1 = 0;
    oldControlState_1 = 0;

    controlState_2 = 0;
    oldControlState_2 = 0;


    buttonState = 0;
    oldButtonState = 0;
    controlMode = LOW;

    channel = 1;

    //    for(int i = 0; i < 8; i++){
    //       potMeterArray[i] = analogRead(analoogIn);
    //    }

    digitalWrite(controlLed, HIGH);
    checkLeds(32768);
    delay(200);
    checkLeds(52224);
    delay(200);
    checkLeds(61152);
    delay(200);
    checkLeds(65535);    
    delay(200);
    checkLeds(0);
    delay(200);
    checkLeds(64399);
    delay(200);
    checkLeds(0);
    delay(200);
    checkLeds(64399);
    delay(200);
    digitalWrite(controlLed, LOW);

}

/* _______SETUP_______ */

/* _______LOOP________ */

void loop()
{
    /* Read the state of all zones. */

    if(controlMode == 0 && digitalRead(controlButton) == 0){
        pinValues = read_shift_regs(qh1);
        pinValues2 = read_shift_regs(qh2); //
        pinValues = (pinValues2 << 8) + pinValues;
        checkLeds(pinValues);

        /* If there was a chage in state, display which ones changed.*/
        if(pinValues != oldPinValues)
        {
            playMidiNotes(pinValues, oldPinValues, checkOctave(), channel);
        }

        oldPinValues = pinValues;
    }


    if (controlMode != 0 && digitalRead(controlButton) == 0){
        pinControlVal = read_shift_regs(qh1);
        pinControlVal2 = read_shift_regs(qh2);
        pinControlVal = ((pinControlVal2 << 8) + pinControlVal);


        if(checkOctave() == 0){
            if(pinControlVal != oldPinControlVal){
                bedienMidiNoten(pinControlVal, oldPinControlVal, controlState_0, oldControlState_0, 0);
            }
            checkLeds(controlState_0);
        }
        if(checkOctave() == 1){
            if(pinControlVal != oldPinControlVal){
                bedienMidiNoten(pinControlVal, oldPinControlVal, controlState_1, oldControlState_1, 1);
            }
            checkLeds(controlState_1);
        }
        if(checkOctave() == 2){
            if(pinControlVal != oldPinControlVal){
                bedienMidiNoten(pinControlVal, oldPinControlVal, controlState_2, oldControlState_2, 2);
            }
            checkLeds(controlState_2);
        }

        oldPinControlVal = pinControlVal;

        oldControlState_0 = controlState_0;
        oldControlState_1 = controlState_1;
        oldControlState_2 = controlState_2;

    }

    if(digitalRead(controlButton) == 1) {
        checkLeds(0);
        //if(controlMode != 0 ) {checkLeds(B00000001 << channel-1);}
        pinValues = read_shift_regs(qh1);
        pinValues2 = read_shift_regs(qh2);

        pinValues = pinValues = (pinValues2 << 8) + pinValues;
        checkLeds(pinValues);     
        if(pinValues != oldPinValues ){
            channel = checkChannel(pinValues);
        }
        oldPinValues = pinValues;
    }

    checkBedienKnopToestand();
    checkAnalog();

    //delay(POLL_DELAY_MSEC);
}
/* _____LOOP_____ */
