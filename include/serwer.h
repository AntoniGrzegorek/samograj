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
void fileManagerHandler();
void filesListHandler();
void fileGetHandler();
void fileUploadHandler();
void fileDeleteHandler();
void fileSaveHandler();
void fileMidiAnalyzeHandler();
void fileConvertHandler();
void logHandler();
void serwerprint(String message);
void loadFilesFromFS();

#endif