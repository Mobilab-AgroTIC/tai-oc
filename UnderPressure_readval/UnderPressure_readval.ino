#include "Arduino.h"
#include <algorithm>


// pin connexions
#define PIN_TRANSISTOR GPIO2
#define PIN_PRESSURE ADC2

int temps = 180; // Frequency to wake up the system, in seconds (DO NOT GO UNDER 300, 5min. Remember of the fair use policy)
const int n = 20; // Nombre de valeurs à prendre en compte
int mesures[n];   // Tableau pour stocker les mesures
int ind = 0;    // Indice actuel dans le tableau


// variables for sensor
int val;
float pressure, voltage;
int calib = 557;

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
  uint8_t battery = getBatteryVoltage()/50; //Voltage, in % (approx)

  digitalWrite(PIN_TRANSISTOR, HIGH); //activate transistor
  delay(20);

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

  Serial.print("Val :");
  Serial.println(val);
  Serial.print("voltage :");
  Serial.println(voltage,3); 
  Serial.print("pressure :");
  Serial.println(pressure,3);



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
