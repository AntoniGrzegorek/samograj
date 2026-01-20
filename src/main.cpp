#include <Arduino.h>
#include <ArduinoJson.h>
#include <LITTLEFS.h>
#include "interrupts.h"

// led i przyciski
const int LED = 6; // J1_6
const int BUTTON1 = 5; // J1_4
const int BUTTON2 = 4; // J1_5

// Serwa
const int MOTOR1 = 9;
const int MOTOR2 = 10;
const int MOTOR3 = 11;
const int MOTOR4 = 12;
const int MOTOR5 = 13;
const int MOTOR6 = 14;

//magnesy
const int MAGNES1 = 43;   // EM1
const int MAGNES2 = 44;  // EM2
const int MAGNES3 = 1;  // EM3
const int MAGNES4 = 2;   // EM4

const int MAGNES5 = 42;  // EM5
const int MAGNES6 = 41;  // EM6 (Schemat mówi GPIO45 dla J3_15, na liście miałeś błąd)
const int MAGNES7 = 40;  // EM7
const int MAGNES8 = 39;  // EM8

const int MAGNES9 = 38;   // EM9
const int MAGNES10 = 37;  // EM10 (UWAGA: Na schemacie EM10 i EM22 to ten sam pin? Sprawdź to!)
const int MAGNES11 = 36;  // EM11
const int MAGNES12 = 35; // EM12

const int MAGNES13 = 48; // EM13
const int MAGNES14 = 47; // EM14
const int MAGNES15 = 21; // EM15
const int MAGNES16 = 20; // EM16

const int MAGNES17 = 19; // EM17
const int MAGNES18 = 7; // EM18 (Poprawiono z J3_3)
const int MAGNES19 = 15; // EM19
const int MAGNES20 = 16; // EM20

const int MAGNES21 = 17; // EM21
const int MAGNES22 = 18;  // EM22 (Konflikt z EM10? Sprawdź J3_20 vs J3_4)
const int MAGNES23 = 8; // EM23
const int MAGNES24 = 3; // EM24

// Tablica pinów magnesów
int MAGNET_PINS[24] = {
  MAGNES1, MAGNES2, MAGNES3, MAGNES4, 
  MAGNES5, MAGNES6, MAGNES7, MAGNES8, 
  MAGNES9, MAGNES10, MAGNES11, MAGNES12,
  MAGNES13, MAGNES14, MAGNES15, MAGNES16, 
  MAGNES17, MAGNES18, MAGNES19, MAGNES20, 
  MAGNES21, MAGNES22, MAGNES23, MAGNES24
};

// Tablica pinów motorków
int MOTOR_PINS[6] = {MOTOR1, MOTOR2, MOTOR3, MOTOR4, MOTOR5, MOTOR6};

// Kanały PWM
const int PWM_FREQ = 50;
const int PWM_RES = 14; // ESP32-S3 ma lepszą rozdzielczość
const int MOTOR_CHANNELS[6] = {0, 1, 2, 3, 4, 5};

volatile bool eightNoteTick = false;
hw_timer_t *bpmTimer = NULL;

// Deklaracje funkcji
int count_JSONs(); //liczenie JSONów
void graj(); //funkcja odtwarzająca utwór
void pwm(int motorChannel, int position);
void odliczaj(); //funkcja odliczająca czasy magnesów
void zapisz_nuty(); //funkcja zapisująca aktualne nuty do timera
void nowaFunkcjaMagnesow(int magnetIndex, int durationMs);
void grajZJson(const char* jsonInput);
void motorki(int nuta);

//zmienne globalne
bool running = false; // Stan odtwarzania
int JSON_count; //liczba JSONów w pamięci
int song_index = 0; //aktualny indeks JSONa
bool przerwanie = false; //czy było przerwanie od ostatniej iteracji pętli
int osemkowe_takty = 0; //liczba ósemkowych taktów od startu utworu
int takt = 0; //aktualny takt
JsonArray nuty; //dokument JSON
int stan_motorkow[6]; //stan motorków

void setup() {
  Serial.begin(115200);

  // Inicjalizacja PWM
  for(int i=0; i<6; i++) {
     ledcSetup(MOTOR_CHANNELS[i], PWM_FREQ, PWM_RES);
     ledcAttachPin(MOTOR_PINS[i], MOTOR_CHANNELS[i]);
  }

//pinmode'y
  pinMode(LED, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  
  for(int i=0; i<24; i++) {
    pinMode(MAGNET_PINS[i], OUTPUT);
    digitalWrite(MAGNET_PINS[i], 0);
  }

  //Inicjalizacja LittleFS
  if(!LITTLEFS.begin()){
    Serial.println("Błąd wczytywania pamięci flash");
    return;
  }

  //Liczenie JSONów
  JSON_count = count_JSONs();

  //ustawinie początkowych stanów motorków
  for(int i=0; i<6; i++) {
    stan_motorkow[i] = -1;
    pwm(MOTOR_CHANNELS[i], -1);
  }
}

void loop() {
  // pętla główna
  if(digitalRead(BUTTON1) == HIGH) {
    //start
    if(!running) {
      running = true;
      digitalWrite(LED, HIGH);
      graj();
    }
    //stop
    else {
      running = false;
      digitalWrite(LED, LOW);
    }
  delay(50); // Debounce
  }
  if(digitalRead(BUTTON2)==HIGH)
  {
    if(!running)
    {
      //następny utwór
      song_index++;
      if(song_index>=JSON_count)
      {
        song_index=0;
      }
      delay(50); //debounce
    }
  }
  if(running && przerwanie){
    odliczaj();
    zapisz_nuty();
    przerwanie = false;
  }
}

int count_JSONs() {
  int count = 0;
  File root = LITTLEFS.open("/");
  File file = root.openNextFile();
  while(file) {
    String filename = file.name();
    if(filename.endsWith(".json")) {
      count++;
    }
    file = root.openNextFile();
  }
  return count;
}

void graj() {
  // Odczytanie odpowiedniego pliku JSON
  String filename = "/" + String(song_index) + ".json";
  File file = LITTLEFS.open(filename, "r");
  if(!file) {
    Serial.println("Nie można otworzyć pliku: " + filename);
    return;
  }

  String jsonContent = "";
  while(file.available()) {
    jsonContent += char(file.read());
  }
  file.close();

  // Odtwarzanie z JSON
  grajZJson(jsonContent.c_str());
}

void odliczaj(){
for (int i = 0; i < MAGNET_COUNT; i++) {
    // Jeśli czas jest większy od zera -> dekrementuj
    if (magnetTimers[i] > 0) {
      magnetTimers[i]--;
      
      // Jeśli wartość spadła do 0 w tym cyklu -> wyłącz magnes
      if (magnetTimers[i] == 0) {
        digitalWrite(MAGNET_PINS[i], 0); // Wyłącz pin
      }
    }
  }
}

void pwm(int motorChannel, int position) {
  // Kalibracja dla 14-bit PWM (0-16383). 
  // Stare wartości (13, 20, 26) były dla 8-bit (0-255).
  // Musisz dobrać te liczby eksperymentalnie! Poniżej przykłady:
  int center = 1229; // 1,5ms
  int left = 819;    // 1ms
  int right = 1638;  // 2ms

  if(position == -1) ledcWrite(motorChannel, left);
  else if(position == 0) ledcWrite(motorChannel, center);
  else if(position == 1) ledcWrite(motorChannel, right);
}

void nowaFunkcjaMagnesow(int magnetIndex, int durationMs) {
  if (magnetIndex < 0 || magnetIndex >= 24) return;
  digitalWrite(MAGNET_PINS[magnetIndex], 1);
  noInterrupts();
  magnetTimers[magnetIndex] = durationMs;
  interrupts();
}

void zapisz_nuty() {
  takt++;
  if(takt > osemkowe_takty) {
    takt = 0;
    timerAlarmDisable(bpmTimer);
    timerEnd(bpmTimer);
    running = false;
  }
  else{
    int nuta = nuty[takt]["n"];
    int dlugosc = nuty[takt]["d"];
    if(nuta!=-1){
      nowaFunkcjaMagnesow(nuta, dlugosc);
      delay(100);
      motorki(nuta);
    }
  }
}

void motorki(int nuta) {
  //sterowanie motorkami na podstawie nut
  if(nuta >= 1 && nuta <= 4) {
    stan_motorkow[0] = -stan_motorkow[0]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[0], stan_motorkow[0]); // Ruch do pozycji aktywnej
    }
  if(nuta >= 5 && nuta <= 8) {
    stan_motorkow[1] = -stan_motorkow[1]; // Zmiana stanu motorka 2
    pwm(MOTOR_CHANNELS[1], stan_motorkow[1]); // Ruch do pozycji aktywnej
    }
  if(nuta >= 9 && nuta <= 12) {
    stan_motorkow[2] = -stan_motorkow[2]; // Zmiana stanu motorka 3
    pwm(MOTOR_CHANNELS[2], stan_motorkow[2]); // Ruch do pozycji aktywnej
    }
  if(nuta >= 13 && nuta <= 16) {
    stan_motorkow[3] = -stan_motorkow[3]; // Zmiana stanu motorka 4
    pwm(MOTOR_CHANNELS[3], stan_motorkow[3]); // Ruch do pozycji aktywnej
    }
  if(nuta >= 17 && nuta <= 20) {
    stan_motorkow[4] = -stan_motorkow[4]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[4], stan_motorkow[4]); // Ruch do pozycji aktywnej
    }
  if(nuta >= 21 && nuta <= 24) {
    stan_motorkow[5] = -stan_motorkow[5]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[5], stan_motorkow[5]); // Ruch do pozycji aktywnej
    }
}

void grajZJson(const char* jsonInput) {
  // Parsowanie JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonInput);
  if (error) return;

  //wczytanie tempa
  int bpm = doc["bpm"];

  //inicjalizacja timera
  initDurationTimer(bpm);

  nuty = doc["notes"];
  osemkowe_takty = nuty.size();
  takt = 0;
}