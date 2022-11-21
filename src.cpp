#include <Arduino.h>
#include <INA.h>
#include <U8glib.h>
#include <stdlib.h>
#include <Wire.h>

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);
INA_Class INA;


///////////////
// Calibration:
///////////////
const unsigned long ZeroTimeout = 100000; // For high response time, a good value would be 100000.
                                          // For reading very low RPM, a good value would be 300000.
/////////////
// Variables:
/////////////
volatile unsigned long LastTimeWeMeasured ;                       // Stores the last time we measured a pulse so we can calculate the period.
volatile unsigned long LastValidTime ;                            // Stores the last valid time we measured a pulse so we can calculate the period.
volatile unsigned long PeriodBetweenPulses ;                      // Stores the period between pulses in microseconds.
unsigned int PulseCounter = 1;                                   // Counts the amount of pulse readings we took so we can average multiple pulses before calculating the period.
unsigned int PulseEvent = 1;
unsigned long PeriodSum; // Stores the summation of all the periods to do the average.
float circunferenciaRoda = 1.596; // Roda do DT1
float velocidadeKm;               // Velocidade em km/h
unsigned int ZeroDebouncingExtra; // Stores the extra value added to the ZeroTimeout to debounce it.
                                  // The ZeroTimeout needs debouncing so when the value is close to the threshold it
                                  // doesn't jump from 0 to the value. This extra value changes the threshold a little
                                  // when we show a 0.

unsigned long LastTimeCycleMeasure = LastTimeWeMeasured; // Stores the last time we measure a pulse in that cycle.
                                                         // We need a variable with a value that is not going to be affected by the interrupt
                                                         // because we are going to do math and functions that are going to mess up if the values
                                                         // changes in the middle of the cycle.
unsigned long CurrentMicros = micros()/1000;                  // Stores the micros in that cycle.
                                                         // We need a variable with a value that is not going to be affected by the interrupt
                                                         // because we are going to do math and functions that are going to mess up if the values
                                                         // changes in the middle of the cycle.


void Pulse_Event() // The interrupt runs this to calculate the period between pulses:
{
  PulseEvent ++;
  LastTimeWeMeasured = micros()/1000; // Stores the current micros so the next time we have a pulse we would have something to compare with.
  //if((LastTimeWeMeasured - LastValidTime)>400){
  LastValidTime = LastTimeWeMeasured;
  PeriodBetweenPulses = micros()/1000 - LastTimeWeMeasured; // Current "micros" minus the old "micros" when the last pulse happens.
                                                       // This will result with the period (microseconds) between both pulses.
                                                       // The way is made, the overflow of the "micros" is not going to cause any issue.
  
  PulseCounter++;                              // Increase the counter for amount of readings by 1.
  PeriodSum = PeriodSum + PeriodBetweenPulses; // Add the periods so later we can average.
  }
//} // End of Pulse_Event.   

void setup() // Start of setup:
{

  Serial.begin(9600);                                             // Begin serial communication.
  attachInterrupt(digitalPinToInterrupt(2), Pulse_Event, RISING); // Enable interruption pin 2 when going from LOW to HIGH.

  INA.begin(80, 1000);
  INA.setBusConversion(8500);
  INA.setShuntConversion(8500);
  INA.setAveraging(128);
  INA.alertOnConversion(true);
  INA.setMode(INA_MODE_CONTINUOUS_BOTH);
  INA.alertOnBusOverVoltage(true, 40000);

  u8g.begin();
} // End of setup.

void loop() // Start of loop:
{

  // The following is going to store the two values that might change in the middle of the cycle.
  // We are going to do math and functions with those values and they can create glitches if they change in the
  // middle of the cycle.
  LastTimeCycleMeasure = LastTimeWeMeasured; // Store the LastTimeWeMeasured in a variable.
  CurrentMicros = micros()/1000;                  // Store the micros() in a variable.

  // CurrentMicros should always be higher than LastTimeWeMeasured, but in rare occasions that's not true.
  // I'm not sure why this happens, but my solution is to compare both and if CurrentMicros is lower than
  // LastTimeCycleMeasure I set it as the CurrentMicros.
  // The need of fixing this is that we later use this information to see if pulses stopped.
  if (CurrentMicros < LastTimeCycleMeasure)
  {
    LastTimeCycleMeasure = CurrentMicros;
  }
/*
  // Detect if pulses stopped or frequency is too low, so we can show 0 Frequency:
  if (PeriodBetweenPulses > ZeroTimeout - ZeroDebouncingExtra || CurrentMicros - LastTimeCycleMeasure > ZeroTimeout - ZeroDebouncingExtra)
  {                             // If the pulses are too far apart that we reached the timeout for zero:
    velocidadeKm = 0;           // Set frequency as 0.
    ZeroDebouncingExtra = 1000; // Change the threshold a little so it doesn't bounce.
  }
  else
  {
    ZeroDebouncingExtra = 0; // Reset the threshold to the normal value so it doesn't bounce.
  }

*/
 
  // Print information on the serial monitor:
  // Comment this section if you have a display and you don't need to monitor the values on the serial monitor.
  // This is because disabling this section would make the loop run faster.
  Serial.print("PulseEvent: ");
  Serial.print(PulseEvent);
  Serial.print("tempo: ");
  Serial.print(PeriodSum);
  Serial.print("\tmedicoes: ");
  Serial.println(PulseCounter);
  Serial.print("\tON: ");
  Serial.println(digitalRead(2));

   u8g.firstPage();
  do {
    u8g.setFont(u8g_font_courR10);
    u8g.setPrintPos(0, 15);
    u8g.print(LastTimeWeMeasured);
    u8g.print(" LTWM");

    u8g.setPrintPos(0, 37);
    u8g.print(digitalRead(2));
    u8g.print(" ON");
    
    u8g.setPrintPos(0, 60);
    u8g.print(PulseCounter);
    u8g.print("medicoes");

  } while ( u8g.nextPage() );

  // Velocidade em km/h

  velocidadeKm = circunferenciaRoda / (3.0*PeriodBetweenPulses);
 
} // End of loop.
