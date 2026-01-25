#include "serwer.h"
#include "main.h"
#include "passy.h"
#include "interrupts.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <LittleFS.h>

WebServer serwer; //serwer HTTP

//wczytywanie plików html - będą załadowane po montowaniu systemu plików
String root = "";
String debug = "";
String filemanager = "";
bool filesLoaded = false;

//zmienne
String debuglog = "";

File uploadFile; //plik do uploadu

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
    filemanager = readFile("/filemanager.html");
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
  serwer.on("/pliki", fileManagerHandler);
  serwer.on("/files", filesListHandler);
  serwer.on("/file", HTTP_GET, fileGetHandler);
  serwer.on("/upload", HTTP_POST, []() {serwer.send(200);}, fileUploadHandler);
  serwer.on("/delete", HTTP_DELETE, fileDeleteHandler);
  serwer.on("/save", HTTP_POST, fileSaveHandler);
  serwer.on("/analyze", HTTP_GET, fileMidiAnalyzeHandler);
  serwer.on("/convert", HTTP_POST, fileConvertHandler);
  serwer.on("/log", HTTP_POST, logHandler);
  serwerprint("Endpointy zainicjalizowane.");
  delay(100);
  serwer.begin();
  serwerprint("Serwer HTTP uruchomiony.");
  delay(100);
}

//add entry to debug log
void serwerprint(String message) {
  //Serial.println(message);
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
  serwerprint("Strona główna załadowana.");
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

void fileManagerHandler() {
  String html = filemanager;
  serwerprint("Strona zarządzania plikami załadowana.");
  serwer.send(200, "text/html", html);
}

// ===== FILE MANAGER ENDPOINTS =====

void filesListHandler() {
  // Zwraca listę plików jako JSON
  serwerprint("Ładowanie listy plików...");
  String json = "[";
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  bool first = true;
  
  while(file) {
    String name = file.name();
    serwerprint("Znaleziono plik: " + name);
    // Filtruj tylko .mid i .json
    if (name.endsWith(".mid") || name.endsWith(".json")) {
      if (!first) json += ",";
      json += "{\"name\":\"" + name + "\"}";
      first = false;
    }
    file = root.openNextFile();
  }
  json += "]";
  
  serwer.send(200, "application/json", json);
}

void fileGetHandler() {
  if (serwer.hasArg("filename")) {
    String filename = serwer.arg("filename");
    String filepath = "/" + filename;
    
    if (LittleFS.exists(filepath)) {
      File file = LittleFS.open(filepath, "r");
      String content = "";
      while(file.available()) {
        content += (char)file.read();
      }
      file.close();
      serwer.send(200, "text/plain", content);
    } else {
      serwer.send(404, "text/plain", "File not found");
    }
  } else {
    serwer.send(400, "text/plain", "Missing filename parameter");
  }
}

void fileUploadHandler() {
  // Obsługa multipart/form-data z file uploadem
  HTTPUpload& upload = serwer.upload();
  if(upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    serwerprint("Rozpoczęto przesyłanie pliku: " + filename);
    uploadFile = LittleFS.open(filename, "w");
    if(!uploadFile) {
      serwerprint("Nie można otworzyć pliku do zapisu: " + filename);
      return;
    }
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    if(uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    } else {
      serwerprint("Błąd zapisu do pliku podczas uploadu: " + String(upload.filename));
    }
  } else if(upload.status == UPLOAD_FILE_END) {
    if(uploadFile) {
      uploadFile.close();
      serwerprint("Zakończono przesyłanie pliku: " + String(upload.filename));
      serwer.send(200, "text/plain", "File uploaded");
    }
    else {
      serwerprint("Błąd zamknięcia pliku po uploadzie: " + String(upload.filename));
    }
  } else if(upload.status == UPLOAD_FILE_ABORTED) {
    serwerprint("Przesyłanie pliku przerwane: " + String(upload.filename));
    LittleFS.remove("/" + String(upload.filename));
  }
}

void fileDeleteHandler() {
  if (serwer.hasArg("filename")) {
    String filename = serwer.arg("filename");
    String filepath = "/" + filename;
    
    if (LittleFS.exists(filepath)) {
      LittleFS.remove(filepath);
      serwer.send(200, "text/plain", "File deleted");
      serwerprint("Plik " + filename + " usunięty");
    } else {
      serwer.send(404, "text/plain", "File not found");
    }
  } else {
    serwer.send(400, "text/plain", "Missing filename parameter");
  }
}

void fileSaveHandler() {
  if (serwer.hasArg("filename")) {
    String filename = serwer.arg("filename");
    String filepath = "/" + filename;
    String content = serwer.arg("plain");
    
    File file = LittleFS.open(filepath, "w");
    if (file) {
      file.print(content);
      file.close();
      serwer.send(200, "text/plain", "File saved");
      serwerprint("Plik " + filename + " zapisany");
    } else {
      serwer.send(500, "text/plain", "Failed to save");
    }
  } else {
    serwer.send(400, "text/plain", "Missing parameters");
  }
}

void fileMidiAnalyzeHandler() {
  if (serwer.hasArg("filename")) {
    String filename = serwer.arg("filename");
    String filepath = "/" + filename;
    
    if (LittleFS.exists(filepath) && filename.endsWith(".mid")) {
      // TODO: Zaimplementować analizę MIDI
      String json = "{\"message\":\"MIDI analysis not yet implemented\",\"filename\":\"" + filename + "\"}";
      serwer.send(200, "application/json", json);
    } else {
      serwer.send(404, "text/plain", "File not found or invalid format");
    }
  } else {
    serwer.send(400, "text/plain", "Missing filename parameter");
  }
}

void fileConvertHandler() {
  if (serwer.hasArg("filename")) {
    String filename = serwer.arg("filename");
    
    if (filename.endsWith(".mid")) {
      // TODO: Zaimplementować konwersję MIDI na JSON
      String json = "{\"message\":\"MIDI to JSON conversion not yet implemented\",\"filename\":\"" + filename + "\"}";
      serwer.send(200, "application/json", json);
    } else {
      serwer.send(400, "text/plain", "Invalid file format");
    }
  } else {
    serwer.send(400, "text/plain", "Missing filename parameter");
  }
}

void logHandler() {
  if (serwer.hasArg("plain")) {
    String logMessage = serwer.arg("plain");
    serwerprint("[CLIENT] " + logMessage);
    serwer.send(200, "text/plain", "OK");
  } else {
    serwer.send(400, "text/plain", "Missing log message");
  }
}

