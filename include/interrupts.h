#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <Arduino.h>

// Ilość magnesów w systemie
#define MAGNET_COUNT 24
//częstotliwość zegara mikrokontrolera w MHz
#define XO_FREQ 40

// Inicjalizacja timera obsługującego czasy trwania (co 1ms)
int initDurationTimer(int bpm);

#endif