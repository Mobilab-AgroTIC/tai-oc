#include "LoRaWanMinimal_APP.h"
#include "Arduino.h"
#include <algorithm>

//CLEFS A MODIFIER SELON TTN
const char* APP_EUI = "0000000000000000";                     
const char* DEV_EUI = "0000000000000000";                     
const char* APP_Key = "00000000000000000000000000000000";     

//PARAMETRE CALIB A MODIFIER SELON LA VALEUR MEDIANE DONNEE DANS LE CODE PRECEDENT
int calib = 557;

// pin connexions
#define PIN_TRANSISTOR GPIO2
#define PIN_PRESSURE GPIO1


int temps = 180; // Frequency to wake up the system, in seconds (DO NOT GO UNDER 300, 5min. Remember of the fair use policy)
const int n = 20; // Nombre de valeurs à prendre en compte
int mesures[n];   // Tableau pour stocker les mesures
int ind = 0;    // Indice actuel dans le tableau

const int AppEUI_len = strlen(APP_EUI);
const int DevEUI_len = strlen(DEV_EUI);
const int AppKey_len = strlen(APP_Key);

byte AppEUI_clefConvertie[8];
byte DevEUI_clefConvertie[8];
byte AppKey_clefConvertie[16];

uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t devEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

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
  
  convertirClef(APP_EUI, AppEUI_clefConvertie, AppEUI_len);
  convertirClef(DEV_EUI, DevEUI_clefConvertie, DevEUI_len);
  convertirClef(APP_Key, AppKey_clefConvertie, AppKey_len);
      
  remplirTableau(appEui, AppEUI_clefConvertie, AppEUI_len);
  remplirTableau(devEui, DevEUI_clefConvertie, DevEUI_len);
  remplirTableau(appKey, AppKey_clefConvertie, AppKey_len);

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


void convertirClef(const char* clef, byte* clefConvertie, int longueur) {
    for (int i = 0; i < longueur; i += 2) {
        char byteStr[3] = {clef[i], clef[i + 1], '\0'};
        clefConvertie[i / 2] = strtol(byteStr, NULL, 16);
    }
}

void remplirTableau(uint8_t* tableau, byte* clefConvertie, int longueur) {
    for (int i = 0; i < longueur / 2; i++) {
        tableau[i] = clefConvertie[i];
    }
}
