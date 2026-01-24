#ifndef SERWER_H
#define SERWER_H

#include <Arduino.h>
#include <WebServer.h>

String debug_message(String message);
bool setupWiFi();
void endpoint_init();
void rootHandler();
void debugHandler();
void magnetHandler();
void motorHandler();
void clearlogHandler();
void serwerprint(String message);
void loadFilesFromFS();

#endif