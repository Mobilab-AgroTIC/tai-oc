#include "Arduino.h"

// pin connexions
#define PIN_TRANSISTOR GPIO2
#define PIN_PRESSURE ADC2


// variables for sensor
int val;

///////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRANSISTOR, OUTPUT);
  digitalWrite(PIN_TRANSISTOR, HIGH);
}

///////////////////////////////////////////////////
void loop()
{
  //WakeUp and get battery Voltage

  delay(50);
  val = analogRead(PIN_PRESSURE); // Lecture de la valeur du capteur

  Serial.print("Val :");
  Serial.println(val);
}
