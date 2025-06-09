#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

const char* ssid = "your_ssid";     // replace with your WiFi SSID
const char* password = "your_password"; // replace with your WiFi password

const int relayPin = 5; // GPIO5 (D1 label) for the relay
const int MAX_TIMES = 8; // Max number of whistle times

struct WhistleTime {
  int hour;
  int minute;
};

WhistleTime whistleTimes[MAX_TIMES];
int numTimes = 0;
int blast1Duration = 500; // first blast length (ms)
int blast2Duration = 1500; // second blast length (ms)
const int blastPause = 200; // pause between blasts (ms)

WebServer server(80);

void handleRoot();
void handleConfig();
void handleTest();
void checkWhistle();
void triggerWhistle();

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println(" connected");

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print('.');
    now = time(nullptr);
  }
  Serial.println(" time synced");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/test", HTTP_POST, handleTest);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  checkWhistle();
}

void handleRoot() {
  String page = "<html><head><style>";
  page += "body{font-family:sans-serif;margin:20px;}";
  page += "form{margin-bottom:1em;}";
  page += "input,button{padding:4px;margin:4px 0;}";
  page += "</style></head><body><h1>Lunch Whistle Timer</h1>";
  page += "<form method='POST' action='/config'>";
  page += "Times (HH:MM, comma separated):<br/>";
  page += "<input type='text' name='times'/><br/>";
  page += "First blast ms:<br/>";
  page += "<input type='number' name='blast1' value='" + String(blast1Duration) + "'/><br/>";
  page += "Second blast ms:<br/>";
  page += "<input type='number' name='blast2' value='" + String(blast2Duration) + "'/><br/>";
  page += "<input type='submit' value='Set'/></form>";
  page += "<form method='POST' action='/test'>";
  page += "<button type='submit'>Test Whistle</button>";
  page += "</form>";
  page += "<h2>Current Times</h2><ul>";
  for (int i=0;i<numTimes;i++) {
    page += "<li>" + String(whistleTimes[i].hour) + ":" + (whistleTimes[i].minute<10?"0":"") + String(whistleTimes[i].minute) + "</li>";
  }
  page += "</ul></body></html>";
  server.send(200, "text/html", page);
}

void handleConfig() {
  if (!server.hasArg("times")) {
    server.send(400, "text/plain", "Missing times parameter");
    return;
  }
  String timesArg = server.arg("times");
  if (server.hasArg("blast1")) {
    blast1Duration = server.arg("blast1").toInt();
  }
  if (server.hasArg("blast2")) {
    blast2Duration = server.arg("blast2").toInt();
  }
  numTimes = 0;
  int last = 0;
  while (last < timesArg.length() && numTimes < MAX_TIMES) {
    int comma = timesArg.indexOf(',', last);
    if (comma == -1) comma = timesArg.length();
    String pair = timesArg.substring(last, comma);
    int colon = pair.indexOf(':');
    if (colon > 0) {
      int h = pair.substring(0, colon).toInt();
      int m = pair.substring(colon + 1).toInt();
      if (h >= 0 && h < 24 && m >= 0 && m < 60) {
        whistleTimes[numTimes].hour = h;
        whistleTimes[numTimes].minute = m;
        numTimes++;
      }
    }
    last = comma + 1;
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleTest() {
  triggerWhistle();
  server.sendHeader("Location", "/");
  server.send(303);
}

void triggerWhistle() {
  digitalWrite(relayPin, HIGH);
  delay(blast1Duration);
  digitalWrite(relayPin, LOW);
  delay(blastPause);
  digitalWrite(relayPin, HIGH);
  delay(blast2Duration);
  digitalWrite(relayPin, LOW);
}

void checkWhistle() {
  static time_t lastCheck = 0;
  time_t now = time(nullptr);
  if (now == lastCheck) return;
  lastCheck = now;

  struct tm* timeinfo = localtime(&now);
  int currentHour = timeinfo->tm_hour;
  int currentMinute = timeinfo->tm_min;
  int currentSecond = timeinfo->tm_sec;

  for (int i=0; i<numTimes; i++) {
    if (whistleTimes[i].hour == currentHour && whistleTimes[i].minute == currentMinute && currentSecond == 0) {
      triggerWhistle();
    }
  }
}

