#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"

const char ssid1[] = "Hello";
const char pass1[] = "11223344";
const char ssid2[] = "GameOn";
const char pass2[] = "asdf112233asdf";
const char ssid3[] = "Info_Sys";
const char pass3[] = "cselab@423";
const char host[] = "http://esinebd.com/projects/IoTBlocks/index.php";

const char user[] = "antor.mee@gmail.com";
const char pass[] = "123456";
const char deviceType[] = "temp";

#define pin D5

ESP8266WiFiMulti wifiMulti;
WiFiClient client;
HTTPClient http;
DHT dht(pin, DHT11);

int i;
String link;
float temp;

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);

  dht.begin();

  wifiMulti.addAP(ssid1, pass1);
  wifiMulti.addAP(ssid2, pass2);
  wifiMulti.addAP(ssid3, pass3);

  wifiMulti.run();
}

void loop() {
  float hum = dht.readHumidity();
  float tmp = dht.readTemperature();

  if (temp != tmp) {
    temp = tmp;
    link = (String)host + "?set&email=" + user;
    link += (String)"&pass=" + pass;
    link += (String)"&deviceId=" + deviceType;
    link += (String)"&state=" + temp;

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
