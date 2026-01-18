#include <Arduino.h>
#include <ArduinoJson.h>
#include "interrupts.h"

// --- 1. POPRAWIONE PINY WG SCHEMATU ESP32-S3 ---
const int LED = 4; // J1_6
const int BUTTON1 = 5; // J1_4
const int BUTTON2 = 6; // J1_5

// Serwa (J1_15 do J1_20)
const int MOTOR1 = 9;
const int MOTOR2 = 10;
const int MOTOR3 = 11;
const int MOTOR4 = 12;
const int MOTOR5 = 13;
const int MOTOR6 = 14;

// Magnesy (Odczytane z Twojego schematu image_6dc548.jpg)
// Uporządkowane logicznie: Struna 1 (EM1-4), Struna 2 (EM5-8)...
// Sprawdź fizycznie czy EM1 to faktycznie 1 próg 1 struny!
const int MAGNES1 = 8;   // EM1
const int MAGNES2 = 15;  // EM2
const int MAGNES3 = 16;  // EM3
const int MAGNES4 = 7;   // EM4

const int MAGNES5 = 35;  // EM5
const int MAGNES6 = 45;  // EM6 (Schemat mówi GPIO45 dla J3_15, na liście miałeś błąd)
const int MAGNES7 = 36;  // EM7
const int MAGNES8 = 39;  // EM8

const int MAGNES9 = 2;   // EM9
const int MAGNES10 = 1;  // EM10 (UWAGA: Na schemacie EM10 i EM22 to ten sam pin? Sprawdź to!)
const int MAGNES11 = 3;  // EM11
const int MAGNES12 = 48; // EM12

const int MAGNES13 = 37; // EM13
const int MAGNES14 = 38; // EM14
const int MAGNES15 = 40; // EM15
const int MAGNES16 = 41; // EM16

const int MAGNES17 = 42; // EM17
const int MAGNES18 = 44; // EM18 (Poprawiono z J3_3)
const int MAGNES19 = 43; // EM19
const int MAGNES20 = 47; // EM20

const int MAGNES21 = 21; // EM21
const int MAGNES22 = 1;  // EM22 (Konflikt z EM10? Sprawdź J3_20 vs J3_4)
const int MAGNES23 = 17; // EM23
const int MAGNES24 = 18; // EM24

// Tablica pinów
int MAGNET_PINS[24] = {
  MAGNES1, MAGNES2, MAGNES3, MAGNES4, 
  MAGNES5, MAGNES6, MAGNES7, MAGNES8, 
  MAGNES9, MAGNES10, MAGNES11, MAGNES12,
  MAGNES13, MAGNES14, MAGNES15, MAGNES16, 
  MAGNES17, MAGNES18, MAGNES19, MAGNES20, 
  MAGNES21, MAGNES22, MAGNES23, MAGNES24
};

// Kanały PWM
const int PWM_FREQ = 50;
const int PWM_RES = 14; // ESP32-S3 ma lepszą rozdzielczość
const int MOTOR_CHANNELS[6] = {0, 1, 2, 3, 4, 5};
bool motorDirection[6] = {false}; 

volatile bool eightNoteTick = false;
hw_timer_t *bpmTimer = NULL;

// Deklaracje
void pwm(int motorChannel, int position);
void nowaFunkcjaMotorkow(int stringIndex);
void nowaFunkcjaMagnesow(int magnetIndex, int durationMs);
void grajZJson(const char* jsonInput);
void IRAM_ATTR onBpmTimer();
int midiToMagnetIndex(int midiNote); // Nowa funkcja pomocnicza

void setup() {
  Serial.begin(115200);

  // Setup PWM dla ESP32-S3 (Arduino Core 3.0 używa ledcAttach)
  // Jeśli używasz starego core, zostaw ledcSetup. Dla S3 zalecane jest nowe:
  for(int i=0; i<6; i++) {
     ledcAttach(MOTOR_PINS[i], PWM_FREQ, PWM_RES); 
     // Jeśli powyższe nie działa (zależy od wersji biblioteki), użyj starego:
     // ledcSetup(MOTOR_CHANNELS[i], PWM_FREQ, PWM_RES);
     // ledcAttachPin(MOTOR_PINS[i], MOTOR_CHANNELS[i]);
  }
  
  // Tablica pinów silników do pętli
  int MOTOR_PINS[6] = {MOTOR1, MOTOR2, MOTOR3, MOTOR4, MOTOR5, MOTOR6};

  pinMode(LED, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  
  for(int i=0; i<24; i++) {
    pinMode(MAGNET_PINS[i], OUTPUT);
    digitalWrite(MAGNET_PINS[i], 0);
  }

  initDurationTimer();
  
  // Testowe uruchomienie (odkomentuj w razie potrzeby)
  // grajZJson("{\"bpm\":120,\"notes\":[{\"n\":40,\"d\":150}]}");
}

void loop() {
  // Pusta pętla
}

void pwm(int motorChannel, int position) {
  // Kalibracja dla 14-bit PWM (0-16383). 
  // Stare wartości (13, 20, 26) były dla 8-bit (0-255).
  // Musisz dobrać te liczby eksperymentalnie! Poniżej przykłady:
  int center = 1200; // ok 7.5% duty
  int left = 600;    // ok 3.5% duty
  int right = 1800;  // ok 11% duty

  if(position == -1) ledcWrite(motorChannel, left);
  else if(position == 0) ledcWrite(motorChannel, center);
  else if(position == 1) ledcWrite(motorChannel, right);
}

void nowaFunkcjaMotorkow(int stringIndex) {
  if (stringIndex < 0 || stringIndex > 5) return;
  motorDirection[stringIndex] = !motorDirection[stringIndex];
  int pos = motorDirection[stringIndex] ? 1 : -1;
  pwm(MOTOR_CHANNELS[stringIndex], pos); // Użyj kanału, nie pinu
}

void nowaFunkcjaMagnesow(int magnetIndex, int durationMs) {
  if (magnetIndex < 0 || magnetIndex >= 24) return;
  digitalWrite(MAGNET_PINS[magnetIndex], 1);
  noInterrupts();
  magnetTimers[magnetIndex] = durationMs;
  interrupts();
}

void IRAM_ATTR onBpmTimer() {
  eightNoteTick = true;
}

// Funkcja zamieniająca MIDI na indeks magnesu (0-23)
// Zakładam strojenie standardowe E A D G B E i 4 progi
int midiToMagnetIndex(int midiNote) {
  // Struna E (Low) = 40. Progi: 41, 42, 43, 44 -> Indeksy 0, 1, 2, 3
  if (midiNote >= 41 && midiNote <= 44) return midiNote - 41 + 0; // Struna 1

  // Struna A = 45. Progi: 46, 47, 48, 49 -> Indeksy 4, 5, 6, 7
  if (midiNote >= 46 && midiNote <= 49) return midiNote - 46 + 4; // Struna 2

  // Struna D = 50. Progi: 51-54 -> Indeksy 8-11
  if (midiNote >= 51 && midiNote <= 54) return midiNote - 51 + 8; // Struna 3

  // Struna G = 55. Progi: 56-59 -> Indeksy 12-15
  if (midiNote >= 56 && midiNote <= 59) return midiNote - 56 + 12; // Struna 4

  // Struna B = 59. Progi: 60-63 -> Indeksy 16-19
  if (midiNote >= 60 && midiNote <= 63) return midiNote - 60 + 16; // Struna 5

  // Struna E (High) = 64. Progi: 65-68 -> Indeksy 20-23
  if (midiNote >= 65 && midiNote <= 68) return midiNote - 65 + 20; // Struna 6

  return -1; // Pusta struna lub poza skalą
}

void grajZJson(const char* jsonInput) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonInput);
  if (error) return;

  int bpm = doc["bpm"];
  uint64_t eighthNoteMicros = (60000000 / bpm) / 2;

  bpmTimer = timerBegin(1, 80, true);
  timerAttachInterrupt(bpmTimer, &onBpmTimer, true);
  timerAlarmWrite(bpmTimer, eighthNoteMicros, true);
  timerAlarmEnable(bpmTimer);

  JsonArray notes = doc["notes"];
  
  for (JsonObject note : notes) {
    eightNoteTick = false;
    while (!eightNoteTick) { delay(1); } // Czekaj na ósemkę

    int midi = note["n"];
    int duration = note["d"];

    if (midi > 0) {
      // 1. Znajdź indeks magnesu
      int magIdx = midiToMagnetIndex(midi);
      
      // 2. Jeśli znaleziono magnes (czyli to nie pusta struna), włącz go
      if (magIdx != -1) {
        nowaFunkcjaMagnesow(magIdx, duration);
      }

      // 3. Znajdź która to struna, żeby ją szarpnąć
      int stringIdx = -1;
      if (midi >= 40 && midi <= 44) stringIdx = 0; // E
      else if (midi >= 45 && midi <= 49) stringIdx = 1; // A
      else if (midi >= 50 && midi <= 54) stringIdx = 2; // D
      else if (midi >= 55 && midi <= 59) stringIdx = 3; // G
      else if (midi >= 59 && midi <= 63) stringIdx = 4; // B
      else if (midi >= 64 && midi <= 68) stringIdx = 5; // e

      if (stringIdx != -1) {
         nowaFunkcjaMotorkow(stringIdx);
      }
    }
  }
  
  timerAlarmDisable(bpmTimer);
  timerEnd(bpmTimer);
}