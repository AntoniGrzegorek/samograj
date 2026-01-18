#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <Arduino.h>

// Struktura do przechowywania informacji o timerze dla każdego magnesu
struct MagnesTimer {
  hw_timer_t* timer;
  int pin;
  volatile bool active;
};

// Tablica struktur dla każdego magnesu (maksymalnie 4 timery na ESP32)
extern MagnesTimer magnesTimers[4];
extern int timerCount;

// Funkcja wywoływana w przerwaniu timera
void IRAM_ATTR onMagnesTimer(MagnesTimer* mt);

#endif
