#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

const char ssid1[] = "Hello";
const char pass1[] = "11223344";
const char ssid2[] = "GameOn";
const char pass2[] = "asdf112233asdf";
const char ssid3[] = "Info_Sys";
const char pass3[] = "cselab@423";
const char host[] = "http://esinebd.com/projects/IoTBlocks/index.php";

const char user[] = "antor.mee@gmail.com";
const char pass[] = "123456";
const char deviceType[] = "button";

#define pin D5

ESP8266WiFiMulti wifiMulti;
WiFiClient client;
HTTPClient http;

String link;
int i, oldState;

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);

  pinMode(pin, INPUT);

  wifiMulti.addAP(ssid1, pass1);
  wifiMulti.addAP(ssid2, pass2);
  wifiMulti.addAP(ssid3, pass3);

  wifiMulti.run();
}

void loop() {
  int state = digitalRead(pin);

  if (oldState != state) {
    oldState = state;
    link = (String)host + "?set&email=" + user;
    link += (String)"&pass=" + pass;
    link += (String)"&deviceId=" + deviceType;
    link += (String)"&state=" + state;

    if (http.begin(client, link)) {
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String resp = http.getString();
        Serial.println(resp);
      }
    }
  }

  delay(100);
}
