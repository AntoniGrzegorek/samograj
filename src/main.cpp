#include <Arduino.h>
#include "interrupts.h"

//inicjalizacja pinów
const int LED = 4;
const int BUTTON1 = 5;
const int BUTTON2 = 6;
const int MOTOR1 = 9;
const int MOTOR2 = 10;
const int MOTOR3 = 11;
const int MOTOR4 = 12;
const int MOTOR5 = 13;
const int MOTOR6 = 14;
const int MAGNES1 = 3;
const int MAGNES2 = 8;
const int MAGNES3 = 18;
const int MAGNES4 = 17;
const int MAGNES5 = 16;
const int MAGNES6 = 15;
const int MAGNES7 = 7;
const int MAGNES8 = 19;
const int MAGNES9 = 20;
const int MAGNES10 = 21;
const int MAGNES11 = 47;
const int MAGNES12 = 48;
const int MAGNES13 = 35;
const int MAGNES14 = 36;
const int MAGNES15 = 37;
const int MAGNES16 = 38;
const int MAGNES17 = 39;
const int MAGNES18 = 40;
const int MAGNES19 = 41;
const int MAGNES20 = 42;
const int MAGNES21 = 2;
const int MAGNES22 = 1;
const int MAGNES23 = 43;
const int MAGNES24 = 44;

//kanały PWM
const int PWM1 = 0;
const int PWM2 = 1;
const int PWM3 = 2;
const int PWM4 = 3;
const int PWM5 = 4;
const int PWM6 = 5;
const int PWM_FREQ = 50;
const int PWM_RES = 8;

// put function declarations here:
void pwm(int motor, int position);
void magnes(int magnes, int t);

void setup() {
  //pwm setup
  ledcSetup(PWM1, PWM_FREQ, PWM_RES);
  ledcSetup(PWM2, PWM_FREQ, PWM_RES);
  ledcSetup(PWM3, PWM_FREQ, PWM_RES);
  ledcSetup(PWM4, PWM_FREQ, PWM_RES);
  ledcSetup(PWM5, PWM_FREQ, PWM_RES);
  ledcSetup(PWM6, PWM_FREQ, PWM_RES);
  ledcAttachPin(MOTOR1, PWM1);
  ledcAttachPin(MOTOR2, PWM2);
  ledcAttachPin(MOTOR3, PWM3);
  ledcAttachPin(MOTOR4, PWM4);
  ledcAttachPin(MOTOR5, PWM5);
  ledcAttachPin(MOTOR6, PWM6);
  //pinmode
  pinMode(LED, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  //pinMode(MOTOR1, OUTPUT);
  //pinMode(MOTOR2, OUTPUT);
  //pinMode(MOTOR3, OUTPUT);
  //pinMode(MOTOR4, OUTPUT);
  //pinMode(MOTOR5, OUTPUT);
  //pinMode(MOTOR6, OUTPUT);
  pinMode(MAGNES1, OUTPUT);
  pinMode(MAGNES2, OUTPUT);
  pinMode(MAGNES3, OUTPUT);
  pinMode(MAGNES4, OUTPUT);
  pinMode(MAGNES5, OUTPUT);
  pinMode(MAGNES6, OUTPUT);
  pinMode(MAGNES7, OUTPUT);
  pinMode(MAGNES8, OUTPUT);
  pinMode(MAGNES9, OUTPUT);
  pinMode(MAGNES10, OUTPUT);
  pinMode(MAGNES11, OUTPUT);
  pinMode(MAGNES12, OUTPUT);
  pinMode(MAGNES13, OUTPUT);
  pinMode(MAGNES14, OUTPUT);
  pinMode(MAGNES15, OUTPUT);
  pinMode(MAGNES16, OUTPUT);
  pinMode(MAGNES17, OUTPUT);
  pinMode(MAGNES18, OUTPUT);
  pinMode(MAGNES19, OUTPUT);
  pinMode(MAGNES20, OUTPUT);
  pinMode(MAGNES21, OUTPUT);
  pinMode(MAGNES22, OUTPUT);
  pinMode(MAGNES23, OUTPUT);
  pinMode(MAGNES24, OUTPUT);
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  //pwm(PWM1, -1);
  magnes(MAGNES1, 500);
  delay(2000);
}

// put function definitions here:

//motor - kanał pwm tj. 0-5
//position -1 - -90, 0 - 0, 1 - 90
void pwm(int motor, int position) {
  if(position == -1)
  {
    ledcWrite(motor, 13);
  }
  else if(position == 0)
  {
    ledcWrite(motor, 20);
  }
  else if(position == 1)
  {
    ledcWrite(motor, 26);
  }
}

void magnes(int pin, int t)
{
  // Włącz pin
  digitalWrite(pin, 1);
  
  // Jeśli już mamy 4 timery, nie możemy dodać więcej
  if (timerCount >= 4) {
    return;
  }
  
  // Zarejestruj nowy magnes
  magnesTimers[timerCount].pin = pin;
  magnesTimers[timerCount].active = true;
  
  // Utwórz timer (1 MHz, auto-reload wyłączony)
  magnesTimers[timerCount].timer = timerBegin(timerCount, 80, true);
  
  // Ustaw przerwanie - czas w mikrosekundach
  timerAlarmWrite(magnesTimers[timerCount].timer, t * 1000, false);
  
  // Dołącz funkcję obsługi przerwania
  timerAttachInterrupt(magnesTimers[timerCount].timer, 
                       []() {
                         // Obsługuj pierwszy aktywny timer
                         for (int i = 0; i < timerCount; i++) {
                           if (magnesTimers[i].active) {
                             onMagnesTimer(&magnesTimers[i]);
                             break;
                           }
                         }
                       }, 
                       true);
  
  // Włącz alarm
  timerAlarmEnable(magnesTimers[timerCount].timer);
  
  timerCount++;
}