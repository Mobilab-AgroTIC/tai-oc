#include "Arduino.h"

// pin connexions
#define PIN_TRANSISTOR GPIO2
#define PIN_PRESSURE ADC2

int temps = 180; // Frequency to wake up the system, in seconds (DO NOT GO UNDER 300, 5min. Remember of the fair use policy)


// variables for sensor
int val, pressure;

///////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRANSISTOR, OUTPUT);
  digitalWrite(PIN_TRANSISTOR, LOW);
}

///////////////////////////////////////////////////
void loop()
{
  //WakeUp and get battery Voltage
  delay(10);
  uint8_t voltage = getBatteryVoltage()/50; //Voltage, in % (approx)
  Serial.printf("\nVoltage : %d\n", voltage);

  digitalWrite(PIN_TRANSISTOR, HIGH); //activate transistor
  delay(2000);


  // get luminosity
  val = analogRead(PIN_PRESSURE);
  pressure = val*5.00/4096.00*100.00;

  Serial.printf("\n pressure : %d\n", pressure);
  Serial.printf("\n val : %d\n", val);
  
  digitalWrite(PIN_TRANSISTOR, LOW); //Turn off transistor
}
