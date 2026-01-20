#include "interrupts.h"

// Tablica czasów dla 24 magnesów (inicjalnie same zera)
volatile int magnetTimers[MAGNET_COUNT] = {0};

// Wskaźnik na timer sprzętowy
extern hw_timer_t *bpmTimer;
extern bool przerwanie;

// Funkcja przerwania
void IRAM_ATTR onDurationTimer() {
  przerwanie = true;
}

void initDurationTimer(int bpm) {

  //bpm to ticks
  int ticks = 60000 / bpm / 2; //ms na ósemkę

  // Używamy Timera 0, prescaler 80 (dla 80MHz daje 1 tick = 1us)
  bpmTimer = timerBegin(0, 80, true);
  
  // Podpinamy funkcję przerwania
  timerAttachInterrupt(bpmTimer, &onDurationTimer, true);

  // Ustawiamy alarm co ticks ticków (1000us = 1ms)
  timerAlarmWrite(bpmTimer, 1000*ticks, true);

  // Włączamy timer
  timerAlarmEnable(bpmTimer);
}