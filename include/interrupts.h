#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <Arduino.h>

// Ilość magnesów w systemie
#define MAGNET_COUNT 24

// Globalna tablica czasów (odlicza czas w dół dla każdego magnesu)
extern volatile int magnetTimers[MAGNET_COUNT];

// Tablica mapująca indeksy (0-23) na fizyczne piny (MAGNES1...MAGNES24)
// Musi być dostępna dla przerwania
extern int MAGNET_PINS[MAGNET_COUNT];

// Inicjalizacja timera obsługującego czasy trwania (co 1ms)
void initDurationTimer();

#endif