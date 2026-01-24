#ifndef SERWER_H
#define SERWER_H

#include "main.h"
#include "passy.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

WebServer serwer(80); //serwer HTTP

//wczytywanie plików html
String root = readFile("/root.html");
String debug = readFile("/debug.html");

//zmienne
String debuglog = "";

//deklaracje funkcji
String debug_message(String message);
bool setupWiFi();
void endpoint_init();
void rootHandler();
void debugHandler();
void serwerprint(String message);

#endif