#include "interrupts.h"

// Tablica czasów dla 24 magnesów (inicjalnie same zera)
volatile int magnetTimers[MAGNET_COUNT] = {0};

// Wskaźnik na timer sprzętowy
hw_timer_t *bpmTimer;
bool przerwanie;

// Funkcja przerwania
void IRAM_ATTR onDurationTimer() {
  przerwanie = true;
}

int initDurationTimer(int bpm) {
  //bpm musi dzielić 30000
  //bpm to ticks
  int ticks = 60000 / bpm / 2; //ms na ósemkę

  // Używamy Timera 0, prescaler XO_FREQ (dla 40MHz daje 1 tick = 1us)
  bpmTimer = timerBegin(0, XO_FREQ, true);
  
  // Podpinamy funkcję przerwania
  timerAttachInterrupt(bpmTimer, &onDurationTimer, true);

  // Ustawiamy alarm co ticks ticków (1000us = 1ms)
  timerAlarmWrite(bpmTimer, 1000*ticks, true);

  // Włączamy timer
  timerAlarmEnable(bpmTimer);
  serwerprint("zainicjalizowano timer przerwań co " + String(ticks) + " ms");
  return ticks;
}