#include <Arduino.h>
#include <DHT.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#define DEVICE_ID "IoT-Block-002"

#define LED_PIN 14
#define BUTTON_PIN 12
#define BUZZER_PIN 13
#define DHT_SENSOR_PIN 4
#define PIR_SENSOR_PIN 5
#define BOARD_LED_PIN LED_BUILTIN

const uint16_t EEPROM_SIZE = 256;
const uint32_t BOARD_LED_BLINK_INTERVAL_MS = 700;
const uint32_t REMOTE_PULL_INTERVAL_MS = 500;
const uint32_t REMOTE_PUSH_MIN_INTERVAL_MS = 200;
const uint32_t SENSOR_READ_INTERVAL_MS = 2000;
const uint32_t STATUS_LOG_INTERVAL_MS = 1000;
const uint32_t CONFIG_MAGIC = 0xB10C2026;
const uint8_t AP_PASSWORD_MIN_LEN = 8;

struct DeviceConfig {
  uint32_t magic;
  char ssid[33];
  char password[65];
  char serverUrl[129];
};

DeviceConfig config = {};
ESP8266WebServer server(80);
DHT dht(DHT_SENSOR_PIN, DHT11);
bool ledState = false;
bool buzzerState = false;
bool motionState = false;
char deviceHostname[64] = {0};
uint32_t boardLedLastToggleMs = 0;
bool boardLedOutput = false;
uint32_t lastRemotePullMs = 0;
uint32_t lastRemotePushMs = 0;
bool remoteStateDirty = true;
bool lastObservedLed = false;
bool lastObservedBuzzer = false;
bool lastObservedButton = false;
bool hasRemotePullResult = false;
bool lastRemotePullOk = false;
int lastRemotePullCode = 0;
bool hasRemotePushResult = false;
bool lastRemotePushOk = false;
int lastRemotePushCode = 0;
bool tempValid = false;
float tempC = NAN;
bool humidityValid = false;
float humidityPercent = NAN;
uint32_t lastSensorReadMs = 0;
uint32_t lastStatusLogMs = 0;
bool lastObservedTempValid = false;
float lastObservedTempC = NAN;
bool lastObservedHumidityValid = false;
float lastObservedHumidity = NAN;
bool lastObservedMotion = false;

bool stationConnected();

String triStateJson(bool hasValue, bool value) {
  if (!hasValue) {
    return "null";
  }
  return value ? "true" : "false";
}

String tempJsonValue() {
  if (!tempValid) {
    return "null";
  }
  return String(tempC, 2);
}

String normalizedServerBaseUrl() {
  String base = String(config.serverUrl);
  base.trim();
  while (base.endsWith("/")) {
    base.remove(base.length() - 1);
  }
  return base;
}

bool isRemoteSyncEnabled() {
  if (!stationConnected()) {
    return false;
  }
  return normalizedServerBaseUrl().length() > 0;
}

String remoteDeviceIoUrl() { return normalizedServerBaseUrl() + "/api/" + String(DEVICE_ID); }

String remoteDeviceIoSetUrl() { return remoteDeviceIoUrl() + "/set"; }

bool parseJsonBoolField(const String& json, const char* key, bool& out) {
  String token = "\"" + String(key) + "\"";
  int keyPos = json.indexOf(token);
  if (keyPos < 0) {
    return false;
  }

  int colonPos = json.indexOf(':', keyPos + token.length());
  if (colonPos < 0) {
    return false;
  }

  int valuePos = colonPos + 1;
  while (valuePos < static_cast<int>(json.length()) &&
         (json[valuePos] == ' ' || json[valuePos] == '\t' || json[valuePos] == '\r' || json[valuePos] == '\n')) {
    valuePos++;
  }

  if (valuePos >= static_cast<int>(json.length())) {
    return false;
  }

  String tail = json.substring(valuePos);
  tail.toLowerCase();
  if (tail.startsWith("true") || tail.startsWith("\"true\"") || tail.startsWith("1") || tail.startsWith("\"1\"")) {
    out = true;
    return true;
  }
  if (tail.startsWith("false") || tail.startsWith("\"false\"") || tail.startsWith("0") || tail.startsWith("\"0\"")) {
    out = false;
    return true;
  }
  return false;
}

void applyRemoteIoState(const String& payload) {
  bool value = false;
  bool changed = false;

  if (parseJsonBoolField(payload, "led", value) && ledState != value) {
    ledState = value;
    changed = true;
  }

  if ((parseJsonBoolField(payload, "buzzer", value) || parseJsonBoolField(payload, "buzz", value)) && buzzerState != value) {
    buzzerState = value;
    changed = true;
  }

  if (changed) {
    Serial.println("Remote state applied.");
  }
}

void pullRemoteIoStateIfNeeded() {
  if (!isRemoteSyncEnabled()) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastRemotePullMs < REMOTE_PULL_INTERVAL_MS) {
    return;
  }
  lastRemotePullMs = now;

  WiFiClient wifiClient;
  HTTPClient http;
  String url = remoteDeviceIoUrl();

  if (!http.begin(wifiClient, url)) {
    Serial.println("Remote pull begin failed.");
    hasRemotePullResult = true;
    lastRemotePullOk = false;
    lastRemotePullCode = -1;
    return;
  }

  http.setTimeout(1200);
  int code = http.GET();
  hasRemotePullResult = true;
  lastRemotePullCode = code;
  if (code > 0 && code < 300) {
    lastRemotePullOk = true;
    applyRemoteIoState(http.getString());
  } else {
    lastRemotePullOk = false;
    Serial.printf("Remote pull failed: %d\n", code);
  }
  http.end();
}

int pushRemoteIoStatePost() {
  WiFiClient wifiClient;
  HTTPClient http;
  String url = remoteDeviceIoSetUrl();
  if (!http.begin(wifiClient, url)) {
    return -1;
  }

  http.setTimeout(1200);
  http.addHeader("Content-Type", "application/json");

  const bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
  String body = "{";
  body += "\"led\":" + String(ledState ? "true" : "false") + ",";
  body += "\"buzzer\":" + String(buzzerState ? "true" : "false") + ",";
  body += "\"button\":" + String(buttonPressed ? "true" : "false") + ",";
  body += "\"motion\":" + String(motionState ? "true" : "false") + ",";
  body += "\"tempC\":" + tempJsonValue() + ",";
  body += "\"humidity\":" + (humidityValid ? String(humidityPercent, 1) : String("null"));
  body += "}";

  int code = http.POST(body);
  if (!(code > 0 && code < 300)) {
    Serial.printf("Remote POST set failed: %d\n", code);
  }
  http.end();
  return code;
}

int pushRemoteIoStateGet() {
  WiFiClient wifiClient;
  HTTPClient http;
  String url = remoteDeviceIoSetUrl();
  url += "?led=" + String(ledState ? "1" : "0");
  url += "&buzz=" + String(buzzerState ? "1" : "0");
  url += "&button=" + String(digitalRead(BUTTON_PIN) == LOW ? "1" : "0");
  url += "&motion=" + String(motionState ? "1" : "0");
  if (tempValid) {
    url += "&tempC=" + String(tempC, 2);
  }
  if (humidityValid) {
    url += "&humidity=" + String(humidityPercent, 1);
  }

  if (!http.begin(wifiClient, url)) {
    return -1;
  }

  http.setTimeout(1200);
  int code = http.GET();
  if (!(code > 0 && code < 300)) {
    Serial.printf("Remote GET set failed: %d\n", code);
  }
  http.end();
  return code;
}

void syncRemoteIoStateIfNeeded() {
  if (!isRemoteSyncEnabled()) {
    return;
  }

  const bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
  const bool humidityChanged = humidityValid != lastObservedHumidityValid ||
                               (humidityValid && fabsf(humidityPercent - lastObservedHumidity) >= 1.0f);
  bool tempChanged = false;
  if (tempValid != lastObservedTempValid) {
    tempChanged = true;
  } else if (tempValid && fabsf(tempC - lastObservedTempC) >= 0.05f) {
    tempChanged = true;
  }

  if (ledState != lastObservedLed || buzzerState != lastObservedBuzzer || buttonPressed != lastObservedButton ||
      motionState != lastObservedMotion || tempChanged || humidityChanged) {
    remoteStateDirty = true;
    lastObservedLed = ledState;
    lastObservedBuzzer = buzzerState;
    lastObservedButton = buttonPressed;
    lastObservedTempValid = tempValid;
    lastObservedTempC = tempC;
    lastObservedHumidityValid = humidityValid;
    lastObservedHumidity = humidityPercent;
    lastObservedMotion = motionState;
  }

  const uint32_t now = millis();
  if (!remoteStateDirty || now - lastRemotePushMs < REMOTE_PUSH_MIN_INTERVAL_MS) {
    return;
  }

  lastRemotePushMs = now;
  int code = pushRemoteIoStatePost();
  bool ok = code > 0 && code < 300;
  if (!ok) {
    code = pushRemoteIoStateGet();
    ok = code > 0 && code < 300;
  }

  hasRemotePushResult = true;
  lastRemotePushOk = ok;
  lastRemotePushCode = code;

  if (ok) {
    remoteStateDirty = false;
  }
}

void updateSensorsIfNeeded() {
  const uint32_t now = millis();
  motionState = digitalRead(PIR_SENSOR_PIN) == HIGH;
  if (now - lastSensorReadMs < SENSOR_READ_INTERVAL_MS) {
    return;
  }

  lastSensorReadMs = now;
  const float measuredTemp = dht.readTemperature();
  const float measuredHumidity = dht.readHumidity();
  tempValid = isfinite(measuredTemp);
  humidityValid = isfinite(measuredHumidity);
  if (tempValid) {
    tempC = measuredTemp;
  }
  if (humidityValid) {
    humidityPercent = measuredHumidity;
  }
}

void logIoStatusIfNeeded() {
  const uint32_t now = millis();
  if (now - lastStatusLogMs < STATUS_LOG_INTERVAL_MS) {
    return;
  }
  lastStatusLogMs = now;

  Serial.printf("[status] pirPin=%s motion=%s button=%s led=%s buzzer=%s tempC=",
                digitalRead(PIR_SENSOR_PIN) == HIGH ? "HIGH" : "LOW", motionState ? "DETECTED" : "CLEAR",
                digitalRead(BUTTON_PIN) == LOW ? "PRESSED" : "RELEASED", ledState ? "ON" : "OFF",
                buzzerState ? "ON" : "OFF");
  if (tempValid) {
    Serial.printf("%.1f", tempC);
  } else {
    Serial.print("INVALID");
  }
  Serial.print(" humidity=");
  if (humidityValid) {
    Serial.printf("%.1f%%", humidityPercent);
  } else {
    Serial.print("INVALID");
  }
  Serial.println();
}

void buildDeviceHostname() {
  size_t outIdx = 0;
  for (size_t i = 0; DEVICE_ID[i] != '\0' && outIdx < sizeof(deviceHostname) - 1; i++) {
    char c = DEVICE_ID[i];
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c + ('a' - 'A'));
    }
    bool isAlphaNum = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
    if (isAlphaNum || c == '-') {
      deviceHostname[outIdx++] = c;
    }
  }

  if (outIdx == 0) {
    strncpy(deviceHostname, "iot-block", sizeof(deviceHostname) - 1);
  }
  deviceHostname[sizeof(deviceHostname) - 1] = '\0';
}

void setupMdnsIfConnected() {
  if (!stationConnected()) {
    Serial.println("mDNS skipped: station not connected.");
    return;
  }

  if (!MDNS.begin(deviceHostname)) {
    Serial.println("mDNS start failed.");
    return;
  }

  MDNS.addService("http", "tcp", 80);
  MDNS.addServiceTxt("http", "tcp", "deviceId", DEVICE_ID);
  MDNS.addServiceTxt("http", "tcp", "type", "iot-block");
  Serial.printf("mDNS ready: http://%s.local\n", deviceHostname);
}

bool parseBoolArg(const String& value, bool& out) {
  if (value == "1" || value == "true" || value == "on" || value == "HIGH") {
    out = true;
    return true;
  }
  if (value == "0" || value == "false" || value == "off" || value == "LOW") {
    out = false;
    return true;
  }
  return false;
}

bool stationConnected() { return WiFi.status() == WL_CONNECTED; }

void sendJson(int code, const String& payload) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", payload);
}

void safeCopy(char* dest, size_t destSize, const String& src) {
  if (destSize == 0) {
    return;
  }
  strncpy(dest, src.c_str(), destSize - 1);
  dest[destSize - 1] = '\0';
}

String htmlEscape(const char* text) {
  String s = text;
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

bool loadConfigFromEeprom() {
  EEPROM.get(0, config);
  if (config.magic != CONFIG_MAGIC) {
    memset(&config, 0, sizeof(config));
    return false;
  }

  config.ssid[sizeof(config.ssid) - 1] = '\0';
  config.password[sizeof(config.password) - 1] = '\0';
  config.serverUrl[sizeof(config.serverUrl) - 1] = '\0';
  return true;
}

bool saveConfigToEeprom() {
  config.magic = CONFIG_MAGIC;
  EEPROM.put(0, config);
  return EEPROM.commit();
}

void printConfig() {
  Serial.println("=== Restored config ===");
  Serial.printf("SSID: %s\n", config.ssid[0] ? config.ssid : "<empty>");
  Serial.printf("Password: %s\n", config.password[0] ? config.password : "<empty>");
  Serial.printf("Server URL: %s\n", config.serverUrl[0] ? config.serverUrl : "<empty>");
  Serial.println("=======================");
}

String buildConfigPage(const String& message = "") {
  String html;
  html.reserve(3400);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>IoT Block Setup</title><style>");
  html += F(":root{--bg:#f3f6fb;--card:#fff;--ink:#10243d;--muted:#5d6e82;--accent:#0b84f3;--accent2:#0a66c2;}");
  html += F("*{box-sizing:border-box}body{margin:0;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;");
  html += F("background:radial-gradient(circle at 10% 10%,#dbeafe,transparent 45%),var(--bg);color:var(--ink);}");
  html += F(".wrap{min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}");
  html += F(".card{width:min(100%,560px);background:var(--card);border-radius:16px;padding:20px;box-shadow:0 14px 40px rgba(0,0,0,.12)}");
  html += F("h1{margin:0 0 6px;font-size:1.35rem}p{margin:0 0 18px;color:var(--muted)}");
  html +=
      F("label{display:block;margin:12px 0 6px;font-weight:600}input{width:100%;padding:12px;border:1px solid "
        "#d1d7e0;border-radius:10px;font-size:1rem}");
  html += F(".password-wrap{position:relative}");
  html += F(".password-wrap input{padding-right:76px}");
  html += F(".toggle-password{position:absolute;right:8px;top:50%;transform:translateY(-50%);margin:0;padding:6px 10px;width:auto;");
  html += F("border:1px solid #c8d2df;border-radius:8px;background:#fff;color:var(--ink);font-weight:600;cursor:pointer}");
  html += F("input:focus{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px rgba(11,132,243,.15)}");
  html += F(".toggle-password:focus{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px rgba(11,132,243,.15)}");
  html +=
      F(".submit-btn{margin-top:16px;width:100%;padding:12px;border:0;border-radius:10px;background:linear-gradient(90deg,var(--accent),"
        "var(--"
        "accent2));color:#fff;font-weight:700;cursor:pointer}");
  html +=
      F(".msg{margin:0 0 14px;padding:10px 12px;background:#e8f4ff;color:#0f4b7c;border-left:4px solid var(--accent);border-radius:8px}");
  html += F("@media (max-width:480px){.card{padding:16px}h1{font-size:1.15rem}}</style></head><body>");
  html += F("<div class='wrap'><div class='card'><h1>IoT Block Setup</h1><p>Save router and server settings to EEPROM.</p>");
  if (message.length() > 0) {
    html += "<div class='msg'>" + message + "</div>";
  }
  html += F("<form method='POST' action='/save'>");
  html += F("<label for='ssid'>Router SSID</label><input id='ssid' name='ssid' maxlength='32' required value='");
  html += htmlEscape(config.ssid);
  html += F("'>");
  html +=
      F("<label for='password'>Router Password</label><div class='password-wrap'><input id='password' name='password' type='password' "
        "maxlength='64' value='");
  html += htmlEscape(config.password);
  html += F("'><button type='button' id='togglePassword' class='toggle-password' aria-label='Show password'>Show</button></div>");
  html +=
      F("<label for='serverUrl'>Server Host URL</label><input id='serverUrl' name='serverUrl' maxlength='128' "
        "placeholder='https://api.example.com' value='");
  html += htmlEscape(config.serverUrl);
  html += F("'>");
  html += F("<button type='submit' class='submit-btn'>Save Settings</button></form></div></div>");
  html += F("<script>(function(){var input=document.getElementById('password');var btn=document.getElementById('togglePassword');");
  html +=
      F("if(!input||!btn){return;}btn.addEventListener('click',function(){var "
        "show=input.type==='password';input.type=show?'text':'password';");
  html += F("btn.textContent=show?'Hide':'Show';btn.setAttribute('aria-label',show?'Hide password':'Show password');});})();</script>");
  html += F("</body></html>");
  return html;
}

void handleRoot() { server.send(200, "text/html", buildConfigPage()); }

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String serverUrl = server.arg("serverUrl");

  ssid.trim();
  password.trim();
  serverUrl.trim();

  if (ssid.length() == 0) {
    server.send(400, "text/html", buildConfigPage("SSID is required."));
    return;
  }

  safeCopy(config.ssid, sizeof(config.ssid), ssid);
  safeCopy(config.password, sizeof(config.password), password);
  safeCopy(config.serverUrl, sizeof(config.serverUrl), serverUrl);

  if (!saveConfigToEeprom()) {
    server.send(500, "text/html", buildConfigPage("Failed to save to EEPROM."));
    return;
  }

  printConfig();
  server.send(200, "text/html", buildConfigPage("Saved. Reboot device to apply Wi-Fi connection changes."));
}

void handleApiStatus() {
  if (!stationConnected()) {
    sendJson(503, "{\"error\":\"station_not_connected\"}");
    return;
  }

  const bool buttonPressed = digitalRead(BUTTON_PIN) == LOW;
  String payload = "{";
  payload += "\"stationConnected\":" + String(stationConnected() ? "true" : "false") + ",";
  payload += "\"staIp\":\"" + WiFi.localIP().toString() + "\",";
  payload += "\"led\":" + String(ledState ? "true" : "false") + ",";
  payload += "\"buzzer\":" + String(buzzerState ? "true" : "false") + ",";
  payload += "\"button\":" + String(buttonPressed ? "true" : "false") + ",";
  payload += "\"motion\":" + String(motionState ? "true" : "false") + ",";
  payload += "\"tempC\":" + tempJsonValue() + ",";
  payload += "\"humidity\":" + (humidityValid ? String(humidityPercent, 1) : String("null"));
  payload += "}";

  sendJson(200, payload);
}

void handleApiSetIo() {
  if (!stationConnected()) {
    sendJson(503, "{\"error\":\"station_not_connected\"}");
    return;
  }

  bool hasLedArg = server.hasArg("led");
  bool hasBuzzArg = server.hasArg("buzz");
  if (!hasLedArg && !hasBuzzArg) {
    sendJson(400, "{\"error\":\"missing_args\",\"hint\":\"use led and/or buzz\"}");
    return;
  }

  if (hasLedArg) {
    bool parsedLed = false;
    if (!parseBoolArg(server.arg("led"), parsedLed)) {
      sendJson(400, "{\"error\":\"invalid_led\",\"accepted\":[\"1\",\"0\",\"true\",\"false\"]}");
      return;
    }
    ledState = parsedLed;
  }

  if (hasBuzzArg) {
    bool parsedBuzz = false;
    if (!parseBoolArg(server.arg("buzz"), parsedBuzz)) {
      sendJson(400, "{\"error\":\"invalid_buzz\",\"accepted\":[\"1\",\"0\",\"true\",\"false\"]}");
      return;
    }
    buzzerState = parsedBuzz;
  }

  remoteStateDirty = true;

  String payload = "{";
  payload += "\"ok\":true,";
  payload += "\"led\":" + String(ledState ? "true" : "false") + ",";
  payload += "\"buzzer\":" + String(buzzerState ? "true" : "false") + ",";
  payload += "\"tempC\":" + tempJsonValue() + ",";
  payload += "\"humidity\":" + (humidityValid ? String(humidityPercent, 1) : String("null")) + ",";
  payload += "\"motion\":" + String(motionState ? "true" : "false");
  payload += "}";
  sendJson(200, payload);
}

void handleApiDiscovery() {
  String payload = "{";
  payload += "\"deviceId\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"hostname\":\"" + String(deviceHostname) + "\",";
  payload += "\"mdns\":\"" + String(deviceHostname) + ".local\",";
  payload += "\"stationConnected\":" + String(stationConnected() ? "true" : "false") + ",";
  payload += "\"staIp\":\"" + (stationConnected() ? WiFi.localIP().toString() : String("")) + "\",";
  payload += "\"motion\":" + String(motionState ? "true" : "false") + ",";
  payload += "\"tempC\":" + tempJsonValue() + ",";
  payload += "\"humidity\":" + (humidityValid ? String(humidityPercent, 1) : String("null")) + ",";
  payload += "\"remoteSyncEnabled\":" + String(isRemoteSyncEnabled() ? "true" : "false") + ",";
  payload += "\"remotePullOk\":" + triStateJson(hasRemotePullResult, lastRemotePullOk) + ",";
  payload += "\"remotePullCode\":" + String(lastRemotePullCode) + ",";
  payload += "\"remotePushOk\":" + triStateJson(hasRemotePushResult, lastRemotePushOk) + ",";
  payload += "\"remotePushCode\":" + String(lastRemotePushCode);
  payload += "}";
  sendJson(200, payload);
}

void setupConfigServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/api/discovery", HTTP_GET, handleApiDiscovery);
  server.on("/api/io", HTTP_GET, handleApiStatus);
  server.on("/api/io/set", HTTP_GET, handleApiSetIo);
  server.on("/api/io/set", HTTP_POST, handleApiSetIo);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.println("Config server started at AP address: 192.168.4.1");
}

void connectStationIfConfigured() {
  if (strlen(config.ssid) == 0) {
    Serial.println("No saved SSID. Station connect skipped.");
    return;
  }

  WiFi.begin(config.ssid, config.password);
  Serial.printf("Connecting to SSID '%s'", config.ssid);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to saved Wi-Fi.");
  }
}

void updateBoardLed() {
  if (!WiFi.isConnected()) {
    boardLedOutput = false;
    digitalWrite(BOARD_LED_PIN, HIGH);
    return;
  }

  const uint32_t now = millis();
  if (now - boardLedLastToggleMs >= BOARD_LED_BLINK_INTERVAL_MS) {
    boardLedLastToggleMs = now;
    boardLedOutput = !boardLedOutput;
    digitalWrite(BOARD_LED_PIN, boardLedOutput ? LOW : HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  bool restored = loadConfigFromEeprom();
  Serial.println(restored ? "Config restored from EEPROM." : "No valid EEPROM config. Using defaults.");
  printConfig();
  buildDeviceHostname();

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP(DEVICE_ID, "12345678");
  Serial.print("AP started. SSID: ");
  Serial.println(DEVICE_ID);

  connectStationIfConfigured();
  setupMdnsIfConnected();
  setupConfigServer();

  dht.begin();

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(BOARD_LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
  digitalWrite(BOARD_LED_PIN, HIGH);
  boardLedLastToggleMs = millis();
}

void loop() {
  server.handleClient();
  MDNS.update();
  updateSensorsIfNeeded();
  pullRemoteIoStateIfNeeded();
  syncRemoteIoStateIfNeeded();

  updateBoardLed();
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
  logIoStatusIfNeeded();
}
