#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "main.h"
#include "interrupts.h"
#include "serwer.h"

//definicje pinów i funkcji w main.h

//serwer HTTP w serwer.h
extern WebServer serwer;

//zmienne dotyczące przerwań
extern hw_timer_t *bpmTimer; //wskaźnik na timer sprzętowy
extern bool przerwanie; //czy było przerwanie od ostatniej iteracji pętli

//zmienne globalne
volatile int magnetTimers[MAGNET_COUNT] = {0}; // Tablica czasów dla 24 magnesów (inicjalnie same zera)
bool running = false; // Stan odtwarzania
int JSON_count; //liczba JSONów w pamięci
int song_index = 0; //aktualny indeks JSONa
int osemkowe_takty = 0; //liczba ósemkowych taktów od startu utworu
int takt = 0; //aktualny takt
JsonDocument songDoc; // Dokument JSON
JsonArray nuty; //tablica nut
int stan_motorkow[6]; //stan motorków
int eight; //długość ósemki w ms
int button1_state_old = LOW;
int button2_state_old = LOW;

void setup() {
  //Serial.begin(115200); // Inicjalizacja portu seryjnego
  delay(100);
  serwerprint("\n\nRozpoczynanie inicjalizacji...");
  //Inicjalizacja LittleFS
  if(!LittleFS.begin()){
    serwerprint("Błąd wczytywania pamięci flash. Próbuję formatować...");
    if(!LittleFS.format()) {
      serwerprint("Błąd: Nie mogę sformatować systemu plików!");
      return;
    }
    serwerprint("System plików sformatowany. Próbuję ponownie...");
    if(!LittleFS.begin()) {
      serwerprint("Błąd: System plików nadal nie montuje się!");
      return;
    }
  }
  serwerprint("Pamięć flash wczytana pomyślnie");
  
  delay(500); // Czekaj na dostępność systemu plików
  
  // Wczytaj pliki HTML z systemu plików
  loadFilesFromFS();

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
  delay(100);

  //pinmode'y
  pinMode(LED, OUTPUT);
  serwerprint("LED zainicjalizowany.");
  delay(100);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  serwerprint("Przyciski zainicjalizowane.");
  delay(100);
  
  for(int i=0; i<24; i++) {//zmienić później 2 na 0
    pinMode(MAGNET_PINS[i], OUTPUT);
    serwerprint("Magnes " + String(i+1) + " zainicjalizowany.");
    delay(100);
    digitalWrite(MAGNET_PINS[i], LOW);
  }
  serwerprint("GPIO zainicjalizowane.");
  delay(100);

  //Liczenie JSONów
  JSON_count = count_JSONs();
  serwerprint("Znaleziono " + String(JSON_count) + " plików JSON.");
  delay(100);

  //ustawinie początkowych stanów motorków
  for(int i=0; i<6; i++) {
    stan_motorkow[i] = -1;
    pwm(MOTOR_CHANNELS[i], -1);
  }
  serwerprint("Inicjalizacja zakończona.");
  delay(100);
}

void loop() {
  // pętla główna
  serwer.handleClient(); //obsługa serwera HTTP

  //obsługa przycisków
  int button1_state = digitalRead(BUTTON1);
  int button2_state = digitalRead(BUTTON2);

  if(button2_state == LOW && button2_state_old == HIGH) {
    serwerprint("Przycisk 1 wciśnięty");
    //start
    if(!running) {
      serwerprint("Rozpoczynanie odtwarzania...");
      running = true;
      digitalWrite(LED, HIGH);
      eight = graj();
    }
    //stop
    else {
      serwerprint("Zatrzymywanie odtwarzania...");
      running = false;
      digitalWrite(LED, LOW);
      timerAlarmDisable(bpmTimer);
      timerEnd(bpmTimer);
      serwerprint("Odtwarzanie zatrzymane.");
    }
  delay(50); // Debounce
  }
  if(button1_state==LOW && button1_state_old==HIGH)
  {
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
      else serwerprint("Nie można zmienić utworu podczas odtwarzania!");
    }
  }

  button1_state_old = button1_state;
  button2_state_old = button2_state;

  //sprawdzamy czy wywołano przerwanie timera
  if(running && przerwanie){
    serwerprint("Przerwanie timera - takt: " + String(takt));
    odliczaj();
    serwerprint("Magnesy odliczone.");
    zapisz_nuty(eight);
    serwerprint("Nuty zagrane.");
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

int graj() {
  int eight;
  serwerprint("Odtwarzanie utworu: " + String(song_index));
  // Odczytanie odpowiedniego pliku JSON
  String filename = "/" + String(song_index) + ".json";
  File file = LittleFS.open(filename, "r");
  if(!file) {
    serwerprint("Nie można otworzyć pliku: " + filename);
    return 0;
  }
  else serwerprint("Plik " + filename + " otwarty pomyślnie.");

  String jsonContent = "";
  while(file.available()) {
    jsonContent += char(file.read());
  }
  file.close();

  // Odtwarzanie z JSON
  eight = grajZJson(jsonContent.c_str());
  return eight;
}

void odliczaj(){
for (int i = 0; i < MAGNET_COUNT; i++) { //zmienić później 2 na 0
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
  //sprawdzamy czy koniec utworu
  if(takt >= osemkowe_takty-1) {
    takt = 0;
    timerAlarmDisable(bpmTimer);
    timerEnd(bpmTimer);
    running = false;
    serwerprint("Koniec utworu.");
    digitalWrite(LED, LOW);
  }
  //odtwarzamy nuty z aktualnego taktu
  else{
    // Pobierz tablicę nut dla aktualnego taktu
    serwerprint("Wczytywanie " + String(takt)+". taktu...");
    JsonArray takt_array = nuty[takt].as<JsonArray>();
    int ilosc = takt_array.size();
    serwerprint("Liczba nut: " + String(ilosc));
    for(int i=0; i<ilosc; i++){
      int nuta = takt_array[i]["n"];
      int osemki = takt_array[i]["d"];
      int dlugosc = osemki * eight;
      serwerprint("Nuta: " + String(nuta) + ", długość: " + String(dlugosc) + " ms");
      if (nuta!=0) nowaFunkcjaMagnesow(nuta_na_magnes(nuta), dlugosc);
      delay(50);
      motorki(nuta);
    }
  }
  //inkrementujemy nr taktu
  takt++;
}

void motorki(int nuta) {
  //sterowanie motorkami na podstawie nut
  if(1<=nuta && nuta <6) {
    stan_motorkow[0] = -stan_motorkow[0]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[0], stan_motorkow[0]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 1 do pozycji " + String(stan_motorkow[0]));
    }
  else if(6<=nuta && nuta <11) {
    stan_motorkow[1] = -stan_motorkow[1]; // Zmiana stanu motorka 2
    pwm(MOTOR_CHANNELS[1], stan_motorkow[1]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 2 do pozycji " + String(stan_motorkow[1]));
    }
  else if(11<=nuta && nuta <16) {
    stan_motorkow[2] = -stan_motorkow[2]; // Zmiana stanu motorka 3
    pwm(MOTOR_CHANNELS[2], stan_motorkow[2]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 3 do pozycji " + String(stan_motorkow[2]));
    }
  else if(16<=nuta && nuta <21) {
    stan_motorkow[3] = -stan_motorkow[3]; // Zmiana stanu motorka 4
    pwm(MOTOR_CHANNELS[3], stan_motorkow[3]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 4 do pozycji " + String(stan_motorkow[3]));
    }
  else if(21<=nuta && nuta <26) {
    stan_motorkow[4] = -stan_motorkow[4]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[4], stan_motorkow[4]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 5 do pozycji " + String(stan_motorkow[4]));
    }
  else if(26<=nuta && nuta <31) {
    stan_motorkow[5] = -stan_motorkow[5]; // Zmiana stanu motorka 1
    pwm(MOTOR_CHANNELS[5], stan_motorkow[5]); // Ruch do pozycji aktywnej
    serwerprint("Motorek 6 do pozycji " + String(stan_motorkow[5]));
    }
    else serwerprint("Pauza.");
}

int nuta_na_magnes(int nuta){
  //funkcja zwracająca numer magnesu na podstawie nuty
  //nuta 1, 6, 11, 16, 21, 26 pusta struna
  if(3<nuta && nuta <6){ //zmienić na 1<nuta<6 potem
    return (nuta - 1);
  }
  else if(6<nuta && nuta <11){
    return (nuta - 2);
  }
  else if(11<nuta && nuta <16){
    return (nuta - 3);
  }
  else if(16<nuta && nuta <21){
    return (nuta - 4);
  }
  else if(21<nuta && nuta <26){
    return (nuta - 5);
  }
  else if(26<nuta && nuta <31){
    return (nuta - 6);
  }
  else{
    return 0; //pauza
  }
}

int grajZJson(const char* jsonInput) {
  int eight;
  // Parsowanie JSON do globalnego dokumentu
  songDoc.clear();
  DeserializationError error = deserializeJson(songDoc, jsonInput);
  if (error)
  {
    serwerprint("Błąd deserializacji JSON");
    return 0;
  }
  else serwerprint("JSON zdeserializowany pomyślnie.");

  //wczytanie tempa
  int bpm = songDoc["bpm"];
  serwerprint("Ustawione BPM: " + String(bpm));

  //inicjalizacja timera
  eight = initDurationTimer(bpm);

  nuty = songDoc["notes"];
  osemkowe_takty = nuty.size();
  serwerprint("Liczba ósemkowych taktów: " + String(osemkowe_takty));
  takt = 0;
  return eight;
}

String readFile(const char* path) {
  File file = LittleFS.open(path, "r");
  if (!file) return String();
  String s;
  while (file.available()) s += (char)file.read();
  file.close();
  return s;
}