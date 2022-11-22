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
const char deviceType[] = "led";

#define pin D5

ESP8266WiFiMulti wifiMulti;
WiFiClient client;
HTTPClient http;

String link;
int i, oldState;

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);

  pinMode(pin, OUTPUT);

  wifiMulti.addAP(ssid1, pass1);
  wifiMulti.addAP(ssid2, pass2);
  wifiMulti.addAP(ssid3, pass3);

  wifiMulti.run();
}

void loop() {
  link = (String)host + "?get&email=" + user;
  link += (String)"&pass=" + pass;
  link += (String)"&deviceId=" + deviceType;

  if (http.begin(client, link)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String resp = http.getString();
      i = resp.indexOf("\"state\":\"");
      resp.remove(0, i + 9);
      int state = resp.toInt();

      if (oldState != state) {
        oldState = state;
        Serial.println(state);
        digitalWrite(pin, state);
      }
    }
  }

  delay(100);
}
