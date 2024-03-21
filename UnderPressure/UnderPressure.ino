#include "LoRaWanMinimal_APP.h"
#include "Arduino.h"

// pin connexions
#define PIN_TRANSISTOR GPIO2
#define PIN_PRESSURE ADC2

//Set these OTAA parameters to match your app/node in TTN
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t devEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

int temps = 180; // Frequency to wake up the system, in seconds (DO NOT GO UNDER 300, 5min. Remember of the fair use policy)

// Parameters for LoRaWAN
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };
static uint8_t counter=0;
uint8_t lora_data[2];
uint8_t downlink ;


// variables for sensor
int val, pressure;

//Some utilities for going into low power mode
TimerEvent_t sleepTimer;
//Records whether our sleep/low power timer expired
bool sleepTimerExpired;
static void wakeUp()
{
  sleepTimerExpired=true;
}
static void lowPowerSleep(uint32_t sleeptime)
{
  sleepTimerExpired=false;
  TimerInit( &sleepTimer, &wakeUp );
  TimerSetValue( &sleepTimer, sleeptime );
  TimerStart( &sleepTimer );
  while (!sleepTimerExpired) lowPowerHandler();
  TimerStop( &sleepTimer );
}

///////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  pinMode(PIN_TRANSISTOR, OUTPUT);
  digitalWrite(PIN_TRANSISTOR, LOW);

  LoRaWAN.begin(LORAWAN_CLASS, ACTIVE_REGION);
  LoRaWAN.setAdaptiveDR(true);
  while (1) {
    Serial.print("Joining... ");
    LoRaWAN.joinOTAA(appEui, appKey, devEui);
    if (!LoRaWAN.isJoined()) {
      Serial.println("JOIN FAILED! Sleeping for 30 seconds");
      lowPowerSleep(30000);
    } else {
      Serial.println("JOINED");
      break;
    }
  }
}


///////////////////////////////////////////////////
void loop()
{
  //WakeUp and get battery Voltage
  counter++; 
  delay(10);
  uint8_t voltage = getBatteryVoltage()/50; //Voltage, in % (approx)
  Serial.printf("\nVoltage : %d\n", voltage);
  lora_data[0] = voltage;

  digitalWrite(PIN_TRANSISTOR, HIGH); //activate transistor
  delay(2000);


  // get luminosity
  val = analogRead(PIN_PRESSURE);
  pressure = val*5.00/4096.00*100.00;
  lora_data[1] = pressure;
  Serial.printf("\n pressure : %d\n", pressure);
  Serial.printf("\n val : %d\n", val);
  
  digitalWrite(PIN_TRANSISTOR, LOW); //Turn off transistor
  
  //Now send the data.
  Serial.printf("\nSending packet with counter=%d\n", counter);
  Serial.printf("\nValue to send 1: %d\n", lora_data[1]);
  //Here we send confirmed packed (ACK requested) only for the first two (remember there is a fair use policy)
  bool requestack=counter<2?true:false;
  if (LoRaWAN.send(sizeof(lora_data), lora_data, 1, requestack)) {  // The parameters are "data size, data pointer, port, request ack"
    Serial.println("Send OK");
  } else {
    Serial.println("Send FAILED");
  }
  lowPowerSleep(temps*1000);  
  delay(10);
}
