#include "interrupts.h"

// Tablica czasów dla 24 magnesów (inicjalnie same zera)
volatile int magnetTimers[MAGNET_COUNT] = {0};

// Wskaźnik na timer sprzętowy
hw_timer_t *durationTimer = NULL;

// Funkcja przerwania (wywoływana co 1ms)
void IRAM_ATTR onDurationTimer() {
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

void initDurationTimer() {
  // Używamy Timera 0, prescaler 80 (dla 80MHz daje 1 tick = 1us)
  durationTimer = timerBegin(0, 80, true);
  
  // Podpinamy funkcję przerwania
  timerAttachInterrupt(durationTimer, &onDurationTimer, true);
  
  // Ustawiamy alarm co 1000 ticków (1000us = 1ms)
  timerAlarmWrite(durationTimer, 1000, true);
  
  // Włączamy timer
  timerAlarmEnable(durationTimer);
}