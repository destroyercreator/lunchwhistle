#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>

const char* ssid = "Sixpenny _EXT 2.4";     // replace with your WiFi SSID
const char* password = "88888888"; // replace with your WiFi password

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
int blastPause = 200; // pause between blasts (ms)

Preferences prefs;

WebServer server(80);

void handleRoot();
void handleConfig();
void handleTest();
void handleTime();

void checkWhistle();
void triggerWhistle();
void parseTimes(const String& timesArg);

void setup() {
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  Serial.begin(115200);
  prefs.begin("whistle", false);
  String stored = prefs.getString("times", "");
  if (stored.length() > 0) {
    parseTimes(stored);
  }
  blast1Duration = prefs.getInt("blast1", blast1Duration);
  blast2Duration = prefs.getInt("blast2", blast2Duration);
  blastPause = prefs.getInt("pause", blastPause);


  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println(" connected");

  configTzTime("EST5EDT,M3.2.0/2,M11.1.0/2", "pool.ntp.org", "time.nist.gov");

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

  server.on("/time", HTTP_GET, handleTime);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Device IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  checkWhistle();
}

void handleRoot() {
  String page = "<html><head><style>";
  page += "body{font-family:sans-serif;background:#f7f7f7;";
  page += "display:flex;justify-content:center;align-items:center;";
  page += "height:100vh;margin:0;}";
  page += ".card{background:white;padding:20px;border-radius:8px;";
  page += "box-shadow:0 2px 4px rgba(0,0,0,0.1);width:300px;text-align:center;}";
  page += "form{margin-bottom:1em;}";
  page += "input,button{padding:6px;margin:6px 0;width:100%;}";

  page += "#clock{font-size:1.2em;margin-top:10px;}";

  page += "</style></head><body><div class='card'><h1>Lunch Whistle Timer</h1>";
  page += "<form method='POST' action='/config'>";
  page += "Times (HH:MM, comma separated):<br/>";
  String timesStr = "";
  for (int i=0;i<numTimes;i++) {
    if (i) timesStr += ",";
    timesStr += String(whistleTimes[i].hour) + ":" + (whistleTimes[i].minute<10?"0":"") + String(whistleTimes[i].minute);
  }
  page += "<input type='text' name='times' value='" + timesStr + "'/><br/>";
  page += "First blast ms:<br/>";
  page += "<input type='number' name='blast1' value='" + String(blast1Duration) + "'/><br/>";
  page += "Second blast ms:<br/>";
  page += "<input type='number' name='blast2' value='" + String(blast2Duration) + "'/><br/>";
  page += "Pause ms:<br/>";
  page += "<input type='number' name='pause' value='" + String(blastPause) + "'/><br/>";

  page += "<input type='submit' value='Set'/></form>";
  page += "<form method='POST' action='/test'>";
  page += "<button type='submit'>Test Whistle</button>";
  page += "</form>";

  page += "<div id='clock'></div>";

  page += "<h2>Current Times</h2><ul>";
  for (int i=0;i<numTimes;i++) {
    page += "<li>" + String(whistleTimes[i].hour) + ":" + (whistleTimes[i].minute<10?"0":"") + String(whistleTimes[i].minute) + "</li>";
  }

  page += "</ul><script>async function u(){let r=await fetch('/time');let t=parseInt(await r.text());document.getElementById('clock').innerText=new Date(t*1000).toLocaleTimeString();}u();setInterval(u,1000);</script></div></body></html>";

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
  if (server.hasArg("pause")) {
    blastPause = server.arg("pause").toInt();
  }

  parseTimes(timesArg);

  prefs.putString("times", timesArg);
  prefs.putInt("blast1", blast1Duration);
  prefs.putInt("blast2", blast2Duration);
  prefs.putInt("pause", blastPause);

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleTest() {
  triggerWhistle();
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleTime() {
  time_t now = time(nullptr);
  server.send(200, "text/plain", String((unsigned long)now));
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

void parseTimes(const String& timesArg) {
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
}

