#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "main.h"
#include "interrupts.h"
#include "serwer.h"

//definicje pinów i funkcji w main.h

//serwer HTTP w serwer.h

//zmienne dotyczące przerwań
extern hw_timer_t *bpmTimer; //wskaźnik na timer sprzętowy
extern bool przerwanie; //czy było przerwanie od ostatniej iteracji pętli

//zmienne globalne
bool running = false; // Stan odtwarzania
int JSON_count; //liczba JSONów w pamięci
int song_index = 0; //aktualny indeks JSONa
int osemkowe_takty = 0; //liczba ósemkowych taktów od startu utworu
int takt = 0; //aktualny takt
JsonArray nuty; //tablica nut
int stan_motorkow[6]; //stan motorków

void setup() {
  serwerprint("Rozpoczynanie inicjalizacji...");
  //Inicjalizacja LittleFS
  if(!LittleFS.begin()){
    serwerprint("Błąd wczytywania pamięci flash");
    return;
  }
  else {
    serwerprint("Pamięć flash wczytana pomyślnie");
  }

  //inicjalizacja WiFi
  setupWiFi();

  //inicjalizacja endpointów serwera
  endpoint_init();

  // Inicjalizacja PWM
  for(int i=0; i<6; i++) {
     ledcSetup(MOTOR_CHANNELS[i], PWM_FREQ, PWM_RES);
     ledcAttachPin(MOTOR_PINS[i], MOTOR_CHANNELS[i]);
  }
  serwerprint("PWM zainicjalizowane.");

  //pinmode'y
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  
  for(int i=0; i<24; i++) {
    pinMode(MAGNET_PINS[i], OUTPUT);
    digitalWrite(MAGNET_PINS[i], LOW);
  }
  serwerprint("GPIO zainicjalizowane.");

  //Liczenie JSONów
  JSON_count = count_JSONs();
  serwerprint("Znaleziono " + String(JSON_count) + " plików JSON.");

  //ustawinie początkowych stanów motorków
  for(int i=0; i<6; i++) {
    stan_motorkow[i] = -1;
    pwm(MOTOR_CHANNELS[i], -1);
  }
  serwerprint("Inicjalizacja zakończona.");
}

void loop() {
  // pętla główna
  serwer.handleClient(); //obsługa serwera HTTP
  if(digitalRead(BUTTON1) == HIGH) {
    serwerprint("Przycisk 1 wciśnięty");
    //start
    if(!running) {
      serwerprint("Rozpoczynanie odtwarzania...");
      running = true;
      digitalWrite(LED, HIGH);
      graj();
    }
    //stop
    else {
      serwerprint("Zatrzymywanie odtwarzania...");
      running = false;
      digitalWrite(LED, LOW);
      timerAlarmDisable(bpmTimer);
    }
  delay(50); // Debounce
  }
  if(digitalRead(BUTTON2)==HIGH)
  serwerprint("Przycisk 2 wciśnięty");
  {
    if(!running)
    {
      //następny utwór
      song_index++;
      if(song_index>=JSON_count)
      {
        song_index=0;
      }
      serwerprint("Wybrano utwór o indeksie: " + String(song_index));
      delay(50); //debounce
    }
  }
  if(running && przerwanie){
    serwerprint("Przerwanie timera - takt: " + String(takt));
    odliczaj();
    zapisz_nuty();
    przerwanie = false;
  }
}

int count_JSONs() {
  int count = 0;
  File root = LittleFS.open("/");
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
  serwerprint("Odtwarzanie utworu index: " + String(song_index));
  // Odczytanie odpowiedniego pliku JSON
  String filename = "/" + String(song_index) + ".json";
  File file = LittleFS.open(filename, "r");
  if(!file) {
    serwerprint("Nie można otworzyć pliku: " + filename);
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
        digitalWrite(MAGNET_PINS[i], LOW); // Wyłącz pin
        serwerprint("Magnes " + String(i) + " wyłączony.");
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
  if (magnetIndex < 0 || magnetIndex >= 24) serwerprint("Błąd: nieprawidłowy indeks magnesu"); return;
  digitalWrite(MAGNET_PINS[magnetIndex], HIGH);
  noInterrupts();
  magnetTimers[magnetIndex] = durationMs;
  interrupts();
  serwerprint("Magnes " + String(magnetIndex) + " włączony na " + String(durationMs) + " ms.");
}

void zapisz_nuty(int eight) {
  //inkrementujemy nr taktu
  takt++;
  //sprawdzamy czy koniec utworu
  if(takt > osemkowe_takty) {
    takt = 0;
    timerAlarmDisable(bpmTimer);
    timerEnd(bpmTimer);
    running = false;
    serwerprint("Koniec utworu.");
    digitalWrite(LED, LOW);
  }
  //odtwarzamy nuty z aktualnego taktu
  else{
    int ilosc = nuty[takt].size();
    serwerprint("Liczba nut: " + String(ilosc));
    for(int i=0; i<ilosc; i++){
      int nuta = nuty[takt][i]["n"];
      int dlugosc = eight*nuty[takt][i]["d"];
      serwerprint("Nuta: " + String(nuta) + ", długość: " + String(dlugosc) + " ms");
      if (nuta!=-1) nowaFunkcjaMagnesow(nuta_na_magnes(nuta), dlugosc);
    }
    delay(100);
    for(int i=0; i<ilosc; i++){
      int nuta = nuty[takt][i]["n"];
      if(nuta!=-1){
        motorki(nuta);
      }
    }
  }
}

void motorki(int nuta) {
  //sterowanie motorkami na podstawie nut
  if(1<nuta<6) {
    stan_motorkow[0] = -stan_motorkow[0]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[0], stan_motorkow[0]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 1 do pozycji " + String(stan_motorkow[0]));
    }
  else if(6<nuta<11) {
    stan_motorkow[1] = -stan_motorkow[1]; // Zmiana stanu motorka 2
    pwm(MOTOR_CHANNELS[1], stan_motorkow[1]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 2 do pozycji " + String(stan_motorkow[1]));
    }
  else if(11<nuta<16) {
    stan_motorkow[2] = -stan_motorkow[2]; // Zmiana stanu motorka 3
    pwm(MOTOR_CHANNELS[2], stan_motorkow[2]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 3 do pozycji " + String(stan_motorkow[2]));
    }
  else if(16<nuta<21) {
    stan_motorkow[3] = -stan_motorkow[3]; // Zmiana stanu motorka 4
    pwm(MOTOR_CHANNELS[3], stan_motorkow[3]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 4 do pozycji " + String(stan_motorkow[3]));
    }
  else if(21<nuta<26) {
    stan_motorkow[4] = -stan_motorkow[4]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[4], stan_motorkow[4]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 5 do pozycji " + String(stan_motorkow[4]));
    }
  else if(26<nuta<31) {
    stan_motorkow[5] = -stan_motorkow[5]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[5], stan_motorkow[5]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 6 do pozycji " + String(stan_motorkow[5]));
    }
    else serwerprint("Pauza.");
}

int nuta_na_magnes(int nuta){
  //funkcja zwracająca numer magnesu na podstawie nuty
  //nuta 1, 6, 11, 16, 21, 26 pusta struna
  if(1<nuta<6){
    return (nuta - 1);
  }
  else if(6<nuta<11){
    return (nuta - 2);
  }
  else if(11<nuta<16){
    return (nuta - 3);
  }
  else if(16<nuta<21){
    return (nuta - 4);
  }
  else if(21<nuta<26){
    return (nuta - 5);
  }
  else if(26<nuta<31){
    return (nuta - 6);
  }
  else{
    return -1; //pauza
  }
}

void grajZJson(const char* jsonInput) {
  // Parsowanie JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonInput);
  if (error) serwerprint("Błąd deserializacji JSON"); return;

  //wczytanie tempa
  int bpm = doc["bpm"];
  serwerprint("Ustawione BPM: " + String(bpm));

  //inicjalizacja timera
  initDurationTimer(bpm);

  nuty = doc["notes"];
  osemkowe_takty = nuty.size();
  serwerprint("Liczba ósemkowych taktów: " + String(osemkowe_takty));
  takt = 0;
}

String readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) return String();
  String s;
  while (file.available()) s += (char)file.read();
  file.close();
  return s;
}