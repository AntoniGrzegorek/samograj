#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

//definicje pinów

    //led i przyciski
const int LED = 6;
const int BUTTON1 = 5;
const int BUTTON2 = 4;

    //Serwa
const int MOTOR1 = 9;
const int MOTOR2 = 10;
const int MOTOR3 = 11;
const int MOTOR4 = 12;
const int MOTOR5 = 13;
const int MOTOR6 = 14;

    //magnesy
const int MAGNES1 = 43;
const int MAGNES2 = 44;
const int MAGNES3 = 1;
const int MAGNES4 = 2;

const int MAGNES5 = 42;
const int MAGNES6 = 41;
const int MAGNES7 = 40;
const int MAGNES8 = 39;

const int MAGNES9 = 38;
const int MAGNES10 = 37;
const int MAGNES11 = 36;
const int MAGNES12 = 35;

const int MAGNES13 = 48;
const int MAGNES14 = 47;
const int MAGNES15 = 21;
const int MAGNES16 = 20;

const int MAGNES17 = 19;
const int MAGNES18 = 7;
const int MAGNES19 = 15;
const int MAGNES20 = 16;

const int MAGNES21 = 17;
const int MAGNES22 = 18;
const int MAGNES23 = 8;
const int MAGNES24 = 3;

    // Tablica pinów magnesów
const int MAGNET_PINS[24] = {
  MAGNES1, MAGNES2, MAGNES3, MAGNES4, 
  MAGNES5, MAGNES6, MAGNES7, MAGNES8, 
  MAGNES9, MAGNES10, MAGNES11, MAGNES12,
  MAGNES13, MAGNES14, MAGNES15, MAGNES16, 
  MAGNES17, MAGNES18, MAGNES19, MAGNES20, 
  MAGNES21, MAGNES22, MAGNES23, MAGNES24
};

    // Tablica pinów motorków
const int MOTOR_PINS[6] = {MOTOR1, MOTOR2, MOTOR3, MOTOR4, MOTOR5, MOTOR6};

// Kanały PWM
const int PWM_FREQ = 50;
const int PWM_RES = 14;
const int MOTOR_CHANNELS[6] = {0, 1, 2, 3, 4, 5};

//deklaracje funkcji
int count_JSONs(); //liczenie JSONów
int graj(); //funkcja odtwarzająca utwór
void pwm(int motorChannel, int position);
void odliczaj(); //funkcja odliczająca czasy magnesów
int nuta_na_magnes(int nuta); //funkcja zwracająca numer magnesu na podstawie nuty
void zapisz_nuty(int eight); //funkcja zapisująca aktualne nuty do timera
void nowaFunkcjaMagnesow(int magnetIndex, int durationMs);
int grajZJson(const char* jsonInput);
void motorki(int nuta);
String readFile(const char* path); //wczytywanie pliku do stringa

#endif