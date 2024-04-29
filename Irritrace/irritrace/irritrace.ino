#include "LoRaWanMinimal_APP.h"
#include "Arduino.h"
#include <algorithm>

// pin connexions
#define PIN_TRANSISTOR GPIO2
#define PIN_PRESSURE ADC2

//Set these OTAA parameters to match your app/node in TTN
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t devEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

int calib = 557;

int temps = 180; // Frequency to wake up the system, in seconds (DO NOT GO UNDER 300, 5min. Remember of the fair use policy)
const int n = 20; // Nombre de valeurs à prendre en compte
int mesures[n];   // Tableau pour stocker les mesures
int ind = 0;    // Indice actuel dans le tableau

// Parameters for LoRaWAN
uint16_t userChannelsMask[6]={ 0x00FF,0x0000,0x0000,0x0000,0x0000,0x0000 };
static uint8_t counter=0;
uint8_t lora_data[2];
uint8_t downlink ;


// variables for sensor
int val, prescbar;
float pressure, voltage;

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

  // Remplissage du tableau
  while (ind < n) {
    mesures[ind] = analogRead(PIN_PRESSURE); // Lecture de la valeur du capteur
    delay(20); // Attendez 20ms avant la prochaine lecture
    ind++; // Incrémentez l'indice
  }

  digitalWrite(PIN_TRANSISTOR, LOW); //Turn off transistor

  // Calcul de la médiane et de la moyenne
  int mediane = calculerMedian();
  float moyenneFiltree = calculerMoyenneSansOutliers();
  /*
  Serial.println("Valeurs du tableau :");
  for (int i = 0; i < n; i++) {
    Serial.print(mesures[i]);
    Serial.print(" ");
  }
  Serial.println();*/
  
  // Affichage des résultats
  Serial.print("Mediane: ");
  Serial.println(mediane);
  //Serial.print("Moyenne sans outliers: ");
  //Serial.println(moyenneFiltree);

  // Réinitialisation pour la prochaine série de mesures
  ind = 0;
  
  voltage = mediane*2.400/4096.000;
  pressure = ((mediane-calib)*2.400/4096.000)*4+1;
  prescbar = 100*pressure;
  
  Serial.print("Val :");
  Serial.println(val);
  Serial.print("voltage :");
  Serial.println(voltage,3); 
  Serial.print("pressure :");
  Serial.println(pressure,3);

  lora_data[1] = highByte(prescbar);
  lora_data[2] = lowByte(prescbar);

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

// Fonction pour calculer la médiane
int calculerMedian() {
  int valeursTriees[n];
  memcpy(valeursTriees, mesures, sizeof(mesures));
  std::sort(valeursTriees, valeursTriees + n);
  return valeursTriees[n / 2];
}

// Fonction pour calculer la moyenne sans les outliers
float calculerMoyenneSansOutliers() {
  int mediane = calculerMedian();
  int somme = 0;
  int nombreDeValeurs = 0;
  for (int i = 0; i < n; i++) {
    if (abs(mesures[i] - mediane) <= 2) {
      somme += mesures[i];
      nombreDeValeurs++;
    }
  }
  return somme / (float)nombreDeValeurs;
}
