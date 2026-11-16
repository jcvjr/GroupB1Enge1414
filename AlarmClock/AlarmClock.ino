// Alarm clock controlled from a web page hosted by the ESP32 (WiFi).
// Open the Serial Monitor at 115200 baud to see the IP address, then visit it from any device on the same network.

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>

#include "secrets.h"     // copy secrets.h.example -> secrets.h
#include "index_html.h"

// ---- Hardware (change to match your wiring) ----
const int BUZZER_PIN = 25;   // active buzzer, + side to this pin
const int LED_PIN    = 2;    // onboard LED

// ---- Time ----
// POSIX timezone string; this one is US Eastern (Virginia Tech). Other zones: https://github.com/nayarsystems/posix_tz_db
const char *TZ_INFO    = "EST5EDT,M3.2.0,M11.1.0";
const char *NTP_SERVER = "pool.ntp.org";

struct AlarmConfig {
  bool enabled   = false;
  int  hour      = 7;
  int  minute    = 0;
  int  snoozeMin = 5;
} cfg;

enum State { IDLE, RINGING, SNOOZED };
State state = IDLE;
time_t snoozeUntil = 0;
int lastFiredDay = -1;           // yday the alarm last fired, so it only rings once per day
unsigned long testUntilMs = 0;   // buzzer test, millis() deadline

WebServer server(80);
Preferences prefs;

// ---------- helpers ----------
bool timeValid() { return time(nullptr) > 1700000000; }

String fmtTime(time_t t, bool seconds) {
  struct tm tm;
  localtime_r(&t, &tm);
  char buf[16];
  strftime(buf, sizeof buf, seconds ? "%H:%M:%S" : "%H:%M", &tm);
  return buf;
}

void loadConfig() {
  prefs.begin("alarm", true);
  cfg.enabled   = prefs.getBool("on", false);
  cfg.hour      = prefs.getInt("h", 7);
  cfg.minute    = prefs.getInt("m", 0);
  cfg.snoozeMin = prefs.getInt("snz", 5);
  prefs.end();
}

void saveConfig() {
  prefs.begin("alarm", false);
  prefs.putBool("on", cfg.enabled);
  prefs.putInt("h", cfg.hour);
  prefs.putInt("m", cfg.minute);
  prefs.putInt("snz", cfg.snoozeMin);
  prefs.end();
}

// ---------- HTTP handlers ----------
void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleStatus() {
  time_t now = time(nullptr);
  char alarm[6];
  snprintf(alarm, sizeof alarm, "%02d:%02d", cfg.hour, cfg.minute);
  String json = "{\"now\":\"" + (timeValid() ? fmtTime(now, true) : String("--:--:--")) + "\"";
  json += ",\"alarm\":\"" + String(alarm) + "\"";
  json += ",\"enabled\":" + String(cfg.enabled ? "true" : "false");
  json += ",\"ringing\":" + String(state == RINGING ? "true" : "false");
  json += ",\"snoozed\":" + String(state == SNOOZED ? "true" : "false");
  json += ",\"snoozeUntil\":\"" + (state == SNOOZED ? fmtTime(snoozeUntil, false) : String("")) + "\"";
  json += ",\"snoozeMin\":" + String(cfg.snoozeMin) + "}";
  server.send(200, "application/json", json);
}

void handleAlarm() {
  int h, m;
  if (!server.hasArg("time") || sscanf(server.arg("time").c_str(), "%d:%d", &h, &m) != 2 ||
      h < 0 || h > 23 || m < 0 || m > 59) {
    server.send(400, "text/plain", "time must be HH:MM");
    return;
  }
  cfg.hour = h;
  cfg.minute = m;
  cfg.enabled = server.arg("enabled") == "1";
  cfg.snoozeMin = constrain(server.arg("snooze").toInt(), 1, 60);
  state = IDLE;          // changing the alarm cancels any pending snooze
  lastFiredDay = -1;
  saveConfig();
  server.send(200, "text/plain", "ok");
}

void snoozeAlarm() {
  if (state != RINGING) return;
  state = SNOOZED;
  snoozeUntil = time(nullptr) + cfg.snoozeMin * 60;
}

void handleSnooze() { snoozeAlarm(); server.send(200, "text/plain", "ok"); }
void handleStop()   { state = IDLE;  server.send(200, "text/plain", "ok"); }
void handleTest()   { testUntilMs = millis() + 500; server.send(200, "text/plain", "ok"); }

// ---------- alarm logic ----------
void checkAlarm() {
  if (!timeValid()) return;
  time_t now = time(nullptr);
  struct tm tm;
  localtime_r(&now, &tm);

  if (state == IDLE && cfg.enabled && tm.tm_hour == cfg.hour && tm.tm_min == cfg.minute && tm.tm_yday != lastFiredDay) {
    lastFiredDay = tm.tm_yday;
    state = RINGING;
  } else if (state == SNOOZED && now >= snoozeUntil) {
    state = RINGING;
  }
}

void updateOutputs() {
  bool on = false;
  if (state == RINGING) on = (millis() / 400) % 2 == 0;       // beep-beep pattern
  if ((long)(testUntilMs - millis()) > 0) on = true;
  digitalWrite(BUZZER_PIN, on);
  digitalWrite(LED_PIN, state == RINGING ? on : (state == SNOOZED));
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  loadConfig();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.printf("\nConnected. Open http://%s/\n", WiFi.localIP().toString().c_str());

  configTzTime(TZ_INFO, NTP_SERVER);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/alarm", HTTP_POST, handleAlarm);
  server.on("/api/snooze", HTTP_POST, handleSnooze);
  server.on("/api/stop", HTTP_POST, handleStop);
  server.on("/api/test", HTTP_POST, handleTest);
  server.begin();
}

void loop() {
  server.handleClient();
  checkAlarm();
  updateOutputs();
}
