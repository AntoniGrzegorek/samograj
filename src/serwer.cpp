#include "serwer.h"

//inicjalizacja WiFi
bool setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(1000);
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

//inicjalizacja endpointów
void endpoint_init() {
  serwer.on("/", rootHandler);
  serwer.begin();
  serwerprint("Serwer HTTP uruchomiony.");
}

//add entry to debug log
void serwerprint(String message) {
  debuglog += message + "\n";
}

String debug_message(String message) {
  //debuglog += message + "\n";
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

