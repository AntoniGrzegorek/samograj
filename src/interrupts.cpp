#include "../include/interrupts.h"

// Tablica struktur dla każdego magnesu (maksymalnie 4 timery na ESP32)
MagnesTimer magnesTimers[4] = {};
int timerCount = 0;

// Funkcja wywoływana w przerwaniu timera
void IRAM_ATTR onMagnesTimer(MagnesTimer* mt) {
  digitalWrite(mt->pin, 0);  // Wyłącz pin
  mt->active = false;
}
