#include "serwer.h"
#include "main.h"
#include "passy.h"
#include "interrupts.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

WebServer serwer; //serwer HTTP

//wczytywanie plików html - będą załadowane po montowaniu systemu plików
String root = "";
String debug = "";
bool filesLoaded = false;

//zmienne
String debuglog = "";

//extern
extern int stan_motorkow[6];

//inicjalizacja WiFi
bool setupWiFi() {
  serwerprint("Łączenie z WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // Czekaj maksymalnie 2,5 sekund na połączenie
  int attempts = 0;
  while(WiFi.status() != WL_CONNECTED && attempts < 5) {
    delay(500);
    attempts++;
  }
  
  bool connected;
  if(WiFi.status() != WL_CONNECTED) {
    serwerprint("Brak połączenia z siecią WiFi");
    connected = false;
  } else {
    serwerprint("Połączono z siecią WiFi. Adres IP: " + WiFi.localIP().toString());
    connected = true;
  }
  return connected;
}

//wczytaj pliki HTML z systemu plików
void loadFilesFromFS() {
  if (!filesLoaded) {
    root = readFile("/root.html");
    debug = readFile("/debug.html");
    filesLoaded = true;
    serwerprint("Pliki HTML wczytane z systemu plików");
  }
}

//inicjalizacja endpointów
void endpoint_init() {
  serwerprint("Inicjalizacja endpointów...");
  serwer.on("/", rootHandler);
  serwer.on("/debug", debugHandler);
  serwer.on("/magnet", magnetHandler);
  serwer.on("/motor", motorHandler);
  serwer.on("/clearlog", clearlogHandler);
  serwerprint("Endpointy zainicjalizowane.");
  delay(100);
  serwer.begin();
  serwerprint("Serwer HTTP uruchomiony.");
  delay(100);
}

//add entry to debug log
void serwerprint(String message) {
  Serial.println(message);
  debuglog += message + "\n";
}

String debug_message(String message) {
String html = "";
  if (debuglog.length() == 0) {
     html += "<p>Brak wpisów w logu.</p>";
  } else {
    // split by newline and render
    int start = 0;
    while (start < debuglog.length()) {
      int nl = debuglog.indexOf('\n', start);
      String line;
      if (nl == -1) {
        line = debuglog.substring(start);
        start = debuglog.length();
      } else {
        line = debuglog.substring(start, nl);
        start = nl + 1;
      }
      html += "<div class='entry'>" + line + "</div>";
    }
  }
    return html;
}

// handlery
void rootHandler() {
  String html = root;
  serwer.send(200, "text/html", html);
}

void debugHandler() {
  String html = debug;
  html.replace("%LOG_ENTRIES%", debug_message(debuglog));
  serwer.send(200, "text/html", html);
}

void magnetHandler() {
  if (serwer.hasArg("id")) {
    int magnetId = serwer.arg("id").toInt();
    
    if (magnetId >= 0 && magnetId < MAGNET_COUNT) {
      // Włącz magnes
      digitalWrite(MAGNET_PINS[magnetId], !digitalRead(MAGNET_PINS[magnetId]));
      serwerprint("Magnes " + String(magnetId) + " przełączony.");
      serwer.send(200, "text/plain", "OK");
      serwer.sendHeader("Location", "/debug");
    } else {
      serwer.send(400, "text/plain", "Invalid parameters");
    }
  } else {
    serwer.send(400, "text/plain", "Missing parameters");
  }
}

void motorHandler() {
  if (serwer.hasArg("channel")) {
    int channel = serwer.arg("channel").toInt();
    
    if (channel >= 0 && channel < 6) {
      stan_motorkow[channel] = -stan_motorkow[channel];
      pwm(MOTOR_CHANNELS[channel], stan_motorkow[channel]);
      serwerprint("Motor " + String(channel) + " przełączony.");
      serwer.send(200, "text/plain", "OK");
      serwer.sendHeader("Location", "/debug");
    } else {
      serwer.send(400, "text/plain", "Invalid channel");
    }
  } else {
    serwer.send(400, "text/plain", "Missing parameters");
  }
}

void clearlogHandler() {
  debuglog = "";
  serwerprint("Log wyczyszczony");
  serwer.send(200, "text/plain", "Log cleared");
  serwer.sendHeader("Location", "/debug");
}

